#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include "../update/Updater.h"

// Ícone de atualização na barra de cima: uma nuvem com seta para baixo, sempre
// visível. Apagado quando não há versão nova (o clique procura agora; pisca
// enquanto procura); aceso, com um ponto, quando há (o clique baixa e instala).
// Ao abrir o Nexus ele procura sozinho e só acende, sem perguntar. O botão
// direito no ícone aceso abre o menu (ver o que mudou, lembrar depois, ignorar
// esta versão). Durante o download, um anel em volta mostra o andamento.
namespace nexus::ui
{
class UpdateBadge : public juce::Component, public juce::SettableTooltipClient, private juce::ChangeListener, private juce::Timer
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
  void timerCallback() override { mBlink = !mBlink; repaint(); }
  bool lit() const;
  bool mBlink = false;
  void refresh();
  void showMenu();
  void showResult();
  void ask();
  void showReleaseNotes();
  void showWhatsNew();
  juce::String text() const;

  juce::SharedResourcePointer<nexus::update::Updater> mUpdater;
  nexus::update::State mShown = nexus::update::State::idle;
};
} // namespace nexus::ui
