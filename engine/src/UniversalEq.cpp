#include "nexus/UniversalEq.h"

#include <algorithm>
#include <cmath>
#include <complex>

namespace nexus
{
namespace
{
constexpr double kPi = 3.14159265358979323846;
// Tamanho do pedaço em que os coeficientes andam atrás do botão.
constexpr int kChunk = 32;

double approach(double current, double target, double alpha)
{
  const double next = current + (target - current) * alpha;
  return std::abs(target - next) < 1e-4 ? target : next;
}

// Filtros do "Audio EQ Cookbook" (R. Bristow-Johnson).
IirSection<2> shelf(bool low, double f, double db, double sr)
{
  const double A = std::pow(10.0, db / 40.0);
  const double w = 2.0 * kPi * f / sr;
  const double c = std::cos(w), s = std::sin(w);
  const double alpha = s / 2.0 * std::sqrt(2.0); // inclinação 1
  const double k = 2.0 * std::sqrt(A) * alpha;
  IirSection<2> q;
  double a0;
  if (low)
  {
    q.b = {A * ((A + 1) - (A - 1) * c + k), 2 * A * ((A - 1) - (A + 1) * c), A * ((A + 1) - (A - 1) * c - k)};
    a0 = (A + 1) + (A - 1) * c + k;
    q.a = {1.0, -2 * ((A - 1) + (A + 1) * c), (A + 1) + (A - 1) * c - k};
  }
  else
  {
    q.b = {A * ((A + 1) + (A - 1) * c + k), -2 * A * ((A - 1) + (A + 1) * c), A * ((A + 1) + (A - 1) * c - k)};
    a0 = (A + 1) - (A - 1) * c + k;
    q.a = {1.0, 2 * ((A - 1) - (A + 1) * c), (A + 1) - (A - 1) * c - k};
  }
  for (auto& v : q.b)
    v /= a0;
  q.a[1] /= a0;
  q.a[2] /= a0;
  return q;
}

IirSection<2> peak(double f, double db, double qf, double sr)
{
  const double A = std::pow(10.0, db / 40.0);
  const double w = 2.0 * kPi * f / sr;
  const double alpha = std::sin(w) / (2.0 * qf);
  const double c = std::cos(w);
  const double a0 = 1 + alpha / A;
  IirSection<2> q;
  q.b = {(1 + alpha * A) / a0, -2 * c / a0, (1 - alpha * A) / a0};
  q.a = {1.0, -2 * c / a0, (1 - alpha / A) / a0};
  return q;
}
} // namespace

double UniversalEq::knobToDb(double knob, int band)
{
  return (std::clamp(knob, 0.0, 10.0) - 5.0) / 5.0 * kMaxDb[band];
}

void UniversalEq::prepare(double sampleRate)
{
  mSampleRate = sampleRate;
  mAlpha = 1.0 - std::exp(-kChunk / (0.03 * sampleRate));
  mCurrent = mTarget;
  rebuild();
  reset();
}

void UniversalEq::reset()
{
  for (auto& b : mBands)
    b.clear();
}

void UniversalEq::rebuild()
{
  const double db[kBands] = {knobToDb(mCurrent.bass, 0), knobToDb(mCurrent.mid, 1), knobToDb(mCurrent.treble, 2),
                             knobToDb(mCurrent.presence, 3)};
  const bool flat = std::all_of(std::begin(db), std::end(db), [](double d) { return std::abs(d) < 1e-6; });
  if (flat && !mFlat)
    reset();
  mFlat = flat;
  // Os estados continuam: só os coeficientes mudam (sem estalo).
  auto keep = [](IirSection<2>& dst, const IirSection<2>& src) {
    dst.b = src.b;
    dst.a = src.a;
  };
  keep(mBands[0], shelf(true, kFrequency[0], db[0], mSampleRate));
  keep(mBands[1], peak(kFrequency[1], db[1], 0.9, mSampleRate));
  keep(mBands[2], shelf(false, kFrequency[2], db[2], mSampleRate));
  keep(mBands[3], peak(kFrequency[3], db[3], 0.8, mSampleRate));
}

void UniversalEq::process(double* buffer, int numFrames)
{
  for (int start = 0; start < numFrames; start += kChunk)
  {
    const int n = std::min(kChunk, numFrames - start);
    if (!(mCurrent == mTarget))
    {
      mCurrent.bass = approach(mCurrent.bass, mTarget.bass, mAlpha);
      mCurrent.mid = approach(mCurrent.mid, mTarget.mid, mAlpha);
      mCurrent.treble = approach(mCurrent.treble, mTarget.treble, mAlpha);
      mCurrent.presence = approach(mCurrent.presence, mTarget.presence, mAlpha);
      rebuild();
    }
    // Tudo em 5: o som da captura passa intacto.
    if (mFlat)
      continue;
    double* x = buffer + start;
    for (int i = 0; i < n; ++i)
    {
      double v = x[i];
      for (auto& b : mBands)
        v = b.tick(v);
      x[i] = v;
    }
  }
}

double UniversalEq::responseDb(double frequency) const
{
  const std::complex<double> z = std::polar(1.0, -2.0 * kPi * frequency / mSampleRate);
  std::complex<double> h = 1.0;
  if (!mFlat)
    for (const auto& b : mBands)
      h *= (b.b[0] + b.b[1] * z + b.b[2] * z * z) / (1.0 + b.a[1] * z + b.a[2] * z * z);
  return 20.0 * std::log10(std::abs(h));
}

} // namespace nexus
