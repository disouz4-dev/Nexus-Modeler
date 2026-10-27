#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include "BinaryData.h"
#include "Language.h"

// Cores e fontes da tela. Fundo quase preto e neutro, como um estúdio
// escuro, para os equipamentos renderizados aparecerem.
namespace nexus::ui
{
namespace colour
{
const juce::Colour background{0xff141414};
const juce::Colour sidebar{0xff0e0e0e};
const juce::Colour line{0xff2b2b2b};
const juce::Colour box{0xff181818};
const juce::Colour boxBorder{0xff3a3a3a};
const juce::Colour boxHover{0xff222222};
const juce::Colour text{0xffeceae6};
const juce::Colour dim{0xff8e8e8e};
const juce::Colour faint{0xff5a5a5a};
const juce::Colour accent{0xff29b6ff}; // ciano da marca
const juce::Colour selected{0xfff4f4f4};
const juce::Colour danger{0xffe0563f};
} // namespace colour

class Fonts
{
public:
  static Fonts& get()
  {
    static Fonts f;
    return f;
  }
  juce::Font ui(float size) const { return juce::Font(juce::FontOptions(mInter).withHeight(size)); }
  juce::Font display(float size) const { return juce::Font(juce::FontOptions(mOswald).withHeight(size)); }
  // Mesma medida de letra do Blender (tamanho do "em"), para os rótulos do painel.
  juce::Font displayEm(float em) const { return juce::Font(juce::FontOptions(mOswald).withPointHeight(em)); }

private:
  Fonts()
  {
    mInter = juce::Typeface::createSystemTypefaceFor(BinaryData::inter_ttf,
                                                     static_cast<size_t>(BinaryData::inter_ttfSize));
    mOswald = juce::Typeface::createSystemTypefaceFor(BinaryData::oswald_ttf,
                                                      static_cast<size_t>(BinaryData::oswald_ttfSize));
  }
  juce::Typeface::Ptr mInter, mOswald;
};

// Maiúsculas que respeitam os acentos do português (á -> Á, ç -> Ç). A
// função do JUCE depende do idioma do sistema e deixa os acentuados em
// minúscula.
inline juce::String upper(const juce::String& s)
{
  juce::String out;
  out.preallocateBytes(s.getNumBytesAsUTF8());
  for (auto p = s.getCharPointer(); !p.isEmpty();)
  {
    auto c = p.getAndAdvance();
    if (c >= 0xE0 && c <= 0xFE && c != 0xF7)
      c -= 0x20;
    else
      c = juce::CharacterFunctions::toUpperCase(c);
    out += juce::String::charToString(c);
  }
  return out;
}

// Texto em maiúsculas com espaço entre letras, como os rótulos de estúdio.
inline void drawCaps(juce::Graphics& g, const juce::String& s, juce::Rectangle<float> r, float size,
                     juce::Justification j, juce::Colour c)
{
  g.setColour(c);
  g.setFont(Fonts::get().ui(size).withExtraKerningFactor(language::allowsLetterSpacing() ? 0.08f : 0.0f));
  g.drawText(upper(s), r, j, false);
}
} // namespace nexus::ui
