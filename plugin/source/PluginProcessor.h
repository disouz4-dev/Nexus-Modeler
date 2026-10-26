#pragma once

#include <array>
#include <atomic>
#include <memory>

#include <juce_audio_processors/juce_audio_processors.h>

#include "nexus/AmpVoice.h"

// O conversor de taxa veio do iPlug2 e espera estas duas definições.
#ifndef DEFAULT_BLOCK_SIZE
  #define DEFAULT_BLOCK_SIZE 1024
#endif
namespace iplug
{
constexpr double PI = 3.14159265358979323846;
}
#include "ResamplingContainer.h"

using Resampler = dsp::ResamplingContainer<double, 1, 12>;

// Equipamento pronto para tocar: pacote carregado e amplificador preparado.
// Montado inteiro fora do áudio e entregue ao áudio por troca de ponteiro.
struct NexusEngine
{
  std::shared_ptr<nexus::Pack> pack;
  nexus::AmpVoice voice;
};

class NexusProcessor : public juce::AudioProcessor, private juce::Timer
{
public:
  // Taxa em que as capturas A2 são treinadas e rodam.
  static constexpr double kEngineRate = 48000.0;
  // Maior pedaço entregue às capturas de uma vez.
  static constexpr int kEngineBlock = 2048;
  static constexpr int kMaxCaptures = 64;

  NexusProcessor();
  ~NexusProcessor() override;

  void prepareToPlay(double sampleRate, int samplesPerBlock) override;
  void releaseResources() override {}
  bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
  void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

  juce::AudioProcessorEditor* createEditor() override;
  bool hasEditor() const override { return true; }

  const juce::String getName() const override { return "Nexus Modeler"; }
  bool acceptsMidi() const override { return false; }
  bool producesMidi() const override { return false; }
  bool isMidiEffect() const override { return false; }
  double getTailLengthSeconds() const override { return 0.0; }

  int getNumPrograms() override { return 1; }
  int getCurrentProgram() override { return 0; }
  void setCurrentProgram(int) override {}
  const juce::String getProgramName(int) override { return {}; }
  void changeProgramName(int, const juce::String&) override {}

  void getStateInformation(juce::MemoryBlock& destData) override;
  void setStateInformation(const void* data, int sizeInBytes) override;

  // Tela.
  void loadPackAsync(const juce::File& folder);
  juce::String statusText() const;
  juce::String knobName() const;
  int captureCount() const { return mUiCaptureCount.load(); }
  float captureWeight(int i) const { return mUiWeights[static_cast<size_t>(i)].load(); }
  float capturePosition(int i) const { return mUiPositions[static_cast<size_t>(i)].load(); }
  float currentPosition() const { return mUiPosition.load(); }

  juce::AudioProcessorValueTreeState params;

private:
  static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
  void timerCallback() override;
  void installEngine(std::unique_ptr<NexusEngine> engine);
  void runEngine(double* buffer, int numFrames);
  nexus::AmpVoice::Controls readControls() const;

  // Troca de equipamento sem trava: a tela deposita em mPending, o áudio pega
  // no começo do bloco e devolve o antigo em mRetired, que a tela apaga.
  std::atomic<NexusEngine*> mPending{nullptr};
  std::atomic<NexusEngine*> mRetired{nullptr};
  NexusEngine* mActive = nullptr;

  std::unique_ptr<Resampler> mResampler;
  Resampler::BlockProcessFunc mResampledFunc;
  bool mNeedsResampling = false;
  std::vector<double> mMono, mMonoOut;

  // Carregamento em segundo plano.
  juce::ThreadPool mLoader{1};
  juce::File mPackFolder;
  bool mLoadedLite = false;
  mutable juce::CriticalSection mStatusLock;
  juce::String mStatus = "Nenhum pacote carregado. Clique em \"Abrir pacote\".";
  juce::String mKnobName = "Ganho";

  // Espelho para a tela, escrito pelo áudio.
  std::atomic<int> mUiCaptureCount{0};
  std::array<std::atomic<float>, kMaxCaptures> mUiWeights{};
  std::array<std::atomic<float>, kMaxCaptures> mUiPositions{};
  std::atomic<float> mUiPosition{0.0f};

  std::atomic<float>* mMain;
  std::atomic<float>* mBass;
  std::atomic<float>* mMid;
  std::atomic<float>* mTreble;
  std::atomic<float>* mPresence;
  std::atomic<float>* mStepped;
  std::atomic<float>* mLite;
  std::atomic<float>* mOutput;
  float mOutputGain = 1.0f;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(NexusProcessor)
};
