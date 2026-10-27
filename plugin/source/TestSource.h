#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_formats/juce_audio_formats.h>

#include <atomic>
#include <memory>

// "Testar o som": em vez da guitarra ligada, a entrada do Nexus recebe uma
// gravação de guitarra direta (sem amplificador) que repete sem parar. Assim
// dá para ouvir a cadeia inteira (pedais, cabeçote, caixa) e ajustar os
// botões sem tocar.
//
// Gravações embutidas por estilo (Limpo, Blues, Rock, Pesado), porque cada
// equipamento se avalia com o jeito de tocar dele: um riff pesado não diz nada
// de um timbre limpo, e um dedilhado limpo soa mal num cabeçote de alto ganho.
// E "Minha guitarra": grava alguns segundos da sua guitarra e repete, que é o
// teste mais fiel (seus captadores, sua mão).
//
// O áudio só lê; a troca de gravação acontece fora dele.
namespace nexus
{
class TestSource
{
public:
  enum Style
  {
    clean,
    blues,
    rock,
    heavy,
    mine, // gravação do usuário
    numStyles,
  };
  static constexpr double kRecordSeconds = 15.0;

  TestSource();
  ~TestSource();

  // Fora do áudio.
  void prepare(double sampleRate);
  void setStyle(Style s);
  Style style() const { return mStyle.load(); }
  bool hasRecording() const;
  void setPlaying(bool on);
  bool playing() const { return mPlaying.load(); }
  // Começa a gravar a entrada; ao terminar, toca a gravação sozinha.
  void startRecording();
  bool recording() const { return mRecording.load(); }
  float recordProgress() const;
  // Chamado pela tela (no relógio dela): termina a gravação e libera memória antiga.
  void poll();

  // No áudio: substitui a entrada (n amostras) se estiver tocando; grava se
  // estiver gravando. "in" pode ser nulo (sem entrada).
  // Devolve true se "dst" recebeu a gravação de teste.
  bool process(const float* in, float* dst, int n);

private:
  struct Clip
  {
    juce::AudioBuffer<float> audio; // mono, na taxa do programa
  };
  void load(Style s);
  std::unique_ptr<Clip> decode(Style s) const;

  double mRate = 48000.0;
  std::atomic<Style> mStyle{rock};
  std::atomic<bool> mPlaying{false};
  std::atomic<bool> mRecording{false};
  std::atomic<bool> mRecordDone{false};
  juce::SpinLock mLock;
  std::unique_ptr<Clip> mClip, mRetired;
  int mPos = 0;
  // Gravação do usuário: memória reservada antes, o áudio só escreve.
  juce::AudioBuffer<float> mRecord;
  std::atomic<int> mRecordPos{0};
  std::unique_ptr<Clip> mMine; // a última gravação (na taxa do programa)
};
} // namespace nexus
