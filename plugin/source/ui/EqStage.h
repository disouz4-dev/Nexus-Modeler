#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

#include <functional>
#include <memory>
#include <vector>

#include "Controls.h"

// Tela do equalizador do fim da cadeia (fixo depois da caixa), no estilo do
// FabFilter Pro-Q: a curva ocupa a tela e cada faixa é um ponto colorido.
//   duplo clique num lugar vazio: cria uma faixa ali;
//   arrastar um ponto: frequência (para os lados) e ganho (para cima e baixo);
//   roda do mouse no ponto: largura (Q);
//   botão direito no ponto: tipo, inclinação, ligar/desligar, apagar.
// Embaixo, os controles da faixa escolhida.
namespace nexus::ui
{
// Botão giratório desenhado (painel de estúdio): tampa colorida, traço branco.
class PanelKnob : public juce::Component
{
public:
  PanelKnob(juce::RangedAudioParameter& p, juce::String label, juce::Colour cap, std::function<juce::String(float)> format);
  void paint(juce::Graphics&) override;
  void mouseDown(const juce::MouseEvent& e) override { mDrag.mouseDown(e); }
  void mouseDrag(const juce::MouseEvent& e) override { mDrag.mouseDrag(e); }
  void mouseUp(const juce::MouseEvent& e) override { mDrag.mouseUp(e); }
  void mouseDoubleClick(const juce::MouseEvent&) override { mDrag.mouseDoubleClick(); }
  void mouseWheelMove(const juce::MouseEvent&, const juce::MouseWheelDetails& w) override { mDrag.mouseWheel(w); }
  void mouseEnter(const juce::MouseEvent&) override { mHover = true; repaint(); }
  void mouseExit(const juce::MouseEvent&) override { mHover = false; repaint(); }
  std::function<void()> onChange;

private:
  DragValue mDrag;
  juce::String mLabel;
  juce::Colour mCap;
  std::function<juce::String(float)> mFormat;
  bool mHover = false;
};

class EqStage : public juce::Component, private juce::Timer
{
public:
  explicit EqStage(juce::AudioProcessorValueTreeState& params);
  ~EqStage() override;

  // Desliga todas as faixas (o som volta a passar intacto).
  void clearBands();

  void paint(juce::Graphics&) override;
  void resized() override;
  void mouseDown(const juce::MouseEvent&) override;
  void mouseDrag(const juce::MouseEvent&) override;
  void mouseUp(const juce::MouseEvent&) override;
  void mouseMove(const juce::MouseEvent&) override;
  void mouseExit(const juce::MouseEvent&) override;
  void mouseWheelMove(const juce::MouseEvent&, const juce::MouseWheelDetails&) override;
  void mouseDoubleClick(const juce::MouseEvent&) override;

  // Miniatura com a curva atual (caminho do sinal).
  static void paintThumb(juce::Graphics& g, juce::Rectangle<float> r, juce::AudioProcessorValueTreeState& params, float alpha = 1.0f);

private:
  void timerCallback() override;
  float value(const juce::String& id) const;
  juce::RangedAudioParameter& param(const juce::String& id) const;
  void setParam(const juce::String& id, float plain);
  bool bandOn(int band) const;
  int bandType(int band) const;
  bool hasGain(int band) const;
  float xOfFreq(double f) const;
  double freqOfX(float x) const;
  float yOfDb(double db) const;
  double dbOfY(float y) const;
  juce::Point<float> bandPoint(int band) const;
  int bandAt(juce::Point<float> p) const;
  void select(int band);
  void showBandMenu(int band);
  void addBandAt(juce::Point<float> p);
  void deleteBand(int band);
  void paintCurve(juce::Graphics&);
  void paintStrip(juce::Graphics&);

  juce::AudioProcessorValueTreeState& mParams;
  juce::Rectangle<float> mCurveArea, mStrip;
  std::vector<std::unique_ptr<PanelKnob>> mKnobs;
  int mSelected = -1, mDrag = -1, mHover = -1;
  juce::uint32 mLastHash = 0;
};
} // namespace nexus::ui
