#pragma once

#include <filesystem>
#include <memory>
#include <string>
#include <vector>

#include "NAM/dsp.h"
#include "nexus/ToneStack.h"

namespace nexus
{

// Uma captura do pacote, já carregada e preparada.
struct Capture
{
  double position = 0.0;        // posição do botão principal (ex.: ganho 7)
  std::string name;             // nome do arquivo, para mostrar na tela
  std::filesystem::path file;
  std::unique_ptr<nam::DSP> dsp;
  double levelGain = 1.0;       // ganho de volume aplicado na saída
  double measuredRmsDb = 0.0;   // volume medido no carregamento
  int warmupSamples = 0;        // som que a captura precisa ouvir antes de valer
};

enum class LevelMode
{
  // Mantém a diferença de volume entre as capturas (como o criador gravou) e
  // só ajusta o pacote inteiro para um volume de referência.
  KeepRelative,
  // Iguala o volume de todas as capturas.
  EqualizeEach,
};

struct PackOptions
{
  double sampleRate = 48000.0;
  int maxBlockSize = 512;
  double slimmableSize = 1.0; // 1 = A2 Full, 0 = A2 Lite
  LevelMode levelMode = LevelMode::KeepRelative;
  double targetRmsDb = -18.0;
};

// Equipamento pronto para tocar: capturas ordenadas pela posição do botão,
// e o equalizador do painel da família.
struct Pack
{
  std::string name;
  std::string knobName = "Ganho";
  std::string toneCircuit = "marshall";
  ToneKnobs captureTone{};      // posição do equalizador quando foi capturado
  double capturePresence = 5.0;
  // Captura "full rig" (cabeçote + caixa): a caixa já está no som, então o
  // bloco da caixa fica trancado e não aceita IR.
  bool includesCab = false;
  std::vector<Capture> captures; // ordenadas por position, sem repetição

  double minPosition() const { return captures.front().position; }
  double maxPosition() const { return captures.back().position; }
};

// Lê um pacote de uma pasta. Se houver "pack.json", ele manda; senão o
// número de cada captura vem do nome do arquivo ("Gain 7.nam" -> 7).
// Faz tudo o que é pesado (abrir, preparar, medir); nunca chamar no áudio.
// Lança std::runtime_error com mensagem em português.
std::unique_ptr<Pack> loadPack(const std::filesystem::path& folder, const PackOptions& options);

// Extrai a posição do nome do arquivo. Prefere o número depois de
// "gain", "ganho", "drive", "g"; senão o último número do nome.
// Retorna false se não houver número.
bool positionFromFileName(const std::string& stem, double& position);

// Diz se a captura inclui a caixa, pelo tipo de equipamento gravado no .nam
// ("amp_cab", "amp_pedal_cab", "full_rig"...). Sem informação: false.
bool captureIncludesCab(const std::filesystem::path& namFile);

} // namespace nexus
