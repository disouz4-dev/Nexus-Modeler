#include "Gear.h"

#include "BinaryData.h"

namespace nexus::ui
{
namespace
{
juce::Image imageResource(const juce::String& name)
{
  int size = 0;
  const auto resource = name.replaceCharacter('.', '_');
  if (const char* data = BinaryData::getNamedResource(resource.toRawUTF8(), size))
    return juce::ImageCache::getFromMemory(data, size);
  return {};
}

juce::var jsonResource(const juce::String& name)
{
  int size = 0;
  const auto resource = name.replaceCharacter('.', '_');
  if (const char* data = BinaryData::getNamedResource(resource.toRawUTF8(), size))
    return juce::JSON::parse(juce::String::fromUTF8(data, size));
  return {};
}
// Frente: JPEG da cor (já multiplicada pela transparência, fundo preto) e a
// máscara de transparência em PNG cinza (tools/gear_studio/frente_jpeg.py).
// O JUCE guarda a cor assim mesmo, multiplicada: é só juntar os canais.
// Sem o par, vale o PNG antigo com transparência.
juce::Image frontImage(const juce::String& prefix)
{
  const auto colour = imageResource(prefix + "_frente.jpg");
  const auto alpha = imageResource(prefix + "_frente_alfa.png");
  if (!colour.isValid() || !alpha.isValid() || colour.getBounds() != alpha.getBounds())
    return imageResource(prefix + "_frente.png");
  juce::Image out(juce::Image::ARGB, colour.getWidth(), colour.getHeight(), false);
  const juce::Image::BitmapData c(colour, juce::Image::BitmapData::readOnly);
  const juce::Image::BitmapData a(alpha, juce::Image::BitmapData::readOnly);
  juce::Image::BitmapData o(out, juce::Image::BitmapData::writeOnly);
  // Posição dos bytes de cada canal dentro do ponto, conforme o formato.
  const int cr = c.pixelFormat == juce::Image::RGB ? juce::PixelRGB::indexR : juce::PixelARGB::indexR;
  const int cg = c.pixelFormat == juce::Image::RGB ? juce::PixelRGB::indexG : juce::PixelARGB::indexG;
  const int cb = c.pixelFormat == juce::Image::RGB ? juce::PixelRGB::indexB : juce::PixelARGB::indexB;
  // A máscara cinza chega como canal único ou com o cinza no vermelho.
  const int ak = a.pixelFormat == juce::Image::SingleChannel ? 0
                 : a.pixelFormat == juce::Image::RGB         ? juce::PixelRGB::indexR
                                                             : juce::PixelARGB::indexR;
  for (int y = 0; y < out.getHeight(); ++y)
  {
    const juce::uint8* cl = c.getLinePointer(y);
    const juce::uint8* al = a.getLinePointer(y);
    auto* ol = reinterpret_cast<juce::PixelARGB*>(o.getLinePointer(y));
    for (int x = 0; x < out.getWidth(); ++x, cl += c.pixelStride, al += a.pixelStride)
    {
      const juce::uint8 k = al[ak];
      // Cor já multiplicada não passa do alfa (arredondamento do JPEG).
      ol[x].setARGB(k, std::min(cl[cr], k), std::min(cl[cg], k), std::min(cl[cb], k));
    }
  }
  return out;
}
} // namespace

juce::Image GearKnobArt::loadStrip() const
{
  return imageResource(stripResource);
}

GearArt GearArt::load(const juce::String& prefix)
{
  GearArt art;
  art.front = frontImage(prefix);
  if (art.front.isValid())
  {
    // Contorno da peça: colunas e linhas com algum ponto opaco (de 2 em 2).
    const juce::Image::BitmapData px(art.front, juce::Image::BitmapData::readOnly);
    int x0 = art.front.getWidth(), x1 = 0, y0 = art.front.getHeight(), y1 = 0;
    const int alphaIndex = px.pixelFormat == juce::Image::ARGB ? juce::PixelARGB::indexA : -1;
    for (int y = 0; y < art.front.getHeight(); y += 2)
    {
      const juce::uint8* line = px.getLinePointer(y);
      for (int x = 0; x < art.front.getWidth(); x += 2)
        if (alphaIndex < 0 || line[x * px.pixelStride + alphaIndex] > 160)
        {
          x0 = std::min(x0, x);
          x1 = std::max(x1, x);
          y0 = std::min(y0, y);
          y1 = std::max(y1, y);
        }
    }
    art.body = x1 > x0 ? juce::Rectangle<float>(static_cast<float>(x0), static_cast<float>(y0), static_cast<float>(x1 - x0), static_cast<float>(y1 - y0))
                       : art.front.getBounds().toFloat();
  }
  const auto map = jsonResource(prefix + "_mapa.json");
  art.name = map["nome"].toString();
  if (const auto* mm = map["medidas_mm"].getArray(); mm != nullptr && mm->size() == 2)
    art.sizeMm = {static_cast<float>((*mm)[0]), static_cast<float>((*mm)[1])};
  const int frames = static_cast<int>(map["posicoes"]);
  if (const auto* knobs = map["botoes"].getArray())
    for (const auto& k : *knobs)
    {
      GearKnobArt knob;
      knob.id = k["id"].toString();
      knob.size = static_cast<int>(k["tamanho"]);
      knob.box = {static_cast<int>(k["x"]), static_cast<int>(k["y"]), knob.size, knob.size};
      if (k["tipo"].toString() == "deslizante")
      {
        knob.slider = true;
        knob.width = static_cast<int>(k["largura"]);
        knob.height = static_cast<int>(k["altura"]);
        knob.box.setSize(knob.width, knob.height);
      }
      knob.stripResource = prefix + "_botao_" + knob.id + ".png";
      // Botão em degraus (drive) pode ter menos quadros que os outros.
      knob.frames = k.hasProperty("posicoes") ? static_cast<int>(k["posicoes"]) : frames;
      int bytes = 0;
      if (BinaryData::getNamedResource(knob.stripResource.replaceCharacter('.', '_').toRawUTF8(), bytes) != nullptr)
        art.knobs.push_back(std::move(knob));
    }
  if (const auto* labels = map["rotulos"].getArray())
    for (const auto& l : *labels)
    {
      GearLabel label;
      label.text = l["texto"].toString();
      label.centre = {static_cast<float>(l["x"]), static_cast<float>(l["y"])};
      label.height = static_cast<float>(l["altura"]);
      label.squash = static_cast<float>(l["achatamento"]);
      label.spacing = static_cast<float>(l.getProperty("espaco", 1.0));
      label.colour = juce::Colour::fromString("ff" + l["cor"].toString().fromFirstOccurrenceOf("#", false, false));
      art.labels.push_back(label);
    }
  if (const auto* leds = map["leds_canais"].getArray())
    for (const auto& l : *leds)
      art.channelLeds.push_back({{static_cast<float>(l["x"]), static_cast<float>(l["y"])}, static_cast<float>(l["raio"])});
  if (const auto* channels = map["canais"].getArray())
    for (const auto& c : *channels)
      art.channels.add(c.toString());
  if (const auto foot = map["pisante"]; foot.isObject())
  {
    art.footswitch = {static_cast<float>(foot["x"]), static_cast<float>(foot["y"])};
    art.footswitchRadius = static_cast<float>(foot["raio"]);
  }
  if (const auto led = map["led"]; led.isObject())
  {
    art.led = {static_cast<float>(led["x"]), static_cast<float>(led["y"])};
    art.ledRadius = static_cast<float>(led["raio"]);
  }
  if (const auto screen = map["visor"]; screen.isObject())
    art.display = {static_cast<float>(screen["x"]), static_cast<float>(screen["y"]), static_cast<float>(screen["w"]),
                   static_cast<float>(screen["h"])};
  return art;
}
} // namespace nexus::ui
