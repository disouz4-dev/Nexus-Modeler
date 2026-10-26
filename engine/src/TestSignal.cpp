#include "nexus/TestSignal.h"

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace nexus
{

namespace
{
// Gerador simples e reprodutível em qualquer sistema (std::rand não é).
struct Lcg
{
  uint32_t state;
  double next()
  {
    state = state * 1664525u + 1013904223u;
    return (state >> 8) * (1.0 / 16777216.0) * 2.0 - 1.0;
  }
};
} // namespace

std::vector<double> makeGuitarLikeSignal(double sampleRate, double seconds, unsigned seed)
{
  const int total = static_cast<int>(seconds * sampleRate);
  std::vector<double> out(total, 0.0);
  Lcg rng{seed};

  // Riff de quintas e notas soltas em Mi, uma nota a cada 250 ms.
  const double notes[] = {82.41, 123.47, 82.41, 110.0, 146.83, 82.41, 98.0, 123.47};
  const int step = static_cast<int>(0.25 * sampleRate);
  int noteIndex = 0;
  for (int start = 0; start < total; start += step, ++noteIndex)
  {
    const double f = notes[noteIndex % 8];
    const int period = std::max(2, static_cast<int>(sampleRate / f));
    // Excitação com o agudo amaciado, como uma palheta numa corda real.
    std::vector<double> line(period);
    double smooth = 0.0;
    for (auto& v : line)
    {
      smooth += 0.35 * (rng.next() - smooth);
      v = smooth;
    }
    const int length = std::min(total - start, static_cast<int>(1.5 * sampleRate));
    int pos = 0;
    for (int i = 0; i < length; ++i)
    {
      const int nextPos = (pos + 1) % period;
      const double v = line[pos];
      line[pos] = 0.4985 * (v + line[nextPos]);
      out[start + i] += v;
      pos = nextPos;
    }
  }

  double peak = 0.0;
  for (double v : out)
    peak = std::max(peak, std::abs(v));
  const double scale = peak > 0.0 ? 0.5 / peak : 1.0;
  for (auto& v : out)
    v *= scale;
  return out;
}

double rmsDb(const double* x, int n)
{
  double sum = 0.0;
  for (int i = 0; i < n; ++i)
    sum += x[i] * x[i];
  const double rms = std::sqrt(sum / std::max(1, n));
  return 20.0 * std::log10(std::max(rms, 1e-12));
}

} // namespace nexus
