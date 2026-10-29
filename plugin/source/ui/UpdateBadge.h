#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include "../update/Updater.h"

// Ícone de atualização na barra de cima: uma nuvem com seta para baixo, acesa,
// quando há versão nova. Ao abrir o Nexus, se houver versão nova, pergunta
// uma vez se a pessoa quer atualizar; o clique no ícone pergunta de novo. O
// botão direito abre o menu (ver o que mudou, ignorar esta versão). Durante o
// download, um anel em volta mostra o andamento. Some quando não há nada.
namespace nexus::ui
{
class UpdateBadge : public juce::Component, public juce::SettableTooltipClient, private juce::ChangeListener
{
public:
  UpdateBadge();
  ~UpdateBadge() override;

  // Para a tela: o ícone apareceu ou sumiu.
  std::function<void()> onLayoutChange;
  // Itens do menu "i" (procurar agora, procurar sozinho).
  void addMenuItems(juce::PopupMenu& menu);
  void checkNow() { mUpdater->checkNow(); }
  void retranslate() { refresh(); }

  void paint(juce::Graphics&) override;
  void mouseUp(const juce::MouseEvent&) override;
  void mouseEnter(const juce::MouseEvent&) override { repaint(); }
  void mouseExit(const juce::MouseEvent&) override { repaint(); }

private:
  void changeListenerCallback(juce::ChangeBroadcaster*) override;
  void refresh();
  void showMenu();
  void showResult();
  void ask();
  juce::String text() const;

  juce::SharedResourcePointer<nexus::update::Updater> mUpdater;
  nexus::update::State mShown = nexus::update::State::idle;
};
} // namespace nexus::ui
