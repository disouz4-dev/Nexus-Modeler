#include "nexus/AmpVoice.h"

#include <cmath>

namespace nexus
{

namespace
{
// Tamanho do pedaço em que os coeficientes do equalizador são recalculados
// enquanto um botão anda.
constexpr int kToneChunk = 32;

double approach(double current, double target, double alpha)
{
  const double next = current + (target - current) * alpha;
  return std::abs(target - next) < 1e-4 ? target : next;
}
} // namespace

void AmpVoice::prepare(std::shared_ptr<Pack> pack, double sampleRate, int maxBlockSize)
{
  mSampleRate = sampleRate;
  mKnobAlpha = 1.0 - std::exp(-kToneChunk / (0.03 * sampleRate));

  mToneStack.setCircuit(circuitByName(pack->toneCircuit));
  mToneStack.setCaptureKnobs(pack->captureTone);
  mToneStack.prepare(sampleRate);
  mPresence.setCaptureKnob(pack->capturePresence);
  // Prateleira de agudos a partir de 1,2 kHz, ±12 dB: é o TOM dos pedais
  // (como no TS9 e no SD-1, que mexem nos médios-agudos). Nos cabeçotes fica
  // na posição da captura (o equalizador deles é o universal, depois da caixa).
  mPresence.prepare(sampleRate, RelativeShelf::Kind::High, 1200.0, 24.0);

  mMorph.prepare(std::move(pack), sampleRate, maxBlockSize);
  jumpToControls();
}

void AmpVoice::enableGraphicEq(const double* frequencies, int numBands)
{
  mGraphic.prepare(mSampleRate, frequencies, numBands);
  mHasGraphic = numBands > 0;
}

void AmpVoice::jumpToControls()
{
  mControls = mTargetControls;
  mMorph.setStepped(mControls.stepped);
  mMorph.setTargetPosition(mControls.main);
  mMorph.jumpToTarget();
  applyControls();
  mToneStack.reset();
  mPresence.reset();
  mGraphic.reset();
}

void AmpVoice::applyControls()
{
  mToneStack.setUserKnobs(mControls.tone);
  mPresence.setUserKnob(mControls.presence);
  if (mHasGraphic)
    for (int b = 0; b < mGraphic.numBands(); ++b)
      mGraphic.setUserBand(b, mControls.graphicDb[b]);
}

void AmpVoice::process(const double* input, double* output, int numFrames)
{
  mMorph.setStepped(mTargetControls.stepped);
  mMorph.setTargetPosition(mTargetControls.main);
  mMorph.process(input, output, numFrames);

  for (int start = 0; start < numFrames; start += kToneChunk)
  {
    const int n = std::min(kToneChunk, numFrames - start);
    auto& c = mControls;
    const auto& t = mTargetControls;
    c.tone.bass = approach(c.tone.bass, t.tone.bass, mKnobAlpha);
    c.tone.mid = approach(c.tone.mid, t.tone.mid, mKnobAlpha);
    c.tone.treble = approach(c.tone.treble, t.tone.treble, mKnobAlpha);
    c.presence = approach(c.presence, t.presence, mKnobAlpha);
    for (size_t b = 0; b < c.graphicDb.size(); ++b)
      c.graphicDb[b] = approach(c.graphicDb[b], t.graphicDb[b], mKnobAlpha);
    applyControls();

    double* chunk = output + start;
    mToneStack.process(chunk, n);
    mPresence.process(chunk, n);
    if (mHasGraphic)
      mGraphic.process(chunk, n);
  }
  mControls.main = mMorph.currentPosition();
  mControls.stepped = mTargetControls.stepped;
}

} // namespace nexus
