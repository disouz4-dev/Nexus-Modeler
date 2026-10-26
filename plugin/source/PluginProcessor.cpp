#include "PluginProcessor.h"

#include "PluginEditor.h"
#include "nexus/Pack.h"

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
  mResampledFunc = [this](double** in, double** out, int n) {
    std::copy(in[0], in[0] + n, out[0]);
    runEngine(out[0], n);
  };
  startTimerHz(30);
}

NexusProcessor::~NexusProcessor()
{
  stopTimer();
  mLoader.removeAllJobs(true, 30000);
  delete mPending.exchange(nullptr);
  delete mRetired.exchange(nullptr);
  delete mActive;
}

juce::AudioProcessorValueTreeState::ParameterLayout NexusProcessor::createLayout()
{
  using P = juce::AudioParameterFloat;
  using B = juce::AudioParameterBool;
  juce::AudioProcessorValueTreeState::ParameterLayout layout;
  layout.add(std::make_unique<P>(juce::ParameterID{"ganho", 1}, "Ganho", juce::NormalisableRange<float>(0.0f, 10.0f, 0.01f), 5.0f));
  layout.add(std::make_unique<P>(juce::ParameterID{"grave", 1}, "Grave", juce::NormalisableRange<float>(0.0f, 10.0f, 0.01f), 5.0f));
  layout.add(std::make_unique<P>(juce::ParameterID{"medio", 1}, juce::String::fromUTF8("Médio"), juce::NormalisableRange<float>(0.0f, 10.0f, 0.01f), 5.0f));
  layout.add(std::make_unique<P>(juce::ParameterID{"agudo", 1}, "Agudo", juce::NormalisableRange<float>(0.0f, 10.0f, 0.01f), 5.0f));
  layout.add(std::make_unique<P>(juce::ParameterID{"presenca", 1}, juce::String::fromUTF8("Presença"), juce::NormalisableRange<float>(0.0f, 10.0f, 0.01f), 5.0f));
  layout.add(std::make_unique<B>(juce::ParameterID{"degraus", 1}, "Degraus", false));
  layout.add(std::make_unique<B>(juce::ParameterID{"lite", 1}, "A2 Lite", false));
  layout.add(std::make_unique<P>(juce::ParameterID{"volume", 1}, "Volume",
                                 juce::NormalisableRange<float>(-24.0f, 12.0f, 0.1f), 0.0f));
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
}

nexus::AmpVoice::Controls NexusProcessor::readControls() const
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

void NexusProcessor::runEngine(double* buffer, int numFrames)
{
  // As capturas foram preparadas para pedaços de até kEngineBlock.
  for (int start = 0; start < numFrames; start += kEngineBlock)
  {
    const int n = std::min(kEngineBlock, numFrames - start);
    mActive->voice.process(buffer + start, buffer + start, n);
  }
}

void NexusProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
  juce::ScopedNoDenormals noDenormals;

  // Equipamento novo esperando? Só troca se o anterior já foi recolhido.
  if (mRetired.load() == nullptr)
    if (auto* next = mPending.exchange(nullptr))
    {
      mRetired.store(mActive);
      mActive = next;
    }

  const int total = buffer.getNumSamples();
  const int inChannels = getTotalNumInputChannels();
  const int outChannels = getTotalNumOutputChannels();
  const int capacity = static_cast<int>(mMono.size());
  const float targetGain = juce::Decibels::decibelsToGain(mOutput->load());

  if (mActive)
    mActive->voice.setControls(readControls());

  for (int start = 0; start < total; start += capacity)
  {
    const int n = std::min(capacity, total - start);
    if (inChannels > 0)
    {
      const float* in = buffer.getReadPointer(0, start);
      for (int i = 0; i < n; ++i)
        mMono[static_cast<size_t>(i)] = in[i];
    }
    else
      std::fill(mMono.begin(), mMono.begin() + n, 0.0);

    if (mActive && mNeedsResampling)
    {
      double* in[1] = {mMono.data()};
      double* out[1] = {mMonoOut.data()};
      mResampler->ProcessBlock(in, out, n, mResampledFunc);
    }
    else
    {
      std::copy(mMono.begin(), mMono.begin() + n, mMonoOut.begin());
      if (mActive)
        runEngine(mMonoOut.data(), n);
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
      }
    }
    mOutputGain = targetGain;
  }

  // Espelho para a tela.
  if (mActive)
  {
    const auto& morph = mActive->voice.morph();
    const int count = std::min(kMaxCaptures, static_cast<int>(mActive->pack->captures.size()));
    for (int i = 0; i < count; ++i)
    {
      mUiWeights[static_cast<size_t>(i)].store(static_cast<float>(morph.weightOf(i)));
      mUiPositions[static_cast<size_t>(i)].store(static_cast<float>(mActive->pack->captures[static_cast<size_t>(i)].position));
    }
    mUiCaptureCount.store(count);
    mUiPosition.store(static_cast<float>(morph.currentPosition()));
  }
}

void NexusProcessor::installEngine(std::unique_ptr<NexusEngine> engine)
{
  delete mPending.exchange(engine.release());
}

void NexusProcessor::loadPackAsync(const juce::File& folder)
{
  mPackFolder = folder;
  const bool lite = mLite->load() > 0.5f;
  mLoadedLite = lite;
  {
    const juce::ScopedLock sl(mStatusLock);
    mStatus = "Carregando " + folder.getFileName() + "...";
  }
  const auto controls = readControls();
  mLoader.addJob([this, folder, lite, controls] {
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
      const juce::String text = juce::String(pack.name) + ": " + juce::String(static_cast<int>(pack.captures.size()))
                                + " capturas, " + juce::String(pack.knobName) + " de "
                                + juce::String(pack.minPosition(), 1) + " a " + juce::String(pack.maxPosition(), 1)
                                + (lite ? " (A2 Lite)" : " (A2 Full)");
      {
        const juce::ScopedLock sl(mStatusLock);
        mStatus = text;
        mKnobName = pack.knobName;
      }
      installEngine(std::move(engine));
    }
    catch (const std::exception& e)
    {
      const juce::ScopedLock sl(mStatusLock);
      mStatus = juce::String("Erro: ") + juce::String::fromUTF8(e.what());
    }
  });
}

juce::String NexusProcessor::statusText() const
{
  const juce::ScopedLock sl(mStatusLock);
  return mStatus;
}

juce::String NexusProcessor::knobName() const
{
  const juce::ScopedLock sl(mStatusLock);
  return mKnobName;
}

void NexusProcessor::timerCallback()
{
  delete mRetired.exchange(nullptr);
  // A2 Lite/Full muda o tamanho das capturas: recarrega fora do áudio.
  const bool lite = mLite->load() > 0.5f;
  if (lite != mLoadedLite && mPackFolder.isDirectory())
    loadPackAsync(mPackFolder);
}

void NexusProcessor::getStateInformation(juce::MemoryBlock& destData)
{
  auto state = params.copyState();
  state.setProperty("pasta", mPackFolder.getFullPathName(), nullptr);
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
        loadPackAsync(folder);
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
