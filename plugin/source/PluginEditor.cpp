#include "PluginEditor.h"

#include "StandaloneLink.h"
#include "ui/Brand.h"
#include "ui/Theme.h"

using namespace nexus::ui;
using Block = NexusProcessor::Block;

namespace
{
constexpr int kTopBar = 68;
constexpr int kPathHeight = 132;
constexpr int kTitleHeight = 112;

const char* const kPedalPrefixes[NexusEditor::kPedalLooks] = {"verde_drive", "laranja_dist", "ouro"};

juce::Rectangle<float> fitImage(const juce::Image& img, juce::Rectangle<float> area)
{
  if (!img.isValid())
    return {};
  return juce::RectanglePlacement(juce::RectanglePlacement::centred).appliedTo(img.getBounds().toFloat(), area);
}

void drawGear(juce::Graphics& g, const juce::Image& img, juce::Rectangle<float> r, float alpha = 1.0f)
{
  if (!img.isValid())
    return;
  g.setImageResamplingQuality(juce::Graphics::highResamplingQuality);
  g.setOpacity(alpha);
  g.drawImage(img, r, juce::RectanglePlacement::stretchToFit);
  g.setOpacity(1.0f);
}

juce::String knobLabel(const juce::String& id)
{
  if (id == "ganho") return "Ganho";
  if (id == "grave") return "Grave";
  if (id == "medio") return juce::String::fromUTF8("Médio");
  if (id == "agudo") return "Agudo";
  if (id == "presenca") return juce::String::fromUTF8("Presença");
  if (id == "tom") return "Tom";
  if (id == "nivel") return juce::String::fromUTF8("Nível");
  return "Volume";
}
} // namespace

// ================================================================ GearStage

NexusEditor::GearStage::GearStage(NexusProcessor& p, Block block)
: mProcessor(p)
, mBlock(block)
{
}

juce::String NexusEditor::GearStage::paramFor(const juce::String& knobId) const
{
  if (mBlock == NexusProcessor::pedal)
    return "pedal_" + knobId;
  // O "volume" do painel é o volume geral do amplificador; a saída do plugin
  // fica no caminho do sinal. O ganho é de cada canal.
  if (knobId == "volume")
    return "master";
  if (knobId == "ganho")
  {
    const int ch = mProcessor.selectedChannel();
    return ch == 0 ? juce::String("ganho") : "ganho_" + juce::String(ch + 1);
  }
  return knobId;
}

void NexusEditor::GearStage::setArt(const GearArt* art)
{
  mKnobs.clear();
  mArt = art;
  if (art != nullptr)
    for (const auto& k : art->knobs)
      if (auto* param = mProcessor.params.getParameter(paramFor(k.id)))
      {
        auto knob = std::make_unique<FilmKnob>(*param, k, knobLabel(k.id));
        knob->onChange = [this] { repaint(); };
        addAndMakeVisible(*knob);
        mKnobs.push_back(std::move(knob));
      }
  resized();
  repaint();
}

bool NexusEditor::GearStage::pedalOn() const
{
  return mBlock != NexusProcessor::pedal || mProcessor.params.getRawParameterValue("pedal_ligado")->load() > 0.5f;
}

juce::Point<float> NexusEditor::GearStage::toScreen(juce::Point<float> px) const
{
  return {mImage.getX() + px.x * mScale, mImage.getY() + px.y * mScale};
}

bool NexusEditor::GearStage::overFootswitch(juce::Point<float> p) const
{
  if (mArt == nullptr || mArt->footswitchRadius <= 0.0f)
    return false;
  return p.getDistanceFrom(toScreen(mArt->footswitch)) <= mArt->footswitchRadius * mScale * 1.15f;
}

void NexusEditor::GearStage::resized()
{
  if (mArt == nullptr)
    return;
  mImage = fitImage(mArt->front, getLocalBounds().toFloat().reduced(24.0f, 18.0f));
  mScale = mImage.getWidth() / static_cast<float>(std::max(1, mArt->front.getWidth()));
  for (auto& knob : mKnobs)
  {
    const auto& box = knob->art().box;
    knob->setBounds(juce::Rectangle<float>(mImage.getX() + box.getX() * mScale, mImage.getY() + box.getY() * mScale,
                                           box.getWidth() * mScale, box.getHeight() * mScale)
                      .toNearestInt());
  }
}

void NexusEditor::GearStage::paint(juce::Graphics& g)
{
  if (mArt == nullptr || !mArt->valid())
  {
    drawCaps(g, juce::String::fromUTF8("visual ainda não renderizado"), getLocalBounds().toFloat(), 12.0f,
             juce::Justification::centred, colour::faint);
    return;
  }
  // Pedal desligado: o próprio pedal (e os botões) ficam um pouco na sombra.
  const bool on = pedalOn();
  drawGear(g, mArt->front, mImage, on ? 1.0f : 0.62f);
  for (auto& k : mKnobs)
    if (k->getAlpha() != (on ? 1.0f : 0.62f))
      k->setAlpha(on ? 1.0f : 0.62f);
}

void NexusEditor::GearStage::paintOverChildren(juce::Graphics& g)
{
  // Pedal desligado: LED apagado.
  if (!pedalOn() && mArt != nullptr)
  {
    if (mArt->ledRadius > 0.0f)
    {
      const auto c = toScreen(mArt->led);
      const float r = mArt->ledRadius * mScale * 1.25f;
      g.setGradientFill(juce::ColourGradient(juce::Colour(0xff2a0805), c.x, c.y - r * 0.4f, juce::Colour(0xff120302), c.x, c.y + r, true));
      g.fillEllipse(c.x - r, c.y - r, r * 2.0f, r * 2.0f);
    }
  }

  for (const auto& k : mKnobs)
    if (k->showsBubble())
    {
      const auto text = k->bubbleText();
      const auto font = Fonts::get().ui(12.5f);
      const float w = static_cast<float>(juce::GlyphArrangement::getStringWidthInt(font, text)) + 24.0f;
      const auto kb = k->getBounds().toFloat();
      juce::Rectangle<float> pill(kb.getCentreX() - w * 0.5f, kb.getY() - 34.0f, w, 24.0f);
      g.setColour(juce::Colour(0xf00c0c0e));
      g.fillRoundedRectangle(pill, 6.0f);
      g.setColour(colour::accent);
      g.fillRect(pill.getX() + 8.0f, pill.getBottom() - 2.0f, pill.getWidth() - 16.0f, 1.5f);
      g.setColour(colour::text);
      g.setFont(font);
      g.drawText(text, pill, juce::Justification::centred);
    }
}

void NexusEditor::GearStage::mouseMove(const juce::MouseEvent& e)
{
  setMouseCursor(overFootswitch(e.position) ? juce::MouseCursor::PointingHandCursor : juce::MouseCursor::NormalCursor);
}

void NexusEditor::GearStage::mouseUp(const juce::MouseEvent& e)
{
  if (mBlock == NexusProcessor::pedal && overFootswitch(e.position))
  {
    auto* on = mProcessor.params.getParameter("pedal_ligado");
    on->setValueNotifyingHost(on->getValue() > 0.5f ? 0.0f : 1.0f);
    repaint();
  }
}

// ================================================================ CabStage

NexusEditor::CabStage::CabStage(NexusProcessor& p, const GearArt& art)
: mProcessor(p)
, mArt(art)
{
  addAndMakeVisible(mModes);
  mModes.setFontSize(12.0f);
  mModes.onSelect = [this](int i) {
    if (auto* param = mProcessor.params.getParameter("caixa"))
      param->setValueNotifyingHost(i == 2 ? 1.0f : 0.0f);
    refresh();
  };
}

void NexusEditor::CabStage::resized()
{
  auto area = getLocalBounds().toFloat();
  mModes.setBounds(area.removeFromTop(56.0f).withSizeKeepingCentre(420.0f, 36.0f).translated(0.0f, 6.0f).toNearestInt());
  mImage = fitImage(mArt.front, area.reduced(24.0f, 8.0f));
}

void NexusEditor::CabStage::paint(juce::Graphics& g)
{
  drawGear(g, mArt.front, mImage, mDim ? 0.42f : 1.0f);
}

void NexusEditor::CabStage::refresh()
{
  const bool fullRig = mProcessor.packIncludesCab();
  const bool ir = mProcessor.params.getRawParameterValue("caixa")->load() > 0.5f;
  mModes.setItemEnabled(0, !fullRig);
  mModes.setItemEnabled(1, fullRig);
  mModes.setItemEnabled(2, !fullRig);
  mModes.setLockedItem(fullRig ? 1 : -1);
  mModes.setSelected(fullRig ? 1 : (ir ? 2 : 0));
  const bool dim = !fullRig && (!ir || !mProcessor.irLoaded());
  if (dim != mDim)
  {
    mDim = dim;
    repaint();
  }
}

// ================================================================ CaptureRail

void NexusEditor::CaptureRail::paint(juce::Graphics& g)
{
  auto rail = getLocalBounds().toFloat();
  const int count = mProcessor.captureCount(mBlock);
  const float y = rail.getCentreY();
  g.setColour(colour::line);
  g.fillRect(rail.getX(), y - 0.5f, rail.getWidth(), 1.0f);
  if (count == 0)
    return;
  const float lo = mProcessor.capturePosition(0, mBlock);
  const float hi = mProcessor.capturePosition(count - 1, mBlock);
  const float span = std::max(hi - lo, 1e-3f);
  auto xOf = [&](float pos) { return rail.getX() + 6.0f + (pos - lo) / span * (rail.getWidth() - 12.0f); };
  for (int i = 0; i < count; ++i)
  {
    const float w = mProcessor.captureWeight(i, mBlock);
    const float x = xOf(mProcessor.capturePosition(i, mBlock));
    if (w > 0.01f)
    {
      g.setColour(colour::accent.withAlpha(0.22f * w));
      g.fillEllipse(x - 8.0f, y - 8.0f, 16.0f, 16.0f);
    }
    // Losango: cada captura é uma "parada" do botão.
    const float r = 2.5f + 2.5f * w;
    juce::Path d;
    d.addQuadrilateral(x, y - r, x + r, y, x, y + r, x - r, y);
    g.setColour(w > 0.01f ? colour::accent : colour::faint);
    g.fillPath(d);
  }
  const float x = xOf(mProcessor.currentPosition(mBlock));
  g.setColour(colour::selected);
  g.fillRect(x - 0.75f, y - 9.0f, 1.5f, 18.0f);
}

// ================================================================ SignalPath

NexusEditor::SignalPath::SignalPath(NexusEditor& editor)
: mEditor(editor)
{
  mInput = std::make_unique<LevelFader>(*editor.mProcessor.params.getParameter("entrada"), "Entrada");
  mOutput = std::make_unique<LevelFader>(*editor.mProcessor.params.getParameter("volume"), juce::String::fromUTF8("Saída"));
  addAndMakeVisible(*mInput);
  addAndMakeVisible(*mOutput);
}

void NexusEditor::SignalPath::resized()
{
  auto r = getLocalBounds().toFloat().reduced(32.0f, 0.0f);
  const float cy = r.getCentreY() - 6.0f;
  mInput->setBounds(juce::Rectangle<float>(r.getX(), cy - 21.0f, 200.0f, 42.0f).toNearestInt());
  mOutput->setBounds(juce::Rectangle<float>(r.getRight() - 200.0f, cy - 21.0f, 200.0f, 42.0f).toNearestInt());

  const float h = 70.0f;
  auto widthFor = [&](const juce::Image& img, float fallback) {
    return img.isValid() ? h * static_cast<float>(img.getWidth()) / static_cast<float>(img.getHeight()) : fallback;
  };
  const auto* pedal = mEditor.pedalArt();
  const float wP = widthFor(pedal ? pedal->front : juce::Image(), 58.0f);
  const float wA = widthFor(mEditor.mAmpArt.front, 160.0f);
  const float wC = widthFor(mEditor.mCabArt.front, 70.0f);
  const float gap = 56.0f;
  float x = r.getCentreX() - (wP + wA + wC + 2.0f * gap) * 0.5f;
  mCards[0] = {x, cy - h * 0.5f, wP, h};
  x += wP + gap;
  mCards[1] = {x, cy - h * 0.5f, wA, h};
  x += wA + gap;
  mCards[2] = {x, cy - h * 0.5f, wC, h};
}

void NexusEditor::SignalPath::paint(juce::Graphics& g)
{
  auto r = getLocalBounds().toFloat();
  g.setColour(juce::Colour(0xff0c0c0e));
  g.fillRect(r);
  g.setColour(colour::line);
  g.fillRect(r.getX(), r.getY(), r.getWidth(), 1.0f);

  // Linha do sinal, com setinhas entre as peças.
  const float cy = mCards[0].getCentreY();
  const float x0 = static_cast<float>(mInput->getRight()) + 18.0f;
  const float x1 = static_cast<float>(mOutput->getX()) - 18.0f;
  g.setColour(colour::line.brighter(0.2f));
  g.fillRect(x0, cy - 0.5f, x1 - x0, 1.0f);
  auto arrow = [&](float x) {
    juce::Path a;
    a.addTriangle(x - 3.0f, cy - 4.0f, x - 3.0f, cy + 4.0f, x + 4.0f, cy);
    g.setColour(colour::dim);
    g.fillPath(a);
  };
  arrow(x0 + 10.0f);
  arrow((mCards[0].getRight() + mCards[1].getX()) * 0.5f);
  arrow((mCards[1].getRight() + mCards[2].getX()) * 0.5f);
  arrow(x1 - 10.0f);

  const auto& proc = mEditor.mProcessor;
  const bool pedalOn = proc.params.getRawParameterValue("pedal_ligado")->load() > 0.5f;
  const bool fullRig = proc.packIncludesCab();
  const bool cabOn = fullRig || (proc.params.getRawParameterValue("caixa")->load() > 0.5f && proc.irLoaded());
  const auto* pedal = mEditor.pedalArt();
  const juce::Image images[3] = {pedal ? pedal->front : juce::Image(), mEditor.mAmpArt.front, mEditor.mCabArt.front};
  const juce::String names[3] = {"Pedal", "Amplificador", "Caixa"};
  const bool active[3] = {pedalOn && proc.packLoaded(NexusProcessor::pedal), true, cabOn};
  const Stage stages[3] = {Stage::pedal, Stage::amp, Stage::cab};

  for (int i = 0; i < 3; ++i)
  {
    const auto card = mCards[static_cast<size_t>(i)];
    const bool selected = mEditor.mStage == stages[i];
    // Fundo escuro atrás da miniatura, para a linha do sinal "entrar" nela.
    g.setColour(juce::Colour(0xff0c0c0e));
    g.fillRect(card.expanded(8.0f, 0.0f));
    if (selected || mHover == i)
    {
      g.setColour(colour::text.withAlpha(selected ? 0.06f : 0.035f));
      g.fillRoundedRectangle(card.expanded(8.0f, 6.0f), 8.0f);
    }
    drawGear(g, images[i], fitImage(images[i], card), active[i] ? 1.0f : 0.38f);
    auto label = juce::Rectangle<float>(card.getCentreX() - 80.0f, card.getBottom() + 10.0f, 160.0f, 14.0f);
    juce::String text = names[i];
    if (i == 0 && !proc.packLoaded(NexusProcessor::pedal))
      text << juce::String::fromUTF8(" · vazio");
    else if (!active[i])
      text << " · desligado";
    drawCaps(g, text, label, 10.0f, juce::Justification::centred, selected ? colour::selected : colour::dim);
    if (selected)
    {
      g.setColour(colour::accent);
      g.fillRoundedRectangle(card.getCentreX() - 14.0f, label.getBottom() + 5.0f, 28.0f, 2.0f, 1.0f);
    }
  }
}

int NexusEditor::SignalPath::cardAt(juce::Point<float> p) const
{
  for (int i = 0; i < 3; ++i)
    if (mCards[static_cast<size_t>(i)].expanded(8.0f, 8.0f).contains(p))
      return i;
  return -1;
}

void NexusEditor::SignalPath::mouseMove(const juce::MouseEvent& e)
{
  const int h = cardAt(e.position);
  setMouseCursor(h >= 0 ? juce::MouseCursor::PointingHandCursor : juce::MouseCursor::NormalCursor);
  if (h != mHover)
  {
    mHover = h;
    repaint();
  }
}

void NexusEditor::SignalPath::mouseUp(const juce::MouseEvent& e)
{
  static const Stage stages[3] = {Stage::pedal, Stage::amp, Stage::cab};
  const int i = cardAt(e.position);
  if (i >= 0)
    mEditor.showStage(stages[i]);
}

// ================================================================ Editor

NexusEditor::NexusEditor(NexusProcessor& p)
: AudioProcessorEditor(&p)
, mProcessor(p)
{
  mLook.setColour(juce::PopupMenu::backgroundColourId, juce::Colour(0xff1b1b1b));
  mLook.setColour(juce::PopupMenu::textColourId, colour::text);
  mLook.setColour(juce::TooltipWindow::backgroundColourId, juce::Colour(0xff1b1b1d));
  mLook.setColour(juce::TooltipWindow::textColourId, colour::text);
  mLook.setColour(juce::TooltipWindow::outlineColourId, colour::boxBorder);
  mLook.setColour(juce::AlertWindow::backgroundColourId, juce::Colour(0xff17171a));
  mLook.setColour(juce::AlertWindow::textColourId, colour::text);
  mLook.setColour(juce::AlertWindow::outlineColourId, colour::boxBorder);
  mLook.setColour(juce::ComboBox::backgroundColourId, juce::Colour(0xff111113));
  mLook.setColour(juce::ComboBox::outlineColourId, colour::boxBorder);
  mLook.setColour(juce::ComboBox::textColourId, colour::text);
  mLook.setColour(juce::ComboBox::arrowColourId, colour::dim);
  mLook.setColour(juce::ComboBox::focusedOutlineColourId, colour::accent);
  mLook.setColour(juce::PopupMenu::highlightedBackgroundColourId, juce::Colour(0xff23384a));
  mLook.setColour(juce::PopupMenu::highlightedTextColourId, colour::selected);
  mLook.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff1d1f23));
  mLook.setColour(juce::TextButton::textColourOffId, colour::text);
  mLook.setColour(juce::ComboBox::buttonColourId, juce::Colour(0xff1d1f23));
  mLook.setDefaultSansSerifTypeface(juce::Typeface::createSystemTypefaceFor(BinaryData::inter_ttf, BinaryData::inter_ttfSize));
  setLookAndFeel(&mLook);

  mAmpArt = GearArt::load("brit800");
  mCabArt = GearArt::load("brit412");
  for (int i = 0; i < kPedalLooks; ++i)
    mPedalArts[static_cast<size_t>(i)] = GearArt::load(kPedalPrefixes[i]);

  addAndMakeVisible(mEngine);
  mEngine.setFontSize(11.5f);
  mEngine.onSelect = [this](int i) { mProcessor.params.getParameter("lite")->setValueNotifyingHost(i == 1 ? 1.0f : 0.0f); };
  addAndMakeVisible(mStepsButton);
  addAndMakeVisible(mInfoButton);
  mStepsButton.onClick = [this] {
    auto* param = mProcessor.params.getParameter("degraus");
    param->setValueNotifyingHost(param->getValue() > 0.5f ? 0.0f : 1.0f);
  };
  mInfoButton.onClick = [this] { showAbout(); };

  // Programa independente: o Nexus cuida da interface de áudio.
  if (auto& link = nexus::standaloneLink(); link.devices != nullptr)
  {
    addAndMakeVisible(mAudioButton);
    mAudioButton.onClick = [this] { showAudioPanel(true); };
    mAudioPanel = std::make_unique<AudioPanel>(
      *link.devices, [] { if (auto& l = nexus::standaloneLink(); l.saveDeviceState) l.saveDeviceState(); },
      [this] { return mProcessor.getLatencySamples(); }, [this] { return mInPeak; });
    mAudioPanel->onClose = [this] { showAudioPanel(false); };
    addChildComponent(*mAudioPanel);
  }

  mPedalStage = std::make_unique<GearStage>(p, NexusProcessor::pedal);
  mAmpStage = std::make_unique<GearStage>(p, NexusProcessor::amp);
  mCabStage = std::make_unique<CabStage>(p, mCabArt);
  mAmpStage->setArt(&mAmpArt);
  addChildComponent(*mPedalStage);
  addAndMakeVisible(*mAmpStage);
  addChildComponent(*mCabStage);

  addChildComponent(mChannels);
  mChannels.setFontSize(11.5f);
  mChannels.onSelect = [this](int item) {
    const int ch = mChannelOfItem[static_cast<size_t>(item)];
    if (ch < 0)
    {
      // "+ canal": o primeiro canal livre recebe um pacote.
      for (int c = 0; c < NexusProcessor::kChannels; ++c)
        if (!mProcessor.channelLoaded(c))
        {
          choosePack(NexusProcessor::amp, c);
          break;
        }
      return;
    }
    auto* param = mProcessor.params.getParameter("canal");
    param->setValueNotifyingHost(param->convertTo0to1(static_cast<float>(ch)));
    timerCallback();
  };

  mRail = std::make_unique<CaptureRail>(p);
  addAndMakeVisible(*mRail);
  addAndMakeVisible(mChangeLink);
  mChangeLink.onClick = [this] {
    if (mStage == Stage::cab)
      chooseIr();
    else
      choosePack(mStage == Stage::pedal ? NexusProcessor::pedal : NexusProcessor::amp);
  };
  addChildComponent(mPrevLook);
  addChildComponent(mNextLook);
  mPrevLook.onClick = [this] { cyclePedalLook(-1); };
  mNextLook.onClick = [this] { cyclePedalLook(1); };

  mPath = std::make_unique<SignalPath>(*this);
  addAndMakeVisible(*mPath);

  setSize(kWidth, kHeight);
  timerCallback();
  startTimerHz(30);
}

NexusEditor::~NexusEditor()
{
  stopTimer();
  setLookAndFeel(nullptr);
}

int NexusEditor::pedalLook() const
{
  return juce::jlimit(0, kPedalLooks - 1, juce::roundToInt(mProcessor.params.getRawParameterValue("pedal_modelo")->load()));
}

const GearArt* NexusEditor::pedalArt() const
{
  const auto& art = mPedalArts[static_cast<size_t>(pedalLook())];
  return art.valid() ? &art : nullptr;
}

void NexusEditor::cyclePedalLook(int delta)
{
  auto* param = mProcessor.params.getParameter("pedal_modelo");
  const int next = (pedalLook() + delta + kPedalLooks) % kPedalLooks;
  param->setValueNotifyingHost(param->convertTo0to1(static_cast<float>(next)));
  timerCallback();
}

void NexusEditor::showStage(Stage s)
{
  mStage = s;
  mPedalStage->setVisible(s == Stage::pedal);
  mAmpStage->setVisible(s == Stage::amp);
  mCabStage->setVisible(s == Stage::cab);
  mCabStage->refresh();
  mRail->setVisible(s != Stage::cab);
  mRail->setBlock(s == Stage::pedal ? NexusProcessor::pedal : NexusProcessor::amp);
  mPrevLook.setVisible(s == Stage::pedal);
  mNextLook.setVisible(s == Stage::pedal);
  timerCallback();
  mPath->repaint();
  repaint();
}

void NexusEditor::choosePack(Block b, int channel)
{
  mChooser = std::make_unique<juce::FileChooser>(b == NexusProcessor::pedal ? "Escolha a pasta do pacote do pedal"
                                                                            : "Escolha a pasta do pacote do amplificador",
                                                 juce::File(), "");
  mChooser->launchAsync(juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectDirectories,
                        [this, b, channel](const juce::FileChooser& fc) {
                          const auto folder = fc.getResult();
                          if (!folder.isDirectory())
                            return;
                          mProcessor.loadPackAsync(folder, b, channel);
                          if (b == NexusProcessor::amp && channel >= 0)
                          {
                            auto* param = mProcessor.params.getParameter("canal");
                            param->setValueNotifyingHost(param->convertTo0to1(static_cast<float>(channel)));
                          }
                        });
}

void NexusEditor::chooseIr()
{
  if (mProcessor.packIncludesCab())
    return;
  mChooser = std::make_unique<juce::FileChooser>("Escolha o IR da caixa", juce::File(), "*.wav;*.aif;*.aiff");
  mChooser->launchAsync(juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                        [this](const juce::FileChooser& fc) {
                          const auto file = fc.getResult();
                          if (file.existsAsFile())
                          {
                            mProcessor.loadIrAsync(file);
                            if (auto* param = mProcessor.params.getParameter("caixa"))
                              param->setValueNotifyingHost(1.0f);
                            showStage(Stage::cab);
                          }
                        });
}

void NexusEditor::refreshChannels()
{
  // Só os canais com pacote aparecem; "+ canal" enquanto houver vaga.
  juce::StringArray items;
  std::vector<int> map;
  int selectedItem = -1;
  for (int c = 0; c < NexusProcessor::kChannels; ++c)
    if (mProcessor.channelLoaded(c))
    {
      if (c == mProcessor.selectedChannel())
        selectedItem = items.size();
      items.add(mProcessor.channelName(c));
      map.push_back(c);
    }
  if (!items.isEmpty() && items.size() < NexusProcessor::kChannels)
  {
    items.add("+ canal");
    map.push_back(-1);
  }
  mChannelOfItem = map;
  mChannels.setItems(items);
  mChannels.setSelected(selectedItem);
  mChannels.setVisible(mStage == Stage::amp && !items.isEmpty());
  const int width = 150 * items.size();
  mChannels.setBounds(getLocalBounds().withTrimmedTop(kTopBar).removeFromTop(52).withSizeKeepingCentre(width, 34).translated(0, 6));

  // O botão de ganho do painel segue o canal escolhido.
  if (mProcessor.selectedChannel() != mShownChannel)
  {
    mShownChannel = mProcessor.selectedChannel();
    mAmpStage->setArt(&mAmpArt);
  }
}

void NexusEditor::showAudioPanel(bool show)
{
  if (!mAudioPanel)
    return;
  mAudioPanel->setVisible(show);
  if (show)
    mAudioPanel->toFront(true);
  mAudioButton.setOn(show);
}

void NexusEditor::showAbout()
{
  juce::AlertWindow::showMessageBoxAsync(
    juce::MessageBoxIconType::NoIcon, "Nexus Modeler",
    juce::String::fromUTF8("Plugin de guitarra gratuito e de código aberto (GPL v3).\n"
                           "Toca capturas NAM A2 e troca de captura conforme o botão gira.\n\n"
                           "github.com/disouz4-dev/Nexus-Modeler"));
}

void NexusEditor::timerCallback()
{
  const bool lite = mProcessor.params.getParameter("lite")->getValue() > 0.5f;
  mEngine.setSelected(lite ? 1 : 0);
  mStepsButton.setOn(mProcessor.params.getParameter("degraus")->getValue() > 0.5f);

  refreshChannels();

  // O visual do pedal pode mudar pelo programa de gravação (automação).
  if (pedalLook() != mShownPedalLook)
  {
    mShownPedalLook = pedalLook();
    mPedalStage->setArt(pedalArt());
    mPath->resized();
    mPath->repaint();
  }

  juce::String title, subtitle, link;
  if (mStage == Stage::amp)
  {
    title = mAmpArt.name;
    subtitle = mProcessor.packLoaded() ? mProcessor.statusText() : juce::String::fromUTF8("Nenhum pacote de capturas carregado");
    link = mProcessor.packLoaded() ? "trocar pacote" : "abrir pacote";
  }
  else if (mStage == Stage::pedal)
  {
    const auto* art = pedalArt();
    title = art ? art->name : juce::String("Pedal");
    const bool loaded = mProcessor.packLoaded(NexusProcessor::pedal);
    subtitle = loaded ? mProcessor.statusText(NexusProcessor::pedal)
                      : juce::String::fromUTF8("Nenhuma captura no pedal. O pisante liga e desliga.");
    link = loaded ? "trocar pacote" : "abrir pacote do pedal";
  }
  else
  {
    title = mCabArt.name;
    if (mProcessor.packIncludesCab())
    {
      subtitle = juce::String::fromUTF8("Caixa incluída na captura (full rig): o IR fica desligado para respeitar o som original");
      link = {};
    }
    else
    {
      subtitle = mProcessor.irLoaded() ? "IR: " + mProcessor.irName() : juce::String::fromUTF8("Nenhum IR escolhido");
      link = mProcessor.irLoaded() ? "trocar IR" : "escolher IR";
    }
    mCabStage->refresh();
  }
  if (title != mTitle || subtitle != mSubtitle)
  {
    mTitle = title;
    mSubtitle = subtitle;
    repaint(mTitleArea);
  }
  mChangeLink.setText(link);
  mChangeLink.setVisible(link.isNotEmpty());
  layoutTitle();

  // Medidores: pico com queda suave.
  mInPeak = std::max(mProcessor.inputPeak(), mInPeak * 0.8f);
  mOutPeak = std::max(mProcessor.outputPeak(), mOutPeak * 0.8f);
  mProcessor.resetPeaks();
  mPath->input().setLevel(mInPeak);
  mPath->output().setLevel(mOutPeak);
  mRail->repaint();
  mPath->repaint();
  if (mStage == Stage::pedal)
    mPedalStage->repaint();
}

void NexusEditor::layoutTitle()
{
  auto t = mTitleArea;
  t.removeFromTop(8);
  auto titleRow = t.removeFromTop(40);
  auto subRow = t.removeFromTop(22);
  const auto subFont = Fonts::get().ui(12.5f);
  const int subW = juce::GlyphArrangement::getStringWidthInt(subFont, mSubtitle);
  const int linkW = mChangeLink.isVisible() ? mChangeLink.idealWidth() : 0;
  const int total = subW + (linkW > 0 ? linkW + 14 : 0);
  const int x = subRow.getCentreX() - total / 2;
  mChangeLink.setBounds(x + subW + 14, subRow.getY(), linkW, subRow.getHeight());
  const auto titleFont = Fonts::get().display(30.0f).withExtraKerningFactor(0.16f);
  const int titleW = juce::GlyphArrangement::getStringWidthInt(titleFont, upper(mTitle));
  mPrevLook.setBounds(titleRow.getCentreX() - titleW / 2 - 44, titleRow.getCentreY() - 15, 30, 30);
  mNextLook.setBounds(titleRow.getCentreX() + titleW / 2 + 14, titleRow.getCentreY() - 15, 30, 30);
  mRail->setBounds(t.withSizeKeepingCentre(420, 22).translated(0, 4));
}

void NexusEditor::paint(juce::Graphics& g)
{
  g.fillAll(colour::background);
  auto r = getLocalBounds().toFloat();

  // Foco de luz de palco atrás do equipamento.
  auto stage = r.withTrimmedTop(static_cast<float>(kTopBar)).withTrimmedBottom(static_cast<float>(kPathHeight));
  juce::ColourGradient spot(juce::Colour(0xff222a33), stage.getCentreX(), stage.getY() + stage.getHeight() * 0.36f,
                            colour::background, stage.getCentreX() + stage.getWidth() * 0.52f, stage.getY() + stage.getHeight() * 0.36f, true);
  g.setGradientFill(spot);
  g.fillRect(stage);

  // Barra de cima: a marca (triângulo impossível, NEXUS e MODELER).
  auto top = r.removeFromTop(static_cast<float>(kTopBar));
  g.setColour(colour::line);
  g.fillRect(top.getX(), top.getBottom() - 1.0f, top.getWidth(), 1.0f);
  brand::drawPenrose(g, {top.getX() + 26.0f, top.getY() + 15.0f, 40.0f, 38.0f});
  const float wordW = brand::drawWordmark(g, {top.getX() + 80.0f, top.getY() + 19.0f}, 17.0f);
  brand::drawModeler(g, {top.getX() + 80.0f, top.getY() + 42.0f, wordW, 12.0f}, 8.5f, true);

  // Nome do equipamento e situação do pacote.
  auto t = mTitleArea.toFloat();
  t.removeFromTop(8.0f);
  g.setColour(colour::text);
  g.setFont(Fonts::get().display(30.0f).withExtraKerningFactor(0.16f));
  g.drawText(upper(mTitle), t.removeFromTop(40.0f), juce::Justification::centred);
  auto sub = t.removeFromTop(22.0f);
  const auto subFont = Fonts::get().ui(12.5f);
  const float subW = static_cast<float>(juce::GlyphArrangement::getStringWidthInt(subFont, mSubtitle));
  const float linkW = mChangeLink.isVisible() ? static_cast<float>(mChangeLink.idealWidth()) + 14.0f : 0.0f;
  g.setFont(subFont);
  g.setColour(colour::dim);
  g.drawText(mSubtitle, juce::Rectangle<float>(sub.getCentreX() - (subW + linkW) * 0.5f, sub.getY(), subW + 2.0f, sub.getHeight()),
             juce::Justification::centredLeft, false);
}

void NexusEditor::resized()
{
  auto r = getLocalBounds();
  auto top = r.removeFromTop(kTopBar);
  auto right = top.removeFromRight(380).reduced(0, 16);
  mInfoButton.setBounds(right.removeFromRight(56).withSizeKeepingCentre(36, 36));
  mStepsButton.setBounds(right.removeFromRight(44).withSizeKeepingCentre(36, 36));
  if (mAudioPanel)
  {
    mAudioButton.setBounds(right.removeFromRight(44).withSizeKeepingCentre(36, 36));
    mAudioPanel->setBounds(getLocalBounds());
  }
  right.removeFromRight(12);
  mEngine.setBounds(right.removeFromRight(196));

  mPath->setBounds(r.removeFromBottom(kPathHeight));
  mTitleArea = r.removeFromBottom(kTitleHeight);
  mPedalStage->setBounds(r);
  mAmpStage->setBounds(r.withTrimmedTop(46));
  mCabStage->setBounds(r);
  layoutTitle();
}
