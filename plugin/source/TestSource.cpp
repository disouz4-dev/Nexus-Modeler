#include "TestSource.h"

#include "BinaryData.h"

namespace nexus
{
namespace
{
// Gravações embutidas (assets/teste, créditos em TERCEIROS.md).
struct Embedded
{
  const char* data;
  int size;
};
Embedded embedded(TestSource::Style s)
{
  switch (s)
  {
    case TestSource::clean:
      return {BinaryData::limpo_flac, BinaryData::limpo_flacSize};
    case TestSource::blues:
      return {BinaryData::blues_flac, BinaryData::blues_flacSize};
    case TestSource::heavy:
      return {BinaryData::pesado_flac, BinaryData::pesado_flacSize};
    case TestSource::rock:
    case TestSource::mine:
    case TestSource::numStyles:
      break;
  }
  return {BinaryData::rock_flac, BinaryData::rock_flacSize};
}

// Converte para a taxa do programa.
juce::AudioBuffer<float> resample(const juce::AudioBuffer<float>& in, double from, double to)
{
  if (std::abs(from - to) < 1.0)
    return in;
  const double ratio = from / to;
  const int outLen = static_cast<int>(in.getNumSamples() / ratio);
  juce::AudioBuffer<float> out(1, outLen);
  juce::LagrangeInterpolator interp;
  interp.process(ratio, in.getReadPointer(0), out.getWritePointer(0), outLen);
  return out;
}
} // namespace

TestSource::TestSource() = default;
TestSource::~TestSource() = default;

void TestSource::prepare(double sampleRate)
{
  const bool changed = std::abs(sampleRate - mRate) > 1.0 || mClip == nullptr;
  mRate = sampleRate;
  mRecord.setSize(1, static_cast<int>(kRecordSeconds * sampleRate) + 1);
  if (changed)
    load(mStyle.load());
}

std::unique_ptr<TestSource::Clip> TestSource::decode(Style s) const
{
  if (s == mine)
  {
    if (mMine == nullptr)
      return nullptr;
    auto c = std::make_unique<Clip>();
    c->audio = mMine->audio;
    return c;
  }
  const auto e = embedded(s);
  juce::FlacAudioFormat flac;
  std::unique_ptr<juce::AudioFormatReader> reader(
    flac.createReaderFor(new juce::MemoryInputStream(e.data, static_cast<size_t>(e.size), false), true));
  if (reader == nullptr)
    return nullptr;
  juce::AudioBuffer<float> raw(1, static_cast<int>(reader->lengthInSamples));
  reader->read(&raw, 0, raw.getNumSamples(), 0, true, false);
  auto c = std::make_unique<Clip>();
  c->audio = resample(raw, reader->sampleRate, mRate);
  return c;
}

void TestSource::load(Style s)
{
  auto next = decode(s);
  if (next == nullptr)
    return;
  {
    const juce::SpinLock::ScopedLockType lock(mLock);
    mRetired = std::move(mClip);
    mClip = std::move(next);
    mPos = 0;
  }
  mRetired.reset(); // fora do áudio
}

void TestSource::setStyle(Style s)
{
  if (s == mine && mMine == nullptr)
    return;
  mStyle = s;
  load(s);
}

bool TestSource::hasRecording() const
{
  return mMine != nullptr;
}

void TestSource::setPlaying(bool on)
{
  if (on && mRecording.load())
    return;
  {
    const juce::SpinLock::ScopedLockType lock(mLock);
    mPos = 0;
  }
  mPlaying = on;
}

void TestSource::startRecording()
{
  mPlaying = false;
  mRecordDone = false;
  mRecordPos = 0;
  mRecording = true;
}

float TestSource::recordProgress() const
{
  return static_cast<float>(mRecordPos.load()) / static_cast<float>(std::max(1, mRecord.getNumSamples() - 1));
}

void TestSource::poll()
{
  if (!mRecordDone.exchange(false))
    return;
  // Corta o silêncio do começo (o tempo de pegar a guitarra) e guarda.
  const auto* r = mRecord.getReadPointer(0);
  const int len = mRecordPos.load();
  float peak = 0.0f;
  for (int i = 0; i < len; ++i)
    peak = std::max(peak, std::abs(r[i]));
  int first = 0;
  while (first < len && std::abs(r[first]) < peak * 0.05f)
    ++first;
  first = std::max(0, first - static_cast<int>(0.01 * mRate));
  if (len - first < static_cast<int>(mRate)) // menos de 1 s tocado: descarta
    return;
  auto c = std::make_unique<Clip>();
  c->audio.setSize(1, len - first);
  c->audio.copyFrom(0, 0, mRecord, 0, first, len - first);
  // Emenda suave no fim, para repetir sem estalo.
  const int fade = std::min(c->audio.getNumSamples() / 4, static_cast<int>(0.02 * mRate));
  c->audio.applyGainRamp(0, c->audio.getNumSamples() - fade, fade, 1.0f, 0.0f);
  mMine = std::move(c);
  setStyle(mine);
  setPlaying(true);
}

bool TestSource::process(const float* in, float* dst, int n)
{
  if (mRecording.load() && in != nullptr)
  {
    int pos = mRecordPos.load();
    const int room = mRecord.getNumSamples() - pos;
    const int take = std::min(n, room);
    if (take > 0)
    {
      mRecord.copyFrom(0, pos, in, take);
      pos += take;
      mRecordPos = pos;
    }
    if (take < n || pos >= mRecord.getNumSamples())
    {
      mRecording = false;
      mRecordDone = true;
    }
  }
  if (!mPlaying.load())
    return false;
  const juce::SpinLock::ScopedTryLockType lock(mLock);
  if (!lock.isLocked() || mClip == nullptr || mClip->audio.getNumSamples() == 0)
  {
    std::fill(dst, dst + n, 0.0f); // trocando de gravação neste instante
    return true;
  }
  const auto* src = mClip->audio.getReadPointer(0);
  const int len = mClip->audio.getNumSamples();
  for (int i = 0; i < n; ++i)
  {
    dst[i] = src[mPos];
    if (++mPos >= len)
      mPos = 0;
  }
  return true;
}
} // namespace nexus
