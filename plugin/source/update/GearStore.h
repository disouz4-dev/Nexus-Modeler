#pragma once

#include <juce_events/juce_events.h>

#include <atomic>
#include <mutex>

// Equipamentos novos sem reinstalar o Nexus (antes "ArteNova", que baixava só
// a arte). O índice (assets/gear/indice.json no ramo main do GitHub, gerado
// por tools/gear_studio/indice_arte.py) diz, para cada peça, os arquivos com o
// sha256 e o tamanho de cada um, a data, a versão mínima do Nexus e, nas que
// têm ficha, o tipo, o nome, o motor... Ao abrir, o Nexus lê o índice e
// compara com o que tem (o instalado em Equipamentos/, ou o que veio dentro do
// programa). O ícone na coluna de equipamentos (ArteBadge) fica apagado sem
// novidade, acende com a quantidade quando há, e o clique baixa só o que é
// novo ou mudou para Equipamentos/.baixando/<id>, confere o sha256 de cada
// arquivo e instala pelo catálogo (GearCatalog::installFolder). Clique no
// ícone apagado procura de novo. Um equipamento que o programa
// não conhecia entra na lista na hora; o que já existia (pode estar tocando)
// troca na próxima abertura.
namespace nexus::update
{
class GearStore : public juce::ChangeBroadcaster, private juce::Thread
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

  GearStore();
  ~GearStore() override;

  // Pasta dos equipamentos baixados (a do catálogo: Equipamentos/<id>/).
  static juce::File folder();
  // Endereço da pasta do índice: o GitHub ou NEXUS_EQUIPAMENTOS_URL /
  // NEXUS_ARTE_URL (testes; aceita file://).
  static juce::String baseUrl();

  void checkIfDue(); // uma vez por abertura: procura e, se automático, baixa
  // Procura de novo agora (clique no ícone apagado).
  void checkNow();
  // Ligado: baixa sozinho o que achar. Desligado (o padrão): o ícone acende e
  // o download espera o clique.
  void setAutomatic(bool on) { mAutomatic = on; }
  void download();   // tudo o que está no aviso
  // Uma peça só (cartão "não está instalado"): devolve false se o índice não
  // tem a peça ou este Nexus não a toca.
  bool downloadOne(const juce::String& id);

  State state() const { return mState.load(); }
  int count() const { return mCount.load(); }
  juce::StringArray names() const;
  float progress() const { return mProgress.load(); }
  juce::String error() const;
  // Entrada da peça no índice do GitHub (vazio se não leu ou não tem).
  juce::var remoteEntry(const juce::String& id) const;
  // Por que este Nexus não toca a peça do índice (vazio: toca).
  static juce::String unsupported(const juce::var& entry);
  // Baixou arte nova de peça que já existia: só vale ao abrir de novo.
  bool needsRestart() const { return mNeedsRestart.load(); }

private:
  void run() override;
  void check();
  void fetch();
  juce::String fetchOne(const juce::String& id, const juce::var& entry);
  void setState(State s);

  std::atomic<State> mState{State::idle};
  std::atomic<int> mCount{0};
  std::atomic<float> mProgress{0.0f};
  std::atomic<bool> mDownload{false};
  std::atomic<bool> mChecked{false};
  std::atomic<bool> mNeedsRestart{false};
  std::atomic<bool> mAutomatic{false};
  mutable std::mutex mLock;
  juce::var mRemote;           // índice do GitHub
  juce::StringArray mPending;  // peças a baixar
  juce::StringArray mNames;    // nomes das peças, para o aviso
  juce::String mError;
};
} // namespace nexus::update
