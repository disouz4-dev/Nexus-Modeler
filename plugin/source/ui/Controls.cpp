#include "Controls.h"

#include "Theme.h"

namespace nexus::ui
{
// ---------------------------------------------------------------- DragValue

DragValue::DragValue(juce::RangedAudioParameter& p, juce::Component& owner)
: mParam(p)
, mAttachment(p, [this, &owner](float v) {
  mValue = mParam.convertTo0to1(v);
  owner.repaint();
  if (onValueChanged)
    onValueChanged();
})
{
  mAttachment.sendInitialUpdate();
}

float DragValue::snapped(float v01) const
{
  v01 = juce::jlimit(0.0f, 1.0f, v01);
  if (mSteps < 2)
    return v01;
  return static_cast<float>(juce::roundToInt(v01 * static_cast<float>(mSteps - 1))) / static_cast<float>(mSteps - 1);
}

int DragValue::step() const
{
  return mSteps < 2 ? 0 : juce::jlimit(0, mSteps - 1, juce::roundToInt(mValue * static_cast<float>(mSteps - 1)));
}

void DragValue::set(float v01)
{
  mAttachment.setValueAsPartOfGesture(mParam.convertFrom0to1(snapped(v01)));
  // Mostra o valor que o parâmetro aceitou: nos botões de estágios, o clique.
  mValue = mParam.getValue();
  if (onValueChanged)
    onValueChanged();
}

void DragValue::mouseDown(const juce::MouseEvent&)
{
  mStartValue = mValue;
  mDragging = true;
  mAttachment.beginGesture();
}

juce::String DragValue::paramId() const
{
  if (auto* p = dynamic_cast<const juce::AudioProcessorParameterWithID*>(&mParam))
    return p->paramID;
  return {};
}

void DragValue::mouseDrag(const juce::MouseEvent& e, bool horizontal)
{
  // 250 pontos de tela percorrem o botão inteiro; Shift deixa 5x mais fino.
  const float range = e.mods.isShiftDown() ? 1250.0f : 250.0f;
  const float dx = static_cast<float>(e.getDistanceFromDragStartX());
  const float dy = static_cast<float>(-e.getDistanceFromDragStartY());
  const float delta = (horizontal ? dx + 0.35f * dy : dy + 0.35f * dx) / range;
  set(mStartValue + delta);
}

void DragValue::mouseUp(const juce::MouseEvent&)
{
  mDragging = false;
  mAttachment.endGesture();
}

void DragValue::mouseDoubleClick()
{
  mAttachment.setValueAsCompleteGesture(mParam.convertFrom0to1(snapped(mParam.getDefaultValue())));
}

void DragValue::mouseWheel(const juce::MouseWheelDetails& w)
{
  const float d = (w.deltaY != 0.0f ? w.deltaY : -w.deltaX) * (w.isReversed ? -1.0f : 1.0f);
  if (mSteps >= 2)
  {
    // Com paradas, cada giro da roda anda uma parada.
    if (d == 0.0f)
      return;
    const int next = juce::jlimit(0, mSteps - 1, step() + (d > 0.0f ? 1 : -1));
    mAttachment.setValueAsCompleteGesture(mParam.convertFrom0to1(static_cast<float>(next) / static_cast<float>(mSteps - 1)));
    return;
  }
  mAttachment.setValueAsCompleteGesture(mParam.convertFrom0to1(juce::jlimit(0.0f, 1.0f, mValue + d * 0.1f)));
}

// ---------------------------------------------------------------- FilmKnob

FilmKnob::FilmKnob(juce::RangedAudioParameter& p, const GearKnobArt& art, juce::String label)
: mArt(art)
, mDrag(p, *this)
, mLabel(std::move(label))
{
  setMouseCursor(juce::MouseCursor::UpDownResizeCursor);
  // O balão com o valor é desenhado pelo palco: avisa a cada clique.
  mDrag.onValueChanged = [this] { changed(); };
}

void FilmKnob::mouseDown(const juce::MouseEvent& e)
{
  if (fixed())
    return;
  if (e.mods.isPopupMenu())
  {
    if (onMidiMenu)
      onMidiMenu(mDrag.paramId(), *this);
    return;
  }
  mDrag.mouseDown(e);
}

void FilmKnob::setCaptureSteps(int count)
{
  if (count == mCaptureSteps)
    return;
  mCaptureSteps = count;
  mDrag.setSteps(count >= 2 ? count : 0);
  setMouseCursor(fixed() ? juce::MouseCursor::NormalCursor : juce::MouseCursor::UpDownResizeCursor);
  repaint();
}

void FilmKnob::paint(juce::Graphics& g)
{
  g.setImageResamplingQuality(juce::Graphics::highResamplingQuality);
  // Com paradas por captura, o botão aparece sempre na parada que toca, mesmo
  // quando o valor vem de um preset, da automação ou do MIDI.
  float shown = mDrag.value01();
  if (fixed())
    shown = 0.5f;
  else if (mCaptureSteps >= 2)
    shown = static_cast<float>(mDrag.step()) / static_cast<float>(mCaptureSteps - 1);
  g.drawImage(mArt.frame(shown), getLocalBounds().toFloat(), juce::RectanglePlacement::stretchToFit);
  if (fixed())
  {
    // Botão morto (uma captura só): parado no meio, com um véu escuro e um
    // cadeado. A frente do cabeçote já traz o botão desenhado por baixo,
    // então deixar o botão transparente não apagaria nada.
    const auto disc = getLocalBounds().toFloat().reduced(getWidth() * 0.1f);
    g.setColour(juce::Colours::black.withAlpha(0.6f));
    g.fillEllipse(disc);
    const float s = disc.getWidth() * 0.3f;
    g.setColour(juce::Colours::white.withAlpha(0.85f));
    g.fillPath(iconPath(Icon::lock, juce::Rectangle<float>(s, s).withCentre(disc.getCentre())));
  }
}

bool FilmKnob::hitTest(int x, int y)
{
  if (mArt.slider)
    return getLocalBounds().contains(x, y);
  const auto c = getLocalBounds().toFloat().getCentre();
  return c.getDistanceFrom({static_cast<float>(x), static_cast<float>(y)}) <= getWidth() * 0.5f;
}

juce::String FilmKnob::bubbleText() const
{
  // Botão de estágios (0 a 10 em cliques) mostra o número inteiro.
  auto& p = mDrag.parameter();
  const float v = p.convertFrom0to1(mDrag.value01());
  const bool stepped = p.getNormalisableRange().interval >= 1.0f;
  if (fixed())
    return TRANS(mLabel) + "  " + TRANS(juce::String::fromUTF8("fixo: este cabeçote tem uma captura só, por enquanto"));
  if (mCaptureSteps >= 2)
    return TRANS(mLabel) + "  " + TRANS("%1 de %2").replace("%1", juce::String(mDrag.step() + 1)).replace("%2", juce::String(mCaptureSteps));
  if (valueText)
    if (const auto custom = valueText(v); custom.isNotEmpty())
      return TRANS(mLabel) + "  " + custom;
  if (mArt.slider)
    return TRANS(mLabel) + "  " + (v > 0.05f ? "+" : "") + juce::String(v, 1).replaceCharacter('.', ',') + " dB";
  return TRANS(mLabel) + "  " + (stepped ? juce::String(juce::roundToInt(v)) : juce::String(v, 1).replaceCharacter('.', ','));
}

// ---------------------------------------------------------------- LevelFader

LevelFader::LevelFader(juce::RangedAudioParameter& p, juce::String label)
: mDrag(p, *this)
, mLabel(std::move(label))
{
  setMouseCursor(juce::MouseCursor::LeftRightResizeCursor);
}

void LevelFader::setMuteParameter(juce::RangedAudioParameter& p)
{
  mMuteParam = &p;
  mMute = std::make_unique<juce::ParameterAttachment>(p, [this](float v) {
    mMuted = v > 0.5f;
    repaint();
  });
  mMute->sendInitialUpdate();
}

juce::Rectangle<float> LevelFader::muteArea() const
{
  if (mMuteParam == nullptr)
    return {};
  const auto font = Fonts::get().ui(10.5f).withExtraKerningFactor(language::allowsLetterSpacing() ? 0.08f : 0.0f);
  const float x = static_cast<float>(juce::GlyphArrangement::getStringWidthInt(font, upper(TRANS(mLabel)))) + 12.0f;
  return {x, -1.0f, 52.0f, 18.0f};
}

bool LevelFader::muted() const
{
  return mMuted || mDrag.parameter().convertFrom0to1(mDrag.value01()) <= mDrag.parameter().getNormalisableRange().start + 0.05f;
}

void LevelFader::mouseDown(const juce::MouseEvent& e)
{
  if (!e.mods.isPopupMenu() && muteArea().contains(e.position))
  {
    mMute->setValueAsCompleteGesture(mMuted ? 0.0f : 1.0f);
    mSkipDrag = true;
    return;
  }
  mSkipDrag = false;
  if (e.mods.isPopupMenu())
  {
    if (onMidiMenu)
      onMidiMenu(mDrag.paramId(), *this);
    return;
  }
  mDrag.mouseDown(e);
}

void LevelFader::paint(juce::Graphics& g)
{
  auto r = getLocalBounds().toFloat();
  auto head = r.removeFromTop(18.0f);
  const float db = mDrag.parameter().convertFrom0to1(mDrag.value01());
  drawCaps(g, TRANS(mLabel), head, 10.5f, juce::Justification::centredLeft, colour::dim);
  if (mMuteParam != nullptr)
  {
    const auto m = muteArea().reduced(0.5f);
    g.setColour(mMuted ? colour::danger.withAlpha(0.18f) : juce::Colour(0xff15161a));
    g.fillRoundedRectangle(m, m.getHeight() * 0.5f);
    g.setColour(mMuted ? colour::danger : colour::boxBorder);
    g.drawRoundedRectangle(m, m.getHeight() * 0.5f, 1.0f);
    drawCaps(g, TRANS("Mudo"), m, 9.0f, juce::Justification::centred, mMuted ? colour::danger : colour::dim);
  }
  const bool off = muted();
  g.setColour(off ? colour::danger : colour::text);
  g.setFont(Fonts::get().ui(12.0f));
  g.drawText(off ? TRANS("no mudo") : (db > 0.05f ? "+" : "") + juce::String(db, 1).replaceCharacter('.', ',') + " dB", head,
             juce::Justification::centredRight);

  // Trilho do ajuste com a alça.
  auto track = r.removeFromTop(16.0f).withSizeKeepingCentre(r.getWidth(), 3.0f);
  g.setColour(colour::line);
  g.fillRoundedRectangle(track, 1.5f);
  const float hx = track.getX() + track.getWidth() * mDrag.value01();
  const float zeroX = track.getX() + track.getWidth() * mDrag.parameter().convertTo0to1(0.0f);
  g.setColour(colour::text.withAlpha(0.35f));
  g.fillRect(juce::Rectangle<float>::leftTopRightBottom(std::min(hx, zeroX), track.getY(), std::max(hx, zeroX), track.getBottom()));
  g.setColour(colour::faint);
  g.fillRect(zeroX - 0.5f, track.getY() - 3.0f, 1.0f, track.getHeight() + 6.0f);
  g.setColour(colour::selected);
  g.fillRoundedRectangle(juce::Rectangle<float>(hx - 2.0f, track.getCentreY() - 7.0f, 4.0f, 14.0f), 2.0f);

  // Medidor de pico: barra fina embaixo, dourada, vermelha perto do limite.
  auto meter = r.removeFromTop(8.0f).withSizeKeepingCentre(r.getWidth(), 2.0f);
  g.setColour(colour::line.darker(0.3f));
  g.fillRect(meter);
  const float peakDb = juce::Decibels::gainToDecibels(mLevel, -60.0f);
  const float fill = juce::jlimit(0.0f, 1.0f, (peakDb + 60.0f) / 60.0f);
  g.setColour(peakDb > -1.0f ? colour::danger : colour::accent);
  g.fillRect(meter.withWidth(meter.getWidth() * fill));
}

// ---------------------------------------------------------------- Segmented

juce::Rectangle<float> Segmented::itemBounds(int i) const
{
  const float w = static_cast<float>(getWidth()) / static_cast<float>(mItems.size());
  return {static_cast<float>(i) * w, 0.0f, w, static_cast<float>(getHeight())};
}

void Segmented::paint(juce::Graphics& g)
{
  auto r = getLocalBounds().toFloat().reduced(0.5f);
  const float radius = r.getHeight() * 0.5f;
  g.setColour(juce::Colour(0xff111113));
  g.fillRoundedRectangle(r, radius);
  g.setColour(colour::boxBorder);
  g.drawRoundedRectangle(r, radius, 1.0f);
  const auto font = Fonts::get().ui(mFontSize).withExtraKerningFactor(language::allowsLetterSpacing() ? 0.06f : 0.0f);
  for (int i = 0; i < mItems.size(); ++i)
  {
    auto b = itemBounds(i).reduced(3.0f);
    const bool sel = i == mSelected;
    if (sel)
    {
      g.setColour(juce::Colour(0xff2b2b30));
      g.fillRoundedRectangle(b, b.getHeight() * 0.5f);
      g.setColour(colour::accent);
      g.fillRoundedRectangle(b.getCentreX() - 9.0f, b.getBottom() - 3.0f, 18.0f, 2.0f, 1.0f);
    }
    g.setFont(font);
    g.setColour(sel ? colour::selected : (mEnabled[i] ? colour::dim : colour::faint.withAlpha(0.6f)));
    if (i == mLocked)
    {
      const float tw = static_cast<float>(juce::GlyphArrangement::getStringWidthInt(font, upper(mItems[i])));
      g.fillPath(iconPath(Icon::lock, {b.getCentreX() - tw * 0.5f - 16.0f, b.getCentreY() - 6.5f, 10.0f, 12.0f}));
      b = b.withTrimmedLeft(14.0f);
    }
    g.drawText(upper(mItems[i]), b, juce::Justification::centred);
  }
}

void Segmented::mouseUp(const juce::MouseEvent& e)
{
  if (e.mods.isPopupMenu())
  {
    if (onRightClick)
      onRightClick();
    return;
  }
  for (int i = 0; i < mItems.size(); ++i)
    if (itemBounds(i).contains(e.position) && mEnabled[i] && onSelect)
      onSelect(i);
}

// ---------------------------------------------------------------- ícones

juce::Path iconPath(Icon icon, juce::Rectangle<float> r)
{
  juce::Path p;
  const float x = r.getX(), y = r.getY(), w = r.getWidth(), h = r.getHeight();
  auto stroke = [&](const juce::Path& src, float t) {
    juce::Path out;
    juce::PathStrokeType(t, juce::PathStrokeType::curved, juce::PathStrokeType::rounded).createStrokedPath(out, src);
    return out;
  };
  switch (icon)
  {
  case Icon::steps:
  {
    juce::Path s;
    s.startNewSubPath(x, y + h * 0.9f);
    s.lineTo(x + w * 0.25f, y + h * 0.9f);
    s.lineTo(x + w * 0.25f, y + h * 0.63f);
    s.lineTo(x + w * 0.5f, y + h * 0.63f);
    s.lineTo(x + w * 0.5f, y + h * 0.37f);
    s.lineTo(x + w * 0.75f, y + h * 0.37f);
    s.lineTo(x + w * 0.75f, y + h * 0.1f);
    s.lineTo(x + w, y + h * 0.1f);
    return stroke(s, 1.7f);
  }
  case Icon::info:
  {
    juce::Path s;
    s.addEllipse(r.reduced(1.0f));
    s.startNewSubPath(x + w * 0.5f, y + h * 0.46f);
    s.lineTo(x + w * 0.5f, y + h * 0.72f);
    auto out = stroke(s, 1.5f);
    out.addEllipse(x + w * 0.5f - 1.3f, y + h * 0.28f - 1.3f, 2.6f, 2.6f);
    return out;
  }
  case Icon::left:
  case Icon::right:
  {
    const bool left = icon == Icon::left;
    juce::Path s;
    s.startNewSubPath(x + w * (left ? 0.65f : 0.35f), y + h * 0.2f);
    s.lineTo(x + w * (left ? 0.35f : 0.65f), y + h * 0.5f);
    s.lineTo(x + w * (left ? 0.65f : 0.35f), y + h * 0.8f);
    return stroke(s, 1.8f);
  }
  case Icon::globe:
  {
    // Globo: círculo, meridiano e paralelos.
    juce::Path s;
    s.addEllipse(r.reduced(1.0f));
    s.addEllipse(x + w * 0.3f, y + 1.0f, w * 0.4f, h - 2.0f);
    s.startNewSubPath(x + 1.0f, y + h * 0.5f);
    s.lineTo(x + w - 1.0f, y + h * 0.5f);
    s.startNewSubPath(x + w * 0.14f, y + h * 0.28f);
    s.lineTo(x + w * 0.86f, y + h * 0.28f);
    s.startNewSubPath(x + w * 0.14f, y + h * 0.72f);
    s.lineTo(x + w * 0.86f, y + h * 0.72f);
    return stroke(s, 1.3f);
  }
  case Icon::audio:
  {
    // Interface de áudio vista de frente: caixa com duas entradas e um botão.
    juce::Path s;
    s.addRoundedRectangle(x, y + h * 0.25f, w, h * 0.5f, 2.0f);
    auto out = stroke(s, 1.5f);
    out.addEllipse(x + w * 0.16f, y + h * 0.40f, w * 0.2f, h * 0.2f);
    out.addEllipse(x + w * 0.42f, y + h * 0.40f, w * 0.2f, h * 0.2f);
    juce::Path k;
    k.addEllipse(x + w * 0.70f, y + h * 0.37f, w * 0.17f, h * 0.26f);
    out.addPath(stroke(k, 1.4f));
    return out;
  }
  case Icon::download:
  {
    // Nuvem com uma seta para baixo: há uma versão nova para baixar.
    juce::Path s;
    s.startNewSubPath(x + w * 0.30f, y + h * 0.74f);
    s.lineTo(x + w * 0.24f, y + h * 0.74f);
    s.cubicTo(x - w * 0.02f, y + h * 0.74f, x + w * 0.02f, y + h * 0.38f, x + w * 0.24f, y + h * 0.40f);
    s.cubicTo(x + w * 0.28f, y + h * 0.06f, x + w * 0.72f, y + h * 0.06f, x + w * 0.76f, y + h * 0.36f);
    s.cubicTo(x + w * 1.02f, y + h * 0.36f, x + w * 1.02f, y + h * 0.74f, x + w * 0.76f, y + h * 0.74f);
    s.lineTo(x + w * 0.70f, y + h * 0.74f);
    s.startNewSubPath(x + w * 0.5f, y + h * 0.42f);
    s.lineTo(x + w * 0.5f, y + h * 0.96f);
    s.startNewSubPath(x + w * 0.36f, y + h * 0.82f);
    s.lineTo(x + w * 0.5f, y + h * 0.96f);
    s.lineTo(x + w * 0.64f, y + h * 0.82f);
    return stroke(s, 1.6f);
  }
  case Icon::lock:
  {
    p.addRoundedRectangle(x, y + h * 0.45f, w, h * 0.55f, 1.5f);
    juce::Path shackle;
    shackle.addCentredArc(x + w * 0.5f, y + h * 0.45f, w * 0.3f, h * 0.3f, 0.0f, -juce::MathConstants<float>::halfPi,
                          juce::MathConstants<float>::halfPi, true);
    p.addPath(stroke(shackle, 1.6f));
    return p;
  }
  }
  return p;
}

void IconButton::paint(juce::Graphics& g)
{
  auto r = getLocalBounds().toFloat();
  if (mOn || mHover)
  {
    g.setColour(mOn ? colour::accent.withAlpha(0.14f) : colour::text.withAlpha(0.06f));
    g.fillEllipse(r.reduced(4.0f));
  }
  g.setColour(mOn ? colour::accent : (mHover ? colour::text : colour::dim));
  g.fillPath(iconPath(mIcon, r.withSizeKeepingCentre(18.0f, 18.0f)));
}

// ---------------------------------------------------------------- TextLink

int TextLink::idealWidth() const
{
  return juce::GlyphArrangement::getStringWidthInt(Fonts::get().ui(12.5f), mText) + 4;
}

void TextLink::paint(juce::Graphics& g)
{
  g.setFont(Fonts::get().ui(12.5f));
  g.setColour(mHover ? colour::selected : colour::accent);
  g.drawText(mText, getLocalBounds(), juce::Justification::centred);
  if (mHover)
    g.fillRect(2.0f, static_cast<float>(getHeight()) - 3.0f, static_cast<float>(getWidth()) - 4.0f, 1.0f);
}
} // namespace nexus::ui
