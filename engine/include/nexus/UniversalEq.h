#pragma once

#include <array>

#include "nexus/ToneStack.h"

namespace nexus
{

// Equalizador universal do cabeçote: depois da captura e da caixa, o mesmo
// para qualquer amplificador. Grave (prateleira em 220 Hz), médio (sino em
// 750 Hz), agudo (prateleira em 2,8 kHz) e presença (sino em 5,5 kHz).
// Botão de 0 a 10; em 5 o som passa intacto, em 0 e 10 corta ou reforça 12 dB
// (a presença, 10 dB). Mono, precisão dupla, sem reservar memória no áudio.
class UniversalEq
{
public:
  struct Knobs
  {
    double bass = 5.0, mid = 5.0, treble = 5.0, presence = 5.0;
    bool operator==(const Knobs&) const = default;
  };

  static constexpr int kBands = 4;
  static constexpr double kFrequency[kBands] = {220.0, 750.0, 2800.0, 5500.0};
  static constexpr double kMaxDb[kBands] = {12.0, 12.0, 12.0, 10.0};

  void prepare(double sampleRate);
  void setKnobs(const Knobs& k) { mTarget = k; }
  void reset();
  void process(double* buffer, int numFrames);

  // Ganho de cada faixa (dB) para um botão de 0 a 10.
  static double knobToDb(double knob, int band);
  // Resposta em dB numa frequência (para os testes e a tela).
  double responseDb(double frequency) const;

private:
  void rebuild();

  std::array<IirSection<2>, kBands> mBands{};
  Knobs mTarget{}, mCurrent{};
  double mSampleRate = 48000.0;
  double mAlpha = 1.0;
  bool mFlat = true;
};

} // namespace nexus
