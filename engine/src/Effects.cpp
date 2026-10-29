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

// ---------------------------------------------------------------- Filtro de variáveis de estado

namespace
{
// Filtro de variáveis de estado (forma "TPT"): estável mesmo com a
// frequência mudando a cada amostra. Devolve o passa-banda (ganho 1 no pico,
// já multiplicado por 1/Q) e o passa-baixa.
struct SvfOut
{
  double band, low;
};
inline SvfOut svf(double x, double hz, double q, double sampleRate, double& ic1, double& ic2)
{
  const double g = std::tan(kPi * std::min(hz, sampleRate * 0.45) / sampleRate);
  const double k = 1.0 / q;
  const double a1 = 1.0 / (1.0 + g * (g + k));
  const double a2 = g * a1;
  const double a3 = g * a2;
  const double v3 = x - ic2;
  const double v1 = a1 * ic1 + a2 * v3;
  const double v2 = ic2 + a2 * ic1 + a3 * v3;
  ic1 = 2.0 * v1 - ic1;
  ic2 = 2.0 * v2 - ic2;
  return {v1 * k, v2};
}

// Coeficiente de um polo simples para a constante de tempo em segundos.
inline double onePole(double seconds, double sampleRate)
{
  return 1.0 - std::exp(-1.0 / (seconds * sampleRate));
}

inline double dbToGain(double db)
{
  return std::pow(10.0, db / 20.0);
}
} // namespace

// ---------------------------------------------------------------- Wah

void Wah::prepare(double sampleRate)
{
  mRate = sampleRate;
  reset();
}

void Wah::reset()
{
  mIc1 = mIc2 = 0.0;
  mPos = -1.0;
}

double Wah::centerHz(double position, double range)
{
  // Varredura em escala de oitavas (o ouvido ouve o pedal andar por igual);
  // a faixa larga desce e sobe um pouco além do Cry Baby.
  const double low = 350.0 / (1.0 + 0.35 * range);
  const double high = 2200.0 * (1.0 + 0.25 * range);
  return low * std::pow(high / low, std::clamp(position, 0.0, 1.0));
}

void Wah::process(double* buffer, int n, double position, double range, double level)
{
  if (mPos < 0.0)
    mPos = position;
  // O pé é mais lento que o botão: 30 ms de suavização (sem "degraus").
  const double footCoef = onePole(0.03, mRate);
  for (int i = 0; i < n; ++i)
  {
    mPos += (position - mPos) * footCoef;
    mRange = smooth(mRange, range, mRate);
    mLevel = smooth(mLevel, level, mRate);
    // Estreito: Q 10 (cantado); largo: Q 2. Na ponta o Q cai um pouco, como
    // no indutor do original.
    const double q = 10.0 * std::pow(0.2, mRange) * (1.0 - 0.25 * mPos);
    const auto f = svf(buffer[i], centerHz(mPos, mRange), q, mRate, mIc1, mIc2);
    // O pico sobe com o Q (o estreito é mais alto, como no pedal); um fio do
    // som limpo e do grave fica por baixo.
    const double wet = f.band * 0.9 * std::sqrt(q) + 0.08 * buffer[i] + 0.05 * f.low;
    buffer[i] = wet * dbToGain((mLevel - 0.5) * 24.0);
  }
}

// ---------------------------------------------------------------- Rotary

void Rotary::prepare(double sampleRate)
{
  mRate = sampleRate;
  mHornLine.prepare(static_cast<int>(0.006 * sampleRate));
  mDrumLine.prepare(static_cast<int>(0.006 * sampleRate));
  reset();
}

void Rotary::reset()
{
  mHornLine.clear();
  mDrumLine.clear();
  mHornPhase = 0.0;
  mDrumPhase = 0.25;
  mHornHz = mDrumHz = -1.0;
  mIc1 = mIc2 = 0.0;
}

void Rotary::process(double* buffer, int n, double speed, double drive, double balance)
{
  // Velocidade: corneta de 0,8 a 6,7 Hz, tambor de 0,67 a 5,9 Hz.
  const double s = std::clamp(speed, 0.0, 1.0);
  const double hornTarget = 0.8 + 5.9 * s;
  const double drumTarget = 0.67 + 5.23 * s;
  if (mHornHz < 0.0)
  {
    mHornHz = hornTarget;
    mDrumHz = drumTarget;
  }
  // Inércia: acelerar é mais rápido que frear; o tambor é pesado.
  const double hornCoef = onePole(hornTarget > mHornHz ? 0.6 : 0.9, mRate);
  const double drumCoef = onePole(drumTarget > mDrumHz ? 3.0 : 4.0, mRate);
  constexpr double kHornAm = 0.45, kDrumAm = 0.3;
  for (int i = 0; i < n; ++i)
  {
    mHornHz += (hornTarget - mHornHz) * hornCoef;
    mDrumHz += (drumTarget - mDrumHz) * drumCoef;
    mDrive = smooth(mDrive, drive, mRate);
    mBalance = smooth(mBalance, balance, mRate);
    // Pré de válvula: ganho 1 nos sinais baixos, arredonda os picos.
    const double g = 1.0 + 7.0 * mDrive * mDrive;
    const double x = std::tanh(buffer[i] * g) / std::sqrt(g);
    // Divisor em 800 Hz: grave pelo passa-baixa, agudo pelo resto (somam
    // de volta exatamente o sinal).
    const double low = svf(x, 800.0, 0.7071, mRate, mIc1, mIc2).low;
    const double high = x - low;
    const double hc = std::cos(2.0 * kPi * mHornPhase);
    const double dc = std::cos(2.0 * kPi * mDrumPhase);
    // Corneta: mais perto do microfone = atraso menor e volume maior.
    mHornLine.push(high);
    const double horn = mHornLine.read((2.0 - 0.45 * hc) * 0.001 * mRate) * (1.0 - kHornAm * (0.5 - 0.5 * hc));
    mDrumLine.push(low);
    const double drum = mDrumLine.read((2.0 - 0.15 * dc) * 0.001 * mRate) * (1.0 - kDrumAm * (0.5 - 0.5 * dc));
    // Equilíbrio de potência igual (meio = os dois cheios); compensa o
    // volume médio que a modulação tira.
    const double hornGain = std::sqrt(2.0 * mBalance) / (1.0 - 0.5 * kHornAm);
    const double drumGain = std::sqrt(2.0 * (1.0 - mBalance)) / (1.0 - 0.5 * kDrumAm);
    buffer[i] = (horn * hornGain + drum * drumGain) * 0.85;
    mHornPhase += mHornHz / mRate;
    if (mHornPhase >= 1.0)
      mHornPhase -= 1.0;
    mDrumPhase += mDrumHz / mRate;
    if (mDrumPhase >= 1.0)
      mDrumPhase -= 1.0;
  }
}

// ---------------------------------------------------------------- AnalogDelay

void AnalogDelay::prepare(double sampleRate)
{
  mRate = sampleRate;
  mLine.prepare(static_cast<int>(1.6 * sampleRate)); // até 1,5 s com o andamento
  reset();
}

void AnalogDelay::reset()
{
  mLine.clear();
  mDelay = 0.0;
  mPhase = 0.0;
  mPhase2 = 0.0;
  mLow1 = mLow2 = mHigh = 0.0;
}

void AnalogDelay::process(const double* input, double* wet, int n, double time, double feedback)
{
  const double secs = syncSeconds > 0.0 ? std::min(1.5, syncSeconds) : seconds(time);
  const double target = secs * mRate;
  if (mDelay <= 0.0)
    mDelay = target;
  // Relógio do BBD: tempo longo = relógio lento = repetição mais escura
  // (de 4,5 kHz em 30 ms a 2 kHz em 550 ms e além).
  const double cutoff = std::clamp(4500.0 * std::pow(secs / 0.03, -0.28), 1200.0, 4500.0);
  const double lowCoef = 1.0 - std::exp(-2.0 * kPi * cutoff / mRate);
  const double highCoef = 1.0 - std::exp(-2.0 * kPi * 70.0 / mRate);
  for (int i = 0; i < n; ++i)
  {
    // Repetições: no máximo 0,98 na volta; com os filtros e a saturação,
    // fica na beira da auto-oscilação sem disparar.
    mFeedback = smooth(mFeedback, std::clamp(feedback, 0.0, 1.0) * 0.98, mRate);
    mDelay += (target - mDelay) * 0.0005;
    // Balanço leve do tempo (o relógio do chip nunca é firme): dois
    // osciladores lentos, ±0,25% do tempo mais 0,1 ms.
    const double wobble = 0.7 * std::sin(2.0 * kPi * mPhase) + 0.3 * std::sin(2.0 * kPi * mPhase2);
    const double echo = mLine.read(mDelay * (1.0 + 0.0025 * wobble) + 0.0001 * mRate * wobble);
    mPhase += 0.43 / mRate;
    if (mPhase >= 1.0)
      mPhase -= 1.0;
    mPhase2 += 1.7 / mRate;
    if (mPhase2 >= 1.0)
      mPhase2 -= 1.0;
    // Dois polos de passa-baixa e um passa-alta leve: cada volta escurece.
    mLow1 += (echo - mLow1) * lowCoef;
    mLow2 += (mLow1 - mLow2) * lowCoef;
    mHigh += (mLow2 - mHigh) * highCoef;
    const double shaped = mLow2 - mHigh;
    // Saturação suave na volta (o chip satura antes de 1,2).
    mLine.push(1.2 * std::tanh((input[i] + shaped * mFeedback) / 1.2));
    wet[i] = shaped;
  }
}

// ---------------------------------------------------------------- Octave

void Octave::prepare(double sampleRate)
{
  mRate = sampleRate;
  reset();
}

void Octave::reset()
{
  mPreIc1 = mPreIc2 = mPre2 = 0.0;
  mEnv = mEnvLow = 0.0;
  mHigh = mFlip1 = mFlip2 = false;
  mSub1Ic1 = mSub1Ic2 = mSub2Ic1 = mSub2Ic2 = 0.0;
}

void Octave::process(double* buffer, int n, double direct, double sub1, double sub2)
{
  const double envUp = onePole(0.001, mRate);
  const double envDown = onePole(0.06, mRate);
  const double pre2Coef = onePole(1.0 / (2.0 * kPi * 400.0), mRate);
  constexpr double kGate = 1e-4; // -80 dB: abaixo disso o contador não anda
  for (int i = 0; i < n; ++i)
  {
    mDirect = smooth(mDirect, direct, mRate);
    mSub1 = smooth(mSub1, sub1, mRate);
    mSub2 = smooth(mSub2, sub2, mRate);
    const double x = buffer[i];
    // Seguidor de envoltória: o volume da nota que a onda quadrada vai ter.
    const double a = std::abs(x);
    mEnv += (a - mEnv) * (a > mEnv ? envUp : envDown);
    // Detector: passa-baixa forte (sobra a fundamental) e a envoltória dele.
    const double pre = svf(x, 250.0, 0.6, mRate, mPreIc1, mPreIc2).low;
    mPre2 += (pre - mPre2) * pre2Coef;
    const double lp = std::abs(mPre2);
    mEnvLow += (lp - mEnvLow) * (lp > mEnvLow ? envUp : envDown);
    // Comparador com histerese (20% da envoltória): sem tremedeira no fim
    // da nota. Cada subida vira o primeiro flip-flop; cada subida dele vira
    // o segundo.
    const double th = 0.2 * mEnvLow + 1e-6;
    if (mEnv > kGate)
    {
      if (!mHigh && mPre2 > th)
      {
        mHigh = true;
        mFlip1 = !mFlip1;
        if (mFlip1)
          mFlip2 = !mFlip2;
      }
      else if (mHigh && mPre2 < -th)
        mHigh = false;
    }
    const double square1 = (mFlip1 ? 1.0 : -1.0) * mEnv;
    const double square2 = (mFlip2 ? 1.0 : -1.0) * mEnv;
    // A quadrada vira um grave redondo (passa-baixa em 600 e 350 Hz).
    const double low1 = svf(square1, 600.0, 0.7071, mRate, mSub1Ic1, mSub1Ic2).low;
    const double low2 = svf(square2, 350.0, 0.7071, mRate, mSub2Ic1, mSub2Ic2).low;
    buffer[i] = 1.2 * (x * mDirect + 0.8 * (low1 * mSub1 + low2 * mSub2));
  }
}

// ---------------------------------------------------------------- Compressor

void Compressor::prepare(double sampleRate)
{
  mRate = sampleRate;
  reset();
}

void Compressor::reset()
{
  mEnv = 0.0;
}

double Compressor::curveDb(double levelDb, double sensitivity)
{
  const double thr = -20.0 - 30.0 * sensitivity;
  const double ratio = 3.0 + 9.0 * sensitivity;
  constexpr double knee = 10.0;
  const double over = levelDb - thr;
  const double slope = 1.0 / ratio - 1.0;
  if (over <= -0.5 * knee)
    return 0.0;
  if (over < 0.5 * knee)
    return slope * (over + 0.5 * knee) * (over + 0.5 * knee) / (2.0 * knee);
  return slope * over;
}

void Compressor::process(double* buffer, int n, double level, double sensitivity, double attack)
{
  // Release lento e fixo, como o do Dyna Comp.
  const double release = onePole(0.3, mRate);
  // Volta do volume: devolve o que a curva tira no nível de referência
  // (-14 dB no detector: é onde fica, na média, uma guitarra com picos perto
  // de -9 dB, já que a nota passa a maior parte do tempo morrendo); assim,
  // no meio do volume, ligado soa como desligado.
  constexpr double kReferenceDb = -14.0;
  for (int i = 0; i < n; ++i)
  {
    mLevel = smooth(mLevel, level, mRate);
    mSensitivity = smooth(mSensitivity, sensitivity, mRate);
    mAttack = smooth(mAttack, attack, mRate);
    // Ataque de 0 (pega o pico na hora, sem deixar a palhetada escapar) a 20 ms.
    const double up = onePole(std::max(1e-6, 0.02 * mAttack * mAttack), mRate);
    const double a = std::abs(buffer[i]);
    mEnv += (a - mEnv) * (a > mEnv ? up : release);
    const double db = 20.0 * std::log10(mEnv + 1e-12);
    const double makeup = -curveDb(kReferenceDb, mSensitivity);
    const double gain = dbToGain(curveDb(db, mSensitivity) + makeup + (mLevel - 0.5) * 24.0);
    // Ataque alto: um pouco do limpo junto (compressão "paralela", natural).
    const double dry = 0.4 * mAttack;
    buffer[i] = buffer[i] * gain * (1.0 - dry) + buffer[i] * dbToGain((mLevel - 0.5) * 24.0) * dry;
  }
}

// ---------------------------------------------------------------- NativePedal

void NativePedal::setSync(double hz, double seconds)
{
  mChorus.syncHz = mPhaser.syncHz = mFlanger.syncHz = mTremolo.syncHz = mVibe.syncHz = hz;
  mDelay.syncSeconds = mAnalogDelay.syncSeconds = seconds;
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
  mWah.prepare(sampleRate);
  mRotary.prepare(sampleRate);
  mAnalogDelay.prepare(sampleRate);
  mOctave.prepare(sampleRate);
  mComp.prepare(sampleRate);
  mGeq.prepare(sampleRate);
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
  mPhaser.reset();
  mFlanger.reset();
  mTremolo.reset();
  mVibe.reset();
  mWah.reset();
  mRotary.reset();
  mAnalogDelay.reset();
  mOctave.reset();
  mComp.reset();
  mGeq.reset();
  mGeqLevelGain = std::pow(10.0, (mGeqLevel - 0.5) * 2.0 * kGeqMaxDb / 20.0);
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

void NativePedal::process(Type type, bool on, double* buffer, int n, const Knobs& knobs, double level)
{
  mGeqKnobs = knobs;
  mGeqLevel = level;
  process(type, on, buffer, n, knobs[0], knobs[1], knobs[2]);
}

void NativePedal::processGeq(double* buffer, int n)
{
  // Botão em 0,5 = 0 dB; 0 = -15 dB; 1 = +15 dB. O FreeEq anda devagar até
  // o ganho pedido (sem chiado de degrau) e pula as faixas em 0 dB: tudo no
  // meio, o som passa intacto.
  auto toDb = [](double v) { return (std::clamp(v, 0.0, 1.0) - 0.5) * 2.0 * kGeqMaxDb; };
  FreeEq::Bands bands{};
  for (int i = 0; i < kGeqBands; ++i)
  {
    auto& b = bands[static_cast<size_t>(i)];
    b.on = true;
    b.type = i == kGeqBands - 1 ? FreeEq::Type::highShelf : FreeEq::Type::bell;
    b.freq = kGeqFrequency[i];
    b.gainDb = toDb(mGeqKnobs[static_cast<size_t>(i)]);
    b.q = 1.4; // cerca de uma oitava de largura, como no GE-7
  }
  mGeq.set(bands);
  mGeq.process(buffer, n);
  const double target = std::pow(10.0, toDb(mGeqLevel) / 20.0);
  for (int i = 0; i < n; ++i)
  {
    mGeqLevelGain = smooth(mGeqLevelGain, target, mRate);
    buffer[i] *= mGeqLevelGain;
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
    mWah.reset();
    mRotary.reset();
    mAnalogDelay.reset();
    mOctave.reset();
    mComp.reset();
    mGeq.reset();
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
  if (type != Type::delay && type != Type::reverb && type != Type::analogdelay)
  {
    // Modulação, wah, oitavador e compressor não têm rastro: liga e
    // desliga com transição de 20 ms.
    std::copy(buffer, buffer + n, mIn.begin());
    switch (type)
    {
      case Type::phaser: mPhaser.process(mIn.data(), n, a, b, c); break;
      case Type::flanger: mFlanger.process(mIn.data(), n, a, b, c); break;
      case Type::tremolo: mTremolo.process(mIn.data(), n, a, b, c); break;
      case Type::vibe: mVibe.process(mIn.data(), n, a, b, c); break;
      case Type::wah: mWah.process(mIn.data(), n, a, b, c); break;
      case Type::rotary: mRotary.process(mIn.data(), n, a, b, c); break;
      case Type::octave: mOctave.process(mIn.data(), n, a, b, c); break;
      case Type::comp: mComp.process(mIn.data(), n, a, b, c); break;
      case Type::geq: processGeq(mIn.data(), n); break;
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
  // Delays e reverb: a entrada do efeito liga e desliga; a saída continua
  // soando até o rastro acabar (o som seco nunca é tocado).
  for (int i = 0; i < n; ++i)
  {
    mGain = on ? std::min(1.0, mGain + step) : std::max(0.0, mGain - step);
    mIn[static_cast<size_t>(i)] = buffer[i] * mGain;
  }
  if (type == Type::delay)
    mDelay.process(mIn.data(), mWet.data(), n, a, b);
  else if (type == Type::analogdelay)
    mAnalogDelay.process(mIn.data(), mWet.data(), n, a, b);
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
