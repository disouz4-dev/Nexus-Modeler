#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

#include "PluginProcessor.h"

// Tela provisória da prova do motor: botões simples e um medidor que mostra
// quais capturas estão tocando e com que peso. A tela 3D vem depois.
class NexusEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
  explicit NexusEditor(NexusProcessor&);
  ~NexusEditor() override;

  void paint(juce::Graphics&) override;
  void resized() override;

private:
  struct Knob
  {
    juce::Slider slider;
    juce::Label label;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
  };

  void timerCallback() override;
  void addKnob(Knob& k, const juce::String& paramId, const juce::String& text);
  void paintCaptureMeter(juce::Graphics& g, juce::Rectangle<int> area);

  NexusProcessor& mProcessor;
  juce::LookAndFeel_V4 mLook;
  juce::TextButton mOpenButton{"Abrir pacote..."};
  juce::Label mStatus;
  Knob mMain, mBass, mMid, mTreble, mPresence, mVolume;
  juce::ToggleButton mStepped{"Degraus"}, mLite{"A2 Lite"};
  std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> mSteppedAttachment, mLiteAttachment;
  std::unique_ptr<juce::FileChooser> mChooser;
  juce::Rectangle<int> mMeterArea;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(NexusEditor)
};
