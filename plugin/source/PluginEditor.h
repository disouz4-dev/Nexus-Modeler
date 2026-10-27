#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

#include <array>

#include "PluginProcessor.h"
#include "ui/Controls.h"
#include "ui/Gear.h"

// Tela do Nexus Modeler. De cima para baixo: barra com a marca e o motor;
// o palco, com o equipamento escolhido renderizado em 3D sob um foco de luz;
// o nome do equipamento, o pacote e o trilho das capturas; e o caminho do
// sinal: entrada, pedal, amplificador, caixa e saída.
class NexusEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
  static constexpr int kWidth = 1200;
  static constexpr int kHeight = 800;
  static constexpr int kPedalLooks = 3;

  enum class Stage
  {
    pedal,
    amp,
    cab
  };

  explicit NexusEditor(NexusProcessor&);
  ~NexusEditor() override;

  void paint(juce::Graphics&) override;
  void resized() override;
  void showStage(Stage s);
  Stage stage() const { return mStage; }

private:
  // Pedal ou amplificador: imagem, botões renderizados e, no pedal, o pisante.
  class GearStage : public juce::Component
  {
  public:
    GearStage(NexusProcessor& p, NexusProcessor::Block block);
    void setArt(const nexus::ui::GearArt* art);
    void paint(juce::Graphics&) override;
    void paintOverChildren(juce::Graphics&) override;
    void resized() override;
    void mouseUp(const juce::MouseEvent&) override;
    void mouseMove(const juce::MouseEvent&) override;

  private:
    juce::String paramFor(const juce::String& knobId) const;
    bool pedalOn() const;
    juce::Point<float> toScreen(juce::Point<float> imagePx) const;
    bool overFootswitch(juce::Point<float> p) const;

    NexusProcessor& mProcessor;
    NexusProcessor::Block mBlock;
    const nexus::ui::GearArt* mArt = nullptr;
    std::vector<std::unique_ptr<nexus::ui::FilmKnob>> mKnobs;
    juce::Rectangle<float> mImage;
    float mScale = 1.0f;
  };

  // Caixa grande, com o seletor Desligada / Captura / IR próprio.
  class CabStage : public juce::Component
  {
  public:
    CabStage(NexusProcessor& p, const nexus::ui::GearArt& art);
    void paint(juce::Graphics&) override;
    void resized() override;
    void refresh();
    nexus::ui::Segmented& modes() { return mModes; }

  private:
    NexusProcessor& mProcessor;
    const nexus::ui::GearArt& mArt;
    nexus::ui::Segmented mModes{{"Desligada", "Captura", juce::String::fromUTF8("IR próprio")}};
    juce::Rectangle<float> mImage;
    bool mDim = false;
  };

  // Trilho das capturas do bloco: um ponto por captura, aceso pelo peso.
  class CaptureRail : public juce::Component
  {
  public:
    explicit CaptureRail(NexusProcessor& p) : mProcessor(p) {}
    void setBlock(NexusProcessor::Block b) { mBlock = b; repaint(); }
    void paint(juce::Graphics&) override;

  private:
    NexusProcessor& mProcessor;
    NexusProcessor::Block mBlock = NexusProcessor::amp;
  };

  // Caminho do sinal: entrada, pedal, amplificador, caixa, saída.
  class SignalPath : public juce::Component
  {
  public:
    explicit SignalPath(NexusEditor& editor);
    void paint(juce::Graphics&) override;
    void resized() override;
    void mouseUp(const juce::MouseEvent&) override;
    void mouseMove(const juce::MouseEvent&) override;
    void mouseExit(const juce::MouseEvent&) override { mHover = -1; repaint(); }
    nexus::ui::LevelFader& input() { return *mInput; }
    nexus::ui::LevelFader& output() { return *mOutput; }

  private:
    int cardAt(juce::Point<float> p) const;
    NexusEditor& mEditor;
    std::unique_ptr<nexus::ui::LevelFader> mInput, mOutput;
    std::array<juce::Rectangle<float>, 3> mCards;
    int mHover = -1;
  };

  void timerCallback() override;
  void choosePack(NexusProcessor::Block b);
  void chooseIr();
  void showAbout();
  void cyclePedalLook(int delta);
  int pedalLook() const;
  const nexus::ui::GearArt* pedalArt() const;
  void layoutTitle();

  NexusProcessor& mProcessor;
  juce::LookAndFeel_V4 mLook;
  juce::TooltipWindow mTooltips{this, 600};
  nexus::ui::GearArt mAmpArt, mCabArt;
  std::array<nexus::ui::GearArt, kPedalLooks> mPedalArts;
  Stage mStage = Stage::amp;
  int mShownPedalLook = -1;

  nexus::ui::Segmented mEngine{{"A2 Full", "A2 Lite"}};
  nexus::ui::IconButton mStepsButton{nexus::ui::Icon::steps, juce::String::fromUTF8("Botões em degraus: param só nas posições capturadas")};
  nexus::ui::IconButton mInfoButton{nexus::ui::Icon::info, "Sobre o Nexus Modeler"};
  std::unique_ptr<GearStage> mPedalStage, mAmpStage;
  std::unique_ptr<CabStage> mCabStage;
  std::unique_ptr<CaptureRail> mRail;
  std::unique_ptr<SignalPath> mPath;
  nexus::ui::TextLink mChangeLink{"trocar pacote"};
  nexus::ui::IconButton mPrevLook{nexus::ui::Icon::left, "Visual anterior do pedal"};
  nexus::ui::IconButton mNextLook{nexus::ui::Icon::right, juce::String::fromUTF8("Próximo visual do pedal")};
  std::unique_ptr<juce::FileChooser> mChooser;
  juce::String mTitle, mSubtitle;
  juce::Rectangle<int> mTitleArea;
  float mInPeak = 0.0f, mOutPeak = 0.0f;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(NexusEditor)
};
