#pragma once

#include <array>
#include <atomic>
#include <memory>

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>

#include "ChainLayout.h"
#include "MidiMap.h"
#include "nexus/AmpVoice.h"
#include "nexus/Effects.h"

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
  static constexpr int kPedals = ChainLayout::kPedals;
  static constexpr int kRigs = ChainLayout::kRigs;
  // Cada cabeçote tem até 3 canais, cada um com pacote e ganho próprios.
  static constexpr int kChannels = 3;
  // Visuais de pedal: 0 a 3 tocam captura (drive); 4 em diante são efeitos
  // do próprio Nexus (chorus, delay, reverb), com os botões "a", "b" e "c".
  static constexpr int kPedalLooks = 7;
  static constexpr int kFirstNativeLook = 4;
  static bool isNativeLook(int look) { return look >= kFirstNativeLook; }

  // Posições dos pacotes: 0 a 11 são os pedais; depois, os canais dos dois
  // cabeçotes (lado A: 12 a 14, lado B: 15 a 17).
  static constexpr int kSlots = kPedals + kRigs * kChannels;
  static constexpr int pedalSlot(int pedal) { return pedal; }
  static constexpr int ampSlot(int rig, int channel) { return kPedals + rig * kChannels + channel; }

  // Nomes dos parâmetros: pedal n -> "p<n>_drive"...; cabeçote A usa o nome
  // simples ("ganho"), o B ganha "_b" ("ganho_b").
  static juce::String pedalParam(int pedal, const char* name) { return "p" + juce::String(pedal + 1) + "_" + name; }
  static juce::String ampParam(int rig, const char* name) { return rig == 0 ? juce::String(name) : juce::String(name) + "_b"; }

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

  // ----- Tela: cadeia
  ChainLayout layout() const { return mLayoutUi; }
  void setLayout(const ChainLayout& layout);
  // Lado que está tocando agora (0 = A, 1 = B).
  int activeRig() const { return mUiActiveRig.load(); }

  // ----- Tela: pacotes
  void loadPedalPack(int pedal, const juce::File& folder) { loadSlot(pedalSlot(pedal), folder); }
  void loadAmpPack(int rig, int channel, const juce::File& folder) { loadSlot(ampSlot(rig, channel), folder); }
  int selectedChannel(int rig) const;
  bool channelLoaded(int rig, int channel) const { return slotLoaded(ampSlot(rig, channel)); }
  // Canal que toca agora no cabeçote (ou o escolhido, se nenhum tocou ainda).
  int audibleSlot(int rig) const;
  int pedalLook(int pedal) const { return juce::roundToInt(mPedals[static_cast<size_t>(pedal)].visual->load()); }
  bool slotLoaded(int slot) const { return mSlotLoaded[static_cast<size_t>(slot)].load(); }
  juce::String statusText(int slot) const;
  juce::String packName(int slot) const;
  int captureCount(int slot) const { return mUi[static_cast<size_t>(slot)].count.load(); }
  float captureWeight(int i, int slot) const { return mUi[static_cast<size_t>(slot)].weights[static_cast<size_t>(i)].load(); }
  float capturePosition(int i, int slot) const { return mUi[static_cast<size_t>(slot)].positions[static_cast<size_t>(i)].load(); }
  float currentPosition(int slot) const { return mUi[static_cast<size_t>(slot)].position.load(); }
  bool packIncludesCab(int rig) const { return mIncludesCab[static_cast<size_t>(audibleSlot(rig))].load(); }
  // Processador: parte de um núcleo usada por bloco (pedal ou cabeçote) no
  // último segundo, de 0 a 1.
  float slotLoad(int slot) const { return mUiLoad[static_cast<size_t>(slot)].load(); }

  // ----- Tela: caixas (IR). Captura full rig tranca a caixa daquele lado.
  void loadIrAsync(int rig, const juce::File& file);
  juce::String irName(int rig) const;
  bool irLoaded(int rig) const { return mCabs[static_cast<size_t>(rig)].loaded.load(); }

  // Medidores de pico (0 a 1) da entrada e da saída.
  float inputPeak() const { return mInPeak.load(); }
  float outputPeak() const { return mOutPeak.load(); }
  void resetPeaks() { mInPeak.store(0.0f); mOutPeak.store(0.0f); }

  juce::AudioProcessorValueTreeState params;
  MidiMap& midi() { return *mMidi; }

private:
  // Troca de pacote sem trava: a tela deposita em pending, o áudio pega no
  // começo do bloco e devolve o antigo em retired, que a tela apaga.
  struct Slot
  {
    std::atomic<NexusEngine*> pending{nullptr};
    std::atomic<NexusEngine*> retired{nullptr};
    NexusEngine* active = nullptr;
    std::atomic<bool> clear{false}; // pedido da tela: tirar o pacote
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

  // Estado de áudio de cada pedal (liga e desliga sem estalo).
  struct PedalState
  {
    float mix = 0.0f;
    float level = 1.0f;
    int warmLeft = 0;
    bool warming = false;
    std::atomic<float>* drive = nullptr;
    std::atomic<float>* tone = nullptr;
    std::atomic<float>* levelParam = nullptr;
    std::atomic<float>* on = nullptr;
    std::atomic<float>* lite = nullptr;
    std::atomic<float>* visual = nullptr;
    std::array<std::atomic<float>*, 3> fx{}; // botões dos efeitos nativos
    nexus::NativePedal native;
  };

  // Estado de áudio de cada cabeçote: troca de canal com aquecimento.
  struct RigState
  {
    int current = -1, next = -1; // posições de pacote
    int warmLeft = 0;
    float fade = 0.0f;
    std::array<std::atomic<float>*, kChannels> gain{};
    std::atomic<float>* channel = nullptr;
    std::atomic<float>* bass = nullptr;
    std::atomic<float>* mid = nullptr;
    std::atomic<float>* treble = nullptr;
    std::atomic<float>* presence = nullptr;
    std::atomic<float>* master = nullptr;
    float masterGain = 1.0f;
  };

  struct Cab
  {
    juce::dsp::Convolution conv{juce::dsp::Convolution::NonUniform{256}};
    juce::LinearSmoothedValue<float> mix;
    juce::File file;
    std::atomic<bool> loaded{false};
    std::atomic<float>* mode = nullptr;
  };

  static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
  void timerCallback() override;
  void loadSlot(int slot, const juce::File& folder);
  // Tira o pacote da posição (preset sem pacote ali).
  void clearSlot(int slot);
  void runEngines(double* buffer, int numFrames);
  void runPedal(int pedal, double* buffer, int n);
  void runRigs(double* buffer, int n);
  void runRig(int rig, double* buffer, int n);
  void runAmp(int rig, double* buffer, int n);
  void runCab(int rig, double* buffer, int n);
  nexus::AmpVoice::Controls ampControls(int rig, int channel) const;
  nexus::AmpVoice::Controls pedalControls(int pedal) const;
  void publish(int slot);
  static int warmupOf(const NexusEngine& e);

  std::array<Slot, kSlots> mSlots;
  std::array<UiMirror, kSlots> mUi;
  std::array<std::atomic<bool>, kSlots> mSlotLoaded{};
  std::array<std::atomic<bool>, kSlots> mIncludesCab{};
  std::array<std::atomic<float>, kSlots> mUiLoad{};
  std::array<double, kSlots> mLoadSeconds{};
  double mLoadWindow = 0.0;

  std::array<PedalState, kPedals> mPedals;
  std::array<RigState, kRigs> mRigs;
  std::array<Cab, kRigs> mCabs;

  // Cadeia: cópia da tela e cópia do áudio, trocadas por ponteiro.
  ChainLayout mLayoutUi;
  std::atomic<ChainLayout*> mLayoutPending{nullptr};
  std::atomic<ChainLayout*> mLayoutRetired{nullptr};
  ChainLayout* mLayoutActive = nullptr;

  // Troca A/B: o lado que entra aquece e cruza em 30 ms.
  int mRigCurrent = 0, mRigNext = -1, mRigWarmLeft = 0;
  float mRigFade = 0.0f;
  std::atomic<int> mUiActiveRig{0};
  std::atomic<float>* mAb = nullptr;

  std::unique_ptr<Resampler> mResampler;
  Resampler::BlockProcessFunc mResampledFunc;
  bool mNeedsResampling = false;
  std::vector<double> mMono, mMonoOut, mDry, mChannelIn, mChannelOut, mRigIn;
  std::vector<float> mCabFloat;

  // Carregamento em segundo plano.
  juce::ThreadPool mLoader{1};
  mutable juce::CriticalSection mStatusLock;

  std::atomic<float>* mLite;
  std::atomic<float>* mOutput;
  std::atomic<float>* mInput;
  float mOutputGain = 1.0f;
  float mInputGain = 1.0f;

  std::atomic<float> mInPeak{0.0f}, mOutPeak{0.0f};
  std::unique_ptr<MidiMap> mMidi;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(NexusProcessor)
};
