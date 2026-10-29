#include "EqStage.h"

#include "../PluginProcessor.h"
#include "Theme.h"
#include "nexus/UniversalEq.h"

namespace nexus::ui
{
namespace
{
juce::String tr(const char* text)
{
  return TRANS(juce::String::fromUTF8(text));
}

juce::String commaNumber(double v, int decimals)
{
  return juce::String(v, decimals).replaceCharacter('.', ',');
}

juce::String formatFreq(float f)
{
  return f < 1000.0f ? juce::String(juce::roundToInt(f)) + " Hz" : commaNumber(f / 1000.0, f < 10000.0f ? 2 : 1) + " kHz";
}

juce::String formatDb(float db)
{
  return (db > 0.05f ? "+" : "") + commaNumber(db, 1) + " dB";
}

// Uma cor por faixa (em ciclo), para achar o ponto e o botão dela.
juce::Colour bandColour(int band)
{
  static const juce::Colour c[] = {juce::Colour(0xff29b6ff), juce::Colour(0xfff5a623), juce::Colour(0xff7ed957), juce::Colour(0xffff6b8b),
                                   juce::Colour(0xffb388ff), juce::Colour(0xff4dd0c4), juce::Colour(0xffffd54f), juce::Colour(0xffff8a65)};
  return c[band % 8];
}

constexpr double kMinFreq = 10.0, kMaxFreq = 22000.0, kRangeDb = 24.0, kSampleRate = 48000.0;
constexpr int kBands = nexus::FreeEq::kMaxBands;

const char* typeName(int t)
{
  static const char* names[] = {"Sino", "Prateleira de graves", "Prateleira de agudos", "Corte de graves", "Corte de agudos", "Notch"};
  return names[juce::jlimit(0, 5, t)];
}

nexus::FreeEq::Bands readBands(juce::AudioProcessorValueTreeState& p)
{
  nexus::FreeEq::Bands b;
  for (int i = 0; i < kBands; ++i)
  {
    auto v = [&](const char* name) { return p.getRawParameterValue(NexusProcessor::eqParam(i, name))->load(); };
    auto& x = b[static_cast<size_t>(i)];
    x.on = v("ligado") > 0.5f;
    x.type = static_cast<nexus::FreeEq::Type>(juce::roundToInt(v("tipo")));
    x.freq = v("freq");
    x.gainDb = v("ganho");
    x.q = v("q");
    x.slope = juce::roundToInt(v("inclinacao")) + 1;
  }
  return b;
}
} // namespace

// ---------------------------------------------------------------- PanelKnob

PanelKnob::PanelKnob(juce::RangedAudioParameter& p, juce::String label, juce::Colour cap, std::function<juce::String(float)> format)
: mDrag(p, *this)
, mLabel(std::move(label))
, mCap(cap)
, mFormat(std::move(format))
{
  setMouseCursor(juce::MouseCursor::UpDownResizeCursor);
  mDrag.onValueChanged = [this] {
    if (onChange)
      onChange();
  };
}

void PanelKnob::paint(juce::Graphics& g)
{
  auto r = getLocalBounds().toFloat();
  auto text = r.removeFromBottom(16.0f);
  const float d = std::min(r.getWidth(), r.getHeight()) - 10.0f;
  const auto c = r.getCentre();
  const float rad = d * 0.5f;
  // Arco do valor, como nos plugins modernos.
  const float a0 = juce::degreesToRadians(-135.0f), a1 = juce::degreesToRadians(135.0f);
  const float av = a0 + (a1 - a0) * mDrag.value01();
  juce::Path track, fill;
  track.addCentredArc(c.x, c.y, rad + 4.0f, rad + 4.0f, 0.0f, a0, a1, true);
  fill.addCentredArc(c.x, c.y, rad + 4.0f, rad + 4.0f, 0.0f, a0, av, true);
  g.setColour(juce::Colour(0xff26282d));
  g.strokePath(track, juce::PathStrokeType(2.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
  g.setColour(mCap);
  g.strokePath(fill, juce::PathStrokeType(2.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
  g.setGradientFill(juce::ColourGradient(juce::Colour(0xff34363b), c.x, c.y - rad, juce::Colour(0xff111214), c.x, c.y + rad, false));
  g.fillEllipse(c.x - rad, c.y - rad, d, d);
  g.setColour(juce::Colours::white.withAlpha(0.08f));
  g.drawEllipse(c.x - rad, c.y - rad, d, d, 1.0f);
  g.setColour(juce::Colours::white);
  juce::Path line;
  line.addRoundedRectangle(-1.2f, -rad * 0.9f, 2.4f, rad * 0.5f, 1.2f);
  g.fillPath(line, juce::AffineTransform::rotation(av).translated(c.x, c.y));
  const bool show = mHover || mDrag.dragging();
  const float v = mDrag.parameter().convertFrom0to1(mDrag.value01());
  drawCaps(g, show ? mFormat(v) : mLabel, text, 9.5f, juce::Justification::centred, show ? mCap : colour::dim);
}

// ---------------------------------------------------------------- EqStage

EqStage::EqStage(juce::AudioProcessorValueTreeState& params) : mParams(params)
{
  for (int i = 0; i < kBands; ++i)
    if (bandOn(i))
    {
      select(i);
      break;
    }
  startTimerHz(30);
}

EqStage::~EqStage() = default;

float EqStage::value(const juce::String& id) const
{
  return mParams.getRawParameterValue(id)->load();
}

juce::RangedAudioParameter& EqStage::param(const juce::String& id) const
{
  return *mParams.getParameter(id);
}

void EqStage::setParam(const juce::String& id, float plain)
{
  auto& p = param(id);
  p.setValueNotifyingHost(p.convertTo0to1(plain));
}

bool EqStage::eqOn() const
{
  return value("eq_ligado") > 0.5f;
}

void EqStage::toggleBypass()
{
  auto& p = param("eq_ligado");
  p.beginChangeGesture();
  p.setValueNotifyingHost(eqOn() ? 0.0f : 1.0f);
  p.endChangeGesture();
  repaint();
}

bool EqStage::bandOn(int band) const
{
  return value(NexusProcessor::eqParam(band, "ligado")) > 0.5f;
}

int EqStage::bandType(int band) const
{
  return juce::roundToInt(value(NexusProcessor::eqParam(band, "tipo")));
}

bool EqStage::hasGain(int band) const
{
  const int t = bandType(band);
  return t == 0 || t == 1 || t == 2;
}

void EqStage::clearBands()
{
  for (int i = 0; i < kBands; ++i)
    if (bandOn(i))
      deleteBand(i);
  select(-1);
}

void EqStage::resized()
{
  auto r = getLocalBounds().toFloat().reduced(36.0f, 18.0f);
  mStrip = r.removeFromBottom(108.0f);
  mBypass = mStrip.withTrimmedLeft(mStrip.getWidth() - 200.0f).reduced(24.0f, 0.0f).withSizeKeepingCentre(152.0f, 40.0f);
  r.removeFromBottom(12.0f);
  mCurveArea = r.reduced(10.0f, 8.0f);
  select(mSelected); // reposiciona os botões da faixa
}

void EqStage::select(int band)
{
  mSelected = band;
  mKnobs.clear();
  if (band >= 0)
  {
    const auto c = bandColour(band);
    auto add = [&](const char* name, const char* label, std::function<juce::String(float)> f) {
      auto k = std::make_unique<PanelKnob>(param(NexusProcessor::eqParam(band, name)), tr(label), c, std::move(f));
      k->onChange = [this] { repaint(); };
      addAndMakeVisible(*k);
      mKnobs.push_back(std::move(k));
    };
    add("freq", "Frequência", formatFreq);
    if (hasGain(band))
      add("ganho", "Ganho", formatDb);
    add("q", bandType(band) >= 3 && bandType(band) <= 4 ? "Ressonância" : "Largura", [](float q) { return "Q " + commaNumber(q, 2); });
    auto knobs = mStrip.withTrimmedLeft(300.0f).withTrimmedRight(260.0f);
    const float w = 92.0f;
    for (size_t i = 0; i < mKnobs.size(); ++i)
      mKnobs[i]->setBounds(juce::Rectangle<float>(knobs.getX() + w * static_cast<float>(i), knobs.getY() + 8.0f, w, knobs.getHeight() - 12.0f).toNearestInt());
  }
  repaint();
}

float EqStage::xOfFreq(double f) const
{
  const double t = std::log(f / kMinFreq) / std::log(kMaxFreq / kMinFreq);
  return mCurveArea.getX() + static_cast<float>(t) * mCurveArea.getWidth();
}

double EqStage::freqOfX(float x) const
{
  const double t = juce::jlimit(0.0, 1.0, static_cast<double>((x - mCurveArea.getX()) / mCurveArea.getWidth()));
  return kMinFreq * std::pow(kMaxFreq / kMinFreq, t);
}

float EqStage::yOfDb(double db) const
{
  return mCurveArea.getCentreY() - static_cast<float>(db / kRangeDb) * mCurveArea.getHeight() * 0.5f;
}

double EqStage::dbOfY(float y) const
{
  return (mCurveArea.getCentreY() - y) / (mCurveArea.getHeight() * 0.5f) * kRangeDb;
}

juce::Point<float> EqStage::bandPoint(int band) const
{
  const double f = value(NexusProcessor::eqParam(band, "freq"));
  // Cortes e notch ficam na linha do 0 dB; os outros, na altura do ganho.
  const double db = hasGain(band) ? value(NexusProcessor::eqParam(band, "ganho")) : 0.0;
  return {xOfFreq(f), yOfDb(juce::jlimit(-kRangeDb, kRangeDb, db))};
}

int EqStage::bandAt(juce::Point<float> p) const
{
  for (int i = kBands - 1; i >= 0; --i)
    if (bandOn(i) && bandPoint(i).getDistanceFrom(p) < 12.0f)
      return i;
  return -1;
}

void EqStage::addBandAt(juce::Point<float> p)
{
  int free = -1;
  for (int i = 0; i < kBands && free < 0; ++i)
    if (!bandOn(i))
      free = i;
  if (free < 0)
    return;
  const double f = freqOfX(p.x);
  // Perto das pontas, a faixa nasce como corte (o uso mais comum ali).
  const int type = f < 35.0 ? 3 : f > 14000.0 ? 4 : 0;
  setParam(NexusProcessor::eqParam(free, "tipo"), static_cast<float>(type));
  setParam(NexusProcessor::eqParam(free, "freq"), static_cast<float>(f));
  setParam(NexusProcessor::eqParam(free, "ganho"), type == 0 ? static_cast<float>(juce::jlimit(-30.0, 30.0, dbOfY(p.y))) : 0.0f);
  setParam(NexusProcessor::eqParam(free, "q"), 1.0f);
  setParam(NexusProcessor::eqParam(free, "inclinacao"), 1.0f);
  setParam(NexusProcessor::eqParam(free, "ligado"), 1.0f);
  select(free);
}

void EqStage::deleteBand(int band)
{
  for (const char* name : {"ligado", "tipo", "freq", "ganho", "q", "inclinacao"})
  {
    auto& p = param(NexusProcessor::eqParam(band, name));
    p.setValueNotifyingHost(p.getDefaultValue());
  }
  if (mSelected == band)
    select(-1);
  repaint();
}

void EqStage::showBandMenu(int band)
{
  juce::PopupMenu menu, types, slopes;
  const int t = bandType(band);
  for (int i = 0; i < 6; ++i)
    types.addItem(tr(typeName(i)), true, i == t, [this, band, i] {
      setParam(NexusProcessor::eqParam(band, "tipo"), static_cast<float>(i));
      select(band);
    });
  const int s = juce::roundToInt(value(NexusProcessor::eqParam(band, "inclinacao")));
  for (int i = 0; i < 4; ++i)
    slopes.addItem(juce::String((i + 1) * 12) + " dB/" + tr("oitava"), true, i == s,
                   [this, band, i] { setParam(NexusProcessor::eqParam(band, "inclinacao"), static_cast<float>(i)); });
  menu.addSectionHeader(tr("Faixa") + " " + juce::String(band + 1));
  menu.addSubMenu(tr("Tipo"), types);
  menu.addSubMenu(tr("Inclinação"), slopes, t == 3 || t == 4);
  menu.addSeparator();
  menu.addItem(tr("Apagar a faixa"), [this, band] { deleteBand(band); });
  menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(this).withMousePosition());
}

void EqStage::paint(juce::Graphics& g)
{
  paintCurve(g);
  paintStrip(g);
}

void EqStage::paintCurve(juce::Graphics& g)
{
  const auto a = mCurveArea;
  const auto frame = a.expanded(10.0f, 8.0f);
  g.setGradientFill(juce::ColourGradient(juce::Colour(0xff121419), frame.getX(), frame.getY(), juce::Colour(0xff0b0c0f), frame.getX(),
                                         frame.getBottom(), false));
  g.fillRoundedRectangle(frame, 10.0f);
  g.setColour(colour::boxBorder.withAlpha(0.6f));
  g.drawRoundedRectangle(frame, 10.0f, 1.0f);
  g.setFont(Fonts::get().ui(10.0f));
  for (double f : {20.0, 50.0, 100.0, 200.0, 500.0, 1000.0, 2000.0, 5000.0, 10000.0, 20000.0})
  {
    const float x = xOfFreq(f);
    g.setColour(juce::Colour(0xff1c1e23));
    g.drawVerticalLine(juce::roundToInt(x), a.getY(), a.getBottom());
    g.setColour(colour::faint);
    g.drawText(f < 1000 ? juce::String(juce::roundToInt(f)) : juce::String(juce::roundToInt(f / 1000)) + "k",
               juce::Rectangle<float>(x - 20.0f, a.getBottom() - 14.0f, 40.0f, 12.0f), juce::Justification::centred);
  }
  for (double db : {-18.0, -12.0, -6.0, 0.0, 6.0, 12.0, 18.0})
  {
    const float y = yOfDb(db);
    g.setColour(db == 0.0 ? juce::Colour(0xff34373d) : juce::Colour(0xff1c1e23));
    g.drawHorizontalLine(juce::roundToInt(y), a.getX(), a.getRight());
    g.setColour(colour::faint);
    g.drawText((db > 0 ? "+" : "") + juce::String(juce::roundToInt(db)), juce::Rectangle<float>(a.getRight() - 28.0f, y - 12.0f, 26.0f, 11.0f),
               juce::Justification::centredRight);
  }
  const auto bands = readBands(mParams);
  const bool on = eqOn();
  const int steps = juce::jmax(80, static_cast<int>(a.getWidth() / 2.5f));
  auto curveOf = [&](int only, juce::Path& line, juce::Path& area) {
    for (int i = 0; i <= steps; ++i)
    {
      const float x = a.getX() + a.getWidth() * static_cast<float>(i) / static_cast<float>(steps);
      const float y = juce::jlimit(a.getY(), a.getBottom(), yOfDb(nexus::FreeEq::responseDb(bands, freqOfX(x), kSampleRate, only)));
      if (i == 0)
      {
        line.startNewSubPath(x, y);
        area.startNewSubPath(x, yOfDb(0.0));
      }
      else
        line.lineTo(x, y);
      area.lineTo(x, y);
    }
    area.lineTo(a.getRight(), yOfDb(0.0));
    area.closeSubPath();
  };
  // A faixa escolhida, sozinha, em cor clara por baixo.
  if (mSelected >= 0 && bands[static_cast<size_t>(mSelected)].on)
  {
    juce::Path line, area;
    curveOf(mSelected, line, area);
    g.setColour(bandColour(mSelected).withAlpha(0.16f));
    g.fillPath(area);
  }
  juce::Path line, area;
  curveOf(-1, line, area);
  g.setColour((on ? colour::accent : colour::faint).withAlpha(0.10f));
  g.fillPath(area);
  g.setColour(on ? colour::accent : colour::faint);
  g.strokePath(line, juce::PathStrokeType(2.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
  // Pontos das faixas, com o número.
  for (int i = 0; i < kBands; ++i)
  {
    if (!bands[static_cast<size_t>(i)].on)
      continue;
    const auto p = bandPoint(i);
    const bool hot = i == mHover || i == mDrag || i == mSelected;
    const float r = hot ? 10.0f : 8.5f;
    g.setColour(juce::Colours::black.withAlpha(0.55f));
    g.fillEllipse(p.x - r - 1.5f, p.y - r - 1.0f, 2.0f * r + 3.0f, 2.0f * r + 3.0f);
    // Em bypass, os pontos ficam apagados como a curva.
    g.setColour(bandColour(i).withMultipliedAlpha(on ? 1.0f : 0.4f));
    g.fillEllipse(p.x - r, p.y - r, 2.0f * r, 2.0f * r);
    if (i == mSelected)
    {
      g.setColour(juce::Colours::white);
      g.drawEllipse(p.x - r - 2.5f, p.y - r - 2.5f, 2.0f * r + 5.0f, 2.0f * r + 5.0f, 1.4f);
    }
    g.setColour(juce::Colour(0xff0b0c0f));
    g.setFont(Fonts::get().ui(10.5f).boldened());
    g.drawText(juce::String(i + 1), juce::Rectangle<float>(p.x - r, p.y - r, 2.0f * r, 2.0f * r), juce::Justification::centred);
  }
  // Frequência do ponto debaixo do mouse, do que está sendo arrastado ou do escolhido.
  const int tag = mDrag >= 0 ? mDrag : mHover >= 0 ? mHover : mSelected;
  if (tag >= 0 && bands[static_cast<size_t>(tag)].on)
    paintTag(g, tag);
  bool any = false;
  for (const auto& b : bands)
    any = any || b.on;
  if (!any)
  {
    g.setFont(Fonts::get().ui(13.0f));
    g.setColour(colour::dim);
    g.drawText(tr("Duplo clique na curva para criar uma faixa"), a.withTrimmedBottom(a.getHeight() * 0.35f), juce::Justification::centredBottom);
  }
}

// Etiqueta em cima do ponto: frequência e, nas faixas com ganho, o ganho.
void EqStage::paintTag(juce::Graphics& g, int band)
{
  auto text = formatFreq(value(NexusProcessor::eqParam(band, "freq")));
  if (hasGain(band))
    text << "   " << formatDb(value(NexusProcessor::eqParam(band, "ganho")));
  const auto font = Fonts::get().ui(12.0f).boldened();
  const float w = juce::GlyphArrangement::getStringWidth(font, text) + 20.0f, h = 22.0f;
  const auto p = bandPoint(band);
  // Em cima do ponto; perto do topo, embaixo. Nunca sai da área da curva.
  float y = p.y - 18.0f - h;
  if (y < mCurveArea.getY())
    y = p.y + 18.0f;
  const float x = juce::jlimit(mCurveArea.getX(), mCurveArea.getRight() - w, p.x - w * 0.5f);
  const juce::Rectangle<float> box(x, y, w, h);
  g.setColour(juce::Colours::black.withAlpha(0.5f));
  g.fillRoundedRectangle(box.translated(0.0f, 1.5f), h * 0.5f);
  g.setColour(juce::Colour(0xf0101216));
  g.fillRoundedRectangle(box, h * 0.5f);
  g.setColour(bandColour(band).withAlpha(0.8f));
  g.drawRoundedRectangle(box.reduced(0.5f), h * 0.5f, 1.2f);
  g.setColour(colour::text);
  g.setFont(font);
  g.drawText(text, box, juce::Justification::centred, false);
}

// Bypass do equalizador inteiro: o som passa intacto, as faixas ficam guardadas.
void EqStage::paintBypass(juce::Graphics& g)
{
  const bool on = eqOn();
  const auto b = mBypass;
  const auto tint = on ? colour::accent : colour::danger;
  g.setColour(tint.withAlpha(mBypassHover ? 0.20f : 0.12f));
  g.fillRoundedRectangle(b, b.getHeight() * 0.5f);
  g.setColour(tint.withAlpha(mBypassHover ? 0.9f : 0.6f));
  g.drawRoundedRectangle(b.reduced(0.5f), b.getHeight() * 0.5f, 1.2f);
  // Símbolo de liga/desliga.
  auto icon = b.withWidth(b.getHeight()).reduced(12.0f).translated(6.0f, 0.0f);
  juce::Path power;
  power.addCentredArc(icon.getCentreX(), icon.getCentreY(), icon.getWidth() * 0.5f, icon.getHeight() * 0.5f, 0.0f, 0.65f,
                      juce::MathConstants<float>::twoPi - 0.65f, true);
  power.startNewSubPath(icon.getCentreX(), icon.getY() - 1.5f);
  power.lineTo(icon.getCentreX(), icon.getCentreY());
  g.setColour(tint);
  g.strokePath(power, juce::PathStrokeType(1.8f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
  drawCaps(g, on ? tr("Ligado") : tr("Bypass"), b.withTrimmedLeft(b.getHeight() + 2.0f).withTrimmedRight(14.0f), 11.0f,
           juce::Justification::centredLeft, on ? colour::text : tint);
}

void EqStage::paintStrip(juce::Graphics& g)
{
  const auto s = mStrip;
  g.setColour(juce::Colour(0xff121317));
  g.fillRoundedRectangle(s, 10.0f);
  g.setColour(colour::boxBorder.withAlpha(0.5f));
  g.drawRoundedRectangle(s.reduced(0.5f), 10.0f, 1.0f);
  paintBypass(g);
  auto left = s.reduced(20.0f, 16.0f).removeFromLeft(270.0f);
  int count = 0;
  for (int i = 0; i < kBands; ++i)
    count += bandOn(i) ? 1 : 0;
  if (mSelected < 0 || !bandOn(mSelected))
  {
    drawCaps(g, tr("Equalizador"), left.removeFromTop(20.0f), 11.0f, juce::Justification::centredLeft, colour::text);
    g.setFont(Fonts::get().ui(12.0f));
    g.setColour(colour::dim);
    g.drawFittedText(tr("Fixo no fim da cadeia: todo o som passa por aqui. Faixas: %1 de %2.")
                       .replace("%1", juce::String(count))
                       .replace("%2", juce::String(kBands)),
                     left.toNearestInt(), juce::Justification::topLeft, 3);
    return;
  }
  const auto c = bandColour(mSelected);
  g.setColour(c);
  g.fillEllipse(left.getX(), left.getY() + 2.0f, 14.0f, 14.0f);
  drawCaps(g, tr("Faixa") + " " + juce::String(mSelected + 1), left.removeFromTop(20.0f).withTrimmedLeft(22.0f), 11.0f,
           juce::Justification::centredLeft, colour::text);
  g.setFont(Fonts::get().ui(13.0f));
  g.setColour(c);
  g.drawText(tr(typeName(bandType(mSelected))), left.removeFromTop(22.0f), juce::Justification::centredLeft);
  g.setFont(Fonts::get().ui(11.5f));
  g.setColour(colour::dim);
  g.drawFittedText(tr("Botão direito no ponto: tipo, inclinação e apagar. Faixas: %1 de %2.")
                     .replace("%1", juce::String(count))
                     .replace("%2", juce::String(kBands)),
                   left.toNearestInt(), juce::Justification::topLeft, 2);
}

void EqStage::timerCallback()
{
  // Redesenha quando algum parâmetro do equalizador mudou (automação, preset).
  juce::uint32 h = 0;
  for (auto* p : mParams.processor.getParameters())
    if (auto* w = dynamic_cast<juce::AudioProcessorParameterWithID*>(p); w != nullptr && w->paramID.startsWith("eq") && w->paramID.containsChar('_'))
      h = h * 31u + static_cast<juce::uint32>(p->getValue() * 100000.0f);
  if (h != mLastHash)
  {
    mLastHash = h;
    if (mSelected >= 0 && !bandOn(mSelected))
      select(-1);
    repaint();
  }
}

void EqStage::mouseMove(const juce::MouseEvent& e)
{
  if (const bool over = mBypass.contains(e.position); over != mBypassHover)
  {
    mBypassHover = over;
    setMouseCursor(over ? juce::MouseCursor::PointingHandCursor : juce::MouseCursor::NormalCursor);
    repaint(mBypass.expanded(2.0f).toNearestInt());
  }
  if (mBypassHover)
    return;
  const int b = bandAt(e.position);
  if (b != mHover)
  {
    mHover = b;
    setMouseCursor(b >= 0 ? juce::MouseCursor::DraggingHandCursor : juce::MouseCursor::NormalCursor);
    repaint();
  }
}

void EqStage::mouseExit(const juce::MouseEvent&)
{
  mHover = -1;
  mBypassHover = false;
  repaint();
}

void EqStage::mouseDown(const juce::MouseEvent& e)
{
  if (mBypass.contains(e.position))
  {
    toggleBypass();
    return;
  }
  const int b = bandAt(e.position);
  if (b < 0)
    return;
  if (e.mods.isPopupMenu())
  {
    select(b);
    showBandMenu(b);
    return;
  }
  if (b != mSelected)
    select(b);
  mDrag = b;
  param(NexusProcessor::eqParam(b, "freq")).beginChangeGesture();
  if (hasGain(b))
    param(NexusProcessor::eqParam(b, "ganho")).beginChangeGesture();
}

void EqStage::mouseDrag(const juce::MouseEvent& e)
{
  if (mDrag < 0)
    return;
  setParam(NexusProcessor::eqParam(mDrag, "freq"), static_cast<float>(freqOfX(e.position.x)));
  if (hasGain(mDrag))
    setParam(NexusProcessor::eqParam(mDrag, "ganho"), static_cast<float>(juce::jlimit(-30.0, 30.0, dbOfY(e.position.y))));
  repaint();
}

void EqStage::mouseUp(const juce::MouseEvent&)
{
  if (mDrag < 0)
    return;
  param(NexusProcessor::eqParam(mDrag, "freq")).endChangeGesture();
  if (hasGain(mDrag))
    param(NexusProcessor::eqParam(mDrag, "ganho")).endChangeGesture();
  mDrag = -1;
}

void EqStage::mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& w)
{
  const int b = bandAt(e.position);
  if (b < 0)
    return;
  auto& q = param(NexusProcessor::eqParam(b, "q"));
  q.setValueNotifyingHost(juce::jlimit(0.0f, 1.0f, q.getValue() + (w.deltaY > 0 ? 0.03f : -0.03f)));
  repaint();
}

void EqStage::mouseDoubleClick(const juce::MouseEvent& e)
{
  if (!mCurveArea.contains(e.position))
    return;
  // Duplo clique num ponto apaga a faixa; num lugar vazio, cria uma.
  if (const int b = bandAt(e.position); b >= 0)
    deleteBand(b);
  else
    addBandAt(e.position);
}

void EqStage::paintThumb(juce::Graphics& g, juce::Rectangle<float> r, juce::AudioProcessorValueTreeState& params, float alpha)
{
  // Miniatura: módulo quadrado com cinco deslizantes. Cada deslizante fica na
  // altura da curva atual na frequência dele (graves à esquerda), então o
  // ícone mostra de relance o que o equalizador está fazendo.
  const float side = std::min(r.getWidth(), r.getHeight());
  const auto box = r.withSizeKeepingCentre(side * 1.08f, side).reduced(1.0f);
  const float corner = side * 0.16f;
  g.setColour(juce::Colours::black.withAlpha(0.45f * alpha));
  g.fillRoundedRectangle(box.translated(0.0f, 1.5f), corner);
  g.setGradientFill(juce::ColourGradient(juce::Colour(0xff2b2e35).withMultipliedAlpha(alpha), box.getX(), box.getY(),
                                         juce::Colour(0xff15171b).withMultipliedAlpha(alpha), box.getX(), box.getBottom(), false));
  g.fillRoundedRectangle(box, corner);
  g.setColour(juce::Colours::white.withAlpha(0.14f * alpha));
  g.drawRoundedRectangle(box.reduced(0.5f), corner, 1.0f);

  const auto bands = readBands(params);
  const double freqs[] = {60.0, 250.0, 1000.0, 4000.0, 12000.0};
  const int n = 5;
  const auto in = box.reduced(box.getWidth() * 0.15f, box.getHeight() * 0.17f);
  const float step = in.getWidth() / static_cast<float>(n - 1);
  const float capW = std::max(5.0f, step * 0.62f), capH = std::max(3.5f, in.getHeight() * 0.15f);
  const float travel = in.getHeight() - capH;
  const float mid = in.getCentreY();
  const auto accent = colour::accent.withMultipliedAlpha(alpha);
  for (int k = 0; k < n; ++k)
  {
    const float x = in.getX() + step * static_cast<float>(k);
    // Trilho: sulco escuro com um traço claro de 0 dB.
    g.setColour(juce::Colour(0xff0a0b0d).withMultipliedAlpha(alpha));
    g.fillRoundedRectangle(x - 1.3f, in.getY(), 2.6f, in.getHeight(), 1.3f);
    g.setColour(juce::Colours::white.withAlpha(0.18f * alpha));
    g.fillRect(x - capW * 0.45f, mid - 0.5f, capW * 0.9f, 1.0f);
    const double db = juce::jlimit(-12.0, 12.0, nexus::FreeEq::responseDb(bands, freqs[k], kSampleRate));
    const float y = mid - static_cast<float>(db / 12.0) * travel * 0.5f;
    // Do 0 dB até o deslizante, o trilho acende.
    if (std::abs(y - mid) > 0.5f)
    {
      g.setColour(accent.withMultipliedAlpha(0.85f));
      g.fillRoundedRectangle(x - 1.3f, std::min(y, mid), 2.6f, std::abs(y - mid), 1.3f);
    }
    auto cap = juce::Rectangle<float>(capW, capH).withCentre({x, y});
    g.setColour(juce::Colours::black.withAlpha(0.5f * alpha));
    g.fillRoundedRectangle(cap.translated(0.0f, 1.0f), 1.6f);
    g.setGradientFill(juce::ColourGradient(juce::Colour(0xffe9ebee).withMultipliedAlpha(alpha), cap.getX(), cap.getY(),
                                           juce::Colour(0xff9da2aa).withMultipliedAlpha(alpha), cap.getX(), cap.getBottom(), false));
    g.fillRoundedRectangle(cap, 1.6f);
    g.setColour(accent);
    g.fillRect(cap.getX() + 1.0f, cap.getCentreY() - 0.6f, cap.getWidth() - 2.0f, 1.2f);
  }
}
} // namespace nexus::ui
