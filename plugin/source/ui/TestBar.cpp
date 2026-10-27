#include "TestBar.h"

#include "Language.h"
#include "Theme.h"

namespace nexus::ui
{
namespace
{
juce::String tr(const char* text)
{
  return TRANS(juce::String::fromUTF8(text));
}
constexpr float kPlayWidth = 132.0f;
constexpr float kRecordWidth = 120.0f;
} // namespace

TestBar::TestBar(nexus::TestSource& source) : mSource(source)
{
  addAndMakeVisible(mStyles);
  mStyles.setFontSize(10.5f);
  mStyles.setSelected(static_cast<int>(mSource.style()));
  mStyles.onSelect = [this](int i) {
    const auto s = static_cast<nexus::TestSource::Style>(i);
    if (s == nexus::TestSource::mine && !mSource.hasRecording())
    {
      // Ainda sem gravação: grava agora.
      mSource.startRecording();
      refresh();
      return;
    }
    mSource.setStyle(s);
    mSource.setPlaying(true);
    refresh();
  };
  setMouseCursor(juce::MouseCursor::PointingHandCursor);
  retranslate();
}

juce::StringArray TestBar::styleNames() const
{
  return {tr("Limpo"), tr("Blues"), tr("Rock"), tr("Pesado"), tr("Minha guitarra")};
}

void TestBar::retranslate()
{
  mStyles.setItems(styleNames());
  setTooltip(tr("Toca uma gravação de guitarra pela cadeia, sem precisar tocar. Escolha o estilo que combina com o equipamento, ou grave a sua guitarra."));
  repaint();
}

int TestBar::idealWidth() const
{
  return static_cast<int>(kPlayWidth + kRecordWidth) + 24 + 490;
}

juce::Rectangle<float> TestBar::playArea() const
{
  return getLocalBounds().toFloat().removeFromLeft(kPlayWidth);
}

juce::Rectangle<float> TestBar::recordArea() const
{
  return getLocalBounds().toFloat().removeFromRight(kRecordWidth);
}

void TestBar::resized()
{
  auto r = getLocalBounds();
  r.removeFromLeft(static_cast<int>(kPlayWidth) + 12);
  r.removeFromRight(static_cast<int>(kRecordWidth) + 12);
  mStyles.setBounds(r);
}

void TestBar::refresh()
{
  mSource.poll();
  const bool playing = mSource.playing(), recording = mSource.recording();
  mStyles.setSelected(static_cast<int>(mSource.style()));
  if (playing != mWasPlaying || recording != mWasRecording || recording)
  {
    mWasPlaying = playing;
    mWasRecording = recording;
    repaint();
  }
}

void TestBar::paint(juce::Graphics& g)
{
  const bool playing = mSource.playing(), recording = mSource.recording();
  const auto mouse = getMouseXYRelative().toFloat();
  const bool over = isMouseOver(true);

  // Tocar / parar.
  {
    auto r = playArea().reduced(0.5f);
    const bool hover = over && r.contains(mouse);
    g.setColour(playing ? colour::accent.withAlpha(0.18f) : juce::Colour(hover ? 0xff1d1f24 : 0xff15161a));
    g.fillRoundedRectangle(r, r.getHeight() * 0.5f);
    g.setColour(playing ? colour::accent : (hover ? colour::dim : colour::boxBorder));
    g.drawRoundedRectangle(r, r.getHeight() * 0.5f, 1.0f);
    const float s = r.getHeight() * 0.36f;
    const float cx = r.getX() + 16.0f, cy = r.getCentreY();
    g.setColour(playing ? colour::accent : colour::text);
    if (playing)
      g.fillRect(cx - s * 0.5f, cy - s * 0.5f, s, s);
    else
    {
      juce::Path tri;
      tri.addTriangle(cx - s * 0.45f, cy - s * 0.6f, cx - s * 0.45f, cy + s * 0.6f, cx + s * 0.6f, cy);
      g.fillPath(tri);
    }
    drawCaps(g, playing ? tr("Parar o teste") : tr("Testar o som"), r.withTrimmedLeft(30.0f).withTrimmedRight(8.0f), 10.5f,
             juce::Justification::centredLeft, playing ? colour::accent : colour::text);
  }

  // Gravar a sua guitarra.
  {
    auto r = recordArea().reduced(0.5f);
    const bool hover = over && r.contains(mouse);
    g.setColour(recording ? colour::danger.withAlpha(0.18f) : juce::Colour(hover ? 0xff1d1f24 : 0xff15161a));
    g.fillRoundedRectangle(r, r.getHeight() * 0.5f);
    g.setColour(recording ? colour::danger : (hover ? colour::dim : colour::boxBorder));
    g.drawRoundedRectangle(r, r.getHeight() * 0.5f, 1.0f);
    if (recording)
    {
      // A pílula enche conforme grava.
      auto fill = r.reduced(2.0f);
      g.setColour(colour::danger.withAlpha(0.25f));
      g.fillRoundedRectangle(fill.withWidth(fill.getWidth() * mSource.recordProgress()), fill.getHeight() * 0.5f);
    }
    const float d = 8.0f;
    g.setColour(colour::danger);
    g.fillEllipse(r.getX() + 12.0f, r.getCentreY() - d * 0.5f, d, d);
    const int left = juce::roundToInt((1.0f - mSource.recordProgress()) * static_cast<float>(nexus::TestSource::kRecordSeconds));
    drawCaps(g, recording ? tr("Toque! %1 s").replace("%1", juce::String(left)) : tr("Gravar 15 s"),
             r.withTrimmedLeft(28.0f).withTrimmedRight(8.0f), 10.5f, juce::Justification::centredLeft,
             recording ? colour::danger : colour::text);
  }
}

void TestBar::mouseUp(const juce::MouseEvent& e)
{
  const auto p = e.position;
  if (playArea().contains(p))
    mSource.setPlaying(!mSource.playing());
  else if (recordArea().contains(p))
  {
    if (mSource.recording())
      return;
    mSource.startRecording();
  }
  refresh();
  repaint();
}
} // namespace nexus::ui
