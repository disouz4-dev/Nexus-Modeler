#include "PluginProcessor.h"

#include "PluginEditor.h"
#include "nexus/Pack.h"

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
    mCabs[static_cast<size_t>(r)].mode = params.getRawParameterValue(ampParam(r, "caixa"));
  }
  mAb = params.getRawParameterValue("ab");
  mLite = params.getRawParameterValue("lite");
  mOutput = params.getRawParameterValue("volume");
  mInput = params.getRawParameterValue("entrada");

  for (auto& s : mSlots)
    s.status = juce::String::fromUTF8("Nenhum pacote.");
  mLayoutUi = ChainLayout::fromValueTree({});
  mLayoutActive = new ChainLayout(mLayoutUi);

  mMidi = std::make_unique<MidiMap>(*this);
  mResampledFunc = [this](double** in, double** out, int n) {
    std::copy(in[0], in[0] + n, out[0]);
    runEngines(out[0], n);
  };
  startTimerHz(30);
}

NexusProcessor::~NexusProcessor()
{
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
  }
  layout.add(std::make_unique<C>(juce::ParameterID{"ab", 1}, "Pedal A/B", juce::StringArray{"A", "B"}, 0));
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
    layout.add(std::make_unique<C>(juce::ParameterID{pedalParam(i, "visual"), 1}, name + "visual",
                                   juce::StringArray{"Lemon Drive", "Orange Drive", "Banana Overdrive", "Gold Centaur", "Blueberry Chorus",
                                                     "Coconut Delay", "Grape Reverb"}, 0));
  }
  layout.add(std::make_unique<B>(juce::ParameterID{"lite", 1}, juce::String::fromUTF8("A2 Lite (cabeçotes)"), false));
  layout.add(std::make_unique<P>(juce::ParameterID{"volume", 1}, juce::String::fromUTF8("Saída"),
                                 juce::NormalisableRange<float>(-24.0f, 12.0f, 0.1f), 0.0f));
  layout.add(std::make_unique<P>(juce::ParameterID{"entrada", 1}, "Entrada",
                                 juce::NormalisableRange<float>(-24.0f, 24.0f, 0.1f), 0.0f));
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
  const int block = std::max(samplesPerBlock, 32);
  mMono.assign(static_cast<size_t>(block), 0.0);
  mMonoOut.assign(static_cast<size_t>(block), 0.0);
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
  for (auto& p : mPedals)
  {
    p.native.prepare(kEngineRate, kEngineBlock);
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
  // As caixas rodam dentro da cadeia, na taxa das capturas.
  for (auto& c : mCabs)
  {
    c.conv.prepare({kEngineRate, static_cast<juce::uint32>(kEngineBlock), 1});
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

void NexusProcessor::runPedal(int pedal, double* buffer, int n)
{
  // Efeito do próprio Nexus (chorus, delay, reverb): sem captura.
  if (const int look = pedalLook(pedal); isNativeLook(look))
  {
    auto& p = mPedals[static_cast<size_t>(pedal)];
    static constexpr nexus::NativePedal::Type types[] = {nexus::NativePedal::Type::chorus, nexus::NativePedal::Type::delay,
                                                         nexus::NativePedal::Type::reverb};
    const auto type = types[std::min(2, look - kFirstNativeLook)];
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
      current->voice.process(buffer, buffer, n);
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
  // As capturas foram preparadas para pedaços de até kEngineBlock.
  const auto& layout = *mLayoutActive;
  for (int start = 0; start < numFrames; start += kEngineBlock)
  {
    const int n = std::min(kEngineBlock, numFrames - start);
    double* b = buffer + start;
    for (int p : layout.pre)
      runPedal(p, b, n);
    runRigs(b, n);
    for (int p : layout.post)
      runPedal(p, b, n);
  }
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
  // MIDI primeiro: canal, pedais, A/B e botões valem já neste bloco.
  mMidi->process(midiMessages);
  midiMessages.clear();

  // Pacotes novos e cadeia nova: só troca se o anterior já foi recolhido.
  for (auto& s : mSlots)
    if (s.retired.load() == nullptr)
      if (auto* next = s.pending.exchange(nullptr))
      {
        s.retired.store(s.active);
        s.active = next;
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
  for (int i = 0; i < kPedals; ++i)
    if (auto* e = mSlots[static_cast<size_t>(pedalSlot(i))].active)
      e->voice.setControls(pedalControls(i));
  for (int r = 0; r < kRigs; ++r)
    for (int c = 0; c < kChannels; ++c)
      if (auto* e = mSlots[static_cast<size_t>(ampSlot(r, c))].active)
        e->voice.setControls(ampControls(r, c));

  const int total = buffer.getNumSamples();
  const int inChannels = getTotalNumInputChannels();
  const int outChannels = getTotalNumOutputChannels();
  const int capacity = static_cast<int>(mMono.size());
  const float targetGain = juce::Decibels::decibelsToGain(mOutput->load());
  const float targetIn = juce::Decibels::decibelsToGain(mInput->load());
  float inPeak = 0.0f, outPeak = 0.0f;

  for (int start = 0; start < total; start += capacity)
  {
    const int n = std::min(capacity, total - start);
    if (inChannels > 0)
    {
      const float* in = buffer.getReadPointer(0, start);
      const float inStep = (targetIn - mInputGain) / static_cast<float>(n);
      float gi = mInputGain;
      for (int i = 0; i < n; ++i)
      {
        gi += inStep;
        const float v = in[i] * gi;
        inPeak = std::max(inPeak, std::abs(v));
        mMono[static_cast<size_t>(i)] = v;
      }
      mInputGain = targetIn;
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
  }

  // Espelho para a tela.
  mInPeak.store(std::max(inPeak, mInPeak.load()));
  mOutPeak.store(std::max(outPeak, mOutPeak.load()));
  for (int s = 0; s < kSlots; ++s)
    publish(s);
  mUiActiveRig.store(mRigNext >= 0 ? mRigNext : mRigCurrent);
}

void NexusProcessor::setLayout(const ChainLayout& layout)
{
  mLayoutUi = layout;
  delete mLayoutPending.exchange(new ChainLayout(layout));
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
      options.maxBlockSize = kEngineBlock;
      options.slimmableSize = lite ? 0.0 : 1.0;
      auto engine = std::make_unique<NexusEngine>();
      engine->pack = nexus::loadPack(folder.getFullPathName().toStdString(), options);
      engine->voice.setControls(controls);
      engine->voice.prepare(engine->pack, kEngineRate, kEngineBlock);
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
    return;
  auto& cab = mCabs[static_cast<size_t>(rig)];
  cab.file = file;
  // O próprio JUCE lê, ajusta a taxa e troca o IR fora do áudio.
  cab.conv.loadImpulseResponse(file, juce::dsp::Convolution::Stereo::no, juce::dsp::Convolution::Trim::yes, 0,
                               juce::dsp::Convolution::Normalise::yes);
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
      if (folder.isDirectory())
        loadSlot(i, folder);
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
      if (ir.existsAsFile())
        loadIrAsync(r, ir);
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
