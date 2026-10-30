#include "GearBrowser.h"

#include <algorithm>

#include "Language.h"
#include "Theme.h"

namespace nexus::ui
{
namespace
{
constexpr int kHeader = 34;
constexpr int kItem = 88;
constexpr int kPlanned = 30;
constexpr int kPad = 12;
const juce::String kPlannedKey = "\x01planned"; // "Em breve" também encolhe

// Minúsculas, sem acento e só letras e números: "JCM 800", "jcm-800" e
// "JCM800" ficam iguais.
juce::String fold(const juce::String& s)
{
  juce::String out;
  for (auto p = s.getCharPointer(); !p.isEmpty();)
  {
    auto c = p.getAndAdvance();
    if (c >= 0xC0 && c <= 0xDE && c != 0xD7)
      c += 0x20;
    c = juce::CharacterFunctions::toLowerCase(c);
    if (c >= 0xE0 && c <= 0xE5)
      c = 'a';
    else if (c == 0xE7)
      c = 'c';
    else if (c >= 0xE8 && c <= 0xEB)
      c = 'e';
    else if (c >= 0xEC && c <= 0xEF)
      c = 'i';
    else if (c == 0xF1)
      c = 'n';
    else if (c >= 0xF2 && c <= 0xF6)
      c = 'o';
    else if (c >= 0xF9 && c <= 0xFC)
      c = 'u';
    if (juce::CharacterFunctions::isLetterOrDigit(c))
      out += juce::String::charToString(c);
  }
  return out;
}
} // namespace

// Lista rolável: cabeçalho de cada categoria, os equipamentos e, no fim, os planejados.
// Com pesquisa, só o que bate, com todas as categorias abertas.
class GearBrowser::List : public juce::Component
{
public:
  explicit List(GearBrowser& owner) : mOwner(owner) {}

  struct Row
  {
    enum class Type
    {
      header,
      item,
      planned,
      empty
    };
    Type type;
    juce::String text;
    int item = -1;
    juce::Rectangle<int> r;
    juce::String key; // cabeçalho: a categoria que ele encolhe
    int count = 0;    // cabeçalho: quantas peças tem embaixo
    bool collapsed = false;
  };

  void rebuild(int width)
  {
    mRows.clear();
    int y = 6;
    auto add = [&](Row::Type t, const juce::String& text, int item, int h) {
      mRows.push_back({t, text, item, {0, y, width, h}});
      y += h;
    };
    const bool searching = !mOwner.mQuery.isEmpty();
    const auto& collapsed = mOwner.mCollapsed[static_cast<size_t>(mOwner.mTab)];
    // Um grupo: o cabeçalho e as peças embaixo (nenhuma, se estiver encolhido).
    auto group = [&](const juce::String& title, const juce::String& key, Row::Type type, auto&& belongs) {
      std::vector<int> members;
      for (size_t i = 0; i < mOwner.mItems.size(); ++i)
        if (const auto& it = mOwner.mItems[i]; it.kind == mOwner.mTab && belongs(it) && mOwner.matches(it))
          members.push_back(static_cast<int>(i));
      if (members.empty())
        return;
      const bool closed = !searching && collapsed.count(key) > 0;
      add(Row::Type::header, title, -1, kHeader);
      mRows.back().key = key;
      mRows.back().count = static_cast<int>(members.size());
      mRows.back().collapsed = closed;
      if (!closed)
        for (int i : members)
          add(type, {}, i, type == Row::Type::item ? kItem : kPlanned);
      y += 6;
    };
    juce::StringArray categories;
    for (const auto& it : mOwner.mItems)
      if (it.kind == mOwner.mTab && it.available() && !categories.contains(it.category))
        categories.add(it.category);
    for (const auto& cat : categories)
      group(cat, cat, Row::Type::item, [&](const GearItem& it) { return it.available() && it.category == cat; });
    group(TRANS("Em breve"), kPlannedKey, Row::Type::planned, [](const GearItem& it) { return !it.available(); });
    if (mRows.empty() && searching)
      add(Row::Type::empty, TRANS("Nada encontrado."), -1, 60);
    mHover = -1;
    setSize(width, y + 12);
    repaint();
  }

  void paint(juce::Graphics& g) override
  {
    for (size_t i = 0; i < mRows.size(); ++i)
    {
      const auto& row = mRows[i];
      auto r = row.r.toFloat().reduced(static_cast<float>(kPad), 0.0f);
      if (row.type == Row::Type::empty)
      {
        g.setColour(colour::faint);
        g.setFont(Fonts::get().ui(12.0f));
        g.drawText(row.text, r, juce::Justification::centred, true);
        continue;
      }
      if (row.type == Row::Type::header)
      {
        // Seta (aberto aponta para baixo), título e, à direita, quantas peças.
        const bool hover = static_cast<int>(i) == mHover;
        auto line = r.withTrimmedTop(10.0f);
        auto arrow = line.removeFromLeft(14.0f).withSizeKeepingCentre(7.0f, 7.0f);
        juce::Path p;
        p.addTriangle(arrow.getX(), arrow.getY() + 1.0f, arrow.getRight(), arrow.getY() + 1.0f, arrow.getCentreX(), arrow.getBottom() - 0.5f);
        if (row.collapsed)
          p.applyTransform(juce::AffineTransform::rotation(-juce::MathConstants<float>::halfPi, arrow.getCentreX(), arrow.getCentreY()));
        const auto c = hover ? colour::text : colour::dim;
        g.setColour(c.withMultipliedAlpha(0.8f));
        g.fillPath(p);
        g.setColour(colour::faint);
        g.setFont(Fonts::get().ui(10.5f));
        g.drawText(juce::String(row.count), line.removeFromRight(30.0f), juce::Justification::centredRight, false);
        drawCaps(g, row.text, line, 10.0f, juce::Justification::centredLeft, c);
        continue;
      }
      const auto& it = mOwner.mItems[static_cast<size_t>(row.item)];
      if (row.type == Row::Type::planned)
      {
        g.setColour(colour::faint);
        g.setFont(Fonts::get().ui(12.0f));
        g.drawText(it.name, r.removeFromLeft(r.getWidth() * 0.5f), juce::Justification::centredLeft, true);
        g.drawText(it.inspiration, r, juce::Justification::centredRight, true);
        continue;
      }
      // Cartão do equipamento.
      const bool hover = static_cast<int>(i) == mHover;
      const bool current = it.kind == mOwner.mCurrentKind && it.look == mOwner.mCurrentLook;
      auto card = r.reduced(0.0f, 4.0f);
      g.setColour(juce::Colour(current ? 0xff1d2733 : hover ? 0xff1c1d21 : 0xff151619));
      g.fillRoundedRectangle(card, 8.0f);
      if (current)
      {
        g.setColour(colour::accent);
        g.fillRoundedRectangle(card.getX(), card.getY() + 10.0f, 2.5f, card.getHeight() - 20.0f, 1.25f);
      }
      auto inner = card.reduced(12.0f, 8.0f);
      auto thumb = inner.removeFromLeft(it.kind == GearItem::Kind::pedal && it.paintThumb == nullptr ? 52.0f : 92.0f);
      if (it.art != nullptr && it.art->valid())
      {
        const auto& img = it.art->front;
        g.setImageResamplingQuality(juce::Graphics::highResamplingQuality);
        g.drawImage(img, juce::RectanglePlacement(juce::RectanglePlacement::centred).appliedTo(img.getBounds().toFloat(), thumb));
      }
      else if (it.paintThumb)
        it.paintThumb(g, thumb);
      inner.removeFromLeft(12.0f);
      g.setColour(colour::text);
      g.setFont(Fonts::get().display(15.0f).withExtraKerningFactor(language::allowsLetterSpacing() ? 0.08f : 0.0f));
      g.drawText(upper(it.name), inner.removeFromTop(22.0f), juce::Justification::bottomLeft, true);
      g.setColour(colour::dim);
      g.setFont(Fonts::get().ui(11.5f));
      g.drawText(TRANS("inspirado no %1").replace("%1", it.inspiration), inner.removeFromTop(18.0f), juce::Justification::centredLeft, true);
      g.setColour(it.source.containsIgnoreCase("TONE3000") ? colour::accent.withAlpha(0.85f) : colour::faint);
      g.setFont(Fonts::get().ui(10.5f));
      g.drawText(TRANS(it.source), inner.removeFromTop(18.0f), juce::Justification::centredLeft, true);
    }
  }

  // Linha clicável (peça ou cabeçalho) no ponto; só peças com itemsOnly.
  int rowAt(juce::Point<int> p, bool itemsOnly = true) const
  {
    for (size_t i = 0; i < mRows.size(); ++i)
      if ((mRows[i].type == Row::Type::item || (!itemsOnly && mRows[i].type == Row::Type::header)) && mRows[i].r.contains(p))
        return static_cast<int>(i);
    return -1;
  }

  void mouseMove(const juce::MouseEvent& e) override
  {
    const int h = rowAt(e.getPosition(), mOwner.mQuery.isEmpty());
    setMouseCursor(h >= 0 ? juce::MouseCursor::PointingHandCursor : juce::MouseCursor::NormalCursor);
    if (h != mHover)
    {
      mHover = h;
      repaint();
    }
  }
  void mouseExit(const juce::MouseEvent&) override
  {
    mHover = -1;
    repaint();
  }

  void mouseDrag(const juce::MouseEvent& e) override
  {
    const int row = rowAt(e.getMouseDownPosition());
    if (row < 0 || mDragging || e.getDistanceFromDragStart() < 6)
      return;
    const auto& it = mOwner.mItems[static_cast<size_t>(mRows[static_cast<size_t>(row)].item)];
    if (auto* container = juce::DragAndDropContainer::findParentDragContainerFor(this))
    {
      // A miniatura segue o mouse até a cadeia.
      const auto& img = it.art->front;
      const int h = it.kind == GearItem::Kind::pedal ? 90 : 60;
      const int w = juce::roundToInt(static_cast<float>(img.getWidth()) * static_cast<float>(h) / static_cast<float>(img.getHeight()));
      auto thumb = img.rescaled(w, h, juce::Graphics::highResamplingQuality);
      mDragging = true;
      container->startDragging(dragDescription(it), this, juce::ScaledImage(thumb), true);
    }
  }

  void mouseUp(const juce::MouseEvent& e) override
  {
    const bool wasDragging = mDragging;
    mDragging = false;
    if (wasDragging || e.mouseWasDraggedSinceMouseDown())
      return;
    // Cabeçalho: encolhe ou abre a categoria (na pesquisa ficam todas abertas).
    if (const int h = rowAt(e.getPosition(), false); h >= 0 && mRows[static_cast<size_t>(h)].type == Row::Type::header)
    {
      if (!mOwner.mQuery.isEmpty())
        return;
      auto& closed = mOwner.mCollapsed[static_cast<size_t>(mOwner.mTab)];
      const auto& key = mRows[static_cast<size_t>(h)].key;
      if (!closed.erase(key))
        closed.insert(key);
      rebuild(getWidth());
      mHover = rowAt(e.getPosition(), false);
      return;
    }
    const int row = rowAt(e.getPosition());
    if (row >= 0 && mOwner.onPick)
      mOwner.onPick(mOwner.mItems[static_cast<size_t>(mRows[static_cast<size_t>(row)].item)]);
  }

private:
  GearBrowser& mOwner;
  std::vector<Row> mRows;
  int mHover = -1;
  bool mDragging = false;
};

GearBrowser::GearBrowser()
{
  mList = std::make_unique<List>(*this);
  addAndMakeVisible(mTabs);
  mTabs.setFontSize(10.5f);
  mTabs.onSelect = [this](int i) { setTab(static_cast<GearItem::Kind>(i)); };
  addAndMakeVisible(mSearch);
  mSearch.setFont(Fonts::get().ui(12.5f));
  mSearch.setIndents(10, 0);
  mSearch.setJustification(juce::Justification::centredLeft);
  mSearch.setColour(juce::TextEditor::backgroundColourId, colour::box);
  mSearch.setColour(juce::TextEditor::outlineColourId, colour::line);
  mSearch.setColour(juce::TextEditor::focusedOutlineColourId, colour::accent.withAlpha(0.7f));
  mSearch.setColour(juce::TextEditor::textColourId, colour::text);
  mSearch.setColour(juce::TextEditor::highlightColourId, colour::accent.withAlpha(0.35f));
  mSearch.setColour(juce::CaretComponent::caretColourId, colour::accent);
  mSearch.setTextToShowWhenEmpty(TRANS("Pesquisar (nome ou original)"), colour::faint);
  mSearch.onTextChange = [this] { searchChanged(); };
  mSearch.onEscapeKey = [this] {
    mSearch.clear();
    searchChanged();
    mSearch.unfocusAllComponents();
  };
  mSearch.onReturnKey = [this] { mSearch.unfocusAllComponents(); };
  addAndMakeVisible(mViewport);
  mViewport.setViewedComponent(mList.get(), false);
  mViewport.setScrollBarsShown(true, false);
  mViewport.setScrollBarThickness(6);
}

GearBrowser::~GearBrowser() = default;

juce::String GearBrowser::dragDescription(const GearItem& item)
{
  return "gear:" + juce::String(static_cast<int>(item.kind)) + ":" + juce::String(item.look);
}

void GearBrowser::setItems(std::vector<GearItem> items)
{
  mItems = std::move(items);
  mHasCabs = std::any_of(mItems.begin(), mItems.end(), [](const GearItem& it) { return it.kind == GearItem::Kind::cab; });
  if (!mHasCabs && mTab == GearItem::Kind::cab)
    setTab(GearItem::Kind::amp);
  updateTabs();
  resized();
}

void GearBrowser::updateTabs()
{
  juce::StringArray names{TRANS("Pedais"), TRANS(juce::String::fromUTF8("Cabeçotes"))};
  if (mHasCabs)
    names.add(TRANS("Caixas"));
  mTabs.setItems(names);
  mTabs.setSelected(static_cast<int>(mTab));
}

bool GearBrowser::matches(const GearItem& item) const
{
  if (mQuery.isEmpty())
    return true;
  const auto hay = fold(item.name + " " + item.inspiration + " " + item.category + " " + TRANS(item.category) + " " + item.source);
  for (const auto& word : mQuery)
    if (!hay.contains(word))
      return false;
  return true;
}

void GearBrowser::searchChanged()
{
  mQuery.clear();
  for (const auto& word : juce::StringArray::fromTokens(mSearch.getText(), " ,/", ""))
    if (const auto w = fold(word); w.isNotEmpty())
      mQuery.add(w);
  // Nada nesta aba e algo em outra (JCM800 digitado em Pedais): vai para lá.
  auto has = [this](GearItem::Kind k) {
    return std::any_of(mItems.begin(), mItems.end(), [&](const GearItem& it) { return it.kind == k && matches(it); });
  };
  if (!mQuery.isEmpty() && !has(mTab))
    for (auto k : {GearItem::Kind::pedal, GearItem::Kind::amp, GearItem::Kind::cab})
      if (has(k))
      {
        setTab(k);
        break;
      }
  resized();
  mViewport.setViewPosition(0, 0);
}

void GearBrowser::setTab(GearItem::Kind kind)
{
  // A mesma aba (escolher ou soltar uma peça): a lista fica onde está. Outra
  // aba: volta ao ponto em que ela estava da última vez.
  if (kind == mTab || (kind == GearItem::Kind::cab && !mHasCabs))
    return;
  mScroll[static_cast<size_t>(mTab)] = mViewport.getViewPositionY();
  mTab = kind;
  mTabs.setSelected(static_cast<int>(kind));
  resized();
  mViewport.setViewPosition(0, mScroll[static_cast<size_t>(kind)]);
}

void GearBrowser::setCurrent(GearItem::Kind kind, int look)
{
  if (kind == mCurrentKind && look == mCurrentLook)
    return;
  mCurrentKind = kind;
  mCurrentLook = look;
  mList->repaint();
}

void GearBrowser::retranslate()
{
  updateTabs();
  mSearch.setTextToShowWhenEmpty(TRANS("Pesquisar (nome ou original)"), colour::faint);
  resized();
}

void GearBrowser::setAccessory(juce::Component* c)
{
  mAccessory = c;
  if (c != nullptr)
    addChildComponent(c);
  resized();
}

void GearBrowser::resized()
{
  auto r = getLocalBounds();
  r.removeFromTop(16);
  mTabs.setBounds(r.removeFromTop(32).reduced(kPad, 0));
  r.removeFromTop(8);
  auto searchRow = r.removeFromTop(30).reduced(kPad, 0);
  if (mAccessory != nullptr && mAccessory->isVisible())
  {
    mAccessory->setBounds(searchRow.removeFromRight(30));
    searchRow.removeFromRight(6);
  }
  mSearch.setBounds(searchRow);
  r.removeFromTop(6);
  r.removeFromBottom(44); // dica embaixo
  mViewport.setBounds(r);
  mList->rebuild(r.getWidth() - 6);
}

void GearBrowser::paint(juce::Graphics& g)
{
  g.fillAll(juce::Colour(0xff0f1012));
  g.setColour(colour::line);
  g.fillRect(0, 0, 1, getHeight());
  auto hint = getLocalBounds().removeFromBottom(44).reduced(kPad, 8).toFloat();
  g.setColour(colour::faint);
  g.setFont(Fonts::get().ui(10.5f));
  g.drawFittedText(TRANS(juce::String::fromUTF8("Clique para trocar a peça escolhida. Arraste para a cadeia para acrescentar.")), hint.toNearestInt(),
                   juce::Justification::centred, 2);
}
} // namespace nexus::ui
