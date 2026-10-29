#pragma once

#include <juce_data_structures/juce_data_structures.h>
#include <juce_events/juce_events.h>

#include <atomic>
#include <memory>

// Atualização automática do Nexus Modeler.
//
// As versões saem como "lançamentos" no GitHub (a etiqueta v0.2.0 vira a
// versão 0.2.0, e o fluxo de pacotes anexa os instaladores). Ao abrir, o Nexus
// pergunta ao GitHub qual é o último lançamento; se for mais novo, pergunta se
// a pessoa quer atualizar e deixa o ícone de atualização no topo da tela. Com
// um clique, baixa a versão nova para a pasta Downloads. No Mac (.zip) e no
// Linux instalado pelo terminal (.tar.gz), o próprio Nexus troca os arquivos
// no lugar; no Windows (.exe) e no Linux pelo .deb, quem instala é o
// instalador do sistema.
//
// Um só por processo (juce::SharedResourcePointer): várias janelas do plugin
// mostram o mesmo aviso e não baixam duas vezes.
namespace nexus::update
{
struct Release
{
  juce::String version;   // "0.2.0"
  juce::String notes;     // texto do lançamento
  juce::String page;      // página do lançamento no GitHub
  juce::String assetName; // instalador deste sistema (vazio: só a página)
  juce::String assetUrl;
  juce::int64 assetSize = 0;
};

enum class State
{
  idle,        // ainda não perguntou
  checking,
  upToDate,
  available,
  downloading,
  ready,       // instalador baixado e aberto
  failed,
};

// Versão deste programa ("0.1.0").
juce::String currentVersion();
// Compara versões "1.2.3" (o "v" da frente é ignorado): <0, 0 ou >0.
int compareVersions(const juce::String& a, const juce::String& b);
// Este programa atende à versão mínima pedida por uma peça? As versões de teste
// valem como o número delas: a 0.2.0-beta.1 toca o que pede "0.2.0".
bool meetsMinimum(const juce::String& minimum);
// Lê a resposta do GitHub e escolhe o instalador para o sistema indicado
// ("windows", "mac", "linux" ou "linux-deb"). Fica aqui, sem rede, para os testes.
Release parseRelease(const juce::String& json, const juce::String& system);
// O sistema onde o programa está rodando, no formato acima.
juce::String thisSystem();
#if JUCE_MAC
// Abre o .zip baixado e troca o programa e os plugins onde já estão
// instalados. Devolve o erro (vazio: deu certo).
juce::String installMacZip(const juce::File& zip, const juce::String& version);
#endif

class Updater : public juce::ChangeBroadcaster, private juce::Thread
{
public:
  Updater();
  ~Updater() override;

  // Ao abrir: pergunta toda vez, se a verificação automática está ligada.
  void checkIfDue();
  // Pelo menu: pergunta agora, mesmo com a automática desligada.
  void checkNow();
  // Baixa o instalador e o abre (sem instalador para o sistema: abre a página).
  void downloadAndOpen();
  void openPage() const;
  // "Ignorar esta versão": o aviso só volta numa versão mais nova.
  void skipThisVersion();
  // "Lembrar depois": some até a próxima vez que o Nexus abrir.
  void dismiss();

  bool automatic() const;
  void setAutomatic(bool on);

  State state() const { return mState.load(); }
  // O aviso deve aparecer no topo?
  bool showBadge() const;
  Release release() const;
  float progress() const { return mProgress.load(); }
  juce::String error() const;
  juce::File downloadedFile() const;
  // Mac, ou Linux instalado pela pasta do usuário: a versão nova já foi
  // colocada no lugar e só falta reabrir o Nexus.
  bool installedInPlace() const { return mInPlace.load(); }
  // Abre o programa de novo (já na versão nova) e fecha este.
  static void restartProgram();
  // Verdadeiro uma vez depois de uma verificação pedida pelo menu (para a
  // tela mostrar o resultado mesmo quando não há versão nova).
  bool takeManualResult() { return mManualResult.exchange(false); }
  // Verdadeiro só na primeira vez: a pergunta "quer atualizar?" aparece uma
  // vez por abertura, mesmo com várias janelas do plugin.
  bool takeFirstAsk() { return !mAsked.exchange(true); }
  // Verdadeiro uma vez na primeira abertura de cada versão (depois de
  // atualizar, a tela mostra as novidades). Já anota a versão como vista.
  bool takeNewVersion();

private:
  enum class Job
  {
    none,
    check,
    download,
  };
  void run() override;
  void start(Job job);
  void check();
  void download();
  void setState(State s);
  juce::URL releaseUrl() const;

  std::unique_ptr<juce::PropertiesFile> mPrefs;
  std::atomic<State> mState{State::idle};
  std::atomic<Job> mJob{Job::none};
  std::atomic<float> mProgress{0.0f};
  std::atomic<bool> mManual{false};
  std::atomic<bool> mManualResult{false};
  std::atomic<bool> mDismissed{false};
  std::atomic<bool> mShowSkipped{false};
  std::atomic<bool> mInPlace{false};
  std::atomic<bool> mAsked{false};
  juce::CriticalSection mLock;
  Release mRelease;
  juce::String mError;
  juce::File mFile;
};
} // namespace nexus::update
