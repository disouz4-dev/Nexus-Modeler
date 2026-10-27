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
      knob.strip = imageResource(prefix + "_botao_" + knob.id + ".png");
      knob.frames = frames;
      if (knob.strip.isValid())
        art.knobs.push_back(std::move(knob));
    }
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
  return art;
}
} // namespace nexus::ui
