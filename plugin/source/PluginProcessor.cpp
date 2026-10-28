#include "PluginProcessor.h"

#include "PluginEditor.h"
#include "nexus/Pack.h"
#include "report/Report.h"

namespace
{
// Pedal e lados A/B entram e saem em transições curtas, sem estalo.
constexpr double kPedalFadeSeconds = 0.02;
constexpr double kSwitchFadeSeconds = 0.03;

juce::String commaDecimal(double v)
{
  return juce::String(v, 1).replaceCharacter('.', ',');
}
} // namespace

NexusProcessor::NexusProcessor()
: AudioProcessor(BusesProperties()
                   .withInput("Entrada", juce::AudioChannelSet::stereo(), true)
                   .withOutput(juce::String::fromUTF8("Saída"), juce::AudioChannelSet::stereo(), true))
, params(*this, nullptr, "NexusModeler", createLayout())
{
  for (int i = 0; i < kPedals; ++i)
  {
    auto& p = mPedals[static_cast<size_t>(i)];
    p.drive = params.getRawParameterValue(pedalParam(i, "drive"));
    p.tone = params.getRawParameterValue(pedalParam(i, "tom"));
    p.levelParam = params.getRawParameterValue(pedalParam(i, "nivel"));
    p.on = params.getRawParameterValue(pedalParam(i, "ligado"));
    p.lite = params.getRawParameterValue(pedalParam(i, "lite"));
    p.visual = params.getRawParameterValue(pedalParam(i, "visual"));
    p.sync = params.getRawParameterValue(pedalParam(i, "sinc"));
    p.mode = params.getRawParameterValue(pedalParam(i, "modo"));
    p.fourth = params.getRawParameterValue(pedalParam(i, "d"));
    for (int k = 0; k < 5; ++k)
      p.more[static_cast<size_t>(k)] = params.getRawParameterValue(pedalParam(i, kTripleKnobs[1 + (k + 1) / 3][(k + 1) % 3]));
    for (int k = 0; k < 3; ++k)
      p.stageOn[static_cast<size_t>(k)] = params.getRawParameterValue(pedalParam(i, kTripleSwitches[k]));
    for (int k = 0; k < 3; ++k)
      p.fx[static_cast<size_t>(k)] = params.getRawParameterValue(pedalParam(i, k == 0 ? "a" : k == 1 ? "b" : "c"));
  }
  for (int r = 0; r < kRigs; ++r)
  {
    auto& rig = mRigs[static_cast<size_t>(r)];
    rig.gain = {params.getRawParameterValue(ampParam(r, "ganho")), params.getRawParameterValue(ampParam(r, "ganho_2")),
                params.getRawParameterValue(ampParam(r, "ganho_3"))};
    rig.channel = params.getRawParameterValue(ampParam(r, "canal"));
    rig.bass = params.getRawParameterValue(ampParam(r, "grave"));
    rig.mid = params.getRawParameterValue(ampParam(r, "medio"));
    rig.treble = params.getRawParameterValue(ampParam(r, "agudo"));
    rig.presence = params.getRawParameterValue(ampParam(r, "presenca"));
    rig.master = params.getRawParameterValue(ampParam(r, "master"));
    rig.look = params.getRawParameterValue(ampParam(r, "cabecote"));
    for (size_t k = 0; k < rig.graphic.size(); ++k)
      rig.graphic[k] = params.getRawParameterValue(ampParam(r, kGraphicIds[k]));
    mCabs[static_cast<size_t>(r)].mode = params.getRawParameterValue(ampParam(r, "caixa"));
  }
  mAb = params.getRawParameterValue("ab");
  mLite = params.getRawParameterValue("lite");
  mOutput = params.getRawParameterValue("volume");
  mInput = params.getRawParameterValue("entrada");
  mInputMute = params.getRawParameterValue("entrada_mudo");
  mTunerMute = params.getRawParameterValue("afinador_mudo");

  for (auto& s : mSlots)
    s.status = juce::String::fromUTF8("Nenhum pacote.");
  mLayoutUi = ChainLayout::fromValueTree({});
  mLayoutActive = new ChainLayout(mLayoutUi);

  mMidi = std::make_unique<MidiMap>(*this);
  mResampledFunc = [this](double** in, double** out, int n) {
    std::copy(in[0], in[0] + n, out[0]);
    runEngines(out[0], n);
  };
  for (auto* p : getParameters())
    p->addListener(this);
  mTempo = params.getRawParameterValue("andamento");
  for (int i = 0; i < getParameters().size(); ++i)
    if (auto* p = dynamic_cast<juce::AudioProcessorParameterWithID*>(getParameters()[i]); p != nullptr && p->paramID == "tap")
      mTapIndex = i;
  mEqOn = params.getRawParameterValue("eq_ligado");
  for (int i = 0; i < nexus::FreeEq::kMaxBands; ++i)
  {
    auto& b = mEqBands[static_cast<size_t>(i)];
    b.on = params.getRawParameterValue(eqParam(i, "ligado"));
    b.type = params.getRawParameterValue(eqParam(i, "tipo"));
    b.freq = params.getRawParameterValue(eqParam(i, "freq"));
    b.gain = params.getRawParameterValue(eqParam(i, "ganho"));
    b.q = params.getRawParameterValue(eqParam(i, "q"));
    b.slope = params.getRawParameterValue(eqParam(i, "inclinacao"));
  }
  startTimerHz(30);
}

void NexusProcessor::parameterGestureChanged(int index, bool starting)
{
  const auto* param = dynamic_cast<juce::AudioProcessorParameterWithID*>(getParameters()[index]);
  if (param == nullptr)
    return;
  const auto& id = param->paramID;
  // Quais equipamentos esse botão troca de captura.
  juce::Array<int> slots;
  for (int i = 0; i < kPedals; ++i)
    if (id == pedalParam(i, "drive"))
      slots.add(pedalSlot(i));
  for (int r = 0; r < kRigs; ++r)
    for (int c = 0; c < kChannels; ++c)
      if (id == ampParam(r, c == 0 ? "ganho" : c == 1 ? "ganho_2" : "ganho_3"))
        slots.add(ampSlot(r, c));
  for (int s : slots)
  {
    auto& a = mAnticipation[static_cast<size_t>(s)];
    const int left = a.gestures.fetch_add(starting ? 1 : -1) + (starting ? 1 : -1);
    if (left <= 0)
    {
      a.gestures = 0;
      // Depois de soltar, as vizinhas seguem aquecidas mais 1,5 s (ajuste fino).
      a.until = juce::Time::getMillisecondCounter() + 1500;
    }
  }
}

NexusProcessor::~NexusProcessor()
{
  for (auto* p : getParameters())
    p->removeListener(this);
  stopTimer();
  mLoader.removeAllJobs(true, 30000);
  for (auto& s : mSlots)
  {
    delete s.pending.exchange(nullptr);
    delete s.retired.exchange(nullptr);
    delete s.active;
  }
  delete mLayoutPending.exchange(nullptr);
  delete mLayoutRetired.exchange(nullptr);
  delete mLayoutActive;
}

juce::AudioProcessorValueTreeState::ParameterLayout NexusProcessor::createLayout()
{
  using P = juce::AudioParameterFloat;
  using B = juce::AudioParameterBool;
  using C = juce::AudioParameterChoice;
  const juce::NormalisableRange<float> knob(0.0f, 10.0f, 0.01f);
  // Botões que carregam capturas (ganho do cabeçote, drive do pedal): 11
  // estágios de clique, 0 a 10. Tom, volume e equalizador giram livres.
  const juce::NormalisableRange<float> stepped(0.0f, 10.0f, 1.0f);
  juce::AudioProcessorValueTreeState::ParameterLayout layout;

  for (int r = 0; r < kRigs; ++r)
  {
    const juce::String side = r == 0 ? " (A)" : " (B)";
    const char* gains[kChannels] = {"ganho", "ganho_2", "ganho_3"};
    for (int c = 0; c < kChannels; ++c)
      layout.add(std::make_unique<P>(juce::ParameterID{ampParam(r, gains[c]), 1}, "Ganho canal " + juce::String(c + 1) + side,
                                     stepped, 5.0f));
    layout.add(std::make_unique<C>(juce::ParameterID{ampParam(r, "canal"), 1}, "Canal" + side,
                                   juce::StringArray{"Canal 1", "Canal 2", "Canal 3"}, 0));
    layout.add(std::make_unique<P>(juce::ParameterID{ampParam(r, "grave"), 1}, "Grave" + side, knob, 5.0f));
    layout.add(std::make_unique<P>(juce::ParameterID{ampParam(r, "medio"), 1}, juce::String::fromUTF8("Médio") + side, knob, 5.0f));
    layout.add(std::make_unique<P>(juce::ParameterID{ampParam(r, "agudo"), 1}, "Agudo" + side, knob, 5.0f));
    layout.add(std::make_unique<P>(juce::ParameterID{ampParam(r, "presenca"), 1}, juce::String::fromUTF8("Presença") + side, knob, 5.0f));
    layout.add(std::make_unique<P>(juce::ParameterID{ampParam(r, "master"), 1}, "Volume do amplificador" + side, knob, 5.0f));
    layout.add(std::make_unique<C>(juce::ParameterID{ampParam(r, "caixa"), 1}, "Caixa" + side,
                                   juce::StringArray{"Desligada", juce::String::fromUTF8("IR próprio")}, 1));
    layout.add(std::make_unique<C>(juce::ParameterID{ampParam(r, "cabecote"), 1}, juce::String::fromUTF8("Cabeçote") + side,
                                   juce::StringArray{"Brit 800", "Mark Five", "Block 5150", "Recto Dual", "SL 100", "BE 100",
                                                     "Top Boost 30", "Match 30", "Tiny 15", "Rocker 50", "Recto Triple", "Power 100", "Uber 100", "Sonic 60", "Silver Twin", "Tweed 59", "Brit Plexi", "Brit Jubilee", "Jazz 120", "Vault 300", "Citrus Bass 200", "Granite 800", "Fliptop 15", "Vault V4", "Hart 1000", "Garden 800", "Glass 500", "Solar T", "Bassline 100", "Eagle 751", "Little Tube", "Workhorse 350", "Underground 800", "Acoustica 360", "Tiny Bass", "Tracer 7", "Brit Bass"}, 0));
    // Visual da caixa: cada cabeçote tem a sua, escolhida junto com ele.
    layout.add(std::make_unique<C>(juce::ParameterID{ampParam(r, "gabinete"), 1}, "Gabinete" + side,
                                   juce::StringArray{"Brit 4x12", "Mark 4x12", "Block 4x12", "SL 4x12", "BE 4x12",
                                                     "Citrus 4x12", "Blue 2x12", "Twin 2x12", "Tweed 4x10", "Match 2x12", "Power 4x12", "Uber 4x12", "Jazz 2x12", "Vault 8x10", "Granite 4x10"}, 0));
    for (const char* id : kGraphicIds)
      layout.add(std::make_unique<P>(juce::ParameterID{ampParam(r, id), 1},
                                     "EQ " + juce::String(id + 2) + " Hz" + side, juce::NormalisableRange<float>(-12.0f, 12.0f, 0.1f), 0.0f));
  }
  layout.add(std::make_unique<C>(juce::ParameterID{"ab", 1}, "Pedal A/B", juce::StringArray{"A", "B"}, 0));

  // Equalizador do fim da cadeia (estilo Pro-Q): ligado/desligado geral e
  // 24 faixas. A tela usa as faixas na ordem em que são criadas.
  layout.add(std::make_unique<B>(juce::ParameterID{"eq_ligado", 1}, "EQ ligado", true));
  layout.add(std::make_unique<B>(juce::ParameterID{"afinador_mudo", 1}, "Afinador: mudo ao afinar", true));
  layout.add(std::make_unique<P>(juce::ParameterID{"andamento", 1}, "Andamento (BPM)", juce::NormalisableRange<float>(40.0f, 240.0f, 0.1f), 120.0f));
  // Cada mudança é um toque (o valor em si não importa): dá para aprender por MIDI.
  layout.add(std::make_unique<B>(juce::ParameterID{"tap", 1}, "Tap tempo", false));
  layout.add(std::make_unique<P>(juce::ParameterID{"afinador_la", 1}, juce::String::fromUTF8("Afinador: lá de referência"),
                                 juce::NormalisableRange<float>(430.0f, 450.0f, 1.0f), 440.0f));
  {
    juce::NormalisableRange<float> freq(10.0f, 22000.0f, 0.1f);
    freq.setSkewForCentre(std::sqrt(10.0f * 22000.0f));
    juce::NormalisableRange<float> q(0.1f, 40.0f, 0.001f);
    q.setSkewForCentre(1.0f);
    const juce::StringArray types{"Sino", "Prateleira de graves", "Prateleira de agudos", "Corte de graves", "Corte de agudos", "Notch"};
    const juce::StringArray slopes{"12 dB/oitava", "24 dB/oitava", "36 dB/oitava", "48 dB/oitava"};
    for (int i = 0; i < nexus::FreeEq::kMaxBands; ++i)
    {
      const juce::String n = "EQ " + juce::String(i + 1) + " ";
      layout.add(std::make_unique<B>(juce::ParameterID{eqParam(i, "ligado"), 1}, n + "ligado", false));
      layout.add(std::make_unique<C>(juce::ParameterID{eqParam(i, "tipo"), 1}, n + "tipo", types, 0));
      layout.add(std::make_unique<P>(juce::ParameterID{eqParam(i, "freq"), 1}, n + juce::String::fromUTF8("frequência"), freq, 1000.0f));
      layout.add(std::make_unique<P>(juce::ParameterID{eqParam(i, "ganho"), 1}, n + "ganho", juce::NormalisableRange<float>(-30.0f, 30.0f, 0.01f), 0.0f));
      layout.add(std::make_unique<P>(juce::ParameterID{eqParam(i, "q"), 1}, n + "Q", q, 1.0f));
      layout.add(std::make_unique<C>(juce::ParameterID{eqParam(i, "inclinacao"), 1}, n + juce::String::fromUTF8("inclinação"), slopes, 1));
    }
  }
  for (int i = 0; i < kPedals; ++i)
  {
    const juce::String name = "Pedal " + juce::String(i + 1) + ": ";
    layout.add(std::make_unique<P>(juce::ParameterID{pedalParam(i, "drive"), 1}, name + "drive", stepped, 5.0f));
    layout.add(std::make_unique<P>(juce::ParameterID{pedalParam(i, "tom"), 1}, name + "tom", knob, 5.0f));
    layout.add(std::make_unique<P>(juce::ParameterID{pedalParam(i, "nivel"), 1}, name + juce::String::fromUTF8("nível"), knob, 5.0f));
    layout.add(std::make_unique<B>(juce::ParameterID{pedalParam(i, "ligado"), 1}, name + "ligado", true));
    // Botões dos efeitos nativos (chorus: velocidade, profundidade, mistura;
    // delay: tempo, repetições, mistura; reverb: tamanho, tom, mistura).
    layout.add(std::make_unique<P>(juce::ParameterID{pedalParam(i, "a"), 1}, name + "efeito A", knob, 5.0f));
    layout.add(std::make_unique<P>(juce::ParameterID{pedalParam(i, "b"), 1}, name + "efeito B", knob, 5.0f));
    layout.add(std::make_unique<P>(juce::ParameterID{pedalParam(i, "c"), 1}, name + "efeito C", knob, 4.0f));
    layout.add(std::make_unique<B>(juce::ParameterID{pedalParam(i, "lite"), 1}, name + "A2 Lite", false));
    layout.add(std::make_unique<B>(juce::ParameterID{pedalParam(i, "sinc"), 1}, name + "sincronizar com o andamento", false));
    layout.add(std::make_unique<C>(juce::ParameterID{pedalParam(i, "visual"), 1}, name + "visual",
                                   juce::StringArray{"Lemon Drive", "Orange Drive", "Banana Overdrive", "Gold Centaur", "Blueberry Chorus",
                                                     "Coconut Delay", "Grape Reverb", "Pear Gate", "Cherry Drop", "Tangerine Phaser",
                                                     "Dragonfruit Flanger", "Kiwi Tremolo", "Blackberry Vibe", juce::String::fromUTF8("Açaí Rat"), "Jabuticaba Buzzsaw",
                                                     "Lychee Fuzz", "Avocado Metal", "Pomegranate Mud", "Snow Pear Treble", "Juniper Sky",
                                                     "Lime Echo", "Plum Mod", "Blueberry Chain"}, 0));
  }
  layout.add(std::make_unique<B>(juce::ParameterID{"lite", 1}, juce::String::fromUTF8("A2 Lite (cabeçotes)"), false));
  layout.add(std::make_unique<P>(juce::ParameterID{"volume", 1}, juce::String::fromUTF8("Saída"),
                                 juce::NormalisableRange<float>(-24.0f, 12.0f, 0.1f), 0.0f));
  layout.add(std::make_unique<P>(juce::ParameterID{"entrada", 1}, "Entrada",
                                 juce::NormalisableRange<float>(-24.0f, 24.0f, 0.1f), 0.0f));
  // Mudo da entrada (microfone do computador apitando, troca de guitarra...).
  // O "Testar o som" continua tocando com a entrada no mudo.
  layout.add(std::make_unique<B>(juce::ParameterID{"entrada_mudo", 1}, "Entrada no mudo", false));
  // Pedais de vários modos (Juniper Sky, Lime Echo, Plum Mod): o seletor de
  // modo e o quarto botão. No fim da lista, para não mexer na ordem dos outros.
  for (int i = 0; i < kPedals; ++i)
  {
    const juce::String name = "Pedal " + juce::String(i + 1) + ": ";
    layout.add(std::make_unique<P>(juce::ParameterID{pedalParam(i, "modo"), 1}, name + "modo", stepped, 0.0f));
    layout.add(std::make_unique<P>(juce::ParameterID{pedalParam(i, "d"), 1}, name + "efeito D", knob, 4.0f));
  }
  // Blueberry Chain: os botões do segundo e do terceiro delay ("d" é o tempo
  // do segundo) e a chave de cada um. No fim, como os de cima.
  for (int i = 0; i < kPedals; ++i)
  {
    const juce::String name = "Pedal " + juce::String(i + 1) + ": ";
    const float defaults[5] = {5.0f, 4.0f, 6.0f, 4.0f, 3.0f}; // e, f, g, h, i
    for (int k = 0; k < 5; ++k)
    {
      const char* id = kTripleKnobs[1 + (k + 1) / 3][(k + 1) % 3];
      layout.add(std::make_unique<P>(juce::ParameterID{pedalParam(i, id), 1}, name + "efeito " + juce::String(id).toUpperCase(), knob,
                                     defaults[k]));
    }
    for (int k = 0; k < 3; ++k)
      layout.add(std::make_unique<B>(juce::ParameterID{pedalParam(i, kTripleSwitches[k]), 1},
                                     name + "delay " + juce::String(k + 1) + " ligado", true));
  }
  return layout;
}

bool NexusProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
  const auto out = layouts.getMainOutputChannelSet();
  if (out != juce::AudioChannelSet::mono() && out != juce::AudioChannelSet::stereo())
    return false;
  const auto in = layouts.getMainInputChannelSet();
  return in == juce::AudioChannelSet::mono() || in == juce::AudioChannelSet::stereo();
}

void NexusProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
  // O JACK do PipeWire pode avisar taxa ou bloco 0 ao religar a placa (visto
  // no Linux em 27/10/2026). Com taxa 0, o conversor de taxa e as contas de
  // bloco viram infinito e o áudio prende; usa valores seguros até o próximo
  // aviso de verdade.
  if (!(sampleRate > 1000.0))
    sampleRate = kEngineRate;
  const int block = std::max(samplesPerBlock > 0 ? samplesPerBlock : 512, 32);
  mMono.assign(static_cast<size_t>(block), 0.0);
  mMonoOut.assign(static_cast<size_t>(block), 0.0);
  mTestBuffer.assign(static_cast<size_t>(block), 0.0f);
  mTest.prepare(sampleRate);
  mTunerRate.store(sampleRate);
  mStudio.prepare(sampleRate, block);
  // Capturas no tamanho do bloco que vai chegar (na taxa das capturas).
  {
    const int want = juce::jlimit(32, kEngineBlock,
                                  juce::nextPowerOfTwo(static_cast<int>(std::ceil(block * kEngineRate / sampleRate))));
    if (want != mEngineBlock.exchange(want))
      for (int i = 0; i < kSlots; ++i)
        if (mSlots[static_cast<size_t>(i)].folder.isDirectory())
          loadSlot(i, mSlots[static_cast<size_t>(i)].folder);
  }
  for (auto* v : {&mDry, &mChannelIn, &mChannelOut, &mRigIn})
    v->assign(static_cast<size_t>(kEngineBlock), 0.0);
  mCabFloat.assign(static_cast<size_t>(kEngineBlock), 0.0f);
  mNeedsResampling = std::abs(sampleRate - kEngineRate) > 0.5;
  if (mNeedsResampling)
  {
    mResampler = std::make_unique<Resampler>(kEngineRate);
    mResampler->Reset(sampleRate, block);
    setLatencySamples(mResampler->GetLatency());
  }
  else
  {
    mResampler.reset();
    setLatencySamples(0);
  }
  mOutputGain = juce::Decibels::decibelsToGain(mOutput->load());
  mInputGain = juce::Decibels::decibelsToGain(mInput->load());
  mDropFloat.assign(static_cast<size_t>(kEngineBlock), 0.0f);
  for (auto& p : mPedals)
  {
    p.native.prepare(kEngineRate, kEngineBlock);
    // Drop: janela de 384 amostras (8 ms de atraso), a escolhida de ouvido
    // entre 256, 384, 512 e 1024 com o riff Pesado descendo 5 semitons.
    if (!p.drop)
      p.drop = std::make_unique<nexus::fx::PitchDrop>();
    p.drop->prepare(kEngineRate, kEngineBlock, 384);
    p.dropMix = 0.0f;
    p.mix = p.on->load() > 0.5f ? 1.0f : 0.0f;
    const float l = p.levelParam->load() / 5.0f;
    p.level = l * l;
    p.warming = false;
  }
  for (auto& r : mRigs)
  {
    const float m = r.master->load() / 5.0f;
    r.masterGain = m * m;
  }
  for (auto& e : mEq)
    e.prepare(kEngineRate);
  for (auto& e : mGraphicEq)
    e.prepare(kEngineRate);
  mFreeEq.prepare(kEngineRate);
  // As caixas rodam dentro da cadeia, na taxa das capturas.
  for (auto& c : mCabs)
  {
    c.conv.prepare({kEngineRate, static_cast<juce::uint32>(mEngineBlock.load()), 1});
    c.mix.reset(kEngineRate, 0.03);
    c.mix.setCurrentAndTargetValue(0.0f);
  }
}

int NexusProcessor::selectedChannel(int rig) const
{
  return juce::jlimit(0, kChannels - 1, juce::roundToInt(mRigs[static_cast<size_t>(rig)].channel->load()));
}

int NexusProcessor::audibleSlot(int rig) const
{
  const int cur = mRigs[static_cast<size_t>(rig)].current;
  return cur >= 0 ? cur : ampSlot(rig, selectedChannel(rig));
}

void NexusProcessor::settle(NexusEngine& e, const double* dry, double* out, int n)
{
  if (e.settleLeft < 0)
    return;
  if (e.settleLeft > 0)
  {
    e.settleLeft = std::max(0, e.settleLeft - n);
    for (int i = 0; i < n; ++i)
      out[i] = dry != nullptr ? dry[i] : 0.0;
    return;
  }
  const float step = static_cast<float>(1.0 / (kSwitchFadeSeconds * kEngineRate));
  for (int i = 0; i < n; ++i)
  {
    e.settleGain = std::min(1.0f, e.settleGain + step);
    out[i] = (dry != nullptr ? dry[i] : 0.0) * (1.0 - e.settleGain) + out[i] * e.settleGain;
  }
  if (e.settleGain >= 1.0f)
    e.settleLeft = -1;
}

int NexusProcessor::warmupOf(const NexusEngine& e)
{
  int warm = 1024;
  for (const auto& c : e.pack->captures)
    warm = std::max(warm, c.warmupSamples);
  return warm;
}

nexus::AmpVoice::Controls NexusProcessor::ampControls(int rig, int channel) const
{
  const auto& r = mRigs[static_cast<size_t>(rig)];
  nexus::AmpVoice::Controls c;
  c.main = r.gain[static_cast<size_t>(channel)]->load();
  c.tone.bass = r.bass->load();
  c.tone.mid = r.mid->load();
  c.tone.treble = r.treble->load();
  c.presence = r.presence->load();
  return c;
}

nexus::AmpVoice::Controls NexusProcessor::pedalControls(int pedal) const
{
  // O pedal usa o mesmo motor: o drive troca as capturas; o "tom" é uma
  // prateleira de agudos relativa à posição 5 (onde o pedal foi capturado).
  const auto& p = mPedals[static_cast<size_t>(pedal)];
  nexus::AmpVoice::Controls c;
  c.main = p.drive->load();
  c.presence = p.tone->load();
  return c;
}

namespace
{
// Soma o tempo de processador gasto num bloco (pedal ou cabeçote) enquanto existe.
struct LoadTimer
{
  explicit LoadTimer(double& total) : mTotal(total), mStart(juce::Time::getHighResolutionTicks()) {}
  ~LoadTimer() { mTotal += juce::Time::highResolutionTicksToSeconds(juce::Time::getHighResolutionTicks() - mStart); }
  double& mTotal;
  juce::int64 mStart;
};
} // namespace

void NexusProcessor::runDrop(int pedal, double* buffer, int n)
{
  // Cherry Drop: botão A escolhe os semitons (meio-dia = sem mudar, para a
  // esquerda desce até -12, para a direita sobe até +12); botão B é o brilho,
  // para compensar o som que fica mais opaco quando desce.
  auto& p = mPedals[static_cast<size_t>(pedal)];
  const float target = p.on->load() > 0.5f ? 1.0f : 0.0f;
  if (target == 0.0f && p.dropMix == 0.0f)
    return;
  const int semis = dropSemitones(p.fx[0]->load());
  const float shiftTarget = semis != 0 ? 1.0f : 0.0f;
  if (semis != 0 && static_cast<float>(semis) != p.dropSemitones)
  {
    p.dropSemitones = static_cast<float>(semis);
    p.drop->setSemitones(p.dropSemitones);
  }
  // O motor só trabalha com drop escolhido; no meio-dia passa o som direto.
  const bool shifting = shiftTarget > 0.0f || p.dropShift > 0.0f;
  if (shifting && (p.dropShift == 0.0f || p.dropMix == 0.0f))
    p.drop->reset(); // começa limpo, sem resto da última vez
  for (int i = 0; i < n; ++i)
    mDropFloat[static_cast<size_t>(i)] = static_cast<float>(buffer[i]);
  if (shifting)
    p.drop->process(mDropFloat.data(), n);
  // Brilho: meio-dia é o som como sai; para a direita realça os agudos, para a
  // esquerda escurece (filtro de prateleira simples em volta de 1,5 kHz).
  const float b = (p.fx[1]->load() - 5.0f) / 5.0f;
  const double bright = b >= 0.0f ? 1.5 * b : 0.75 * b;
  const double lowCoef = 1.0 - std::exp(-juce::MathConstants<double>::twoPi * 1500.0 / kEngineRate);
  const float mixStep = static_cast<float>(1.0 / (kPedalFadeSeconds * kEngineRate));
  for (int i = 0; i < n; ++i)
  {
    p.dropMix = target > p.dropMix ? std::min(target, p.dropMix + mixStep) : std::max(target, p.dropMix - mixStep);
    p.dropShift = shiftTarget > p.dropShift ? std::min(shiftTarget, p.dropShift + mixStep) : std::max(shiftTarget, p.dropShift - mixStep);
    double effect = buffer[i] * (1.0 - p.dropShift) + mDropFloat[static_cast<size_t>(i)] * p.dropShift;
    p.dropLow += lowCoef * (effect - p.dropLow);
    effect += bright * (effect - p.dropLow);
    buffer[i] = buffer[i] * (1.0 - p.dropMix) + effect * p.dropMix;
  }
}

void NexusProcessor::runPedal(int pedal, double* buffer, int n)
{
  const LoadTimer timer(mLoadSeconds[static_cast<size_t>(pedalSlot(pedal))]);
  if (pedalLook(pedal) == kDropLook)
    return runDrop(pedal, buffer, n);
  // Efeito do próprio Nexus (modulação, delay, reverb, gate): sem captura.
  if (const int look = pedalLook(pedal); isNativeLook(look))
  {
    auto& p = mPedals[static_cast<size_t>(pedal)];
    if (pedalKind(look) == PedalKind::triple)
    {
      // Três delays em série: cada um com seus botões; no andamento, o tempo
      // de cada um escolhe a sua divisão.
      const bool sync = p.sync->load() > 0.5f;
      const std::array<float, 9> k = {p.fx[0]->load(), p.fx[1]->load(), p.fx[2]->load(), p.fourth->load(), p.more[0]->load(),
                                      p.more[1]->load(), p.more[2]->load(), p.more[3]->load(), p.more[4]->load()};
      std::array<nexus::TripleDelay::Stage, 3> stages{};
      for (size_t s = 0; s < 3; ++s)
      {
        auto& st = stages[s];
        st.on = p.stageOn[s]->load() > 0.5f;
        st.time = k[s * 3] / 10.0;
        st.repeats = k[s * 3 + 1] / 10.0;
        st.level = k[s * 3 + 2] / 10.0;
        st.syncSeconds = sync ? divisionBeats(divisionFor(k[s * 3])) * 60.0 / tempoBpm() : 0.0;
      }
      p.native.setTriple(stages);
      p.native.process(nexus::NativePedal::Type::triple, p.on->load() > 0.5f, buffer, n, 0.0, 0.0, 0.0);
      return;
    }
    // Andamento: a velocidade (botão A) escolhe a divisão; no delay, o tempo.
    if (p.sync->load() > 0.5f && syncsToTempo(look))
    {
      const double seconds = divisionBeats(divisionFor(p.fx[0]->load())) * 60.0 / tempoBpm();
      if (pedalKind(look) == PedalKind::delay || pedalKind(look) == PedalKind::echo)
        p.native.setSync(0.0, seconds);
      else
        p.native.setSync(1.0 / seconds, 0.0);
    }
    else
      p.native.setSync(0.0, 0.0);
    using T = nexus::NativePedal::Type;
    T type = T::chorus;
    switch (pedalKind(look))
    {
      case PedalKind::delay: type = T::delay; break;
      case PedalKind::reverb: type = T::reverb; break;
      case PedalKind::gate: type = T::gate; break;
      case PedalKind::phaser: type = T::phaser; break;
      case PedalKind::flanger: type = T::flanger; break;
      case PedalKind::tremolo: type = T::tremolo; break;
      case PedalKind::vibe: type = T::vibe; break;
      case PedalKind::sky: type = T::sky; break;
      case PedalKind::echo: type = T::echo; break;
      case PedalKind::multiMod: type = T::multiMod; break;
      default: break;
    }
    // Vários modos: o modo, o tom, o quarto botão e (no reverb) o shimmer, que usa o "nível".
    if (hasModes(look))
      p.native.setExtras(juce::roundToInt(p.mode->load()), p.tone->load() / 10.0, p.fourth->load() / 10.0, p.levelParam->load() / 10.0);
    p.native.process(type, p.on->load() > 0.5f, buffer, n, p.fx[0]->load() / 10.0, p.fx[1]->load() / 10.0, p.fx[2]->load() / 10.0);
    return;
  }
  auto* engine = mSlots[static_cast<size_t>(pedalSlot(pedal))].active;
  if (engine == nullptr)
    return; // pedal sem captura: o sinal passa direto
  auto& p = mPedals[static_cast<size_t>(pedal)];
  const float target = p.on->load() > 0.5f ? 1.0f : 0.0f;
  // Desligado e já em silêncio: não gasta processador.
  if (target == 0.0f && p.mix == 0.0f)
    return;
  // Vai ligar: primeiro aquece ouvindo a guitarra, depois entra.
  if (target == 1.0f && p.mix == 0.0f && !p.warming)
  {
    p.warming = true;
    p.warmLeft = warmupOf(*engine);
  }
  std::copy(buffer, buffer + n, mDry.begin());
  engine->voice.process(buffer, buffer, n);
  settle(*engine, mDry.data(), buffer, n);
  if (p.warming && p.warmLeft > 0)
  {
    p.warmLeft -= n;
    std::copy(mDry.begin(), mDry.begin() + n, buffer);
    return;
  }
  p.warming = false;
  const float mixStep = static_cast<float>(1.0 / (kPedalFadeSeconds * kEngineRate));
  const float l = p.levelParam->load() / 5.0f;
  const float levelStep = (l * l - p.level) / static_cast<float>(n);
  for (int i = 0; i < n; ++i)
  {
    p.mix = target > p.mix ? std::min(target, p.mix + mixStep) : std::max(target, p.mix - mixStep);
    p.level += levelStep;
    buffer[i] = mDry[static_cast<size_t>(i)] * (1.0 - p.mix) + buffer[i] * p.level * p.mix;
  }
}

void NexusProcessor::runAmp(int rig, double* buffer, int n)
{
  const LoadTimer timer(mLoadSeconds[static_cast<size_t>(audibleSlot(rig))]);
  auto& r = mRigs[static_cast<size_t>(rig)];
  // Canal pedido; se ele não tem pacote, fica no que está tocando.
  const int first = ampSlot(rig, 0);
  const int wanted = ampSlot(rig, selectedChannel(rig));
  if (r.current < 0 || mSlots[static_cast<size_t>(r.current)].active == nullptr)
  {
    r.current = -1;
    for (int c = 0; c < kChannels && r.current < 0; ++c)
    {
      const int slot = first + (wanted - first + c) % kChannels;
      if (mSlots[static_cast<size_t>(slot)].active)
        r.current = slot;
    }
    r.next = -1;
  }
  if (r.current >= 0 && wanted != r.current && wanted != r.next && mSlots[static_cast<size_t>(wanted)].active)
  {
    r.next = wanted;
    r.warmLeft = warmupOf(*mSlots[static_cast<size_t>(wanted)].active);
    r.fade = 0.0f;
  }
  if (r.next >= 0 && mSlots[static_cast<size_t>(r.next)].active == nullptr)
    r.next = -1;

  if (r.current >= 0)
  {
    auto* current = mSlots[static_cast<size_t>(r.current)].active;
    if (r.next < 0)
    {
      current->voice.process(buffer, buffer, n);
      settle(*current, nullptr, buffer, n); // cabeçote novo: sobe do silêncio
    }
    else
    {
      auto* next = mSlots[static_cast<size_t>(r.next)].active;
      std::copy(buffer, buffer + n, mChannelIn.begin());
      current->voice.process(buffer, buffer, n);
      next->voice.process(mChannelIn.data(), mChannelOut.data(), n);
      if (r.warmLeft > 0)
        r.warmLeft -= n; // o canal que entra ainda está ouvindo a guitarra
      else
      {
        const float step = static_cast<float>(1.0 / (kSwitchFadeSeconds * kEngineRate));
        for (int i = 0; i < n; ++i)
        {
          r.fade = std::min(1.0f, r.fade + step);
          buffer[i] = buffer[i] * (1.0 - r.fade) + mChannelOut[static_cast<size_t>(i)] * r.fade;
        }
        if (r.fade >= 1.0f)
        {
          r.current = r.next;
          r.next = -1;
        }
      }
    }
  }

  // Volume do painel: 5 = como a captura soa, 10 = +12 dB, 0 = mudo.
  const float m = r.master->load() / 5.0f;
  const float step = (m * m - r.masterGain) / static_cast<float>(n);
  for (int i = 0; i < n; ++i)
  {
    r.masterGain += step;
    buffer[i] *= r.masterGain;
  }
}

void NexusProcessor::runCab(int rig, double* buffer, int n)
{
  const LoadTimer timer(mLoadSeconds[static_cast<size_t>(audibleSlot(rig))]); // a caixa conta no cabeçote
  auto& cab = mCabs[static_cast<size_t>(rig)];
  // A caixa só toca com IR, modo "IR próprio" e captura sem caixa.
  const bool on = cab.loaded.load() && cab.mode->load() > 0.5f && !mIncludesCab[static_cast<size_t>(audibleSlot(rig))].load();
  cab.mix.setTargetValue(on ? 1.0f : 0.0f);
  if (!cab.loaded.load() || (!on && !cab.mix.isSmoothing() && cab.mix.getCurrentValue() == 0.0f))
  {
    cab.mix.skip(n);
    return;
  }
  for (int i = 0; i < n; ++i)
    mCabFloat[static_cast<size_t>(i)] = static_cast<float>(buffer[i]);
  float* wet = mCabFloat.data();
  juce::dsp::AudioBlock<float> block(&wet, 1, static_cast<size_t>(n));
  cab.conv.process(juce::dsp::ProcessContextReplacing<float>(block));
  for (int i = 0; i < n; ++i)
  {
    const float m = cab.mix.getNextValue();
    buffer[i] = buffer[i] * (1.0 - m) + wet[i] * m;
  }
}

void NexusProcessor::runRig(int rig, double* buffer, int n)
{
  runAmp(rig, buffer, n);
  for (int p : mLayoutActive->loop[static_cast<size_t>(rig)])
    runPedal(p, buffer, n);
  runCab(rig, buffer, n);
  // Equalizador universal: depois da captura e da caixa, igual em todo cabeçote.
  const auto& r = mRigs[static_cast<size_t>(rig)];
  mEq[static_cast<size_t>(rig)].setKnobs({r.bass->load(), r.mid->load(), r.treble->load(), r.presence->load()});
  mEq[static_cast<size_t>(rig)].process(buffer, n);
  // Cabeçote com equalizador gráfico (Mark Five): as 5 faixas, também depois da caixa.
  if (ampLook(rig) == kMarkFive)
  {
    std::array<double, nexus::GraphicEq5::kBands> db{};
    for (size_t k = 0; k < db.size(); ++k)
      db[k] = r.graphic[k]->load();
    mGraphicEq[static_cast<size_t>(rig)].setBands(db);
    mGraphicEq[static_cast<size_t>(rig)].process(buffer, n);
  }
}

void NexusProcessor::runRigs(double* buffer, int n)
{
  // Pedal A/B: escolhe o lado; com um cabeçote só, sempre o A.
  const int wanted = mLayoutActive->dual && mAb->load() > 0.5f ? 1 : 0;
  if (wanted != mRigCurrent && wanted != mRigNext)
  {
    mRigNext = wanted;
    mRigFade = 0.0f;
    auto* engine = mSlots[static_cast<size_t>(audibleSlot(wanted))].active;
    mRigWarmLeft = engine ? warmupOf(*engine) : 0;
  }
  if (mRigNext < 0)
  {
    runRig(mRigCurrent, buffer, n);
    return;
  }
  // Os dois lados tocam juntos só durante a troca.
  std::copy(buffer, buffer + n, mRigIn.begin());
  runRig(mRigCurrent, buffer, n);
  runRig(mRigNext, mRigIn.data(), n);
  if (mRigWarmLeft > 0)
  {
    mRigWarmLeft -= n;
    return;
  }
  const float step = static_cast<float>(1.0 / (kSwitchFadeSeconds * kEngineRate));
  for (int i = 0; i < n; ++i)
  {
    mRigFade = std::min(1.0f, mRigFade + step);
    buffer[i] = buffer[i] * (1.0 - mRigFade) + mRigIn[static_cast<size_t>(i)] * mRigFade;
  }
  if (mRigFade >= 1.0f)
  {
    mRigCurrent = mRigNext;
    mRigNext = -1;
  }
}

void NexusProcessor::runEngines(double* buffer, int numFrames)
{
  // Pedaços do tamanho para o qual as capturas foram preparadas (a menor
  // entre as ativas: durante uma recarga podem conviver duas medidas).
  int chunk = mEngineBlock.load();
  for (const auto& s : mSlots)
    if (s.active != nullptr && s.active->block > 0)
      chunk = std::min(chunk, s.active->block);
  chunk = std::max(chunk, 1);
  const auto& layout = *mLayoutActive;
  for (int start = 0; start < numFrames; start += chunk)
  {
    const int n = std::min(chunk, numFrames - start);
    double* b = buffer + start;
    for (int p : layout.pre)
      runPedal(p, b, n);
    runRigs(b, n);
    for (int p : layout.post)
      runPedal(p, b, n);
    runEndEqs(b, n);
  }
  // A cada meio segundo de som, publica a parte de um núcleo que cada bloco usou.
  mLoadWindow += numFrames / kEngineRate;
  if (mLoadWindow >= 0.5)
  {
    for (size_t i = 0; i < mLoadSeconds.size(); ++i)
    {
      mUiLoad[i].store(static_cast<float>(mLoadSeconds[i] / mLoadWindow));
      mLoadSeconds[i] = 0.0;
    }
    mLoadWindow = 0.0;
  }
}

void NexusProcessor::runEndEqs(double* buffer, int n)
{
  // Equalizador do fim: fixo, todo o som passa por ele (plano sem faixas).
  if (mEqOn->load() < 0.5f)
    return;
  nexus::FreeEq::Bands bands;
  for (size_t i = 0; i < bands.size(); ++i)
  {
    const auto& p = mEqBands[i];
    auto& b = bands[i];
    b.on = p.on->load() > 0.5f;
    b.type = static_cast<nexus::FreeEq::Type>(juce::roundToInt(p.type->load()));
    b.freq = p.freq->load();
    b.gainDb = p.gain->load();
    b.q = p.q->load();
    b.slope = juce::roundToInt(p.slope->load()) + 1;
  }
  mFreeEq.set(bands);
  mFreeEq.process(buffer, n);
}

void NexusProcessor::publish(int slot)
{
  auto* engine = mSlots[static_cast<size_t>(slot)].active;
  auto& ui = mUi[static_cast<size_t>(slot)];
  if (!engine)
  {
    ui.count.store(0);
    return;
  }
  const auto& morph = engine->voice.morph();
  const int count = std::min(kMaxCaptures, static_cast<int>(engine->pack->captures.size()));
  for (int i = 0; i < count; ++i)
  {
    ui.weights[static_cast<size_t>(i)].store(static_cast<float>(morph.weightOf(i)));
    ui.positions[static_cast<size_t>(i)].store(static_cast<float>(engine->pack->captures[static_cast<size_t>(i)].position));
  }
  ui.count.store(count);
  ui.position.store(static_cast<float>(morph.currentPosition()));
}

void NexusProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
  juce::ScopedNoDenormals noDenormals;
  // Andamento do programa de música (quando ele manda): vale no lugar do tap.
  double hostBpm = 0.0;
  if (auto* head = getPlayHead())
    if (const auto pos = head->getPosition())
      if (const auto bpm = pos->getBpm())
        hostBpm = *bpm;
  mHostBpm.store(juce::JUCEApplicationBase::isStandaloneApp() ? 0.0 : hostBpm);
  // MIDI primeiro: canal, pedais, A/B e botões valem já neste bloco.
  mMidi->process(midiMessages);
  midiMessages.clear();

  // Pacotes novos e cadeia nova: só troca se o anterior já foi recolhido.
  for (auto& s : mSlots)
    if (s.retired.load() == nullptr)
    {
      if (auto* next = s.pending.exchange(nullptr))
      {
        s.retired.store(s.active);
        next->settleLeft = warmupOf(*next);
        next->settleGain = 0.0f;
        s.active = next;
      }
      else if (s.clear.exchange(false))
      {
        s.retired.store(s.active);
        s.active = nullptr;
      }
    }
  if (mLayoutRetired.load() == nullptr)
    if (auto* next = mLayoutPending.exchange(nullptr))
    {
      mLayoutRetired.store(mLayoutActive);
      mLayoutActive = next;
    }

  bool anyEngine = false;
  for (auto& s : mSlots)
    anyEngine = anyEngine || s.active != nullptr;
  // Efeito do Nexus na cadeia também processa, mesmo sem nenhuma captura.
  for (const auto* list : {&mLayoutActive->pre, &mLayoutActive->loop[0], &mLayoutActive->loop[1], &mLayoutActive->post})
    for (int pedal : *list)
      anyEngine = anyEngine || isNativeLook(pedalLook(pedal));
  const auto now = juce::Time::getMillisecondCounter();
  for (int i = 0; i < kSlots; ++i)
    if (auto* e = mSlots[static_cast<size_t>(i)].active)
    {
      const auto& a = mAnticipation[static_cast<size_t>(i)];
      e->voice.setAnticipate(a.gestures.load() > 0 || now < a.until.load());
    }
  for (int i = 0; i < kPedals; ++i)
    if (auto* e = mSlots[static_cast<size_t>(pedalSlot(i))].active)
      e->voice.setControls(pedalControls(i));
  for (int r = 0; r < kRigs; ++r)
    for (int c = 0; c < kChannels; ++c)
      if (auto* e = mSlots[static_cast<size_t>(ampSlot(r, c))].active)
      {
        // O equalizador do cabeçote agora é o universal (depois da caixa):
        // o da família fica na posição da captura, sem mexer no som.
        auto controls = ampControls(r, c);
        controls.tone = e->pack->captureTone;
        controls.presence = e->pack->capturePresence;
        e->voice.setControls(controls);
      }

  const int total = buffer.getNumSamples();
  const int inChannels = getTotalNumInputChannels();
  const int outChannels = getTotalNumOutputChannels();
  const int capacity = static_cast<int>(mMono.size());
  if (capacity <= 0)
  {
    // Ainda não preparado: silêncio em vez de um laço sem fim.
    buffer.clear();
    return;
  }
  // Afinando com o mudo: a saída desce a zero (com a rampa de sempre, sem estalo).
  const bool tunerMute = mTunerOpen.load() && mTunerMute->load() > 0.5f;
  const float targetGain = tunerMute ? 0.0f : juce::Decibels::decibelsToGain(mOutput->load());
  // Entrada no mudo, ou o controle todo para baixo: corta de vez (o mínimo de
  // -24 dB ainda deixa passar o bastante para apitar com ganho alto).
  const bool inputOff = mInputMute->load() > 0.5f || mInput->load() <= -23.95f;
  const float targetIn = inputOff ? 0.0f : juce::Decibels::decibelsToGain(mInput->load());
  float inPeak = 0.0f, outPeak = 0.0f;

  for (int start = 0; start < total; start += capacity)
  {
    const int n = std::min(capacity, total - start);
    // Entrada: a guitarra ou, no "Testar o som", a gravação de teste.
    const float* in = inChannels > 0 ? buffer.getReadPointer(0, start) : nullptr;
    const bool testing = mTest.process(in, mTestBuffer.data(), n);
    if (testing)
      in = mTestBuffer.data();
    if (in != nullptr)
    {
      // Afinador: guarda a guitarra crua (o mudo da entrada não cala o afinador).
      int w = mTunerWrite.load(std::memory_order_relaxed);
      for (int i = 0; i < n; ++i)
      {
        mTunerRing[static_cast<size_t>(w)] = in[i];
        w = (w + 1) & (kTunerRing - 1);
      }
      mTunerWrite.store(w, std::memory_order_release);
      // O teste já vem no nível de uma guitarra: não passa pelo controle de
      // entrada nem pelo mudo (quem baixou a entrada por causa do microfone
      // do notebook ouviria o teste quase mudo).
      const float wantIn = testing ? 1.0f : targetIn;
      const float inStep = (wantIn - mInputGain) / static_cast<float>(n);
      float gi = mInputGain;
      for (int i = 0; i < n; ++i)
      {
        gi += inStep;
        const float v = in[i] * gi;
        inPeak = std::max(inPeak, std::abs(v));
        mMono[static_cast<size_t>(i)] = v;
      }
      mInputGain = wantIn;
    }
    else
      std::fill(mMono.begin(), mMono.begin() + n, 0.0);

    if (anyEngine && mNeedsResampling)
    {
      double* in[1] = {mMono.data()};
      double* out[1] = {mMonoOut.data()};
      mResampler->ProcessBlock(in, out, n, mResampledFunc);
    }
    else
    {
      std::copy(mMono.begin(), mMono.begin() + n, mMonoOut.begin());
      if (anyEngine)
        runEngines(mMonoOut.data(), n);
    }

    const float step = (targetGain - mOutputGain) / static_cast<float>(n);
    for (int ch = 0; ch < outChannels; ++ch)
    {
      float* out = buffer.getWritePointer(ch, start);
      float g = mOutputGain;
      for (int i = 0; i < n; ++i)
      {
        g += step;
        out[i] = static_cast<float>(mMonoOut[static_cast<size_t>(i)]) * g;
        if (ch == 0)
          outPeak = std::max(outPeak, std::abs(out[i]));
      }
    }
    mOutputGain = targetGain;
    // Estúdio: a música entra depois da guitarra (não passa pela cadeia).
    if (outChannels > 0)
      mStudio.process(buffer.getWritePointer(0, start), outChannels > 1 ? buffer.getWritePointer(1, start) : nullptr, n);
  }

  // Espelho para a tela.
  mInPeak.store(std::max(inPeak, mInPeak.load()));
  mOutPeak.store(std::max(outPeak, mOutPeak.load()));
  for (int s = 0; s < kSlots; ++s)
    publish(s);
  mUiActiveRig.store(mRigNext >= 0 ? mRigNext : mRigCurrent);
}

const char* NexusProcessor::divisionName(int i)
{
  static const char* const names[kDivisions] = {"1/1", "1/2", "1/4", "1/8.", "1/6", "1/8", "1/12", "1/16", "1/32"};
  return names[juce::jlimit(0, kDivisions - 1, i)];
}

juce::String NexusProcessor::modeName(int look, float modeKnob)
{
  const int mode = juce::roundToInt(modeKnob);
  switch (pedalKind(look))
  {
    case PedalKind::sky: return TRANS(juce::String::fromUTF8(nexus::SkyReverb::modeName(mode)));
    case PedalKind::echo: return TRANS(juce::String::fromUTF8(nexus::EchoDelay::modeName(mode)));
    case PedalKind::multiMod: return TRANS(juce::String::fromUTF8(nexus::MultiMod::modeName(mode)));
    default: return {};
  }
}

double NexusProcessor::divisionBeats(int i)
{
  static constexpr double beats[kDivisions] = {4.0, 2.0, 1.0, 0.75, 2.0 / 3.0, 0.5, 1.0 / 3.0, 0.25, 0.125};
  return beats[juce::jlimit(0, kDivisions - 1, i)];
}

double NexusProcessor::tempoBpm() const
{
  const double host = mHostBpm.load();
  return host > 0.0 ? host : static_cast<double>(mTempo->load());
}

void NexusProcessor::parameterValueChanged(int index, float)
{
  if (index == mTapIndex)
    tap();
}

void NexusProcessor::tap()
{
  // Toques com mais de 2 s de distância começam a contagem de novo.
  const double now = juce::Time::getMillisecondCounterHiRes();
  const double interval = now - mLastTap;
  mLastTap = now;
  if (interval > 2000.0 || interval < 150.0)
  {
    mTapCount = 0;
    return;
  }
  mTapIntervals[static_cast<size_t>(mTapCount % static_cast<int>(mTapIntervals.size()))] = interval;
  ++mTapCount;
  const int n = std::min(mTapCount, static_cast<int>(mTapIntervals.size()));
  double sum = 0.0;
  for (int i = 0; i < n; ++i)
    sum += mTapIntervals[static_cast<size_t>(i)];
  const double bpm = juce::jlimit(40.0, 240.0, 60000.0 / (sum / n));
  if (auto* tempo = params.getParameter("andamento"))
    tempo->setValueNotifyingHost(tempo->convertTo0to1(static_cast<float>(bpm)));
}

double NexusProcessor::tunerSnapshot(float* dest, int count) const
{
  // Cópia sem trava: um pedaço pode ser sobrescrito no meio, o que só atrapalha
  // uma leitura entre trinta por segundo.
  count = std::min(count, kTunerRing);
  const int end = mTunerWrite.load(std::memory_order_acquire);
  for (int i = 0; i < count; ++i)
    dest[i] = mTunerRing[static_cast<size_t>((end - count + i + kTunerRing) & (kTunerRing - 1))];
  return mTunerRate.load();
}

void NexusProcessor::setLayout(const ChainLayout& layout)
{
  mLayoutUi = layout;
  delete mLayoutPending.exchange(new ChainLayout(layout));
}

void NexusProcessor::clearSlot(int slotIndex)
{
  auto& slot = mSlots[static_cast<size_t>(slotIndex)];
  if (slot.folder == juce::File() && !mSlotLoaded[static_cast<size_t>(slotIndex)].load())
    return;
  slot.folder = juce::File();
  mSlotLoaded[static_cast<size_t>(slotIndex)].store(false);
  mIncludesCab[static_cast<size_t>(slotIndex)].store(false);
  mUi[static_cast<size_t>(slotIndex)].count.store(0);
  {
    const juce::ScopedLock sl(mStatusLock);
    slot.status.clear();
    slot.name.clear();
  }
  slot.clear.store(true);
}

void NexusProcessor::loadSlot(int slotIndex, const juce::File& folder)
{
  auto& slot = mSlots[static_cast<size_t>(slotIndex)];
  slot.folder = folder;
  const bool isPedal = slotIndex < kPedals;
  const bool lite = isPedal ? mPedals[static_cast<size_t>(slotIndex)].lite->load() > 0.5f : mLite->load() > 0.5f;
  slot.loadedLite = lite;
  {
    const juce::ScopedLock sl(mStatusLock);
    slot.status = "Carregando " + folder.getFileName() + "...";
  }
  const auto controls = isPedal ? pedalControls(slotIndex)
                                : ampControls((slotIndex - kPedals) / kChannels, (slotIndex - kPedals) % kChannels);
  mLoader.addJob([this, folder, lite, controls, slotIndex] {
    auto& s = mSlots[static_cast<size_t>(slotIndex)];
    try
    {
      nexus::PackOptions options;
      options.sampleRate = kEngineRate;
      const int block = mEngineBlock.load();
      options.maxBlockSize = block;
      options.slimmableSize = lite ? 0.0 : 1.0;
      // Pedal sai no nível da própria captura; cabeçote, no volume de referência.
      options.keepCaptureLevel = slotIndex < kPedals;
      auto engine = std::make_unique<NexusEngine>();
      engine->block = block;
      engine->pack = nexus::loadPack(folder.getFullPathName().toStdString(), options);
      auto start = controls;
      if (slotIndex >= kPedals)
      {
        // Cabeçote: o equalizador é o universal (depois da caixa).
        start.tone = engine->pack->captureTone;
        start.presence = engine->pack->capturePresence;
      }
      engine->voice.setControls(start);
      engine->voice.prepare(engine->pack, kEngineRate, block);
      const auto& pack = *engine->pack;
      const juce::String text = juce::String::fromUTF8(pack.name.c_str()) + ": "
                                + juce::String(static_cast<int>(pack.captures.size())) + " capturas, de "
                                + commaDecimal(pack.minPosition()) + " a " + commaDecimal(pack.maxPosition())
                                + (lite ? " (A2 Lite)" : " (A2 Full)");
      {
        const juce::ScopedLock sl(mStatusLock);
        s.status = text;
        s.name = juce::String::fromUTF8(pack.name.c_str());
      }
      mIncludesCab[static_cast<size_t>(slotIndex)].store(pack.includesCab);
      mSlotLoaded[static_cast<size_t>(slotIndex)].store(true);
      delete s.pending.exchange(engine.release());
    }
    catch (const std::exception& e)
    {
      const juce::ScopedLock sl(mStatusLock);
      s.status = juce::String("Erro: ") + juce::String::fromUTF8(e.what());
      nexus::report::log("captura", folder.getFullPathName() + ": " + juce::String::fromUTF8(e.what()));
    }
  });
}

juce::String NexusProcessor::statusText(int slot) const
{
  const juce::ScopedLock sl(mStatusLock);
  return mSlots[static_cast<size_t>(slot)].status;
}

juce::String NexusProcessor::packName(int slot) const
{
  const juce::ScopedLock sl(mStatusLock);
  return mSlots[static_cast<size_t>(slot)].name;
}

void NexusProcessor::loadIrAsync(int rig, const juce::File& file)
{
  if (!file.existsAsFile())
  {
    if (file != juce::File())
      nexus::report::log("ir", "arquivo não encontrado: " + file.getFullPathName());
    return;
  }
  auto& cab = mCabs[static_cast<size_t>(rig)];
  cab.file = file;
  juce::AudioFormatManager formats;
  formats.registerBasicFormats();
  std::unique_ptr<juce::AudioFormatReader> reader(formats.createReaderFor(file));
  if (reader == nullptr || reader->lengthInSamples <= 0)
  {
    nexus::report::log("ir", "não consegui ler: " + file.getFullPathName());
    return;
  }
  const int length = static_cast<int>(std::min<juce::int64>(reader->lengthInSamples, static_cast<juce::int64>(reader->sampleRate * 2.0)));
  juce::AudioBuffer<float> ir(1, length);
  reader->read(&ir, 0, length, 0, true, false);
  // Volume do IR: o JUCE normalizaria para 18 dB abaixo, e ligar a caixa
  // derrubava o som em 10 dB. Aqui o ganho deixa o volume igual ao de sem
  // caixa para um som com o equilíbrio de uma saída de cabeçote (energia
  // caindo com a frequência, de 80 Hz a 6 kHz).
  {
    const int order = juce::jlimit(10, 17, static_cast<int>(std::ceil(std::log2(static_cast<double>(length)))));
    juce::dsp::FFT fft(order);
    std::vector<float> data(static_cast<size_t>(2 << order), 0.0f);
    std::copy(ir.getReadPointer(0), ir.getReadPointer(0) + std::min(length, 1 << order), data.begin());
    fft.performFrequencyOnlyForwardTransform(data.data());
    double weighted = 0.0, reference = 0.0;
    const double binHz = reader->sampleRate / static_cast<double>(1 << order);
    for (int k = 1; k < (1 << order) / 2; ++k)
    {
      const double f = k * binHz;
      if (f < 80.0 || f > 6000.0)
        continue;
      const double w = 1.0 / f;
      weighted += w * static_cast<double>(data[static_cast<size_t>(k)]) * data[static_cast<size_t>(k)];
      reference += w;
    }
    if (weighted > 0.0)
      ir.applyGain(static_cast<float>(std::sqrt(reference / weighted)));
  }
  // O JUCE ajusta a taxa e troca o IR fora do áudio.
  cab.conv.loadImpulseResponse(std::move(ir), reader->sampleRate, juce::dsp::Convolution::Stereo::no, juce::dsp::Convolution::Trim::yes,
                               juce::dsp::Convolution::Normalise::no);
  cab.loaded.store(true);
}

juce::String NexusProcessor::irName(int rig) const
{
  return mCabs[static_cast<size_t>(rig)].file.getFileNameWithoutExtension();
}

void NexusProcessor::timerCallback()
{
  delete mLayoutRetired.exchange(nullptr);
  // A2 Lite/Full muda o tamanho das capturas: recarrega fora do áudio.
  for (int i = 0; i < kSlots; ++i)
  {
    auto& s = mSlots[static_cast<size_t>(i)];
    delete s.retired.exchange(nullptr);
    const bool lite = i < kPedals ? mPedals[static_cast<size_t>(i)].lite->load() > 0.5f : mLite->load() > 0.5f;
    if (lite != s.loadedLite && s.folder.isDirectory())
      loadSlot(i, s.folder);
  }
}

void NexusProcessor::getStateInformation(juce::MemoryBlock& destData)
{
  auto state = params.copyState();
  for (int i = 0; i < kSlots; ++i)
    state.setProperty("pacote_" + juce::String(i), mSlots[static_cast<size_t>(i)].folder.getFullPathName(), nullptr);
  for (int r = 0; r < kRigs; ++r)
    state.setProperty("ir_" + juce::String(r), mCabs[static_cast<size_t>(r)].file.getFullPathName(), nullptr);
  for (const char* child : {"midi", "cadeia"})
    state.removeChild(state.getChildWithName(child), nullptr);
  state.appendChild(mMidi->toValueTree(), nullptr);
  state.appendChild(mLayoutUi.toValueTree(), nullptr);
  if (auto xml = state.createXml())
    copyXmlToBinary(*xml, destData);
}

void NexusProcessor::setStateInformation(const void* data, int sizeInBytes)
{
  if (auto xml = getXmlFromBinary(data, sizeInBytes))
  {
    auto state = juce::ValueTree::fromXml(*xml);
    if (!state.hasType(params.state.getType()))
      return;
    params.replaceState(state);
    mMidi->fromValueTree(state.getChildWithName("midi"));
    setLayout(ChainLayout::fromValueTree(state.getChildWithName("cadeia")));
    for (int i = 0; i < kSlots; ++i)
    {
      const juce::File folder(state.getProperty("pacote_" + juce::String(i)).toString());
      auto& slot = mSlots[static_cast<size_t>(i)];
      if (folder.isDirectory())
      {
        // Mesmo pacote já aberto (troca de preset): não carrega de novo.
        if (folder != slot.folder || !mSlotLoaded[static_cast<size_t>(i)].load())
          loadSlot(i, folder);
      }
      else if (state.hasProperty("pacote_" + juce::String(i)))
        clearSlot(i);
    }
    // Projetos salvos antes da cadeia: um cabeçote de três canais e um pedal.
    const std::pair<const char*, int> old[] = {
      {"pasta", ampSlot(0, 0)}, {"pasta_canal2", ampSlot(0, 1)}, {"pasta_canal3", ampSlot(0, 2)}, {"pasta_pedal", pedalSlot(0)}};
    for (const auto& [key, slot] : old)
    {
      const juce::File folder(state.getProperty(key).toString());
      if (folder.isDirectory() && state.getProperty("pacote_" + juce::String(slot)).toString().isEmpty())
        loadSlot(slot, folder);
    }
    if (const juce::File ir(state.getProperty("ir").toString()); ir.existsAsFile() && state.getProperty("ir_0").toString().isEmpty())
      loadIrAsync(0, ir);
    for (int r = 0; r < kRigs; ++r)
    {
      const juce::File ir(state.getProperty("ir_" + juce::String(r)).toString());
      auto& cab = mCabs[static_cast<size_t>(r)];
      if (ir.existsAsFile())
      {
        if (ir != cab.file || !cab.loaded.load())
          loadIrAsync(r, ir);
      }
      else if (state.hasProperty("ir_" + juce::String(r)))
      {
        cab.loaded.store(false);
        cab.file = juce::File();
      }
    }
  }
}

juce::AudioProcessorEditor* NexusProcessor::createEditor()
{
  return new NexusEditor(*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
  return new NexusProcessor();
}
