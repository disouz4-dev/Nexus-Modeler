#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

#include <functional>

#include "Gear.h"

// Peças da tela: botão renderizado, controle deslizante com medidor, seletor
// segmentado, ícones e links de texto.
namespace nexus::ui
{
// Arrastar para cima ou para baixo gira; Shift dá ajuste fino; duplo clique
// volta ao padrão; a roda do mouse também gira.
class DragValue
{
public:
  DragValue(juce::RangedAudioParameter& p, juce::Component& owner);
  float value01() const { return mValue; }
  juce::RangedAudioParameter& parameter() { return mParam; }
  const juce::RangedAudioParameter& parameter() const { return mParam; }
  void mouseDown(const juce::MouseEvent&);
  void mouseDrag(const juce::MouseEvent&, bool horizontal = false);
  void mouseUp(const juce::MouseEvent&);
  void mouseDoubleClick();
  void mouseWheel(const juce::MouseWheelDetails&);
  bool dragging() const { return mDragging; }
  juce::String paramId() const;
  // Chamado a cada mudança de valor (arraste, roda, automação, MIDI).
  std::function<void()> onValueChanged;

private:
  void set(float v01);
  juce::RangedAudioParameter& mParam;
  juce::ParameterAttachment mAttachment;
  float mValue = 0.0f;
  float mStartValue = 0.0f;
  bool mDragging = false;
};

// Botão do equipamento: mostra a posição certa da tira renderizada.
class FilmKnob : public juce::Component
{
public:
  FilmKnob(juce::RangedAudioParameter& p, const GearKnobArt& art, juce::String label);
  void paint(juce::Graphics&) override;
  bool hitTest(int x, int y) override;
  void mouseDown(const juce::MouseEvent& e) override;
  void mouseDrag(const juce::MouseEvent& e) override { if (!e.mods.isPopupMenu()) mDrag.mouseDrag(e); }
  void mouseUp(const juce::MouseEvent& e) override { if (!e.mods.isPopupMenu()) mDrag.mouseUp(e); }
  void mouseDoubleClick(const juce::MouseEvent&) override { mDrag.mouseDoubleClick(); }
  void mouseWheelMove(const juce::MouseEvent&, const juce::MouseWheelDetails& w) override { mDrag.mouseWheel(w); }
  void mouseEnter(const juce::MouseEvent&) override { mHover = true; changed(); }
  // Clique com o botão direito: menu de MIDI (aprender / esquecer).
  std::function<void(const juce::String& paramId, juce::Component&)> onMidiMenu;
  void mouseExit(const juce::MouseEvent&) override { mHover = false; changed(); }

  bool showsBubble() const { return mHover || mDrag.dragging(); }
  juce::String bubbleText() const;
  const GearKnobArt& art() const { return mArt; }
  std::function<void()> onChange;
  // Texto do valor no balão (vazio = o número de sempre). Ex.: "1/8" com o
  // pedal sincronizado ao andamento.
  std::function<juce::String(float value)> valueText;

private:
  void changed() { if (onChange) onChange(); }
  const GearKnobArt& mArt;
  DragValue mDrag;
  juce::String mLabel;
  bool mHover = false;
};

// Controle deslizante de nível (entrada e saída) com o medidor de pico
// desenhado por baixo: a barra mostra o som, a alça mostra o ajuste.
class LevelFader : public juce::Component
{
public:
  LevelFader(juce::RangedAudioParameter& p, juce::String label);
  void paint(juce::Graphics&) override;
  void mouseDown(const juce::MouseEvent& e) override;
  void mouseDrag(const juce::MouseEvent& e) override { if (!e.mods.isPopupMenu() && !mSkipDrag) { mDrag.mouseDrag(e, true); repaint(); } }
  void mouseUp(const juce::MouseEvent& e) override { if (!e.mods.isPopupMenu() && !mSkipDrag) { mDrag.mouseUp(e); repaint(); } }
  std::function<void(const juce::String& paramId, juce::Component&)> onMidiMenu;
  void mouseDoubleClick(const juce::MouseEvent& e) override { if (!muteArea().contains(e.position)) mDrag.mouseDoubleClick(); }
  void mouseWheelMove(const juce::MouseEvent&, const juce::MouseWheelDetails& w) override { mDrag.mouseWheel(w); }
  void setLevel(float peak) { if (peak != mLevel) { mLevel = peak; repaint(); } }
  // Botão "MUDO" ao lado do nome (liga e desliga esse parâmetro).
  void setMuteParameter(juce::RangedAudioParameter& p);

private:
  juce::Rectangle<float> muteArea() const;
  bool muted() const;
  DragValue mDrag;
  juce::String mLabel;
  float mLevel = 0.0f;
  juce::RangedAudioParameter* mMuteParam = nullptr;
  std::unique_ptr<juce::ParameterAttachment> mMute;
  bool mMuted = false;
  bool mSkipDrag = false;
};

// Seletor segmentado: uma peça só, dividida em partes; a escolhida acende.
// Uma parte pode ficar trancada (cadeado) e as outras apagadas.
class Segmented : public juce::Component
{
public:
  explicit Segmented(juce::StringArray items) : mItems(std::move(items)) { mEnabled.insertMultiple(0, true, mItems.size()); }
  void setSelected(int i) { if (i != mSelected) { mSelected = i; repaint(); } }
  int selected() const { return mSelected; }
  void setItems(const juce::StringArray& items)
  {
    if (items == mItems)
      return;
    mItems = items;
    mEnabled.clearQuick();
    mEnabled.insertMultiple(0, true, mItems.size());
    repaint();
  }
  int numItems() const { return mItems.size(); }
  void setItemEnabled(int i, bool e) { if (mEnabled[i] != e) { mEnabled.set(i, e); repaint(); } }
  void setLockedItem(int i) { if (i != mLocked) { mLocked = i; repaint(); } }
  void setFontSize(float s) { mFontSize = s; repaint(); }
  void paint(juce::Graphics&) override;
  void mouseUp(const juce::MouseEvent&) override;
  std::function<void(int)> onSelect;
  std::function<void()> onRightClick;

private:
  juce::Rectangle<float> itemBounds(int i) const;
  juce::StringArray mItems;
  juce::Array<bool> mEnabled;
  int mSelected = 0;
  int mLocked = -1;
  float mFontSize = 13.0f;
};

enum class Icon
{
  steps,
  info,
  lock,
  left,
  right,
  audio,
  globe,
  download,
};
juce::Path iconPath(Icon icon, juce::Rectangle<float> r);

class IconButton : public juce::Component, public juce::SettableTooltipClient
{
public:
  IconButton(Icon icon, const juce::String& tip) : mIcon(icon) { setTooltip(tip); }
  void setOn(bool on) { if (on != mOn) { mOn = on; repaint(); } }
  void paint(juce::Graphics&) override;
  void mouseUp(const juce::MouseEvent& e) override { if (onClick && contains(e.getPosition())) onClick(); }
  void mouseEnter(const juce::MouseEvent&) override { mHover = true; repaint(); }
  void mouseExit(const juce::MouseEvent&) override { mHover = false; repaint(); }
  std::function<void()> onClick;

private:
  Icon mIcon;
  bool mOn = false, mHover = false;
};

// Texto clicável que acende ao passar o mouse ("trocar pacote").
class TextLink : public juce::Component
{
public:
  explicit TextLink(juce::String text) : mText(std::move(text)) { setMouseCursor(juce::MouseCursor::PointingHandCursor); }
  void setText(const juce::String& t) { if (t != mText) { mText = t; repaint(); } }
  int idealWidth() const;
  void paint(juce::Graphics&) override;
  void mouseUp(const juce::MouseEvent& e) override { if (onClick && contains(e.getPosition())) onClick(); }
  void mouseEnter(const juce::MouseEvent&) override { mHover = true; repaint(); }
  void mouseExit(const juce::MouseEvent&) override { mHover = false; repaint(); }
  std::function<void()> onClick;

private:
  juce::String mText;
  bool mHover = false;
};
} // namespace nexus::ui
