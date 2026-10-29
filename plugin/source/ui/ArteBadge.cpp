#include "ArteBadge.h"

#include "../update/Updater.h"
#include "Language.h"
#include "Theme.h"

namespace nexus::ui
{
using State = nexus::update::GearStore::State;

namespace
{
juce::String tr(const char* text)
{
  return TRANS(juce::String::fromUTF8(text));
}

juce::String countText(int n)
{
  return n == 1 ? tr("1 equipamento novo") : tr("%1 equipamentos novos").replace("%1", juce::String(n));
}
} // namespace

ArteBadge::ArteBadge()
{
  setMouseCursor(juce::MouseCursor::PointingHandCursor);
  mArte->addChangeListener(this);
  mArte->checkIfDue();
  refresh();
}

ArteBadge::~ArteBadge()
{
  mArte->removeChangeListener(this);
}

void ArteBadge::refresh()
{
  // Só mostra o estado: procurando, baixando, quantas chegaram ou o erro.
  const auto s = mArte->state();
  const bool show = s == State::checking || s == State::available || s == State::downloading || s == State::ready || s == State::failed;
  if (show != isVisible())
  {
    setVisible(show);
    if (onLayoutChange)
      onLayoutChange();
  }
  juce::String tip;
  if (s == State::checking)
    tip = tr("Procurando equipamentos novos");
  else if (s == State::downloading || s == State::available)
    tip = tr("Baixando equipamentos novos") + "  " + juce::String(juce::roundToInt(mArte->progress() * 100.0f)) + "%";
  else if (s == State::ready)
  {
    auto names = mArte->names();
    if (names.size() > 12)
    {
      names.removeRange(12, names.size());
      names.add(juce::String::fromUTF8("…"));
    }
    tip = (mArte->count() == 1 ? tr("Chegou 1 equipamento novo") : tr("Chegaram %1 equipamentos novos").replace("%1", juce::String(mArte->count())))
          + ": " + names.joinIntoString(", ");
    if (mArte->needsRestart())
      tip += "\n" + tr("A arte nova das peças que já existiam vale ao abrir o Nexus de novo.");
  }
  else if (s == State::failed)
    tip = mArte->error() + "\n" + tr("Clique para tentar de novo");
  setTooltip(tip);
  repaint();
}

void ArteBadge::changeListenerCallback(juce::ChangeBroadcaster*)
{
  mShown = mArte->state();
  refresh();
}

void ArteBadge::mouseUp(const juce::MouseEvent&)
{
  // Sem confirmação: só o erro aceita clique, para tentar de novo.
  if (mArte->state() == State::failed)
    mArte->download();
}

void ArteBadge::paint(juce::Graphics& g)
{
  auto r = getLocalBounds().toFloat();
  const auto s = mArte->state();
  const auto tint = s == State::failed ? colour::danger : colour::accent;
  g.setColour(tint.withAlpha(isMouseOver(true) ? 0.24f : 0.14f));
  g.fillEllipse(r.reduced(4.0f));

  if (s == State::downloading)
  {
    const auto ring = r.reduced(3.0f);
    juce::Path arc;
    arc.addCentredArc(ring.getCentreX(), ring.getCentreY(), ring.getWidth() * 0.5f, ring.getHeight() * 0.5f, 0.0f, 0.0f,
                      juce::MathConstants<float>::twoPi * juce::jlimit(0.0f, 1.0f, mArte->progress()), true);
    g.setColour(tint.withAlpha(0.25f));
    g.drawEllipse(ring, 2.0f);
    g.setColour(tint);
    g.strokePath(arc, juce::PathStrokeType(2.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
  }

  // Amplificador: caixa, faixa do painel com três botões e a tela embaixo.
  const auto amp = r.withSizeKeepingCentre(20.0f, 15.0f);
  g.setColour(tint);
  g.drawRoundedRectangle(amp, 2.0f, 1.6f);
  g.fillRect(amp.withHeight(4.0f).reduced(2.5f, 0.0f).translated(0.0f, 2.5f));
  g.setColour(tint.withAlpha(0.55f));
  for (int i = 0; i < 3; ++i)
    g.drawHorizontalLine(juce::roundToInt(amp.getY() + 9.0f + 2.0f * static_cast<float>(i)), amp.getX() + 3.0f, amp.getRight() - 3.0f);

  // No canto: quantas chegam (ou chegaram) e "!" no erro.
  if (s == State::available || s == State::downloading || s == State::failed || s == State::ready)
  {
    const auto n = mArte->count();
    const auto text = s == State::failed ? juce::String("!") : n > 99 ? juce::String("99+") : juce::String(n);
    const float w = juce::jmax(14.0f, 7.0f * static_cast<float>(text.length()) + 6.0f);
    const auto dot = juce::Rectangle<float>(w, 14.0f).withPosition(r.getRight() - w - 1.0f, r.getY() + 1.0f);
    g.setColour(s == State::failed ? colour::danger : colour::accent);
    g.fillRoundedRectangle(dot, 7.0f);
    g.setColour(juce::Colours::white);
    g.setFont(juce::FontOptions(10.0f, juce::Font::bold));
    g.drawText(text, dot, juce::Justification::centred, false);
  }
}
} // namespace nexus::ui
