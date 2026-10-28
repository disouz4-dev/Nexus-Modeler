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

// Filtros de corte (Butterworth de 2ª ordem; dois em cascata dão 24 dB/oitava).
IirSection<2> cutFilter(bool highPass, double f, double q, double sr)
{
  const double w = 2.0 * kPi * std::clamp(f, 10.0, sr * 0.45) / sr;
  const double c = std::cos(w), alpha = std::sin(w) / (2.0 * q);
  const double a0 = 1 + alpha;
  IirSection<2> s;
  if (highPass)
    s.b = {(1 + c) / 2 / a0, -(1 + c) / a0, (1 + c) / 2 / a0};
  else
    s.b = {(1 - c) / 2 / a0, (1 - c) / a0, (1 - c) / 2 / a0};
  s.a = {1.0, -2 * c / a0, (1 - alpha) / a0};
  return s;
}

std::complex<double> sectionAt(const IirSection<2>& b, std::complex<double> z)
{
  return (b.b[0] + b.b[1] * z + b.b[2] * z * z) / (1.0 + b.a[1] * z + b.a[2] * z * z);
}

// ---------------------------------------------------------------------------

void GraphicEq5::prepare(double sampleRate)
{
  mSampleRate = sampleRate;
  mAlpha = 1.0 - std::exp(-kChunk / (0.03 * sampleRate));
  mCurrent = mTarget;
  rebuild();
  reset();
}

void GraphicEq5::reset()
{
  for (auto& b : mBands)
    b.clear();
}

void GraphicEq5::rebuild()
{
  const bool flat = std::all_of(mCurrent.begin(), mCurrent.end(), [](double d) { return std::abs(d) < 1e-6; });
  if (flat && !mFlat)
    reset();
  mFlat = flat;
  for (int i = 0; i < kBands; ++i)
  {
    const auto q = peak(kFrequency[i], std::clamp(mCurrent[static_cast<size_t>(i)], -12.0, 12.0), 1.1, mSampleRate);
    mBands[static_cast<size_t>(i)].b = q.b;
    mBands[static_cast<size_t>(i)].a = q.a;
  }
}

void GraphicEq5::process(double* buffer, int numFrames)
{
  for (int start = 0; start < numFrames; start += kChunk)
  {
    const int n = std::min(kChunk, numFrames - start);
    if (mCurrent != mTarget)
    {
      for (int i = 0; i < kBands; ++i)
        mCurrent[static_cast<size_t>(i)] = approach(mCurrent[static_cast<size_t>(i)], mTarget[static_cast<size_t>(i)], mAlpha);
      rebuild();
    }
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

// ---------------------------------------------------------------------------

FreeEq::Sections FreeEq::design(const Band& b, double sr)
{
  Sections out;
  const double f = std::clamp(b.freq, 10.0, sr * 0.45);
  switch (b.type)
  {
    case Type::bell:
      out.s[0] = peak(f, std::clamp(b.gainDb, -30.0, 30.0), std::clamp(b.q, 0.1, 40.0), sr);
      out.count = 1;
      break;
    case Type::lowShelf:
    case Type::highShelf:
      out.s[0] = shelf(b.type == Type::lowShelf, f, std::clamp(b.gainDb, -30.0, 30.0), sr);
      out.count = 1;
      break;
    case Type::notch:
    {
      // Corte estreito e fundo (o "notch"): sino de -30 dB com a largura escolhida.
      out.s[0] = peak(f, -30.0, std::clamp(b.q, 0.5, 40.0), sr);
      out.count = 1;
      break;
    }
    case Type::lowCut:
    case Type::highCut:
    {
      // Butterworth: 1 a 4 seções de 2ª ordem (12 a 48 dB por oitava).
      const int n = std::clamp(b.slope, 1, 4);
      for (int k = 0; k < n; ++k)
      {
        const double q = 1.0 / (2.0 * std::sin(kPi * (2.0 * k + 1.0) / (4.0 * n)));
        out.s[static_cast<size_t>(k)] = cutFilter(b.type == Type::lowCut, f, n == 1 ? std::clamp(b.q, 0.3, 4.0) * 0.7071 : q, sr);
      }
      out.count = n;
      break;
    }
  }
  return out;
}

void FreeEq::prepare(double sampleRate)
{
  mSampleRate = sampleRate;
  mAlpha = 1.0 - std::exp(-kChunk / (0.03 * sampleRate));
  mCurrent = mTarget;
  for (int i = 0; i < kMaxBands; ++i)
    rebuild(i);
  reset();
}

void FreeEq::reset()
{
  for (auto& f : mFilters)
    for (auto& s : f.s)
      s.clear();
}

void FreeEq::rebuild(int i)
{
  auto& f = mFilters[static_cast<size_t>(i)];
  const auto old = f;
  f = design(mCurrent[static_cast<size_t>(i)], mSampleRate);
  // Os estados continuam (sem estalo ao mexer); seção nova começa do zero.
  for (int k = 0; k < 4; ++k)
    f.s[static_cast<size_t>(k)].s = k < old.count ? old.s[static_cast<size_t>(k)].s : decltype(f.s[0].s){};
}

void FreeEq::process(double* buffer, int numFrames)
{
  for (int start = 0; start < numFrames; start += kChunk)
  {
    const int n = std::min(kChunk, numFrames - start);
    for (int i = 0; i < kMaxBands; ++i)
    {
      auto& c = mCurrent[static_cast<size_t>(i)];
      const auto& t = mTarget[static_cast<size_t>(i)];
      if (c == t)
        continue;
      if (c.on != t.on || c.type != t.type || c.slope != t.slope)
      {
        // Ligar, desligar ou trocar o tipo: começa limpo.
        c = t;
        rebuild(i);
        for (auto& s : mFilters[static_cast<size_t>(i)].s)
          s.clear();
        continue;
      }
      c.freq = std::exp(approach(std::log(c.freq), std::log(t.freq), mAlpha));
      c.gainDb = approach(c.gainDb, t.gainDb, mAlpha);
      c.q = std::exp(approach(std::log(c.q), std::log(t.q), mAlpha));
      rebuild(i);
    }
    double* x = buffer + start;
    for (int i = 0; i < kMaxBands; ++i)
    {
      const auto& b = mCurrent[static_cast<size_t>(i)];
      if (!b.on || ((b.type == Type::bell || b.type == Type::lowShelf || b.type == Type::highShelf) && std::abs(b.gainDb) < 1e-4))
        continue;
      auto& f = mFilters[static_cast<size_t>(i)];
      for (int k = 0; k < f.count; ++k)
      {
        auto& sec = f.s[static_cast<size_t>(k)];
        for (int j = 0; j < n; ++j)
          x[j] = sec.tick(x[j]);
      }
    }
  }
}

double FreeEq::responseDb(const Bands& bands, double frequency, double sr, int only)
{
  const std::complex<double> z = std::polar(1.0, -2.0 * kPi * frequency / sr);
  std::complex<double> h = 1.0;
  for (int i = 0; i < kMaxBands; ++i)
  {
    const auto& b = bands[static_cast<size_t>(i)];
    if (!b.on || (only >= 0 && i != only))
      continue;
    const auto f = design(b, sr);
    for (int k = 0; k < f.count; ++k)
      h *= sectionAt(f.s[static_cast<size_t>(k)], z);
  }
  return 20.0 * std::log10(std::abs(h) + 1e-12);
}

double GraphicEq5::responseDb(double frequency) const
{
  const std::complex<double> z = std::polar(1.0, -2.0 * kPi * frequency / mSampleRate);
  std::complex<double> h = 1.0;
  if (!mFlat)
    for (const auto& b : mBands)
      h *= (b.b[0] + b.b[1] * z + b.b[2] * z * z) / (1.0 + b.a[1] * z + b.a[2] * z * z);
  return 20.0 * std::log10(std::abs(h));
}

} // namespace nexus
