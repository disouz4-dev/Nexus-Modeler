#pragma once

#include <array>
#include <atomic>
#include <functional>
#include <memory>
#include <string>
#include <vector>

// Separação de faixas do estúdio: uma música vira seis faixas (bateria,
// baixo, resto, voz, guitarra e piano) pelo Demucs de 6 faixas
// (htdemucs_6s, da Meta) no demucs.cpp (C++ puro, licença MIT).
//
// Roda fora do tempo real: a música é dividida em pedaços que vão para
// vários núcleos do processador ao mesmo tempo e depois são emendados com
// sobreposição. Não depende do JUCE.
namespace nexus::studio
{
// Na ordem em que o modelo devolve.
enum Stem
{
  drums,
  bass,
  other,
  vocals,
  guitar,
  piano,
  numStems,
};

// O modelo só trabalha em 44,1 kHz, estéreo.
constexpr int kSeparationRate = 44100;

struct Stereo
{
  std::vector<float> left, right;
  size_t frames() const { return left.size(); }
};

class Separator
{
public:
  Separator();
  ~Separator();

  // Lê os pesos (ggml-model-htdemucs-6s-f16.bin). Pode demorar um pouco.
  bool load(const std::string& modelFile, std::string& error);
  bool loaded() const;

  // Separa "mix" (44,1 kHz). "progress" recebe de 0 a 1 de qualquer linha de
  // execução. "cancel" é lido durante o trabalho; se virar true, devolve false
  // com o erro "cancelado".
  bool separate(const Stereo& mix, int threads, const std::function<void(float)>& progress,
                const std::atomic<bool>& cancel, std::array<Stereo, numStems>& out, std::string& error) const;

  // Quantos núcleos usar por padrão: metade dos disponíveis, para o resto do
  // computador (e o áudio) seguir livre.
  static int defaultThreads();

private:
  struct Model;
  std::unique_ptr<Model> mModel;
};
} // namespace nexus::studio
