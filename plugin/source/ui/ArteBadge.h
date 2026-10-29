#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include "../update/GearStore.h"

// Ícone de equipamentos novos na coluna de equipamentos, ao lado da pesquisa:
// um amplificador pequeno que só mostra o estado do GearStore, que procura e
// baixa sozinho ao abrir (procurando, baixando com o andamento, quantas peças
// chegaram, ou o erro; clique só no erro, para tentar de novo). Equipamento
// novo entra na lista na hora; arte refeita de uma peça que já existia vale
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

  juce::SharedResourcePointer<nexus::update::GearStore> mArte;
  nexus::update::GearStore::State mShown = nexus::update::GearStore::State::idle;
};
} // namespace nexus::ui
