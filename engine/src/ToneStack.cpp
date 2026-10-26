#include "nexus/ToneStack.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace nexus
{

namespace
{
constexpr double kPi = 3.14159265358979323846;

// Botão de 0 a 10 -> fração do potenciômetro. Médio e agudo são lineares;
// o grave usa curva logarítmica (potenciômetro de áudio), como no original.
// As pontas ficam longe de 0 e 1 para o circuito nunca degenerar.
double linearTaper(double knob)
{
  return std::clamp(knob / 10.0, 0.001, 0.999);
}

double audioTaper(double knob)
{
  return std::clamp(std::exp((knob / 10.0 - 1.0) * 3.4), 0.001, 0.999);
}

template <int N>
std::array<double, N + 1> polyMul(const std::array<double, N + 1>& p, int degP, const double* q, int degQ)
{
  std::array<double, N + 1> r{};
  for (int i = 0; i <= degP; ++i)
    for (int j = 0; j <= degQ; ++j)
      r[i + j] += p[i] * q[j];
  return r;
}

template <int N>
void normalize(std::array<double, N + 1>& b, std::array<double, N + 1>& a)
{
  const double a0 = a[0];
  for (auto& v : b)
    v /= a0;
  for (auto& v : a)
    v /= a0;
}
} // namespace

FmvCircuit circuitByName(const std::string& name)
{
  if (name == "marshall")
    return {250e3, 1e6, 25e3, 33e3, 470e-12, 22e-9, 22e-9};
  if (name == "fender")
    return {250e3, 250e3, 10e3, 100e3, 250e-12, 100e-9, 47e-9};
  if (name == "alto_ganho")
    return {250e3, 1e6, 25e3, 47e3, 470e-12, 22e-9, 22e-9};
  if (name == "mesa_mark")
    return {250e3, 250e3, 10e3, 100e3, 250e-12, 100e-9, 47e-9};
  throw std::runtime_error("Circuito de tonalidade desconhecido: " + name);
}

// Coeficientes do circuito FMV (D. T. Yeh e J. O. Smith, "Discretization of
// the '59 Fender Bassman Tone Stack", DAFx 2006). Conferidos contra a análise
// nodal do circuito na ferramenta de medida.
PotFractions potFractions(const ToneKnobs& k)
{
  return {linearTaper(k.treble), linearTaper(k.mid), audioTaper(k.bass)};
}

FmvPolynomials fmvPolynomials(const FmvCircuit& c, const ToneKnobs& k)
{
  const auto [t, m, l] = potFractions(k);
  const double R1 = c.R1, R2 = c.R2, R3 = c.R3, R4 = c.R4, C1 = c.C1, C2 = c.C2, C3 = c.C3;
  const double m2 = m * m;

  FmvPolynomials p{};
  p.num[0] = t * C1 * R1 + m * C3 * R3 + l * (C1 * R2 + C2 * R2) + (C1 * R3 + C2 * R3);
  p.num[1] = t * (C1 * C2 * R1 * R4 + C1 * C3 * R1 * R4) - m2 * (C1 * C3 * R3 * R3 + C2 * C3 * R3 * R3)
             + m * (C1 * C3 * R1 * R3 + C1 * C3 * R3 * R3 + C2 * C3 * R3 * R3)
             + l * (C1 * C2 * R1 * R2 + C1 * C2 * R2 * R4 + C1 * C3 * R2 * R4)
             + l * m * (C1 * C3 * R2 * R3 + C2 * C3 * R2 * R3) + (C1 * C2 * R1 * R3 + C1 * C2 * R3 * R4 + C1 * C3 * R3 * R4);
  p.num[2] = l * m * (C1 * C2 * C3 * R1 * R2 * R3 + C1 * C2 * C3 * R2 * R3 * R4)
             - m2 * (C1 * C2 * C3 * R1 * R3 * R3 + C1 * C2 * C3 * R3 * R3 * R4)
             + m * (C1 * C2 * C3 * R1 * R3 * R3 + C1 * C2 * C3 * R3 * R3 * R4) + t * C1 * C2 * C3 * R1 * R3 * R4
             - t * m * C1 * C2 * C3 * R1 * R3 * R4 + t * l * C1 * C2 * C3 * R1 * R2 * R4;

  p.den[0] = 1.0;
  p.den[1] = (C1 * R1 + C1 * R3 + C2 * R3 + C2 * R4 + C3 * R4) + m * C3 * R3 + l * (C1 * R2 + C2 * R2);
  p.den[2] = m * (C1 * C3 * R1 * R3 - C2 * C3 * R3 * R4 + C1 * C3 * R3 * R3 + C2 * C3 * R3 * R3)
             + l * m * (C1 * C3 * R2 * R3 + C2 * C3 * R2 * R3) - m2 * (C1 * C3 * R3 * R3 + C2 * C3 * R3 * R3)
             + l * (C1 * C2 * R2 * R4 + C1 * C2 * R1 * R2 + C1 * C3 * R2 * R4 + C2 * C3 * R2 * R4)
             + (C1 * C2 * R1 * R4 + C1 * C3 * R1 * R4 + C1 * C2 * R3 * R4 + C1 * C2 * R1 * R3 + C1 * C3 * R3 * R4
                + C2 * C3 * R3 * R4);
  p.den[3] = l * m * (C1 * C2 * C3 * R1 * R2 * R3 + C1 * C2 * C3 * R2 * R3 * R4)
             - m2 * (C1 * C2 * C3 * R1 * R3 * R3 + C1 * C2 * C3 * R3 * R3 * R4)
             + m * (C1 * C2 * C3 * R3 * R3 * R4 + C1 * C2 * C3 * R1 * R3 * R3 - C1 * C2 * C3 * R1 * R3 * R4)
             + l * C1 * C2 * C3 * R1 * R2 * R4 + C1 * C2 * C3 * R1 * R3 * R4;
  return p;
}

template <int N>
std::array<double, N + 1> bilinear(const std::array<double, N + 1>& analog, double sampleRate)
{
  const double c = 2.0 * sampleRate;
  const double minus[2] = {1.0, -1.0}; // 1 - z^-1
  const double plus[2] = {1.0, 1.0};   // 1 + z^-1
  std::array<double, N + 1> out{};
  double ck = 1.0;
  for (int k = 0; k <= N; ++k, ck *= c)
  {
    std::array<double, N + 1> term{};
    term[0] = analog[k] * ck;
    int deg = 0;
    for (int i = 0; i < k; ++i, ++deg)
      term = polyMul<N>(term, deg, minus, 1);
    for (int i = 0; i < N - k; ++i, ++deg)
      term = polyMul<N>(term, deg, plus, 1);
    for (int i = 0; i <= N; ++i)
      out[i] += term[i];
  }
  return out;
}

template std::array<double, 3> bilinear<2>(const std::array<double, 3>&, double);
template std::array<double, 4> bilinear<3>(const std::array<double, 4>&, double);

// ---------------------------------------------------------------------------

void RelativeToneStack::prepare(double sampleRate)
{
  mSampleRate = sampleRate;
  rebuild();
  reset();
}

void RelativeToneStack::setCircuit(const FmvCircuit& c)
{
  mCircuit = c;
  mCapturePoly = fmvPolynomials(mCircuit, mCapture);
  rebuild();
}

void RelativeToneStack::setCaptureKnobs(const ToneKnobs& k)
{
  mCapture = k;
  mCapturePoly = fmvPolynomials(mCircuit, mCapture);
  rebuild();
}

void RelativeToneStack::setUserKnobs(const ToneKnobs& k)
{
  if (k == mUser)
    return;
  mUser = k;
  rebuild();
}

void RelativeToneStack::rebuild()
{
  const bool wasIdentity = mIdentity;
  mIdentity = (mUser == mCapture);
  if (mIdentity)
    return;
  if (wasIdentity)
    reset();

  const FmvPolynomials user = fmvPolynomials(mCircuit, mUser);

  // Parte 1: numerador do usuário sobre numerador da captura (grau 2).
  std::array<double, 3> qb = bilinear<2>({user.num[0], user.num[1], user.num[2]}, mSampleRate);
  std::array<double, 3> qa =
    bilinear<2>({mCapturePoly.num[0], mCapturePoly.num[1], mCapturePoly.num[2]}, mSampleRate);
  normalize<2>(qb, qa);
  mQuadratic.b = qb;
  mQuadratic.a = qa;

  // Parte 2: denominador da captura sobre denominador do usuário (grau 3).
  std::array<double, 4> cb = bilinear<3>(mCapturePoly.den, mSampleRate);
  std::array<double, 4> ca = bilinear<3>(user.den, mSampleRate);
  normalize<3>(cb, ca);
  mCubic.b = cb;
  mCubic.a = ca;
}

void RelativeToneStack::process(double* buffer, int numFrames)
{
  if (mIdentity)
    return;
  for (int i = 0; i < numFrames; ++i)
    buffer[i] = mCubic.tick(mQuadratic.tick(buffer[i]));
}

void RelativeToneStack::reset()
{
  mQuadratic.clear();
  mCubic.clear();
}

// ---------------------------------------------------------------------------

namespace
{
// Fórmulas do "Audio EQ Cookbook" (R. Bristow-Johnson).
void designShelf(IirSection<2>& f, bool high, double fs, double freq, double gainDb)
{
  const double A = std::pow(10.0, gainDb / 40.0);
  const double w0 = 2.0 * kPi * freq / fs;
  const double cw = std::cos(w0), sw = std::sin(w0);
  const double alpha = sw / 2.0 * std::sqrt(2.0); // inclinação S = 1
  const double sqA = 2.0 * std::sqrt(A) * alpha;
  std::array<double, 3> b, a;
  if (high)
  {
    b = {A * ((A + 1) + (A - 1) * cw + sqA), -2 * A * ((A - 1) + (A + 1) * cw), A * ((A + 1) + (A - 1) * cw - sqA)};
    a = {(A + 1) - (A - 1) * cw + sqA, 2 * ((A - 1) - (A + 1) * cw), (A + 1) - (A - 1) * cw - sqA};
  }
  else
  {
    b = {A * ((A + 1) - (A - 1) * cw + sqA), 2 * A * ((A - 1) - (A + 1) * cw), A * ((A + 1) - (A - 1) * cw - sqA)};
    a = {(A + 1) + (A - 1) * cw + sqA, -2 * ((A - 1) + (A + 1) * cw), (A + 1) + (A - 1) * cw - sqA};
  }
  normalize<2>(b, a);
  f.b = b;
  f.a = a;
}

void designPeak(IirSection<2>& f, double fs, double freq, double q, double gainDb)
{
  const double A = std::pow(10.0, gainDb / 40.0);
  const double w0 = 2.0 * kPi * freq / fs;
  const double alpha = std::sin(w0) / (2.0 * q);
  const double cw = std::cos(w0);
  std::array<double, 3> b = {1 + alpha * A, -2 * cw, 1 - alpha * A};
  std::array<double, 3> a = {1 + alpha / A, -2 * cw, 1 - alpha / A};
  normalize<2>(b, a);
  f.b = b;
  f.a = a;
}
} // namespace

void RelativeShelf::prepare(double sampleRate, Kind kind, double frequency, double maxBoostDb)
{
  mSampleRate = sampleRate;
  mKind = kind;
  mFrequency = frequency;
  mMaxBoostDb = maxBoostDb;
  mIdentity = true;
  setUserKnob(mUser);
  reset();
}

void RelativeShelf::setUserKnob(double v)
{
  mUser = v;
  const double db = mMaxBoostDb * (mUser - mCapture) / 10.0;
  const bool identity = std::abs(db) < 1e-9;
  if (identity != mIdentity && !identity)
    reset();
  mIdentity = identity;
  if (!mIdentity)
    designShelf(mFilter, mKind == Kind::High, mSampleRate, mFrequency, db);
}

void RelativeShelf::process(double* buffer, int numFrames)
{
  if (mIdentity)
    return;
  for (int i = 0; i < numFrames; ++i)
    buffer[i] = mFilter.tick(buffer[i]);
}

void RelativeGraphicEq::prepare(double sampleRate, const double* frequencies, int numBands)
{
  mSampleRate = sampleRate;
  mNumBands = std::min(numBands, kMaxBands);
  for (int i = 0; i < mNumBands; ++i)
  {
    mFreq[i] = frequencies[i];
    mIdentity[i] = true;
    setUserBand(i, mUserDb[i]);
  }
  reset();
}

void RelativeGraphicEq::setUserBand(int band, double db)
{
  mUserDb[band] = db;
  const double rel = mUserDb[band] - mCaptureDb[band];
  const bool identity = std::abs(rel) < 1e-9;
  if (identity != mIdentity[band] && !identity)
    mBands[band].clear();
  mIdentity[band] = identity;
  if (!identity)
    designPeak(mBands[band], mSampleRate, mFreq[band], 1.0, rel);
}

void RelativeGraphicEq::process(double* buffer, int numFrames)
{
  for (int band = 0; band < mNumBands; ++band)
  {
    if (mIdentity[band])
      continue;
    auto& f = mBands[band];
    for (int i = 0; i < numFrames; ++i)
      buffer[i] = f.tick(buffer[i]);
  }
}

void RelativeGraphicEq::reset()
{
  for (auto& b : mBands)
    b.clear();
}

} // namespace nexus
