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

// Triângulo impossível. Pontos em coordenadas baricêntricas (a, b, c), com
// a + b + c = n; cada viga tem duas faces de largura 1: a de fora (azul) e a
// de dentro (aço). Em cada canto uma viga passa na frente da outra, em ciclo,
// e é isso que cria a ilusão.
inline void drawPenrose(juce::Graphics& g, juce::Rectangle<float> area)
{
  const float side = std::min(area.getWidth(), area.getHeight() / 0.8660254f);
  const float h = side * 0.8660254f;
  const juce::Point<float> A(area.getCentreX(), area.getCentreY() - h * 0.5f);
  const juce::Point<float> B(area.getCentreX() - side * 0.5f, area.getCentreY() + h * 0.5f);
  const juce::Point<float> C(area.getCentreX() + side * 0.5f, area.getCentreY() + h * 0.5f);
  constexpr float n = 9.0f;
  auto P = [&](float a, float b, float c) {
    return juce::Point<float>((a * A.x + b * B.x + c * C.x) / n, (a * A.y + b * B.y + c * C.y) / n);
  };
  auto poly = [](std::initializer_list<juce::Point<float>> pts) {
    juce::Path p;
    bool first = true;
    for (auto pt : pts)
    {
      if (first)
        p.startNewSubPath(pt);
      else
        p.lineTo(pt);
      first = false;
    }
    p.closeSubPath();
    return p;
  };

  // Faces de cada viga: [0] fora, [1] dentro.
  // Viga de baixo: a de 0 a 2. Esquerda: c de 0 a 2. Direita: b de 0 a 2.
  const juce::Path bottom[2] = {poly({P(0, n, 0), P(0, 0, n), P(1, 0, n - 1), P(1, n - 1, 0)}),
                                poly({P(1, n - 1, 0), P(1, 0, n - 1), P(2, 0, n - 2), P(2, n - 2, 0)})};
  const juce::Path left[2] = {poly({P(0, n, 0), P(n, 0, 0), P(n - 1, 0, 1), P(0, n - 1, 1)}),
                              poly({P(0, n - 1, 1), P(n - 1, 0, 1), P(n - 2, 0, 2), P(0, n - 2, 2)})};
  const juce::Path right[2] = {poly({P(n, 0, 0), P(0, 0, n), P(0, 1, n - 1), P(n - 1, 1, 0)}),
                               poly({P(n - 1, 1, 0), P(0, 1, n - 1), P(0, 2, n - 2), P(n - 2, 2, 0)})};

  auto paintBeam = [&](const juce::Path* beam, juce::Point<float> from, juce::Point<float> to) {
    g.setGradientFill(juce::ColourGradient(blueLight, from.x, from.y, blueDeep, to.x, to.y, false));
    g.fillPath(beam[0]);
    g.setGradientFill(juce::ColourGradient(steelLight, from.x, from.y, steelDark, to.x, to.y, false));
    g.fillPath(beam[1]);
  };
  paintBeam(bottom, C, B);
  paintBeam(left, B, A);
  paintBeam(right, A, C);

  // Cantos: quem passa na frente (baixo sobre esquerda em B, esquerda sobre
  // direita em A, direita sobre baixo em C).
  struct Corner
  {
    juce::Path region;
    const juce::Path* beam;
    juce::Point<float> from, to;
  };
  const Corner corners[3] = {
    {poly({P(0, n, 0), P(2, n - 2, 0), P(2, n - 4, 2), P(0, n - 2, 2)}), bottom, C, B},
    {poly({P(n, 0, 0), P(n - 2, 0, 2), P(n - 4, 2, 2), P(n - 2, 2, 0)}), left, B, A},
    {poly({P(0, 0, n), P(0, 2, n - 2), P(2, 2, n - 4), P(2, 0, n - 2)}), right, A, C},
  };
  for (const auto& c : corners)
  {
    juce::Graphics::ScopedSaveState save(g);
    g.reduceClipRegion(c.region);
    paintBeam(c.beam, c.from, c.to);
  }

  // Brilho fino na borda de fora.
  juce::Path outline = poly({A, B, C});
  g.setColour(cyan.withAlpha(0.55f));
  g.strokePath(outline, juce::PathStrokeType(std::max(0.8f, side * 0.012f)));
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
