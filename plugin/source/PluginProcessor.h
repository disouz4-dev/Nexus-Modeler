#pragma once

#include <array>
#include <atomic>
#include <memory>

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>

#include "MidiMap.h"
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

// Equipamento pronto para tocar: pacote carregado e voz preparada. Montado
// inteiro fora do áudio e entregue ao áudio por troca de ponteiro.
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

  // Blocos que tocam capturas, na ordem do sinal.
  enum Block
  {
    pedal = 0,
    amp = 1,
    kBlocks = 2
  };
  // O cabeçote tem até 3 canais (limpo, crunch, alto ganho), cada um com o
  // próprio pacote de capturas e o próprio ganho.
  static constexpr int kChannels = 3;

  NexusProcessor();
  ~NexusProcessor() override;

  void prepareToPlay(double sampleRate, int samplesPerBlock) override;
  void releaseResources() override {}
  bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
  void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

  juce::AudioProcessorEditor* createEditor() override;
  bool hasEditor() const override { return true; }

  const juce::String getName() const override { return "Nexus Modeler"; }
  bool acceptsMidi() const override { return true; }
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

  // Tela. "b" é o bloco (pedal ou amplificador); no amplificador, "channel"
  // é o canal (0 a 2); -1 é o canal escolhido agora.
  void loadPackAsync(const juce::File& folder, Block b = amp, int channel = -1);
  int selectedChannel() const;
  bool channelLoaded(int channel) const { return mChannelLoaded[static_cast<size_t>(channel)].load(); }
  juce::String channelPackName(int channel) const;
  juce::String statusText(Block b = amp) const;
  juce::String knobName() const;
  juce::String packName(Block b = amp) const;
  int captureCount(Block b = amp) const { return mUi[b].count.load(); }
  float captureWeight(int i, Block b = amp) const { return mUi[b].weights[static_cast<size_t>(i)].load(); }
  float capturePosition(int i, Block b = amp) const { return mUi[b].positions[static_cast<size_t>(i)].load(); }
  float currentPosition(Block b = amp) const { return mUi[b].position.load(); }
  bool packLoaded(Block b = amp) const { return mUi[b].count.load() > 0; }
  bool packIncludesCab() const { return mChannelIncludesCab[static_cast<size_t>(juce::jlimit(0, kChannels - 1, mAudibleChannel.load()))].load(); }

  // Caixa (IR). Com captura full rig a caixa fica trancada e o IR não toca.
  void loadIrAsync(const juce::File& file);
  juce::String irName() const;
  bool irLoaded() const { return mIrLoaded.load(); }

  // Medidores de pico (0 a 1) da entrada e da saída.
  float inputPeak() const { return mInPeak.load(); }
  float outputPeak() const { return mOutPeak.load(); }
  void resetPeaks() { mInPeak.store(0.0f); mOutPeak.store(0.0f); }

  juce::AudioProcessorValueTreeState params;
  MidiMap& midi() { return *mMidi; }

private:
  // Troca de equipamento sem trava: a tela deposita em pending, o áudio pega
  // no começo do bloco e devolve o antigo em retired, que a tela apaga.
  struct Slot
  {
    std::atomic<NexusEngine*> pending{nullptr};
    std::atomic<NexusEngine*> retired{nullptr};
    NexusEngine* active = nullptr;
    juce::File folder;
    bool loadedLite = false;
    juce::String status, name;
  };

  // Espelho para a tela, escrito pelo áudio.
  struct UiMirror
  {
    std::atomic<int> count{0};
    std::array<std::atomic<float>, kMaxCaptures> weights{};
    std::array<std::atomic<float>, kMaxCaptures> positions{};
    std::atomic<float> position{0.0f};
  };

  static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
  void timerCallback() override;
  void runEngines(double* buffer, int numFrames);
  void runPedal(double* buffer, int n);
  void runAmp(double* buffer, int n);
  int slotIndex(Block b, int channel) const;
  nexus::AmpVoice::Controls ampControls(int channel) const;
  nexus::AmpVoice::Controls pedalControls() const;
  void publish(Block b);

  // Posição 0: pedal; 1 a 3: canais do cabeçote.
  static constexpr int kSlots = 1 + kChannels;
  std::array<Slot, kSlots> mSlots;
  // Troca de canal: o canal que entra aquece ouvindo a guitarra e depois
  // entra numa transição curta; os outros canais ficam parados.
  int mAmpCurrent = -1, mAmpNext = -1;
  int mAmpWarmLeft = 0;
  float mAmpFade = 0.0f;
  std::vector<double> mChannelIn, mChannelOut;
  std::array<std::atomic<bool>, kChannels> mChannelLoaded{};
  std::array<std::atomic<bool>, kChannels> mChannelIncludesCab{};
  std::atomic<int> mAudibleChannel{0};
  std::array<UiMirror, kBlocks> mUi;

  std::unique_ptr<Resampler> mResampler;
  Resampler::BlockProcessFunc mResampledFunc;
  bool mNeedsResampling = false;
  std::vector<double> mMono, mMonoOut, mDry;

  // Carregamento em segundo plano.
  juce::ThreadPool mLoader{1};
  mutable juce::CriticalSection mStatusLock;
  juce::String mKnobName = "Ganho";

  std::atomic<float>* mMain;
  std::array<std::atomic<float>*, kChannels> mChannelGain{};
  std::atomic<float>* mChannel;
  std::atomic<float>* mBass;
  std::atomic<float>* mMid;
  std::atomic<float>* mTreble;
  std::atomic<float>* mPresence;
  std::atomic<float>* mStepped;
  std::atomic<float>* mLite;
  std::atomic<float>* mOutput;
  std::atomic<float>* mInput;
  std::atomic<float>* mMaster;
  std::atomic<float>* mCabMode;
  std::atomic<float>* mPedalGain;
  std::atomic<float>* mPedalTone;
  std::atomic<float>* mPedalLevel;
  std::atomic<float>* mPedalOn;
  float mOutputGain = 1.0f;
  float mInputGain = 1.0f;

  // Pedal: liga e desliga com transição curta; o nível é o volume dele.
  float mPedalMix = 0.0f;
  float mPedalLevelGain = 1.0f;

  juce::dsp::Convolution mCab{juce::dsp::Convolution::NonUniform{256}};
  juce::AudioBuffer<float> mCabBuffer;
  juce::LinearSmoothedValue<float> mCabMix;
  juce::File mIrFile;
  std::atomic<bool> mIrLoaded{false};

  std::atomic<float> mInPeak{0.0f}, mOutPeak{0.0f};
  std::unique_ptr<MidiMap> mMidi;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(NexusProcessor)
};
