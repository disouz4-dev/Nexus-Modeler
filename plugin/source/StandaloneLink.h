#pragma once

#include <juce_audio_devices/juce_audio_devices.h>

#include <functional>

// Ponte entre a tela e o programa independente. Dentro de um programa de
// gravação (VST3, AU, LV2) quem cuida da interface de áudio é ele, e
// "devices" fica vazio; no programa independente, o próprio Nexus abre a
// interface e a tela mostra o painel de áudio.
namespace nexus
{
struct StandaloneLink
{
  juce::AudioDeviceManager* devices = nullptr;
  std::function<void()> saveDeviceState;
};

inline StandaloneLink& standaloneLink()
{
  static StandaloneLink link;
  return link;
}
} // namespace nexus
