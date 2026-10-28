#pragma once

#include <array>
#include <vector>

#include "nexus/Ambience.h"
#include "nexus/DelayLine.h"

// Efeitos feitos pelo próprio Nexus (modulação, tempo e ambiente não se
// capturam bem com NAM). Mono, em double, na taxa do motor. Os três botões
// de cada pedal chegam de 0 a 1.
namespace nexus
{
// Chorus de duas vozes (estilo CE-2): atraso de 6 a 16 ms balançado por um
// oscilador lento. Botões: velocidade, profundidade, mistura.
class Chorus
{
public:
  // Andamento: velocidade em Hz presa à divisão escolhida (0 = segue o botão).
  double syncHz = 0.0;
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
  // Andamento: tempo em segundos preso à divisão escolhida (0 = segue o botão).
  double syncSeconds = 0.0;
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

// Phaser no molde do MXR Phase 90: quatro estágios passa-tudo de primeira
// ordem (no pedal, um JFET como resistor variável com um capacitor de 47 nF
// em cada estágio), varridos por um oscilador triangular, e o som limpo somado
// meio a meio com o defasado, o que abre dois vales fundos no espectro (em
// 0,41 e 2,41 vezes a frequência dos estágios). Botões:
//   velocidade   0,1 a 10 Hz, em escala de oitavas (5 = 1 Hz)
//   profundidade largura da varredura (10 = de 140 Hz a 2,5 kHz nos estágios)
//   intensidade  realimentação do quarto estágio para o primeiro, como no
//                Phase 90 de logotipo em bloco (0 = o "script", sem nenhuma)
class Phaser
{
public:
  // Andamento: velocidade em Hz presa à divisão escolhida (0 = segue o botão).
  double syncHz = 0.0;
  // Velocidade do botão (0 a 1) em Hz.
  static double rateHz(double knob01);
  void prepare(double sampleRate);
  void reset();
  void process(double* buffer, int n, double rate, double depth, double feedback);

private:
  double mRate = 48000.0;
  double mPhase = 0.0;
  std::array<double, 4> mState{};
  double mLast = 0.0;
  double mHz = 1.0, mDepth = 0.5, mFeedback = 0.0;
};

// Flanger (estilo Electric Mistress): atraso de 0,4 a 7 ms varrido devagar,
// com realimentação. Botões: velocidade, profundidade, realimentação.
class Flanger
{
public:
  // Andamento: velocidade em Hz presa à divisão escolhida (0 = segue o botão).
  double syncHz = 0.0;
  void prepare(double sampleRate);
  void reset();
  void process(double* buffer, int n, double rate, double depth, double feedback);

private:
  DelayLine mLine;
  double mRate = 48000.0;
  double mPhase = 0.0;
  double mLast = 0.0;
  double mDepth = 0.5, mFeedback = 0.4;
};

// Tremolo (estilo TR-2): o volume sobe e desce. Botões: velocidade,
// profundidade, forma (de onda redonda a quadrada).
class Tremolo
{
public:
  // Andamento: velocidade em Hz presa à divisão escolhida (0 = segue o botão).
  double syncHz = 0.0;
  void prepare(double sampleRate);
  void reset();
  void process(double* buffer, int n, double rate, double depth, double shape);

private:
  double mRate = 48000.0;
  double mPhase = 0.0;
  double mDepth = 0.5, mShape = 0.0;
};

// Vibe (estilo Uni-Vibe): quatro passa-tudo de frequências diferentes,
// movidos por um oscilador "de lâmpada" (sobe rápido, desce devagar).
// Botões: velocidade, intensidade, mistura (0 = vibrato, 1 = chorus).
class Vibe
{
public:
  // Andamento: velocidade em Hz presa à divisão escolhida (0 = segue o botão).
  double syncHz = 0.0;
  void prepare(double sampleRate);
  void reset();
  void process(double* buffer, int n, double rate, double intensity, double mix);

private:
  double mRate = 48000.0;
  double mPhase = 0.0;
  double mLamp = 0.0;
  std::array<double, 4> mState{};
  double mIntensity = 0.5, mMix = 0.5;
};

// Pedal nativo completo: o efeito, a mistura com o som seco e o "rastro"
// (desligado, o delay e o reverb terminam de soar em vez de cortar).
// Noise gate (estilo ISP Decimator): fecha o som abaixo do limiar, sem
// cortar o fim das notas. Detector rápido de pico, histerese de 6 dB (abre
// no limiar, fecha 6 dB abaixo), espera de 25 ms antes de fechar, abertura
// no tempo do "ataque" e fechamento no tempo do "release".
class NoiseGate
{
public:
  void prepare(double sampleRate);
  void reset();
  // sensitivity, attack, release: botões de 0 a 1.
  //   sensibilidade: limiar de -90 dB (quase não corta) a -30 dB (corta muito)
  //   ataque: 0,1 a 30 ms     release: 10 ms a 1,5 s
  void process(double* buffer, int n, double sensitivity, double attack, double release);
  static double thresholdDb(double sensitivity) { return -90.0 + 60.0 * sensitivity; }
  double gainDb() const;

private:
  double mRate = 48000.0;
  double mEnv = 0.0;
  double mGain = 1.0;
  int mHold = 0;
  bool mOpen = true;
};

class NativePedal
{
public:
  enum class Type
  {
    chorus,
    delay,
    reverb,
    gate,
    phaser,
    flanger,
    tremolo,
    vibe,
    sky,     // Juniper Sky: reverb de vários modos
    echo,    // Lime Echo: delay de vários modos
    multiMod, // Plum Mod: modulação de vários modos
    triple    // Blueberry Chain: três delays em série
  };
  void prepare(double sampleRate, int maxBlock);
  void reset();
  // Andamento (tap tempo ou o da música): velocidade da modulação em Hz e
  // tempo do delay em segundos; 0 nos dois = os botões mandam.
  void setSync(double hz, double seconds);
  // Pedais de vários modos: o modo e os botões além dos três (tom, o quarto
  // botão e, no reverb, o shimmer), de 0 a 1.
  void setExtras(int mode, double tone, double fourth, double fifth)
  {
    mMode = mode;
    mTone = tone;
    mFourth = fourth;
    mFifth = fifth;
  }
  // Blueberry Chain: os botões e as chaves dos três delays.
  void setTriple(const std::array<TripleDelay::Stage, TripleDelay::kStages>& stages) { mStages = stages; }
  // on: pedal ligado; a, b, c: os três botões (0 a 1). Mexe no buffer no lugar.
  void process(Type type, bool on, double* buffer, int n, double a, double b, double c);
  // Ainda soando (ligado ou com rastro)? Quando não, não gasta processador.
  bool active() const { return mGain > 0.0 || mTail > 0; }

private:
  Chorus mChorus;
  Delay mDelay;
  Reverb mReverb;
  NoiseGate mGate;
  Phaser mPhaser;
  Flanger mFlanger;
  Tremolo mTremolo;
  Vibe mVibe;
  SkyReverb mSky;
  EchoDelay mEcho;
  MultiMod mMulti;
  TripleDelay mTriple;
  std::array<TripleDelay::Stage, TripleDelay::kStages> mStages{};
  std::vector<double> mIn, mWet, mDry, mGains;
  int mMode = 0;
  double mTone = 0.5, mFourth = 0.5, mFifth = 0.5;
  double mRate = 48000.0;
  double mGain = 0.0; // entrada do efeito: 0 desligado, 1 ligado (transição de 20 ms)
  double mMix = 0.0;
  int mTail = 0;      // amostras de rastro que ainda faltam depois de desligar
  Type mLast = Type::chorus;
};
} // namespace nexus
