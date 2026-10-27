#include "Player.h"

#include <signalsmith-stretch/signalsmith-stretch.h>

namespace nexus::studio
{
namespace
{
// Estoque lido à frente de cada faixa: 4 s.
constexpr int kAhead = 4 * kSeparationRate;
// Rampa dos volumes, em amostras, para mexer sem estalo.
constexpr float kGainStep = 1.0f / 2048.0f;
} // namespace

struct Player::Source
{
  std::array<std::unique_ptr<juce::BufferingAudioReader>, kStems> readers;
  juce::int64 frames = 0;
};

Player::Player()
{
  mFormats.registerFormat(new juce::FlacAudioFormat(), true);
  mReader.startThread(juce::Thread::Priority::normal);
  for (auto& g : mTargetGain)
    g = 1.0f;
  for (auto& p : mPeak)
    p = 0.0f;
}

Player::~Player()
{
  {
    const juce::SpinLock::ScopedLockType lock(mLock);
    mRetired = std::move(mSource);
  }
  mRetired.reset();
  mReader.stopThread(2000);
}

void Player::prepare(double hostRate, int maxBlock)
{
  const juce::SpinLock::ScopedLockType lock(mLock);
  mHostRate = hostRate;
  mMaxBlock = std::max(32, maxBlock);
  // Pior caso: velocidade 100% e placa a 22 kHz lendo de 44,1 kHz.
  const int maxIn = mMaxBlock * 2 + 64;
  mRead.setSize(2, maxIn);
  for (auto* v : {&mMixL, &mMixR})
    v->assign(static_cast<size_t>(maxIn), 0.0f);
  for (auto* v : {&mOutL, &mOutR})
    v->assign(static_cast<size_t>(mMaxBlock), 0.0f);
  mStretch = std::make_unique<Stretch>();
  mStretch->presetDefault(2, static_cast<float>(kSeparationRate));
  mAppliedSemitones = 0x7fff;
  mFraction = 0.0;
}

bool Player::open(const Song& song)
{
  auto next = std::make_unique<Source>();
  for (int s = 0; s < kStems; ++s)
  {
    auto* reader = mFormats.createReaderFor(song.stem(s));
    if (reader == nullptr)
      return false;
    next->frames = std::max(next->frames, reader->lengthInSamples);
    auto buffered = std::make_unique<juce::BufferingAudioReader>(reader, mReader, kAhead);
    buffered->setReadTimeout(0);
    next->readers[static_cast<size_t>(s)] = std::move(buffered);
  }
  mPlaying = false;
  {
    const juce::SpinLock::ScopedLockType lock(mLock);
    mRetired = std::move(mSource);
    mSource = std::move(next);
    mPos = 0;
    mSeekTo = 0;
    mSongId = song.id();
  }
  mRetired.reset();
  mLength = static_cast<double>(mSource->frames) / kSeparationRate;
  mOpen = true;
  setMix(song.mix);
  return true;
}

void Player::close()
{
  mPlaying = false;
  mOpen = false;
  {
    const juce::SpinLock::ScopedLockType lock(mLock);
    mRetired = std::move(mSource);
    mSongId = {};
  }
  mRetired.reset();
  mLength = 0.0;
}

juce::String Player::songId() const
{
  const juce::SpinLock::ScopedLockType lock(mLock);
  return mSongId;
}

void Player::setPlaying(bool on)
{
  mPlaying = on && mOpen.load();
}

void Player::seek(double seconds)
{
  mSeekTo = static_cast<juce::int64>(juce::jlimit(0.0, mLength.load(), seconds) * kSeparationRate);
}

double Player::position() const
{
  const auto pending = mSeekTo.load();
  return static_cast<double>(pending >= 0 ? pending : mPos.load()) / kSeparationRate;
}

void Player::setMix(const Mix& mix)
{
  bool anySolo = false;
  for (auto s : mix.solo)
    anySolo = anySolo || s;
  for (int s = 0; s < kStems; ++s)
  {
    const auto i = static_cast<size_t>(s);
    const bool audible = !mix.mute[i] && (!anySolo || mix.solo[i]);
    mTargetGain[i] = audible ? mix.volume[i] : 0.0f;
  }
  mSpeed = juce::jlimit(0.5, 1.0, mix.speed);
  mSemitones = juce::jlimit(-6, 6, mix.semitones);
  const auto a = static_cast<juce::int64>(mix.loopStart * kSeparationRate);
  const auto b = static_cast<juce::int64>(mix.loopEnd * kSeparationRate);
  mLoopStart = std::min(a, b);
  mLoopEnd = std::max(a, b);
  mLoop = mix.loop && std::abs(b - a) > kSeparationRate / 4;
}

float Player::stemLevel(int stem)
{
  return mPeak[static_cast<size_t>(stem)].exchange(0.0f);
}

void Player::process(float* left, float* right, int n)
{
  const juce::SpinLock::ScopedTryLockType lock(mLock);
  if (!lock.isLocked() || mSource == nullptr || mStretch == nullptr || n > mMaxBlock)
    return;

  // Pular: recomeça o processador de tempo para não arrastar o som antigo.
  if (const auto to = mSeekTo.exchange(-1); to >= 0)
  {
    mPos = to;
    mStretch->reset();
    mFraction = 0.0;
  }
  if (!mPlaying.load())
    return;

  const int semis = mSemitones.load();
  if (semis != mAppliedSemitones)
  {
    // A faixa está em 44,1 kHz e sai na taxa da placa: corrige a diferença.
    mStretch->setTransposeFactor(static_cast<float>(std::pow(2.0, semis / 12.0) * kSeparationRate / mHostRate));
    mAppliedSemitones = semis;
  }

  // Quantas amostras da música entram neste bloco.
  const double want = n * mSpeed.load() * kSeparationRate / mHostRate + mFraction;
  const int in = std::min(static_cast<int>(want), mRead.getNumSamples());
  mFraction = want - in;

  juce::int64 pos = mPos.load();
  const auto frames = mSource->frames;
  const bool loop = mLoop.load();
  const auto loopStart = mLoopStart.load(), loopEnd = std::min(mLoopEnd.load(), frames);

  std::fill(mMixL.begin(), mMixL.begin() + in, 0.0f);
  std::fill(mMixR.begin(), mMixR.begin() + in, 0.0f);
  // Lê em até dois pedaços (o segundo quando o trecho A–B volta ao começo).
  int done = 0;
  bool ended = false;
  const bool looping = loop && loopEnd > loopStart;
  while (done < in)
  {
    // Chegou (ou passou) do B: volta para o A.
    if (looping && pos >= loopEnd)
      pos = loopStart;
    const juce::int64 limit = looping ? loopEnd : frames;
    const int take = static_cast<int>(std::min<juce::int64>(in - done, limit - pos));
    if (take <= 0)
    {
      ended = true;
      break;
    }
    for (int s = 0; s < kStems; ++s)
    {
      const auto i = static_cast<size_t>(s);
      mSource->readers[i]->read(&mRead, 0, take, pos, true, true);
      const float* l = mRead.getReadPointer(0);
      const float* r = mRead.getReadPointer(1);
      float g = mGain[i];
      const float target = mTargetGain[i].load();
      float peak = 0.0f;
      for (int k = 0; k < take; ++k)
      {
        g += juce::jlimit(-kGainStep, kGainStep, target - g);
        mMixL[static_cast<size_t>(done + k)] += l[k] * g;
        mMixR[static_cast<size_t>(done + k)] += r[k] * g;
        peak = std::max(peak, std::abs(l[k]) * g);
      }
      mGain[i] = g;
      if (peak > mPeak[i].load())
        mPeak[i] = peak;
    }
    done += take;
    pos += take;
  }
  mPos = pos;

  float* inputs[2] = {mMixL.data(), mMixR.data()};
  float* outputs[2] = {mOutL.data(), mOutR.data()};
  mStretch->process(inputs, in, outputs, n);

  const float volume = mVolume.load();
  const float step = (volume - mAppliedVolume) / static_cast<float>(n);
  float v = mAppliedVolume;
  for (int i = 0; i < n; ++i)
  {
    v += step;
    left[i] += mOutL[static_cast<size_t>(i)] * v;
    if (right != nullptr)
      right[i] += mOutR[static_cast<size_t>(i)] * v;
  }
  mAppliedVolume = volume;

  if (ended)
  {
    mPlaying = false;
    mFinished = true;
    mSeekTo = 0;
  }
}
} // namespace nexus::studio
