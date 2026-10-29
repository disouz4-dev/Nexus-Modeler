#pragma once

#include <juce_events/juce_events.h>

#include <array>
#include <atomic>
#include <mutex>
#include <optional>
#include <vector>

// Catálogo único dos equipamentos (pedais, cabeçotes e caixas). Cada peça tem
// uma ficha (manifesto): as de fábrica vêm embutidas no programa
// (assets/gear/<id>_manifesto.json, geradas por tools/gear_studio/manifestos.py)
// e as baixadas ficam em Equipamentos/<id>/<id>_manifesto.json, na pasta de
// dados do Nexus. Fábrica e baixado passam pelo mesmo código.
//
// Cada peça ganha uma posição ("look") dentro do seu tipo, fixa enquanto o
// programa está aberto:
//  - as de fábrica ficam na posição antiga dos parâmetros de escolha
//    (p<n>_visual, cabecote, gabinete), que estão congelados para os presets
//    e automações antigos continuarem iguais ("indice_de_fabrica");
//  - as outras (embutidas sem índice e baixadas) entram depois, na ordem em
//    que aparecem. Equipamento baixado nunca entra nos parâmetros: o preset o
//    guarda pelo id em texto (ver NexusProcessor::getStateInformation).
namespace nexus::gear
{
enum class Kind
{
  pedal,
  amp,
  cab
};

// Motores dos pedais, na ordem de NexusProcessor::PedalKind.
inline constexpr const char* kPedalEngines[] = {"captura", "chorus", "delay", "reverb", "gate", "drop", "phaser",
                                                 "flanger", "tremolo", "vibe", "wah", "rotativo", "delay_analogico",
                                                 "oitava", "compressor", "equalizador_grafico", "reverb_modos",
                                                 "delay_modos", "modulacao_modos", "delay_triplo"};
inline constexpr int kPedalEngineCount = static_cast<int>(sizeof(kPedalEngines) / sizeof(kPedalEngines[0]));
// Número do motor do pedal pelo nome da ficha (-1: este Nexus não conhece).
int pedalEngine(const juce::String& name);
// Tipo pelo texto da ficha ("pedal", "cabecote", "caixa").
std::optional<Kind> kindFromText(const juce::String& text);
juce::String kindText(Kind kind);

struct GearEntry
{
  enum class Origin
  {
    builtIn,
    downloaded
  };
  juce::String id; // "lemon_drive": nunca muda, é o que vai nos presets
  Kind kind = Kind::pedal;
  juce::String name, inspiration, category, source;
  juce::String engine;        // "captura", "chorus"...; caixa: "caixa"
  int engineCode = 0;         // pedais: número do motor (kPedalEngines)
  juce::var tuning;           // "regulagem"
  juce::var captures;         // "capturas" (origem e recomendadas)
  juce::String cabId;         // cabeçote: caixa que abre junto
  bool graphicEq = false;     // cabeçote com o equalizador gráfico de 5 faixas
  juce::String version, minimumVersion;
  Origin origin = Origin::builtIn;
  int builtInIndex = -1;      // posição nos parâmetros antigos (só fábrica)
  juce::File folder;          // pasta da peça baixada (vazia nas embutidas)
  int look = -1;              // posição no catálogo do tipo

  // Bloco das capturas recomendadas da própria ficha (vazio: vale o
  // recomendadas.json embutido).
  juce::var recommended() const { return captures["recomendadas"]; }
};

class GearCatalog : public juce::ChangeBroadcaster
{
public:
  static constexpr int kMaxLooks = 1024;

  // Um catálogo por processo (vários Nexus abertos no mesmo programa de
  // gravação dividem a leitura do disco). Abre as fichas embutidas e as da
  // pasta Equipamentos na primeira chamada.
  static GearCatalog& instance();

  // Pasta dos equipamentos baixados (Application Support/Nexus Modeler/
  // Equipamentos no Mac). Muda com NEXUS_EQUIPAMENTOS_PASTA (testes).
  static juce::File folder();
  // Lê uma ficha (texto JSON). Devolve vazio se faltar campo obrigatório.
  static std::optional<GearEntry> parse(const juce::var& manifest);
  // Motivo para este Nexus não tocar a ficha (vazio: toca): motor
  // desconhecido ou versão mínima maior que a deste programa.
  static juce::String unsupported(const juce::var& manifest);

  // Leitura em qualquer linha, inclusive a do áudio (sem trava).
  int count(Kind kind) const { return mCount[static_cast<size_t>(kind)].load(); }
  int pedalEngineAt(int look) const;
  bool graphicEqAt(int look) const;

  // Linha principal.
  std::vector<GearEntry> entries(Kind kind) const;
  std::optional<GearEntry> entry(Kind kind, int look) const;
  int find(Kind kind, const juce::String& id) const; // -1: não está instalado
  juce::String idOf(Kind kind, int look) const;
  // Parâmetro de fábrica que fica no lugar de uma peça que não é de fábrica:
  // o primeiro de fábrica com o mesmo motor (programa de gravação velho ainda
  // toca algo parecido).
  int fallbackFor(Kind kind, int look) const;
  // Relê a pasta Equipamentos: peças novas entram no fim (as posições das que
  // já estavam não mudam). Avisa os ouvintes se entrou alguma.
  void reload();

private:
  GearCatalog();
  void loadEmbedded();
  void add(GearEntry e);

  mutable std::mutex mLock;
  std::array<std::vector<GearEntry>, 3> mEntries;
  std::array<std::atomic<int>, 3> mCount{};
  std::array<std::atomic<int>, kMaxLooks> mPedalEngine{};
  std::array<std::atomic<bool>, kMaxLooks> mGraphicEq{};
};
} // namespace nexus::gear
