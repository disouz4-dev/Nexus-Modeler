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

namespace
{
// Primeira abertura: 48 kHz (a taxa das capturas, sem conversão) e buffer de
// 128 amostras. Depois vale o que o usuário escolher, guardado nos ajustes.
juce::AudioDeviceManager::AudioDeviceSetup firstRunSetup()
{
  juce::AudioDeviceManager::AudioDeviceSetup setup;
  setup.sampleRate = 48000.0;
  setup.bufferSize = 128;
  return setup;
}

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
      setContentOwned(editor, true);
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
    const auto setup = firstRunSetup();
    // Último argumento: abre sozinho todo dispositivo MIDI que aparecer (USB ou
    // Bluetooth já pareado), conferindo a cada meio segundo.
    mHolder = std::make_unique<juce::StandalonePluginHolder>(mProperties.getUserSettings(), false, juce::String(), &setup,
                                                             juce::Array<juce::StandalonePluginHolder::PluginInOuts>(), true);
    // O JUCE silencia a entrada por padrão (medo de microfonia com microfone).
    // Num simulador de guitarra a entrada é a guitarra: fica sempre aberta.
    mHolder->getMuteInputValue().setValue(false);

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
