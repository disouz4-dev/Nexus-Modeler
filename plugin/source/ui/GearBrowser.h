#pragma once

#include <array>
#include <set>

#include <juce_gui_basics/juce_gui_basics.h>

#include <functional>
#include <vector>

#include "Controls.h"
#include "Gear.h"

// Coluna de equipamentos, à direita da tela: abas Pedais, Amplificadores e
// Caixas; cada equipamento com a imagem 3D, o nome, o clássico que inspirou e
// de onde vem o som. Clique troca a peça escolhida; arrastar para a cadeia
// acrescenta a peça onde ela for solta. Os planejados aparecem em "Em breve".
// A pesquisa procura no nome e no original ("JCM800" acha o Bridge 800); cada
// categoria encolhe com um clique no título.
namespace nexus::ui
{
struct GearItem
{
  enum class Kind
  {
    pedal,
    amp,
    cab
  };
  Kind kind = Kind::pedal;
  int look = -1; // visual do pedal (0 em diante); -1 = ainda não existe
  juce::String name;
  juce::String inspiration; // "Ibanez TS9"
  juce::String category;    // "Drive e distorção", "Modulação"...
  juce::String source;      // "Captura da TONE3000" ou "Efeito do Nexus"
  const GearArt* art = nullptr;
  // Peças sem imagem renderizada (equalizadores): miniatura desenhada.
  std::function<void(juce::Graphics&, juce::Rectangle<float>)> paintThumb;
  bool available() const { return look >= 0 && ((art != nullptr && art->valid()) || paintThumb != nullptr); }
};

class GearBrowser : public juce::Component
{
public:
  GearBrowser();
  ~GearBrowser() override;

  void setItems(std::vector<GearItem> items);
  void setTab(GearItem::Kind kind);
  GearItem::Kind tab() const { return mTab; }
  // A aba Caixas só aparece com alguma caixa na lista (a beta.3 começa sem
  // nenhuma; ela volta quando a primeira caixa revisada chega pelo botão).
  bool hasCabs() const { return mHasCabs; }
  // Peça que a tela mostra agora (fica marcada na lista).
  void setCurrent(GearItem::Kind kind, int look);
  void retranslate();
  // Componente pequeno à direita da pesquisa (o ícone de equipamentos novos);
  // só ocupa lugar quando está visível.
  void setAccessory(juce::Component* c);

  void paint(juce::Graphics&) override;
  void resized() override;

  std::function<void(const GearItem&)> onPick;
  // Texto do arrasto: "gear:<tipo>:<visual>" (tipo 0 = pedal, 1 = cabeçote, 2 = caixa).
  static juce::String dragDescription(const GearItem& item);

private:
  class List;
  bool matches(const GearItem& item) const;
  void searchChanged();
  void updateTabs();

  Segmented mTabs{{"Pedais", juce::String::fromUTF8("Cabeçotes"), "Caixas"}};
  juce::TextEditor mSearch;
  juce::StringArray mQuery; // palavras da pesquisa, já simplificadas
  std::array<std::set<juce::String>, 3> mCollapsed; // categorias encolhidas em cada aba
  std::unique_ptr<List> mList;
  juce::Viewport mViewport;
  std::vector<GearItem> mItems;
  GearItem::Kind mTab = GearItem::Kind::pedal;
  std::array<int, 3> mScroll{}; // onde cada aba estava (pedais, cabeçotes, caixas)
  juce::Component* mAccessory = nullptr;
  GearItem::Kind mCurrentKind = GearItem::Kind::amp;
  int mCurrentLook = -1;
  bool mHasCabs = true;
};
} // namespace nexus::ui
