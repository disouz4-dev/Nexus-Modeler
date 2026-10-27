#include "PluginEditor.h"

#include "StandaloneLink.h"
#include "report/Report.h"
#include "ui/Brand.h"
#include "ui/Language.h"
#include "ui/Theme.h"

using namespace nexus::ui;


namespace
{
constexpr int kTopBar = 68;
constexpr int kPathHeight = 176;
constexpr int kTitleHeight = 112;

const char* const kPedalPrefixes[NexusEditor::kPedalLooks] = {"lemon_drive", "orange_drive", "banana_overdrive", "gold_centaur",
                                                               "blueberry_chorus", "coconut_delay", "grape_reverb", "pear_gate"};
// Cabeçotes, na ordem do parâmetro "cabecote".
const char* const kAmpPrefixes[NexusProcessor::kAmpLooks] = {"brit800", "mark5"};
// Caixas, na ordem do parâmetro "gabinete"; cada cabeçote abre com a sua.
const char* const kCabPrefixes[NexusProcessor::kCabLooks] = {"brit412", "mark5_412"};
const int kCabOfAmp[NexusProcessor::kAmpLooks] = {0, 1};
struct CabInfo
{
  const char* inspiration;
};
const CabInfo kCabInfo[NexusProcessor::kCabLooks] = {{"1960A 4x12"}, {"Mesa 4x12"}};

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

// Rótulos do painel: caixas de texto transparentes por cima da imagem 3D,
// traduzidas, na posição, tamanho e perspectiva anotados pelo gerador.
void drawGearLabels(juce::Graphics& g, const GearArt& art, juce::Rectangle<float> image, float alpha = 1.0f)
{
  if (!art.valid())
    return;
  const float scale = image.getWidth() / static_cast<float>(art.front.getWidth());
  for (const auto& l : art.labels)
  {
    // O tamanho do Blender e o "em" do JUCE não medem igual: 0,74 deixa a
    // letra do tamanho que estava gravada no painel.
    const float em = l.height * scale * 0.74f;
    if (em < 2.0f)
      continue;
    const float kerning = language::allowsLetterSpacing() ? std::max(0.0f, l.spacing - 1.0f) + 0.02f : 0.0f;
    const auto font = Fonts::get().displayEm(em).withExtraKerningFactor(kerning);
    const auto text = upper(TRANS(l.text));
    const juce::Point<float> at(image.getX() + l.centre.x * scale, image.getY() + l.centre.y * scale);
    // Texto desenhado direto (a fonte reserva do sistema cobre japonês, chinês,
    // árabe...), achatado pela perspectiva e um pouco engrossado, como tinta
    // de serigrafia.
    juce::Graphics::ScopedSaveState save(g);
    g.addTransform(juce::AffineTransform::scale(1.0f, l.squash, at.x, at.y));
    g.setFont(font);
    g.setColour(l.colour.withAlpha(0.9f * alpha));
    const auto box = juce::Rectangle<float>(at.x - 400.0f, at.y - em, 800.0f, em * 2.0f);
    const float d = std::max(0.25f, em * 0.018f);
    for (const auto offset : {juce::Point<float>(-d, 0.0f), juce::Point<float>(d, 0.0f), juce::Point<float>(0.0f, 0.0f)})
      g.drawText(text, box.translated(offset.x, offset.y), juce::Justification::centred, false);
  }
}

// LED aceso: halo vermelho suave em volta, núcleo vermelho intenso e um
// ponto quase branco no centro, como um LED de verdade visto de frente.
void drawLed(juce::Graphics& g, juce::Point<float> c, float r)
{
  const juce::Colour red(0xffff2a1c);
  juce::ColourGradient halo(red.withAlpha(0.55f), c.x, c.y, red.withAlpha(0.0f), c.x + r * 4.2f, c.y, true);
  g.setGradientFill(halo);
  g.fillEllipse(c.x - r * 4.2f, c.y - r * 4.2f, r * 8.4f, r * 8.4f);
  juce::ColourGradient core(juce::Colour(0xffffe6dc), c.x, c.y - r * 0.15f, juce::Colour(0xffd80f05), c.x, c.y + r, true);
  core.addColour(0.35, juce::Colour(0xffff4a30));
  g.setGradientFill(core);
  g.fillEllipse(c.x - r, c.y - r, r * 2.0f, r * 2.0f);
  g.setColour(juce::Colours::white.withAlpha(0.85f));
  g.fillEllipse(c.x - r * 0.35f, c.y - r * 0.55f, r * 0.5f, r * 0.35f); // reflexo da cúpula
}

juce::String nativeKnobLabel(int look, const juce::String& id)
{
  static const char* const names[4][3] = {{"Velocidade", "Profundidade", "Mistura"},
                                         {"Tempo", "Repetições", "Mistura"},
                                         {"Tamanho", "Tom", "Mistura"},
                                         {"Sensibilidade", "Ataque", "Release"}};
  const int type = juce::jlimit(0, 3, look - NexusProcessor::kFirstNativeLook);
  const int knob = id == "a" ? 0 : id == "b" ? 1 : 2;
  return juce::String::fromUTF8(names[type][knob]);
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
  // Faixas do equalizador gráfico ("eq750" -> "750 Hz").
  if (id.startsWith("eq")) return id.substring(2) + " Hz";
  return "Volume";
}

// Pedal A/B desenhado: caixa de metal escuro com A e B e um LED no lado ativo.
void drawAbPedal(juce::Graphics& g, juce::Rectangle<float> r, int active, float alpha)
{
  g.setGradientFill(juce::ColourGradient(juce::Colour(0xff3a3f46).withMultipliedAlpha(alpha), r.getX(), r.getY(),
                                         juce::Colour(0xff1d2025).withMultipliedAlpha(alpha), r.getX(), r.getBottom(), false));
  g.fillRoundedRectangle(r, 5.0f);
  g.setColour(colour::boxBorder.withMultipliedAlpha(alpha));
  g.drawRoundedRectangle(r.reduced(0.5f), 5.0f, 1.0f);
  auto top = r.reduced(4.0f).removeFromTop(r.getHeight() * 0.45f);
  for (int side = 0; side < 2; ++side)
  {
    auto half = side == 0 ? top.withWidth(top.getWidth() * 0.5f) : top.withTrimmedLeft(top.getWidth() * 0.5f);
    const auto c = half.getCentre().translated(0.0f, -2.0f);
    const float lr = std::max(2.0f, r.getWidth() * 0.06f);
    if (side == active)
      drawLed(g, c, lr);
    else
    {
      g.setColour(juce::Colour(0xff3a0b07).withMultipliedAlpha(alpha));
      g.fillEllipse(c.x - lr, c.y - lr, lr * 2.0f, lr * 2.0f);
    }
    g.setColour(colour::text.withMultipliedAlpha(alpha));
    g.setFont(Fonts::get().display(std::max(8.0f, r.getHeight() * 0.2f)));
    g.drawText(side == 0 ? "A" : "B", half.withTrimmedTop(half.getHeight() * 0.55f), juce::Justification::centred);
  }
  // Pisante de metal no meio.
  const auto foot = juce::Rectangle<float>(r.getCentreX() - r.getWidth() * 0.16f, r.getBottom() - r.getHeight() * 0.42f,
                                           r.getWidth() * 0.32f, r.getWidth() * 0.32f);
  g.setGradientFill(juce::ColourGradient(juce::Colour(0xffe8e8ea).withMultipliedAlpha(alpha), foot.getX(), foot.getY(),
                                         juce::Colour(0xff7b7f86).withMultipliedAlpha(alpha), foot.getRight(), foot.getBottom(), false));
  g.fillEllipse(foot);
}
} // namespace

// ================================================================ GearStage

NexusEditor::GearStage::GearStage(NexusProcessor& p, Kind kind)
: mProcessor(p)
, mKind(kind)
{
}

juce::String NexusEditor::GearStage::paramFor(const juce::String& knobId) const
{
  if (mKind == Kind::pedal)
  {
    // O botão que troca as capturas do pedal é o "ganho" da receita (drive).
    const auto name = knobId == "ganho" ? juce::String("drive") : knobId;
    return NexusProcessor::pedalParam(mIndex, name.toRawUTF8());
  }
  // O "volume" do painel é o volume geral do cabeçote; o ganho é de cada canal.
  if (knobId == "volume")
    return NexusProcessor::ampParam(mIndex, "master");
  if (knobId == "ganho")
  {
    const int ch = mProcessor.selectedChannel(mIndex);
    return NexusProcessor::ampParam(mIndex, ch == 0 ? "ganho" : (ch == 1 ? "ganho_2" : "ganho_3"));
  }
  return NexusProcessor::ampParam(mIndex, knobId.toRawUTF8());
}

void NexusEditor::GearStage::setTarget(int index, const GearArt* art, bool rebuild)
{
  if (!rebuild && index == mIndex && art == mArt && !mKnobs.empty())
    return;
  mKnobs.clear();
  mIndex = index;
  mArt = art;
  if (art != nullptr)
    for (const auto& k : art->knobs)
      if (auto* param = mProcessor.params.getParameter(paramFor(k.id)))
      {
        // No pedal, o botão que troca as capturas é o drive; nos efeitos do
        // Nexus, os botões "a", "b" e "c" mudam de nome conforme o efeito.
        auto label = mKind == Kind::pedal && k.id == "ganho" ? juce::String("Drive") : knobLabel(k.id);
        if (mKind == Kind::pedal && (k.id == "a" || k.id == "b" || k.id == "c"))
          label = nativeKnobLabel(mProcessor.pedalLook(index), k.id);
        auto knob = std::make_unique<FilmKnob>(*param, k, label);
        knob->onChange = [this] { repaint(); };
        knob->onMidiMenu = [this](const juce::String& id, juce::Component& c) { if (onMidiMenu) onMidiMenu(id, c); };
        addAndMakeVisible(*knob);
        mKnobs.push_back(std::move(knob));
      }
  resized();
  repaint();
}

bool NexusEditor::GearStage::pedalOn() const
{
  return mKind != Kind::pedal || mProcessor.params.getRawParameterValue(NexusProcessor::pedalParam(mIndex, "ligado"))->load() > 0.5f;
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
    drawCaps(g, TRANS(juce::String::fromUTF8("visual ainda não renderizado")), getLocalBounds().toFloat(), 12.0f,
             juce::Justification::centred, colour::faint);
    return;
  }
  // Ligado ou desligado, o pedal tem a mesma luz: só o LED muda.
  drawGear(g, mArt->front, mImage);
  drawGearLabels(g, *mArt, mImage);
}

void NexusEditor::GearStage::paintOverChildren(juce::Graphics& g)
{
  // LED: o modelo 3D traz o LED apagado; ligado, a tela acende por cima.
  if (mKind == Kind::pedal && pedalOn() && mArt != nullptr && mArt->ledRadius > 0.0f)
    drawLed(g, toScreen(mArt->led), mArt->ledRadius * mScale);
  // Cabeçote de vários canais: acende o LED do canal que está tocando.
  if (mKind == Kind::amp && mArt != nullptr)
  {
    const int ch = mProcessor.selectedChannel(mIndex);
    if (ch < static_cast<int>(mArt->channelLeds.size()) && mProcessor.channelLoaded(mIndex, ch))
      drawLed(g, toScreen(mArt->channelLeds[static_cast<size_t>(ch)].first), mArt->channelLeds[static_cast<size_t>(ch)].second * mScale);
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
  if (mKind != Kind::pedal || !overFootswitch(e.position))
    return;
  const auto id = NexusProcessor::pedalParam(mIndex, "ligado");
  if (e.mods.isPopupMenu())
  {
    if (onMidiMenu)
      onMidiMenu(id, *this);
    return;
  }
  auto* on = mProcessor.params.getParameter(id);
  on->setValueNotifyingHost(on->getValue() > 0.5f ? 0.0f : 1.0f);
  repaint();
}

// ================================================================ CabStage

const GearArt& NexusEditor::CabStage::art() const
{
  return mArts[juce::jlimit(0, NexusProcessor::kCabLooks - 1, mProcessor.cabLook(mRig))];
}

const GearArt& NexusEditor::cabArt(int rig) const
{
  return mCabArts[static_cast<size_t>(juce::jlimit(0, NexusProcessor::kCabLooks - 1, mProcessor.cabLook(rig)))];
}

NexusEditor::CabStage::CabStage(NexusProcessor& p, const GearArt* arts)
: mProcessor(p)
, mArts(arts)
{
  addAndMakeVisible(mModes);
  mModes.setFontSize(12.0f);
  mModes.onSelect = [this](int i) {
    if (auto* param = mProcessor.params.getParameter(NexusProcessor::ampParam(mRig, "caixa")))
      param->setValueNotifyingHost(i == 2 ? 1.0f : 0.0f);
    refresh();
  };
}

void NexusEditor::CabStage::resized()
{
  auto area = getLocalBounds().toFloat();
  mModes.setBounds(area.removeFromTop(56.0f).withSizeKeepingCentre(420.0f, 36.0f).translated(0.0f, 6.0f).toNearestInt());
  mImage = fitImage(art().front, area.reduced(24.0f, 8.0f));
}

void NexusEditor::CabStage::paint(juce::Graphics& g)
{
  drawGear(g, art().front, mImage, mDim ? 0.42f : 1.0f);
}

void NexusEditor::CabStage::refresh()
{
  const bool fullRig = mProcessor.packIncludesCab(mRig);
  const bool ir = mProcessor.params.getRawParameterValue(NexusProcessor::ampParam(mRig, "caixa"))->load() > 0.5f;
  mModes.setItems({TRANS("Desligada"), TRANS("Captura"), TRANS(juce::String::fromUTF8("IR próprio"))});
  mModes.setItemEnabled(0, !fullRig);
  mModes.setItemEnabled(1, fullRig);
  mModes.setItemEnabled(2, !fullRig);
  mModes.setLockedItem(fullRig ? 1 : -1);
  mModes.setSelected(fullRig ? 1 : (ir ? 2 : 0));
  const bool dim = !fullRig && (!ir || !mProcessor.irLoaded(mRig));
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
  const int count = mProcessor.captureCount(mSlot);
  const float y = rail.getCentreY();
  g.setColour(colour::line);
  g.fillRect(rail.getX(), y - 0.5f, rail.getWidth(), 1.0f);
  if (count == 0)
    return;
  const float lo = mProcessor.capturePosition(0, mSlot);
  const float hi = mProcessor.capturePosition(count - 1, mSlot);
  const float span = std::max(hi - lo, 1e-3f);
  auto xOf = [&](float pos) { return rail.getX() + 6.0f + (pos - lo) / span * (rail.getWidth() - 12.0f); };
  for (int i = 0; i < count; ++i)
  {
    const float w = mProcessor.captureWeight(i, mSlot);
    const float x = xOf(mProcessor.capturePosition(i, mSlot));
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
  const float x = xOf(mProcessor.currentPosition(mSlot));
  g.setColour(colour::selected);
  g.fillRect(x - 0.75f, y - 9.0f, 1.5f, 18.0f);
}

// ================================================================ ChainView

NexusEditor::ChainView::ChainView(NexusEditor& editor)
: mEditor(editor)
{
  mInput = std::make_unique<LevelFader>(*editor.mProcessor.params.getParameter("entrada"), "Entrada");
  mInput->setMuteParameter(*editor.mProcessor.params.getParameter("entrada_mudo"));
  mOutput = std::make_unique<LevelFader>(*editor.mProcessor.params.getParameter("volume"), juce::String::fromUTF8("Saída"));
  addAndMakeVisible(*mInput);
  addAndMakeVisible(*mOutput);
  mTest = std::make_unique<nexus::ui::TestBar>(editor.mProcessor.testSource());
  addAndMakeVisible(*mTest);
  addAndMakeVisible(mRigs);
  mRigs.setFontSize(11.0f);
  mRigs.onSelect = [this](int i) {
    auto layout = mEditor.mProcessor.layout();
    layout.dual = i == 1;
    if (!layout.dual)
    {
      // Um cabeçote só: os pedais do lado B passam para o lado A.
      for (int p : layout.loop[1])
        layout.loop[0].push_back(p);
      layout.loop[1].clear();
      if (mEditor.mSel.kind != Kind::pedal && mEditor.mSel.index == 1)
        mEditor.select({mEditor.mSel.kind, 0});
    }
    mEditor.mProcessor.setLayout(layout);
    rebuild();
  };
}

std::vector<int>& NexusEditor::ChainView::list(ChainLayout& l, int which)
{
  switch (which)
  {
  case 0:
    return l.pre;
  case 1:
    return l.loop[0];
  case 2:
    return l.loop[1];
  default:
    return l.post;
  }
}

void NexusEditor::ChainView::resized()
{
  auto r = getLocalBounds().toFloat().reduced(32.0f, 0.0f);
  const float cy = r.getCentreY() - 2.0f;
  mInput->setBounds(juce::Rectangle<float>(r.getX(), cy - 21.0f, 170.0f, 42.0f).toNearestInt());
  mOutput->setBounds(juce::Rectangle<float>(r.getRight() - 170.0f, cy - 21.0f, 170.0f, 42.0f).toNearestInt());
  mRigs.setBounds(juce::Rectangle<float>(r.getRight() - 96.0f, 10.0f, 96.0f, 24.0f).toNearestInt());
  // "Testar o som" em cima da entrada (a gravação entra no lugar da guitarra).
  const int testW = std::min(mTest->idealWidth(), static_cast<int>(r.getWidth()) - 260);
  mTest->setBounds(static_cast<int>(r.getX()), 10, testW, 24);
  rebuild();
}

void NexusEditor::ChainView::rebuild()
{
  const auto layout = mEditor.mProcessor.layout();
  mRigs.setSelected(layout.dual ? 1 : 0);
  mCards.clear();
  mDrops.clear();
  mWires.clear();

  const float x0 = static_cast<float>(mInput->getRight()) + 24.0f;
  const float x1 = static_cast<float>(mOutput->getX()) - 24.0f;
  const float cy = getHeight() * 0.5f + 4.0f;
  const float rowGap = layout.dual ? 40.0f : 0.0f;

  // Tamanhos-base; tudo encolhe junto se a cadeia não couber.
  float pw = 52.0f, ph = 68.0f, aw = 124.0f, ah = 56.0f, cw = 56.0f, gap = 18.0f;
  auto widthOf = [&] {
    float w = 0.0f;
    w += static_cast<float>(layout.pre.size()) * (pw + gap) + 34.0f + gap;           // pedais antes + "+"
    if (layout.dual)
      w += pw + gap;                                                                   // pedal A/B
    const size_t loops = std::max(layout.loop[0].size(), layout.dual ? layout.loop[1].size() : size_t(0));
    w += aw + gap + static_cast<float>(loops) * (pw + gap) + cw + gap;                // cabeçote, entre, caixa
    w += static_cast<float>(layout.post.size()) * (pw + gap);                         // pedais depois
    w += aw * 0.62f + gap;                                                            // equalizador do fim
    return w;
  };
  const float scale = std::min(1.0f, (x1 - x0) / std::max(1.0f, widthOf()));
  if (layout.dual)
    for (auto* v : {&pw, &ph, &aw, &ah, &cw})
      *v *= 0.78f;
  for (auto* v : {&pw, &ph, &aw, &ah, &cw, &gap})
    *v *= scale;

  float x = x0 + std::max(0.0f, ((x1 - x0) - widthOf()) * 0.5f);
  auto addCard = [&](CardType t, int index, float w, float h, float y) {
    mCards.push_back({t, index, {x, y - h * 0.5f, w, h}});
    x += w + gap;
  };
  auto addDrop = [&](int listId, int pos, float atX, float y) { mDrops.push_back({listId, pos, {atX, y}}); };

  // Pedais antes do cabeçote.
  const float startX = x;
  for (size_t i = 0; i < layout.pre.size(); ++i)
  {
    addDrop(0, static_cast<int>(i), x - gap * 0.5f, cy);
    addCard(CardType::pedal, layout.pre[i], pw, ph, cy);
  }
  addDrop(0, static_cast<int>(layout.pre.size()), x - gap * 0.5f, cy);
  if (layout.freePedal() >= 0)
    addCard(CardType::add, -1, 32.0f * scale, 32.0f * scale, cy);
  if (layout.dual)
    addCard(CardType::ab, 0, pw, ph, cy);
  const float splitX = x - gap * 0.5f;

  // Cabeçotes (um ou dois), pedais entre cabeçote e caixa, caixas.
  float rowsEnd = x;
  const int rigs = layout.dual ? 2 : 1;
  for (int rig = 0; rig < rigs; ++rig)
  {
    const float y = cy + (rig == 0 ? -rowGap : rowGap);
    float rx = x;
    std::swap(rx, x);
    addCard(CardType::amp, rig, aw, ah, y);
    const auto& loop = layout.loop[static_cast<size_t>(rig)];
    for (size_t i = 0; i < loop.size(); ++i)
    {
      addDrop(1 + rig, static_cast<int>(i), x - gap * 0.5f, y);
      addCard(CardType::pedal, loop[i], pw, ph, y);
    }
    addDrop(1 + rig, static_cast<int>(loop.size()), x - gap * 0.5f, y);
    addCard(CardType::cab, rig, cw, cw, y);
    rowsEnd = std::max(rowsEnd, x);
    x = rx;
  }
  x = rowsEnd;
  // Pedais depois da caixa.
  const float mergeX = x - gap * 0.5f;
  for (size_t i = 0; i < layout.post.size(); ++i)
  {
    addDrop(3, static_cast<int>(i), x - gap * 0.5f, cy);
    addCard(CardType::pedal, layout.post[i], pw, ph, cy);
  }
  addDrop(3, static_cast<int>(layout.post.size()), x - gap * 0.5f, cy);
  // Equalizador do fim da cadeia: fixo, todo o som passa por ele.
  addCard(CardType::eq, 0, cw * 0.86f, cw * 0.86f, cy);

  // Fios: entrada até a divisão, os dois lados, a junção até a saída.
  juce::Path main;
  main.startNewSubPath(static_cast<float>(mInput->getRight()) + 14.0f, cy);
  main.lineTo(layout.dual ? splitX : mergeX, cy);
  mWires.push_back(main);
  if (layout.dual)
    for (int rig = 0; rig < 2; ++rig)
    {
      const float y = cy + (rig == 0 ? -rowGap : rowGap);
      juce::Path p;
      p.startNewSubPath(splitX, cy);
      p.cubicTo(splitX + gap, cy, splitX, y, splitX + gap, y);
      p.lineTo(mergeX - gap, y);
      p.cubicTo(mergeX, y, mergeX - gap, cy, mergeX, cy);
      mWires.push_back(p);
    }
  juce::Path tail;
  tail.startNewSubPath(layout.dual ? mergeX : startX, cy);
  tail.lineTo(static_cast<float>(mOutput->getX()) - 14.0f, cy);
  mWires.push_back(tail);
  repaint();
}

void NexusEditor::ChainView::paint(juce::Graphics& g)
{
  auto r = getLocalBounds().toFloat();
  g.setColour(juce::Colour(0xff0c0c0e));
  g.fillRect(r);
  g.setColour(colour::line);
  g.fillRect(r.getX(), r.getY(), r.getWidth(), 1.0f);
  drawCaps(g, TRANS(juce::String::fromUTF8("Cabeçotes")), {static_cast<float>(mRigs.getX()) - 110.0f, static_cast<float>(mRigs.getY()), 104.0f, 24.0f},
           10.0f, juce::Justification::centredRight, colour::dim);

  for (const auto& w : mWires)
  {
    g.setColour(colour::line.brighter(0.25f));
    g.strokePath(w, juce::PathStrokeType(1.2f));
  }

  auto& proc = mEditor.mProcessor;
  const auto layout = proc.layout();
  const int activeRig = proc.activeRig();
  for (size_t i = 0; i < mCards.size(); ++i)
  {
    const auto& c = mCards[i];
    const bool hover = static_cast<int>(i) == mHover;
    Selection sel{c.type == CardType::pedal ? Kind::pedal : (c.type == CardType::cab ? Kind::cab : Kind::amp), c.index};
    const bool selected = (c.type == CardType::pedal || c.type == CardType::amp || c.type == CardType::cab) && mEditor.mSel == sel;
    // Fundo escuro atrás da miniatura, para o fio "entrar" nela.
    g.setColour(juce::Colour(0xff0c0c0e));
    g.fillRect(c.r.expanded(4.0f, 0.0f));
    if (selected || hover)
    {
      g.setColour(colour::text.withAlpha(selected ? 0.07f : 0.04f));
      g.fillRoundedRectangle(c.r.expanded(6.0f, 5.0f), 7.0f);
    }
    if (mDragging && c.type == CardType::pedal && c.index == mCards[static_cast<size_t>(mPressed)].index)
      continue; // o pedal arrastado é desenhado no ponteiro do mouse

    switch (c.type)
    {
    case CardType::pedal:
    {
      const auto* art = mEditor.pedalArt(c.index);
      const bool loaded = proc.slotLoaded(NexusProcessor::pedalSlot(c.index));
      const bool on = proc.params.getRawParameterValue(NexusProcessor::pedalParam(c.index, "ligado"))->load() > 0.5f;
      if (art != nullptr)
        drawGear(g, art->front, fitImage(art->front, c.r)); // só o LED indica ligado
      if (art != nullptr && on && loaded && art->ledRadius > 0.0f)
      {
        const auto img = fitImage(art->front, c.r);
        const float s = img.getWidth() / static_cast<float>(art->front.getWidth());
        drawLed(g, {img.getX() + art->led.x * s, img.getY() + art->led.y * s}, std::max(1.6f, art->ledRadius * s));
      }
      break;
    }
    case CardType::amp:
      drawGear(g, mEditor.ampArt(c.index).front, fitImage(mEditor.ampArt(c.index).front, c.r), layout.dual && activeRig != c.index ? 0.45f : 1.0f);
      break;
    case CardType::cab:
    {
      const bool cabOn = proc.packIncludesCab(c.index)
                         || (proc.params.getRawParameterValue(NexusProcessor::ampParam(c.index, "caixa"))->load() > 0.5f && proc.irLoaded(c.index));
      drawGear(g, mEditor.cabArt(c.index).front, fitImage(mEditor.cabArt(c.index).front, c.r),
               cabOn && !(layout.dual && activeRig != c.index) ? 1.0f : 0.4f);
      break;
    }
    case CardType::ab:
      drawAbPedal(g, c.r, activeRig, 1.0f);
      break;
    case CardType::eq:
    {
      const bool on = proc.params.getRawParameterValue("eq_ligado")->load() > 0.5f;
      nexus::ui::EqStage::paintThumb(g, c.r, proc.params, on ? 1.0f : 0.4f);
      break;
    }
    case CardType::add:
    {
      juce::Path dash;
      juce::Path box;
      box.addRoundedRectangle(c.r, 5.0f);
      const float pattern[] = {3.0f, 3.0f};
      juce::PathStrokeType(1.2f).createDashedStroke(dash, box, pattern, 2);
      g.setColour(hover ? colour::accent : colour::dim);
      g.fillPath(dash);
      g.setFont(Fonts::get().ui(c.r.getHeight() * 0.7f));
      g.drawText("+", c.r, juce::Justification::centred);
      break;
    }
    }
    if (layout.dual && (c.type == CardType::amp || c.type == CardType::cab))
    {
      g.setColour(colour::dim);
      g.setFont(Fonts::get().display(10.0f));
      g.drawText(c.index == 0 ? "A" : "B", c.r.withX(c.r.getX() - 14.0f).withWidth(12.0f), juce::Justification::centredRight);
    }
    if (selected)
    {
      g.setColour(colour::accent);
      g.fillRoundedRectangle(c.r.getCentreX() - 12.0f, c.r.getBottom() + 7.0f, 24.0f, 2.0f, 1.0f);
    }
    // Processador que o bloco usa agora (parte de um núcleo).
    if (c.type == CardType::pedal || c.type == CardType::amp)
    {
      const int slot = c.type == CardType::pedal ? NexusProcessor::pedalSlot(c.index) : proc.audibleSlot(c.index);
      if (const float load = proc.slotLoad(slot); load >= 0.001f)
      {
        g.setColour(load > 0.5f ? colour::danger : colour::faint);
        g.setFont(Fonts::get().ui(9.5f));
        g.drawText(juce::String(load * 100.0f, 1).replaceCharacter('.', ',') + "%",
                   c.r.withY(c.r.getBottom() + 10.0f).withHeight(12.0f).expanded(10.0f, 0.0f), juce::Justification::centred);
      }
    }
  }

  // Nome do que está sob o mouse, embaixo da cadeia.
  if (mHover >= 0 && mHover < static_cast<int>(mCards.size()) && !mDragging)
  {
    const auto& c = mCards[static_cast<size_t>(mHover)];
    juce::String name;
    switch (c.type)
    {
    case CardType::pedal:
      if (const auto* art = mEditor.pedalArt(c.index))
        name = art->name;
      name << "  (" << TRANS("pedal") << " " << (c.index + 1) << ")";
      break;
    case CardType::amp: name = TRANS(juce::String::fromUTF8("Cabeçote")); break;
    case CardType::cab: name = TRANS("Caixa"); break;
    case CardType::ab: name = TRANS("Pedal A/B: clique para trocar o lado"); break;
    case CardType::add: name = TRANS("Adicionar pedal"); break;
    case CardType::eq: name = TRANS("Equalizador"); break;
    }
    drawCaps(g, name, {0.0f, r.getBottom() - 22.0f, r.getWidth(), 16.0f}, 10.0f, juce::Justification::centred, colour::dim);
  }
  else
    drawCaps(g, TRANS("Arraste os pedais para mudar a ordem"), {0.0f, r.getBottom() - 22.0f, r.getWidth(), 16.0f}, 10.0f,
             juce::Justification::centred, colour::faint);

  // Arrastando: o pedal segue o mouse e o lugar de encaixe acende.
  if (mDragging && mPressed >= 0)
  {
    const auto& c = mCards[static_cast<size_t>(mPressed)];
    if (const auto* drop = nearestDrop(mDragAt))
    {
      g.setColour(colour::accent);
      g.fillRoundedRectangle(drop->at.x - 1.5f, drop->at.y - c.r.getHeight() * 0.5f, 3.0f, c.r.getHeight(), 1.5f);
    }
    if (const auto* art = mEditor.pedalArt(c.index))
      drawGear(g, art->front, fitImage(art->front, c.r.withCentre(mDragAt)), 0.9f);
  }

  // Pedal vindo da coluna: acende o pedal que será trocado ou o lugar de encaixe.
  if (mExternal)
  {
    const int card = cardAt(mDragAt);
    if (card >= 0 && mCards[static_cast<size_t>(card)].type == CardType::pedal)
    {
      g.setColour(colour::accent);
      g.drawRoundedRectangle(mCards[static_cast<size_t>(card)].r.expanded(5.0f), 6.0f, 1.5f);
    }
    else if (const auto* drop = nearestDrop(mDragAt))
    {
      g.setColour(colour::accent);
      g.fillRoundedRectangle(drop->at.x - 1.5f, drop->at.y - 26.0f, 3.0f, 52.0f, 1.5f);
    }
  }
}

int NexusEditor::ChainView::cardAt(juce::Point<float> p) const
{
  for (size_t i = 0; i < mCards.size(); ++i)
    if (mCards[i].r.expanded(5.0f).contains(p))
      return static_cast<int>(i);
  return -1;
}

const NexusEditor::ChainView::Drop* NexusEditor::ChainView::nearestDrop(juce::Point<float> p) const
{
  const Drop* best = nullptr;
  float bestDistance = 60.0f;
  for (const auto& d : mDrops)
    if (const float dist = d.at.getDistanceFrom(p); dist < bestDistance)
    {
      bestDistance = dist;
      best = &d;
    }
  return best;
}

void NexusEditor::ChainView::mouseMove(const juce::MouseEvent& e)
{
  const int h = cardAt(e.position);
  setMouseCursor(h >= 0 ? juce::MouseCursor::PointingHandCursor : juce::MouseCursor::NormalCursor);
  if (h != mHover)
  {
    mHover = h;
    repaint();
  }
}

void NexusEditor::ChainView::mouseDown(const juce::MouseEvent& e)
{
  mPressed = cardAt(e.position);
  mDragging = false;
  mDragAt = e.position;
}

void NexusEditor::ChainView::mouseDrag(const juce::MouseEvent& e)
{
  if (mPressed < 0 || mCards[static_cast<size_t>(mPressed)].type != CardType::pedal || e.mods.isPopupMenu())
    return;
  if (!mDragging && e.getDistanceFromDragStart() > 5)
    mDragging = true;
  if (mDragging)
  {
    mDragAt = e.position;
    setMouseCursor(juce::MouseCursor::DraggingHandCursor);
    repaint();
  }
}

void NexusEditor::ChainView::mouseUp(const juce::MouseEvent& e)
{
  const int card = mPressed;
  mPressed = -1;
  if (card < 0)
    return;
  const auto c = mCards[static_cast<size_t>(card)];
  if (mDragging)
  {
    mDragging = false;
    if (const auto* drop = nearestDrop(e.position))
      dropPedal(c.index, *drop);
    repaint();
    return;
  }
  if (e.mods.isPopupMenu())
  {
    showCardMenu(card);
    return;
  }
  switch (c.type)
  {
  case CardType::pedal: mEditor.select({Kind::pedal, c.index}); break;
  case CardType::amp: mEditor.select({Kind::amp, c.index}); break;
  case CardType::cab: mEditor.select({Kind::cab, c.index}); break;
  case CardType::eq: mEditor.select({Kind::eq, c.index}); break;
  case CardType::add: mEditor.addPedal(); break;
  case CardType::ab:
  {
    auto* ab = mEditor.mProcessor.params.getParameter("ab");
    ab->setValueNotifyingHost(ab->getValue() > 0.5f ? 0.0f : 1.0f);
    break;
  }
  }
}

void NexusEditor::ChainView::dropPedal(int pedal, const Drop& d)
{
  auto layout = mEditor.mProcessor.layout();
  // Posição na lista de destino, descontando a saída do próprio pedal.
  int position = d.position;
  auto& target = list(layout, d.list);
  const auto old = std::find(target.begin(), target.end(), pedal);
  if (old != target.end() && static_cast<int>(old - target.begin()) < position)
    --position;
  layout.remove(pedal);
  auto& dest = list(layout, d.list);
  dest.insert(dest.begin() + juce::jlimit(0, static_cast<int>(dest.size()), position), pedal);
  mEditor.mProcessor.setLayout(layout);
  rebuild();
}

bool NexusEditor::ChainView::isInterestedInDragSource(const SourceDetails& d)
{
  return d.description.toString().startsWith("gear:0:");
}

void NexusEditor::ChainView::itemDragMove(const SourceDetails& d)
{
  mExternal = true;
  mDragAt = d.localPosition.toFloat();
  repaint();
}

void NexusEditor::ChainView::itemDragExit(const SourceDetails&)
{
  mExternal = false;
  repaint();
}

void NexusEditor::ChainView::itemDropped(const SourceDetails& d)
{
  mExternal = false;
  const int look = d.description.toString().fromLastOccurrenceOf(":", false, false).getIntValue();
  const auto at = d.localPosition.toFloat();

  // Solto em cima de um pedal: troca o visual dele; senão, pedal novo ali.
  const int card = cardAt(at);
  if (card >= 0 && mCards[static_cast<size_t>(card)].type == CardType::pedal)
  {
    const int pedal = mCards[static_cast<size_t>(card)].index;
    auto* visual = mEditor.mProcessor.params.getParameter(NexusProcessor::pedalParam(pedal, "visual"));
    visual->setValueNotifyingHost(visual->convertTo0to1(static_cast<float>(look)));
    mEditor.select({Kind::pedal, pedal});
  }
  else if (const auto* drop = nearestDrop(at))
    mEditor.insertPedal(look, drop->list, drop->position);
  repaint();
}

void NexusEditor::ChainView::showCardMenu(int card)
{
  const auto c = mCards[static_cast<size_t>(card)];
  auto& proc = mEditor.mProcessor;
  if (c.type == CardType::ab)
  {
    mEditor.showMidiMenu("ab", *this);
    return;
  }
  if (c.type == CardType::amp)
  {
    mEditor.showMidiMenu(NexusProcessor::ampParam(c.index, "canal"), *this);
    return;
  }
  if (c.type == CardType::eq)
  {
    const bool isOn = proc.params.getRawParameterValue("eq_ligado")->load() > 0.5f;
    juce::PopupMenu menu;
    menu.addItem(TRANS(isOn ? "Desligar" : "Ligar"), [this, isOn] {
      mEditor.mProcessor.params.getParameter("eq_ligado")->setValueNotifyingHost(isOn ? 0.0f : 1.0f);
      repaint();
    });
    menu.addItem(TRANS("Zerar o equalizador"), [this] { mEditor.mEqStage->clearBands(); });
    menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(this));
    return;
  }
  if (c.type != CardType::pedal)
    return;
  const auto layout = proc.layout();
  juce::PopupMenu menu;
  menu.addSectionHeader(TRANS("Pedal") + " " + juce::String(c.index + 1));
  menu.addItem(1, TRANS(juce::String::fromUTF8("Mover para antes do cabeçote")));
  menu.addItem(2, TRANS(juce::String::fromUTF8("Mover para entre cabeçote e caixa")) + (layout.dual ? " (A)" : ""));
  if (layout.dual)
    menu.addItem(3, TRANS(juce::String::fromUTF8("Mover para entre cabeçote e caixa")) + " (B)");
  menu.addItem(4, TRANS("Mover para depois da caixa"));
  menu.addSeparator();
  const bool pedalLite = proc.params.getRawParameterValue(NexusProcessor::pedalParam(c.index, "lite"))->load() > 0.5f;
  menu.addItem(7, TRANS(juce::String::fromUTF8("Motor A2 Lite neste pedal (usa menos processador)")),
               !NexusProcessor::isNativeLook(proc.pedalLook(c.index)), pedalLite);
  menu.addItem(5, TRANS("Aprender MIDI (liga e desliga)"));
  menu.addItem(6, TRANS("Remover pedal"));
  menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(this), [this, pedal = c.index](int r) {
    if (r >= 1 && r <= 4)
    {
      auto l = mEditor.mProcessor.layout();
      const int listId = r - 1;
      Drop d{listId, static_cast<int>(list(l, listId).size()), {}};
      dropPedal(pedal, d);
    }
    else if (r == 5)
      mEditor.showMidiMenu(NexusProcessor::pedalParam(pedal, "ligado"), *this);
    else if (r == 7)
    {
      auto* lite = mEditor.mProcessor.params.getParameter(NexusProcessor::pedalParam(pedal, "lite"));
      lite->setValueNotifyingHost(lite->getValue() > 0.5f ? 0.0f : 1.0f);
    }
    else if (r == 6)
    {
      auto l = mEditor.mProcessor.layout();
      l.remove(pedal);
      mEditor.mProcessor.setLayout(l);
      if (mEditor.mSel.kind == Kind::pedal && mEditor.mSel.index == pedal)
        mEditor.select({Kind::amp, 0});
      rebuild();
    }
  });
}

// ================================================================ Editor

juce::Font NexusEditor::Look::getTextButtonFont(juce::TextButton&, int height)
{
  return Fonts::get().ui(std::min(13.5f, static_cast<float>(height) * 0.42f));
}

NexusEditor::NexusEditor(NexusProcessor& p)
: AudioProcessorEditor(&p)
, mProcessor(p)
{
  language::applyStartup();
  mLook.setColour(juce::PopupMenu::backgroundColourId, juce::Colour(0xff1b1b1b));
  mLook.setColour(juce::PopupMenu::textColourId, colour::text);
  mLook.setColour(juce::PopupMenu::highlightedBackgroundColourId, juce::Colour(0xff23384a));
  mLook.setColour(juce::PopupMenu::highlightedTextColourId, colour::selected);
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
  mLook.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff1d1f23));
  mLook.setColour(juce::TextButton::textColourOffId, colour::text);
  mLook.setColour(juce::ComboBox::buttonColourId, juce::Colour(0xff1d1f23));
  mLook.setDefaultSansSerifTypeface(juce::Typeface::createSystemTypefaceFor(BinaryData::inter_ttf, BinaryData::inter_ttfSize));
  setLookAndFeel(&mLook);

  for (int i = 0; i < NexusProcessor::kAmpLooks; ++i)
    mAmpArts[static_cast<size_t>(i)] = GearArt::load(kAmpPrefixes[i]);
  for (int i = 0; i < NexusProcessor::kCabLooks; ++i)
    mCabArts[static_cast<size_t>(i)] = GearArt::load(kCabPrefixes[i]);
  for (int i = 0; i < kPedalLooks; ++i)
    mPedalArts[static_cast<size_t>(i)] = GearArt::load(kPedalPrefixes[i]);

  addAndMakeVisible(mEngine);
  mEngine.setFontSize(11.5f);
  mEngine.onSelect = [this](int i) { mProcessor.params.getParameter("lite")->setValueNotifyingHost(i == 1 ? 1.0f : 0.0f); };
  addAndMakeVisible(mInfoButton);
  mInfoButton.onClick = [this] { showAbout(); };
  addAndMakeVisible(mLanguageButton);
  mLanguageButton.onClick = [this] { showLanguageMenu(); };

  mPedalStage = std::make_unique<GearStage>(p, Kind::pedal);
  mAmpStage = std::make_unique<GearStage>(p, Kind::amp);
  mCabStage = std::make_unique<CabStage>(p, mCabArts.data());
  addChildComponent(*mPedalStage);
  addChildComponent(*mAmpStage);
  addChildComponent(*mCabStage);

  addChildComponent(mChannels);
  mChannels.setFontSize(11.5f);
  mChannels.onSelect = [this](int ch) {
    const int rig = mSel.index;
    if (!mProcessor.channelLoaded(rig, ch))
    {
      auto* param = mProcessor.params.getParameter(NexusProcessor::ampParam(rig, "canal"));
      param->setValueNotifyingHost(param->convertTo0to1(static_cast<float>(ch)));
      choosePack();
      return;
    }
    auto* param = mProcessor.params.getParameter(NexusProcessor::ampParam(rig, "canal"));
    param->setValueNotifyingHost(param->convertTo0to1(static_cast<float>(ch)));
    timerCallback();
  };

  mRail = std::make_unique<CaptureRail>(p);
  addAndMakeVisible(*mRail);
  addAndMakeVisible(mChangeLink);
  mChangeLink.onClick = [this] {
    if (mSel.kind == Kind::eq)
      mEqStage->clearBands();
    else if (mSel.kind == Kind::cab)
      chooseIr(mSel.index);
    else
      choosePack();
  };
  mPresets = std::make_unique<PresetBar>(
    [this] {
      juce::MemoryBlock data;
      mProcessor.getStateInformation(data);
      return data;
    },
    [this](const juce::MemoryBlock& data) {
      mProcessor.setStateInformation(data.getData(), static_cast<int>(data.getSize()));
      // Tela acompanha o preset: cadeia, peça escolhida e coluna.
      mChain->rebuild();
      select({Kind::amp, 0});
    });
  addAndMakeVisible(*mPresets);
  addChildComponent(mUpdate);
  mUpdate.onLayoutChange = [this] { resized(); };
  // Depois da janela aparecer: pergunta se manda o relatório de travamento.
  juce::Timer::callAfterDelay(1500, [safe = juce::Component::SafePointer<NexusEditor>(this)] {
    if (safe != nullptr && safe->isShowing())
      safe->offerCrashReport();
  });
  mBrowser = std::make_unique<GearBrowser>();
  mBrowser->onPick = [this](const GearItem& item) { pickGear(item); };
  addAndMakeVisible(*mBrowser);
  refreshBrowser();

  mChain = std::make_unique<ChainView>(*this);
  addAndMakeVisible(*mChain);

  // Estúdio: no lugar dos equipamentos, com o caminho do sinal embaixo.
  mEqStage = std::make_unique<nexus::ui::EqStage>(mProcessor.params);
  addChildComponent(*mEqStage);
  mStudio = std::make_unique<nexus::ui::StudioView>(mProcessor.studio(), mProcessor.separation());
  addChildComponent(*mStudio);
  addAndMakeVisible(mMode);
  mMode.setFontSize(11.0f);
  mMode.onSelect = [this](int i) { setMode(i); };

  // MIDI: clique com o botão direito em qualquer controle.
  auto midiMenu = [this](const juce::String& id, juce::Component& c) { showMidiMenu(id, c); };
  mPedalStage->onMidiMenu = midiMenu;
  mAmpStage->onMidiMenu = midiMenu;
  mChain->input().onMidiMenu = midiMenu;
  mChain->output().onMidiMenu = midiMenu;
  mChannels.onRightClick = [this] { showMidiMenu(NexusProcessor::ampParam(mSel.index, "canal"), mChannels); };

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

  mCapturePanel = std::make_unique<CapturePanel>();
  mCapturePanel->onClose = [this] { mCapturePanel->setVisible(false); };
  mCapturePanel->onManual = [this] {
    if (mPackTarget.kind == Kind::cab)
      chooseIrFile(mPackTarget.index);
    else
      chooseFolder();
  };
  mCapturePanel->onLoaded = [this](const juce::File& folder) {
    mCapturePanel->setVisible(false);
    if (mPackTarget.kind != Kind::cab)
      return landPack(folder);
    // Pacote de IRs: entra o indicado pelo Nexus, ou o primeiro.
    const auto info = juce::JSON::parse(folder.getChildFile("pack.json"));
    const auto preferred = info["ir_padrao"].toString();
    auto files = folder.findChildFiles(juce::File::findFiles, false, "*.wav;*.aif;*.aiff");
    files.sort();
    juce::File pick = files.isEmpty() ? juce::File() : files.getFirst();
    for (const auto& f : files)
      if (preferred.isNotEmpty() && f.getFileNameWithoutExtension() == preferred)
        pick = f;
    if (pick.existsAsFile())
      applyIr(mPackTarget.index, pick);
  };
  addChildComponent(*mCapturePanel);
  mMapper = std::make_unique<PackMapper>();
  mMapper->onClose = [this] { mMapper->setVisible(false); };
  mMapper->onSaved = [this](const juce::File& folder) {
    mMapper->setVisible(false);
    loadPackInto(mPackTarget, mPackChannel, folder);
  };
  addChildComponent(*mMapper);

  setSize(kWidth, kHeight);
  select({Kind::amp, 0});
  retranslate();
  startTimerHz(30);
}

NexusEditor::~NexusEditor()
{
  stopTimer();
  setLookAndFeel(nullptr);
}

int NexusEditor::pedalLook(int pedal) const
{
  return juce::jlimit(0, kPedalLooks - 1,
                      juce::roundToInt(mProcessor.params.getRawParameterValue(NexusProcessor::pedalParam(pedal, "visual"))->load()));
}

const GearArt* NexusEditor::pedalArt(int pedal) const
{
  const auto& art = mPedalArts[static_cast<size_t>(pedalLook(pedal))];
  return art.valid() ? &art : nullptr;
}


void NexusEditor::addPedal()
{
  const auto layout = mProcessor.layout();
  insertPedal(0, 0, static_cast<int>(layout.pre.size()));
}

void NexusEditor::insertPedal(int look, int list, int position)
{

  auto layout = mProcessor.layout();
  const int pedal = layout.freePedal();
  if (pedal < 0)
    return;
  // Pedal novo: visual escolhido, ligado, com os botões no padrão.
  for (const char* id : {"drive", "tom", "nivel", "ligado"})
    if (auto* param = mProcessor.params.getParameter(NexusProcessor::pedalParam(pedal, id)))
      param->setValueNotifyingHost(param->getDefaultValue());
  auto* visual = mProcessor.params.getParameter(NexusProcessor::pedalParam(pedal, "visual"));
  visual->setValueNotifyingHost(visual->convertTo0to1(static_cast<float>(look)));
  auto& dest = ChainView::list(layout, list);
  dest.insert(dest.begin() + juce::jlimit(0, static_cast<int>(dest.size()), position), pedal);
  mProcessor.setLayout(layout);
  mChain->rebuild();
  select({Kind::pedal, pedal});
}

namespace
{
// Pedais: o clássico que inspirou cada visual e de onde vem o som.
struct PedalInfo
{
  const char* name;
  const char* inspiration;
  const char* category;
  const char* source;
};
const PedalInfo kPedalInfo[NexusEditor::kPedalLooks] = {
  {"Lemon Drive", "TS9", "Drive e distorção", "Captura da TONE3000"},
  {"Orange Drive", "DS-1", "Drive e distorção", "Captura da TONE3000"},
  {"Banana Overdrive", "SD-1", "Drive e distorção", "Captura da TONE3000"},
  {"Gold Centaur", "Klon Centaur", "Drive e distorção", "Captura da TONE3000"},
  {"Blueberry Chorus", "CE-2", "Modulação", "Efeito do Nexus"},
  {"Coconut Delay", "DD-3", "Delay", "Efeito do Nexus"},
  {"Grape Reverb", "RV-6", "Reverb", "Efeito do Nexus"},
  {"Pear Gate", "MXR Smart Gate", "Utilidades", "Efeito do Nexus"},
};

// Cabeçotes: o original que inspirou cada um e a família na coluna.
struct AmpInfo
{
  const char* inspiration;
  const char* category;
};
const AmpInfo kAmpInfo[NexusProcessor::kAmpLooks] = {
  {"JCM800 2203", "Britânicos"},
  {"Mark V", "Americanos de alto ganho"},
};

// Planejados (docs/CATALOGO.md): aparecem em "Em breve".
struct Planned
{
  GearItem::Kind kind;
  const char* name;
  const char* inspiration;
};
const Planned kPlanned[] = {
  {GearItem::Kind::pedal, "Blueberry Blues", "BD-2"},
  {GearItem::Kind::pedal, "Plum Rat", "RAT"},
  {GearItem::Kind::pedal, "Fig Fuzz", "Big Muff"},
  {GearItem::Kind::pedal, "Cherry Fuzz", "Fuzz Face"},
  {GearItem::Kind::pedal, "Blackberry Metal", "MT-2"},
  {GearItem::Kind::pedal, "Kiwi Drive", "OCD"},
  {GearItem::Kind::pedal, "Peach Phaser", "Phase 90"},
  {GearItem::Kind::pedal, "Lime Flanger", "MXR Flanger"},
  {GearItem::Kind::pedal, "Melon Tremolo", "TR-2"},
  {GearItem::Kind::pedal, "Apple Comp", "Dyna Comp"},
  {GearItem::Kind::pedal, "Mango Wah", "Cry Baby"},
  {GearItem::Kind::amp, "Brit Plexi", "Super Lead 1959"},
  {GearItem::Kind::amp, "Silver Twin", "Twin Reverb"},
  {GearItem::Kind::amp, "Tweed 59", "Bassman 5F6-A"},
  {GearItem::Kind::amp, "Top Boost 30", "AC30"},
  {GearItem::Kind::amp, "Block 5150", "5150"},
  {GearItem::Kind::amp, "Recto Dual", "Dual Rectifier"},
  {GearItem::Kind::amp, "SL 100", "SLO-100"},
  {GearItem::Kind::amp, "BE 100", "BE-100"},
  {GearItem::Kind::amp, "Jazz 120", "JC-120"},
  {GearItem::Kind::cab, "Recto 4x12", "Rectifier 4x12"},
  {GearItem::Kind::cab, "Blue 2x12", "AC30 2x12"},
  {GearItem::Kind::cab, "Twin 2x12", "Twin Reverb 2x12"},
};
} // namespace

void NexusEditor::refreshBrowser()
{
  std::vector<GearItem> items;
  for (int i = 0; i < kPedalLooks; ++i)
  {
    GearItem it;
    it.kind = GearItem::Kind::pedal;
    it.look = i;
    it.art = &mPedalArts[static_cast<size_t>(i)];
    it.name = kPedalInfo[i].name;
    it.inspiration = kPedalInfo[i].inspiration;
    it.category = TRANS(juce::String::fromUTF8(kPedalInfo[i].category));
    it.source = kPedalInfo[i].source;
    items.push_back(it);
  }
  for (int i = 0; i < NexusProcessor::kAmpLooks; ++i)
  {
    GearItem amp;
    amp.kind = GearItem::Kind::amp;
    amp.look = i;
    amp.art = &mAmpArts[static_cast<size_t>(i)];
    amp.name = mAmpArts[static_cast<size_t>(i)].name;
    amp.inspiration = kAmpInfo[i].inspiration;
    amp.category = TRANS(juce::String::fromUTF8(kAmpInfo[i].category));
    amp.source = "Captura da TONE3000";
    items.push_back(amp);
  }
  for (int i = 0; i < NexusProcessor::kCabLooks; ++i)
  {
    GearItem cab;
    cab.kind = GearItem::Kind::cab;
    cab.look = i;
    cab.art = &mCabArts[static_cast<size_t>(i)];
    cab.name = cab.art->name;
    cab.inspiration = kCabInfo[i].inspiration;
    cab.category = "4x12";
    cab.source = "IR ou captura full rig";
    items.push_back(cab);
  }
  for (const auto& p : kPlanned)
  {
    GearItem it;
    it.kind = p.kind;
    it.name = p.name;
    it.inspiration = p.inspiration;
    items.push_back(it);
  }
  mBrowser->setItems(std::move(items));
}

void NexusEditor::pickGear(const GearItem& item)
{
  switch (item.kind)
  {
  case GearItem::Kind::pedal:
    if (mSel.kind == Kind::pedal)
    {
      // Troca o pedal escolhido por este.
      auto* visual = mProcessor.params.getParameter(NexusProcessor::pedalParam(mSel.index, "visual"));
      visual->setValueNotifyingHost(visual->convertTo0to1(static_cast<float>(item.look)));
      select(mSel);
    }
    else
      insertPedal(item.look, 0, static_cast<int>(mProcessor.layout().pre.size()));
    break;
  case GearItem::Kind::amp:
  {
    // Troca o cabeçote do lado escolhido por este e abre os pacotes de cada
    // canal que estiverem na pasta dele.
    const int rig = mSel.kind == Kind::pedal ? 0 : mSel.index;
    if (auto* look = mProcessor.params.getParameter(NexusProcessor::ampParam(rig, "cabecote")))
      look->setValueNotifyingHost(look->convertTo0to1(static_cast<float>(item.look)));
    // A caixa acompanha o cabeçote (dá para trocar depois na lista de caixas).
    if (auto* cab = mProcessor.params.getParameter(NexusProcessor::ampParam(rig, "gabinete")))
      cab->setValueNotifyingHost(cab->convertTo0to1(static_cast<float>(kCabOfAmp[juce::jlimit(0, NexusProcessor::kAmpLooks - 1, item.look)])));
    loadHeadChannels(rig);
    select({Kind::amp, rig});
    break;
  }
  case GearItem::Kind::cab:
  {
    const int rig = mSel.kind == Kind::pedal ? 0 : mSel.index;
    if (auto* cab = mProcessor.params.getParameter(NexusProcessor::ampParam(rig, "gabinete")))
      cab->setValueNotifyingHost(cab->convertTo0to1(static_cast<float>(item.look)));
    select({Kind::cab, rig});
    break;
  }
  }
  mChain->repaint();
}

int NexusEditor::railSlot() const
{
  if (mSel.kind == Kind::pedal)
    return NexusProcessor::pedalSlot(mSel.index);
  return NexusProcessor::ampSlot(mSel.index, mProcessor.selectedChannel(mSel.index));
}

void NexusEditor::select(Selection s)
{
  mSel = s;
  mPedalStage->setVisible(s.kind == Kind::pedal);
  mAmpStage->setVisible(s.kind == Kind::amp);
  mCabStage->setVisible(s.kind == Kind::cab);
  mEqStage->setVisible(s.kind == Kind::eq);

  if (s.kind == Kind::pedal)
  {
    mShownPedalLook = pedalLook(s.index);
    mPedalStage->setTarget(s.index, pedalArt(s.index));
  }
  if (s.kind == Kind::amp)
  {
    mShownChannel = mProcessor.selectedChannel(s.index);
    mShownAmpLook = mProcessor.ampLook(s.index);
    mAmpStage->setTarget(s.index, &ampArt(s.index), true);
  }
  if (s.kind == Kind::cab)
    mCabStage->setRig(s.index);
  mRail->setVisible(s.kind == Kind::amp || (s.kind == Kind::pedal && !NexusProcessor::isNativeLook(pedalLook(s.index))));
  mRail->setSlot(railSlot());
  // Equalizadores ficam na aba dos pedais; o cabeçote marca o modelo escolhido.
  const auto tab = s.kind == Kind::pedal ? GearItem::Kind::pedal
                   : s.kind == Kind::amp                      ? GearItem::Kind::amp
                                                              : GearItem::Kind::cab;
  mBrowser->setTab(tab);
  mBrowser->setCurrent(tab, s.kind == Kind::pedal ? pedalLook(s.index)
                            : s.kind == Kind::amp ? mProcessor.ampLook(s.index)
                                                  : mProcessor.cabLook(s.index));
  timerCallback();
  mChain->repaint();
  repaint();
}

void NexusEditor::choosePack()
{
  if (mSel.kind == Kind::pedal && NexusProcessor::isNativeLook(pedalLook(mSel.index)))
    return; // efeito do Nexus: não tem pacote
  mPackTarget = mSel;
  mPackChannel = mSel.kind == Kind::amp ? mProcessor.selectedChannel(mSel.index) : 0;
  const bool pedal = mSel.kind == Kind::pedal;
  const juce::String prefix = pedal ? juce::String(kPedalPrefixes[pedalLook(mSel.index)])
                                   : juce::String(kAmpPrefixes[juce::jlimit(0, NexusProcessor::kAmpLooks - 1, mProcessor.ampLook(mSel.index))]);
  const juce::String name = pedal ? (pedalArt(mSel.index) ? pedalArt(mSel.index)->name : TRANS("Pedal")) : ampArt(mSel.index).name;
  mPackGearName = name;
  mCapturePanel->open(prefix, name, pedal ? nexus::ui::CapturePanel::Gear::pedal : nexus::ui::CapturePanel::Gear::amp);
  mCapturePanel->setVisible(true);
  mCapturePanel->toFront(true);
}

namespace
{
// Canal de um pacote pelo nome da pasta: "... Canal 2 Crunch" vai no canal 2
// (índice 1). Sem "Canal N" no nome, -1.
int packChannel(const juce::File& folder)
{
  const auto name = folder.getFileName();
  const int at = name.indexOfIgnoreCase("Canal ");
  if (at < 0)
    return -1;
  const int n = name.substring(at + 6, at + 7).getIntValue();
  return n >= 1 && n <= NexusProcessor::kChannels ? n - 1 : -1;
}
} // namespace

void NexusEditor::loadPackInto(Selection target, int channel, const juce::File& folder)
{
  if (target.kind == Kind::pedal)
  {
    mProcessor.loadPedalPack(target.index, folder);
    return;
  }
  // Pacote de um canal certo (como os do Mark Five) vai para esse canal,
  // qualquer que seja o canal escolhido na tela.
  const int fixed = packChannel(folder);
  if (fixed >= 0 && fixed < ampArt(target.index).channels.size())
    channel = fixed;
  mProcessor.loadAmpPack(target.index, channel, folder);
  auto* param = mProcessor.params.getParameter(NexusProcessor::ampParam(target.index, "canal"));
  param->setValueNotifyingHost(param->convertTo0to1(static_cast<float>(channel)));
}

void NexusEditor::loadHeadChannels(int rig)
{
  // Cabeçote novo: cada canal abre o pacote dele na pasta do cabeçote
  // (Documentos/Nexus Modeler/<cabeçote>). Fica o que já estava aberto se for
  // desse cabeçote; canal sem pacote do cabeçote fica vazio (ao clicar, pede).
  const auto& art = ampArt(rig);
  const auto root = nexus::t3k::Connection::packsFolder(art.name);
  auto folders = root.findChildFiles(juce::File::findDirectories, false);
  folders.sort();
  const int count = std::min(art.channels.size(), NexusProcessor::kChannels);
  int first = -1;
  for (int c = 0; c < count; ++c)
  {
    const auto current = mProcessor.slotFolder(NexusProcessor::ampSlot(rig, c));
    const bool ours = current.isDirectory() && current.getParentDirectory() == root;
    juce::File pick = ours ? current : juce::File();
    if (pick == juce::File())
      for (const auto& f : folders)
        if (packChannel(f) == c || (count == 1 && packChannel(f) < 0))
        {
          pick = f;
          break;
        }
    if (pick == juce::File())
      mProcessor.clearAmpPack(rig, c);
    else
    {
      if (!ours)
        mProcessor.loadAmpPack(rig, c, pick);
      first = first < 0 ? c : first;
    }
  }
  // Pacote sem "Canal N" (de um canal só) entra no primeiro canal vazio.
  if (first < 0)
    for (const auto& f : folders)
      if (packChannel(f) < 0)
      {
        mProcessor.loadAmpPack(rig, 0, f);
        first = 0;
        break;
      }
  auto* param = mProcessor.params.getParameter(NexusProcessor::ampParam(rig, "canal"));
  param->setValueNotifyingHost(param->convertTo0to1(static_cast<float>(std::max(0, first))));
}

void NexusEditor::chooseFolder()
{
  const auto gear = mPackGearName;
  // Pasta, .zip ou capturas .nam soltas; tudo vai para Documentos/Nexus Modeler/<equipamento>.
  mChooser = std::make_unique<juce::FileChooser>(TRANS(juce::String::fromUTF8("Escolha a pasta, o .zip ou as capturas .nam do pacote")),
                                                 nexus::t3k::Connection::packsFolder(gear), "*.zip;*.nam");
  mChooser->launchAsync(juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles
                          | juce::FileBrowserComponent::canSelectDirectories | juce::FileBrowserComponent::canSelectMultipleItems,
                        [this, gear](const juce::FileChooser& fc) {
                          const auto chosen = fc.getResults();
                          if (chosen.isEmpty())
                            return;
                          juce::String error;
                          const auto folder = nexus::ui::packs::importInto(chosen, gear, error);
                          if (!folder.isDirectory())
                          {
                            if (error.isNotEmpty())
                            {
                              nexus::report::log("pacote", error);
                              juce::AlertWindow::showMessageBoxAsync(juce::MessageBoxIconType::WarningIcon, TRANS("Inserir manualmente"), error);
                            }
                            return;
                          }
                          mCapturePanel->setVisible(false);
                          landPack(folder);
                        });
}

void NexusEditor::landPack(const juce::File& folder)
{
  bool needsMapping = false;
  nexus::ui::packs::analyze(folder, needsMapping);
  if (!needsMapping)
  {
    loadPackInto(mPackTarget, mPackChannel, folder);
    return;
  }
  mMapper->open(folder, mPackGearName, mPackTarget.kind == Kind::pedal ? juce::String("Drive") : TRANS("Ganho"));
  mMapper->setVisible(true);
  mMapper->toFront(true);
}

void NexusEditor::applyIr(int rig, const juce::File& file)
{
  mProcessor.loadIrAsync(rig, file);
  if (auto* param = mProcessor.params.getParameter(NexusProcessor::ampParam(rig, "caixa")))
    param->setValueNotifyingHost(1.0f);
  select({Kind::cab, rig});
}

void NexusEditor::openIrCatalog(int rig)
{
  mPackTarget = {Kind::cab, rig};
  mPackChannel = 0;
  mPackGearName = cabArt(rig).name;
  // Lista de IRs recomendados da caixa; sem lista própria, a da Brit 4x12.
  mCapturePanel->open(kCabPrefixes[juce::jlimit(0, NexusProcessor::kCabLooks - 1, mProcessor.cabLook(rig))], cabArt(rig).name,
                      nexus::ui::CapturePanel::Gear::cab);
  mCapturePanel->setVisible(true);
  mCapturePanel->toFront(true);
}

void NexusEditor::chooseIr(int rig)
{
  if (mProcessor.packIncludesCab(rig))
    return;
  if (!mProcessor.irLoaded(rig))
    return openIrCatalog(rig);
  // Já tem IR: os outros microfones do mesmo pacote, mais IRs da TONE3000 ou
  // um arquivo do computador.
  const auto current = mProcessor.irFile(rig);
  auto files = current.getParentDirectory().findChildFiles(juce::File::findFiles, false, "*.wav;*.aif;*.aiff");
  files.sort();
  juce::PopupMenu menu;
  if (files.size() > 1)
  {
    menu.addSectionHeader(TRANS("Microfones deste pacote"));
    juce::PopupMenu more;
    for (int i = 0; i < files.size(); ++i)
    {
      const auto f = files[i];
      (i < 24 ? menu : more).addItem(f.getFileNameWithoutExtension(), true, f == current, [this, rig, f] { applyIr(rig, f); });
    }
    if (more.getNumItems() > 0)
      menu.addSubMenu(TRANS("Mais microfones"), more);
    menu.addSeparator();
  }
  menu.addItem(TRANS("IRs da TONE3000..."), [this, rig] { openIrCatalog(rig); });
  menu.addItem(TRANS("Arquivo do computador..."), [this, rig] { chooseIrFile(rig); });
  menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(&mChangeLink));
}

void NexusEditor::chooseIrFile(int rig)
{
  mCapturePanel->setVisible(false);
  mChooser = std::make_unique<juce::FileChooser>(TRANS("Escolha o IR da caixa"), juce::File(), "*.wav;*.aif;*.aiff");
  mChooser->launchAsync(juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                        [this, rig](const juce::FileChooser& fc) {
                          if (fc.getResult().existsAsFile())
                            applyIr(rig, fc.getResult());
                        });
}

void NexusEditor::showAbout()
{
  juce::PopupMenu menu;
  menu.addSectionHeader("Nexus Modeler " + nexus::update::currentVersion());
  menu.addItem(TRANS("Sobre o Nexus Modeler"), [] {
    juce::AlertWindow::showMessageBoxAsync(
      juce::MessageBoxIconType::NoIcon, "Nexus Modeler " + nexus::update::currentVersion(),
      TRANS(juce::String::fromUTF8("Plugin de guitarra gratuito e de código aberto (GPL v3).")) + "\n"
        + TRANS(juce::String::fromUTF8("Toca capturas NAM A2 e troca de captura conforme o botão gira.")) + "\n\n"
        + "github.com/disouz4-dev/Nexus-Modeler");
  });
  menu.addSeparator();
  mUpdate.addMenuItems(menu);
  menu.addSeparator();
  menu.addItem(TRANS(juce::String::fromUTF8("Reportar um problema...")), [this] { reportProblem(); });
  menu.addItem(TRANS(juce::String::fromUTF8("Abrir a pasta do registro de erros")),
               [] { nexus::report::logFile().existsAsFile() ? nexus::report::logFile().revealToUser() : nexus::report::folder().revealToUser(); });
  menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(&mInfoButton));
}

void NexusEditor::reportProblem()
{
  auto* w = new juce::AlertWindow(TRANS(juce::String::fromUTF8("Reportar um problema")),
                                  TRANS(juce::String::fromUTF8("Conte o que aconteceu e o que você estava fazendo. O Nexus abre o GitHub com o relatório "
                                                               "preenchido (versão, sistema e os últimos erros registrados); é só conferir e enviar. "
                                                               "Precisa de uma conta no GitHub.")),
                                  juce::MessageBoxIconType::NoIcon, this);
  w->addTextEditor("texto", {}, {});
  if (auto* editor = w->getTextEditor("texto"))
  {
    editor->setMultiLine(true, true);
    editor->setReturnKeyStartsNewLine(true);
    editor->setSize(460, 120);
  }
  w->addButton(TRANS(juce::String::fromUTF8("Abrir no GitHub")), 1, juce::KeyPress(juce::KeyPress::returnKey, juce::ModifierKeys::commandModifier, 0));
  w->addButton(TRANS("Cancelar"), 0, juce::KeyPress(juce::KeyPress::escapeKey));
  w->enterModalState(true, juce::ModalCallbackFunction::create([w](int result) {
    if (result == 1)
    {
      const auto text = w->getTextEditorContents("texto").trim();
      nexus::report::send(nexus::report::Kind::problem, text, nexus::report::recentLog(60));
    }
  }), true);
}

void NexusEditor::offerCrashReport()
{
  const auto crash = nexus::report::takeCrash();
  if (crash.isEmpty())
    return;
  juce::AlertWindow::showOkCancelBox(
    juce::MessageBoxIconType::WarningIcon, TRANS(juce::String::fromUTF8("O Nexus fechou sozinho da última vez")),
    TRANS(juce::String::fromUTF8("Quer enviar o relatório para o GitHub? Ele leva a versão, o sistema e onde o programa parou, "
                                 "sem nenhum dado pessoal. Isso ajuda a corrigir o problema.")),
    TRANS(juce::String::fromUTF8("Enviar relatório")), TRANS(juce::String::fromUTF8("Agora não")), this,
    juce::ModalCallbackFunction::create([crash](int ok) {
      if (ok != 0)
        nexus::report::send(nexus::report::Kind::crash,
                            juce::String::fromUTF8("O Nexus fechou sozinho. (Se lembrar, conte aqui o que estava fazendo.)"),
                            crash + juce::String::fromUTF8("\n\nÚltimos erros registrados:\n") + nexus::report::recentLog(30));
    }));
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

void NexusEditor::showLanguageMenu()
{
  juce::PopupMenu menu;
  const auto& list = language::all();
  for (size_t i = 0; i < list.size(); ++i)
    menu.addItem(static_cast<int>(i) + 1, juce::String::fromUTF8(list[i].native), true, language::current() == list[i].code);
  menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(&mLanguageButton), [this](int r) {
    if (r <= 0)
      return;
    language::apply(language::all()[static_cast<size_t>(r - 1)].code);
    retranslate();
  });
}

void NexusEditor::setMode(int mode)
{
  mMode.setSelected(mode);
  mStudio->setVisible(mode == 1);
  mBrowser->setVisible(mode == 0);
  if (mode == 1)
  {
    mStudio->toFront(false);
    mStudio->grabKeyboardFocus();
  }
  // Janelas por cima (capturas, mapeamento, áudio) continuam por cima.
  for (juce::Component* c : {static_cast<juce::Component*>(mCapturePanel.get()), static_cast<juce::Component*>(mMapper.get()),
                             static_cast<juce::Component*>(mAudioPanel.get())})
    if (c != nullptr && c->isVisible())
      c->toFront(false);
  repaint();
}

void NexusEditor::retranslate()
{
  mMode.setItems({TRANS("Equipamento"), TRANS(juce::String::fromUTF8("Estúdio"))});
  if (mStudio)
    mStudio->retranslate();
  mInfoButton.setTooltip(TRANS("Sobre o Nexus Modeler"));
  mAudioButton.setTooltip(TRANS(juce::String::fromUTF8("Interface de áudio")));
  mLanguageButton.setTooltip(TRANS("Idioma"));
  mUpdate.retranslate();
  if (mChain)
    mChain->testBar().retranslate();
  mBrowser->retranslate();
  refreshBrowser();
  if (mAudioPanel)
    mAudioPanel->retranslate();
  if (mCapturePanel)
    mCapturePanel->retranslate();
  if (mMapper)
    mMapper->retranslate();
  mTitle.clear();
  timerCallback();
  repaint();
}

void NexusEditor::showMidiMenu(const juce::String& paramId, juce::Component& target)
{
  auto& midi = mProcessor.midi();
  auto* param = mProcessor.params.getParameter(paramId);
  if (param == nullptr)
    return;
  const int cc = midi.ccFor(paramId);
  juce::PopupMenu menu;
  menu.addSectionHeader("MIDI: " + param->getName(48));
  menu.addItem(1, cc < 0 ? TRANS("Aprender MIDI") : TRANS("Trocar controle MIDI (agora CC %1)").replace("%1", juce::String(cc)));
  if (cc >= 0)
    menu.addItem(2, TRANS("Esquecer MIDI (CC %1)").replace("%1", juce::String(cc)));
  if (midi.learning())
    menu.addItem(4, TRANS("Cancelar aprender MIDI"));
  if (paramId == "canal")
    menu.addItem(3, TRANS(juce::String::fromUTF8("Troca de patch 1, 2, 3 já escolhe o canal")), false);
  menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(&target),
                     [this, paramId, name = param->getName(48)](int r) {
                       if (r == 1)
                       {
                         mProcessor.midi().learn(paramId);
                         mMidiNotice = TRANS("Mexa no controle da pedaleira para ligar a %1").replace("%1", name);
                         mMidiNoticeUntil = juce::Time::getMillisecondCounter() + 30000;
                       }
                       else if (r == 4)
                       {
                         mProcessor.midi().cancelLearn();
                         mMidiNotice.clear();
                       }
                       else if (r == 2)
                       {
                         mProcessor.midi().forget(paramId);
                         mMidiNotice = TRANS(juce::String::fromUTF8("%1 não responde mais ao MIDI")).replace("%1", name);
                         mMidiNoticeUntil = juce::Time::getMillisecondCounter() + 2500;
                       }
                       repaint();
                     });
}

void NexusEditor::paintOverChildren(juce::Graphics& g)
{
  if (mMidiNotice.isEmpty() || juce::Time::getMillisecondCounter() > mMidiNoticeUntil)
    return;
  const auto font = Fonts::get().ui(13.0f);
  const float w = static_cast<float>(juce::GlyphArrangement::getStringWidthInt(font, mMidiNotice)) + 48.0f;
  juce::Rectangle<float> pill(getWidth() * 0.5f - w * 0.5f, static_cast<float>(kTopBar) + 14.0f, w, 34.0f);
  g.setColour(juce::Colour(0xf0101418));
  g.fillRoundedRectangle(pill, 17.0f);
  g.setColour(colour::accent);
  g.drawRoundedRectangle(pill.reduced(0.5f), 17.0f, 1.0f);
  g.fillEllipse(pill.getX() + 16.0f, pill.getCentreY() - 3.5f, 7.0f, 7.0f);
  g.setColour(colour::text);
  g.setFont(font);
  g.drawText(mMidiNotice, pill.withTrimmedLeft(20.0f), juce::Justification::centred);
}

juce::String NexusEditor::packSummary(int slot) const
{
  if (!mProcessor.slotLoaded(slot) || mProcessor.captureCount(slot) == 0)
  {
    // Carregando ou erro: o aviso do processador; senão, o convite.
    const auto status = mProcessor.statusText(slot);
    if (status.startsWith("Carregando"))
      return TRANS("Carregando...");
    if (status.startsWith("Erro"))
      return status;
    return slot < NexusProcessor::kPedals ? TRANS("Nenhuma captura no pedal. O pisante liga e desliga.")
                                          : TRANS("Nenhum pacote de capturas carregado");
  }
  const int count = mProcessor.captureCount(slot);
  const auto number = [](float v) { return juce::String(v, 1).replaceCharacter('.', ','); };
  const bool lite = slot < NexusProcessor::kPedals
                      ? mProcessor.params.getParameter(NexusProcessor::pedalParam(slot, "lite"))->getValue() > 0.5f
                      : mProcessor.params.getParameter("lite")->getValue() > 0.5f;
  return TRANS("%1: %2 capturas, ganho de %3 a %4 (%5)")
    .replace("%1", mProcessor.packName(slot))
    .replace("%2", juce::String(count))
    .replace("%3", number(mProcessor.capturePosition(0, slot)))
    .replace("%4", number(mProcessor.capturePosition(count - 1, slot)))
    .replace("%5", lite ? "A2 Lite" : "A2 Full");
}

void NexusEditor::refreshChannels()
{
  // Os canais são os do amplificador original: um amplificador de um canal
  // não mostra seletor. Canal sem pacote aparece apagado e, ao clicar, pede
  // o pacote dele.
  const int rig = mSel.index;
  const auto& art = ampArt(rig);
  const int count = std::min(art.channels.size(), NexusProcessor::kChannels);
  juce::StringArray items;
  for (int c = 0; c < count; ++c)
    items.add(TRANS(art.channels[c]) + (mProcessor.channelLoaded(rig, c) ? juce::String() : juce::String::fromUTF8(" · ") + TRANS("vazio")));
  mChannels.setItems(items);
  mChannels.setSelected(juce::jlimit(0, std::max(0, count - 1), mProcessor.selectedChannel(rig)));
  mChannels.setVisible(mSel.kind == Kind::amp && count > 1);
  mChannels.setBounds(getLocalBounds().withTrimmedRight(kBrowserWidth).withTrimmedTop(kTopBar).removeFromTop(52).withSizeKeepingCentre(170 * std::max(1, count), 34).translated(0, 6));

  // O botão de ganho do painel segue o canal escolhido.
  // O botão de ganho segue o canal; o visual segue o cabeçote escolhido.
  if (mSel.kind == Kind::amp && (mProcessor.selectedChannel(rig) != mShownChannel || mProcessor.ampLook(rig) != mShownAmpLook))
  {
    const bool lookChanged = mProcessor.ampLook(rig) != mShownAmpLook;
    mShownChannel = mProcessor.selectedChannel(rig);
    mShownAmpLook = mProcessor.ampLook(rig);
    mAmpStage->setTarget(rig, &art, true);
    mRail->setSlot(railSlot());
    if (lookChanged)
    {
      resized();
      mChain->repaint();
      refreshBrowser();
    }
  }
}

void NexusEditor::timerCallback()
{
  const bool lite = mProcessor.params.getParameter("lite")->getValue() > 0.5f;
  mEngine.setSelected(lite ? 1 : 0);
  if (mSel.kind == Kind::amp)
    refreshChannels();
  else
    mChannels.setVisible(false);

  // MIDI aprendido: confirma; tempo esgotado: cancela.
  auto& midi = mProcessor.midi();
  if (const int cc = midi.takeLearned(); cc >= 0)
  {
    mMidiNotice = TRANS("CC %1 ligado. Pronto para tocar.").replace("%1", juce::String(cc));
    mMidiNoticeUntil = juce::Time::getMillisecondCounter() + 2500;
    repaint();
  }
  if (mMidiNotice.isNotEmpty() && juce::Time::getMillisecondCounter() > mMidiNoticeUntil)
  {
    midi.cancelLearn();
    mMidiNotice.clear();
    repaint();
  }

  // O visual do pedal escolhido pode mudar pelas setas ou pela automação.
  if (mSel.kind == Kind::pedal && pedalLook(mSel.index) != mShownPedalLook)
  {
    mShownPedalLook = pedalLook(mSel.index);
    mPedalStage->setTarget(mSel.index, pedalArt(mSel.index), true);
    mChain->repaint();
  }

  const auto layout = mProcessor.layout();
  const juce::String side = layout.dual ? (mSel.index == 0 ? "  A" : "  B") : juce::String();
  juce::String title, subtitle, link;
  if (mSel.kind == Kind::amp)
  {
    title = ampArt(mSel.index).name + side;
    subtitle = packSummary(railSlot());
    link = mProcessor.slotLoaded(railSlot()) ? TRANS("trocar pacote") : TRANS("abrir pacote");
  }
  else if (mSel.kind == Kind::pedal)
  {
    const auto* art = pedalArt(mSel.index);
    title = art ? art->name : juce::String(kPedalInfo[pedalLook(mSel.index)].name);
    if (NexusProcessor::isNativeLook(pedalLook(mSel.index)))
    {
      subtitle = TRANS(juce::String::fromUTF8("Efeito do Nexus: não usa captura. O pisante liga e desliga."));
      link = {};
    }
    else
    {
      const bool loaded = mProcessor.slotLoaded(railSlot());
      subtitle = packSummary(railSlot());
      link = loaded ? TRANS("trocar pacote") : TRANS("abrir pacote do pedal");
    }
  }
  else if (mSel.kind == Kind::eq)
  {
    title = TRANS("Equalizador");
    subtitle = TRANS(juce::String::fromUTF8("Duplo clique na curva cria uma faixa; arraste para mudar; roda do mouse muda a largura; botão direito troca o tipo."));
    link = TRANS("zerar");
  }
  else
  {
    title = cabArt(mSel.index).name + side;
    if (mProcessor.packIncludesCab(mSel.index))
    {
      subtitle = TRANS(juce::String::fromUTF8("Caixa incluída na captura (full rig): o IR fica desligado para respeitar o som original"));
      link = {};
    }
    else
    {
      subtitle = mProcessor.irLoaded(mSel.index) ? "IR: " + mProcessor.irName(mSel.index) : TRANS("Nenhum IR escolhido");
      link = mProcessor.irLoaded(mSel.index) ? TRANS("trocar IR") : TRANS("escolher IR");
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
  mChain->input().setLevel(mInPeak);
  mChain->testBar().refresh();
  mChain->output().setLevel(mOutPeak);
  mRail->repaint();
  mChain->repaint();
  if (mSel.kind == Kind::pedal)
    mPedalStage->repaint();
}

void NexusEditor::layoutTitle()
{
  auto t = mTitleArea;
  t.removeFromTop(8);
  t.removeFromTop(40); // linha do título
  auto subRow = t.removeFromTop(22);
  const auto subFont = Fonts::get().ui(12.5f);
  const int subW = juce::GlyphArrangement::getStringWidthInt(subFont, mSubtitle);
  const int linkW = mChangeLink.isVisible() ? mChangeLink.idealWidth() : 0;
  const int total = subW + (linkW > 0 ? linkW + 14 : 0);
  const int x = subRow.getCentreX() - total / 2;
  mChangeLink.setBounds(x + subW + 14, subRow.getY(), linkW, subRow.getHeight());
  mRail->setBounds(t.withSizeKeepingCentre(420, 22).translated(0, 4));
}

void NexusEditor::paint(juce::Graphics& g)
{
  g.fillAll(colour::background);
  auto r = getLocalBounds().toFloat();

  // Foco de luz de palco atrás do equipamento.
  auto stage = r.withTrimmedTop(static_cast<float>(kTopBar)).withTrimmedBottom(static_cast<float>(kPathHeight)).withTrimmedRight(static_cast<float>(kBrowserWidth));
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

  // Nome da peça e situação do pacote.
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
  auto right = top.removeFromRight(420).reduced(0, 16);
  mInfoButton.setBounds(right.removeFromRight(56).withSizeKeepingCentre(36, 36));
  mLanguageButton.setBounds(right.removeFromRight(44).withSizeKeepingCentre(36, 36));
  if (mAudioPanel)
  {
    mAudioButton.setBounds(right.removeFromRight(44).withSizeKeepingCentre(36, 36));
    mAudioPanel->setBounds(getLocalBounds());
  }
  right.removeFromRight(12);
  mEngine.setBounds(right.removeFromRight(196));
  mCapturePanel->setBounds(getLocalBounds());
  mMapper->setBounds(getLocalBounds());
  // Equipamento | Estúdio ao lado do logo; depois o aviso de versão nova (se
  // houver) e os presets no meio do espaço que sobra.
  mMode.setBounds(232, (kTopBar - 36) / 2, 236, 36);
  {
    int left = mMode.getRight() + 20;
    if (mUpdate.isVisible())
    {
      const int w = mUpdate.idealWidth();
      mUpdate.setBounds(left, (kTopBar - 40) / 2, w, 40);
      left += w + 16;
    }
    const int rightEdge = mEngine.getX() - 20;
    const int w = juce::jmin(380, rightEdge - left);
    mPresets->setBounds(left + (rightEdge - left - w) / 2, (kTopBar - 40) / 2, w, 40);
  }
  mStudio->setBounds(getLocalBounds().withTrimmedTop(kTopBar).withTrimmedBottom(kPathHeight));

  mBrowser->setBounds(r.removeFromRight(kBrowserWidth));
  mChain->setBounds(r.removeFromBottom(kPathHeight));
  mTitleArea = r.removeFromBottom(kTitleHeight);
  mPedalStage->setBounds(r);
  mAmpStage->setBounds(ampArt(mSel.kind == Kind::amp ? mSel.index : 0).channels.size() > 1 ? r.withTrimmedTop(46) : r);
  mCabStage->setBounds(r);
  mEqStage->setBounds(r);
  layoutTitle();
}
