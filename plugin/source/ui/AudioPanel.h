#pragma once

#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include <functional>

// Painel de áudio do programa independente: sistema de áudio (ASIO, WASAPI,
// Core Audio, JACK, ALSA), interface, entrada da guitarra, saídas, taxa e
// buffer, com a latência de ida e volta em milissegundos e um medidor do
// sinal da guitarra para conferir a ligação.
namespace nexus::ui
{
class AudioPanel : public juce::Component, private juce::ChangeListener, private juce::Timer
{
public:
  // "pluginLatency" devolve a latência extra do Nexus em amostras (conversão
  // de taxa quando a interface não está em 48 kHz); "inputPeak" o pico da
  // guitarra (0 a 1) desde a última leitura.
  AudioPanel(juce::AudioDeviceManager& devices, std::function<void()> save, std::function<int()> pluginLatency,
             std::function<float()> inputPeak);
  ~AudioPanel() override;

  void paint(juce::Graphics&) override;
  void resized() override;
  void mouseUp(const juce::MouseEvent&) override;
  void visibilityChanged() override;
  std::function<void()> onClose;

  // Latência de ida e volta em milissegundos (entrada + buffer + saída + Nexus).
  double roundTripMs() const;

private:
  struct Row
  {
    juce::String label;
    juce::ComboBox box;
  };

  void changeListenerCallback(juce::ChangeBroadcaster*) override { refresh(); }
  void timerCallback() override;
  void refresh();
  void apply(const std::function<void(juce::AudioDeviceManager::AudioDeviceSetup&)>& change);
  juce::Rectangle<int> card() const;

  juce::AudioDeviceManager& mDevices;
  std::function<void()> mSave;
  std::function<int()> mPluginLatency;
  std::function<float()> mInputPeak;
  Row mType, mInputDevice, mOutputDevice, mGuitar, mOutputs, mRate, mBuffer;
  juce::TextButton mDriverPanel{"Painel do driver"};
  juce::TextButton mClose{"Fechar"};
  juce::String mError;
  float mMeter = 0.0f;
  double mCpu = 0.0;
  bool mRefreshing = false;
};
} // namespace nexus::ui
