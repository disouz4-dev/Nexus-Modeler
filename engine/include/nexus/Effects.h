#pragma once

#include <array>
#include <vector>

// Efeitos feitos pelo próprio Nexus (modulação, tempo e ambiente não se
// capturam bem com NAM). Mono, em double, na taxa do motor. Os três botões
// de cada pedal chegam de 0 a 1.
namespace nexus
{
// Linha de atraso com leitura fracionária (interpolação linear).
class DelayLine
{
public:
  void prepare(int maxSamples);
  void clear();
  void push(double x)
  {
    mWrite = (mWrite + 1) % static_cast<int>(mBuffer.size());
    mBuffer[static_cast<size_t>(mWrite)] = x;
  }
  // "delay" amostras atrás da última escrita (0 = a última).
  double read(double delay) const;

private:
  std::vector<double> mBuffer;
  int mWrite = 0;
};

// Chorus de duas vozes (estilo CE-2): atraso de 6 a 16 ms balançado por um
// oscilador lento. Botões: velocidade, profundidade, mistura.
class Chorus
{
public:
  void prepare(double sampleRate);
  void reset();
  void process(double* buffer, int n, double rate, double depth, double mix);

private:
  DelayLine mLine;
  double mRate = 48000.0;
  double mPhase = 0.0;
  double mDepth = 0.5, mMix = 0.5; // suavizados
};

// Delay analógico (estilo DD-3 com repetições que escurecem): tempo de 40 ms a
// 1,2 s, repetições até quase sem fim, filtro na volta. Botões: tempo,
// repetições, mistura.
class Delay
{
public:
  void prepare(double sampleRate);
  void reset();
  void process(const double* input, double* wet, int n, double time, double feedback);

private:
  DelayLine mLine;
  double mRate = 48000.0;
  double mDelay = 0.0; // em amostras, suavizado (sem estalo ao girar o tempo)
  double mLow = 0.0, mHigh = 0.0;
};

// Reverb de sala (Freeverb: 8 filtros de pente e 4 passa-tudo). Botões:
// tamanho, tom (brilho), mistura.
class Reverb
{
public:
  void prepare(double sampleRate);
  void reset();
  void process(const double* input, double* wet, int n, double size, double tone);

private:
  struct Comb
  {
    std::vector<double> buffer;
    int index = 0;
    double store = 0.0;
  };
  struct AllPass
  {
    std::vector<double> buffer;
    int index = 0;
  };
  std::array<Comb, 8> mCombs;
  std::array<AllPass, 4> mAllPasses;
  std::vector<double> mPre; // pré-atraso curto
  int mPreIndex = 0;
};

// Pedal nativo completo: o efeito, a mistura com o som seco e o "rastro"
// (desligado, o delay e o reverb terminam de soar em vez de cortar).
class NativePedal
{
public:
  enum class Type
  {
    chorus,
    delay,
    reverb
  };
  void prepare(double sampleRate, int maxBlock);
  void reset();
  // on: pedal ligado; a, b, c: os três botões (0 a 1). Mexe no buffer no lugar.
  void process(Type type, bool on, double* buffer, int n, double a, double b, double c);
  // Ainda soando (ligado ou com rastro)? Quando não, não gasta processador.
  bool active() const { return mGain > 0.0 || mTail > 0; }

private:
  Chorus mChorus;
  Delay mDelay;
  Reverb mReverb;
  std::vector<double> mIn, mWet;
  double mRate = 48000.0;
  double mGain = 0.0; // entrada do efeito: 0 desligado, 1 ligado (transição de 20 ms)
  double mMix = 0.0;
  int mTail = 0;      // amostras de rastro que ainda faltam depois de desligar
  Type mLast = Type::chorus;
};
} // namespace nexus
