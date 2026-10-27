#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include "BinaryData.h"

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

// Texto em maiúsculas com espaço entre letras, como os rótulos de estúdio.
inline void drawCaps(juce::Graphics& g, const juce::String& s, juce::Rectangle<float> r, float size,
                     juce::Justification j, juce::Colour c)
{
  g.setColour(c);
  g.setFont(Fonts::get().ui(size).withExtraKerningFactor(0.08f));
  g.drawText(s.toUpperCase(), r, j, false);
}
} // namespace nexus::ui
