#include "PitchDrop.h"

#include <algorithm>
#include <vector>

#include <signalsmith-stretch/signalsmith-stretch.h>

namespace nexus::fx
{
struct PitchDrop::Impl
{
  signalsmith::stretch::SignalsmithStretch<float> stretch;
  std::vector<float> in, out;
  double sampleRate = 48000.0;
  float semitones = 0.0f;
};

PitchDrop::PitchDrop() : mImpl(std::make_unique<Impl>()) {}
PitchDrop::~PitchDrop() = default;

void PitchDrop::prepare(double sampleRate, int maxBlock, int window)
{
  auto& m = *mImpl;
  m.sampleRate = sampleRate;
  m.stretch.configure(1, window, window / 4, false);
  m.in.assign(static_cast<size_t>(maxBlock), 0.0f);
  m.out.assign(static_cast<size_t>(maxBlock), 0.0f);
  setSemitones(m.semitones);
}

void PitchDrop::reset()
{
  mImpl->stretch.reset();
}

void PitchDrop::setSemitones(float semitones)
{
  auto& m = *mImpl;
  m.semitones = semitones;
  // Limite de tonalidade: acima de 8 kHz o som é mais ruído que nota.
  m.stretch.setTransposeSemitones(semitones, static_cast<float>(8000.0 / m.sampleRate));
}

void PitchDrop::process(float* buffer, int numFrames)
{
  auto& m = *mImpl;
  for (int start = 0; start < numFrames;)
  {
    const int n = std::min(numFrames - start, static_cast<int>(m.in.size()));
    if (n <= 0)
      return;
    std::copy(buffer + start, buffer + start + n, m.in.begin());
    float* ip[1] = {m.in.data()};
    float* op[1] = {m.out.data()};
    m.stretch.process(ip, n, op, n);
    std::copy(m.out.begin(), m.out.begin() + n, buffer + start);
    start += n;
  }
}

int PitchDrop::latencySamples() const
{
  return mImpl->stretch.inputLatency() + mImpl->stretch.outputLatency();
}
} // namespace nexus::fx
