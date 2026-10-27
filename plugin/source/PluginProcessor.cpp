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
  mSlots[amp].status = juce::String::fromUTF8("Nenhum pacote carregado.");
  mSlots[pedal].status = juce::String::fromUTF8("Nenhum pacote no pedal.");
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
  layout.add(std::make_unique<P>(juce::ParameterID{"ganho", 1}, "Ganho", knob, 5.0f));
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

nexus::AmpVoice::Controls NexusProcessor::ampControls() const
{
  nexus::AmpVoice::Controls c;
  c.main = mMain->load();
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

void NexusProcessor::runEngines(double* buffer, int numFrames)
{
  // As capturas foram preparadas para pedaços de até kEngineBlock.
  for (int start = 0; start < numFrames; start += kEngineBlock)
  {
    const int n = std::min(kEngineBlock, numFrames - start);
    if (mSlots[pedal].active)
      runPedal(buffer + start, n);
    if (mSlots[amp].active)
      mSlots[amp].active->voice.process(buffer + start, buffer + start, n);
  }
}

void NexusProcessor::publish(Block b)
{
  auto* engine = mSlots[b].active;
  auto& ui = mUi[b];
  if (!engine)
    return;
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

void NexusProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
  juce::ScopedNoDenormals noDenormals;

  // Equipamento novo esperando? Só troca se o anterior já foi recolhido.
  for (auto& s : mSlots)
    if (s.retired.load() == nullptr)
      if (auto* next = s.pending.exchange(nullptr))
      {
        s.retired.store(s.active);
        s.active = next;
      }

  auto* ampEngine = mSlots[amp].active;
  const bool anyEngine = ampEngine != nullptr || mSlots[pedal].active != nullptr;
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

  if (ampEngine)
    ampEngine->voice.setControls(ampControls());
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

void NexusProcessor::loadPackAsync(const juce::File& folder, Block b)
{
  auto& slot = mSlots[b];
  slot.folder = folder;
  const bool lite = mLite->load() > 0.5f;
  slot.loadedLite = lite;
  {
    const juce::ScopedLock sl(mStatusLock);
    slot.status = "Carregando " + folder.getFileName() + "...";
  }
  const auto controls = b == amp ? ampControls() : pedalControls();
  mLoader.addJob([this, folder, lite, controls, b] {
    auto& s = mSlots[b];
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
        mPackIncludesCab.store(pack.includesCab);
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
  return mSlots[b].status;
}

juce::String NexusProcessor::packName(Block b) const
{
  const juce::ScopedLock sl(mStatusLock);
  return mSlots[b].name;
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
  for (int b = 0; b < kBlocks; ++b)
  {
    auto& s = mSlots[static_cast<size_t>(b)];
    delete s.retired.exchange(nullptr);
    if (lite != s.loadedLite && s.folder.isDirectory())
      loadPackAsync(s.folder, static_cast<Block>(b));
  }
}

void NexusProcessor::getStateInformation(juce::MemoryBlock& destData)
{
  auto state = params.copyState();
  state.setProperty("pasta", mSlots[amp].folder.getFullPathName(), nullptr);
  state.setProperty("pasta_pedal", mSlots[pedal].folder.getFullPathName(), nullptr);
  state.setProperty("ir", mIrFile.getFullPathName(), nullptr);
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
      const juce::File folder(state.getProperty("pasta").toString());
      if (folder.isDirectory())
        loadPackAsync(folder, amp);
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
