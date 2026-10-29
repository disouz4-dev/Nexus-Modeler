#include "ArteBadge.h"

#include "../update/Updater.h"
#include "Language.h"
#include "Theme.h"

namespace nexus::ui
{
using State = nexus::update::ArteNova::State;

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
  const auto s = mArte->state();
  const bool show = s == State::available || s == State::downloading || s == State::ready || s == State::failed;
  if (show != isVisible())
  {
    setVisible(show);
    if (onLayoutChange)
      onLayoutChange();
  }
  juce::String tip = countText(mArte->count());
  if (s == State::downloading)
    tip = tr("Baixando equipamentos novos") + "  " + juce::String(juce::roundToInt(mArte->progress() * 100.0f)) + "%";
  else if (s == State::ready)
    tip = tr("Equipamentos novos baixados: abra o Nexus de novo para usar");
  else if (s == State::failed)
    tip = mArte->error();
  else
    tip += "\n" + tr("Clique para baixar");
  setTooltip(tip);
  repaint();
}

void ArteBadge::changeListenerCallback(juce::ChangeBroadcaster*)
{
  const auto s = mArte->state();
  const bool done = s == State::ready && mShown != State::ready;
  mShown = s;
  refresh();
  if (done)
    finished();
}

void ArteBadge::ask()
{
  const auto s = mArte->state();
  if (s == State::ready)
  {
    finished();
    return;
  }
  if (s != State::available && s != State::failed)
    return;
  auto names = mArte->names();
  auto list = names.joinIntoString(", ");
  if (names.size() > 12)
  {
    names.removeRange(12, names.size());
    list = names.joinIntoString(", ") + "…";
  }
  juce::AlertWindow::showOkCancelBox(juce::MessageBoxIconType::NoIcon, countText(mArte->count()),
                                     list + "\n\n" + tr("Baixar agora? Eles ficam guardados no computador."), tr("Baixar"),
                                     tr("Agora não"), this, juce::ModalCallbackFunction::create([this](int ok) {
                                       if (ok == 1)
                                         mArte->download();
                                     }));
}

void ArteBadge::finished()
{
  if (juce::JUCEApplicationBase::isStandaloneApp())
    juce::AlertWindow::showOkCancelBox(juce::MessageBoxIconType::NoIcon, tr("Equipamentos novos baixados"),
                                       tr("Abra o Nexus de novo para usar a arte nova."), tr("Reabrir agora"), tr("Depois"), nullptr,
                                       juce::ModalCallbackFunction::create([](int ok) {
                                         if (ok == 1)
                                           nexus::update::Updater::restartProgram();
                                       }));
  else
    juce::AlertWindow::showMessageBoxAsync(juce::MessageBoxIconType::NoIcon, tr("Equipamentos novos baixados"),
                                           tr("Feche e abra de novo a janela do Nexus para usar a arte nova."));
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

  // Quantidade no canto.
  if (s == State::available || s == State::failed || s == State::ready)
  {
    const auto n = mArte->count();
    const auto text = s == State::ready ? juce::String::fromUTF8("✓") : n > 99 ? juce::String("99+") : juce::String(n);
    const float w = juce::jmax(14.0f, 7.0f * static_cast<float>(text.length()) + 6.0f);
    const auto dot = juce::Rectangle<float>(w, 14.0f).withPosition(r.getRight() - w - 1.0f, r.getY() + 1.0f);
    g.setColour(s == State::ready ? colour::accent : colour::danger);
    g.fillRoundedRectangle(dot, 7.0f);
    g.setColour(juce::Colours::white);
    g.setFont(juce::FontOptions(10.0f, juce::Font::bold));
    g.drawText(text, dot, juce::Justification::centred, false);
  }
}

void ArteBadge::mouseUp(const juce::MouseEvent& e)
{
  if (contains(e.getPosition()))
    ask();
}
} // namespace nexus::ui
