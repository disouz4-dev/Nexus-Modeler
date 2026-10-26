#pragma once

#include <array>
#include <string>

namespace nexus
{

// Peças do circuito de tonalidade clássico (Fender/Marshall/Vox, "FMV").
// R1 = potenciômetro de agudo, R2 = grave, R3 = médio, R4 = resistor de
// inclinação; C1 = capacitor de agudo, C2 = grave, C3 = médio.
struct FmvCircuit
{
  double R1, R2, R3, R4, C1, C2, C3;
};

// Valores de referência por família. Os de alto ganho são aproximados e serão
// conferidos família a família contra o esquema de cada amplificador.
FmvCircuit circuitByName(const std::string& name);

// Posição dos botões de 0 a 10, como no painel.
struct ToneKnobs
{
  double bass = 5.0;
  double mid = 5.0;
  double treble = 5.0;

  bool operator==(const ToneKnobs& o) const { return bass == o.bass && mid == o.mid && treble == o.treble; }
};

// Polinômios em s (Laplace) do circuito para uma posição de botões.
// H(s) = s * (b1 + b2 s + b3 s^2) / (a0 + a1 s + a2 s^2 + a3 s^3)
// O fator s comum é separado porque o circuito não passa corrente contínua:
// ele se cancela na razão entre duas posições.
struct FmvPolynomials
{
  std::array<double, 3> num; // b1, b2, b3
  std::array<double, 4> den; // a0..a3
};

FmvPolynomials fmvPolynomials(const FmvCircuit& c, const ToneKnobs& k);

// Fração de cada potenciômetro para uma posição de botões (agudo, médio,
// grave). O grave usa curva logarítmica, como o potenciômetro de áudio real.
struct PotFractions
{
  double t, m, l;
};
PotFractions potFractions(const ToneKnobs& k);

// Biquad/cúbico em forma direta transposta, coeficientes normalizados (a0 = 1).
template <int N>
struct IirSection
{
  std::array<double, N + 1> b{};
  std::array<double, N + 1> a{};
  std::array<double, N> s{};

  inline double tick(double x)
  {
    const double y = b[0] * x + s[0];
    for (int i = 0; i < N - 1; ++i)
      s[i] = b[i + 1] * x - a[i + 1] * y + s[i + 1];
    s[N - 1] = b[N] * x - a[N] * y;
    return y;
  }
  void clear() { s.fill(0.0); }
};

// Transformação bilinear de um polinômio analógico de grau <= N
// (coeficientes em ordem crescente de s) para coeficientes em z^-1.
template <int N>
std::array<double, N + 1> bilinear(const std::array<double, N + 1>& analog, double sampleRate);

// Equalizador do painel: aplica só a DIFERENÇA entre a posição em que a captura
// foi feita e a posição escolhida pelo usuário. Na posição da captura o sinal
// passa intacto.
class RelativeToneStack
{
public:
  void prepare(double sampleRate);
  void setCircuit(const FmvCircuit& c);
  // Fora do áudio: posição dos botões quando a captura foi gravada.
  void setCaptureKnobs(const ToneKnobs& k);
  // Do áudio: posição atual (já suavizada por quem chama).
  void setUserKnobs(const ToneKnobs& k);
  void process(double* buffer, int numFrames);
  void reset();
  bool isIdentity() const { return mIdentity; }

private:
  void rebuild();

  double mSampleRate = 48000.0;
  FmvCircuit mCircuit{};
  ToneKnobs mCapture{}, mUser{};
  FmvPolynomials mCapturePoly{};
  bool mIdentity = true;
  IirSection<2> mQuadratic; // (b_usuario) / (b_captura)
  IirSection<3> mCubic;     // (a_captura) / (a_usuario)
};

// Prateleira (realce de agudo ou de grave) em relação à posição de captura:
// presença e ressonância do estágio de potência.
class RelativeShelf
{
public:
  enum class Kind
  {
    High,
    Low
  };
  void prepare(double sampleRate, Kind kind, double frequency, double maxBoostDb);
  void setCaptureKnob(double v) { mCapture = v; }
  void setUserKnob(double v);
  void process(double* buffer, int numFrames);
  void reset() { mFilter.clear(); }

private:
  double mSampleRate = 48000.0, mFrequency = 4000.0, mMaxBoostDb = 8.0;
  double mCapture = 5.0, mUser = 5.0;
  Kind mKind = Kind::High;
  bool mIdentity = true;
  IirSection<2> mFilter;
};

// Equalizador gráfico (ex.: as 5 barrinhas do Mark), também relativo à
// posição de captura. Cada barra vai de -12 dB a +12 dB, 0 no meio.
class RelativeGraphicEq
{
public:
  static constexpr int kMaxBands = 8;
  void prepare(double sampleRate, const double* frequencies, int numBands);
  void setCaptureBand(int band, double db) { mCaptureDb[band] = db; }
  void setUserBand(int band, double db);
  int numBands() const { return mNumBands; }
  void process(double* buffer, int numFrames);
  void reset();

private:
  double mSampleRate = 48000.0;
  int mNumBands = 0;
  std::array<double, kMaxBands> mFreq{}, mCaptureDb{}, mUserDb{};
  std::array<bool, kMaxBands> mIdentity{};
  std::array<IirSection<2>, kMaxBands> mBands;
};

} // namespace nexus
