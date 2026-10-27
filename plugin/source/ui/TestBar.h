#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include "../TestSource.h"
#include "Controls.h"

// Barra "Testar o som", em cima da entrada no caminho do sinal: tocar ou
// parar, o estilo da gravação (Limpo, Blues, Rock, Pesado, Minha guitarra) e
// o botão de gravar a sua guitarra.
namespace nexus::ui
{
class TestBar : public juce::Component, public juce::SettableTooltipClient
{
public:
  explicit TestBar(nexus::TestSource& source);

  // Chamado pelo relógio da tela.
  void refresh();
  void retranslate();
  int idealWidth() const;

  void paint(juce::Graphics&) override;
  void resized() override;
  void mouseUp(const juce::MouseEvent&) override;
  void mouseMove(const juce::MouseEvent&) override { repaint(); }
  void mouseExit(const juce::MouseEvent&) override { repaint(); }

private:
  juce::Rectangle<float> playArea() const;
  juce::Rectangle<float> recordArea() const;
  juce::StringArray styleNames() const;

  nexus::TestSource& mSource;
  Segmented mStyles{{"Limpo", "Blues", "Rock", "Pesado", "Minha guitarra"}};
  bool mWasPlaying = false, mWasRecording = false;
};
} // namespace nexus::ui
