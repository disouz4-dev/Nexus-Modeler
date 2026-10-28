#include "nexus/Ambience.h"

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

// Coeficiente de um filtro de um polo na frequência hz.
inline double onePole(double hz, double sampleRate)
{
  return 1.0 - std::exp(-2.0 * kPi * std::min(hz, sampleRate * 0.45) / sampleRate);
}

inline double wrap(double phase)
{
  return phase - std::floor(phase);
}

// Segue o volume: sobe em "attack" e desce em "release" (coeficientes por amostra).
inline void follow(double& env, double x, double attack, double release)
{
  const double a = std::abs(x);
  env += (a - env) * (a > env ? attack : release);
}

// Valor de 0 a 1 de um botão preso a um modo (0 a modeCount - 1).
inline int clampMode(int mode, int count)
{
  return std::clamp(mode, 0, count - 1);
}
} // namespace

// ---------------------------------------------------------------- PitchShifter

void PitchShifter::prepare(double sampleRate, double windowSeconds)
{
  mWindow = std::max(64.0, windowSeconds * sampleRate);
  mLine.prepare(static_cast<int>(mWindow) + 8);
  reset();
}

void PitchShifter::reset()
{
  mLine.clear();
  mPhase = 0.0;
}

double PitchShifter::process(double x, double ratio)
{
  mLine.push(x);
  // A leitura anda mais rápido (ou mais devagar) que a escrita; quando chega
  // ao fim da janela, a outra leitura já assumiu.
  mPhase = wrap(mPhase + (1.0 - ratio) / mWindow);
  double out = 0.0;
  for (const double offset : {0.0, 0.5})
  {
    const double p = wrap(mPhase + offset);
    const double w = std::sin(kPi * p);
    out += w * w * mLine.read(2.0 + p * mWindow);
  }
  return out;
}

// ---------------------------------------------------------------- StateVariable

void StateVariable::process(double x, double hz, double q, double sampleRate)
{
  // Dois passos por amostra: estável até perto de um quarto da taxa.
  const double f = 2.0 * std::sin(kPi * std::min(hz, sampleRate * 0.2) / (2.0 * sampleRate));
  const double damp = 1.0 / std::max(0.5, q);
  for (int i = 0; i < 2; ++i)
  {
    low += f * band;
    high = x - low - damp * band;
    band += f * high;
  }
}

// ---------------------------------------------------------------- SkyReverb

namespace
{
struct SkyShape
{
  double scale;     // tamanho das linhas (1 = sala média)
  double diffusion; // difusão na entrada
  int diffusers;    // quantos difusores
  double rtMin, rtMax; // decaimento (s) nos dois extremos do botão
  double modAmount; // quanto a modulação mexe nas linhas
  double bright;    // brilho da cauda (multiplica o corte do tom)
  double lowCut;    // corte de grave na saída (Hz)
  double level;     // volume da cauda
};
// Na ordem de SkyReverb::Mode.
constexpr SkyShape kSkyShapes[SkyReverb::modeCount] = {
  {1.35, 0.62, 4, 0.8, 10.0, 1.0, 1.0, 80.0, 1.0},  // hall
  {0.50, 0.55, 4, 0.25, 2.5, 0.5, 0.9, 90.0, 1.1},  // sala
  {0.85, 0.72, 4, 0.5, 7.0, 0.6, 1.5, 100.0, 0.9},  // placa
  {0.45, 0.45, 2, 0.6, 4.0, 0.3, 0.7, 150.0, 0.8},  // mola
  {2.40, 0.75, 6, 3.0, 25.0, 2.0, 0.8, 120.0, 0.8}, // nuvem
  {1.35, 0.62, 4, 1.0, 12.0, 1.0, 1.0, 80.0, 1.0},  // swell
  {1.60, 0.65, 5, 2.0, 18.0, 1.2, 1.1, 150.0, 0.9}, // shimmer oitava acima
  {1.60, 0.65, 5, 2.0, 18.0, 1.2, 1.1, 150.0, 0.9}, // quinta + oitava
  {1.60, 0.65, 5, 2.0, 18.0, 1.2, 0.8, 90.0, 0.9},  // oitava abaixo
  {1.60, 0.65, 5, 2.0, 18.0, 1.2, 1.1, 150.0, 0.9}, // shimmer na cauda
  {1.60, 0.65, 5, 2.0, 18.0, 1.2, 1.0, 100.0, 0.9}, // shimmer duplo
};
// Linhas da rede (ms, sala média): números sem divisor comum.
constexpr double kLineMs[8] = {31.7, 37.3, 41.9, 47.5, 53.9, 59.3, 67.1, 73.7};
constexpr double kDiffuserMs[6] = {4.77, 3.59, 2.73, 1.79, 7.31, 11.1};
// Reflexões iniciais (ms depois do pré-atraso) e o volume de cada uma.
constexpr double kEarlyMs[6] = {4.3, 7.9, 11.2, 15.7, 19.1, 23.5};
constexpr double kEarlyGain[6] = {0.8, -0.65, 0.55, -0.45, 0.38, -0.3};

// Matriz de Hadamard 8x8 (rápida), normalizada: mistura sem ganhar energia.
inline void hadamard8(std::array<double, 8>& v)
{
  for (int size = 1; size < 8; size *= 2)
    for (int i = 0; i < 8; i += 2 * size)
      for (int j = i; j < i + size; ++j)
      {
        const double a = v[static_cast<size_t>(j)];
        const double b = v[static_cast<size_t>(j + size)];
        v[static_cast<size_t>(j)] = a + b;
        v[static_cast<size_t>(j + size)] = a - b;
      }
  constexpr double norm = 0.35355339059327373; // 1 / raiz de 8
  for (auto& x : v)
    x *= norm;
}
} // namespace

const char* SkyReverb::modeName(int mode)
{
  static const char* const names[modeCount] = {"Hall",
                                               "Sala",
                                               "Placa",
                                               "Mola",
                                               "Nuvem",
                                               "Swell",
                                               "Shimmer oitava acima",
                                               "Shimmer quinta + oitava",
                                               "Shimmer oitava abaixo",
                                               "Shimmer na cauda",
                                               "Shimmer duplo"};
  return names[clampMode(mode, modeCount)];
}

double SkyReverb::AllPass::process(double x, double g)
{
  const double d = buffer[static_cast<size_t>(index)];
  const double y = d - g * x;
  buffer[static_cast<size_t>(index)] = x + g * y;
  index = (index + 1) % length;
  return y;
}

void SkyReverb::prepare(double sampleRate)
{
  mRate = sampleRate;
  mPre.prepare(static_cast<int>(0.6 * sampleRate));
  for (auto& d : mDiffusers)
    d.buffer.assign(static_cast<size_t>(0.05 * sampleRate), 0.0);
  for (auto& line : mLines)
    line.prepare(static_cast<int>(0.25 * sampleRate));
  for (auto& s : mSprings)
    s.line.prepare(static_cast<int>(0.06 * sampleRate));
  mShiftA.prepare(sampleRate, 0.08);
  mShiftB.prepare(sampleRate, 0.07);
  mMode = -1;
  reset();
}

void SkyReverb::reset()
{
  mPre.clear();
  for (auto& d : mDiffusers)
  {
    std::fill(d.buffer.begin(), d.buffer.end(), 0.0);
    d.index = 0;
  }
  for (auto& line : mLines)
    line.clear();
  for (auto& s : mSprings)
  {
    s.line.clear();
    s.chirp.fill(0.0);
    s.low = 0.0;
  }
  mDamp.fill(0.0);
  mFeedback.fill(0.0);
  for (size_t k = 0; k < mLfo.size(); ++k)
    mLfo[k] = static_cast<double>(k) / static_cast<double>(mLfo.size());
  mShiftA.reset();
  mShiftB.reset();
  mShimmerLow = mShimmerHigh = 0.0;
  mOutLow = mOutHigh = 0.0;
  mEnvIn = mEnvWet = mTailGate = 0.0;
  mSwell = 1.0;
}

void SkyReverb::process(const double* in, double* wet, double* dry, int n, int mode, double decay, double preDelay, double tone,
                        double mod, double shimmer)
{
  mode = clampMode(mode, modeCount);
  const auto& shape = kSkyShapes[mode];
  if (mode != mMode)
  {
    // Outro modo: outra sala. Começa limpo e acerta o tamanho das peças.
    reset();
    mMode = mode;
    const double diffScale = std::min(2.2, shape.scale);
    for (size_t j = 0; j < mDiffusers.size(); ++j)
    {
      auto& d = mDiffusers[j];
      d.length = std::clamp(static_cast<int>(kDiffuserMs[j] * diffScale * 0.001 * mRate), 1, static_cast<int>(d.buffer.size()));
    }
    mSprings[0].length = 0.0331 * mRate;
    mSprings[1].length = 0.0417 * mRate;
  }
  // Decaimento e tom mudam por bloco (as contas são caras); o resto, por amostra.
  mDecay += (decay - mDecay) * 0.3;
  mTone += (tone - mTone) * 0.3;
  const double rt = shape.rtMin * std::pow(shape.rtMax / shape.rtMin, mDecay);
  std::array<double, kLines> length{}, gain{};
  for (int k = 0; k < kLines; ++k)
  {
    length[static_cast<size_t>(k)] = kLineMs[k] * shape.scale * 0.001 * mRate;
    gain[static_cast<size_t>(k)] = std::pow(10.0, -3.0 * length[static_cast<size_t>(k)] / (rt * mRate));
  }
  const double cutoff = 1500.0 * std::pow(2.0, mTone * 3.3) * shape.bright;
  const double dampCoef = onePole(cutoff, mRate);
  const double outLowCoef = onePole(cutoff * 1.3, mRate);
  const double outHighCoef = onePole(shape.lowCut, mRate);
  const double shimLowCoef = onePole(7000.0, mRate);
  const double shimHighCoef = onePole(300.0, mRate);
  const double springLowCoef = onePole(2500.0 + 2500.0 * mTone, mRate);
  const double springGain = std::pow(10.0, -3.0 * 0.037 / std::max(0.3, rt));
  const double fast = 1.0 - std::exp(-1.0 / (0.005 * mRate));
  const double slow = 1.0 - std::exp(-1.0 / (0.2 * mRate));
  const double gateCoef = 1.0 - std::exp(-1.0 / (0.4 * mRate));
  const bool shimmerMode = mode >= shimmerUp;
  const bool earlyMode = mode == hall || mode == room || mode == plate || mode == swell;

  for (int i = 0; i < n; ++i)
  {
    mPreDelay = smooth(mPreDelay, preDelay, mRate);
    mMod = smooth(mMod, mod, mRate);
    mShimmer = smooth(mShimmer, shimmer, mRate);
    double x = in[i];

    // Swell: a nota começa em silêncio e sobe (o seco também).
    if (mode == swell)
    {
      follow(mEnvIn, x, fast, slow);
      const double rise = 1.0 / ((0.08 + 2.0 * mShimmer * mShimmer) * mRate);
      if (mEnvIn < 0.002)
        mSwell = std::max(0.0, mSwell - 1.0 / (0.03 * mRate));
      else
        mSwell = std::min(1.0, mSwell + rise);
      x *= mSwell;
      dry[i] = mSwell;
    }
    else
      dry[i] = 1.0;

    mPre.push(x);
    const double preSamples = mPreDelay * mPreDelay * 0.5 * mRate;
    const double pre = mPre.read(preSamples);

    double early = 0.0;
    if (earlyMode)
      for (int t = 0; t < 6; ++t)
        early += kEarlyGain[t] * mPre.read(preSamples + kEarlyMs[t] * shape.scale * 0.001 * mRate);

    // Mola: duas molas (laços curtos com passa-tudos que "espalham" o som,
    // o gotejar típico). O botão de shimmer, aqui, é o quanto goteja.
    double springOut = 0.0;
    double feed = pre;
    if (mode == spring)
    {
      const double chirp = -(0.45 + 0.4 * mShimmer);
      for (auto& s : mSprings)
      {
        double y = s.line.read(s.length);
        for (auto& z : s.chirp)
        {
          const double out = chirp * y + z;
          z = y - chirp * out;
          y = out;
        }
        s.low += (y - s.low) * springLowCoef;
        s.line.push(std::tanh(pre * 0.6 + s.low * springGain));
        springOut += s.low;
      }
      springOut *= 0.5;
      feed = pre * 0.3 + springOut * 0.4;
    }

    // Shimmer: a cauda volta para dentro com a afinação trocada.
    if (shimmerMode)
    {
      double last = 0.0;
      for (size_t k = 0; k < mFeedback.size(); ++k)
        last += mFeedback[k] * ((k & 1) ? -1.0 : 1.0);
      last *= 0.35;
      double shifted = 0.0;
      switch (mode)
      {
        case shimmerFifth: shifted = 0.55 * mShiftA.process(last, 2.0) + 0.55 * mShiftB.process(last, 1.5); break;
        case shimmerDown: shifted = mShiftA.process(last, 0.5); break;
        case shimmerDual: shifted = 0.6 * mShiftA.process(last, 2.0) + 0.6 * mShiftB.process(last, 0.5); break;
        default: shifted = mShiftA.process(last, 2.0); break;
      }
      mShimmerLow += (shifted - mShimmerLow) * shimLowCoef;
      mShimmerHigh += (mShimmerLow - mShimmerHigh) * shimHighCoef;
      double amount = mShimmer * 0.85;
      if (mode == shimmerTail)
      {
        // Só floresce quando a guitarra para: enquanto toca, a cauda é limpa.
        follow(mEnvIn, x, fast, slow);
        follow(mEnvWet, last, fast, slow);
        const double target = mEnvIn > 0.6 * mEnvWet ? 0.0 : 1.0;
        mTailGate += (target - mTailGate) * gateCoef;
        amount *= mTailGate;
      }
      feed += std::tanh((mShimmerLow - mShimmerHigh) * amount * 1.2) * 0.6;
    }

    // Difusão: o ataque vira uma nuvem de pequenos ecos antes da cauda.
    for (int j = 0; j < shape.diffusers; ++j)
      feed = mDiffusers[static_cast<size_t>(j)].process(feed, shape.diffusion);

    // Rede de oito linhas: cada uma lê, escurece, perde um pouco e passa
    // pela matriz para todas as outras.
    const double depth = mMod * mMod * shape.modAmount * 0.0016 * mRate;
    std::array<double, kLines> v{};
    double out = 0.0;
    for (size_t k = 0; k < static_cast<size_t>(kLines); ++k)
    {
      mLfo[k] = wrap(mLfo[k] + (0.25 + 0.13 * static_cast<double>(k)) * (0.4 + mMod) / mRate);
      const double y = mLines[k].read(length[k] + depth * (1.0 + std::sin(2.0 * kPi * mLfo[k])));
      mDamp[k] += (y - mDamp[k]) * dampCoef;
      v[k] = mDamp[k] * gain[k];
      out += y * ((k & 1) ? -1.0 : 1.0);
    }
    mFeedback = v;
    hadamard8(v);
    for (size_t k = 0; k < static_cast<size_t>(kLines); ++k)
      mLines[k].push(v[k] + feed * ((k % 3) ? 0.5 : -0.5));
    out *= 0.35;
    if (mode == spring)
      out = out * 0.5 + springOut * 1.6;
    if (earlyMode)
      out += early * 0.25 * (0.3 + mShimmer);

    // Tom da saída: corta o agudo junto com o tom e o grave que embola.
    mOutLow += (out - mOutLow) * outLowCoef;
    mOutHigh += (mOutLow - mOutHigh) * outHighCoef;
    wet[i] = (mOutLow - mOutHigh) * shape.level;
  }
}

// ---------------------------------------------------------------- EchoDelay

const char* EchoDelay::modeName(int mode)
{
  static const char* const names[modeCount] = {"Digital", "Analógico", "Fita",      "Multi-cabeça", "Ping-pong", "Reverso",
                                               "Modulado", "Lo-fi",     "Slapback", "Swell",        "Dueto"};
  return names[clampMode(mode, modeCount)];
}

void EchoDelay::prepare(double sampleRate)
{
  mRate = sampleRate;
  mLine.prepare(static_cast<int>(4.2 * sampleRate));
  mLineB.prepare(static_cast<int>(4.2 * sampleRate));
  mMode = -1;
  reset();
}

void EchoDelay::reset()
{
  mLine.clear();
  mLineB.clear();
  mDelay = 0.0;
  mLow = mHigh = mLowB = mHighB = 0.0;
  mWow = mFlutter = mDrift = 0.0;
  mReverse = 0.0;
  mHeld = mHoldCount = 0.0;
  mSwell = 1.0;
}

void EchoDelay::process(const double* in, double* wet, double* dry, int n, int mode, double time, double repeats, double tone, double mod)
{
  mode = clampMode(mode, modeCount);
  if (mode != mMode)
  {
    reset();
    mMode = mode;
  }
  double seconds = syncSeconds > 0.0 ? std::min(3.0, syncSeconds) : 0.04 * std::pow(50.0, time); // 40 ms a 2 s
  if (mode == slapback && syncSeconds <= 0.0)
    seconds = 0.06 + 0.16 * time; // 60 a 220 ms
  if (mode == multiHead)
    seconds = std::min(seconds, 1.3); // a terceira cabeça fica em 3x
  if (mode == reverse)
    seconds = std::min(seconds, 2.0);
  const double target = seconds * mRate;
  if (mDelay <= 0.0)
    mDelay = target;
  // Fita escorrega devagar no tempo novo (a afinação balança); o resto, mais rápido.
  const double glide = mode == tape || mode == multiHead ? 0.0002 : 0.0006;

  // Corte de agudo e de grave das repetições, por modo (o tom move o agudo).
  double lowHz = 6000.0, highHz = 60.0, drive = 1.0;
  switch (mode)
  {
    case digital: lowHz = 3000.0 * std::pow(2.0, tone * 2.6); highHz = 40.0; break;
    case analog: lowHz = 1200.0 * std::pow(2.0, tone * 1.9); highHz = 100.0; drive = 1.4; break;
    case tape:
    case multiHead:
    case slapback: lowHz = 2200.0 * std::pow(2.0, tone * 1.9); highHz = 80.0; drive = 1.8; break;
    case lofi: lowHz = 1600.0 + 1800.0 * tone; highHz = 450.0; drive = 2.0; break;
    default: lowHz = 2500.0 * std::pow(2.0, tone * 2.2); highHz = 70.0; drive = 1.2; break;
  }
  const double lowCoef = onePole(lowHz, mRate);
  const double highCoef = onePole(highHz, mRate);
  // Ping-pong: a outra "volta" é mais escura (o lado de lá).
  const double lowCoefB = onePole(lowHz * 0.55, mRate);
  const double fast = 1.0 - std::exp(-1.0 / (0.002 * mRate));
  const double slow = 1.0 - std::exp(-1.0 / (0.08 * mRate));

  for (int i = 0; i < n; ++i)
  {
    mRepeats = smooth(mRepeats, repeats, mRate);
    mMod = smooth(mMod, mod, mRate);
    mDelay += (target - mDelay) * glide;
    double x = in[i];
    dry[i] = 1.0;

    if (mode == swell)
    {
      // Volume que sobe a cada nota (o ajuste é o tempo da subida).
      follow(mHeld, x, fast, slow);
      if (mHeld < 0.002)
        mSwell = std::max(0.0, mSwell - 1.0 / (0.03 * mRate));
      else
        mSwell = std::min(1.0, mSwell + 1.0 / ((0.05 + 2.0 * mMod * mMod) * mRate));
      x *= mSwell;
      dry[i] = mSwell;
    }

    // Balanço da fita (lento), tremida (rápida) e um desvio que vaga devagar.
    mWow = wrap(mWow + 0.55 / mRate);
    mFlutter = wrap(mFlutter + 6.3 / mRate);
    mDrift = wrap(mDrift + 0.13 / mRate);
    double wobble = 0.0;
    const double m2 = mMod * mMod;
    switch (mode)
    {
      case tape:
      case multiHead:
      case slapback:
        wobble = m2 * mRate * (0.0022 * std::sin(2.0 * kPi * mWow) + 0.0003 * std::sin(2.0 * kPi * mFlutter)
                               + 0.0009 * std::sin(2.0 * kPi * mDrift));
        break;
      case analog: wobble = m2 * mRate * 0.0012 * std::sin(2.0 * kPi * mWow * 1.6); break;
      case modulated: wobble = mMod * mRate * 0.004 * std::sin(2.0 * kPi * mWow * 1.4); break;
      case digital: wobble = m2 * mRate * 0.0004 * std::sin(2.0 * kPi * mWow); break;
      default: break;
    }
    const double d = std::max(1.0, mDelay + wobble);
    double fb = std::min(0.95, mRepeats * 0.98);
    double echo = 0.0, out = 0.0;

    switch (mode)
    {
      case multiHead:
      {
        // Três cabeças: 1x, 2x e 3x o tempo, a primeira mais forte.
        echo = 0.55 * mLine.read(d) + 0.4 * mLine.read(2.0 * d) + 0.33 * mLine.read(3.0 * d);
        fb = std::min(0.9, mRepeats * 0.9);
        break;
      }
      case reverse:
      {
        // Duas leituras de trás para frente, cada uma em janela suave.
        mReverse = wrap(mReverse + 1.0 / std::max(1.0, mDelay));
        for (const double offset : {0.0, 0.5})
        {
          const double p = wrap(mReverse + offset);
          const double w = std::sin(kPi * p);
          echo += w * w * mLine.read(2.0 + 2.0 * p * mDelay);
        }
        fb = std::min(0.8, mRepeats * 0.8);
        break;
      }
      case slapback: echo = mLine.read(d); fb = std::min(0.35, mRepeats * 0.4); break;
      default: echo = mLine.read(d); break;
    }

    mLow += (echo - mLow) * lowCoef;
    mHigh += (mLow - mHigh) * highCoef;
    double shaped = mLow - mHigh;

    if (mode == lofi)
    {
      // Rádio velho: menos amostras e menos degraus de volume.
      const double hold = 1.0 + mMod * 10.0;
      mHoldCount += 1.0;
      if (mHoldCount >= hold)
      {
        mHoldCount -= hold;
        const double steps = std::pow(2.0, 11.0 - mMod * 7.0);
        mHeld = std::round(shaped * steps) / steps;
      }
      shaped = mHeld;
    }

    if (mode == pingPong)
    {
      // Duas voltas: a A repete na B, a B volta na A. Cada repetição alterna o timbre.
      const double back = mLineB.read(d);
      mLowB += (back - mLowB) * lowCoefB;
      mHighB += (mLowB - mHighB) * highCoef;
      const double other = mLowB - mHighB;
      mLine.push(std::tanh(x + other * fb));
      mLineB.push(shaped);
      out = shaped + other * 0.85;
    }
    else if (mode == duet)
    {
      // Dueto: a segunda cabeça em 3/4 do tempo (a colcheia pontuada da semínima).
      const double second = mLineB.read(std::max(1.0, mDelay * 0.75));
      mLowB += (second - mLowB) * lowCoef;
      mHighB += (mLowB - mHighB) * highCoef;
      const double other = mLowB - mHighB;
      mLine.push(std::tanh(x + shaped * fb));
      mLineB.push(std::tanh(x + other * fb * 0.9));
      out = shaped * (1.0 - 0.6 * mMod) + other * (0.4 + 0.6 * mMod);
    }
    else
    {
      // Saturação na volta (mais forte na fita e no analógico): nunca explode.
      mLine.push(std::tanh(drive * (x + shaped * fb)) / drive);
      out = shaped;
    }
    wet[i] = out;
  }
}

// ---------------------------------------------------------------- MultiMod

const char* MultiMod::modeName(int mode)
{
  static const char* const names[modeCount] = {"Rotary",         "Tremolo harmônico", "Vibrato",          "Filtro de envelope",
                                               "Wah automático", "Modulador em anel", "Chorus triplo",    "Tremolo picotado",
                                               "Phaser 12",      "Flanger por zero",  "Detune"};
  return names[clampMode(mode, modeCount)];
}

void MultiMod::prepare(double sampleRate)
{
  mRate = sampleRate;
  mLine.prepare(static_cast<int>(0.04 * sampleRate));
  mUp.prepare(sampleRate, 0.03);
  mDown.prepare(sampleRate, 0.037);
  mMode = -1;
  reset();
}

void MultiMod::reset()
{
  mLine.clear();
  mStages.fill(0.0);
  mFilter.reset();
  mSplit.reset();
  mSplitB.reset();
  mUp.reset();
  mDown.reset();
  mPhase = mPhaseB = 0.0;
  mHornHz = 0.8;
  mDrumHz = 0.7;
  mEnv = mLast = mToneLow = 0.0;
}

void MultiMod::process(double* buffer, int n, int mode, double rate, double depth, double mix, double tone, double tweak)
{
  mode = clampMode(mode, modeCount);
  if (mode != mMode)
  {
    reset();
    mMode = mode;
  }
  const double hz = syncHz > 0.0 ? syncHz : 0.1 + 9.9 * rate * rate;
  // Rotary: lento (coral) ou rápido (tremolo), com a inércia das cornetas.
  const bool fastRotor = syncHz > 0.0 ? syncHz >= 3.0 : rate >= 0.5;
  const double hornTarget = syncHz > 0.0 ? syncHz : (fastRotor ? 6.8 : 0.85);
  const double drumTarget = hornTarget * (fastRotor ? 0.87 : 0.8);
  const double hornInertia = 1.0 - std::exp(-1.0 / (0.7 * mRate));
  const double drumInertia = 1.0 - std::exp(-1.0 / (2.5 * mRate));
  const double toneCoef = onePole(1800.0 * std::pow(2.0, tone * 3.6), mRate);
  const double attack = 1.0 - std::exp(-1.0 / (0.002 * mRate));
  const double release = 1.0 - std::exp(-1.0 / ((0.03 + 0.6 * rate) * mRate));
  const double edge = 1.0 - std::exp(-1.0 / (0.002 * mRate));
  const bool decorrelated = mode == vibrato || mode == triChorus || mode == thruZero || mode == detune;

  for (int i = 0; i < n; ++i)
  {
    mDepth = smooth(mDepth, depth, mRate);
    mMix = smooth(mMix, mix, mRate);
    mTone = smooth(mTone, tone, mRate);
    mTweak = smooth(mTweak, tweak, mRate);
    const double x = buffer[i];
    double y = x;
    double step = hz;

    switch (mode)
    {
      case rotary:
      {
        mHornHz += (hornTarget - mHornHz) * hornInertia;
        mDrumHz += (drumTarget - mDrumHz) * drumInertia;
        const double driven = std::tanh(x * (1.0 + 4.0 * mTweak)) / (1.0 + 1.5 * mTweak);
        mSplit.process(driven, 800.0, 0.7, mRate);
        const double lows = mSplit.low;
        const double highs = driven - lows;
        mLine.push(highs);
        const double horn = std::sin(2.0 * kPi * mPhase);
        // Corneta: chega e vai (efeito Doppler) e fica mais alta quando vira para frente.
        const double hornOut = mLine.read(0.0012 * mRate * (1.0 + 0.8 * mDepth * horn))
                               * (1.0 - 0.45 * mDepth * (0.5 + 0.5 * std::sin(2.0 * kPi * mPhase + 0.6)));
        const double drumOut = lows * (1.0 - 0.35 * mDepth * (0.5 + 0.5 * std::sin(2.0 * kPi * mPhaseB)));
        y = (hornOut + drumOut) * 0.85;
        mPhaseB = wrap(mPhaseB + mDrumHz / mRate);
        step = mHornHz;
        break;
      }
      case harmonicTremolo:
      {
        // Grave e agudo sobem e descem em tempos opostos (Fender marrom).
        const double s = std::sin(2.0 * kPi * mPhase);
        mSplit.process(x, 300.0 * std::pow(5.0, mTweak), 0.6, mRate);
        const double lows = mSplit.low;
        const double highs = x - lows;
        y = lows * (1.0 - mDepth * 0.5 * (1.0 + s)) + highs * (1.0 - mDepth * 0.5 * (1.0 - s));
        y *= 1.0 + 0.4 * mDepth;
        break;
      }
      case vibrato:
      {
        mLine.push(x);
        y = mLine.read((0.002 + mDepth * 0.004 * (0.5 + 0.5 * std::sin(2.0 * kPi * mPhase))) * mRate);
        break;
      }
      case envelopeFilter:
      {
        // A palhetada abre o filtro; a velocidade é o quanto demora a fechar.
        follow(mEnv, x, attack, release);
        const double open = std::min(1.0, mEnv * 10.0);
        mFilter.process(x, 220.0 * std::pow(2.0, open * (1.0 + 4.5 * mDepth)), 1.0 + 7.0 * mTweak, mRate);
        y = mFilter.band * 1.55 + mFilter.low * 0.5;
        step = 0.0;
        break;
      }
      case autoWah:
      {
        const double lfo = 0.5 - 0.5 * std::cos(2.0 * kPi * mPhase);
        mFilter.process(x, 350.0 * std::pow(2.0, lfo * (0.5 + 3.5 * mDepth)), 1.5 + 6.5 * mTweak, mRate);
        y = mFilter.band * 1.7;
        break;
      }
      case ringMod:
      {
        // Portadora de 30 Hz a 2 kHz (no andamento, pulsa na divisão); o ajuste a faz balançar.
        const double carrierHz = (syncHz > 0.0 ? syncHz : 30.0 * std::pow(2.0, rate * 6.0))
                                 * (1.0 + 0.3 * mTweak * std::sin(2.0 * kPi * mPhaseB));
        mPhaseB = wrap(mPhaseB + 3.0 / mRate);
        y = x * ((1.0 - mDepth) + mDepth * std::sin(2.0 * kPi * mPhase));
        step = carrierHz;
        break;
      }
      case triChorus:
      {
        mLine.push(x);
        double sum = 0.0;
        for (int v = 0; v < 3; ++v)
          sum += mLine.read((0.007 + 0.008 * mTweak + mDepth * 0.003 * std::sin(2.0 * kPi * (mPhase + v / 3.0))) * mRate);
        y = sum / 3.0 * 1.2;
        break;
      }
      case sliceTremolo:
      {
        // Onda quadrada com bordas de 2 ms; o ajuste é quanto tempo fica aberto.
        const double wave = mPhase < 0.25 + 0.5 * mTweak ? 1.0 : 0.0;
        mEnv += (wave - mEnv) * edge;
        y = x * (1.0 - mDepth * (1.0 - mEnv));
        break;
      }
      case phaser12:
      {
        const double lfo = 0.5 - 0.5 * std::cos(2.0 * kPi * mPhase);
        const double f = 100.0 * std::pow(2.0, lfo * (1.0 + 4.0 * mDepth));
        const double t = std::tan(kPi * std::min(f, mRate * 0.45) / mRate);
        const double a = (t - 1.0) / (t + 1.0);
        double v = x + mLast * 0.8 * mTweak;
        for (auto& z : mStages)
        {
          const double out = a * v + z;
          z = v - a * out;
          v = out;
        }
        mLast = v;
        y = 0.5 * (x + v) * 1.15;
        break;
      }
      case thruZero:
      {
        // O lado fixo e o que varre (de 0 a 2x o fixo) se cruzam: passa pelo zero.
        const double lfo = 0.5 - 0.5 * std::cos(2.0 * kPi * mPhase);
        mLine.push(x + mLast * 0.7 * mTweak);
        const double fixed = mLine.read(0.003 * mRate);
        const double swept = mLine.read(0.003 * mRate * (1.0 + mDepth * (2.0 * lfo - 1.0)));
        mLast = swept;
        y = 0.5 * (fixed + swept) * 1.1;
        break;
      }
      case detune:
      {
        // Duas vozes levemente desafinadas (uma acima, uma abaixo).
        const double cents = 4.0 + 26.0 * mDepth;
        const double up = std::pow(2.0, cents / 1200.0);
        y = 0.5 * (mUp.process(x, up) + mDown.process(x, 1.0 / up)) * 1.3;
        step = 0.0;
        break;
      }
      default: break;
    }
    mPhase = wrap(mPhase + step / mRate);
    // Tom só no efeito (o seco passa inteiro).
    mToneLow += (y - mToneLow) * toneCoef;
    // Nos modos de atraso, seco e efeito não somam em fase: compensa o
    // volume que some na mistura (no meio, cerca de 3 dB).
    const double comp = decorrelated ? 1.0 / std::sqrt((1.0 - mMix) * (1.0 - mMix) + mMix * mMix) : 1.0;
    buffer[i] = (x + (mToneLow - x) * mMix) * comp;
  }
}

// ---------------------------------------------------------------- TripleDelay

void TripleDelay::prepare(double sampleRate)
{
  mRate = sampleRate;
  for (auto& l : mLines)
    l.line.prepare(static_cast<int>(3.2 * sampleRate));
  reset();
}

void TripleDelay::reset()
{
  for (auto& l : mLines)
  {
    l.line.clear();
    l.delay = 0.0;
    l.low = l.high = 0.0;
    l.level = 0.0;
  }
}

double TripleDelay::secondsFor(double time)
{
  return 0.02 * std::pow(50.0, std::clamp(time, 0.0, 1.0)); // 20 ms a 1 s
}

void TripleDelay::process(const double* in, double* wet, int n, const std::array<Stage, kStages>& stages)
{
  // Digital: repetições limpas, só um corte suave de agudo e de grave para
  // a cascata não embolar.
  const double lowCoef = onePole(7500.0, mRate);
  const double highCoef = onePole(50.0, mRate);
  std::array<double, kStages> targets{};
  for (int k = 0; k < kStages; ++k)
  {
    const auto& s = stages[static_cast<size_t>(k)];
    targets[static_cast<size_t>(k)] = std::clamp(s.seconds, 0.001, 3.0) * mRate;
    auto& l = mLines[static_cast<size_t>(k)];
    if (l.delay <= 0.0)
      l.delay = targets[static_cast<size_t>(k)];
  }
  for (int i = 0; i < n; ++i)
  {
    // Cada delay recebe o que saiu do anterior (o som seco mais as repetições).
    double x = in[i];
    for (int k = 0; k < kStages; ++k)
    {
      const auto& s = stages[static_cast<size_t>(k)];
      auto& l = mLines[static_cast<size_t>(k)];
      l.repeats = smooth(l.repeats, s.repeats, mRate);
      // Repetição: 10 = na altura do som que entra. Desligado, o delay para de
      // receber, mas o que já está nele continua soando até acabar.
      l.level = smooth(l.level, 0.8 * s.level, mRate);
      l.delay += (targets[static_cast<size_t>(k)] - l.delay) * 0.0006;
      const double echo = l.line.read(std::max(1.0, l.delay));
      l.low += (echo - l.low) * lowCoef;
      l.high += (l.low - l.high) * highCoef;
      const double shaped = l.low - l.high;
      const double fb = 0.92 * l.repeats;
      l.line.push(std::tanh((s.on ? x : 0.0) + shaped * fb));
      x += shaped * l.level;
    }
    wet[i] = x - in[i];
  }
}
} // namespace nexus
