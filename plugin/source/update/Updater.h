#pragma once

#include <juce_data_structures/juce_data_structures.h>
#include <juce_events/juce_events.h>

#include <atomic>
#include <memory>

// Atualização automática do Nexus Modeler.
//
// As versões saem como "lançamentos" no GitHub (a etiqueta v0.2.0 vira a
// versão 0.2.0, e o fluxo de pacotes anexa os instaladores). Ao abrir, o Nexus
// pergunta ao GitHub qual é o último lançamento, no máximo uma vez por dia;
// se for mais novo, mostra o aviso no topo da tela. Com um clique, baixa o
// instalador do sistema (Windows .exe, Mac .dmg, Linux .deb ou .tar.gz) para a
// pasta Downloads e o abre. A instalação em si é a do instalador: um plugin
// aberto num programa de música não consegue trocar o próprio arquivo.
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
// Lê a resposta do GitHub e escolhe o instalador para o sistema indicado
// ("windows", "mac", "linux" ou "linux-deb"). Fica aqui, sem rede, para os testes.
Release parseRelease(const juce::String& json, const juce::String& system);
// O sistema onde o programa está rodando, no formato acima.
juce::String thisSystem();

class Updater : public juce::ChangeBroadcaster, private juce::Thread
{
public:
  Updater();
  ~Updater() override;

  // Ao abrir: só pergunta se a verificação automática está ligada e se já
  // passou um dia da última vez.
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
  // Linux instalado pela pasta do usuário: a versão nova já foi colocada no
  // lugar e só falta reabrir o Nexus.
  bool installedInPlace() const { return mInPlace.load(); }
  // Abre o programa de novo (já na versão nova) e fecha este.
  static void restartProgram();
  // Verdadeiro uma vez depois de uma verificação pedida pelo menu (para a
  // tela mostrar o resultado mesmo quando não há versão nova).
  bool takeManualResult() { return mManualResult.exchange(false); }

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
  juce::CriticalSection mLock;
  Release mRelease;
  juce::String mError;
  juce::File mFile;
};
} // namespace nexus::update
