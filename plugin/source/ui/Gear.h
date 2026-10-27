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
  // Deslizante (equalizador gráfico): quadro retangular, arrasta na vertical.
  bool slider = false;
  int width = 0, height = 0;

  juce::Image frame(float value01) const
  {
    const int f = juce::jlimit(0, frames - 1, juce::roundToInt(value01 * static_cast<float>(frames - 1)));
    const int w = width > 0 ? width : size, h = height > 0 ? height : size;
    return strip.getClippedImage({0, f * h, w, h});
  }
};

// Rótulo do painel (GANHO, GRAVE, LIGA...): caixa de texto transparente
// por cima da imagem, traduzida para o idioma escolhido. A posição, o tamanho
// da letra e a perspectiva vêm do gerador 3D.
struct GearLabel
{
  juce::String text;           // em português; é a chave da tradução
  juce::Point<float> centre;   // em pixels da imagem da frente
  float height = 10.0f;        // altura da letra (em) em pixels da imagem
  float squash = 1.0f;         // achatamento vertical (texto deitado no pedal)
  float spacing = 1.0f;
  juce::Colour colour;
};

struct GearArt
{
  juce::String name;
  juce::Image front;
  std::vector<GearKnobArt> knobs;
  // Canais do amplificador original, na ordem do painel (vazio = um canal).
  juce::StringArray channels;
  std::vector<GearLabel> labels;
  // Pedais: onde pisar e onde fica o LED, em pixels da frente (raio 0 = não tem).
  juce::Point<float> footswitch;
  float footswitchRadius = 0.0f;
  juce::Point<float> led;
  float ledRadius = 0.0f;
  // Amplificadores de vários canais: um LED por canal, na ordem dos canais.
  std::vector<std::pair<juce::Point<float>, float>> channelLeds;

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
