#include "WhatsNew.h"

#include "../update/Updater.h"
#include "BinaryData.h"
#include "Theme.h"

namespace nexus::ui::whatsnew
{
namespace
{
constexpr int kWidth = 560;
constexpr int kMaxHeight = 420;

juce::String tidy(juce::String s)
{
  // Só texto: tira o negrito e o código do Markdown.
  return s.replace("**", "").replace("`", "").trim();
}

// O texto da seção, desenhado: títulos "###", itens "-" (e "  -" dentro de
// um item) e parágrafos. Linhas recuadas continuam o bloco de cima.
class Content : public juce::Component
{
public:
  explicit Content(const juce::String& markdown)
  {
    for (auto line : juce::StringArray::fromLines(markdown))
    {
      const auto t = line.trim();
      if (t.isEmpty())
      {
        mOpen = false;
        continue;
      }
      if (t.startsWith("#"))
        add(Block::heading, t.trimCharactersAtStart("# "));
      else if (t.startsWith("- ") || t.startsWith("* "))
        add(line.startsWith(" ") ? Block::subItem : Block::item, t.substring(2));
      else if (mOpen && (line.startsWith(" ") || mBlocks.back().type == Block::paragraph))
        mBlocks.back().text << " " << t;
      else
        add(Block::paragraph, t);
    }
    layout();
  }

private:
  struct Block
  {
    enum Type
    {
      heading,
      item,
      subItem,
      paragraph
    } type;
    juce::String text;
    juce::TextLayout layout;
    juce::Rectangle<float> r;
  };

  void add(Block::Type type, const juce::String& text)
  {
    mBlocks.push_back({type, text, {}, {}});
    mOpen = true;
  }

  void layout()
  {
    float y = 4.0f;
    for (auto& b : mBlocks)
    {
      const float indent = b.type == Block::item ? 18.0f : b.type == Block::subItem ? 36.0f : 0.0f;
      if (b.type == Block::heading && &b != &mBlocks.front())
        y += 12.0f;
      juce::AttributedString s;
      s.setWordWrap(juce::AttributedString::byWord);
      if (b.type == Block::heading)
        s.append(upper(tidy(b.text)), Fonts::get().display(14.0f).withExtraKerningFactor(language::allowsLetterSpacing() ? 0.06f : 0.0f),
                 colour::accent);
      else
        s.append(tidy(b.text), Fonts::get().ui(13.0f), b.type == Block::subItem ? colour::dim : colour::text);
      const float width = static_cast<float>(kWidth - 16) - indent;
      b.layout.createLayout(s, width);
      b.r = {indent, y, width, b.layout.getHeight()};
      y += b.layout.getHeight() + (b.type == Block::heading ? 6.0f : 5.0f);
    }
    setSize(kWidth - 10, juce::roundToInt(y) + 4);
  }

  void paint(juce::Graphics& g) override
  {
    for (const auto& b : mBlocks)
    {
      if (b.type == Block::item || b.type == Block::subItem)
      {
        g.setColour(b.type == Block::item ? colour::accent : colour::faint);
        const float d = b.type == Block::item ? 5.0f : 4.0f;
        g.fillEllipse(b.r.getX() - 12.0f, b.r.getY() + 7.0f, d, d);
      }
      b.layout.draw(g, b.r);
    }
  }

  std::vector<Block> mBlocks;
  bool mOpen = false;
};
} // namespace

juce::String section(const juce::String& markdown, const juce::String& version)
{
  const auto wanted = version.trimCharactersAtStart("vV");
  auto lines = juce::StringArray::fromLines(markdown);
  auto find = [&](bool exact) {
    for (int i = 0; i < lines.size(); ++i)
      if (lines[i].startsWith("## "))
      {
        const auto v = lines[i].substring(3).trim().upToFirstOccurrenceOf(" ", false, false).trimCharactersAtStart("vV");
        if (exact ? v == wanted : v.startsWith(wanted))
          return i;
      }
    return -1;
  };
  int start = find(true);
  if (start < 0)
    start = find(false);
  if (start < 0 || wanted.isEmpty())
    return {};
  juce::StringArray out;
  for (int i = start + 1; i < lines.size() && !lines[i].startsWith("## "); ++i)
    out.add(lines[i]);
  return out.joinIntoString("\n").trim();
}

juce::String bundled()
{
  return section(juce::String::fromUTF8(BinaryData::NOTASDEVERSAO_md, BinaryData::NOTASDEVERSAO_mdSize), nexus::update::currentVersion());
}

void show(const juce::String& title, const juce::String& notes, const juce::String& ok, const juce::String& cancel,
          std::function<void(bool)> done, juce::Component* around)
{
  auto* w = new juce::AlertWindow(title, {}, juce::MessageBoxIconType::NoIcon, around);
  auto* view = new juce::Viewport();
  auto* content = new Content(notes);
  view->setViewedComponent(content, true);
  view->setScrollBarsShown(true, false);
  view->setScrollBarThickness(6);
  view->setSize(kWidth, juce::jmin(kMaxHeight, content->getHeight()));
  w->addCustomComponent(view);
  if (cancel.isNotEmpty())
  {
    w->addButton(ok, 1, juce::KeyPress(juce::KeyPress::returnKey));
    w->addButton(cancel, 0, juce::KeyPress(juce::KeyPress::escapeKey));
  }
  else
    w->addButton(ok, 1, juce::KeyPress(juce::KeyPress::returnKey), juce::KeyPress(juce::KeyPress::escapeKey));
  // A janela não é dona da área das notas: ela vai junto quando a janela fecha.
  w->enterModalState(true, juce::ModalCallbackFunction::create([view, done](int result) {
                       delete view;
                       if (done)
                         done(result == 1);
                     }),
                     true);
}
} // namespace nexus::ui::whatsnew
