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
  const double hz = syncHz > 0.0 ? syncHz : 0.1 + 5.9 * rate * rate;
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
  mLine.prepare(static_cast<int>(3.1 * sampleRate)); // até 3 s com o andamento
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
  const double seconds = syncSeconds > 0.0 ? std::min(3.0, syncSeconds) : 0.04 * std::pow(30.0, time);
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

// ---------------------------------------------------------------- Phaser

namespace
{
// Passa-tudo de primeira ordem com o coeficiente a (estado em "z").
inline double allPass(double x, double a, double& z)
{
  const double y = a * x + z;
  z = x - a * y;
  return y;
}

// Coeficiente do passa-tudo para a frequência de quebra hz.
inline double allPassCoef(double hz, double sampleRate)
{
  const double t = std::tan(kPi * std::min(hz, sampleRate * 0.45) / sampleRate);
  return (t - 1.0) / (t + 1.0);
}
} // namespace

void Phaser::prepare(double sampleRate)
{
  mRate = sampleRate;
  reset();
}

void Phaser::reset()
{
  mState.fill(0.0);
  mLast = 0.0;
  mPhase = 0.0;
}

void Phaser::process(double* buffer, int n, double rate, double depth, double feedback)
{
  // 0,05 a 8 Hz; varredura de 200 Hz até 200 Hz x (2 a 16), em escala de oitavas.
  const double hz = syncHz > 0.0 ? syncHz : 0.05 + 7.95 * rate * rate;
  const double step = hz / mRate;
  for (int i = 0; i < n; ++i)
  {
    mDepth = smooth(mDepth, depth, mRate);
    mFeedback = smooth(mFeedback, feedback, mRate);
    const double lfo = 0.5 - 0.5 * std::cos(2.0 * kPi * mPhase);
    const double f = 200.0 * std::pow(2.0, lfo * (1.0 + 3.0 * mDepth));
    const double a = allPassCoef(f, mRate);
    double y = buffer[i] + mLast * 0.75 * mFeedback;
    for (auto& z : mState)
      y = allPass(y, a, z);
    mLast = y;
    buffer[i] = 0.5 * (buffer[i] + y) * 1.15;
    mPhase += step;
    if (mPhase >= 1.0)
      mPhase -= 1.0;
  }
}

// ---------------------------------------------------------------- Flanger

void Flanger::prepare(double sampleRate)
{
  mRate = sampleRate;
  mLine.prepare(static_cast<int>(0.012 * sampleRate));
  reset();
}

void Flanger::reset()
{
  mLine.clear();
  mLast = 0.0;
  mPhase = 0.0;
}

void Flanger::process(double* buffer, int n, double rate, double depth, double feedback)
{
  // 0,03 a 4 Hz (flanger gosta de lento); atraso de 0,4 ms até 0,4 + 6,6 ms.
  const double hz = syncHz > 0.0 ? syncHz : 0.03 + 3.97 * rate * rate;
  const double step = hz / mRate;
  for (int i = 0; i < n; ++i)
  {
    mDepth = smooth(mDepth, depth, mRate);
    mFeedback = smooth(mFeedback, feedback, mRate);
    const double lfo = 0.5 - 0.5 * std::cos(2.0 * kPi * mPhase);
    const double ms = 0.4 + 6.6 * mDepth * lfo;
    mLine.push(buffer[i] + mLast * 0.85 * mFeedback);
    const double wet = mLine.read(ms * 0.001 * mRate);
    mLast = wet;
    buffer[i] = 0.5 * (buffer[i] + wet) * 1.1;
    mPhase += step;
    if (mPhase >= 1.0)
      mPhase -= 1.0;
  }
}

// ---------------------------------------------------------------- Tremolo

void Tremolo::prepare(double sampleRate)
{
  mRate = sampleRate;
  reset();
}

void Tremolo::reset()
{
  mPhase = 0.0;
}

void Tremolo::process(double* buffer, int n, double rate, double depth, double shape)
{
  // 1 a 12 Hz.
  const double hz = syncHz > 0.0 ? syncHz : 1.0 + 11.0 * rate;
  const double step = hz / mRate;
  for (int i = 0; i < n; ++i)
  {
    mDepth = smooth(mDepth, depth, mRate);
    mShape = smooth(mShape, shape, mRate);
    const double s = std::sin(2.0 * kPi * mPhase);
    // Forma: seno puro até quase quadrada (seno "espremido"), sem estalo.
    const double k = 1.0 + 14.0 * mShape;
    const double wave = std::tanh(k * s) / std::tanh(k);
    const double gain = 1.0 - mDepth * (0.5 - 0.5 * wave);
    buffer[i] *= gain;
    mPhase += step;
    if (mPhase >= 1.0)
      mPhase -= 1.0;
  }
}

// ---------------------------------------------------------------- Vibe

void Vibe::prepare(double sampleRate)
{
  mRate = sampleRate;
  reset();
}

void Vibe::reset()
{
  mState.fill(0.0);
  mPhase = 0.0;
  mLamp = 0.0;
}

void Vibe::process(double* buffer, int n, double rate, double intensity, double mix)
{
  // 0,5 a 10 Hz. Cada estágio tem a sua frequência (como as células de luz
  // do original, que nunca são iguais).
  static constexpr double kBase[4] = {90.0, 260.0, 720.0, 1900.0};
  const double hz = syncHz > 0.0 ? syncHz : 0.5 + 9.5 * rate * rate;
  const double step = hz / mRate;
  const double rise = 1.0 - std::exp(-1.0 / (0.004 * mRate));
  const double fall = 1.0 - std::exp(-1.0 / (0.030 * mRate));
  for (int i = 0; i < n; ++i)
  {
    mIntensity = smooth(mIntensity, intensity, mRate);
    mMix = smooth(mMix, mix, mRate);
    const double lfo = 0.5 - 0.5 * std::cos(2.0 * kPi * mPhase);
    const double lamp = lfo * lfo; // a lâmpada acende mais devagar no começo
    mLamp += (lamp - mLamp) * (lamp > mLamp ? rise : fall);
    double y = buffer[i];
    for (size_t s = 0; s < mState.size(); ++s)
      y = allPass(y, allPassCoef(kBase[s] * std::pow(2.0, 2.2 * mIntensity * mLamp), mRate), mState[s]);
    // Mistura 0: só o sinal deslocado (vibrato); 1: metade e metade (chorus).
    // Seco e deslocado se cancelam em parte: compensa o volume que some.
    const double wet = 0.5 * mMix;
    buffer[i] = (buffer[i] * wet + y * (1.0 - wet)) * (1.0 + 0.9 * mMix);
    mPhase += step;
    if (mPhase >= 1.0)
      mPhase -= 1.0;
  }
}

// ---------------------------------------------------------------- NativePedal

void NativePedal::setSync(double hz, double seconds)
{
  mChorus.syncHz = mPhaser.syncHz = mFlanger.syncHz = mTremolo.syncHz = mVibe.syncHz = hz;
  mDelay.syncSeconds = mEcho.syncSeconds = seconds;
  mMulti.syncHz = hz;
}

void NativePedal::prepare(double sampleRate, int maxBlock)
{
  mRate = sampleRate;
  mChorus.prepare(sampleRate);
  mDelay.prepare(sampleRate);
  mReverb.prepare(sampleRate);
  mGate.prepare(sampleRate);
  mPhaser.prepare(sampleRate);
  mFlanger.prepare(sampleRate);
  mTremolo.prepare(sampleRate);
  mVibe.prepare(sampleRate);
  mSky.prepare(sampleRate);
  mEcho.prepare(sampleRate);
  mMulti.prepare(sampleRate);
  mTriple.prepare(sampleRate);
  mIn.assign(static_cast<size_t>(maxBlock), 0.0);
  mWet.assign(static_cast<size_t>(maxBlock), 0.0);
  mDry.assign(static_cast<size_t>(maxBlock), 1.0);
  mGains.assign(static_cast<size_t>(maxBlock), 0.0);
  reset();
}

void NativePedal::reset()
{
  mChorus.reset();
  mDelay.reset();
  mReverb.reset();
  mGate.reset();
  mPhaser.reset();
  mFlanger.reset();
  mTremolo.reset();
  mVibe.reset();
  mSky.reset();
  mEcho.reset();
  mMulti.reset();
  mTriple.reset();
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
    mPhaser.reset();
    mFlanger.reset();
    mTremolo.reset();
    mVibe.reset();
    mSky.reset();
    mEcho.reset();
    mMulti.reset();
    mTriple.reset();
    mLast = type;
  }
  // Rastro de até 6 s depois de desligar (o reverb de vários modos tem
  // caudas de até 25 s; o delay, repetições longas).
  const double tailSeconds = type == Type::sky ? 25.0 : type == Type::echo || type == Type::triple ? 12.0 : 6.0;
  if (on)
    mTail = static_cast<int>(tailSeconds * mRate);
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
  if (type == Type::chorus || type == Type::phaser || type == Type::flanger || type == Type::tremolo || type == Type::vibe
      || type == Type::multiMod)
  {
    // Modulação não tem rastro: liga e desliga com transição de 20 ms.
    std::copy(buffer, buffer + n, mIn.begin());
    switch (type)
    {
      case Type::phaser: mPhaser.process(mIn.data(), n, a, b, c); break;
      case Type::flanger: mFlanger.process(mIn.data(), n, a, b, c); break;
      case Type::tremolo: mTremolo.process(mIn.data(), n, a, b, c); break;
      case Type::vibe: mVibe.process(mIn.data(), n, a, b, c); break;
      case Type::multiMod: mMulti.process(mIn.data(), n, mMode, a, b, c, mTone, mFourth); break;
      default: mChorus.process(mIn.data(), n, a, b, c); break;
    }
    for (int i = 0; i < n; ++i)
    {
      mGain = on ? std::min(1.0, mGain + step) : std::max(0.0, mGain - step);
      buffer[i] = buffer[i] * (1.0 - mGain) + mIn[static_cast<size_t>(i)] * mGain;
    }
    if (!on && mGain <= 0.0)
      mTail = 0;
    return;
  }
  // Reverb e delay de vários modos: como os de baixo, mas o modo swell
  // também mexe no seco (a nota entra devagar) enquanto o pedal está ligado.
  if (type == Type::sky || type == Type::echo)
  {
    for (int i = 0; i < n; ++i)
    {
      mGain = on ? std::min(1.0, mGain + step) : std::max(0.0, mGain - step);
      mGains[static_cast<size_t>(i)] = mGain;
      mIn[static_cast<size_t>(i)] = buffer[i] * mGain;
    }
    if (type == Type::sky)
      mSky.process(mIn.data(), mWet.data(), mDry.data(), n, mMode, a, b, mTone, mFourth, mFifth);
    else
      mEcho.process(mIn.data(), mWet.data(), mDry.data(), n, mMode, a, b, mTone, mFourth);
    for (int i = 0; i < n; ++i)
    {
      const auto s = static_cast<size_t>(i);
      mMix = smooth(mMix, c, mRate);
      const double dry = 1.0 - mGains[s] * (1.0 - mDry[s]);
      buffer[i] = buffer[i] * dry + mWet[s] * mMix;
    }
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
  else if (type == Type::triple)
    mTriple.process(mIn.data(), mWet.data(), n, mStages);
  else
    mReverb.process(mIn.data(), mWet.data(), n, a, b);
  // No Blueberry Chain o nível de cada delay já está nas repetições.
  const double target = type == Type::triple ? 1.0 : type == Type::reverb ? c * 1.2 : c;
  for (int i = 0; i < n; ++i)
  {
    mMix = smooth(mMix, target, mRate);
    buffer[i] += mWet[static_cast<size_t>(i)] * mMix;
  }
}
} // namespace nexus
