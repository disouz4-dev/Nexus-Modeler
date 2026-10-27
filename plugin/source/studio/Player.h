#pragma once

#include <juce_audio_formats/juce_audio_formats.h>

#include <array>
#include <atomic>
#include <memory>
#include <vector>

#include "Library.h"

namespace signalsmith::stretch
{
template <typename Sample, class RandomEngine>
struct SignalsmithStretch;
}

// Player do estúdio: toca as seis faixas de uma música separada, cada uma
// com volume, mudo e solo, mais velocidade (50 a 100%, sem mudar o tom), tom
// (-6 a +6 semitons, sem mudar a velocidade) e repetição de um trecho (A–B).
//
// As faixas são lidas do disco aos poucos (uma linha de leitura enche um
// estoque à frente); o áudio só lê o que já está pronto. A mudança de
// velocidade e de tom é do Signalsmith Stretch (licença MIT).
namespace nexus::studio
{
class Player
{
public:
  Player();
  ~Player();

  // Fora do áudio.
  void prepare(double hostRate, int maxBlock);
  bool open(const Song& song);
  void close();
  bool isOpen() const { return mOpen.load(); }
  juce::String songId() const;

  void setPlaying(bool on);
  bool playing() const { return mPlaying.load(); }
  void seek(double seconds);
  double position() const; // segundos na música
  double length() const { return mLength.load(); }
  // A música chegou ao fim desde a última consulta (para a playlist).
  bool takeFinished() { return mFinished.exchange(false); }

  void setMix(const Mix& mix);
  void setVolume(float gain) { mVolume = gain; }
  float stemLevel(int stem); // pico desde a última leitura, 0 a 1

  // No áudio: soma a música em "left" e "right".
  void process(float* left, float* right, int n);

private:
  struct Source;
  using Stretch = signalsmith::stretch::SignalsmithStretch<float, void>;

  juce::TimeSliceThread mReader{"Nexus: estúdio"};
  juce::AudioFormatManager mFormats;
  juce::SpinLock mLock;
  std::unique_ptr<Source> mSource, mRetired;
  std::unique_ptr<Stretch> mStretch;

  double mHostRate = 48000.0;
  int mMaxBlock = 512;
  std::atomic<bool> mOpen{false}, mPlaying{false}, mFinished{false};
  std::atomic<double> mLength{0.0};
  std::atomic<juce::int64> mPos{0};       // amostra (44,1 kHz) lida
  std::atomic<juce::int64> mSeekTo{-1};   // pedido de pular
  double mFraction = 0.0;
  std::array<std::atomic<float>, kStems> mTargetGain;
  std::array<float, kStems> mGain{};
  std::array<std::atomic<float>, kStems> mPeak;
  std::atomic<double> mSpeed{1.0};
  std::atomic<int> mSemitones{0};
  int mAppliedSemitones = 0x7fff;
  std::atomic<juce::int64> mLoopStart{0}, mLoopEnd{0};
  std::atomic<bool> mLoop{false};
  std::atomic<float> mVolume{1.0f};
  float mAppliedVolume = 1.0f;

  juce::AudioBuffer<float> mRead;           // uma faixa lida
  std::vector<float> mMixL, mMixR;          // soma das faixas
  std::vector<float> mOutL, mOutR;          // depois da velocidade e do tom
  juce::String mSongId;
};
} // namespace nexus::studio
