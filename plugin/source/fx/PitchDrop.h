#pragma once

#include <memory>

// Motor do pedal de drop: baixa o tom da guitarra em tempo real, polifônico
// (acordes inteiros), de 1 a 12 semitons. Usa a análise em frequência do
// Signalsmith Stretch (código aberto) com janela curta, para o atraso ficar
// baixo o bastante para tocar. Mono, sem reservar memória no áudio.
namespace nexus::fx
{
class PitchDrop
{
public:
  PitchDrop();
  ~PitchDrop();

  // window: tamanho da janela de análise em amostras (define o atraso).
  void prepare(double sampleRate, int maxBlock, int window = 512);
  void reset();
  void setSemitones(float semitones); // 0 a -12 (negativo: para baixo)
  // Processa no lugar: entra a guitarra, sai a guitarra com o tom mudado.
  void process(float* buffer, int numFrames);
  int latencySamples() const;

private:
  struct Impl;
  std::unique_ptr<Impl> mImpl;
};
} // namespace nexus::fx
