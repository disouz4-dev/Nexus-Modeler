#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include "../update/ArteNova.h"

// Ícone de equipamentos novos na barra de cima: um amplificador pequeno com a
// quantidade de peças novas ou refeitas que já podem ser baixadas. O clique
// mostra quais são e baixa; depois de baixadas, ficam no Mac e passam a valer
// quando o Nexus abre de novo. Some quando não há nada novo.
namespace nexus::ui
{
class ArteBadge : public juce::Component, public juce::SettableTooltipClient, private juce::ChangeListener
{
public:
  ArteBadge();
  ~ArteBadge() override;

  // Para a tela: o ícone apareceu ou sumiu.
  std::function<void()> onLayoutChange;
  void retranslate() { refresh(); }

  void paint(juce::Graphics&) override;
  void mouseUp(const juce::MouseEvent&) override;
  void mouseEnter(const juce::MouseEvent&) override { repaint(); }
  void mouseExit(const juce::MouseEvent&) override { repaint(); }

private:
  void changeListenerCallback(juce::ChangeBroadcaster*) override;
  void refresh();
  void ask();
  void finished();

  juce::SharedResourcePointer<nexus::update::ArteNova> mArte;
  nexus::update::ArteNova::State mShown = nexus::update::ArteNova::State::idle;
};
} // namespace nexus::ui
