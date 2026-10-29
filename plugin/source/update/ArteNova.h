#pragma once

#include <juce_events/juce_events.h>

#include <atomic>
#include <mutex>

// Equipamentos novos sem reinstalar o Nexus. O índice da arte
// (assets/gear/indice.json, gerado por tools/gear_studio/indice_arte.py) diz,
// para cada peça, o hash dos arquivos, a data e a versão mínima do Nexus que a
// entende. Ao abrir, o Nexus lê o índice no GitHub e compara com a arte que
// tem (a baixada antes, na pasta Arte, ou a que veio dentro do programa). O
// que mudou aparece como aviso; o clique baixa só essas peças para a pasta
// Arte, e dali em diante o Nexus usa a cópia local, sem baixar de novo.
namespace nexus::update
{
class ArteNova : public juce::ChangeBroadcaster, private juce::Thread
{
public:
  enum class State
  {
    idle,
    checking,
    upToDate,
    available,
    downloading,
    ready,
    failed
  };

  ArteNova();
  ~ArteNova() override;

  // Pasta da arte baixada (Application Support/Nexus Modeler/Arte no Mac).
  static juce::File folder();
  // Antes de carregar a arte: apaga da pasta o que ficou para trás (peça que
  // veio igual ou mais nova dentro do programa atualizado).
  static void prune();

  void checkIfDue(); // uma vez por abertura
  void download();

  State state() const { return mState.load(); }
  int count() const { return mCount.load(); }
  juce::StringArray names() const;
  float progress() const { return mProgress.load(); }
  juce::String error() const;

private:
  void run() override;
  void check();
  void fetch();
  void setState(State s);

  std::atomic<State> mState{State::idle};
  std::atomic<int> mCount{0};
  std::atomic<float> mProgress{0.0f};
  std::atomic<bool> mDownload{false};
  std::atomic<bool> mChecked{false};
  mutable std::mutex mLock;
  juce::var mRemote;           // índice do GitHub
  juce::StringArray mPending;  // peças a baixar
  juce::StringArray mNames;    // nomes das peças, para o aviso
  juce::String mError;
};
} // namespace nexus::update
