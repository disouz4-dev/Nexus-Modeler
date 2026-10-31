// Abre o plugin compilado como um programa de gravação faria e confere que
// ele carrega um pacote, gira os botões e devolve som válido em várias taxas.
//
// Uso: nexus_host_test <caminho do plugin .vst3/.component> <pasta do pacote> [foto_da_tela.png]

#include <cmath>
#include <cstdio>

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_events/juce_events.h>

#include "nexus/TestSignal.h"

namespace
{
int gFailures = 0;
juce::String gSnapshot;

juce::String u8(const char* text)
{
  return juce::String::fromUTF8(text);
}

void report(bool ok, const juce::String& what, const juce::String& detail)
{
  std::printf("  [%s] %s\n         %s\n", ok ? " OK  " : "FALHA", what.toRawUTF8(), detail.toRawUTF8());
  if (!ok)
    ++gFailures;
}

juce::AudioProcessorParameter* findParam(juce::AudioPluginInstance& p, const juce::String& id)
{
  for (auto* param : p.getParameters())
    if (auto* withId = dynamic_cast<juce::HostedAudioProcessorParameter*>(param))
      if (withId->getParameterID() == id)
        return param;
  for (auto* param : p.getParameters())
    if (param->getName(64).equalsIgnoreCase(id))
      return param;
  return nullptr;
}

struct RunResult
{
  bool engineActive = false;
  bool finite = true;
  double rmsDb = -300.0;
  double peak = 0.0;
};

RunResult run(juce::AudioPluginInstance& plugin, double sampleRate, const juce::String& packFolder)
{
  const int block = 128;
  plugin.setPlayConfigDetails(2, 2, sampleRate, block);
  plugin.prepareToPlay(sampleRate, block);

  // Põe a pasta do pacote no estado salvo, como um projeto reaberto. No VST3
  // o estado do plugin vai dentro de <IComponent>, em base64.
  juce::MemoryBlock state;
  plugin.getStateInformation(state);
  if (auto host = juce::AudioProcessor::getXmlFromBinary(state.getData(), static_cast<int>(state.getSize())))
    if (auto* component = host->getChildByName("IComponent"))
    {
      juce::MemoryBlock chunk;
      chunk.fromBase64Encoding(component->getAllSubText());
      if (auto xml = juce::AudioProcessor::getXmlFromBinary(chunk.getData(), static_cast<int>(chunk.getSize())))
      {
        xml->setAttribute("pasta", packFolder);
        // Sessão nova começa sem cabeçote: o projeto reaberto tem um, que toca o pacote.
        xml->setAttribute("cabecote_id", "recto_dual");
        juce::MemoryBlock newChunk;
        juce::AudioProcessor::copyXmlToBinary(*xml, newChunk);
        component->deleteAllTextElements();
        component->addTextElement(newChunk.toBase64Encoding());
        juce::MemoryBlock newState;
        juce::AudioProcessor::copyXmlToBinary(*host, newState);
        plugin.setStateInformation(newState.getData(), static_cast<int>(newState.getSize()));
      }
    }

  // Mesmo sinal em todas as taxas: gerado a 48 kHz e convertido.
  const auto base = nexus::makeGuitarLikeSignal(48000.0, 6.0, 5);
  std::vector<double> signal(static_cast<size_t>(6.0 * sampleRate));
  for (size_t i = 0; i < signal.size(); ++i)
  {
    const double x = double(i) * 48000.0 / sampleRate;
    const size_t k = std::min(static_cast<size_t>(x), base.size() - 2);
    signal[i] = base[k] + (base[k + 1] - base[k]) * (x - double(k));
  }
  juce::AudioBuffer<float> buffer(2, block);
  juce::MidiBuffer midi;
  RunResult r;

  // Espera o carregamento em segundo plano: a saída deixa de ser o sinal seco.
  size_t pos = 0;
  for (int tries = 0; tries < 4000 && !r.engineActive; ++tries)
  {
    for (int i = 0; i < block; ++i)
    {
      const float v = static_cast<float>(signal[(pos + i) % signal.size()]);
      buffer.setSample(0, i, v);
      buffer.setSample(1, i, v);
    }
    plugin.processBlock(buffer, midi);
    double diff = 0.0;
    for (int i = 0; i < block; ++i)
      diff = std::max(diff, std::abs(double(buffer.getSample(0, i)) - signal[(pos + i) % signal.size()]));
    r.engineActive = diff > 1e-3;
    pos += block;
    if (!r.engineActive)
      juce::Thread::sleep(2);
  }
  if (!r.engineActive)
    return r;

  // Toca 6 s girando o ganho e o equalizador como um usuário faria.
  auto* main = findParam(plugin, "ganho");
  auto* treble = findParam(plugin, "agudo");
  double sum = 0.0;
  size_t count = 0;
  for (size_t start = 0; start + block <= signal.size(); start += block)
  {
    const double t = double(start) / sampleRate;
    if (main)
      main->setValue(static_cast<float>(0.5 + 0.5 * std::sin(t * 2.0)));
    if (treble)
      treble->setValue(static_cast<float>(0.5 + 0.5 * std::sin(t * 1.3)));
    for (int i = 0; i < block; ++i)
    {
      const float v = static_cast<float>(signal[start + static_cast<size_t>(i)]);
      buffer.setSample(0, i, v);
      buffer.setSample(1, i, v);
    }
    plugin.processBlock(buffer, midi);
    for (int ch = 0; ch < 2; ++ch)
      for (int i = 0; i < block; ++i)
      {
        const double v = buffer.getSample(ch, i);
        if (!std::isfinite(v))
          r.finite = false;
        r.peak = std::max(r.peak, std::abs(v));
        sum += v * v;
        ++count;
      }
  }
  r.rmsDb = 10.0 * std::log10(std::max(sum / double(count), 1e-30));

  // Foto da tela com o pacote carregado, para conferência visual.
  if (gSnapshot.isNotEmpty() && sampleRate == 48000.0)
    if (auto* editor = plugin.createEditorIfNeeded())
    {
      juce::Thread::sleep(100);
      const auto image = editor->createComponentSnapshot(editor->getLocalBounds(), true, 2.0f);
      juce::File file(gSnapshot);
      file.deleteFile();
      juce::FileOutputStream stream(file);
      juce::PNGImageFormat().writeImageToStream(image, stream);
      delete editor;
    }
  plugin.releaseResources();
  return r;
}
} // namespace

int main(int argc, char** argv)
{
  if (argc < 3)
  {
    std::printf("Uso: nexus_host_test <plugin> <pasta_do_pacote>\n");
    return 2;
  }
  juce::ScopedJuceInitialiser_GUI juce;
  const juce::String pluginPath = juce::String::fromUTF8(argv[1]);
  const juce::String packFolder = juce::File(juce::String::fromUTF8(argv[2])).getFullPathName();
  if (argc > 3)
    gSnapshot = juce::File(juce::String::fromUTF8(argv[3])).getFullPathName();

  juce::AudioPluginFormatManager formats;
  juce::addDefaultFormatsToManager(formats);

  std::printf("Nexus Modeler — teste do plugin aberto como num programa de gravação\n");
  std::printf("  plugin: %s\n", pluginPath.toRawUTF8());

  juce::OwnedArray<juce::PluginDescription> found;
  for (auto* format : formats.getFormats())
    if (format->fileMightContainThisPluginType(pluginPath))
      format->findAllTypesForFile(found, pluginPath);
  report(!found.isEmpty(), u8("O programa de gravação reconhece o plugin"),
         found.isEmpty() ? juce::String("nenhum plugin encontrado no arquivo")
                         : found[0]->name + " (" + found[0]->pluginFormatName + ")");
  if (found.isEmpty())
    return 1;

  double reference = 0.0;
  for (double rate : {48000.0, 44100.0, 96000.0})
  {
    juce::String error;
    auto plugin = formats.createPluginInstance(*found[0], rate, 128, error);
    if (!plugin)
    {
      report(false, "Abrir o plugin a " + juce::String(rate, 0) + " Hz", error);
      continue;
    }
    const auto r = run(*plugin, rate, packFolder);
    const juce::String hz = juce::String(rate / 1000.0, 1) + " kHz";
    report(r.engineActive, "Carrega o pacote e passa a processar (" + hz + ")",
           r.engineActive ? juce::String("motor ativo") : juce::String(u8("a saída continuou igual à entrada")));
    if (!r.engineActive)
      continue;
    report(r.finite && r.peak < 4.0 && r.rmsDb > -50.0, u8("Som válido girando ganho e agudo (") + hz + ")",
           u8("volume médio ") + juce::String(r.rmsDb, 1) + " dBFS, pico " + juce::String(r.peak, 2)
             + (r.finite ? "" : u8(", com valores inválidos")));
    if (rate == 48000.0)
      reference = r.rmsDb;
    else
      report(std::abs(r.rmsDb - reference) < 1.5, "Mesmo volume que a 48 kHz (" + hz + u8(", com conversão de taxa)"),
             u8("diferença ") + juce::String(r.rmsDb - reference, 2) + " dB (limite 1,5 dB)");
  }

  std::printf("\n%s (%d falha%s)\n", gFailures ? "REPROVADO" : "APROVADO", gFailures, gFailures == 1 ? "" : "s");
  return gFailures ? 1 : 0;
}
