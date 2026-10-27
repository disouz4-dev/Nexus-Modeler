#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include "Theme.h"

// Marca do Nexus Modeler desenhada em vetor (nítida em qualquer tamanho):
// o triângulo impossível azul e o nome "NEXUS" com o E sem a haste e o X em
// ciano, seguido de "MODELER" espaçado entre dois traços.
namespace nexus::ui::brand
{
const juce::Colour cyan{0xff29b6ff};
const juce::Colour blueLight{0xff5fd0ff};
const juce::Colour blueDeep{0xff0f5fd6};
const juce::Colour steelLight{0xff8fa3b8};
const juce::Colour steelDark{0xff3a4856};

// Triângulo impossível: o desenho vem de assets/marca/nexus_marca.svg, gerado
// por tools/marca/logo.py (o mesmo arquivo do ícone e do site). Vetor: nítido
// em qualquer tamanho.
inline void drawPenrose(juce::Graphics& g, juce::Rectangle<float> area)
{
  static const std::unique_ptr<juce::Drawable> mark =
    juce::Drawable::createFromImageData(BinaryData::nexus_marca_svg, static_cast<size_t>(BinaryData::nexus_marca_svgSize));
  if (mark != nullptr)
    mark->drawWithin(g, area, juce::RectanglePlacement::centred, 1.0f);
}

// "NEXUS" desenhado letra por letra. Devolve a largura usada.
inline float drawWordmark(juce::Graphics& g, juce::Point<float> topLeft, float h)
{
  const float t = h * 0.20f;  // espessura do traço
  const float w = h * 1.22f;  // largura de cada letra
  const float gap = h * 0.30f;
  const float r = h * 0.34f;  // raio das curvas do U e do S
  float x = topLeft.x;
  const float y = topLeft.y;
  juce::Path letters;
  juce::Path xMark;

  // N
  letters.addRectangle(x, y, t, h);
  letters.addRectangle(x + w - t, y, t, h);
  {
    juce::Path d;
    d.startNewSubPath(x, y);
    d.lineTo(x + t * 1.35f, y);
    d.lineTo(x + w, y + h);
    d.lineTo(x + w - t * 1.35f, y + h);
    d.closeSubPath();
    letters.addPath(d);
  }
  x += w + gap;
  // E sem haste: três barras.
  letters.addRectangle(x, y, w, t);
  letters.addRectangle(x, y + (h - t) * 0.5f, w * 0.92f, t);
  letters.addRectangle(x, y + h - t, w, t);
  x += w + gap;
  // X (em ciano)
  {
    const float k = t * 0.75f;
    juce::Path d1, d2;
    d1.startNewSubPath(x, y);
    d1.lineTo(x + k * 1.6f, y);
    d1.lineTo(x + w, y + h);
    d1.lineTo(x + w - k * 1.6f, y + h);
    d1.closeSubPath();
    d2.startNewSubPath(x + w, y);
    d2.lineTo(x + w - k * 1.6f, y);
    d2.lineTo(x, y + h);
    d2.lineTo(x + k * 1.6f, y + h);
    d2.closeSubPath();
    xMark.addPath(d1);
    xMark.addPath(d2);
  }
  x += w + gap;
  // U: contorno único com as duas curvas de baixo.
  juce::Path u;
  {
    const float ri = std::max(r - t, 0.5f);
    u.startNewSubPath(x, y);
    u.lineTo(x, y + h - r);
    u.quadraticTo(x, y + h, x + r, y + h);
    u.lineTo(x + w - r, y + h);
    u.quadraticTo(x + w, y + h, x + w, y + h - r);
    u.lineTo(x + w, y);
    u.lineTo(x + w - t, y);
    u.lineTo(x + w - t, y + h - t - ri);
    u.quadraticTo(x + w - t, y + h - t, x + w - t - ri, y + h - t);
    u.lineTo(x + t + ri, y + h - t);
    u.quadraticTo(x + t, y + h - t, x + t, y + h - t - ri);
    u.lineTo(x + t, y);
    u.closeSubPath();
  }
  x += w + gap;
  // S: contorno único; barra de cima, haste esquerda, barra do meio, haste
  // direita e barra de baixo, com cantos arredondados.
  juce::Path sPath;
  {
    const float m = (h - t) * 0.5f;
    const float ro = r * 0.8f;
    const float ri = std::max(ro - t, 0.5f);
    auto X = [&](float v) { return x + v; };
    auto Y = [&](float v) { return y + v; };
    sPath.startNewSubPath(X(w), Y(0));
    sPath.lineTo(X(w), Y(t));
    sPath.lineTo(X(t + ri), Y(t));
    sPath.quadraticTo(X(t), Y(t), X(t), Y(t + ri));
    sPath.lineTo(X(t), Y(m - ri));
    sPath.quadraticTo(X(t), Y(m), X(t + ri), Y(m));
    sPath.lineTo(X(w - ro), Y(m));
    sPath.quadraticTo(X(w), Y(m), X(w), Y(m + ro));
    sPath.lineTo(X(w), Y(h - ro));
    sPath.quadraticTo(X(w), Y(h), X(w - ro), Y(h));
    sPath.lineTo(X(0), Y(h));
    sPath.lineTo(X(0), Y(h - t));
    sPath.lineTo(X(w - t - ri), Y(h - t));
    sPath.quadraticTo(X(w - t), Y(h - t), X(w - t), Y(h - t - ri));
    sPath.lineTo(X(w - t), Y(m + t + ri));
    sPath.quadraticTo(X(w - t), Y(m + t), X(w - t - ri), Y(m + t));
    sPath.lineTo(X(ro), Y(m + t));
    sPath.quadraticTo(X(0), Y(m + t), X(0), Y(m + t - ro));
    sPath.lineTo(X(0), Y(ro));
    sPath.quadraticTo(X(0), Y(0), X(ro), Y(0));
    sPath.closeSubPath();
  }
  x += w;

  const juce::ColourGradient metal(juce::Colour(0xfff4f8fc), 0.0f, y, juce::Colour(0xff93a8bf), 0.0f, y + h, false);
  g.setGradientFill(metal);
  g.fillPath(letters);
  g.fillPath(u);
  g.fillPath(sPath);
  g.setGradientFill(juce::ColourGradient(blueLight, 0.0f, y, blueDeep, 0.0f, y + h, false));
  g.fillPath(xMark);
  return x - topLeft.x;
}

// "MODELER" espaçado, em ciano, entre dois traços, centrado em "area".
inline void drawModeler(juce::Graphics& g, juce::Rectangle<float> area, float size, bool withLines)
{
  const auto font = Fonts::get().ui(size).withExtraKerningFactor(0.55f);
  const float tw = static_cast<float>(juce::GlyphArrangement::getStringWidthInt(font, "MODELER"));
  g.setColour(cyan);
  g.setFont(font);
  g.drawText("MODELER", area, juce::Justification::centred, false);
  if (withLines)
  {
    const float cy = area.getCentreY();
    const float lineW = (area.getWidth() - tw) * 0.5f - size * 1.2f;
    if (lineW > 4.0f)
    {
      g.fillRect(area.getX(), cy - 0.5f, lineW, 1.2f);
      g.fillRect(area.getRight() - lineW, cy - 0.5f, lineW, 1.2f);
    }
  }
}
} // namespace nexus::ui::brand
