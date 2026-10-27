#include "nexus/Effects.h"

#include <algorithm>
#include <cmath>

namespace nexus
{
namespace
{
constexpr double kPi = 3.14159265358979323846;

// Um passo de suavização por amostra (cerca de 20 ms).
inline double smooth(double current, double target, double sampleRate)
{
  return current + (target - current) * (1.0 - std::exp(-1.0 / (0.02 * sampleRate)));
}
} // namespace

// ---------------------------------------------------------------- DelayLine

void DelayLine::prepare(int maxSamples)
{
  mBuffer.assign(static_cast<size_t>(std::max(4, maxSamples + 2)), 0.0);
  mWrite = 0;
}

void DelayLine::clear()
{
  std::fill(mBuffer.begin(), mBuffer.end(), 0.0);
}

double DelayLine::read(double delay) const
{
  const int size = static_cast<int>(mBuffer.size());
  delay = std::clamp(delay, 0.0, static_cast<double>(size - 2));
  const int whole = static_cast<int>(delay);
  const double frac = delay - whole;
  const int i0 = (mWrite - whole + size) % size;
  const int i1 = (i0 - 1 + size) % size;
  return mBuffer[static_cast<size_t>(i0)] * (1.0 - frac) + mBuffer[static_cast<size_t>(i1)] * frac;
}

// ---------------------------------------------------------------- Chorus

void Chorus::prepare(double sampleRate)
{
  mRate = sampleRate;
  mLine.prepare(static_cast<int>(0.03 * sampleRate));
  reset();
}

void Chorus::reset()
{
  mLine.clear();
  mPhase = 0.0;
}

void Chorus::process(double* buffer, int n, double rate, double depth, double mix)
{
  // Velocidade de 0,1 a 6 Hz (curva para dar mais resolução no lento).
  const double hz = 0.1 + 5.9 * rate * rate;
  const double step = hz / mRate;
  for (int i = 0; i < n; ++i)
  {
    mDepth = smooth(mDepth, depth, mRate);
    mMix = smooth(mMix, mix, mRate);
    mLine.push(buffer[i]);
    double wet = 0.0;
    for (int v = 0; v < 2; ++v)
    {
      const double lfo = std::sin(2.0 * kPi * (mPhase + v * 0.25));
      const double ms = 9.0 + v * 2.0 + mDepth * 5.0 * lfo;
      wet += mLine.read(ms * 0.001 * mRate);
    }
    wet *= 0.5;
    mPhase += step;
    if (mPhase >= 1.0)
      mPhase -= 1.0;
    // Mistura "igual" à de um chorus de pedal: no meio, seco e molhado juntos.
    buffer[i] = buffer[i] * (1.0 - 0.5 * mMix) + wet * mMix * 0.85;
  }
}

// ---------------------------------------------------------------- Delay

void Delay::prepare(double sampleRate)
{
  mRate = sampleRate;
  mLine.prepare(static_cast<int>(1.3 * sampleRate));
  reset();
}

void Delay::reset()
{
  mLine.clear();
  mDelay = 0.0;
  mLow = mHigh = 0.0;
}

void Delay::process(const double* input, double* wet, int n, double time, double feedback)
{
  // Tempo de 40 ms a 1,2 s em curva (o meio do botão fica perto de 300 ms).
  const double seconds = 0.04 * std::pow(30.0, time);
  const double target = seconds * mRate;
  if (mDelay <= 0.0)
    mDelay = target;
  const double fb = std::min(0.92, feedback * 0.95);
  // Repetições escurecem (passa-baixa em 3,2 kHz) e perdem o grave (passa-alta em 90 Hz).
  const double lowCoef = 1.0 - std::exp(-2.0 * kPi * 3200.0 / mRate);
  const double highCoef = 1.0 - std::exp(-2.0 * kPi * 90.0 / mRate);
  for (int i = 0; i < n; ++i)
  {
    // O tempo desliza devagar, como a fita de um delay de verdade (sem estalo).
    mDelay += (target - mDelay) * 0.0005;
    const double echo = mLine.read(mDelay);
    mLow += (echo - mLow) * lowCoef;
    mHigh += (mLow - mHigh) * highCoef;
    const double shaped = mLow - mHigh;
    // Saturação leve na volta: as repetições não explodem.
    mLine.push(std::tanh(input[i] + shaped * fb));
    wet[i] = shaped;
  }
}

// ---------------------------------------------------------------- Reverb

void Reverb::prepare(double sampleRate)
{
  // Tamanhos do Freeverb (a 44,1 kHz), ajustados para a taxa.
  static const int combs[8] = {1116, 1188, 1277, 1356, 1422, 1491, 1557, 1617};
  static const int allPasses[4] = {556, 441, 341, 225};
  const double scale = sampleRate / 44100.0;
  for (size_t i = 0; i < 8; ++i)
    mCombs[i].buffer.assign(static_cast<size_t>(combs[i] * scale), 0.0);
  for (size_t i = 0; i < 4; ++i)
    mAllPasses[i].buffer.assign(static_cast<size_t>(allPasses[i] * scale), 0.0);
  mPre.assign(static_cast<size_t>(0.012 * sampleRate), 0.0);
  reset();
}

void Reverb::reset()
{
  for (auto& c : mCombs)
  {
    std::fill(c.buffer.begin(), c.buffer.end(), 0.0);
    c.index = 0;
    c.store = 0.0;
  }
  for (auto& a : mAllPasses)
  {
    std::fill(a.buffer.begin(), a.buffer.end(), 0.0);
    a.index = 0;
  }
  std::fill(mPre.begin(), mPre.end(), 0.0);
  mPreIndex = 0;
}

void Reverb::process(const double* input, double* wet, int n, double size, double tone)
{
  const double room = 0.70 + 0.28 * size;   // quanto o som demora a morrer
  const double damp = 0.55 - 0.45 * tone;    // tom alto = cauda mais brilhante
  for (int i = 0; i < n; ++i)
  {
    const double pre = mPre[static_cast<size_t>(mPreIndex)];
    mPre[static_cast<size_t>(mPreIndex)] = input[i] * 0.015;
    mPreIndex = (mPreIndex + 1) % static_cast<int>(mPre.size());
    double out = 0.0;
    for (auto& c : mCombs)
    {
      const double y = c.buffer[static_cast<size_t>(c.index)];
      c.store = y * (1.0 - damp) + c.store * damp;
      c.buffer[static_cast<size_t>(c.index)] = pre + c.store * room;
      c.index = (c.index + 1) % static_cast<int>(c.buffer.size());
      out += y;
    }
    for (auto& a : mAllPasses)
    {
      const double b = a.buffer[static_cast<size_t>(a.index)];
      a.buffer[static_cast<size_t>(a.index)] = out + b * 0.5;
      a.index = (a.index + 1) % static_cast<int>(a.buffer.size());
      out = b - out;
    }
    wet[i] = out;
  }
}

// ---------------------------------------------------------------- NativePedal

void NativePedal::prepare(double sampleRate, int maxBlock)
{
  mRate = sampleRate;
  mChorus.prepare(sampleRate);
  mDelay.prepare(sampleRate);
  mReverb.prepare(sampleRate);
  mGate.prepare(sampleRate);
  mIn.assign(static_cast<size_t>(maxBlock), 0.0);
  mWet.assign(static_cast<size_t>(maxBlock), 0.0);
  reset();
}

void NativePedal::reset()
{
  mChorus.reset();
  mDelay.reset();
  mReverb.reset();
  mGate.reset();
  mGain = 0.0;
  mMix = 0.0;
  mTail = 0;
}

void NoiseGate::prepare(double sampleRate)
{
  mRate = sampleRate;
  reset();
}

void NoiseGate::reset()
{
  mEnv = 0.0;
  mGain = 1.0;
  mHold = 0;
  mOpen = true;
}

double NoiseGate::gainDb() const
{
  return 20.0 * std::log10(mGain + 1e-9);
}

void NoiseGate::process(double* buffer, int n, double sensitivity, double attack, double release)
{
  const double open = std::pow(10.0, thresholdDb(sensitivity) / 20.0);
  const double close = open * 0.5; // 6 dB abaixo
  const double attackMs = 0.1 * std::pow(300.0, std::clamp(attack, 0.0, 1.0));
  const double releaseMs = 10.0 * std::pow(150.0, std::clamp(release, 0.0, 1.0));
  // Detector: sobe na hora, desce em 10 ms.
  const double envFall = std::exp(-1.0 / (0.010 * mRate));
  const double up = 1.0 - std::exp(-1.0 / (attackMs * 0.001 * mRate));
  const double down = 1.0 - std::exp(-1.0 / (releaseMs * 0.001 * mRate));
  const int hold = static_cast<int>(0.025 * mRate);
  constexpr double floorGain = 1e-5; // -100 dB: fechado de verdade
  for (int i = 0; i < n; ++i)
  {
    const double a = std::abs(buffer[i]);
    mEnv = a > mEnv ? a : mEnv * envFall;
    if (mEnv >= open)
    {
      mOpen = true;
      mHold = hold;
    }
    else if (mEnv < close)
    {
      if (mHold > 0)
        --mHold;
      else
        mOpen = false;
    }
    const double target = mOpen ? 1.0 : floorGain;
    mGain += (target - mGain) * (target > mGain ? up : down);
    buffer[i] *= mGain;
  }
}

void NativePedal::process(Type type, bool on, double* buffer, int n, double a, double b, double c)
{
  // Trocar de efeito no meio: começa limpo.
  if (type != mLast)
  {
    mChorus.reset();
    mDelay.reset();
    mReverb.reset();
    mGate.reset();
    mLast = type;
  }
  if (on)
    mTail = static_cast<int>(6.0 * mRate); // rastro de até 6 s depois de desligar
  else if (mTail > 0)
    mTail -= n;
  if (!on && mGain <= 0.0 && mTail <= 0)
    return; // desligado e em silêncio

  const double step = 1.0 / (0.02 * mRate);
  if (type == Type::gate)
  {
    // Gate: liga e desliga com transição de 20 ms; desligado, o som passa.
    std::copy(buffer, buffer + n, mIn.begin());
    mGate.process(mIn.data(), n, a, b, c);
    for (int i = 0; i < n; ++i)
    {
      mGain = on ? std::min(1.0, mGain + step) : std::max(0.0, mGain - step);
      buffer[i] = buffer[i] * (1.0 - mGain) + mIn[static_cast<size_t>(i)] * mGain;
    }
    if (!on && mGain <= 0.0)
      mTail = 0;
    return;
  }
  if (type == Type::chorus)
  {
    // Modulação não tem rastro: liga e desliga com transição de 20 ms.
    std::copy(buffer, buffer + n, mIn.begin());
    mChorus.process(mIn.data(), n, a, b, c);
    for (int i = 0; i < n; ++i)
    {
      mGain = on ? std::min(1.0, mGain + step) : std::max(0.0, mGain - step);
      buffer[i] = buffer[i] * (1.0 - mGain) + mIn[static_cast<size_t>(i)] * mGain;
    }
    if (!on && mGain <= 0.0)
      mTail = 0;
    return;
  }
  // Delay e reverb: a entrada do efeito liga e desliga; a saída continua
  // soando até o rastro acabar (o som seco nunca é tocado).
  for (int i = 0; i < n; ++i)
  {
    mGain = on ? std::min(1.0, mGain + step) : std::max(0.0, mGain - step);
    mIn[static_cast<size_t>(i)] = buffer[i] * mGain;
  }
  if (type == Type::delay)
    mDelay.process(mIn.data(), mWet.data(), n, a, b);
  else
    mReverb.process(mIn.data(), mWet.data(), n, a, b);
  const double target = type == Type::reverb ? c * 1.2 : c;
  for (int i = 0; i < n; ++i)
  {
    mMix = smooth(mMix, target, mRate);
    buffer[i] += mWet[static_cast<size_t>(i)] * mMix;
  }
}
} // namespace nexus
