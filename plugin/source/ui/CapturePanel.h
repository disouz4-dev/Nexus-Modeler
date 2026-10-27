#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include <functional>
#include <vector>

#include "../tone3000/Tone3000.h"
#include "Controls.h"

// Janela "Abrir pacote": as capturas da TONE3000 para o equipamento escolhido,
// dentro da tela do Nexus e com a TONE3000 como fonte. Abas: recomendadas
// para aquele equipamento, favoritos e baixados da conta (filtrados pelo nome
// do equipamento) e em alta. Embaixo: catálogo completo da TONE3000 e
// "inserir manualmente" (pasta do computador).
namespace nexus::ui
{
class CapturePanel : public juce::Component, private juce::ListBoxModel
{
public:
  CapturePanel();
  ~CapturePanel() override;

  enum class Gear
  {
    amp,
    pedal,
    cab, // IRs de caixa
  };
  // gearPrefix: visual do equipamento ("brit800"); name: nome na tela.
  void open(const juce::String& gearPrefix, const juce::String& name, Gear gear);
  void retranslate();

  void paint(juce::Graphics&) override;
  void resized() override;
  void mouseUp(const juce::MouseEvent&) override;

  std::function<void(const juce::File& folder)> onLoaded;
  std::function<void()> onManual;
  std::function<void()> onClose;

private:
  int getNumRows() override { return static_cast<int>(mTones.size()); }
  void paintListBoxItem(int row, juce::Graphics&, int width, int height, bool selected) override;
  void listBoxItemClicked(int row, const juce::MouseEvent&) override;
  juce::MouseCursor getMouseCursorForRow(int) override { return juce::MouseCursor::PointingHandCursor; }

  juce::Rectangle<int> card() const;
  void showTab(int tab);
  void load(const t3k::Tone& tone);
  void refreshAccount();
  void setStatus(const juce::String& text, bool error = false);
  juce::String gearType() const { return mGear == Gear::pedal ? "pedal" : mGear == Gear::cab ? "cab" : "amp"; }

  juce::SharedResourcePointer<t3k::Connection> mConnection;
  juce::String mPrefix, mName;
  Gear mGear = Gear::amp;
  int mTab = 0;
  bool mBusy = false;
  std::vector<t3k::Tone> mTones;
  juce::String mEmpty;
  juce::String mStatus;
  bool mStatusError = false;
  int mRequest = 0; // descarta respostas de abas antigas

  Segmented mTabs{{"Recomendadas", "Seus favoritos", "Seus downloads", "Em alta"}};
  juce::ListBox mList{"capturas", this};
  juce::TextButton mSignIn{"Entrar com a TONE3000"};
  TextLink mSignOut{"sair"};
  juce::TextButton mCatalog{juce::String::fromUTF8("Catálogo completo da TONE3000")};
  juce::TextButton mManual{"Inserir manualmente"};
  juce::TextButton mClose{"Fechar"};
};
} // namespace nexus::ui
