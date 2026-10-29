#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

#include <array>
#include <atomic>

// Controle por MIDI (USB, Bluetooth ou vindo do programa de gravação).
//
// - Troca de patch (Program Change) 1, 2, 3 escolhe o canal do cabeçote.
// - Cada controle contínuo (CC 0 a 127) pode ser ligado a um parâmetro:
//   botão (0 a 127 vira 0 a 10), liga/desliga (64 ou mais = ligado) ou
//   escolha (o valor percorre as opções).
// - "Aprender": a tela marca um parâmetro, e o próximo controle que chegar
//   fica ligado a ele: CC, troca de patch (PC) ou nota. Pedaleiras como a
//   M-VAVE Chocolate mandam PC ou nota a cada pisada; cada uma liga e desliga
//   (ou passa para a próxima opção). PC aprendido deixa de trocar o canal.
//
// O valor é aplicado no próprio bloco de áudio em que a mensagem chega.
class MidiMap
{
public:
  explicit MidiMap(juce::AudioProcessor& processor);

  // Áudio: lê as mensagens do bloco e aplica.
  void process(const juce::MidiBuffer& midi);

  // Tela: aprender, esquecer, consultar.
  void learn(const juce::String& paramId);
  void cancelLearn() { mLearn.store(-1); }
  bool learning() const { return mLearn.load() >= 0; }
  juce::String learningParam() const;
  void forget(const juce::String& paramId);
  // Controle ligado ao parâmetro (-1 = nenhum): 0 a 127 CC, 128 a 255 PC,
  // 256 a 383 nota. describe() escreve "CC 20", "PC 3" ou "Nota 60".
  int ccFor(const juce::String& paramId) const;
  static juce::String describe(int control);
  // Último controle aprendido (para a tela confirmar), ou -1.
  int takeLearned() { return mLearned.exchange(-1); }

  // Guardar e restaurar junto com o ajuste do plugin.
  juce::ValueTree toValueTree() const;
  void fromValueTree(const juce::ValueTree& tree);

  static constexpr int kProgramChannels = 3;

private:
  int indexOf(const juce::String& paramId) const;
  void apply(int paramIndex, float normalised);

  juce::AudioProcessor& mProcessor;
  juce::Array<juce::AudioProcessorParameter*> mParams;
  static constexpr int kControls = 384; // CC, PC e notas
  void bind(int control, int paramIndex);
  void press(int target); // PC ou nota: liga/desliga ou próxima opção
  std::array<std::atomic<int>, kControls> mCcTarget; // índice do parâmetro, -1 = livre
  std::atomic<int> mLearn{-1};
  std::atomic<int> mLearned{-1};
};
