#include "PluginEditor.h"

#include <thread>

#include <juce_audio_utils/juce_audio_utils.h>

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
                                                               "neptune_chorus", "coconut_delay", "grape_reverb", "pear_gate",
                                                               "cherry_drop", "mars_phaser", "nebula_flanger", "moon_tremolo",
                                                               "eclipse_vibe", "acai_rat", "jabuticaba_buzzsaw", "lychee_fuzz",
                                                               "avocado_metal", "pomegranate_mud", "snowpear_treble",
                                                               "juniper_sky", "lime_echo", "galaxy_mod", "blueberry_chain",
                                                               "olive_drive", "pistachio_drive", "blueberry_blues", "apricot_overdrive", "clementine_turbo", "cherry_fuzz", "fig_bender", "kiwi_drive", "quince_plus", "starfruit_rock", "tamarind_amp", "blackcurrant_bass", "mulberry_omega", "guava_precision", "peach_glory", "raspberry_drive", "honeydew_drive", "pineapple_king", "cashew_boost", "papaya_grind", "persimmon_33", "strawberry_5150", "gooseberry_g3", "nectarine_od", "cranberry_wylde", "kumquat_klone", "elderberry_war", "mangosteen_fuzz", "grapefruit_drive", "jackfruit_monkey", "pitanga_250", "lychee_bassfuzz", "cupuacu_hammer", "graviola_bassdi", "acerola_eq", "comet_wah", "saturn_rotary", "pulsar_echo", "jupiter_octave", "apple_comp"};
// Cabeçotes, na ordem do parâmetro "cabecote".
const char* const kAmpPrefixes[NexusProcessor::kAmpLooks] = {"brit800", "mark5", "block5150", "recto_dual", "sl100", "be100",
                                                             "topboost30", "match30", "tiny15", "rocker50", "recto_triple", "power100", "uber100", "sonic60", "silvertwin", "tweed59", "britplexi", "britjubilee", "jazz120", "vault300",
                                                             "citrusbass200", "granite800", "fliptop15", "vaultv4", "hart1000", "garden800", "glass500", "solart", "bassline100", "eagle751", "littletube", "workhorse350", "underground800", "acoustica360", "tinybass", "tracer7", "britbass",
                                                             "britplexi50", "brit45", "brit900", "brit410", "brit2000", "britds20", "britrack1", "silverdeluxe", "silversuper", "silverprince", "tweed57", "hotrod40", "champion5", "mark2plus", "mark3", "mark4", "markjp", "badland100", "lonestar100", "block5150iii", "topboost15", "citrus50", "citrus30", "blackcountry100", "iron120", "fire100", "feral120", "steel20", "vier100", "herb180", "bliss100", "goddess80", "dirty40", "special100", "doublestone50", "blackcat30", "hi103", "darkstar20", "tri100", "gen120", "nova100", "emperor20", "rg100", "arch100", "kraken50", "magick25", "deliver120", "ash500"};
// Caixas, na ordem do parâmetro "gabinete"; cada cabeçote abre com a sua.
const char* const kCabPrefixes[NexusProcessor::kCabLooks] = {"brit412", "mark5_412", "block_412", "sl_412", "be_412",
                                                             "citrus_412", "blue_212", "twin_212", "tweed_410", "match_212", "power_412", "uber_412", "jazz_212", "vault_810",
                                                             "granite_410", "silver_112", "mark_112", "star_412", "country_412", "vier_412", "hi_412", "rg_412", "hart_410", "glass_410", "solar_412", "fliptop_115"};
const int kCabOfAmp[NexusProcessor::kAmpLooks] = {0, 1, 2, 1, 3, 4, 6, 9, 5, 5, 1, 10, 11, 7, 7, 8, 0, 0, 12, 13, 14, 14, 25, 13, 22, 14, 23, 24, 14, 14, 14, 14, 14, 14, 14, 14, 14, 0, 0, 0, 0, 0, 0, 0, 15, 8, 15, 15, 15, 15, 16, 16, 1, 1, 1, 16, 2, 6, 5, 5, 18, 18, 10, 10, 10, 19, 19, 11, 11, 4, 7, 7, 9, 20, 17, 10, 1, 1, 15, 21, 1, 1, 15, 1, 22};
struct CabInfo
{
  const char* inspiration;
};
const CabInfo kCabInfo[NexusProcessor::kCabLooks] = {{"1960A 4x12"}, {"Mesa 4x12"}, {"5150 4x12"}, {"Soldano 4x12"}, {"Friedman 4x12"},
                                                    {"Orange 4x12"}, {"Vox AC30 2x12"}, {"Fender Twin 2x12"}, {"Fender 4x10"}, {"Matchless 2x12"}, {"ENGL 4x12"}, {"Bogner 4x12"}, {"Roland JC 2x12"},
                                                    {"Ampeg 8x10"}, {"Gallien-Krueger 4x10"},
                                                    {"Fender Deluxe Reverb 1x12"}, {"Mesa Boogie Mark 1x12"}, {"Blackstar 4x12"}, {"Laney 4x12"}, {"Diezel 4x12"}, {"Hiwatt 4x12"}, {"Randall 4x12"}, {"Hartke 4x10"}, {"Darkglass 4x10"}, {"Sunn 4x12"}, {"Ampeg B-15 1x15"}};

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
    auto font = Fonts::get().displayEm(em).withExtraKerningFactor(kerning);
    const auto text = upper(TRANS(l.text));
    const juce::Point<float> at(image.getX() + l.centre.x * scale, image.getY() + l.centre.y * scale);
    // Palavra longa (ou tradução maior) perto da borda: diminui a letra até
    // caber dentro da peça, com uma folguinha da borda.
    const float left = image.getX() + art.body.getX() * scale, right = image.getX() + art.body.getRight() * scale;
    const float margin = art.body.getWidth() * scale * 0.05f;
    const float room = 2.0f * std::min(at.x - left - margin, right - margin - at.x);
    if (const float width = juce::GlyphArrangement::getStringWidth(font, text); room > 0.0f && width > room)
      font = Fonts::get().displayEm(em * room / width).withExtraKerningFactor(kerning);
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

// Chave alavanca de metal vista de cima: porca sextavada, a alavanca cromada
// apontando para cima ou para baixo e as duas posições escritas acima e
// abaixo. r: raio da porca.
void drawToggle(juce::Graphics& g, juce::Point<float> c, float r, bool up, const juce::String& upText, const juce::String& downText)
{
  // Arruela e porca.
  g.setColour(juce::Colours::black.withAlpha(0.45f));
  g.fillEllipse(c.x - r * 1.25f, c.y - r * 1.05f, r * 2.5f, r * 2.5f);
  g.setGradientFill(juce::ColourGradient(juce::Colour(0xffe9ecf1), c.x - r, c.y - r, juce::Colour(0xff6c717a), c.x + r, c.y + r, false));
  g.fillEllipse(c.x - r * 1.15f, c.y - r * 1.15f, r * 2.3f, r * 2.3f);
  juce::Path hex;
  for (int i = 0; i < 6; ++i)
  {
    const float a = juce::MathConstants<float>::pi / 3.0f * static_cast<float>(i) + juce::MathConstants<float>::pi / 6.0f;
    const juce::Point<float> p{c.x + std::cos(a) * r * 0.85f, c.y + std::sin(a) * r * 0.85f};
    if (i == 0)
      hex.startNewSubPath(p);
    else
      hex.lineTo(p);
  }
  hex.closeSubPath();
  g.setGradientFill(juce::ColourGradient(juce::Colour(0xffc9ced6), c.x - r, c.y - r, juce::Colour(0xff4b5058), c.x + r, c.y + r, false));
  g.fillPath(hex);
  g.setColour(juce::Colours::black.withAlpha(0.35f));
  g.strokePath(hex, juce::PathStrokeType(std::max(1.0f, r * 0.06f)));
  // Alavanca: sai do centro para o lado escolhido, mais larga na ponta.
  const float dir = up ? -1.0f : 1.0f;
  const juce::Point<float> tip{c.x, c.y + dir * r * 1.9f};
  juce::Path lever;
  lever.startNewSubPath(c.x - r * 0.22f, c.y);
  lever.lineTo(tip.x - r * 0.30f, tip.y);
  lever.lineTo(tip.x + r * 0.30f, tip.y);
  lever.lineTo(c.x + r * 0.22f, c.y);
  lever.closeSubPath();
  g.setColour(juce::Colours::black.withAlpha(0.4f));
  g.fillPath(lever, juce::AffineTransform::translation(r * 0.18f, r * 0.22f)); // sombra
  juce::ColourGradient chrome(juce::Colour(0xff7d838c), c.x - r * 0.32f, c.y, juce::Colour(0xff6a7079), c.x + r * 0.32f, c.y, false);
  chrome.addColour(0.45, juce::Colour(0xfff4f6f9));
  g.setGradientFill(chrome);
  g.fillPath(lever);
  g.setGradientFill(juce::ColourGradient(juce::Colour(0xfffbfcfd), tip.x - r * 0.2f, tip.y - r * 0.2f, juce::Colour(0xff737983), tip.x + r * 0.3f,
                                         tip.y + r * 0.3f, true));
  g.fillEllipse(tip.x - r * 0.36f, tip.y - r * 0.36f, r * 0.72f, r * 0.72f);
  // Posições escritas no tampo: a escolhida acesa, a outra apagada.
  const juce::Colour ink(0xfff1e8d4);
  g.setFont(juce::Font(juce::FontOptions(r * 1.3f, juce::Font::bold)));
  const auto box = [&](float dy) { return juce::Rectangle<float>(c.x - r * 1.5f, c.y + dy - r * 0.8f, r * 3.0f, r * 1.6f); };
  g.setColour(ink.withAlpha(up ? 1.0f : 0.45f));
  g.drawText(upText, box(-r * 3.1f), juce::Justification::centred, false);
  g.setColour(ink.withAlpha(up ? 0.45f : 1.0f));
  g.drawText(downText, box(r * 3.1f), juce::Justification::centred, false);
}

// Visor do Cherry Drop: cristal azul sempre aceso com os semitons escolhidos
// (o LED é que mostra se o pedal está ligado).
void drawDropDisplay(juce::Graphics& g, juce::Rectangle<float> r, float knobA)
{
  const int semis = NexusProcessor::dropSemitones(knobA);
  const juce::String big = semis < 0 ? juce::String::fromUTF8("\u2212") + juce::String(-semis) : semis > 0 ? "+" + juce::String(semis) : juce::String("0");
  const juce::String small = std::abs(semis) == 12 ? TRANS("oitava") : TRANS("semitons");
  const float corner = r.getHeight() * 0.10f;
  juce::ColourGradient glass(juce::Colour(0xff2f7bff), r.getCentreX(), r.getY(), juce::Colour(0xff0b2f8f), r.getCentreX(), r.getBottom(), false);
  g.setGradientFill(glass);
  g.fillRoundedRectangle(r, corner);
  // Linhas finas da matriz do cristal.
  g.setColour(juce::Colours::black.withAlpha(0.10f));
  for (float y = r.getY() + 2.0f; y < r.getBottom(); y += 3.0f)
    g.drawHorizontalLine(juce::roundToInt(y), r.getX() + 1.0f, r.getRight() - 1.0f);
  auto text = r.reduced(r.getWidth() * 0.06f, r.getHeight() * 0.08f);
  auto bottom = text.removeFromBottom(text.getHeight() * 0.28f);
  const juce::Colour ink(0xffeaf6ff);
  g.setFont(juce::Font(juce::FontOptions(text.getHeight() * 0.92f, juce::Font::bold)).withExtraKerningFactor(0.04f));
  g.setColour(ink.withAlpha(0.25f)); // brilho em volta do número
  for (const auto d : {-1.2f, 1.2f})
    g.drawText(big, text.translated(d, 0.0f), juce::Justification::centred, false);
  g.setColour(ink);
  g.drawText(big, text, juce::Justification::centred, false);
  g.setFont(juce::Font(juce::FontOptions(bottom.getHeight() * 0.80f, juce::Font::bold)).withExtraKerningFactor(0.12f));
  g.setColour(ink.withAlpha(0.85f));
  g.drawText(small.toUpperCase(), bottom, juce::Justification::centred, false);
  // Reflexo do vidro por cima.
  juce::ColourGradient shine(juce::Colours::white.withAlpha(0.16f), r.getX(), r.getY(), juce::Colours::white.withAlpha(0.0f), r.getX(),
                             r.getCentreY(), false);
  g.setGradientFill(shine);
  g.fillRoundedRectangle(r.withHeight(r.getHeight() * 0.5f), corner);
}

// Visor dos pedais de vários modos: o número e o nome do modo, em cristal
// aceso da cor do pedal (azul-gelo no reverb, verde no delay, lilás na modulação).
void drawModeDisplay(juce::Graphics& g, juce::Rectangle<float> r, int look, float modeKnob)
{
  using K = NexusProcessor::PedalKind;
  const auto kind = NexusProcessor::pedalKind(look);
  const juce::Colour top = kind == K::sky ? juce::Colour(0xff3d6f9e) : kind == K::echo ? juce::Colour(0xff2f8a3a) : juce::Colour(0xff6a3f9a);
  const juce::Colour bottom = top.darker(1.4f);
  const float corner = r.getHeight() * 0.10f;
  g.setGradientFill(juce::ColourGradient(top, r.getCentreX(), r.getY(), bottom, r.getCentreX(), r.getBottom(), false));
  g.fillRoundedRectangle(r, corner);
  g.setColour(juce::Colours::black.withAlpha(0.10f));
  for (float y = r.getY() + 2.0f; y < r.getBottom(); y += 3.0f)
    g.drawHorizontalLine(juce::roundToInt(y), r.getX() + 1.0f, r.getRight() - 1.0f);
  auto text = r.reduced(r.getWidth() * 0.05f, r.getHeight() * 0.10f);
  auto number = text.removeFromTop(text.getHeight() * 0.42f);
  const juce::Colour ink(0xffeefaff);
  g.setColour(ink.withAlpha(0.8f));
  g.setFont(juce::Font(juce::FontOptions(number.getHeight() * 0.9f, juce::Font::bold)).withExtraKerningFactor(0.08f));
  g.drawText(juce::String(juce::roundToInt(modeKnob) + 1).paddedLeft('0', 2), number, juce::Justification::centred, false);
  g.setColour(ink);
  g.setFont(juce::Font(juce::FontOptions(text.getHeight() * 0.62f, juce::Font::bold)).withExtraKerningFactor(0.06f));
  // Maiúsculas com acento (o toUpperCase deixa "ç" e "ô" minúsculos).
  auto name = NexusProcessor::modeName(look, modeKnob).toUpperCase();
  for (const auto* pair : {"çÇ", "áÁ", "âÂ", "ãÃ", "àÀ", "éÉ", "êÊ", "íÍ", "óÓ", "ôÔ", "õÕ", "úÚ"})
  {
    const auto chars = juce::String::fromUTF8(pair);
    name = name.replaceCharacter(chars[0], chars[1]);
  }
  g.drawFittedText(name, text.toNearestInt(), juce::Justification::centred, 2, 0.6f);
  juce::ColourGradient shine(juce::Colours::white.withAlpha(0.16f), r.getX(), r.getY(), juce::Colours::white.withAlpha(0.0f), r.getX(),
                             r.getCentreY(), false);
  g.setGradientFill(shine);
  g.fillRoundedRectangle(r.withHeight(r.getHeight() * 0.5f), corner);
}

juce::String nativeKnobLabel(int look, const juce::String& id)
{
  using K = NexusProcessor::PedalKind;
  const char* const* names = nullptr;
  static const char* const chorus[3] = {"Velocidade", "Profundidade", "Mistura"};
  static const char* const delay[3] = {"Tempo", "Repetições", "Mistura"};
  static const char* const reverb[3] = {"Tamanho", "Tom", "Mistura"};
  static const char* const gate[3] = {"Sensibilidade", "Ataque", "Release"};
  static const char* const drop[3] = {"Drop", "Brilho", ""};
  static const char* const phaser[3] = {"Velocidade", "Profundidade", "Intensidade"};
  static const char* const flanger[3] = {"Velocidade", "Profundidade", "Realimentação"};
  static const char* const tremolo[3] = {"Velocidade", "Profundidade", "Forma"};
  static const char* const vibe[3] = {"Velocidade", "Intensidade", "Mistura"};
  // Equalizador gráfico: "a" a "g" são as sete faixas, do grave ao agudo.
  if (NexusProcessor::pedalKind(look) == K::geq)
  {
    static const char* const bands[7] = {"100 Hz", "200 Hz", "400 Hz", "800 Hz", "1,6 kHz", "3,2 kHz", "6,4 kHz"};
    const int band = id.length() == 1 ? id[0] - 'a' : -1;
    return band >= 0 && band < 7 ? juce::String(bands[band]) : juce::String();
  }
  // Vários modos: a, b, c, d, tom, nível (o shimmer, no reverb) e modo.
  static const char* const sky[6] = {"Decaimento", "Pré-atraso", "Mistura", "Modulação", "Tom", "Shimmer"};
  static const char* const echo[6] = {"Tempo", "Repetições", "Mistura", "Modulação", "Tom", "Nível"};
  static const char* const multi[6] = {"Velocidade", "Profundidade", "Mistura", "Ajuste", "Tom", "Nível"};
  // Blueberry Chain: tempo, feedback e repetição (o volume das repetições).
  if (NexusProcessor::pedalKind(look) == K::triple)
    return juce::String::fromUTF8(id == "a" ? "Tempo" : id == "b" ? "Feedback" : "Repetição");
  if (NexusProcessor::hasModes(look))
  {
    if (id == "modo")
      return "Modo";
    const auto k = NexusProcessor::pedalKind(look);
    const char* const* six = k == K::sky ? sky : k == K::echo ? echo : multi;
    const int i = id == "a" ? 0 : id == "b" ? 1 : id == "c" ? 2 : id == "d" ? 3 : id == "tom" ? 4 : 5;
    return juce::String::fromUTF8(six[i]);
  }
  switch (NexusProcessor::pedalKind(look))
  {
    case K::delay: names = delay; break;
    case K::reverb: names = reverb; break;
    case K::gate: names = gate; break;
    case K::drop: names = drop; break;
    case K::phaser: names = phaser; break;
    case K::flanger: names = flanger; break;
    case K::tremolo: names = tremolo; break;
    case K::vibe: names = vibe; break;
    default: names = chorus; break;
  }
  const int knob = id == "a" ? 0 : id == "b" ? 1 : 2;
  return juce::String::fromUTF8(names[knob]);
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
// Blueberry Chain: as chaves ficam no tampo, de cada lado do LED (posição
// relativa ao corpo, a mesma da receita 3D: x de -1 a 1, y para cima).
// Centro e raio em pixels da imagem da frente.
std::vector<std::pair<juce::Point<float>, float>> chainTogglePixels(const GearArt& art)
{
  std::vector<std::pair<juce::Point<float>, float>> out;
  const auto& b = art.body;
  for (const float x : {-0.56f, 0.56f})
    out.push_back({{b.getCentreX() + x * b.getWidth() * 0.5f, b.getCentreY() - 0.10f * b.getHeight() * 0.5f}, b.getWidth() * 0.035f});
  return out;
}
} // namespace

// ================================================================ GearStage

NexusEditor::GearStage::GearStage(NexusProcessor& p, Kind kind)
: mProcessor(p)
, mKind(kind)
{
  // Andamento (só nos pedais de modulação e no delay; refreshTempo mostra).
  addChildComponent(mSync);
  mSync.setButtonText(TRANS("Sincronizar"));
  mSync.setTooltip(TRANS(juce::String::fromUTF8("A velocidade segue o andamento: o botão escolhe a divisão (1/4, 1/8, 1/16...)")));
  mSync.setColour(juce::TextButton::buttonOnColourId, colour::accent.withAlpha(0.35f));
  mSync.onClick = [this] {
    if (auto* param = mProcessor.params.getParameter(NexusProcessor::pedalParam(mIndex, "sinc")))
      param->setValueNotifyingHost(param->getValue() > 0.5f ? 0.0f : 1.0f);
    refreshTempo();
    repaint();
  };
  addChildComponent(mTap);
  mTap.setTooltip(TRANS(juce::String::fromUTF8("Toque no ritmo da música (3 ou 4 vezes). Botão direito: ligar a uma pisada da pedaleira.")));
  mTap.onClick = [this] {
    if (auto* param = mProcessor.params.getParameter("tap"))
      param->setValueNotifyingHost(param->getValue() > 0.5f ? 0.0f : 1.0f);
  };
  mTap.onRightClick = [this] {
    if (onMidiMenu)
      onMidiMenu("tap", mTap);
  };
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
        if (mKind == Kind::pedal && (k.id == "a" || k.id == "b" || k.id == "c" || k.id == "d" || k.id == "e" || k.id == "f" || k.id == "g"))
          label = nativeKnobLabel(mProcessor.pedalLook(index), k.id);
        if (mKind == Kind::pedal && NexusProcessor::hasModes(mProcessor.pedalLook(index)))
          label = nativeKnobLabel(mProcessor.pedalLook(index), k.id);
        const bool triple = mKind == Kind::pedal && NexusProcessor::pedalKind(mProcessor.pedalLook(index)) == NexusProcessor::PedalKind::triple;
        if (triple)
          label = nativeKnobLabel(mProcessor.pedalLook(index), k.id);
        auto knob = std::make_unique<FilmKnob>(*param, k, label);
        knob->onChange = [this] { repaint(); };
        // Seletor de modo: o balão mostra o nome do modo.
        if (mKind == Kind::pedal && k.id == "modo")
          knob->valueText = [this](float v) { return NexusProcessor::modeName(mProcessor.pedalLook(mIndex), v); };
        // Sincronizado ao andamento: a velocidade (ou o tempo do delay) vira divisão.
        // (no Blueberry Chain, o tempo de cada um dos três delays).
        if (mKind == Kind::pedal && NexusProcessor::syncsToTempo(mProcessor.pedalLook(index))
            && k.id == "a")
          knob->valueText = [this](float v) {
            if (mProcessor.params.getRawParameterValue(NexusProcessor::pedalParam(mIndex, "sinc"))->load() > 0.5f)
              return juce::String(NexusProcessor::divisionName(NexusProcessor::divisionFor(v)));
            // Phaser: a velocidade em Hz, para ajustar com precisão.
            if (NexusProcessor::pedalKind(mProcessor.pedalLook(mIndex)) == NexusProcessor::PedalKind::phaser)
              return juce::String(nexus::Phaser::rateHz(v / 10.0), 2).replaceCharacter('.', ',') + " Hz";
            return juce::String();
          };
        // Cherry Drop: o botão do drop mostra os semitons (meio-dia = 0).
        if (mKind == Kind::pedal && k.id == "a" && mProcessor.pedalLook(index) == NexusProcessor::kDropLook)
          knob->valueText = [](float v) {
            const int semis = NexusProcessor::dropSemitones(v);
            return (semis > 0 ? "+" : "") + juce::String(semis) + " " + TRANS("semitons");
          };
        knob->onMidiMenu = [this](const juce::String& id, juce::Component& c) { if (onMidiMenu) onMidiMenu(id, c); };
        addAndMakeVisible(*knob);
        mKnobs.push_back(std::move(knob));
      }
  refreshTempo();
  resized();
  repaint();
}

void NexusEditor::GearStage::refreshTempo()
{
  const bool show = mKind == Kind::pedal && mArt != nullptr && NexusProcessor::syncsToTempo(mProcessor.pedalLook(mIndex));
  mSync.setVisible(show);
  mTap.setVisible(show);
  if (!show)
    return;
  const bool on = mProcessor.params.getRawParameterValue(NexusProcessor::pedalParam(mIndex, "sinc"))->load() > 0.5f;
  mSync.setToggleState(on, juce::dontSendNotification);
  const auto bpm = juce::String(mProcessor.tempoBpm(), 1).replaceCharacter('.', ',') + " BPM";
  // Dentro do programa de música, o andamento é o da música (o tap não vale).
  const auto text = mProcessor.tempoFromHost() ? bpm + TRANS(juce::String::fromUTF8(" (música)")) : "TAP  " + bpm;
  if (mTap.getButtonText() != text)
    mTap.setButtonText(text);
  mTap.setEnabled(!mProcessor.tempoFromHost());
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

void NexusEditor::GearStage::setArea(juce::Rectangle<float> area)
{
  if (area == mArea)
    return;
  mArea = area;
  resized();
  repaint();
}

bool NexusEditor::GearStage::hitTest(int x, int y)
{
  if (mArea.isEmpty())
    return true;
  const juce::Point<int> p{x, y};
  return mImage.contains(p.toFloat()) || (mSync.isVisible() && mSync.getBounds().contains(p)) || (mTap.isVisible() && mTap.getBounds().contains(p));
}

void NexusEditor::GearStage::resized()
{
  if (mArt == nullptr)
    return;
  mImage = fitImage(mArt->front, mArea.isEmpty() ? getLocalBounds().toFloat().reduced(24.0f, 18.0f) : mArea);
  mScale = mImage.getWidth() / static_cast<float>(std::max(1, mArt->front.getWidth()));
  for (auto& knob : mKnobs)
  {
    const auto& box = knob->art().box;
    knob->setBounds(juce::Rectangle<float>(mImage.getX() + box.getX() * mScale, mImage.getY() + box.getY() * mScale,
                                           box.getWidth() * mScale, box.getHeight() * mScale)
                      .toNearestInt());
  }
  // Andamento: ao lado direito da peça, na altura do meio.
  const auto body = juce::Rectangle<float>(mImage.getX() + mArt->body.getX() * mScale, mImage.getY() + mArt->body.getY() * mScale,
                                           mArt->body.getWidth() * mScale, mArt->body.getHeight() * mScale);
  if (!mArea.isEmpty())
  {
    // Na pedaleira: no canto de cima, à direita, fora dos pedais.
    mTap.setBounds(getWidth() - 186, 6, 170, 30);
    mSync.setBounds(mTap.getX() - 182, 6, 170, 30);
    return;
  }
  const int x = juce::roundToInt(body.getRight()) + 40;
  const int y = juce::roundToInt(body.getCentreY()) - 40;
  mSync.setBounds(x, y, 170, 34);
  mTap.setBounds(x, y + 46, 170, 34);
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
  if (mKind == Kind::pedal && mArt != nullptr && !mArt->display.isEmpty())
  {
    const auto tl = toScreen(mArt->display.getTopLeft());
    const juce::Rectangle<float> r{tl.x, tl.y, mArt->display.getWidth() * mScale, mArt->display.getHeight() * mScale};
    if (const int look = mProcessor.pedalLook(mIndex); NexusProcessor::hasModes(look))
      drawModeDisplay(g, r, look, mProcessor.params.getRawParameterValue(NexusProcessor::pedalParam(mIndex, "modo"))->load());
    else
      drawDropDisplay(g, r, mProcessor.params.getRawParameterValue(NexusProcessor::pedalParam(mIndex, "a"))->load());
  }
  // Blueberry Chain: as duas chaves alavanca (2 | 3 delays e x | /).
  const auto toggles = chainToggles();
  for (size_t t = 0; t < toggles.size(); ++t)
  {
    const bool up = mProcessor.params.getRawParameterValue(NexusProcessor::pedalParam(mIndex, NexusProcessor::kChainSwitches[t]))->load() > 0.5f;
    drawToggle(g, toggles[t].first, toggles[t].second, up, t == 0 ? "3" : juce::String::fromUTF8("\u00d7"),
               t == 0 ? "2" : juce::String::fromUTF8("\u00f7"));
  }
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

std::vector<std::pair<juce::Point<float>, float>> NexusEditor::GearStage::chainToggles() const
{
  std::vector<std::pair<juce::Point<float>, float>> out;
  if (mKind != Kind::pedal || mArt == nullptr || NexusProcessor::pedalKind(mProcessor.pedalLook(mIndex)) != NexusProcessor::PedalKind::triple)
    return out;
  for (const auto& [px, r] : chainTogglePixels(*mArt))
    out.push_back({toScreen(px), r * mScale});
  return out;
}

void NexusEditor::GearStage::mouseUp(const juce::MouseEvent& e)
{
  if (mKind != Kind::pedal)
    return;
  const auto toggles = chainToggles();
  for (size_t t = 0; t < toggles.size(); ++t)
    if (e.position.getDistanceFrom(toggles[t].first) <= toggles[t].second * 2.5f)
    {
      const auto id = NexusProcessor::pedalParam(mIndex, NexusProcessor::kChainSwitches[t]);
      if (e.mods.isPopupMenu())
      {
        if (onMidiMenu)
          onMidiMenu(id, *this);
        return;
      }
      auto* param = mProcessor.params.getParameter(id);
      param->setValueNotifyingHost(param->getValue() > 0.5f ? 0.0f : 1.0f);
      repaint();
      return;
    }
  if (!overFootswitch(e.position))
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

// ================================================================ BoardView

namespace
{
// Cabo de pedal visto de cima: sai do topo de um pedal (plugue em pé), faz um
// arco por trás e entra no topo do outro.
void drawPatchCable(juce::Graphics& g, juce::Point<float> from, juce::Point<float> to, float rise, float thick)
{
  juce::Path cable;
  cable.startNewSubPath(from);
  cable.cubicTo(from.translated(0.0f, -rise), to.translated(0.0f, -rise), to);
  g.setColour(juce::Colours::black.withAlpha(0.45f)); // sombra no velcro
  g.strokePath(cable, juce::PathStrokeType(thick * 1.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded),
               juce::AffineTransform::translation(thick * 0.5f, thick * 0.8f));
  g.setColour(juce::Colour(0xff0d0d0e));
  g.strokePath(cable, juce::PathStrokeType(thick, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
  g.setColour(juce::Colour(0xff3c3e42)); // brilho da capa de borracha
  g.strokePath(cable, juce::PathStrokeType(thick * 0.28f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded),
               juce::AffineTransform::translation(-thick * 0.2f, -thick * 0.2f));
  // Plugues: corpo de metal escuro com o anel cromado.
  for (const auto& p : {from, to})
  {
    const juce::Rectangle<float> plug(p.x - thick * 0.95f, p.y - thick * 2.6f, thick * 1.9f, thick * 2.8f);
    g.setGradientFill(juce::ColourGradient(juce::Colour(0xff5b5f66), plug.getX(), 0.0f, juce::Colour(0xff17181a), plug.getRight(), 0.0f, false));
    g.fillRoundedRectangle(plug, thick * 0.4f);
    g.setColour(juce::Colour(0xffc9ccd2));
    g.fillRect(plug.getX(), plug.getBottom() - thick * 0.7f, plug.getWidth(), thick * 0.35f);
  }
}
} // namespace

NexusEditor::BoardView::BoardView(NexusEditor& editor)
: mEditor(editor)
, mBoard(GearArt::load("pedaleira"))
{
}

std::vector<NexusEditor::BoardView::Slot> NexusEditor::BoardView::layoutSlots() const
{
  std::vector<Slot> out;
  // Na ordem do som: antes, entre cabeçote e caixa (A e B) e depois.
  const auto layout = mEditor.mProcessor.layout();
  std::vector<int> pedals(layout.pre);
  for (const auto* v : {&layout.loop[0], &layout.loop[1], &layout.post})
    if (v != &layout.loop[1] || layout.dual)
      pedals.insert(pedals.end(), v->begin(), v->end());
  if (pedals.empty() || mBoardImage.isEmpty())
    return out;

  // Faixa útil: entre as cabeceiras, deixando a ripa de trás para os cabos.
  const auto inner = mBoardImage.reduced(mBoardImage.getWidth() * 0.05f, 0.0f)
                       .withTrimmedTop(mBoardImage.getHeight() * 0.13f)
                       .withTrimmedBottom(mBoardImage.getHeight() * 0.04f);
  const float gap = inner.getWidth() * 0.02f;
  // Até quatro pedais, uma fileira; mais que isso, duas (como numa pedaleira
  // de verdade): a de trás com o começo do som, a da frente com o resto.
  const size_t rows = pedals.size() > 4 ? 2 : 1;
  const size_t perRow = (pedals.size() + rows - 1) / rows;
  const float rowGap = rows > 1 ? mBoardImage.getHeight() * 0.10f : 0.0f; // espaço dos cabos da fileira da frente
  const float rowH = (inner.getHeight() - rowGap) / static_cast<float>(rows);
  auto bodyOf = [this](int p) {
    const auto* art = mEditor.pedalArt(p);
    return art != nullptr && !art->body.isEmpty() ? art->body : juce::Rectangle<float>(0.0f, 0.0f, 360.0f, 600.0f);
  };
  // Mesma altura para todos: a da fileira mais larga, se não couber inteira.
  float h = rowH;
  for (size_t r = 0; r < rows; ++r)
  {
    float aspect = 0.0f; // soma das larguras com altura 1
    size_t n = 0;
    for (size_t i = r * perRow; i < std::min(pedals.size(), (r + 1) * perRow); ++i, ++n)
      aspect += bodyOf(pedals[i]).getWidth() / bodyOf(pedals[i]).getHeight();
    h = std::min(h, (inner.getWidth() - gap * static_cast<float>(n - 1)) / aspect);
  }
  for (size_t r = 0; r < rows; ++r)
  {
    const size_t first = r * perRow, last = std::min(pedals.size(), (r + 1) * perRow);
    float width = gap * static_cast<float>(last - first - 1);
    for (size_t i = first; i < last; ++i)
      width += bodyOf(pedals[i]).getWidth() / bodyOf(pedals[i]).getHeight() * h;
    const float cy = inner.getY() + rowH * 0.5f + static_cast<float>(r) * (rowH + rowGap);
    float x = inner.getCentreX() - width * 0.5f;
    for (size_t i = first; i < last; ++i)
    {
      const auto* art = mEditor.pedalArt(pedals[i]);
      const auto b = bodyOf(pedals[i]);
      const float k = h / b.getHeight();
      const juce::Rectangle<float> body(x, cy - h * 0.5f, b.getWidth() * k, h);
      const auto image = art != nullptr ? juce::Rectangle<float>(body.getX() - b.getX() * k, body.getY() - b.getY() * k,
                                                                 static_cast<float>(art->front.getWidth()) * k,
                                                                 static_cast<float>(art->front.getHeight()) * k)
                                        : body;
      out.push_back({pedals[i], image, body, k});
      x += body.getWidth() + gap;
    }
  }
  return out;
}

juce::Rectangle<float> NexusEditor::BoardView::imageOf(int pedal) const
{
  for (const auto& s : layoutSlots())
    if (s.pedal == pedal)
      return s.image;
  return {};
}

const NexusEditor::BoardView::Slot* NexusEditor::BoardView::slotAt(juce::Point<float> p) const
{
  for (const auto& s : mSlots)
    if (s.body.contains(p))
      return &s;
  return nullptr;
}

bool NexusEditor::BoardView::overFootswitch(const Slot& s, juce::Point<float> p) const
{
  const auto* art = mEditor.pedalArt(s.pedal);
  if (art == nullptr || art->footswitchRadius <= 0.0f)
    return false;
  const auto c = s.image.getTopLeft() + art->footswitch * s.scale;
  return p.getDistanceFrom(c) <= std::max(10.0f, art->footswitchRadius * s.scale * 1.3f);
}

juce::String NexusEditor::BoardView::signature() const
{
  auto& params = mEditor.mProcessor.params;
  const auto value = [&params](int pedal, const char* id) {
    auto* raw = params.getRawParameterValue(NexusProcessor::pedalParam(pedal, id));
    return raw != nullptr ? juce::String(raw->load(), 2) : juce::String();
  };
  const auto layout = mEditor.mProcessor.layout();
  juce::String sig = layout.toValueTree().toXmlString() + juce::String(getWidth()) + "x" + juce::String(getHeight());
  sig << "|" << (mEditor.mSel.kind == Kind::pedal ? mEditor.mSel.index : -1);
  for (int p = 0; p < ChainLayout::kPedals; ++p)
  {
    if (!layout.uses(p))
      continue;
    sig << "|" << mEditor.pedalLook(p) << value(p, "ligado") << value(p, "modo");
    if (const auto* art = mEditor.pedalArt(p))
      for (const auto& k : art->knobs)
        sig << "," << value(p, k.id == "ganho" ? "drive" : k.id.toRawUTF8());
    for (const auto* id : NexusProcessor::kChainSwitches)
      sig << value(p, id);
  }
  return sig;
}

void NexusEditor::BoardView::tick()
{
  if (auto sig = signature(); sig != mSignature)
  {
    mSignature = sig;
    repaint();
  }
}

void NexusEditor::BoardView::resized()
{
  // Espaço de cima livre (sincronizar e tap do pedal escolhido).
  const auto area = getLocalBounds().toFloat().withTrimmedTop(40.0f).reduced(14.0f, 2.0f);
  mBoardImage = mBoard.valid() ? fitImage(mBoard.front, area) : area;
  mSignature.clear();
}

void NexusEditor::BoardView::paint(juce::Graphics& g)
{
  mSlots = layoutSlots();
  if (mBoard.valid())
    drawGear(g, mBoard.front, mBoardImage);
  else
  {
    g.setColour(juce::Colour(0xff0c0c0d));
    g.fillRoundedRectangle(mBoardImage, 12.0f);
  }
  if (mSlots.empty())
  {
    drawCaps(g, TRANS("Arraste um pedal da coluna da direita para a cadeia"), mBoardImage, 12.0f, juce::Justification::centred, colour::dim);
    return;
  }

  // Cabos: entrada pela esquerda, de pedal em pedal, e saída pela direita.
  const float thick = juce::jlimit(3.0f, 7.0f, mSlots.front().body.getWidth() * 0.035f);
  const float rise = mBoardImage.getHeight() * 0.09f;
  const auto in = [](const Slot& s) { return juce::Point<float>(s.body.getX() + s.body.getWidth() * 0.26f, s.body.getY() + 2.0f); };
  const auto out = [](const Slot& s) { return juce::Point<float>(s.body.getRight() - s.body.getWidth() * 0.26f, s.body.getY() + 2.0f); };
  drawPatchCable(g, {mBoardImage.getX() + mBoardImage.getWidth() * 0.012f, mSlots.front().body.getY() - rise * 0.7f}, in(mSlots.front()),
                 rise * 0.4f, thick);
  // Da fileira de trás para a da frente, o cabo passa por baixo dos pedais.
  for (size_t i = 0; i + 1 < mSlots.size(); ++i)
    drawPatchCable(g, out(mSlots[i]), in(mSlots[i + 1]), rise, thick);
  drawPatchCable(g, out(mSlots.back()), {mBoardImage.getRight() - mBoardImage.getWidth() * 0.012f, mSlots.back().body.getY() - rise * 0.7f},
                 rise * 0.4f, thick);

  auto& params = mEditor.mProcessor.params;
  for (const auto& s : mSlots)
  {
    const bool chosen = mEditor.mSel.kind == Kind::pedal && mEditor.mSel.index == s.pedal;
    if (chosen)
    {
      // O escolhido: brilho ciano por baixo; o GearStage desenha o pedal.
      g.setColour(colour::accent.withAlpha(0.16f));
      g.fillRoundedRectangle(s.body.expanded(7.0f), 12.0f);
      g.setColour(colour::accent.withAlpha(0.75f));
      g.drawRoundedRectangle(s.body.expanded(7.0f), 12.0f, 1.6f);
      continue;
    }
    const auto* art = mEditor.pedalArt(s.pedal);
    if (art == nullptr)
    {
      g.setColour(juce::Colour(0xff2a2d33));
      g.fillRoundedRectangle(s.body, 6.0f);
      continue;
    }
    drawGear(g, art->front, s.image);
    drawGearLabels(g, *art, s.image);
    const auto at = [&s](juce::Point<float> px) { return s.image.getTopLeft() + px * s.scale; };
    g.setImageResamplingQuality(juce::Graphics::highResamplingQuality);
    for (const auto& k : art->knobs)
      if (auto* param = params.getParameter(NexusProcessor::pedalParam(s.pedal, k.id == "ganho" ? "drive" : k.id.toRawUTF8())))
      {
        const auto box = k.box.toFloat();
        g.drawImage(k.frame(param->getValue()), {s.image.getX() + box.getX() * s.scale, s.image.getY() + box.getY() * s.scale,
                                                 box.getWidth() * s.scale, box.getHeight() * s.scale},
                    juce::RectanglePlacement::stretchToFit);
      }
    const int look = mEditor.pedalLook(s.pedal);
    if (!art->display.isEmpty())
    {
      const auto tl = at(art->display.getTopLeft());
      const juce::Rectangle<float> r{tl.x, tl.y, art->display.getWidth() * s.scale, art->display.getHeight() * s.scale};
      if (NexusProcessor::hasModes(look))
        drawModeDisplay(g, r, look, params.getRawParameterValue(NexusProcessor::pedalParam(s.pedal, "modo"))->load());
      else
        drawDropDisplay(g, r, params.getRawParameterValue(NexusProcessor::pedalParam(s.pedal, "a"))->load());
    }
    if (NexusProcessor::pedalKind(look) == NexusProcessor::PedalKind::triple)
    {
      const auto toggles = chainTogglePixels(*art);
      for (size_t t = 0; t < toggles.size(); ++t)
      {
        const bool up = params.getRawParameterValue(NexusProcessor::pedalParam(s.pedal, NexusProcessor::kChainSwitches[t]))->load() > 0.5f;
        drawToggle(g, at(toggles[t].first), toggles[t].second * s.scale, up, t == 0 ? "3" : juce::String::fromUTF8("×"),
                   t == 0 ? "2" : juce::String::fromUTF8("÷"));
      }
    }
    // LED: o que diz se o pedal está ligado (com um tamanho mínimo para ver de longe).
    if (art->ledRadius > 0.0f && params.getRawParameterValue(NexusProcessor::pedalParam(s.pedal, "ligado"))->load() > 0.5f)
      drawLed(g, at(art->led), std::max(2.6f, art->ledRadius * s.scale));
  }
}

void NexusEditor::BoardView::mouseMove(const juce::MouseEvent& e)
{
  setMouseCursor(slotAt(e.position) != nullptr ? juce::MouseCursor::PointingHandCursor : juce::MouseCursor::NormalCursor);
}

void NexusEditor::BoardView::mouseUp(const juce::MouseEvent& e)
{
  const auto* s = slotAt(e.position);
  if (s == nullptr)
    return;
  const int pedal = s->pedal;
  // Pisante: liga e desliga sem trocar o pedal escolhido.
  if (overFootswitch(*s, e.position))
  {
    const auto id = NexusProcessor::pedalParam(pedal, "ligado");
    if (e.mods.isPopupMenu())
    {
      mEditor.showMidiMenu(id, *this);
      return;
    }
    auto* on = mEditor.mProcessor.params.getParameter(id);
    on->setValueNotifyingHost(on->getValue() > 0.5f ? 0.0f : 1.0f);
    tick();
    return;
  }
  if (!e.mods.isPopupMenu())
    mEditor.select({Kind::pedal, pedal});
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
  mArea = area.reduced(24.0f, 8.0f);
}

void NexusEditor::CabStage::paint(juce::Graphics& g)
{
  // Cada caixa tem a sua proporção (a 8x10 é em pé): o encaixe é refeito a
  // cada desenho, não só quando o palco muda de tamanho.
  mImage = fitImage(art().front, mArea);
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
    w += cw * 0.86f + gap;                                                            // afinador do começo
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

  // Afinador: fixo no começo, antes de qualquer pedal.
  addCard(CardType::tuner, 0, cw * 0.86f, cw * 0.86f, cy);
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
    case CardType::tuner:
      nexus::ui::TunerStage::paintThumb(g, c.r, 1.0f);
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
    case CardType::tuner: name = TRANS("Afinador"); break;
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
  case CardType::tuner: mEditor.select({Kind::tuner, 0}); break;
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
    mEditor.ensurePedal(pedal); // a captura do pedal novo entra sozinha
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
  if (c.type == CardType::tuner)
  {
    const bool isMuted = proc.params.getRawParameterValue("afinador_mudo")->load() > 0.5f;
    juce::PopupMenu menu;
    menu.addItem(TRANS("Mudo ao afinar"), true, isMuted, [this, isMuted] {
      mEditor.mProcessor.params.getParameter("afinador_mudo")->setValueNotifyingHost(isMuted ? 0.0f : 1.0f);
    });
    menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(this));
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

void NexusEditor::Look::drawAlertBox(juce::Graphics& g, juce::AlertWindow& alert, const juce::Rectangle<int>& textArea,
                                     juce::TextLayout& textLayout)
{
  const auto bounds = alert.getLocalBounds().toFloat();
  g.setColour(alert.findColour(juce::AlertWindow::backgroundColourId));
  g.fillRoundedRectangle(bounds, 8.0f);
  g.setColour(alert.findColour(juce::AlertWindow::outlineColourId));
  g.drawRoundedRectangle(bounds.reduced(0.5f), 8.0f, 1.0f);
  if (alert.getAlertType() != juce::MessageBoxIconType::NoIcon)
  {
    const auto tone = alert.getAlertType() == juce::MessageBoxIconType::WarningIcon ? colour::danger : colour::accent;
    g.setColour(tone);
    g.fillRoundedRectangle(bounds.withWidth(4.0f).reduced(0.0f, 10.0f).translated(8.0f, 0.0f), 2.0f);
  }
  g.setColour(alert.findColour(juce::AlertWindow::textColourId));
  textLayout.draw(g, textArea.toFloat());
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

  {
    // Arte de todos os equipamentos (abrir as imagens é o que mais demora ao
    // abrir a tela): em paralelo, uma peça por vez em cada núcleo.
    const auto started = juce::Time::getMillisecondCounterHiRes();
    std::vector<std::function<void()>> jobs;
    for (int i = 0; i < NexusProcessor::kAmpLooks; ++i)
      jobs.push_back([this, i] { mAmpArts[static_cast<size_t>(i)] = GearArt::load(kAmpPrefixes[i]); });
    for (int i = 0; i < NexusProcessor::kCabLooks; ++i)
      jobs.push_back([this, i] { mCabArts[static_cast<size_t>(i)] = GearArt::load(kCabPrefixes[i]); });
    for (int i = 0; i < kPedalLooks; ++i)
      jobs.push_back([this, i] { mPedalArts[static_cast<size_t>(i)] = GearArt::load(kPedalPrefixes[i]); });
    std::atomic<size_t> next{0};
    std::vector<std::thread> workers;
    for (int t = 0; t < juce::jlimit(1, 8, juce::SystemStats::getNumCpus()); ++t)
      workers.emplace_back([&] {
        for (size_t j = next++; j < jobs.size(); j = next++)
          jobs[j]();
      });
    for (auto& w : workers)
      w.join();
    if (juce::SystemStats::getEnvironmentVariable("NEXUS_DEPURAR", {}).isNotEmpty())
      std::printf("arte dos equipamentos: %.0f ms\n", juce::Time::getMillisecondCounterHiRes() - started);
  }

  addAndMakeVisible(mEngine);
  mEngine.setFontSize(11.5f);
  mEngine.onSelect = [this](int i) { mProcessor.params.getParameter("lite")->setValueNotifyingHost(i == 1 ? 1.0f : 0.0f); };
  addAndMakeVisible(mInfoButton);
  mInfoButton.onClick = [this] { showAbout(); };
  addAndMakeVisible(mLanguageButton);
  mLanguageButton.onClick = [this] { showLanguageMenu(); };

  mBoard = std::make_unique<BoardView>(*this);
  mPedalStage = std::make_unique<GearStage>(p, Kind::pedal);
  mAmpStage = std::make_unique<GearStage>(p, Kind::amp);
  mCabStage = std::make_unique<CabStage>(p, mCabArts.data());
  addChildComponent(*mBoard); // por baixo do pedal escolhido
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
  mTunerStage = std::make_unique<nexus::ui::TunerStage>(mProcessor);
  addChildComponent(*mTunerStage);
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
  // Depois que o estado salvo terminou de abrir as peças: completa as vazias.
  juce::Timer::callAfterDelay(1500, [safe = juce::Component::SafePointer<NexusEditor>(this)] {
    if (safe != nullptr)
      safe->ensureDefaults();
  });
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
  ensurePedal(pedal);
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
  {"Neptune Chorus", "CE-2", "Modulação", "Efeito do Nexus"},
  {"Coconut Delay", "DD-3", "Delay", "Efeito do Nexus"},
  {"Grape Reverb", "RV-6", "Reverb", "Efeito do Nexus"},
  {"Pear Gate", "MXR Smart Gate", "Utilidades", "Efeito do Nexus"},
  {"Cherry Drop", "DigiTech Drop", "Afinação", "Efeito do Nexus"},
  {"Mars Phaser", "MXR Phase 90", "Modulação", "Efeito do Nexus"},
  {"Nebula Flanger", "Electric Mistress", "Modulação", "Efeito do Nexus"},
  {"Moon Tremolo", "TR-2", "Modulação", "Efeito do Nexus"},
  {"Eclipse Vibe", "Uni-Vibe", "Modulação", "Efeito do Nexus"},
  {"Açaí Rat", "ProCo RAT", "Drive e distorção", "Captura da TONE3000"},
  {"Jabuticaba Buzzsaw", "HM-2", "Drive e distorção", "Captura da TONE3000"},
  {"Lychee Fuzz", "Big Muff", "Drive e distorção", "Captura da TONE3000"},
  {"Avocado Metal", "MT-2", "Drive e distorção", "Captura da TONE3000"},
  {"Pomegranate Mud", "Mudhoney", "Drive e distorção", "Captura da TONE3000"},
  {"Snow Pear Treble", "Rangemaster", "Drive e distorção", "Captura da TONE3000"},
  {"Juniper Sky", "Strymon BigSky", "Reverb", "Efeito do Nexus"},
  {"Lime Echo", "Line 6 DL4", "Delay", "Efeito do Nexus"},
  {"Galaxy Mod", "Line 6 MM4", "Modulação", "Efeito do Nexus"},
  {"Blueberry Chain", "Três DD-3 em série", "Delay", "Efeito do Nexus"},
  {"Olive Drive", "Ibanez TS808 Tube Screamer", "Drive e distorção", "Captura da TONE3000"},
  {"Pistachio Drive", "Maxon OD808", "Drive e distorção", "Captura da TONE3000"},
  {"Blueberry Blues", "Boss BD-2 Blues Driver", "Drive e distorção", "Captura da TONE3000"},
  {"Apricot Overdrive", "Boss OD-3", "Drive e distorção", "Captura da TONE3000"},
  {"Clementine Turbo", "Boss DS-2 Turbo Distortion", "Drive e distorção", "Captura da TONE3000"},
  {"Cherry Fuzz", "Dunlop Fuzz Face", "Drive e distorção", "Captura da TONE3000"},
  {"Fig Bender", "Tone Bender", "Drive e distorção", "Captura da TONE3000"},
  {"Kiwi Drive", "Fulltone OCD", "Drive e distorção", "Captura da TONE3000"},
  {"Quince Plus", "MXR Distortion+", "Drive e distorção", "Captura da TONE3000"},
  {"Starfruit Rock", "Rockman X100", "Drive e distorção", "Captura da TONE3000"},
  {"Tamarind Amp", "Tech 21 SansAmp", "Drive e distorção", "Captura da TONE3000"},
  {"Blackcurrant Bass", "Darkglass B7K", "Drive e distorção", "Captura da TONE3000"},
  {"Mulberry Omega", "Darkglass Alpha Omega", "Drive e distorção", "Captura da TONE3000"},
  {"Guava Precision", "Horizon Devices Precision Drive", "Drive e distorção", "Captura da TONE3000"},
  {"Peach Glory", "JHS Morning Glory", "Drive e distorção", "Captura da TONE3000"},
  {"Raspberry Drive", "JHS Andy Timmons", "Drive e distorção", "Captura da TONE3000"},
  {"Honeydew Drive", "Paul Cochrane Timmy", "Drive e distorção", "Captura da TONE3000"},
  {"Pineapple King", "Analogman King of Tone", "Drive e distorção", "Captura da TONE3000"},
  {"Cashew Boost", "Xotic EP Booster", "Drive e distorção", "Captura da TONE3000"},
  {"Papaya Grind", "Fortin Grind", "Drive e distorção", "Captura da TONE3000"},
  {"Persimmon 33", "Fortin 33", "Drive e distorção", "Captura da TONE3000"},
  {"Strawberry 5150", "EVH 5150 Overdrive", "Drive e distorção", "Captura da TONE3000"},
  {"Gooseberry G3", "Revv G3", "Drive e distorção", "Captura da TONE3000"},
  {"Nectarine OD", "Friedman BE-OD", "Drive e distorção", "Captura da TONE3000"},
  {"Cranberry Wylde", "MXR Wylde Overdrive", "Drive e distorção", "Captura da TONE3000"},
  {"Kumquat Klone", "Wampler Tumnus", "Drive e distorção", "Captura da TONE3000"},
  {"Elderberry War", "Death By Audio Fuzz War", "Drive e distorção", "Captura da TONE3000"},
  {"Mangosteen Fuzz", "EarthQuaker Hizumitas", "Drive e distorção", "Captura da TONE3000"},
  {"Grapefruit Drive", "EarthQuaker Plumes", "Drive e distorção", "Captura da TONE3000"},
  {"Jackfruit Monkey", "DigiTech Bad Monkey", "Drive e distorção", "Captura da TONE3000"},
  {"Pitanga 250", "DOD Overdrive Preamp 250", "Drive e distorção", "Captura da TONE3000"},
  {"Lychee Bass Fuzz", "Electro-Harmonix Bass Big Muff", "Drive e distorção", "Captura da TONE3000"},
  {"Cupuaçu Hammer", "Aguilar Tone Hammer", "Drive e distorção", "Captura da TONE3000"},
  {"Graviola Bass DI", "MXR M80 Bass DI", "Drive e distorção", "Captura da TONE3000"},
  {"Acerola EQ", "Boss GE-7", "Utilidades", "Efeito do Nexus"},
  {"Comet Wah", "Dunlop Cry Baby", "Modulação", "Efeito do Nexus"},
  {"Saturn Rotary", "Leslie (caixa rotativa)", "Modulação", "Efeito do Nexus"},
  {"Pulsar Echo", "Electro-Harmonix Memory Man", "Delay", "Efeito do Nexus"},
  {"Jupiter Octave", "Boss OC-2", "Afinação", "Efeito do Nexus"},
  {"Apple Comp", "MXR Dyna Comp", "Utilidades", "Efeito do Nexus"},
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
  {"5150", "Americanos de alto ganho"},
  {"Dual Rectifier", "Americanos de alto ganho"},
  {"SLO-100", "Americanos de alto ganho"},
  {"BE-100", "Americanos de alto ganho"},
  {"AC30", "Britânicos"},
  {"DC-30", "Americanos boutique"},
  {"Tiny Terror", "Britânicos"},
  {"Rockerverb 50", "Britânicos"},
  {"Triple Rectifier", "Americanos de alto ganho"},
  {"Powerball", "Alemães de alto ganho"},
  {"Uberschall", "Alemães de alto ganho"},
  {"Super-Sonic", "Americanos clássicos"},
  {"Twin Reverb", "Americanos clássicos"},
  {"Bassman 5F6-A", "Americanos clássicos"},
  {"Super Lead 1959", "Britânicos"},
  {"Silver Jubilee 2555", "Britânicos"},
  {"JC-120", "Limpos de transistor"},
  {"Ampeg SVT", "Baixo"},
  {"Orange AD200", "Baixo"},
  {"Gallien-Krueger 800RB", "Baixo"},
  {"Ampeg B-15", "Baixo"},
  {"Ampeg V-4B", "Baixo"},
  {"Hartke LH1000", "Baixo"},
  {"Eden", "Baixo"},
  {"Darkglass Microtubes 500", "Baixo"},
  {"Sunn Model T", "Baixo"},
  {"Fender Bassman 100T", "Baixo"},
  {"Aguilar DB751", "Baixo"},
  {"Markbass Little Mark Tube", "Baixo"},
  {"SWR Bass 350", "Baixo"},
  {"Mesa Bass 800D", "Baixo"},
  {"Acoustic 360", "Baixo"},
  {"Orange Terror Bass", "Baixo"},
  {"Trace Elliot GP7", "Baixo"},
  {"Marshall Super Bass", "Baixo"},
  {"Marshall 1987 Lead 50 W", "Britânicos"},
  {"Marshall JTM45", "Britânicos"},
  {"Marshall JCM900", "Britânicos"},
  {"Marshall JVM410", "Britânicos"},
  {"Marshall JCM2000 (DSL e TSL)", "Britânicos"},
  {"Marshall DSL (série nova)", "Britânicos"},
  {"Marshall JMP-1", "Britânicos"},
  {"Fender Deluxe Reverb", "Americanos clássicos"},
  {"Fender Super Reverb", "Americanos clássicos"},
  {"Fender Princeton Reverb", "Americanos clássicos"},
  {"Fender Tweed Deluxe 5E3", "Americanos clássicos"},
  {"Fender Hot Rod Deluxe", "Americanos clássicos"},
  {"Fender Champ", "Americanos clássicos"},
  {"Mesa Boogie Mark IIC+", "Americanos de alto ganho"},
  {"Mesa Boogie Mark III", "Americanos de alto ganho"},
  {"Mesa Boogie Mark IV", "Americanos de alto ganho"},
  {"Mesa Boogie JP-2C", "Americanos de alto ganho"},
  {"Mesa Boogie Badlander", "Americanos de alto ganho"},
  {"Mesa Boogie Lonestar", "Americanos clássicos"},
  {"EVH 5150 III", "Americanos de alto ganho"},
  {"Vox AC15", "Britânicos"},
  {"Orange OR50", "Britânicos"},
  {"Orange AD30", "Britânicos"},
  {"Laney Supergroup", "Britânicos"},
  {"Laney Ironheart", "Britânicos"},
  {"ENGL Fireball", "Alemães de alto ganho"},
  {"ENGL Savage", "Alemães de alto ganho"},
  {"ENGL Ironball", "Alemães de alto ganho"},
  {"Diezel VH4", "Alemães de alto ganho"},
  {"Diezel Herbert", "Alemães de alto ganho"},
  {"Bogner Ecstasy", "Alemães de alto ganho"},
  {"Bogner Shiva", "Alemães de alto ganho"},
  {"Friedman Dirty Shirley", "Americanos de alto ganho"},
  {"Dumble Overdrive Special", "Americanos boutique"},
  {"Two-Rock Classic Reverb", "Americanos boutique"},
  {"Bad Cat Hot Cat", "Americanos boutique"},
  {"Hiwatt DR103", "Britânicos"},
  {"Blackstar HT e Series One", "Britânicos"},
  {"Hughes & Kettner TriAmp", "Alemães de alto ganho"},
  {"Revv Generator 120", "Americanos de alto ganho"},
  {"Fortin Natas", "Americanos de alto ganho"},
  {"Tone King Imperial", "Americanos clássicos"},
  {"Randall RG100ES", "Americanos de alto ganho"},
  {"PRS Archon", "Americanos de alto ganho"},
  {"Victory Kraken", "Britânicos"},
  {"Supro Black Magick", "Americanos clássicos"},
  {"Fryette Deliverance", "Americanos de alto ganho"},
  {"Ashdown ABM", "Baixo"},
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
    it.name = juce::String::fromUTF8(kPedalInfo[i].name);
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
    amp.name = mAmpArts[static_cast<size_t>(i)].name.isNotEmpty() ? mAmpArts[static_cast<size_t>(i)].name
                                                                  : juce::String(kAmpPrefixes[i]);
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
    // Grupo pelo tamanho no fim do nome ("4x12", "2x12", "8x10"...).
    cab.category = juce::String(kCabInfo[i].inspiration).fromLastOccurrenceOf(" ", false, false);
    cab.source = "IR ou captura full rig";
    items.push_back(cab);
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
      ensurePedal(mSel.index);
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
    ensureCab(rig, true);
    select({Kind::amp, rig});
    break;
  }
  case GearItem::Kind::cab:
  {
    const int rig = mSel.kind == Kind::pedal ? 0 : mSel.index;
    if (auto* cab = mProcessor.params.getParameter(NexusProcessor::ampParam(rig, "gabinete")))
      cab->setValueNotifyingHost(cab->convertTo0to1(static_cast<float>(item.look)));
    select({Kind::cab, rig});
    ensureCab(rig, true);
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
  // Pedal: a pedaleira inteira; cabeçote ou caixa: só a peça.
  mBoard->setVisible(s.kind == Kind::pedal);
  mPedalStage->setVisible(s.kind == Kind::pedal);
  mAmpStage->setVisible(s.kind == Kind::amp);
  mCabStage->setVisible(s.kind == Kind::cab);
  mEqStage->setVisible(s.kind == Kind::eq);
  mTunerStage->setVisible(s.kind == Kind::tuner);

  if (s.kind == Kind::pedal)
  {
    mShownPedalLook = pedalLook(s.index);
    mPedalStage->setTarget(s.index, pedalArt(s.index));
    mPedalStage->setArea(mBoard->imageOf(s.index));
    mBoard->tick();
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

namespace
{
// Os testes e as ferramentas de foto desligam a captura padrão (NEXUS_SEM_PADRAO=1):
// a tela deles não pode baixar nem trocar peças sozinha.
bool defaultsEnabled()
{
  return juce::SystemStats::getEnvironmentVariable("NEXUS_SEM_PADRAO", {}).isEmpty();
}
} // namespace

void NexusEditor::fetchDefault(const nexus::t3k::Tone& tone, const char* filter, const juce::String& gearName, int slotKey,
                               std::function<void(const juce::File&)> land, const juce::String& searchWord)
{
  const auto key = juce::String(tone.id) + "|" + tone.packName + "|" + searchWord;
  if (!defaultsEnabled() || !mT3k->hasKey() || !mT3k->signedIn() || mFetching.count(key) > 0)
    return;
  mFetching.insert(key);
  mFetchingSlots.insert(slotKey);
  auto safe = juce::Component::SafePointer<NexusEditor>(this);
  const juce::String gearFilter(filter);
  // Baixa um tom; se ele não serve sozinho (sem a posição do botão nos nomes),
  // passa para o próximo; no fim, a recomendada.
  auto tryList = std::make_shared<std::function<void(std::vector<nexus::t3k::Tone>, size_t)>>();
  *tryList = [safe, key, slotKey, land, gearFilter, gearName, tone, tryList](std::vector<nexus::t3k::Tone> list, size_t i) {
    if (safe == nullptr)
      return;
    if (i >= list.size())
    {
      safe->mFetching.erase(key);
      safe->mFetchingSlots.erase(slotKey);
      nexus::report::log("tone3000", juce::String::fromUTF8("captura padrão: nenhuma serviu"));
      return;
    }
    auto candidate = list[i];
    candidate.firstIfUnnamed = true;
    auto accept = [safe, key, slotKey, land](const juce::File& folder) {
      safe->mFetching.erase(key);
      safe->mFetchingSlots.erase(slotKey);
      land(folder);
    };
    if (const auto have = nexus::t3k::downloadedFolder(candidate, gearName); have.isDirectory())
      return accept(have); // downloadedFolder só devolve pasta que dá para tocar
    nexus::report::log("tone3000", juce::String::fromUTF8("captura padrão: ") + candidate.title + " " + candidate.packName + " ("
                                     + juce::String(candidate.favorites) + " favoritos)");
    safe->mT3k->download(candidate, gearFilter, gearName, [](juce::String) {},
                         [safe, list, i, accept, tryList](juce::File folder, juce::String error) {
                           if (safe == nullptr)
                             return;
                           const bool usable = error.isEmpty() && folder.isDirectory() && nexus::t3k::usableFolder(folder);
                           if (usable)
                             return accept(folder);
                           nexus::report::log("tone3000", juce::String::fromUTF8("captura padrão: não serviu ") + list[i].title + " " + error);
                           (*tryList)(list, i + 1);
                         });
  };
  if (searchWord.isEmpty())
    return (*tryList)({tone}, 0);
  // A mais curtida da comunidade primeiro (até três), depois a recomendada.
  mT3k->list(nexus::t3k::Connection::List::mostLiked, gearFilter == "pedal" ? juce::String("pedal") : gearFilter == "cab" ? juce::String("cab")
                                                                                                  : juce::String("amp_amp-cab"),
             searchWord, [tone, tryList](std::vector<nexus::t3k::Tone> found, juce::String) {
               std::vector<nexus::t3k::Tone> list(found.begin(), found.begin() + static_cast<long>(std::min<size_t>(3, found.size())));
               list.push_back(tone);
               (*tryList)(list, 0);
             });
}

void NexusEditor::ensurePedal(int pedal)
{
  const int look = pedalLook(pedal);
  const auto* art = pedalArt(pedal);
  if (!defaultsEnabled() || NexusProcessor::isNativeLook(look) || art == nullptr)
    return;
  // Já tem um pacote deste pedal (o visual pode ter mudado e o pacote não).
  const auto root = nexus::t3k::Connection::packsFolder(art->name);
  const auto current = mProcessor.slotFolder(NexusProcessor::pedalSlot(pedal));
  if (current.isDirectory() && current.getParentDirectory() == root && mProcessor.slotLoaded(NexusProcessor::pedalSlot(pedal)))
    return;
  const auto def = nexus::t3k::defaultTone(kPedalPrefixes[look]);
  juce::File pick = def ? nexus::t3k::downloadedFolder(*def, art->name) : juce::File();
  if (!pick.isDirectory())
  {
    auto folders = root.findChildFiles(juce::File::findDirectories, false);
    folders.sort();
    for (const auto& f : folders)
      if (nexus::t3k::usableFolder(f))
      {
        pick = f;
        break;
      }
  }
  if (pick.isDirectory())
  {
    mProcessor.loadPedalPack(pedal, pick);
    return;
  }
  if (def)
    fetchDefault(*def, "pedal", art->name, NexusProcessor::pedalSlot(pedal),
                 [this, pedal](const juce::File& folder) { mProcessor.loadPedalPack(pedal, folder); }, nexus::t3k::filterWord(kPedalPrefixes[look]));
}

void NexusEditor::ensureCab(int rig, bool swapping)
{
  // swapping: a caixa acabou de trocar (cabeçote ou caixa novos): o IR da
  // caixa nova entra mesmo havendo outro carregado.
  if (!defaultsEnabled() || mProcessor.packIncludesCab(rig)
      || (!swapping && (mProcessor.irLoaded(rig) || mProcessor.irName(rig).isNotEmpty())))
    return;
  // Caixa sem lista própria de IRs usa a da Brit 4x12 (os IRs servem a qualquer cabeçote).
  int look = juce::jlimit(0, NexusProcessor::kCabLooks - 1, mProcessor.cabLook(rig));
  auto def = nexus::t3k::defaultTone(kCabPrefixes[look]);
  if (!def)
  {
    look = 0;
    def = nexus::t3k::defaultTone(kCabPrefixes[0]);
  }
  if (!def)
    return;
  const auto gearName = mCabArts[static_cast<size_t>(look)].name;
  auto land = [this, rig](const juce::File& folder) {
    const auto preferred = juce::JSON::parse(folder.getChildFile("pack.json"))["ir_padrao"].toString();
    auto files = folder.findChildFiles(juce::File::findFiles, false, "*.wav;*.aif;*.aiff");
    files.sort();
    juce::File pick = files.isEmpty() ? juce::File() : files.getFirst();
    for (const auto& f : files)
      if (preferred.isNotEmpty() && f.getFileNameWithoutExtension() == preferred)
        pick = f;
    if (!pick.existsAsFile())
      return;
    mProcessor.loadIrAsync(rig, pick);
    if (auto* param = mProcessor.params.getParameter(NexusProcessor::ampParam(rig, "caixa")))
      param->setValueNotifyingHost(1.0f);
  };
  if (const auto folder = nexus::t3k::downloadedFolder(*def, gearName); folder.isDirectory())
    return land(folder);
  fetchDefault(*def, "cab", gearName, kCabSlotKey + rig, land, nexus::t3k::filterWord(kCabPrefixes[look]));
}

void NexusEditor::ensureDefaults()
{
  // Abertura do programa: tudo o que está na cadeia e vazio recebe a padrão.
  if (!defaultsEnabled())
    return;
  const auto layout = mProcessor.layout();
  for (const auto* list : {&layout.pre, &layout.loop[0], &layout.loop[1], &layout.post})
    for (int pedal : *list)
      ensurePedal(pedal);
  for (int rig = 0; rig < (layout.dual ? 2 : 1); ++rig)
  {
    bool anyEmpty = false;
    for (int c = 0; c < std::min(ampArt(rig).channels.size(), NexusProcessor::kChannels); ++c)
      anyEmpty = anyEmpty || !mProcessor.slotLoaded(NexusProcessor::ampSlot(rig, c));
    if (anyEmpty)
      loadHeadChannels(rig);
    ensureCab(rig);
  }
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
    // Pasta que não carregou (pack.json com defeito) não segura o canal.
    const bool ours = current.isDirectory() && current.getParentDirectory() == root
                      && mProcessor.slotLoaded(NexusProcessor::ampSlot(rig, c));
    juce::File pick = ours ? current : juce::File();
    const auto def = nexus::t3k::defaultTone(kAmpPrefixes[juce::jlimit(0, NexusProcessor::kAmpLooks - 1, mProcessor.ampLook(rig))], c);
    if (pick == juce::File() && def)
      pick = nexus::t3k::downloadedFolder(*def, art.name);
    if (pick == juce::File())
      for (const auto& f : folders)
        if ((packChannel(f) == c || (count == 1 && packChannel(f) < 0)) && nexus::t3k::usableFolder(f))
        {
          pick = f;
          break;
        }
    if (pick == juce::File())
    {
      mProcessor.clearAmpPack(rig, c);
      if (def)
        // Cabeçote de um canal: a mais curtida da TONE3000. Vários canais: a
        // recomendada de cada canal (a busca não sabe qual captura é de qual canal).
        fetchDefault(*def, "amp_amp-cab", art.name, NexusProcessor::ampSlot(rig, c),
                     [this, rig, c](const juce::File& folder) { mProcessor.loadAmpPack(rig, c, folder); },
                     count == 1 ? nexus::t3k::filterWord(kAmpPrefixes[juce::jlimit(0, NexusProcessor::kAmpLooks - 1, mProcessor.ampLook(rig))])
                                : juce::String());
    }
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
        + TRANS("Criado por Diego de Souza, desenvolvedor.") + "\n"
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
                                  TRANS(juce::String::fromUTF8("Conte o que aconteceu e o que você estava fazendo. O Nexus envia junto a versão, o sistema "
                                                               "e os últimos erros registrados, sem nenhum dado pessoal. Não precisa de conta.")),
                                  juce::MessageBoxIconType::NoIcon, this);
  w->addTextEditor("texto", {}, {});
  if (auto* editor = w->getTextEditor("texto"))
  {
    editor->setMultiLine(true, true);
    editor->setReturnKeyStartsNewLine(true);
    editor->setSize(460, 120);
  }
  w->addButton(TRANS(juce::String::fromUTF8("Enviar relatório")), 1, juce::KeyPress(juce::KeyPress::returnKey, juce::ModifierKeys::commandModifier, 0));
  w->addButton(TRANS("Cancelar"), 0, juce::KeyPress(juce::KeyPress::escapeKey));
  auto safe = juce::Component::SafePointer<NexusEditor>(this);
  w->enterModalState(true, juce::ModalCallbackFunction::create([w, safe](int result) {
    if (result == 1)
    {
      const auto text = w->getTextEditorContents("texto").trim();
      nexus::report::send(nexus::report::Kind::problem, text, nexus::report::recentLog(60),
                          [safe](nexus::report::Sent sent) { if (safe != nullptr) safe->showReportResult(sent); });
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
    juce::ModalCallbackFunction::create([crash, safe = juce::Component::SafePointer<NexusEditor>(this)](int ok) {
      if (ok != 0)
        nexus::report::send(nexus::report::Kind::crash, juce::String::fromUTF8("O Nexus fechou sozinho."),
                            crash + juce::String::fromUTF8("\n\nÚltimos erros registrados:\n") + nexus::report::recentLog(30),
                            [safe](nexus::report::Sent sent) { if (safe != nullptr) safe->showReportResult(sent); });
    }));
}

void NexusEditor::showReportResult(const nexus::report::Sent& sent)
{
  if (sent.openedBrowser)
    return; // o GitHub abriu no navegador: a pessoa termina lá
  juce::AlertWindow::showMessageBoxAsync(
    juce::MessageBoxIconType::InfoIcon, TRANS(juce::String::fromUTF8("Relatório enviado")),
    TRANS(juce::String::fromUTF8("Obrigado! O relatório virou o issue %1 no GitHub, e vamos tratar por lá.")).replace("%1", "#" + juce::String(sent.number)),
    "OK", this);
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
  menu.addItem(1, cc < 0 ? TRANS("Aprender MIDI") : TRANS("Trocar controle MIDI (agora %1)").replace("%1", MidiMap::describe(cc)));
  if (cc >= 0)
    menu.addItem(2, TRANS("Esquecer MIDI (%1)").replace("%1", MidiMap::describe(cc)));
  if (midi.learning())
    menu.addItem(4, TRANS("Cancelar aprender MIDI"));
  if (paramId == "canal")
    menu.addItem(3, TRANS(juce::String::fromUTF8("Troca de patch 1, 2, 3 já escolhe o canal")), false);
  menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(&target),
                     [this, paramId, name = param->getName(48)](int r) {
                       if (r == 1)
                       {
                         mProcessor.midi().learn(paramId);
                         nexus::report::log("midi", "aprendendo " + paramId);
                         mMidiNotice = TRANS("Mexa no controle da pedaleira para ligar a %1").replace("%1", name);
                         mMidiNoticeUntil = juce::Time::getMillisecondCounter() + 30000;
                         // Programa independente sem nenhum aparelho MIDI: a pedaleira
                         // Bluetooth (M-VAVE Chocolate e parecidas) precisa ser conectada
                         // antes; sem isso o aprender espera para sempre.
                         if (juce::JUCEApplicationBase::isStandaloneApp() && juce::MidiInput::getAvailableDevices().isEmpty())
                         {
                           const bool bluetooth = juce::BluetoothMidiDevicePairingDialogue::isAvailable();
                           const auto message = TRANS(juce::String::fromUTF8(
                             "O Nexus não está recebendo nenhum aparelho MIDI. Conecte a pedaleira pelo cabo USB ou, se ela for Bluetooth, conecte-a primeiro (o Bluetooth comum do sistema não basta para MIDI)."));
                           if (bluetooth)
                             juce::AlertWindow::showOkCancelBox(juce::MessageBoxIconType::InfoIcon, TRANS("Nenhuma pedaleira MIDI conectada"), message,
                                                                TRANS("Conectar por Bluetooth"), TRANS("Fechar"), nullptr,
                                                                juce::ModalCallbackFunction::create([](int ok) {
                                                                  if (ok == 1)
                                                                    juce::BluetoothMidiDevicePairingDialogue::open();
                                                                }));
                           else
                             juce::AlertWindow::showMessageBoxAsync(juce::MessageBoxIconType::InfoIcon, TRANS("Nenhuma pedaleira MIDI conectada"), message);
                         }
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
    if (mFetchingSlots.count(slot) > 0)
      return TRANS(juce::String::fromUTF8("Baixando a captura padrão da TONE3000..."));
    if (status.startsWith("Carregando"))
      return TRANS("Carregando...");
    if (status.startsWith("Erro"))
      return status;
    return slot < NexusProcessor::kPedals ? TRANS("Nenhuma captura no pedal.")
                                          : TRANS("Nenhum pacote de capturas carregado");
  }
  const int count = mProcessor.captureCount(slot);
  const auto number = [](float v) { return juce::String(v, 1).replaceCharacter('.', ','); };
  const bool lite = slot < NexusProcessor::kPedals
                      ? mProcessor.params.getParameter(NexusProcessor::pedalParam(slot, "lite"))->getValue() > 0.5f
                      : mProcessor.params.getParameter("lite")->getValue() > 0.5f;
  if (count == 1)
    return TRANS("%1: 1 captura, ganho %2 (%3)")
      .replace("%1", mProcessor.packName(slot))
      .replace("%2", number(mProcessor.capturePosition(0, slot)))
      .replace("%3", lite ? "A2 Lite" : "A2 Full");
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
    nexus::report::log("midi", "aprendeu " + MidiMap::describe(cc));
    mMidiNotice = TRANS("%1 ligado. Pronto para tocar.").replace("%1", MidiMap::describe(cc));
    mMidiNoticeUntil = juce::Time::getMillisecondCounter() + 2500;
    repaint();
  }
  if (mMidiNotice.isNotEmpty() && juce::Time::getMillisecondCounter() > mMidiNoticeUntil)
  {
    midi.cancelLearn();
    mMidiNotice.clear();
    repaint();
  }

  // Os mesmos pedais na pedaleira e na cadeia de baixo, na mesma ordem.
  if (const auto shown = mProcessor.layout().toValueTree().toXmlString(); shown != mShownLayout)
  {
    mShownLayout = shown;
    mChain->rebuild();
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
    title = art ? art->name : juce::String::fromUTF8(kPedalInfo[pedalLook(mSel.index)].name);
    if (NexusProcessor::isNativeLook(pedalLook(mSel.index)))
    {
      subtitle = TRANS(juce::String::fromUTF8("Efeito do Nexus: não usa captura."));
      if (const int look = pedalLook(mSel.index); NexusProcessor::hasModes(look))
        subtitle = TRANS("Modo") + ": "
                   + NexusProcessor::modeName(look, mProcessor.params.getRawParameterValue(NexusProcessor::pedalParam(mSel.index, "modo"))->load())
                   + ". " + TRANS(juce::String::fromUTF8("Efeito do Nexus: não usa captura."));
      link = {};
    }
    else
    {
      const bool loaded = mProcessor.slotLoaded(railSlot());
      subtitle = packSummary(railSlot());
      link = loaded ? TRANS("trocar pacote") : TRANS("abrir pacote do pedal");
    }
  }
  else if (mSel.kind == Kind::tuner)
  {
    title = TRANS("Afinador");
    subtitle = TRANS(juce::String::fromUTF8("Toque uma corda solta. Verde no meio: afinada."));
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
      subtitle = mProcessor.irLoaded(mSel.index)                         ? "IR: " + mProcessor.irName(mSel.index)
                 : mFetchingSlots.count(kCabSlotKey + mSel.index) > 0 ? TRANS(juce::String::fromUTF8("Baixando a captura padrão da TONE3000..."))
                                                                      : TRANS("Nenhum IR escolhido");
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
  {
    // A cadeia pode ter mudado (pedal posto, tirado ou arrastado).
    mPedalStage->setArea(mBoard->imageOf(mSel.index));
    mBoard->tick();
    mPedalStage->tick();
    mPedalStage->repaint();
  }
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
  // Ícone de atualização (só quando há versão nova), junto dos outros ícones.
  if (mUpdate.isVisible())
    mUpdate.setBounds(right.removeFromRight(44).withSizeKeepingCentre(36, 36));
  right.removeFromRight(12);
  mEngine.setBounds(right.removeFromRight(196));
  mCapturePanel->setBounds(getLocalBounds());
  mMapper->setBounds(getLocalBounds());
  // Equipamento | Estúdio ao lado do logo; os presets no meio do espaço que sobra.
  mMode.setBounds(232, (kTopBar - 36) / 2, 236, 36);
  {
    const int left = mMode.getRight() + 20;
    const int rightEdge = mEngine.getX() - 20;
    const int w = juce::jmin(380, rightEdge - left);
    mPresets->setBounds(left + (rightEdge - left - w) / 2, (kTopBar - 40) / 2, w, 40);
  }
  mStudio->setBounds(getLocalBounds().withTrimmedTop(kTopBar).withTrimmedBottom(kPathHeight));

  mBrowser->setBounds(r.removeFromRight(kBrowserWidth));
  mChain->setBounds(r.removeFromBottom(kPathHeight));
  mTitleArea = r.removeFromBottom(kTitleHeight);
  mBoard->setBounds(r);
  mPedalStage->setBounds(r);
  if (mSel.kind == Kind::pedal)
    mPedalStage->setArea(mBoard->imageOf(mSel.index));
  mAmpStage->setBounds(ampArt(mSel.kind == Kind::amp ? mSel.index : 0).channels.size() > 1 ? r.withTrimmedTop(46) : r);
  mCabStage->setBounds(r);
  mEqStage->setBounds(r);
  mTunerStage->setBounds(r);
  layoutTitle();
}
