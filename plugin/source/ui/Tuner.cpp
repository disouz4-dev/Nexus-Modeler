#include "Tuner.h"

#include <algorithm>
#include <cmath>

#include "../PluginProcessor.h"
#include "Brand.h"
#include "Theme.h"

namespace nexus::ui
{
namespace
{
// ~85 ms de som (4096 amostras a 48 kHz): pega até o Si grave do baixo de 5 cordas.
constexpr double kAnalysisSeconds = 4096.0 / 48000.0;
const char* const kNotes[12] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};

// Diferença média normalizada do YIN para o atraso tau.
double yinDiff(const double* x, int window, int tau)
{
  double d = 0.0;
  for (int i = 0; i < window; ++i)
  {
    const double v = x[i] - x[i + tau];
    d += v * v;
  }
  return d;
}

// Cor de cada luz da fileira: verde no centro, âmbar perto, vermelho longe.
juce::Colour ledColour(int i, int count)
{
  const int centre = count / 2;
  const int dist = std::abs(i - centre);
  if (dist == 0)
    return juce::Colour(0xff35f06a);
  if (dist <= count / 8)
    return juce::Colour(0xffffc22e);
  return juce::Colour(0xffff3b2a);
}

void drawScrew(juce::Graphics& g, juce::Point<float> c, float r)
{
  juce::ColourGradient head(juce::Colour(0xff8c8d90), c.x - r, c.y - r, juce::Colour(0xff35363a), c.x + r, c.y + r, false);
  g.setGradientFill(head);
  g.fillEllipse(c.x - r, c.y - r, r * 2.0f, r * 2.0f);
  g.setColour(juce::Colour(0xff1a1a1c));
  const float t = std::max(1.0f, r * 0.18f);
  g.drawLine(c.x - r * 0.55f, c.y, c.x + r * 0.55f, c.y, t);
  g.drawLine(c.x, c.y - r * 0.55f, c.x, c.y + r * 0.55f, t);
}

// Painel de rack: alumínio preto escovado com as orelhas e os parafusos.
void drawRack(juce::Graphics& g, juce::Rectangle<float> r, float alpha)
{
  const float ear = r.getHeight() * 0.32f;
  const float corner = r.getHeight() * 0.04f;
  juce::ColourGradient body(juce::Colour(0xff26272a).withMultipliedAlpha(alpha), r.getX(), r.getY(), juce::Colour(0xff111214).withMultipliedAlpha(alpha),
                            r.getX(), r.getBottom(), false);
  g.setGradientFill(body);
  g.fillRoundedRectangle(r, corner);
  // Escovado: riscos horizontais bem fracos.
  juce::Random rnd(7);
  for (float y = r.getY() + 1.0f; y < r.getBottom() - 1.0f; y += 1.5f)
  {
    g.setColour(juce::Colours::white.withAlpha(alpha * (0.012f + 0.02f * rnd.nextFloat())));
    g.drawHorizontalLine(juce::roundToInt(y), r.getX() + ear, r.getRight() - ear);
  }
  g.setColour(juce::Colours::white.withAlpha(0.10f * alpha));
  g.drawHorizontalLine(juce::roundToInt(r.getY() + 1.0f), r.getX() + corner, r.getRight() - corner);
  g.setColour(juce::Colours::black.withAlpha(0.6f * alpha));
  g.drawRoundedRectangle(r, corner, 1.0f);
  // Orelhas: faixa mais escura com dois furos e parafusos.
  for (const float x : {r.getX(), r.getRight() - ear})
  {
    const auto e = juce::Rectangle<float>(x, r.getY(), ear, r.getHeight());
    g.setColour(juce::Colours::black.withAlpha(0.25f * alpha));
    g.fillRect(e.reduced(0.0f, 1.0f));
    for (const float fy : {0.22f, 0.78f})
    {
      const auto c = juce::Point<float>(e.getCentreX(), e.getY() + e.getHeight() * fy);
      const float rr = ear * 0.12f;
      g.setColour(juce::Colours::black.withAlpha(0.85f * alpha));
      g.fillRoundedRectangle(c.x - rr * 1.9f, c.y - rr * 1.15f, rr * 3.8f, rr * 2.3f, rr);
      if (alpha > 0.5f)
        drawScrew(g, c, rr);
    }
  }
}

// Fileira de luzes: aceso = a posição do desvio (e as vizinhas mais fracas).
void drawLeds(juce::Graphics& g, juce::Rectangle<float> r, int count, double cents, bool active, float alpha)
{
  const float gap = r.getWidth() / static_cast<float>(count) * 0.28f;
  const float w = (r.getWidth() - gap * static_cast<float>(count - 1)) / static_cast<float>(count);
  const int centre = count / 2;
  const double pos = centre + std::clamp(cents, -50.0, 50.0) / 50.0 * centre;
  for (int i = 0; i < count; ++i)
  {
    auto led = juce::Rectangle<float>(r.getX() + static_cast<float>(i) * (w + gap), r.getY(), w, r.getHeight());
    if (i == centre)
      led = led.expanded(w * 0.15f, r.getHeight() * 0.12f);
    const auto c = ledColour(i, count);
    float level = 0.0f;
    if (active)
    {
      const double d = std::abs(pos - i);
      level = d < 0.5 ? 1.0f : d < 1.5 ? 0.35f : 0.0f;
      if (std::abs(cents) < 2.0 && i == centre)
        level = 1.0f;
    }
    g.setColour(c.withMultipliedBrightness(0.18f).withAlpha(alpha));
    g.fillRoundedRectangle(led, w * 0.25f);
    if (level > 0.0f)
    {
      g.setColour(c.withAlpha(0.35f * level * alpha));
      g.fillRoundedRectangle(led.expanded(w * 0.6f, w * 0.6f), w);
      g.setColour(c.withAlpha(level * alpha));
      g.fillRoundedRectangle(led, w * 0.25f);
      g.setColour(juce::Colours::white.withAlpha(0.35f * level * alpha));
      g.fillRoundedRectangle(led.reduced(w * 0.25f, led.getHeight() * 0.3f).withY(led.getY() + led.getHeight() * 0.12f), w * 0.15f);
    }
  }
}
} // namespace

// ---------------------------------------------------------------- PitchDetector

double PitchDetector::detect(const float* samples, int count, double rate)
{
  // Silêncio ou quase: sem nota.
  double energy = 0.0;
  for (int i = 0; i < count; ++i)
    energy += static_cast<double>(samples[i]) * samples[i];
  if (count <= 0 || std::sqrt(energy / count) < 0.0015)
    return 0.0;

  // 1) Cópia reduzida para uns 12 kHz (média das amostras: corta o agudo antes de reduzir).
  const int kDecimate = std::max(1, static_cast<int>(std::lround(rate / 12000.0)));
  const int small = count / kDecimate;
  mSmall.assign(static_cast<size_t>(small), 0.0);
  for (int i = 0; i < small; ++i)
  {
    double s = 0.0;
    for (int k = 0; k < kDecimate; ++k)
      s += samples[i * kDecimate + k];
    mSmall[static_cast<size_t>(i)] = s / kDecimate;
  }
  const double smallRate = rate / kDecimate;
  const int window = small / 2;
  const int minTau = std::max(2, static_cast<int>(smallRate / 1400.0)); // até ~1,4 kHz
  const int maxTau = std::min(window - 1, static_cast<int>(smallRate / 28.0)); // até 28 Hz
  mDiff.assign(static_cast<size_t>(maxTau + 2), 1.0);
  double running = 0.0;
  int best = -1;
  for (int tau = 1; tau <= maxTau; ++tau)
  {
    const double d = yinDiff(mSmall.data(), window, tau);
    running += d;
    mDiff[static_cast<size_t>(tau)] = running > 0.0 ? d * tau / running : 1.0;
  }
  // Primeiro vale abaixo do limiar (o período, não um múltiplo dele).
  for (int tau = minTau; tau < maxTau; ++tau)
    if (mDiff[static_cast<size_t>(tau)] < 0.15)
    {
      while (tau + 1 < maxTau && mDiff[static_cast<size_t>(tau + 1)] < mDiff[static_cast<size_t>(tau)])
        ++tau;
      best = tau;
      break;
    }
  if (best < 0)
    return 0.0;

  // 2) Refina na taxa cheia, em volta do período achado, com interpolação.
  std::vector<double> full(static_cast<size_t>(count));
  for (int i = 0; i < count; ++i)
    full[static_cast<size_t>(i)] = samples[i];
  const int fullWindow = count / 2;
  const int centre = best * kDecimate;
  int bestFull = centre;
  double bestValue = 1e300;
  const int lo = std::max(2, centre - kDecimate - 1), hi = std::min(count - fullWindow - 2, centre + kDecimate + 1);
  std::vector<double> local(static_cast<size_t>(hi - lo + 3), 0.0);
  for (int tau = lo - 1; tau <= hi + 1; ++tau)
  {
    const double d = yinDiff(full.data(), fullWindow, tau);
    local[static_cast<size_t>(tau - lo + 1)] = d;
    if (tau >= lo && tau <= hi && d < bestValue)
    {
      bestValue = d;
      bestFull = tau;
    }
  }
  const double a = local[static_cast<size_t>(bestFull - lo)];
  const double b = local[static_cast<size_t>(bestFull - lo + 1)];
  const double c = local[static_cast<size_t>(bestFull - lo + 2)];
  const double den = a - 2.0 * b + c;
  const double shift = std::abs(den) > 1e-12 ? 0.5 * (a - c) / den : 0.0;
  const double period = bestFull + std::clamp(shift, -1.0, 1.0);
  return period > 0.0 ? rate / period : 0.0;
}

// ---------------------------------------------------------------- TunerStage

TunerStage::TunerStage(NexusProcessor& processor) : mProcessor(processor)
{
  mSamples.assign(NexusProcessor::kTunerRing, 0.0f);
  startTimerHz(30);
}

TunerStage::~TunerStage()
{
  stopTimer();
  mProcessor.setTunerOpen(false);
}

float TunerStage::referenceA() const
{
  return mProcessor.params.getRawParameterValue("afinador_la")->load();
}

bool TunerStage::muted() const
{
  return mProcessor.params.getRawParameterValue("afinador_mudo")->load() > 0.5f;
}

void TunerStage::visibilityChanged()
{
  // O relógio confere a cada volta se a tela aparece; aqui só adianta o mudo.
  timerCallback();
}

void TunerStage::timerCallback()
{
  // Relógio sempre ligado (o aviso de "ficou visível" não chega em todo
  // caminho de troca de tela): só lê a guitarra com o afinador aparecendo.
  const bool shown = isShowing();
  if (shown != mOpen)
  {
    mOpen = shown;
    mProcessor.setTunerOpen(shown);
  }
  if (shown)
    update();
}

void TunerStage::update()
{
  const double rate = mProcessor.tunerSnapshot(mSamples.data(), NexusProcessor::kTunerRing);
  // O mesmo tempo de som em qualquer taxa (as últimas amostras da cópia).
  const int count = std::min(NexusProcessor::kTunerRing, static_cast<int>(rate * kAnalysisSeconds));
  const double hz = mDetector.detect(mSamples.data() + NexusProcessor::kTunerRing - count, count, rate);
  if (hz > 0.0)
  {
    mRecent.push_back(hz);
    if (mRecent.size() > 5)
      mRecent.erase(mRecent.begin());
    auto sorted = mRecent;
    std::sort(sorted.begin(), sorted.end());
    const double median = sorted[sorted.size() / 2];
    const double ref = referenceA();
    const double midi = 69.0 + 12.0 * std::log2(median / ref);
    const double cents = (midi - std::round(midi)) * 100.0;
    // Nota nova: pula direto; mesma nota: suaviza o ponteiro.
    const bool sameNote = mHz > 0.0 && std::round(69.0 + 12.0 * std::log2(mHz / ref)) == std::round(midi);
    mCents = sameNote ? mCents + (cents - mCents) * 0.35 : cents;
    mHz = median;
    mHold = 12;
  }
  else if (mHold > 0 && --mHold == 0)
  {
    mHz = 0.0;
    mRecent.clear();
  }
  repaint();
}

void TunerStage::resized()
{
  auto area = getLocalBounds().toFloat().reduced(40.0f, 30.0f);
  const float w = std::min(area.getWidth(), 980.0f);
  const float h = w / 4.6f;
  mPanel = juce::Rectangle<float>(w, h).withCentre(area.getCentre().translated(0.0f, -20.0f));
  const float ear = mPanel.getHeight() * 0.32f;
  auto controls = mPanel.withTrimmedLeft(mPanel.getWidth() - ear - mPanel.getWidth() * 0.17f).withTrimmedRight(ear).reduced(10.0f, h * 0.16f);
  const float rowH = controls.getHeight() * 0.36f;
  auto refRow = controls.removeFromTop(rowH);
  mRefDown = refRow.removeFromLeft(rowH);
  mRefUp = refRow.removeFromRight(rowH);
  controls.removeFromTop(controls.getHeight() - rowH);
  mMute = controls;
}

void TunerStage::mouseDown(const juce::MouseEvent& e)
{
  auto* ref = mProcessor.params.getParameter("afinador_la");
  auto setRef = [ref](float hz) { ref->setValueNotifyingHost(ref->convertTo0to1(juce::jlimit(430.0f, 450.0f, hz))); };
  if (mRefDown.contains(e.position))
    setRef(referenceA() - 1.0f);
  else if (mRefUp.contains(e.position))
    setRef(referenceA() + 1.0f);
  else if (mMute.contains(e.position))
    mProcessor.params.getParameter("afinador_mudo")->setValueNotifyingHost(muted() ? 0.0f : 1.0f);
  else
    return;
  repaint();
}

void TunerStage::paint(juce::Graphics& g)
{
  drawRack(g, mPanel, 1.0f);
  const float h = mPanel.getHeight();
  const float ear = h * 0.32f;
  auto inner = mPanel.reduced(ear + h * 0.08f, h * 0.13f);

  // Esquerda: marca e nome.
  auto left = inner.removeFromLeft(inner.getWidth() * 0.17f);
  brand::drawPenrose(g, left.removeFromTop(left.getHeight() * 0.42f).withSizeKeepingCentre(h * 0.26f, h * 0.24f));
  g.setColour(colour::text);
  g.setFont(Fonts::get().display(h * 0.15f).withExtraKerningFactor(0.18f));
  g.drawText("NEXUS", left.removeFromTop(left.getHeight() * 0.5f), juce::Justification::centred);
  g.setColour(colour::accent);
  g.setFont(Fonts::get().display(h * 0.085f).withExtraKerningFactor(0.3f));
  g.drawText(TRANS("AFINADOR"), left, juce::Justification::centredTop);

  // Direita: lá de referência e mudo (desenhados depois do visor).
  inner.removeFromRight(mPanel.getWidth() * 0.17f + h * 0.04f);

  // Centro: visor com a nota, a fileira de luzes e os números.
  auto display = inner.reduced(h * 0.03f, 0.0f);
  g.setColour(juce::Colour(0xff050506));
  g.fillRoundedRectangle(display, h * 0.04f);
  g.setColour(juce::Colours::white.withAlpha(0.06f));
  g.drawRoundedRectangle(display, h * 0.04f, 1.0f);
  auto content = display.reduced(h * 0.07f, h * 0.06f);
  const bool active = mHz > 0.0;
  const double ref = referenceA();
  const double midi = active ? 69.0 + 12.0 * std::log2(mHz / ref) : 0.0;
  const int note = active ? static_cast<int>(std::round(midi)) : 0;
  const bool inTune = active && std::abs(mCents) < 2.0;

  auto noteBox = content.removeFromLeft(content.getHeight() * 1.25f);
  const juce::Colour noteColour = inTune ? juce::Colour(0xff35f06a) : juce::Colour(0xffffc22e);
  if (active)
  {
    const juce::String name = kNotes[((note % 12) + 12) % 12];
    g.setColour(noteColour.withAlpha(0.25f));
    g.setFont(Fonts::get().display(noteBox.getHeight() * 0.95f));
    for (const float d : {-1.5f, 1.5f})
      g.drawText(name.substring(0, 1), noteBox.translated(d, 0.0f), juce::Justification::centred);
    g.setColour(noteColour);
    g.drawText(name.substring(0, 1), noteBox, juce::Justification::centred);
    g.setFont(Fonts::get().display(noteBox.getHeight() * 0.32f));
    auto corner = noteBox.withTrimmedLeft(noteBox.getWidth() * 0.72f);
    if (name.length() > 1)
      g.drawText("#", corner.removeFromTop(corner.getHeight() * 0.45f), juce::Justification::centredLeft);
    else
      corner.removeFromTop(corner.getHeight() * 0.45f);
    g.setColour(noteColour.withAlpha(0.7f));
    g.drawText(juce::String(note / 12 - 1), corner, juce::Justification::centredLeft);
  }
  else
  {
    g.setColour(juce::Colour(0xff2a2410));
    g.setFont(Fonts::get().display(noteBox.getHeight() * 0.95f));
    g.drawText("-", noteBox, juce::Justification::centred);
  }

  content.removeFromLeft(h * 0.05f);
  auto numbers = content.removeFromBottom(content.getHeight() * 0.26f);
  auto ledArea = content.withSizeKeepingCentre(content.getWidth(), content.getHeight() * 0.42f);
  drawLeds(g, ledArea, 33, mCents, active, 1.0f);
  // Marcas de -50, 0 e +50 em cima da fileira.
  g.setColour(colour::faint);
  g.setFont(Fonts::get().ui(h * 0.06f));
  const auto marks = ledArea.withHeight(h * 0.08f).translated(0.0f, -h * 0.11f);
  g.drawText("-50", marks, juce::Justification::centredLeft);
  g.drawText("0", marks, juce::Justification::centred);
  g.drawText("+50", marks, juce::Justification::centredRight);
  g.setFont(Fonts::get().ui(h * 0.075f));
  g.setColour(active ? colour::text : colour::faint);
  g.drawText(active ? juce::String(mHz, 1) + " Hz" : TRANS("toque uma corda"), numbers, juce::Justification::centredLeft);
  if (active)
    g.drawText((mCents >= 0.0 ? "+" : "") + juce::String(mCents, 1) + " cents", numbers, juce::Justification::centredRight);

  // Lá de referência: - 440 Hz +
  auto refRow = mRefDown.getUnion(mRefUp);
  g.setColour(colour::faint);
  g.setFont(Fonts::get().ui(h * 0.06f).withExtraKerningFactor(0.1f));
  g.drawText(TRANS(juce::String::fromUTF8("LÁ DE REFERÊNCIA")), refRow.translated(0.0f, -refRow.getHeight() * 0.8f), juce::Justification::centred);
  g.setColour(juce::Colour(0xff050506));
  g.fillRoundedRectangle(refRow, h * 0.03f);
  g.setColour(colour::text);
  g.setFont(Fonts::get().display(refRow.getHeight() * 0.62f));
  g.drawText(juce::String(juce::roundToInt(ref)) + " Hz", refRow, juce::Justification::centred);
  g.setColour(colour::dim);
  g.drawText("-", mRefDown, juce::Justification::centred);
  g.drawText("+", mRefUp, juce::Justification::centred);

  // Mudo ao afinar: botão com LED.
  const bool mute = muted();
  g.setColour(juce::Colour(0xff1c1d20));
  g.fillRoundedRectangle(mMute, h * 0.03f);
  g.setColour(juce::Colours::black.withAlpha(0.6f));
  g.drawRoundedRectangle(mMute, h * 0.03f, 1.0f);
  const float lr = mMute.getHeight() * 0.13f;
  const auto lc = juce::Point<float>(mMute.getX() + mMute.getHeight() * 0.45f, mMute.getCentreY());
  g.setColour(mute ? juce::Colour(0xffff3b2a) : juce::Colour(0xff3a0d0a));
  g.fillEllipse(lc.x - lr, lc.y - lr, lr * 2.0f, lr * 2.0f);
  if (mute)
  {
    g.setColour(juce::Colour(0xffff3b2a).withAlpha(0.3f));
    g.fillEllipse(lc.x - lr * 2.5f, lc.y - lr * 2.5f, lr * 5.0f, lr * 5.0f);
  }
  g.setColour(mute ? colour::text : colour::dim);
  g.setFont(Fonts::get().display(mMute.getHeight() * 0.4f).withExtraKerningFactor(0.12f));
  g.drawText(TRANS("MUDO"), mMute.withTrimmedLeft(mMute.getHeight() * 0.8f), juce::Justification::centredLeft);

  // Explicação embaixo do rack.
  g.setColour(colour::dim);
  g.setFont(Fonts::get().ui(13.0f));
  g.drawText(TRANS(juce::String::fromUTF8("O afinador fica no começo da cadeia e escuta a guitarra antes de tudo. Com MUDO aceso, o som sai em silêncio enquanto esta tela estiver aberta.")),
             mPanel.withY(mPanel.getBottom() + 18.0f).withHeight(40.0f).expanded(60.0f, 0.0f), juce::Justification::centredTop);
}

void TunerStage::paintThumb(juce::Graphics& g, juce::Rectangle<float> r, float alpha)
{
  auto rack = r.withSizeKeepingCentre(r.getWidth(), r.getWidth() / 3.2f);
  drawRack(g, rack, alpha);
  const float ear = rack.getHeight() * 0.32f;
  auto screen = rack.reduced(ear + 2.0f, rack.getHeight() * 0.24f);
  g.setColour(juce::Colour(0xff050506).withAlpha(alpha));
  g.fillRoundedRectangle(screen, 2.0f);
  drawLeds(g, screen.reduced(screen.getWidth() * 0.08f, screen.getHeight() * 0.3f), 9, 0.0, true, alpha);
}
} // namespace nexus::ui
