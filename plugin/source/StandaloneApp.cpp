// Programa independente do Nexus Modeler: abre a interface de áudio direto
// (ASIO ou WASAPI no Windows, Core Audio no Mac, JACK ou ALSA no Linux) e
// mostra a mesma tela do plugin numa janela própria, sem o botão "Options"
// genérico do JUCE: a escolha da interface fica no painel de áudio do Nexus.

// Mesma sequência de cabeçalhos que o JUCE usa no programa independente padrão.
#include <juce_core/system/juce_TargetPlatform.h>
#include <juce_audio_plugin_client/detail/juce_CheckSettingMacros.h>
#include <juce_audio_plugin_client/detail/juce_IncludeSystemHeaders.h>
#include <juce_audio_plugin_client/detail/juce_IncludeModuleHeaders.h>
#include <juce_audio_plugin_client/detail/juce_PluginUtilities.h>
#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_gui_extra/juce_gui_extra.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_audio_plugin_client/Standalone/juce_StandaloneFilterWindow.h>

#include "StandaloneLink.h"
#include "report/Report.h"

#if JUCE_LINUX || JUCE_BSD
 #include <dlfcn.h>
#endif

namespace
{
#if JUCE_LINUX || JUCE_BSD
// No Linux de hoje (PipeWire), a placa de som pertence ao PipeWire: pelo ALSA
// o Nexus só vê o "padrão do sistema" e passa por uma fila a mais (latência
// alta e estalos quando o bloco do PipeWire e o do Nexus não batem). O JACK
// do PipeWire dá cada placa pelo nome, no mesmo bloco que ela roda. O JUCE
// abre "libjack.so.0"; carregando antes a do PipeWire, é ela que ele usa, sem
// precisar abrir o Nexus pelo "pw-jack".
bool preferPipeWireJack()
{
  for (const char* path : {"/usr/lib/x86_64-linux-gnu/pipewire-0.3/jack/libjack.so.0", "/usr/lib/aarch64-linux-gnu/pipewire-0.3/jack/libjack.so.0",
                           "/usr/lib64/pipewire-0.3/jack/libjack.so.0", "/usr/lib/pipewire-0.3/jack/libjack.so.0"})
    if (dlopen(path, RTLD_NOW | RTLD_GLOBAL) != nullptr)
    {
      nexus::report::log("audio", juce::String("JACK do PipeWire: ") + path);
      return true;
    }
  return false;
}
#endif

// Primeira abertura: 48 kHz (a taxa das capturas, sem conversão) e buffer de
// 128 amostras. Depois vale o que o usuário escolher, guardado nos ajustes.
juce::AudioDeviceManager::AudioDeviceSetup firstRunSetup()
{
  juce::AudioDeviceManager::AudioDeviceSetup setup;
  setup.sampleRate = 48000.0;
  setup.bufferSize = 128;
  return setup;
}

// Tela do Nexus dentro da janela. Numa tela menor que a do Nexus (notebook
// 1366x768, máquina virtual 1280x800), reduz tudo na mesma proporção para a
// janela caber com a barra de título; a tela em si não muda de tamanho.
// (o JUCE define "Component" como juce::Component neste arquivo)
class ScaledEditor : public Component
{
public:
  explicit ScaledEditor(juce::AudioProcessorEditor* editor) : mEditor(editor)
  {
    addAndMakeVisible(*mEditor);
    float scale = 1.0f;
    if (const auto* display = juce::Desktop::getInstance().getDisplays().getPrimaryDisplay())
    {
      const auto area = display->userArea;
      scale = std::min({1.0f, (area.getWidth() - 16.0f) / static_cast<float>(mEditor->getWidth()),
                        (area.getHeight() - 48.0f) / static_cast<float>(mEditor->getHeight())});
    }
    mEditor->setTransform(juce::AffineTransform::scale(scale));
    setSize(juce::roundToInt(mEditor->getWidth() * scale), juce::roundToInt(mEditor->getHeight() * scale));
  }

private:
  std::unique_ptr<juce::AudioProcessorEditor> mEditor;
};

class MainWindow : public juce::DocumentWindow
{
public:
  MainWindow(const juce::String& name, juce::StandalonePluginHolder& holder)
  : DocumentWindow(name, juce::Colour(0xff141414), DocumentWindow::minimiseButton | DocumentWindow::closeButton)
  , mHolder(holder)
  {
    setUsingNativeTitleBar(true);
    setResizable(false, false);
    if (auto* editor = holder.processor->createEditorIfNeeded())
      setContentOwned(new ScaledEditor(editor), true);
    centreWithSize(getWidth(), getHeight());
    setVisible(true);
  }

  void closeButtonPressed() override { juce::JUCEApplication::getInstance()->systemRequestedQuit(); }

private:
  juce::StandalonePluginHolder& mHolder;
};

class NexusStandaloneApp : public juce::JUCEApplication
{
public:
  NexusStandaloneApp()
  {
    juce::PropertiesFile::Options options;
    options.applicationName = "Nexus Modeler";
    options.filenameSuffix = ".settings";
    options.osxLibrarySubFolder = "Application Support";
   #if JUCE_LINUX || JUCE_BSD
    options.folderName = "~/.config";
   #else
    options.folderName = "";
   #endif
    mProperties.setStorageParameters(options);
  }

  const juce::String getApplicationName() override { return "Nexus Modeler"; }
  const juce::String getApplicationVersion() override { return JucePlugin_VersionString; }
  bool moreThanOneInstanceAllowed() override { return false; }

  void initialise(const juce::String&) override
  {
    nexus::report::installCrashHandler();
    nexus::report::log("inicio", nexus::report::systemInfo().replace("\n", " | "));
   #if JUCE_LINUX || JUCE_BSD
    const bool pipeWireJack = preferPipeWireJack();
    // Bloco pedido ao PipeWire: o buffer guardado (128 na primeira abertura).
    // Sem o pedido, o JACK do PipeWire abre com 1024 amostras (33 ms).
    if (getenv("PIPEWIRE_LATENCY") == nullptr)
    {
      int block = 128, rate = 48000;
      if (auto xml = mProperties.getUserSettings()->getXmlValue("audioSetup"))
      {
        block = xml->getIntAttribute("audioDeviceBufferSize", block);
        rate = juce::roundToInt(xml->getDoubleAttribute("audioDeviceRate", rate));
      }
      setenv("PIPEWIRE_LATENCY", (juce::String(block > 0 ? block : 128) + "/" + juce::String(rate > 0 ? rate : 48000)).toRawUTF8(), 1);
    }
   #endif
    const bool firstRun = mProperties.getUserSettings()->getXmlValue("audioSetup") == nullptr;
    const auto setup = firstRunSetup();
    // Último argumento: abre sozinho todo dispositivo MIDI que aparecer (USB ou
    // Bluetooth já pareado), conferindo a cada meio segundo.
    mHolder = std::make_unique<juce::StandalonePluginHolder>(mProperties.getUserSettings(), false, juce::String(), &setup,
                                                             juce::Array<juce::StandalonePluginHolder::PluginInOuts>(), true);
    // O JUCE silencia a entrada por padrão (medo de microfonia com microfone).
    // Num simulador de guitarra a entrada é a guitarra: fica sempre aberta.
    mHolder->getMuteInputValue().setValue(false);
   #if JUCE_LINUX || JUCE_BSD
    // Primeira abertura com o JACK do PipeWire: começa por ele (cada placa pelo
    // nome, sem fila a mais). Depois vale o que o usuário escolher.
    if (firstRun && pipeWireJack)
    {
      auto& devices = mHolder->deviceManager;
      devices.setCurrentAudioDeviceType("JACK", true);
      if (devices.getCurrentAudioDevice() == nullptr)
        devices.setCurrentAudioDeviceType("ALSA", true);
    }
   #else
    juce::ignoreUnused(firstRun);
   #endif

    auto& link = nexus::standaloneLink();
    link.devices = &mHolder->deviceManager;
    link.saveDeviceState = [this] {
      if (mHolder)
        mHolder->saveAudioDeviceState();
    };
    mWindow = std::make_unique<MainWindow>(getApplicationName(), *mHolder);
  }

  void shutdown() override
  {
    if (mHolder)
    {
      mHolder->savePluginState();
      mHolder->saveAudioDeviceState();
    }
    nexus::standaloneLink() = {};
    mWindow.reset();
    mHolder.reset();
  }

  void systemRequestedQuit() override
  {
    if (mHolder)
      mHolder->savePluginState();
    quit();
  }

  void anotherInstanceStarted(const juce::String&) override
  {
    if (mWindow)
      mWindow->toFront(true);
  }

private:
  juce::ApplicationProperties mProperties;
  std::unique_ptr<juce::StandalonePluginHolder> mHolder;
  std::unique_ptr<MainWindow> mWindow;
};
} // namespace

juce::JUCEApplicationBase* juce_CreateApplication()
{
  return new NexusStandaloneApp();
}
