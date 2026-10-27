#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include <functional>

#include "Controls.h"

// Presets: o som inteiro (cadeia, pedais, cabeçotes, botões, capturas, IRs e
// MIDI) num arquivo .nexus em "Nexus Modeler/Presets". Na barra de cima: o
// nome do preset, setas para o anterior e o próximo, e um menu para salvar,
// salvar como, renomear e apagar.
namespace nexus::ui
{
class PresetBar : public juce::Component
{
public:
  // getState: o estado atual do plugin; setState: aplica um estado salvo.
  PresetBar(std::function<juce::MemoryBlock()> getState, std::function<void(const juce::MemoryBlock&)> setState);

  static juce::File folder();
  juce::Array<juce::File> list() const;
  void load(const juce::File& file);
  // Grava; sem arquivo atual, pergunta o nome.
  void save(bool askName);
  void step(int delta);
  juce::String currentName() const { return mCurrent.existsAsFile() ? mCurrent.getFileNameWithoutExtension() : juce::String(); }

  void paint(juce::Graphics&) override;
  void resized() override;
  void mouseUp(const juce::MouseEvent&) override;
  void retranslate() { repaint(); }

private:
  void showMenu();
  void askName(const juce::String& title, const juce::String& initial, std::function<void(juce::String)> done);
  juce::Rectangle<int> nameArea() const;

  std::function<juce::MemoryBlock()> mGetState;
  std::function<void(const juce::MemoryBlock&)> mSetState;
  juce::File mCurrent;
  IconButton mPrev{Icon::left, "Preset anterior"};
  IconButton mNext{Icon::right, juce::String::fromUTF8("Próximo preset")};
  std::unique_ptr<juce::AlertWindow> mDialog;
};
} // namespace nexus::ui
