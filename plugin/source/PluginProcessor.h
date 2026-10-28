#pragma once

#include <array>
#include <atomic>
#include <memory>

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>

#include "ChainLayout.h"
#include "fx/PitchDrop.h"
#include "MidiMap.h"
#include "TestSource.h"
#include "studio/Player.h"
#include "nexus/UniversalEq.h"
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
  int block = 0; // maior pedaço para o qual as capturas foram preparadas
  // Acabou de entrar na cadeia: primeiro aquece ouvindo a guitarra (sem
  // tocar), depois entra suave. Sem isso, o pacote novo estala ao entrar.
  int settleLeft = -1;
  float settleGain = 1.0f;
};

class NexusProcessor : public juce::AudioProcessor, private juce::Timer, private juce::AudioProcessorParameter::Listener
{
public:
  // Taxa em que as capturas A2 são treinadas e rodam.
  static constexpr double kEngineRate = 48000.0;
  // Maior pedaço entregue às capturas de uma vez.
  // Maior pedaço de áudio que as capturas aceitam. Elas são preparadas no
  // tamanho do bloco da placa (mEngineBlock): preparar para 2048 e tocar
  // blocos de 128 deixa uma captura A2 duas vezes mais lenta (medido).
  static constexpr int kEngineBlock = 2048;
  static constexpr int kMaxCaptures = 64;
  static constexpr int kPedals = ChainLayout::kPedals;
  static constexpr int kRigs = ChainLayout::kRigs;
  // Cada cabeçote tem até 3 canais, cada um com pacote e ganho próprios.
  static constexpr int kChannels = 3;
  // Visuais de pedal, na ordem do parâmetro "visual" (os presets guardam o
  // número: os novos entram sempre no fim). Cada um toca captura (drive) ou
  // um efeito do próprio Nexus, com os botões "a", "b" e "c".
  enum class PedalKind
  {
    capture,
    chorus,
    delay,
    reverb,
    gate,
    drop,
    phaser,
    flanger,
    tremolo,
    vibe
  };
  static constexpr int kPedalLooks = 19;
  static constexpr PedalKind kPedalKinds[kPedalLooks] = {
    PedalKind::capture, PedalKind::capture, PedalKind::capture, PedalKind::capture, // Lemon, Orange, Banana, Gold
    PedalKind::chorus,  PedalKind::delay,   PedalKind::reverb,  PedalKind::gate,    // Blueberry, Coconut, Grape, Pear
    PedalKind::drop,                                                                 // Cherry Drop
    PedalKind::phaser,  PedalKind::flanger, PedalKind::tremolo, PedalKind::vibe,    // Peach, Lime, Melon, Tamarind
    PedalKind::capture, PedalKind::capture, PedalKind::capture, PedalKind::capture, // Plum, Fig, Grapefruit, Mango
    PedalKind::capture, PedalKind::capture};                                         // Guava, Apricot
  static constexpr int kDropLook = 8; // Cherry Drop (nativo, motor fx::PitchDrop)
  static constexpr int kGateLook = 7;
  static PedalKind pedalKind(int look) { return look >= 0 && look < kPedalLooks ? kPedalKinds[look] : PedalKind::capture; }
  // Cabeçotes: o visual (e as capturas recomendadas) de cada lado.
  static constexpr int kAmpLooks = 19;
  static constexpr int kMarkFive = 1;
  // Faixas do equalizador gráfico (cabeçotes que têm): parâmetros "eq80"...
  static constexpr const char* kGraphicIds[5] = {"eq80", "eq240", "eq750", "eq2200", "eq6600"};
  // Parâmetro de uma faixa do equalizador do fim: "eq3_freq"...
  static juce::String eqParam(int band, const char* name) { return "eq" + juce::String(band + 1) + "_" + name; }
  static bool isNativeLook(int look) { return pedalKind(look) != PedalKind::capture; }

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
  void clearAmpPack(int rig, int channel) { clearSlot(ampSlot(rig, channel)); }
  // Pasta do pacote aberto no espaço (vazia se não há pacote).
  juce::File slotFolder(int slot) const { return mSlots[static_cast<size_t>(slot)].folder; }
  int selectedChannel(int rig) const;
  bool channelLoaded(int rig, int channel) const { return slotLoaded(ampSlot(rig, channel)); }
  // Canal que toca agora no cabeçote (ou o escolhido, se nenhum tocou ainda).
  int audibleSlot(int rig) const;
  int pedalLook(int pedal) const { return juce::roundToInt(mPedals[static_cast<size_t>(pedal)].visual->load()); }
  int ampLook(int rig) const { return juce::roundToInt(mRigs[static_cast<size_t>(rig)].look->load()); }
  static constexpr int kCabLooks = 13;
  int cabLook(int rig) const { return juce::roundToInt(params.getRawParameterValue(ampParam(rig, "gabinete"))->load()); }
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
  juce::File irFile(int rig) const { return mCabs[static_cast<size_t>(rig)].file; }
  bool irLoaded(int rig) const { return mCabs[static_cast<size_t>(rig)].loaded.load(); }

  // Medidores de pico (0 a 1) da entrada e da saída.
  float inputPeak() const { return mInPeak.load(); }
  // "Testar o som": gravação de guitarra no lugar da entrada.
  nexus::TestSource& testSource() { return mTest; }
  // Estúdio: a música separada toca junto com a guitarra.
  nexus::studio::Player& studio() { return mStudio; }
  // Separação em segundo plano: continua mesmo com a tela fechada.
  nexus::studio::Separation& separation() { return mSeparation; }
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
    std::unique_ptr<nexus::fx::PitchDrop> drop; // só o Cherry Drop usa
    float dropMix = 0.0f;
    float dropSemitones = 0.0f;
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
    std::atomic<float>* look = nullptr;
    std::array<std::atomic<float>*, 5> graphic{};
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
  void runDrop(int pedal, double* buffer, int n);
  void runRigs(double* buffer, int n);
  void runRig(int rig, double* buffer, int n);
  void runAmp(int rig, double* buffer, int n);
  void runCab(int rig, double* buffer, int n);
  nexus::AmpVoice::Controls ampControls(int rig, int channel) const;
  nexus::AmpVoice::Controls pedalControls(int pedal) const;
  void publish(int slot);
  static int warmupOf(const NexusEngine& e);
  // Pacote recém-entrado: troca a saída por "dry" enquanto aquece e cruza.
  static void settle(NexusEngine& e, const double* dry, double* out, int n);

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
  std::vector<float> mDropFloat; // pedal de drop (o motor trabalha em float)

  // Carregamento em segundo plano.
  juce::ThreadPool mLoader{1};
  mutable juce::CriticalSection mStatusLock;

  std::atomic<float>* mLite;
  std::atomic<float>* mOutput;
  std::atomic<float>* mInput;
  std::atomic<float>* mInputMute;
  float mOutputGain = 1.0f;
  float mInputGain = 1.0f;

  std::atomic<float> mInPeak{0.0f}, mOutPeak{0.0f};
  nexus::TestSource mTest;
  nexus::studio::Player mStudio;
  nexus::studio::Separation mSeparation;
  std::atomic<int> mEngineBlock{256};
  // Grave, médio, agudo e presença: equalizador universal depois da caixa.
  std::array<nexus::UniversalEq, kRigs> mEq;
  std::array<nexus::GraphicEq5, kRigs> mGraphicEq;
  // Equalizador do fim da cadeia (estilo Pro-Q, fixo depois da caixa).
  nexus::FreeEq mFreeEq;
  void runEndEqs(double* buffer, int n);
  struct EqBandParams
  {
    std::atomic<float>*on = nullptr, *type = nullptr, *freq = nullptr, *gain = nullptr, *q = nullptr, *slope = nullptr;
  };
  std::atomic<float>* mEqOn = nullptr;
  std::array<EqBandParams, nexus::FreeEq::kMaxBands> mEqBands;
  // Mão no botão que troca captura (drive de um pedal, ganho de um
  // cabeçote): aquece as vizinhas só daquele equipamento, até um pouco
  // depois de soltar.
  void parameterValueChanged(int, float) override {}
  void parameterGestureChanged(int index, bool starting) override;
  struct Anticipation
  {
    std::atomic<int> gestures{0};
    std::atomic<juce::uint32> until{0};
  };
  std::array<Anticipation, kSlots> mAnticipation;
  std::vector<float> mTestBuffer;
  std::unique_ptr<MidiMap> mMidi;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(NexusProcessor)
};
