#include "GearBrowser.h"

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
} // namespace

// Lista rolável: cabeçalho de cada categoria, os equipamentos e, no fim, os planejados.
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
      planned
    };
    Type type;
    juce::String text;
    int item = -1;
    juce::Rectangle<int> r;
  };

  void rebuild(int width)
  {
    mRows.clear();
    int y = 6;
    auto add = [&](Row::Type t, const juce::String& text, int item, int h) {
      mRows.push_back({t, text, item, {0, y, width, h}});
      y += h;
    };
    juce::StringArray categories;
    for (const auto& it : mOwner.mItems)
      if (it.kind == mOwner.mTab && it.available() && !categories.contains(it.category))
        categories.add(it.category);
    for (const auto& cat : categories)
    {
      add(Row::Type::header, cat, -1, kHeader);
      for (size_t i = 0; i < mOwner.mItems.size(); ++i)
        if (const auto& it = mOwner.mItems[i]; it.kind == mOwner.mTab && it.available() && it.category == cat)
          add(Row::Type::item, {}, static_cast<int>(i), kItem);
      y += 6;
    }
    bool plannedHeader = false;
    for (size_t i = 0; i < mOwner.mItems.size(); ++i)
      if (const auto& it = mOwner.mItems[i]; it.kind == mOwner.mTab && !it.available())
      {
        if (!plannedHeader)
        {
          add(Row::Type::header, TRANS("Em breve"), -1, kHeader);
          plannedHeader = true;
        }
        add(Row::Type::planned, {}, static_cast<int>(i), kPlanned);
      }
    setSize(width, y + 12);
    repaint();
  }

  void paint(juce::Graphics& g) override
  {
    for (size_t i = 0; i < mRows.size(); ++i)
    {
      const auto& row = mRows[i];
      auto r = row.r.toFloat().reduced(static_cast<float>(kPad), 0.0f);
      if (row.type == Row::Type::header)
      {
        drawCaps(g, row.text, r.withTrimmedTop(10.0f), 10.0f, juce::Justification::centredLeft, colour::dim);
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

  int rowAt(juce::Point<int> p) const
  {
    for (size_t i = 0; i < mRows.size(); ++i)
      if (mRows[i].type == Row::Type::item && mRows[i].r.contains(p))
        return static_cast<int>(i);
    return -1;
  }

  void mouseMove(const juce::MouseEvent& e) override
  {
    const int h = rowAt(e.getPosition());
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
  resized();
}

void GearBrowser::setTab(GearItem::Kind kind)
{
  mTab = kind;
  mTabs.setSelected(static_cast<int>(kind));
  mViewport.setViewPosition(0, 0);
  resized();
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
  mTabs.setItems({TRANS("Pedais"), TRANS(juce::String::fromUTF8("Cabeçotes")), TRANS("Caixas")});
  resized();
}

void GearBrowser::resized()
{
  auto r = getLocalBounds();
  r.removeFromTop(16);
  mTabs.setBounds(r.removeFromTop(32).reduced(kPad, 0));
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
