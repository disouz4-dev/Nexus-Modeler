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
} // namespace

GearArt GearArt::load(const juce::String& prefix)
{
  GearArt art;
  art.front = imageResource(prefix + "_frente.png");
  const auto map = jsonResource(prefix + "_mapa.json");
  art.name = map["nome"].toString();
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
      knob.strip = imageResource(prefix + "_botao_" + knob.id + ".png");
      // Botão em degraus (drive) pode ter menos quadros que os outros.
      knob.frames = k.hasProperty("posicoes") ? static_cast<int>(k["posicoes"]) : frames;
      if (knob.strip.isValid())
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
