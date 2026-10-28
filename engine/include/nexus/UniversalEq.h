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

// Equalizador gráfico de 5 faixas dos cabeçotes de inspiração Mark (80, 240,
// 750, 2200 e 6600 Hz), também depois da caixa. Cada deslizante vai de -12 a
// +12 dB; tudo em 0 deixa o som intacto.
class GraphicEq5
{
public:
  static constexpr int kBands = 5;
  static constexpr double kFrequency[kBands] = {80.0, 240.0, 750.0, 2200.0, 6600.0};

  void prepare(double sampleRate);
  void setBands(const std::array<double, kBands>& db) { mTarget = db; }
  void reset();
  void process(double* buffer, int numFrames);
  double responseDb(double frequency) const;

private:
  void rebuild();
  std::array<IirSection<2>, kBands> mBands{};
  std::array<double, kBands> mTarget{}, mCurrent{};
  double mSampleRate = 48000.0;
  double mAlpha = 1.0;
  bool mFlat = true;
};

// Equalizador do fim da cadeia, no estilo do FabFilter Pro-Q: fixo depois
// da caixa (todo o som passa por ele) e com até 24 faixas livres. Cada faixa
// tem tipo, frequência, ganho, largura (Q) e, nos cortes, inclinação de 12 a
// 48 dB por oitava. Sem nenhuma faixa ligada, o som passa intacto.
class FreeEq
{
public:
  static constexpr int kMaxBands = 24;
  enum class Type
  {
    bell,
    lowShelf,
    highShelf,
    lowCut,
    highCut,
    notch,
  };
  struct Band
  {
    bool on = false;
    Type type = Type::bell;
    double freq = 1000.0;
    double gainDb = 0.0;
    double q = 1.0;
    int slope = 1; // cortes: 1 a 4 (12, 24, 36 ou 48 dB por oitava)
    bool operator==(const Band&) const = default;
  };
  using Bands = std::array<Band, kMaxBands>;

  void prepare(double sampleRate);
  void set(const Bands& b) { mTarget = b; }
  void reset();
  void process(double* buffer, int numFrames);
  // Resposta (dB) de um ajuste qualquer: a curva da tela (todas as faixas ou
  // só uma, com "only").
  static double responseDb(const Bands& b, double frequency, double sampleRate, int only = -1);

private:
  struct Sections
  {
    std::array<IirSection<2>, 4> s{};
    int count = 0;
  };
  static Sections design(const Band& b, double sampleRate);
  void rebuild(int band);
  Bands mTarget{}, mCurrent{};
  std::array<Sections, kMaxBands> mFilters{};
  double mSampleRate = 48000.0;
  double mAlpha = 1.0;
};

} // namespace nexus
