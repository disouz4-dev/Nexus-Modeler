#include "Gear.h"

#include "BinaryData.h"
#include "../gear/GearCatalog.h"

#include <mutex>

namespace nexus::ui
{
namespace
{
// Arquivo da peça: o da pasta dela (equipamento baixado ou arte refeita, em
// Equipamentos/<id>/) vale antes do que veio dentro do programa.
juce::File onDisk(const juce::File& dir, const juce::String& name)
{
  return dir == juce::File() ? juce::File() : dir.getChildFile(name);
}

juce::Image imageResource(const juce::File& dir, const juce::String& name)
{
  if (const auto file = onDisk(dir, name); file.existsAsFile())
    return juce::ImageCache::getFromFile(file);
  int size = 0;
  const auto resource = name.replaceCharacter('.', '_');
  if (const char* data = BinaryData::getNamedResource(resource.toRawUTF8(), size))
    return juce::ImageCache::getFromMemory(data, size);
  return {};
}

juce::var jsonResource(const juce::File& dir, const juce::String& name)
{
  if (const auto file = onDisk(dir, name); file.existsAsFile())
    return juce::JSON::parse(file);
  int size = 0;
  const auto resource = name.replaceCharacter('.', '_');
  if (const char* data = BinaryData::getNamedResource(resource.toRawUTF8(), size))
    return juce::JSON::parse(juce::String::fromUTF8(data, size));
  return {};
}

bool hasResource(const juce::File& dir, const juce::String& name)
{
  int size = 0;
  return onDisk(dir, name).existsAsFile()
         || BinaryData::getNamedResource(name.replaceCharacter('.', '_').toRawUTF8(), size) != nullptr;
}
// Frente: JPEG da cor (já multiplicada pela transparência, fundo preto) e a
// máscara de transparência em PNG cinza (tools/gear_studio/frente_jpeg.py).
// O JUCE guarda a cor assim mesmo, multiplicada: é só juntar os canais.
// Sem o par, vale o PNG antigo com transparência.
juce::Image frontImage(const juce::File& dir, const juce::String& prefix)
{
  const auto colour = imageResource(dir, prefix + "_frente.jpg");
  const auto alpha = imageResource(dir, prefix + "_frente_alfa.png");
  if (!colour.isValid() || !alpha.isValid() || colour.getBounds() != alpha.getBounds())
    return imageResource(dir, prefix + "_frente.png");
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
  return imageResource(folder, stripResource);
}

GearArt GearArt::load(const juce::String& prefix)
{
  // O catálogo abre antes da primeira imagem: passa a arte da pasta antiga
  // (Arte) para Equipamentos e limpa o que o programa atualizado já traz.
  static std::once_flag prepared;
  std::call_once(prepared, [] { nexus::gear::GearCatalog::instance(); });
  return loadFolder(nexus::gear::GearCatalog::folder().getChildFile(prefix), prefix);
}

GearArt GearArt::loadFolder(const juce::File& folder, const juce::String& prefix)
{
  GearArt art;
  art.front = frontImage(folder, prefix);
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
  const auto map = jsonResource(folder, prefix + "_mapa.json");
  art.name = map["nome"].toString();
  if (const auto* mm = map["medidas_mm"].getArray(); mm != nullptr && mm->size() == 2)
    art.sizeMm = {static_cast<float>((*mm)[0]), static_cast<float>((*mm)[1])};
  if (const auto* tip = map["ponta"].getArray(); tip != nullptr && tip->size() == 2)
    art.tip = {static_cast<float>((*tip)[0]), static_cast<float>((*tip)[1])};
  art.pixelsPerMm = static_cast<float>(map["pixels_por_mm"]);
  if (const auto speakers = map["falantes"]; speakers.isObject())
  {
    art.speakerRows = static_cast<int>(speakers["linhas"]);
    art.speakerCols = static_cast<int>(speakers["colunas"]);
    art.speakerInches = static_cast<float>(speakers["polegadas"]);
  }
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
      knob.folder = folder;
      // Botão em degraus (drive) pode ter menos quadros que os outros.
      knob.frames = k.hasProperty("posicoes") ? static_cast<int>(k["posicoes"]) : frames;
      if (hasResource(folder, knob.stripResource))
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
