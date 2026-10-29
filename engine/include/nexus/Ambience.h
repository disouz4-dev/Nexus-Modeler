#pragma once

#include <array>
#include <vector>

#include "nexus/DelayLine.h"

// Pedais de ambiente do próprio Nexus, com vários modos cada: reverb (Juniper
// Sky, no espírito do BigSky, com shimmer), delay (Lime Echo, no espírito do
// DL4) e modulação (Galaxy Mod, no espírito do MM4). Mono, em double, na taxa
// do motor; botões de 0 a 1.
namespace nexus
{
// Troca de afinação por duas leituras de uma linha de atraso que se
// revezam (cada uma entra e sai em janela suave). Barata e sem latência
// grande: serve para o shimmer e para o detune.
class PitchShifter
{
public:
  void prepare(double sampleRate, double windowSeconds);
  void reset();
  // ratio: 2 = oitava acima, 0,5 = oitava abaixo, 1,5 = quinta acima.
  double process(double x, double ratio);

private:
  DelayLine mLine;
  double mWindow = 2400.0; // em amostras
  double mPhase = 0.0;
};

// Filtro de estado variável (Chamberlin com dois passos): passa-baixa,
// passa-banda e passa-alta de uma vez, estável em toda a faixa.
struct StateVariable
{
  double low = 0.0, band = 0.0, high = 0.0;
  void reset() { low = band = high = 0.0; }
  void process(double x, double hz, double q, double sampleRate);
};

// Reverb de vários modos (Juniper Sky): rede de oito linhas realimentadas
// (FDN) com difusão na entrada, modulação nas linhas e troca de afinação
// dentro da cauda nos modos shimmer.
class SkyReverb
{
public:
  enum Mode
  {
    hall,
    room,
    plate,
    spring,
    cloud,
    swell,
    shimmerUp,    // oitava acima
    shimmerFifth, // quinta + oitava
    shimmerDown,  // oitava abaixo
    shimmerTail,  // shimmer só quando a nota fica soando
    shimmerDual,  // oitava acima e abaixo
    modeCount
  };
  static const char* modeName(int mode);
  void prepare(double sampleRate);
  void reset();
  // in: o que entra no reverb; wet: só a cauda; dry: quanto do som seco
  // fica (o modo swell abaixa o ataque das notas). Botões de 0 a 1:
  // decaimento, pré-atraso, tom, modulação e shimmer (nos modos sem
  // shimmer, o último dá a cor de cada modo: reflexões, gotejar da mola,
  // espessura da nuvem, tempo do swell).
  void process(const double* in, double* wet, double* dry, int n, int mode, double decay, double preDelay, double tone,
               double mod, double shimmer);

private:
  static constexpr int kLines = 8;
  struct AllPass
  {
    std::vector<double> buffer;
    int index = 0;
    int length = 1;
    double process(double x, double g);
  };
  struct Spring
  {
    DelayLine line;
    std::array<double, 8> chirp{};
    double low = 0.0;
    double length = 1.0;
  };
  double mRate = 48000.0;
  DelayLine mPre;
  std::array<AllPass, 6> mDiffusers;
  std::array<DelayLine, kLines> mLines;
  std::array<double, kLines> mDamp{};
  std::array<double, kLines> mLfo{};
  std::array<Spring, 2> mSprings;
  PitchShifter mShiftA, mShiftB;
  double mShimmerLow = 0.0, mShimmerHigh = 0.0;
  double mOutLow = 0.0, mOutHigh = 0.0;
  double mEnvIn = 0.0, mEnvWet = 0.0, mTailGate = 0.0;
  double mSwell = 1.0;
  std::array<double, kLines> mFeedback{}; // saída de cada linha (volta pela matriz)
  double mDecay = 0.5, mPreDelay = 0.0, mTone = 0.5, mMod = 0.3, mShimmer = 0.5; // suavizados
  int mMode = -1;
};

// Delay de vários modos (Lime Echo).
class EchoDelay
{
public:
  enum Mode
  {
    digital,
    analog,
    tape,
    multiHead, // três cabeças de fita (estilo Space Echo)
    pingPong,  // repetições alternando timbre (a cadeia é mono)
    reverse,
    modulated, // chorus nas repetições
    lofi,
    slapback,
    swell, // o som entra devagar e as repetições ficam
    duet,  // duas repetições juntas (1/4 e 1/8 pontuada)
    modeCount
  };
  static const char* modeName(int mode);
  // Andamento: tempo em segundos preso à divisão escolhida (0 = segue o botão).
  double syncSeconds = 0.0;
  void prepare(double sampleRate);
  void reset();
  // Botões de 0 a 1: tempo, repetições, tom e modulação (no lo-fi, quanto
  // o som se desfaz; no swell, o tempo de subida; no dueto, o equilíbrio).
  void process(const double* in, double* wet, double* dry, int n, int mode, double time, double repeats, double tone, double mod);

private:
  double mRate = 48000.0;
  DelayLine mLine, mLineB;
  double mDelay = 0.0;
  double mLow = 0.0, mHigh = 0.0, mLowB = 0.0, mHighB = 0.0;
  double mWow = 0.0, mFlutter = 0.0, mDrift = 0.0;
  double mReverse = 0.0; // fase do reverso (0 a 1)
  double mHeld = 0.0, mHoldCount = 0.0; // lo-fi: amostra segurada
  double mSwell = 1.0;
  double mRepeats = 0.5, mTone = 0.5, mMod = 0.3; // suavizados
  int mMode = -1;
};

// Modulação de vários modos (Galaxy Mod). Sem rastro: mexe no som no lugar.
class MultiMod
{
public:
  enum Mode
  {
    rotary,
    harmonicTremolo,
    vibrato,
    envelopeFilter,
    autoWah,
    ringMod,
    triChorus,
    sliceTremolo, // tremolo picotado (onda quadrada)
    phaser12,
    thruZero, // flanger que passa pelo zero
    detune,
    modeCount
  };
  static const char* modeName(int mode);
  // Andamento: velocidade em Hz presa à divisão escolhida (0 = segue o botão).
  double syncHz = 0.0;
  void prepare(double sampleRate);
  void reset();
  // Botões de 0 a 1: velocidade, profundidade, mistura, tom e ajuste (o
  // ajuste muda de papel com o modo: no rotary é o drive, nos filtros a
  // ressonância, no phaser e no flanger a realimentação).
  void process(double* buffer, int n, int mode, double rate, double depth, double mix, double tone, double tweak);

private:
  double mRate = 48000.0;
  DelayLine mLine;
  std::array<double, 12> mStages{};
  StateVariable mFilter, mSplit, mSplitB;
  PitchShifter mUp, mDown;
  double mPhase = 0.0, mPhaseB = 0.0;
  double mHornHz = 0.8, mDrumHz = 0.7; // rotary: velocidade atual (com inércia)
  double mEnv = 0.0, mLast = 0.0, mToneLow = 0.0;
  double mDepth = 0.5, mMix = 0.5, mTone = 0.5, mTweak = 0.5; // suavizados
  int mMode = -1;
};

// Três delays digitais em série (Blueberry Chain): o primeiro alimenta o
// segundo, que alimenta o terceiro, e as repetições se multiplicam em cascata.
class TripleDelay
{
public:
  static constexpr int kStages = 3;
  struct Stage
  {
    bool on = true;        // desligado, o som passa direto por este delay
    double seconds = 0.25; // tempo da repetição (até 3 s)
    double repeats = 4.0;  // quantas repetições até sumir (cair 40 dB)
    double level = 0.4;    // volume da repetição, de 0 a 1
  };
  void prepare(double sampleRate);
  void reset();
  // Tempo do primeiro delay para a posição do botão (de 0 a 1): 20 ms a 1 s.
  static double secondsFor(double time);
  // Feedback que faz o som repetir "count" vezes até cair 40 dB.
  static double feedbackFor(double count);
  // in: o que entra (já com o liga/desliga); wet: só o que os delays somam.
  void process(const double* in, double* wet, int n, const std::array<Stage, kStages>& stages);

private:
  struct Line
  {
    DelayLine line;
    double delay = 0.0; // em amostras (escorrega até o tempo novo)
    double low = 0.0, high = 0.0;
    double feedback = 0.0, level = 0.0; // suavizados
  };
  double mRate = 48000.0;
  std::array<Line, kStages> mLines;
};
} // namespace nexus
