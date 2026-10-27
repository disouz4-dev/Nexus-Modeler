#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include <vector>

// Imagens renderizadas pelo Nexus Gear Studio (tools/gear_studio), embutidas
// no plugin: a frente do equipamento e, para cada botão, uma tira com todas
// as posições empilhadas.
namespace nexus::ui
{
struct GearKnobArt
{
  juce::String id;
  juce::Rectangle<int> box; // onde o botão fica na imagem da frente, em pixels
  juce::Image strip;        // posições empilhadas de cima para baixo
  int frames = 1;
  int size = 0;

  juce::Image frame(float value01) const
  {
    const int f = juce::jlimit(0, frames - 1, juce::roundToInt(value01 * static_cast<float>(frames - 1)));
    return strip.getClippedImage({0, f * size, size, size});
  }
};

struct GearArt
{
  juce::String name;
  juce::Image front;
  std::vector<GearKnobArt> knobs;
  // Canais do amplificador original, na ordem do painel (vazio = um canal).
  juce::StringArray channels;
  // Pedais: onde pisar e onde fica o LED, em pixels da frente (raio 0 = não tem).
  juce::Point<float> footswitch;
  float footswitchRadius = 0.0f;
  juce::Point<float> led;
  float ledRadius = 0.0f;

  bool valid() const { return front.isValid(); }
  const GearKnobArt* knob(const juce::String& id) const
  {
    for (const auto& k : knobs)
      if (k.id == id)
        return &k;
    return nullptr;
  }

  // Carrega "<prefixo>_frente.png", "<prefixo>_mapa.json" e as tiras dos botões.
  static GearArt load(const juce::String& prefix);
};
} // namespace nexus::ui
