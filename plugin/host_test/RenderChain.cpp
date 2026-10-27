// Passa uma gravação de guitarra pela cadeia do Nexus, com material da
// TONE3000, e grava o resultado para ouvir:
//   Banana Overdrive (captura do SD-1) -> Brit 800 (captura do JCM800) -> IR.
//
// Baixa as capturas pela conta conectada (o mesmo caminho do botão
// "Carregar" do programa, para Documentos/Nexus Modeler/<equipamento>) e usa o
// mesmo processador do plugin.
//
// Uso: nexus_render <guitarra.wav> <ir.wav> <pasta_de_saida> [ganho_de_entrada_dB]

#include <cmath>
#include <cstdio>

#include "../../tools/Wav.h"
#include "../source/PluginProcessor.h"
#include "../source/tone3000/Tone3000.h"

namespace
{
template <typename Done>
void waitFor(Done&& done, int seconds)
{
  for (int i = 0; i < seconds * 10 && !done(); ++i)
    juce::MessageManager::getInstance()->runDispatchLoopUntil(100);
}

void setParam(NexusProcessor& p, const char* id, float plain)
{
  auto* param = p.params.getParameter(id);
  param->setValueNotifyingHost(param->convertTo0to1(plain));
}

void settle(NexusProcessor& p, int blocks)
{
  juce::AudioBuffer<float> buffer(2, 128);
  juce::MidiBuffer midi;
  for (int i = 0; i < blocks; ++i)
  {
    buffer.clear();
    p.processBlock(buffer, midi);
    juce::Thread::sleep(1);
  }
}

// Baixa um tom recomendado e devolve a pasta do pacote.
juce::File fetch(nexus::t3k::Connection& t3k, const char* prefix, int id, const juce::String& gearName, const char* filter)
{
  nexus::t3k::Tone tone;
  for (const auto& r : nexus::t3k::recommended(prefix))
    if (r.id == id)
      tone = r;
  bool finished = false;
  juce::File folder;
  juce::String error;
  t3k.download(
    tone, filter, gearName, [](juce::String s) { std::printf("  ... %s\n", s.toRawUTF8()); },
    [&](juce::File f, juce::String e) {
      folder = f;
      error = e;
      finished = true;
    });
  waitFor([&] { return finished; }, 600);
  if (!error.isEmpty() || !folder.isDirectory())
    std::printf("  ERRO ao baixar %s: %s\n", tone.title.toRawUTF8(), error.toRawUTF8());
  return folder;
}

struct Setting
{
  const char* file;
  const char* description;
  bool pedal;
  float drive, tone, level;
  float gain, bass, middle, treble, presence, volume;
};
} // namespace

int main(int argc, char** argv)
{
  if (argc < 4)
  {
    std::printf("Uso: nexus_render <guitarra.wav> <ir.wav> <pasta_de_saida> [ganho_de_entrada_dB]\n");
    return 2;
  }
  juce::ScopedJuceInitialiser_GUI juce;
  const auto cwd = juce::File::getCurrentWorkingDirectory();
  const auto input = cwd.getChildFile(juce::String::fromUTF8(argv[1]));
  const auto ir = cwd.getChildFile(juce::String::fromUTF8(argv[2]));
  const auto out = cwd.getChildFile(juce::String::fromUTF8(argv[3]));
  const double inputDb = argc > 4 ? std::atof(argv[4]) : 0.0;
  out.createDirectory();

  double rate = 48000.0;
  auto dry = wav::read(input.getFullPathName().toStdString(), rate);
  double peak = 0.0;
  for (auto& v : dry)
  {
    v *= std::pow(10.0, inputDb / 20.0);
    peak = std::max(peak, std::abs(v));
  }
  std::printf("Guitarra: %.1f s a %.0f Hz, pico %.1f dBFS\n", dry.size() / rate, rate, 20.0 * std::log10(peak + 1e-12));

  nexus::t3k::Connection t3k;
  if (!t3k.signedIn())
  {
    std::printf("Conecte a conta da TONE3000 no Nexus antes.\n");
    return 1;
  }
  std::printf("\nBaixando da TONE3000\n");
  const auto banana = fetch(t3k, "banana_overdrive", 82811, "Banana Overdrive", "pedal");
  const auto brit = fetch(t3k, "brit800", 1071, "Brit 800", "amp_amp-cab");
  if (!banana.isDirectory() || !brit.isDirectory())
    return 1;

  const Setting settings[] = {
    {"1_guitarra_limpa_sem_nexus.wav", "a gravação original, sem passar pelo Nexus", false, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {"2_brit800_crunch.wav", "Brit 800 no ganho 5, sem pedal", false, 0, 5, 5, 5, 5, 6, 6, 5, 5},
    {"3_banana_brit800_rock.wav", "Banana drive 3 empurrando o Brit 800 no ganho 7", true, 3, 5, 7, 7, 5, 6, 6, 6, 5},
    {"4_banana_brit800_solo.wav", "Banana drive 6 e Brit 800 no ganho 10", true, 6, 6, 7, 10, 5, 7, 6, 6, 5},
  };

  for (const auto& s : settings)
  {
    std::vector<double> result;
    if (!s.pedal && s.gain == 0)
      result = dry;
    else
    {
      NexusProcessor p;
      p.setPlayConfigDetails(2, 2, rate, 128);
      p.prepareToPlay(rate, 128);
      ChainLayout layout;
      if (s.pedal)
        layout.pre = {0};
      p.setLayout(layout);
      setParam(p, "p1_visual", 2.0f); // Banana Overdrive
      setParam(p, "p1_drive", s.drive);
      setParam(p, "p1_tom", s.tone);
      setParam(p, "p1_nivel", s.level);
      setParam(p, "ganho", s.gain);
      setParam(p, "grave", s.bass);
      setParam(p, "medio", s.middle);
      setParam(p, "agudo", s.treble);
      setParam(p, "presenca", s.presence);
      setParam(p, "volume", s.volume);
      p.loadAmpPack(0, 0, brit);
      if (s.pedal)
        p.loadPedalPack(0, banana);
      p.loadIrAsync(0, ir);
      for (int t = 0; t < 2400 && !(p.channelLoaded(0, 0) && (!s.pedal || p.slotLoaded(0)) && p.irLoaded(0)); ++t)
        settle(p, 4);
      setParam(p, "caixa", 1.0f);
      settle(p, 400); // botões e trocas assentam

      juce::AudioBuffer<float> buffer(2, 128);
      juce::MidiBuffer midi;
      result.reserve(dry.size());
      for (size_t start = 0; start < dry.size(); start += 128)
      {
        const int n = static_cast<int>(std::min<size_t>(128, dry.size() - start));
        buffer.setSize(2, n, false, false, true);
        for (int ch = 0; ch < 2; ++ch)
          for (int i = 0; i < n; ++i)
            buffer.setSample(ch, i, static_cast<float>(dry[start + static_cast<size_t>(i)]));
        p.processBlock(buffer, midi);
        for (int i = 0; i < n; ++i)
          result.push_back(buffer.getSample(0, i));
      }
    }
    // Mesmo volume de pico em todos (-1 dBFS), para comparar sem susto.
    double m = 0.0;
    for (auto v : result)
      m = std::max(m, std::abs(v));
    const double k = m > 0.0 ? std::pow(10.0, -1.0 / 20.0) / m : 1.0;
    for (auto& v : result)
      v *= k;
    const auto file = out.getChildFile(s.file);
    wav::write(file.getFullPathName().toStdString(), result, rate);
    std::printf("Gravado: %s  (%s)\n", file.getFileName().toRawUTF8(), s.description);
  }
  return 0;
}
