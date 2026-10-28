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

void MidiMap::bind(int control, int paramIndex)
{
  // Um parâmetro só tem um controle: tira o antigo e liga o novo.
  for (auto& t : mCcTarget)
    if (t.load() == paramIndex)
      t.store(-1);
  mCcTarget[static_cast<size_t>(control)].store(paramIndex);
  mLearned.store(control);
}

void MidiMap::press(int target)
{
  auto* p = mParams[target];
  const float now = p->getValue();
  if (p->isDiscrete() && !p->isBoolean() && p->getNumSteps() > 2)
  {
    // Escolha: cada pisada passa para a próxima opção (e volta ao começo).
    const int steps = p->getNumSteps();
    const int next = (juce::roundToInt(now * static_cast<float>(steps - 1)) + 1) % steps;
    apply(target, static_cast<float>(next) / static_cast<float>(steps - 1));
    return;
  }
  apply(target, now >= 0.5f ? 0.0f : 1.0f);
}

namespace
{
// Escolha de banco (CC 0 e CC 32): muitas pedaleiras mandam antes de cada troca
// de patch, igual em todas as pisadas. Não serve para distinguir uma pisada.
bool isBankSelect(const juce::MidiMessage& msg)
{
  return msg.isController() && (msg.getControllerNumber() == 0 || msg.getControllerNumber() == 32);
}

int controlOf(const juce::MidiMessage& msg)
{
  if (msg.isProgramChange())
    return 128 + msg.getProgramChangeNumber();
  if (msg.isNoteOn())
    return 256 + msg.getNoteNumber();
  if (msg.isController() && !isBankSelect(msg))
    return msg.getControllerNumber();
  return -1;
}
} // namespace

void MidiMap::process(const juce::MidiBuffer& midi)
{
  // Aprender: a pisada costuma chegar com mais de uma mensagem no mesmo
  // instante. A troca de patch ou a nota identificam a pisada; o CC só vale
  // quando vem sozinho.
  if (mLearn.load() >= 0)
  {
    int pick = -1;
    for (const auto meta : midi)
    {
      const auto msg = meta.getMessage();
      const int control = controlOf(msg);
      if (control >= 128)
      {
        pick = control;
        break;
      }
      if (control >= 0 && pick < 0)
        pick = control;
    }
    if (pick >= 0)
      if (const int learn = mLearn.exchange(-1); learn >= 0)
        bind(pick, learn);
  }
  for (const auto meta : midi)
  {
    const auto msg = meta.getMessage();
    if (msg.isProgramChange() || msg.isNoteOn())
    {
      const int control = controlOf(msg);
      if (const int target = mCcTarget[static_cast<size_t>(control)].load(); target >= 0)
      {
        press(target);
        continue;
      }
      // PC sem nada aprendido: 1, 2, 3 (0, 1, 2 no protocolo) = canal do cabeçote.
      if (msg.isProgramChange())
      {
        const int program = msg.getProgramChangeNumber();
        const int channel = indexOf("canal");
        if (program < kProgramChannels && channel >= 0)
          apply(channel, static_cast<float>(program) / static_cast<float>(kProgramChannels - 1));
      }
      continue;
    }
    if (!msg.isController() || isBankSelect(msg))
      continue;
    const int cc = msg.getControllerNumber();
    const int value = msg.getControllerValue();
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

juce::String MidiMap::describe(int control)
{
  if (control < 128)
    return "CC " + juce::String(control);
  if (control < 256)
    return "PC " + juce::String(control - 128 + 1); // como as pedaleiras numeram (1 a 128)
  return TRANS("Nota") + " " + juce::String(control - 256);
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
  for (int c = 0; c < kControls; ++c)
    if (i >= 0 && mCcTarget[static_cast<size_t>(c)].load() == i)
      return c;
  return -1;
}

namespace
{
// Nome de cada controle no ajuste salvo: "cc20", "pc3" (0 a 127), "nota60".
juce::String keyOf(int control)
{
  return control < 128 ? "cc" + juce::String(control) : control < 256 ? "pc" + juce::String(control - 128) : "nota" + juce::String(control - 256);
}
} // namespace

juce::ValueTree MidiMap::toValueTree() const
{
  juce::ValueTree tree("midi");
  for (int c = 0; c < kControls; ++c)
    if (const int t = mCcTarget[static_cast<size_t>(c)].load(); t >= 0)
      if (auto* p = dynamic_cast<juce::AudioProcessorParameterWithID*>(mParams[t]))
        tree.setProperty(keyOf(c), p->paramID, nullptr);
  return tree;
}

void MidiMap::fromValueTree(const juce::ValueTree& tree)
{
  for (auto& t : mCcTarget)
    t.store(-1);
  for (int c = 0; c < kControls; ++c)
    if (const auto id = tree.getProperty(keyOf(c)).toString(); id.isNotEmpty())
      mCcTarget[static_cast<size_t>(c)].store(indexOf(id));
}
