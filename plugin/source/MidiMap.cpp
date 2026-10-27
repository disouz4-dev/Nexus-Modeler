#include "MidiMap.h"

MidiMap::MidiMap(juce::AudioProcessor& processor)
: mProcessor(processor)
{
  for (auto* p : processor.getParameters())
    mParams.add(p);
  for (auto& t : mCcTarget)
    t.store(-1);
}

int MidiMap::indexOf(const juce::String& paramId) const
{
  for (int i = 0; i < mParams.size(); ++i)
    if (auto* p = dynamic_cast<juce::AudioProcessorParameterWithID*>(mParams[i]); p && p->paramID == paramId)
      return i;
  return -1;
}

void MidiMap::apply(int paramIndex, float normalised)
{
  // Aplica já, no próprio bloco de áudio: o motor lê o valor novo neste
  // mesmo bloco, e a tela e o programa de gravação são avisados.
  mParams[paramIndex]->setValueNotifyingHost(normalised);
}

void MidiMap::process(const juce::MidiBuffer& midi)
{
  for (const auto meta : midi)
  {
    const auto msg = meta.getMessage();
    if (msg.isProgramChange())
    {
      // Troca de patch 1, 2, 3 (números 0, 1, 2 no protocolo) = canal do cabeçote.
      const int program = msg.getProgramChangeNumber();
      const int channel = indexOf("canal");
      if (program < kProgramChannels && channel >= 0)
        apply(channel, static_cast<float>(program) / static_cast<float>(kProgramChannels - 1));
      continue;
    }
    if (!msg.isController())
      continue;
    const int cc = msg.getControllerNumber();
    const int value = msg.getControllerValue();
    if (const int learn = mLearn.exchange(-1); learn >= 0)
    {
      // Um parâmetro só tem um CC: tira o antigo e liga o novo.
      for (auto& t : mCcTarget)
        if (t.load() == learn)
          t.store(-1);
      mCcTarget[static_cast<size_t>(cc)].store(learn);
      mLearned.store(cc);
    }
    const int target = mCcTarget[static_cast<size_t>(cc)].load();
    if (target < 0)
      continue;
    auto* p = mParams[target];
    float norm = static_cast<float>(value) / 127.0f;
    if (p->isBoolean())
      norm = value >= 64 ? 1.0f : 0.0f;
    else if (p->isDiscrete())
    {
      const int steps = std::max(2, p->getNumSteps());
      norm = static_cast<float>(juce::jmin(steps - 1, value * steps / 128)) / static_cast<float>(steps - 1);
    }
    apply(target, norm);
  }
}

void MidiMap::learn(const juce::String& paramId)
{
  mLearn.store(indexOf(paramId));
}

juce::String MidiMap::learningParam() const
{
  const int i = mLearn.load();
  if (i < 0)
    return {};
  return mParams[i]->getName(64);
}

void MidiMap::forget(const juce::String& paramId)
{
  const int i = indexOf(paramId);
  for (auto& t : mCcTarget)
    if (t.load() == i)
      t.store(-1);
}

int MidiMap::ccFor(const juce::String& paramId) const
{
  const int i = indexOf(paramId);
  for (int cc = 0; cc < 128; ++cc)
    if (i >= 0 && mCcTarget[static_cast<size_t>(cc)].load() == i)
      return cc;
  return -1;
}

juce::ValueTree MidiMap::toValueTree() const
{
  juce::ValueTree tree("midi");
  for (int cc = 0; cc < 128; ++cc)
    if (const int t = mCcTarget[static_cast<size_t>(cc)].load(); t >= 0)
      if (auto* p = dynamic_cast<juce::AudioProcessorParameterWithID*>(mParams[t]))
        tree.setProperty("cc" + juce::String(cc), p->paramID, nullptr);
  return tree;
}

void MidiMap::fromValueTree(const juce::ValueTree& tree)
{
  for (auto& t : mCcTarget)
    t.store(-1);
  for (int cc = 0; cc < 128; ++cc)
    if (const auto id = tree.getProperty("cc" + juce::String(cc)).toString(); id.isNotEmpty())
      mCcTarget[static_cast<size_t>(cc)].store(indexOf(id));
}
