#pragma once

#include <memory>
#include <vector>

#include "nexus/Pack.h"

namespace nexus
{

// Motor de troca de capturas. Recebe a posição do botão principal e decide,
// a cada bloco, quais capturas tocam e com que peso:
//  - botão sobre uma captura: só ela toca;
//  - botão entre duas: as duas vizinhas tocam, misturadas na proporção;
//  - captura que vai entrar ouve primeiro o som recente da guitarra
//    ("aquecimento") e só então entra numa transição suave.
// process() não abre arquivo, não reserva memória e não espera nada.
class MorphEngine
{
public:
  struct Settings
  {
    double positionSmoothingMs = 40.0; // inércia do botão
    double crossfadeMs = 30.0;         // duração da transição entre capturas
    double coolDownMs = 300.0;         // captura parada há tanto tempo deixa de rodar
    int maxRunning = 4;                // limite de capturas rodando juntas
    // Mão no botão: a vizinha de cada lado já fica aquecida, para o giro
    // não esperar o aquecimento (132 ms numa A2). Mais que isso pesa demais
    // em A2 Full (cada captura real custa 10 a 20% de um núcleo).
    int anticipateEachSide = 1;
    int maxRunningAnticipating = 4;
  };

  // Fora do áudio. O pacote já deve estar preparado na mesma taxa e bloco.
  void prepare(std::shared_ptr<Pack> pack, double sampleRate, int maxBlockSize, Settings settings);
  void prepare(std::shared_ptr<Pack> pack, double sampleRate, int maxBlockSize)
  {
    prepare(std::move(pack), sampleRate, maxBlockSize, Settings{});
  }

  // Do áudio.
  void setTargetPosition(double position) { mTarget = position; }
  void setStepped(bool stepped) { mStepped = stepped; }
  // Alguém pegou o botão (ou automação começou): aquece as vizinhas antes.
  void setAnticipate(bool on) { mAnticipate = on; }
  // Vai direto para a posição alvo, sem transição (usar só antes de tocar).
  void jumpToTarget();
  void process(const double* input, double* output, int numFrames);

  // Para a tela e para os testes.
  double currentPosition() const { return mPosition; }
  int runningCount() const;
  // Peso atual de cada captura na mistura (0 a 1).
  double weightOf(int capture) const { return mSlots[capture].weight; }
  double targetWeightOf(int capture) const { return mSlots[capture].target; }
  const Pack* pack() const { return mPack.get(); }

private:
  enum class State
  {
    Cold,
    Warming,
    Hot
  };
  struct Slot
  {
    State state = State::Cold;
    int warmLeft = 0;
    int idleSamples = 0;
    double weight = 0.0; // peso atual, anda em rampa
    double target = 0.0; // peso desejado já considerando quem está pronto
    double desired = 0.0; // peso pela posição do botão
    bool near = false;    // vizinha que fica aquecida com a mão no botão
    std::vector<double> buffer;
  };

  void computeDesired(double position);
  void startWarming();
  void computeTargets();
  bool isRunning(const Slot& s) const;

  std::shared_ptr<Pack> mPack;
  std::vector<Slot> mSlots;
  std::vector<double> mInputCopy;
  std::vector<int> mOrder;
  Settings mSettings;
  double mSampleRate = 48000.0;
  double mTarget = 0.0;
  double mPosition = 0.0;
  bool mStepped = false;
  bool mAnticipate = false;
  double mWeightStep = 0.0;
};

} // namespace nexus
