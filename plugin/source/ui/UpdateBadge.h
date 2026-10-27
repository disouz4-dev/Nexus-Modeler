#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include "../update/Updater.h"

// Aviso de versão nova na barra de cima: "Nova versão 0.2.0". O clique abre o
// menu (baixar e instalar, ver o que mudou, lembrar depois, ignorar esta
// versão); durante o download mostra a porcentagem. Some sozinho quando não há
// nada para mostrar.
namespace nexus::ui
{
class UpdateBadge : public juce::Component, public juce::SettableTooltipClient, private juce::ChangeListener
{
public:
  UpdateBadge();
  ~UpdateBadge() override;

  // Para a tela: o aviso mudou de tamanho ou de visibilidade.
  std::function<void()> onLayoutChange;
  int idealWidth() const;
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
  juce::String text() const;

  juce::SharedResourcePointer<nexus::update::Updater> mUpdater;
  nexus::update::State mShown = nexus::update::State::idle;
};
} // namespace nexus::ui
