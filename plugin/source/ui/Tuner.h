#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

#include <vector>

class NexusProcessor;

// Afinador de rack, fixo no começo da cadeia (escuta a guitarra antes de
// tudo). A tela lê as últimas amostras da entrada trinta vezes por segundo e
// acha a nota pelo método YIN: primeiro numa cópia reduzida (rápido), depois
// refina o período na taxa cheia.
namespace nexus::ui
{
class PitchDetector
{
public:
  // Frequência em Hz (0 = sem nota firme) das amostras na taxa dada.
  double detect(const float* samples, int count, double rate);

private:
  std::vector<double> mSmall, mDiff;
};

class TunerStage : public juce::Component, private juce::Timer
{
public:
  explicit TunerStage(NexusProcessor& processor);
  ~TunerStage() override;

  void paint(juce::Graphics&) override;
  void resized() override;
  void mouseDown(const juce::MouseEvent&) override;
  void visibilityChanged() override;

  // Lê as últimas amostras e acha a nota (o relógio chama 30 vezes por segundo).
  void update();

  // Miniatura na cadeia: a unidade de rack com a fileira de luzes.
  static void paintThumb(juce::Graphics& g, juce::Rectangle<float> r, float alpha);

private:
  void timerCallback() override;
  float referenceA() const;
  bool muted() const;

  NexusProcessor& mProcessor;
  PitchDetector mDetector;
  std::vector<float> mSamples;
  std::vector<double> mRecent; // últimas leituras, para tirar a mediana
  double mHz = 0.0;            // frequência mostrada (0 = sem nota)
  double mCents = 0.0;         // desvio suavizado
  int mHold = 0;               // leituras sem nota antes de apagar
  bool mOpen = false;          // a tela do afinador está aparecendo
  juce::Rectangle<float> mPanel, mRefDown, mRefUp, mMute;
};
} // namespace nexus::ui
