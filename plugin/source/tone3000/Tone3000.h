#pragma once

#include <optional>

#include <juce_data_structures/juce_data_structures.h>
#include <juce_events/juce_events.h>

#include <functional>
#include <memory>
#include <vector>

// Conexão com a TONE3000 (www.tone3000.com), de onde vêm as capturas.
//
// Regras do plano gratuito (o nosso, por ser gratuito e de código aberto):
// - login pelo navegador do sistema (OAuth com PKCE); a TONE3000 devolve a
//   resposta para um endereço local do próprio computador;
// - listas da conta (favoritos, baixados) e listas públicas (em alta);
// - carregar um tom pelo número ("load_tone") ou pela tela de escolha da
//   TONE3000 ("select_tone"); as capturas do tom são baixadas logo depois.
// A busca por marca e modelo em todo o catálogo é do plano comercial; por
// isso as "recomendadas" de cada equipamento são uma lista nossa, com o
// número de cada tom (assets/tone3000/recomendadas.json).
//
// O login e o servidor local foram adaptados do plugin oficial da TONE3000
// (tone-3000/tone3000-plugin, licença MIT; ver TERCEIROS.md).
namespace nexus::t3k
{
// Um tom da TONE3000: um pacote com uma ou mais capturas.
struct Tone
{
  int id = 0;
  juce::String title;
  juce::String creator;
  juce::String gear; // amp, amp-cab, pedal, cab...
  int models = 0;    // capturas A2
  int downloads = 0;
  int favorites = 0; // "favoritos" na TONE3000 (o "like" da comunidade)
  // Só nas recomendadas: como achar a posição do botão no nome de cada
  // captura (expressão com um grupo) e os trechos que o nome precisa ter
  // para entrar (a série certa do pacote).
  juce::String pattern;
  juce::StringArray series;
  juce::String note;
  juce::String defaultIr; // caixas: o IR do pacote que entra primeiro
  // Recomendadas sem número no nome: trecho do nome -> posição do botão.
  std::vector<std::pair<juce::String, double>> fixedPositions;
  // Nome da pasta deste pacote, quando o mesmo tom rende vários (um por canal).
  juce::String packName;
  bool isDefault = false; // "padrao" nas recomendadas
  // Escolha automática: sem a posição do botão nos nomes, a primeira captura
  // vai para o meio do botão (em vez da tela de mapear).
  bool firstIfUnnamed = false;
};

// Recomendadas de um equipamento (prefixo do visual: "brit800", "lemon_drive"...),
// e a palavra usada para filtrar as listas da conta ("JCM800").
std::vector<Tone> recommended(const juce::String& gearPrefix);
// Quem responde antes do recomendadas.json embutido (o catálogo, com as
// fichas dos equipamentos baixados): devolve o bloco {"busca", "tons"} ou vazio.
void setRecommendedLookup(std::function<juce::var(const juce::String&)> lookup);
// Captura padrão de um equipamento (a que o Nexus baixa sozinho quando a peça
// está vazia). channel >= 0: a do canal (pacote com "Canal N" no nome); um
// cabeçote sem pacotes por canal usa a primeira no canal 1.
std::optional<Tone> defaultTone(const juce::String& gearPrefix, int channel = -1);
// Pasta já baixada desse tom (Documentos/Nexus Modeler/<equipamento>), ou vazia.
// Pasta de pacote que dá para tocar (capturas com posição ou algum IR).
bool usableFolder(const juce::File& folder);
juce::File downloadedFolder(const Tone& tone, const juce::String& gearName);
juce::String filterWord(const juce::String& gearPrefix);

// Uma captura do tom e a posição do botão que ela representa.
struct Capture
{
  double position = 0.0;
  juce::String name;
  juce::String url;
};
// Escolhe as capturas que formam o pacote: a série certa e uma captura por
// posição do botão. Fica aqui, sem rede, para os testes.
std::vector<Capture> chooseCaptures(const std::vector<std::pair<juce::String, juce::String>>& models, const Tone& tone);

class Connection : private juce::Timer
{
public:
  Connection();
  ~Connection() override;

  enum class List
  {
    favorites,
    downloaded,
    trending,
    mostLiked // busca pela palavra do equipamento, ordenada por favoritos
  };

  // Chave pública do Nexus na TONE3000 (definida na compilação). Sem ela,
  // nada da TONE3000 funciona e a tela oferece só a pasta do computador.
  bool hasKey() const { return mKey.isNotEmpty(); }
  bool signedIn() const;
  juce::String userName() const;
  void signOut();

  // Abre o navegador para entrar na conta. done(erro): vazio deu certo.
  void signIn(std::function<void(juce::String error)> done);

  // Lista da conta ou pública, filtrada por tipo ("amp", "pedal") e palavra.
  void list(List which, const juce::String& gear, const juce::String& word,
            std::function<void(std::vector<Tone>, juce::String error)> done);

  // Baixa um tom para Documentos/Nexus Modeler/<gearName>/<número e nome>,
  // pronto para o motor. Com a conta conectada, baixa direto, sem abrir o
  // site; sem conta, abre o login uma vez. Sem "tone.id", abre a tela de
  // escolha da TONE3000 filtrada por "gearFilter" ("amp_amp-cab", "pedal").
  // Se o Nexus não achar a posição do botão no nome das capturas, baixa todas
  // e marca o pacote para o usuário mapear ("mapear": true no pack.json).
  // progress(texto) informa cada etapa.
  void download(const Tone& tone, const juce::String& gearFilter, const juce::String& gearName,
                std::function<void(juce::String)> progress, std::function<void(juce::File folder, juce::String error)> done);

  // Pasta raiz dos pacotes (Documentos/Nexus Modeler) ou a de um equipamento.
  static juce::File packsFolder(const juce::String& gearName = {});

private:
  class Loopback;
  struct Tokens
  {
    juce::String access, refresh;
    juce::int64 expiresAt = 0;
  };

  void timerCallback() override;
  // Login com uma das telas da TONE3000; done(tone_id devolvido, erro).
  void runFlow(const juce::StringPairArray& extra, std::function<void(int toneId, juce::String error)> done);
  void finishFlow(const juce::String& query);
  // Daqui para baixo, só na linha de trabalho (bloqueiam).
  juce::String accessToken();
  bool exchange(const juce::StringPairArray& form, juce::String& error);
  juce::var getJson(const juce::String& path, juce::String& error, const juce::String& token);
  void saveTokens(const Tokens& t);
  Tokens loadTokens() const;
  void post(std::function<void()> fn);

  juce::String mKey;
  std::unique_ptr<juce::PropertiesFile> mPrefs;
  juce::CriticalSection mPrefsLock;
  juce::ThreadPool mPool{1};
  std::unique_ptr<Loopback> mLoopback;
  juce::String mVerifier, mState, mRedirect;
  std::function<void(int, juce::String)> mFlowDone;
  juce::uint32 mFlowDeadline = 0;

  JUCE_DECLARE_WEAK_REFERENCEABLE(Connection)
  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Connection)
};
} // namespace nexus::t3k
