// Teste com interface de áudio de verdade (não roda na compilação automática,
// que não tem placa de som). Abre a entrada pedida e manda a saída para o
// dispositivo de saída pedido, roda o Nexus com um pacote carregado em cada
// tamanho de buffer e mede: chamadas de áudio, falhas (buffer que não chegou
// a tempo), uso do processador e latência de ida e volta. Salva também uma
// foto do painel de áudio.
//
// Uso: nexus_device_test <pacote> <entrada> <saída> [pasta das fotos]
// Ex.: nexus_device_test build/test_packs/ganho "Realtek USB2.0 Audio" "BlackHole 2ch" build/medidas

#include <cstdio>

#include <juce_audio_utils/juce_audio_utils.h>

#include "../source/PluginEditor.h"
#include "../source/PluginProcessor.h"
#include "../source/StandaloneLink.h"

namespace
{
// Conta as chamadas da interface e passa o áudio para o Nexus.
struct CountingPlayer : juce::AudioIODeviceCallback
{
  juce::AudioProcessorPlayer player;
  std::atomic<int> calls{0};
  void audioDeviceIOCallbackWithContext(const float* const* in, int nIn, float* const* out, int nOut, int n,
                                        const juce::AudioIODeviceCallbackContext& ctx) override
  {
    player.audioDeviceIOCallbackWithContext(in, nIn, out, nOut, n, ctx);
    ++calls;
  }
  void audioDeviceAboutToStart(juce::AudioIODevice* d) override { player.audioDeviceAboutToStart(d); }
  void audioDeviceStopped() override { player.audioDeviceStopped(); }
};
} // namespace

int main(int argc, char** argv)
{
  if (argc < 4)
  {
    std::printf("Uso: nexus_device_test <pacote> <entrada> <saida> [pasta das fotos]\n");
    return 2;
  }
  juce::ScopedJuceInitialiser_GUI gui;
  const auto cwd = juce::File::getCurrentWorkingDirectory();
  const juce::String inName = juce::String::fromUTF8(argv[2]);
  const juce::String outName = juce::String::fromUTF8(argv[3]);
  const juce::File photos = argc > 4 ? cwd.getChildFile(juce::String::fromUTF8(argv[4])) : juce::File();

  juce::AudioDeviceManager devices;
  devices.initialiseWithDefaultDevices(2, 2);
  std::printf("Nexus Modeler — teste com interface de áudio real\nSistema de áudio: %s\n",
              devices.getCurrentAudioDeviceType().toRawUTF8());
  if (auto* type = devices.getCurrentDeviceTypeObject())
  {
    std::printf("Entradas disponíveis: %s\n", type->getDeviceNames(true).joinIntoString(", ").toRawUTF8());
    std::printf("Saídas disponíveis:   %s\n", type->getDeviceNames(false).joinIntoString(", ").toRawUTF8());
  }

  NexusProcessor processor;
  CountingPlayer counter;
  counter.player.setProcessor(&processor);
  processor.loadAmpPack(0, 0, cwd.getChildFile(juce::String::fromUTF8(argv[1])));
  // Nada de som alto por acidente: saída do plugin no mínimo.
  processor.params.getParameter("volume")->setValueNotifyingHost(0.0f);

  int failures = 0;
  for (int buffer : {32, 64, 128, 256})
  {
    auto setup = devices.getAudioDeviceSetup();
    setup.inputDeviceName = inName;
    setup.outputDeviceName = outName;
    setup.sampleRate = 48000.0;
    setup.bufferSize = buffer;
    setup.useDefaultInputChannels = false;
    setup.inputChannels.clear();
    setup.inputChannels.setBit(0);
    setup.useDefaultOutputChannels = false;
    setup.outputChannels.clear();
    setup.outputChannels.setRange(0, 2, true);
    const auto error = devices.setAudioDeviceSetup(setup, true);
    auto* device = devices.getCurrentAudioDevice();
    if (error.isNotEmpty() || device == nullptr)
    {
      std::printf("\nbuffer %d: não abriu (%s)\n", buffer, error.toRawUTF8());
      ++failures;
      continue;
    }
    devices.addAudioCallback(&counter);
    // Espera o pacote carregar antes da primeira medida.
    for (int t = 0; t < 60000 && !processor.channelLoaded(0, 0); t += 50)
      juce::MessageManager::getInstance()->runDispatchLoopUntil(50);
    counter.calls = 0;
    const int xrunsBefore = device->getXRunCount();
    const double seconds = 4.0;
    juce::MessageManager::getInstance()->runDispatchLoopUntil(static_cast<int>(seconds * 1000));
    const double cpu = devices.getCpuUsage();
    const int xruns = device->getXRunCount() - xrunsBefore;
    devices.removeAudioCallback(&counter);

    const double sr = device->getCurrentSampleRate();
    const int realBuffer = device->getCurrentBufferSizeSamples();
    const int lin = device->getInputLatencyInSamples();
    const int lout = device->getOutputLatencyInSamples();
    // As latências do driver já incluem o buffer de cada lado.
    const double rtMs = 1000.0 * (lin + lout + processor.getLatencySamples()) / sr;
    const int expected = static_cast<int>(seconds * sr / realBuffer);
    const bool ok = xruns <= 0 && counter.calls >= expected * 0.9;
    if (!ok)
      ++failures;
    const auto opened = devices.getAudioDeviceSetup();
    std::printf("\n[%s] entrada %s -> saída %s, %.0f Hz, buffer %d (%.2f ms)\n", ok ? " OK  " : "FALHA",
                opened.inputDeviceName.toRawUTF8(), opened.outputDeviceName.toRawUTF8(), sr, realBuffer, 1000.0 * realBuffer / sr);
    std::printf("        chamadas %d de ~%d, falhas de áudio %d, processador %.0f%%\n", counter.calls.load(), expected,
                xruns, cpu * 100.0);
    std::printf("        latência: entrada %d + saída %d amostras (com o buffer); ida e volta %.1f ms\n", lin, lout, rtMs);
  }

  if (photos != juce::File())
  {
    nexus::standaloneLink().devices = &devices;
    std::unique_ptr<juce::AudioProcessorEditor> editor(processor.createEditor());
    if (auto* e = dynamic_cast<NexusEditor*>(editor.get()))
      if (auto* panel = e->audioPanel())
      {
        panel->setVisible(true);
        panel->toFront(false);
      }
    juce::MessageManager::getInstance()->runDispatchLoopUntil(300);
    const auto image = editor->createComponentSnapshot(editor->getLocalBounds(), true, 2.0f);
    const auto out = photos.getChildFile("painel_audio.png");
    out.deleteFile();
    juce::FileOutputStream stream(out);
    juce::PNGImageFormat().writeImageToStream(image, stream);
    editor.reset();
    nexus::standaloneLink() = {};
  }
  counter.player.setProcessor(nullptr);
  std::printf("\n%s (%d falha%s)\n", failures ? "REPROVADO" : "APROVADO", failures, failures == 1 ? "" : "s");
  return failures ? 1 : 0;
}
