#include "PluginProcessor.h"

#include "PluginEditor.h"
#include "nexus/Pack.h"

namespace
{
// Pedal liga e desliga em 20 ms, sem estalo.
constexpr double kPedalFadeSeconds = 0.02;

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
  mMain = params.getRawParameterValue("ganho");
  mBass = params.getRawParameterValue("grave");
  mMid = params.getRawParameterValue("medio");
  mTreble = params.getRawParameterValue("agudo");
  mPresence = params.getRawParameterValue("presenca");
  mStepped = params.getRawParameterValue("degraus");
  mLite = params.getRawParameterValue("lite");
  mOutput = params.getRawParameterValue("volume");
  mInput = params.getRawParameterValue("entrada");
  mMaster = params.getRawParameterValue("master");
  mCabMode = params.getRawParameterValue("caixa");
  mPedalGain = params.getRawParameterValue("pedal_ganho");
  mPedalTone = params.getRawParameterValue("pedal_tom");
  mPedalLevel = params.getRawParameterValue("pedal_nivel");
  mPedalOn = params.getRawParameterValue("pedal_ligado");
  mChannelGain = {mMain, params.getRawParameterValue("ganho_2"), params.getRawParameterValue("ganho_3")};
  mChannel = params.getRawParameterValue("canal");
  for (int c = 0; c < kChannels; ++c)
    mSlots[static_cast<size_t>(1 + c)].status = juce::String::fromUTF8("Nenhum pacote neste canal.");
  mSlots[pedal].status = juce::String::fromUTF8("Nenhum pacote no pedal.");
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
}

juce::AudioProcessorValueTreeState::ParameterLayout NexusProcessor::createLayout()
{
  using P = juce::AudioParameterFloat;
  using B = juce::AudioParameterBool;
  const juce::NormalisableRange<float> knob(0.0f, 10.0f, 0.01f);
  juce::AudioProcessorValueTreeState::ParameterLayout layout;
  layout.add(std::make_unique<P>(juce::ParameterID{"ganho", 1}, "Ganho (canal 1)", knob, 5.0f));
  layout.add(std::make_unique<P>(juce::ParameterID{"ganho_2", 1}, "Ganho (canal 2)", knob, 5.0f));
  layout.add(std::make_unique<P>(juce::ParameterID{"ganho_3", 1}, "Ganho (canal 3)", knob, 5.0f));
  layout.add(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{"canal", 1}, "Canal do amplificador",
                                                          juce::StringArray{"Canal 1", "Canal 2", "Canal 3"}, 0));
  layout.add(std::make_unique<P>(juce::ParameterID{"grave", 1}, "Grave", knob, 5.0f));
  layout.add(std::make_unique<P>(juce::ParameterID{"medio", 1}, juce::String::fromUTF8("Médio"), knob, 5.0f));
  layout.add(std::make_unique<P>(juce::ParameterID{"agudo", 1}, "Agudo", knob, 5.0f));
  layout.add(std::make_unique<P>(juce::ParameterID{"presenca", 1}, juce::String::fromUTF8("Presença"), knob, 5.0f));
  layout.add(std::make_unique<B>(juce::ParameterID{"degraus", 1}, "Degraus", false));
  layout.add(std::make_unique<B>(juce::ParameterID{"lite", 1}, "A2 Lite", false));
  layout.add(std::make_unique<P>(juce::ParameterID{"volume", 1}, juce::String::fromUTF8("Saída"),
                                 juce::NormalisableRange<float>(-24.0f, 12.0f, 0.1f), 0.0f));
  layout.add(std::make_unique<P>(juce::ParameterID{"master", 1}, "Volume do amplificador", knob, 5.0f));
  layout.add(std::make_unique<P>(juce::ParameterID{"entrada", 1}, "Entrada",
                                 juce::NormalisableRange<float>(-24.0f, 24.0f, 0.1f), 0.0f));
  layout.add(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{"caixa", 1}, "Caixa",
                                                          juce::StringArray{"Desligada", juce::String::fromUTF8("IR próprio")}, 1));
  layout.add(std::make_unique<P>(juce::ParameterID{"pedal_ganho", 1}, "Pedal: ganho", knob, 5.0f));
  layout.add(std::make_unique<P>(juce::ParameterID{"pedal_tom", 1}, "Pedal: tom", knob, 5.0f));
  layout.add(std::make_unique<P>(juce::ParameterID{"pedal_nivel", 1}, juce::String::fromUTF8("Pedal: nível"), knob, 5.0f));
  layout.add(std::make_unique<B>(juce::ParameterID{"pedal_ligado", 1}, "Pedal ligado", true));
  layout.add(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{"pedal_modelo", 1}, "Pedal: visual",
                                                          juce::StringArray{"Verde Drive", "Laranja Dist", "Ouro"}, 0));
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
  mDry.assign(static_cast<size_t>(kEngineBlock), 0.0);
  mChannelIn.assign(static_cast<size_t>(kEngineBlock), 0.0);
  mChannelOut.assign(static_cast<size_t>(kEngineBlock), 0.0);
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
  const float master = mMaster->load() / 5.0f;
  mOutputGain = juce::Decibels::decibelsToGain(mOutput->load()) * master * master;
  mInputGain = juce::Decibels::decibelsToGain(mInput->load());
  mPedalMix = mPedalOn->load() > 0.5f ? 1.0f : 0.0f;
  const float lvl = mPedalLevel->load() / 5.0f;
  mPedalLevelGain = lvl * lvl;
  mCabBuffer.setSize(1, block);
  mCab.prepare({sampleRate, static_cast<juce::uint32>(block), 1});
  mCabMix.reset(sampleRate, 0.03);
  mCabMix.setCurrentAndTargetValue(0.0f);
}

int NexusProcessor::slotIndex(Block b, int channel) const
{
  if (b == pedal)
    return 0;
  return 1 + juce::jlimit(0, kChannels - 1, channel < 0 ? selectedChannel() : channel);
}

int NexusProcessor::selectedChannel() const
{
  return juce::jlimit(0, kChannels - 1, juce::roundToInt(mChannel->load()));
}

nexus::AmpVoice::Controls NexusProcessor::ampControls(int channel) const
{
  nexus::AmpVoice::Controls c;
  c.main = mChannelGain[static_cast<size_t>(channel)]->load();
  c.tone.bass = mBass->load();
  c.tone.mid = mMid->load();
  c.tone.treble = mTreble->load();
  c.presence = mPresence->load();
  c.stepped = mStepped->load() > 0.5f;
  return c;
}

nexus::AmpVoice::Controls NexusProcessor::pedalControls() const
{
  // O pedal usa o mesmo motor: o ganho troca as capturas; o "tom" é uma
  // prateleira de agudos relativa à posição 5 (onde o pedal foi capturado).
  nexus::AmpVoice::Controls c;
  c.main = mPedalGain->load();
  c.presence = mPedalTone->load();
  c.stepped = mStepped->load() > 0.5f;
  return c;
}

void NexusProcessor::runPedal(double* buffer, int n)
{
  auto* engine = mSlots[pedal].active;
  std::copy(buffer, buffer + n, mDry.begin());
  engine->voice.process(buffer, buffer, n);
  const float targetMix = mPedalOn->load() > 0.5f ? 1.0f : 0.0f;
  const float mixStep = static_cast<float>(1.0 / (kPedalFadeSeconds * kEngineRate));
  const float lvl = mPedalLevel->load() / 5.0f;
  const float levelStep = (lvl * lvl - mPedalLevelGain) / static_cast<float>(n);
  for (int i = 0; i < n; ++i)
  {
    if (mPedalMix < targetMix)
      mPedalMix = std::min(targetMix, mPedalMix + mixStep);
    else if (mPedalMix > targetMix)
      mPedalMix = std::max(targetMix, mPedalMix - mixStep);
    mPedalLevelGain += levelStep;
    const double wet = buffer[i] * mPedalLevelGain;
    buffer[i] = mDry[static_cast<size_t>(i)] * (1.0 - mPedalMix) + wet * mPedalMix;
  }
}

void NexusProcessor::runAmp(double* buffer, int n)
{
  // Canal pedido; se ele não tem pacote, fica no que está tocando.
  const int wanted = 1 + selectedChannel();
  if (mAmpCurrent < 0 || mSlots[static_cast<size_t>(mAmpCurrent)].active == nullptr)
  {
    mAmpCurrent = -1;
    for (int c = 0; c < kChannels && mAmpCurrent < 0; ++c)
    {
      const int slot = c == 0 ? wanted : 1 + (wanted - 1 + c) % kChannels;
      if (mSlots[static_cast<size_t>(slot)].active)
        mAmpCurrent = slot;
    }
    mAmpNext = -1;
  }
  if (mAmpCurrent < 0)
    return;
  if (wanted != mAmpCurrent && wanted != mAmpNext && mSlots[static_cast<size_t>(wanted)].active)
  {
    mAmpNext = wanted;
    int warm = 1024;
    for (const auto& c : mSlots[static_cast<size_t>(wanted)].active->pack->captures)
      warm = std::max(warm, c.warmupSamples);
    mAmpWarmLeft = warm;
    mAmpFade = 0.0f;
  }
  if (mAmpNext >= 0 && mSlots[static_cast<size_t>(mAmpNext)].active == nullptr)
    mAmpNext = -1;

  auto* current = mSlots[static_cast<size_t>(mAmpCurrent)].active;
  if (mAmpNext < 0)
  {
    current->voice.process(buffer, buffer, n);
    return;
  }
  auto* next = mSlots[static_cast<size_t>(mAmpNext)].active;
  std::copy(buffer, buffer + n, mChannelIn.begin());
  current->voice.process(buffer, buffer, n);
  next->voice.process(mChannelIn.data(), mChannelOut.data(), n);
  if (mAmpWarmLeft > 0)
  {
    // O canal que entra ainda está ouvindo a guitarra para valer.
    mAmpWarmLeft -= n;
    return;
  }
  const float step = static_cast<float>(1.0 / (0.03 * kEngineRate));
  for (int i = 0; i < n; ++i)
  {
    mAmpFade = std::min(1.0f, mAmpFade + step);
    buffer[i] = buffer[i] * (1.0 - mAmpFade) + mChannelOut[static_cast<size_t>(i)] * mAmpFade;
  }
  if (mAmpFade >= 1.0f)
  {
    mAmpCurrent = mAmpNext;
    mAmpNext = -1;
  }
}

void NexusProcessor::runEngines(double* buffer, int numFrames)
{
  // As capturas foram preparadas para pedaços de até kEngineBlock.
  for (int start = 0; start < numFrames; start += kEngineBlock)
  {
    const int n = std::min(kEngineBlock, numFrames - start);
    if (mSlots[pedal].active)
      runPedal(buffer + start, n);
    runAmp(buffer + start, n);
  }
}

void NexusProcessor::publish(Block b)
{
  const int slot = b == pedal ? 0 : (mAmpCurrent >= 0 ? mAmpCurrent : slotIndex(amp, -1));
  auto* engine = mSlots[static_cast<size_t>(slot)].active;
  auto& ui = mUi[b];
  if (!engine)
  {
    ui.count.store(0);
    return;
  }
  if (b == amp)
    mAudibleChannel.store(slot - 1);
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
  // MIDI primeiro: troca de canal, pedal e botões valem já neste bloco.
  mMidi->process(midiMessages);
  midiMessages.clear();

  // Equipamento novo esperando? Só troca se o anterior já foi recolhido.
  for (auto& s : mSlots)
    if (s.retired.load() == nullptr)
      if (auto* next = s.pending.exchange(nullptr))
      {
        s.retired.store(s.active);
        s.active = next;
      }

  bool anyEngine = false;
  for (auto& s : mSlots)
    anyEngine = anyEngine || s.active != nullptr;
  auto* ampEngine = mAmpCurrent >= 0 ? mSlots[static_cast<size_t>(mAmpCurrent)].active : nullptr;
  const int total = buffer.getNumSamples();
  const int inChannels = getTotalNumInputChannels();
  const int outChannels = getTotalNumOutputChannels();
  const int capacity = static_cast<int>(mMono.size());
  // Volume do painel: 5 = como a captura soa, 10 = +12 dB, 0 = mudo.
  const float master = mMaster->load() / 5.0f;
  const float targetGain = juce::Decibels::decibelsToGain(mOutput->load()) * master * master;
  const float targetIn = juce::Decibels::decibelsToGain(mInput->load());
  // Caixa: só toca com IR carregado, modo "IR próprio" e captura sem caixa.
  const bool cabOn = mIrLoaded.load() && mCabMode->load() > 0.5f && !(ampEngine && ampEngine->pack->includesCab);
  mCabMix.setTargetValue(cabOn ? 1.0f : 0.0f);
  float inPeak = 0.0f, outPeak = 0.0f;

  for (int c = 0; c < kChannels; ++c)
    if (auto* e = mSlots[static_cast<size_t>(1 + c)].active)
      e->voice.setControls(ampControls(c));
  if (auto* p = mSlots[pedal].active)
    p->voice.setControls(pedalControls());

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

    // Caixa: o IR roda sempre que existe e a mistura entra e sai em 30 ms,
    // para ligar, desligar ou trancar sem estalo.
    float* mono = mCabBuffer.getWritePointer(0);
    for (int i = 0; i < n; ++i)
      mono[i] = static_cast<float>(mMonoOut[static_cast<size_t>(i)]);
    if (mIrLoaded.load() && (cabOn || mCabMix.isSmoothing() || mCabMix.getCurrentValue() > 0.0f))
    {
      float* wet = mCabBuffer.getWritePointer(0);
      juce::dsp::AudioBlock<float> block(&wet, 1, static_cast<size_t>(n));
      mCab.process(juce::dsp::ProcessContextReplacing<float>(block));
      for (int i = 0; i < n; ++i)
      {
        const float m = mCabMix.getNextValue();
        mono[i] = static_cast<float>(mMonoOut[static_cast<size_t>(i)]) * (1.0f - m) + wet[i] * m;
      }
    }
    else
      mCabMix.skip(n);

    const float step = (targetGain - mOutputGain) / static_cast<float>(n);
    for (int ch = 0; ch < outChannels; ++ch)
    {
      float* out = buffer.getWritePointer(ch, start);
      float g = mOutputGain;
      for (int i = 0; i < n; ++i)
      {
        g += step;
        out[i] = mono[i] * g;
        if (ch == 0)
          outPeak = std::max(outPeak, std::abs(out[i]));
      }
    }
    mOutputGain = targetGain;
  }

  // Espelho para a tela.
  mInPeak.store(std::max(inPeak, mInPeak.load()));
  mOutPeak.store(std::max(outPeak, mOutPeak.load()));
  publish(amp);
  publish(pedal);
}

void NexusProcessor::loadPackAsync(const juce::File& folder, Block b, int channel)
{
  const int index = slotIndex(b, channel);
  auto& slot = mSlots[static_cast<size_t>(index)];
  slot.folder = folder;
  const bool lite = mLite->load() > 0.5f;
  slot.loadedLite = lite;
  {
    const juce::ScopedLock sl(mStatusLock);
    slot.status = "Carregando " + folder.getFileName() + "...";
  }
  const auto controls = b == amp ? ampControls(index - 1) : pedalControls();
  mLoader.addJob([this, folder, lite, controls, b, index] {
    auto& s = mSlots[static_cast<size_t>(index)];
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
                                + juce::String(static_cast<int>(pack.captures.size())) + " capturas, "
                                + juce::String::fromUTF8(pack.knobName.c_str()) + " de " + commaDecimal(pack.minPosition())
                                + " a " + commaDecimal(pack.maxPosition()) + (lite ? " (A2 Lite)" : " (A2 Full)");
      {
        const juce::ScopedLock sl(mStatusLock);
        s.status = text;
        s.name = juce::String::fromUTF8(pack.name.c_str());
        if (b == amp)
          mKnobName = juce::String::fromUTF8(pack.knobName.c_str());
      }
      if (b == amp)
      {
        mChannelIncludesCab[static_cast<size_t>(index - 1)].store(pack.includesCab);
        mChannelLoaded[static_cast<size_t>(index - 1)].store(true);
      }
      delete s.pending.exchange(engine.release());
    }
    catch (const std::exception& e)
    {
      const juce::ScopedLock sl(mStatusLock);
      s.status = juce::String("Erro: ") + juce::String::fromUTF8(e.what());
    }
  });
}

juce::String NexusProcessor::statusText(Block b) const
{
  const juce::ScopedLock sl(mStatusLock);
  return mSlots[static_cast<size_t>(slotIndex(b, -1))].status;
}

juce::String NexusProcessor::packName(Block b) const
{
  const juce::ScopedLock sl(mStatusLock);
  return mSlots[static_cast<size_t>(slotIndex(b, -1))].name;
}

juce::String NexusProcessor::channelPackName(int channel) const
{
  const juce::ScopedLock sl(mStatusLock);
  return mSlots[static_cast<size_t>(slotIndex(amp, channel))].name;
}

juce::String NexusProcessor::knobName() const
{
  const juce::ScopedLock sl(mStatusLock);
  return mKnobName;
}

void NexusProcessor::loadIrAsync(const juce::File& file)
{
  if (!file.existsAsFile())
    return;
  mIrFile = file;
  // O próprio JUCE lê, ajusta a taxa e troca o IR fora do áudio.
  mCab.loadImpulseResponse(file, juce::dsp::Convolution::Stereo::no, juce::dsp::Convolution::Trim::yes, 0,
                           juce::dsp::Convolution::Normalise::yes);
  mIrLoaded.store(true);
}

juce::String NexusProcessor::irName() const
{
  return mIrFile.getFileNameWithoutExtension();
}

void NexusProcessor::timerCallback()
{
  // A2 Lite/Full muda o tamanho das capturas: recarrega fora do áudio.
  const bool lite = mLite->load() > 0.5f;
  for (int i = 0; i < kSlots; ++i)
  {
    auto& s = mSlots[static_cast<size_t>(i)];
    delete s.retired.exchange(nullptr);
    if (lite != s.loadedLite && s.folder.isDirectory())
      loadPackAsync(s.folder, i == 0 ? pedal : amp, i - 1);
  }
}

void NexusProcessor::getStateInformation(juce::MemoryBlock& destData)
{
  auto state = params.copyState();
  state.setProperty("pasta", mSlots[1].folder.getFullPathName(), nullptr);
  state.setProperty("pasta_canal2", mSlots[2].folder.getFullPathName(), nullptr);
  state.setProperty("pasta_canal3", mSlots[3].folder.getFullPathName(), nullptr);
  state.setProperty("pasta_pedal", mSlots[pedal].folder.getFullPathName(), nullptr);
  state.setProperty("ir", mIrFile.getFullPathName(), nullptr);
  state.removeChild(state.getChildWithName("midi"), nullptr);
  state.appendChild(mMidi->toValueTree(), nullptr);
  if (auto xml = state.createXml())
    copyXmlToBinary(*xml, destData);
}

void NexusProcessor::setStateInformation(const void* data, int sizeInBytes)
{
  if (auto xml = getXmlFromBinary(data, sizeInBytes))
  {
    auto state = juce::ValueTree::fromXml(*xml);
    if (state.hasType(params.state.getType()))
    {
      params.replaceState(state);
      mMidi->fromValueTree(state.getChildWithName("midi"));
      const char* channelKeys[kChannels] = {"pasta", "pasta_canal2", "pasta_canal3"};
      for (int c = 0; c < kChannels; ++c)
      {
        const juce::File folder(state.getProperty(channelKeys[c]).toString());
        if (folder.isDirectory())
          loadPackAsync(folder, amp, c);
      }
      const juce::File pedalFolder(state.getProperty("pasta_pedal").toString());
      if (pedalFolder.isDirectory())
        loadPackAsync(pedalFolder, pedal);
      const juce::File ir(state.getProperty("ir").toString());
      if (ir.existsAsFile())
        loadIrAsync(ir);
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
