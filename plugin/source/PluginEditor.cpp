#include "PluginEditor.h"

namespace
{
const juce::Colour kBackground(0xff121417);
const juce::Colour kPanel(0xff1c2026);
const juce::Colour kAccent(0xffe0a43a);
const juce::Colour kText(0xffe8e6e1);
const juce::Colour kDim(0xff8a8f98);
} // namespace

NexusEditor::NexusEditor(NexusProcessor& p)
: AudioProcessorEditor(&p)
, mProcessor(p)
{
  mLook.setColour(juce::Slider::rotarySliderFillColourId, kAccent);
  mLook.setColour(juce::Slider::rotarySliderOutlineColourId, kPanel.brighter(0.15f));
  mLook.setColour(juce::Slider::thumbColourId, kText);
  mLook.setColour(juce::Slider::textBoxTextColourId, kText);
  mLook.setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
  mLook.setColour(juce::TextButton::buttonColourId, kPanel.brighter(0.1f));
  mLook.setColour(juce::TextButton::textColourOffId, kText);
  mLook.setColour(juce::ToggleButton::textColourId, kText);
  mLook.setColour(juce::ToggleButton::tickColourId, kAccent);
  mLook.setColour(juce::Label::textColourId, kText);
  setLookAndFeel(&mLook);

  addAndMakeVisible(mOpenButton);
  mOpenButton.onClick = [this] {
    mChooser = std::make_unique<juce::FileChooser>("Escolha a pasta do pacote de capturas", juce::File(), "");
    mChooser->launchAsync(juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectDirectories,
                          [this](const juce::FileChooser& fc) {
                            const auto folder = fc.getResult();
                            if (folder.isDirectory())
                              mProcessor.loadPackAsync(folder);
                          });
  };

  mStatus.setColour(juce::Label::textColourId, kDim);
  mStatus.setJustificationType(juce::Justification::centredLeft);
  addAndMakeVisible(mStatus);

  addKnob(mMain, "ganho", "Ganho");
  addKnob(mBass, "grave", "Grave");
  addKnob(mMid, "medio", juce::String::fromUTF8("Médio"));
  addKnob(mTreble, "agudo", "Agudo");
  addKnob(mPresence, "presenca", juce::String::fromUTF8("Presença"));
  addKnob(mVolume, "volume", "Volume (dB)");

  addAndMakeVisible(mStepped);
  addAndMakeVisible(mLite);
  mSteppedAttachment =
    std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(p.params, "degraus", mStepped);
  mLiteAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(p.params, "lite", mLite);

  setSize(780, 440);
  startTimerHz(30);
}

NexusEditor::~NexusEditor()
{
  stopTimer();
  setLookAndFeel(nullptr);
}

void NexusEditor::addKnob(Knob& k, const juce::String& paramId, const juce::String& text)
{
  k.slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
  k.slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 70, 20);
  k.label.setText(text, juce::dontSendNotification);
  k.label.setJustificationType(juce::Justification::centred);
  addAndMakeVisible(k.slider);
  addAndMakeVisible(k.label);
  k.attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(mProcessor.params, paramId, k.slider);
}

void NexusEditor::timerCallback()
{
  mStatus.setText(mProcessor.statusText(), juce::dontSendNotification);
  mMain.label.setText(mProcessor.knobName(), juce::dontSendNotification);
  repaint(mMeterArea);
}

void NexusEditor::paint(juce::Graphics& g)
{
  g.fillAll(kBackground);
  g.setColour(kText);
  g.setFont(juce::FontOptions(22.0f, juce::Font::bold));
  g.drawText("NEXUS MODELER", 20, 14, 300, 28, juce::Justification::centredLeft);
  g.setColour(kDim);
  g.setFont(juce::FontOptions(13.0f));
  g.drawText("prova do motor de troca de capturas", 20, 40, 400, 18, juce::Justification::centredLeft);
  paintCaptureMeter(g, mMeterArea);
}

void NexusEditor::paintCaptureMeter(juce::Graphics& g, juce::Rectangle<int> area)
{
  g.setColour(kPanel);
  g.fillRoundedRectangle(area.toFloat(), 8.0f);
  auto inner = area.reduced(18, 14);
  g.setColour(kDim);
  g.setFont(juce::FontOptions(12.0f));
  g.drawText("Capturas na mistura agora", inner.removeFromTop(16), juce::Justification::centredLeft);

  const int count = mProcessor.captureCount();
  if (count == 0)
    return;
  const float lo = mProcessor.capturePosition(0);
  const float hi = mProcessor.capturePosition(count - 1);
  const float span = std::max(hi - lo, 1e-3f);
  auto track = inner.reduced(10, 6).toFloat();
  const float base = track.getBottom() - 18.0f;
  auto xOf = [&](float pos) { return track.getX() + (pos - lo) / span * track.getWidth(); };

  g.setColour(kDim.withAlpha(0.4f));
  g.drawLine(track.getX(), base, track.getRight(), base, 1.0f);
  for (int i = 0; i < count; ++i)
  {
    const float x = xOf(mProcessor.capturePosition(i));
    const float w = mProcessor.captureWeight(i);
    const float h = (base - track.getY() - 6.0f) * w;
    g.setColour(w > 0.0f ? kAccent : kDim.withAlpha(0.5f));
    g.fillRect(juce::Rectangle<float>(x - 6.0f, base - std::max(h, 2.0f), 12.0f, std::max(h, 2.0f)));
    g.setColour(kDim);
    g.drawText(juce::String(mProcessor.capturePosition(i), 1), juce::Rectangle<float>(x - 20.0f, base + 2.0f, 40.0f, 14.0f),
               juce::Justification::centred);
  }
  const float x = xOf(mProcessor.currentPosition());
  g.setColour(kText);
  g.drawLine(x, track.getY(), x, base, 1.5f);
}

void NexusEditor::resized()
{
  auto area = getLocalBounds().reduced(20);
  auto top = area.removeFromTop(48);
  mOpenButton.setBounds(top.removeFromRight(150).withSizeKeepingCentre(150, 30));
  area.removeFromTop(6);
  mStatus.setBounds(area.removeFromTop(22));
  area.removeFromTop(10);

  auto knobs = area.removeFromTop(150);
  Knob* all[] = {&mMain, &mBass, &mMid, &mTreble, &mPresence, &mVolume};
  const int w = knobs.getWidth() / 6;
  for (auto* k : all)
  {
    auto cell = knobs.removeFromLeft(w);
    k->label.setBounds(cell.removeFromTop(20));
    k->slider.setBounds(cell.reduced(6, 0));
  }
  auto toggles = area.removeFromTop(34);
  mStepped.setBounds(toggles.removeFromLeft(130));
  mLite.setBounds(toggles.removeFromLeft(130));
  area.removeFromTop(8);
  mMeterArea = area;
}
