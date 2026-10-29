#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include "../update/GearStore.h"

// Ícone de equipamentos novos na coluna de equipamentos, ao lado da pesquisa:
// um amplificador pequeno, sempre visível. Apagado quando não há novidade (o
// clique procura agora); aceso, com a quantidade, quando há equipamento novo
// ou refeito (o clique baixa). Ao abrir o Nexus ele procura sozinho, mas só
// baixa no clique. Durante a procura pisca; no download, um anel mostra o
// andamento; no erro fica vermelho e o clique tenta de novo. Equipamento novo
// entra na lista na hora; arte refeita de uma peça que já existia vale quando
// o Nexus abre de novo.
namespace nexus::ui
{
class ArteBadge : public juce::Component, public juce::SettableTooltipClient, private juce::ChangeListener, private juce::Timer
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
  void timerCallback() override { mBlink = !mBlink; repaint(); }
  void refresh();
  bool mBlink = false;

  juce::SharedResourcePointer<nexus::update::GearStore> mArte;
  nexus::update::GearStore::State mShown = nexus::update::GearStore::State::idle;
};
} // namespace nexus::ui
