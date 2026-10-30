#include "Language.h"

#include <juce_data_structures/juce_data_structures.h>

#include "BinaryData.h"
#include "../update/NovaInstalacao.h"

namespace nexus::ui::language
{
namespace
{
juce::String gCurrent = "pt-BR";

juce::PropertiesFile& preferences()
{
  static juce::PropertiesFile file(nexus::update::settingsFile("preferencias.settings"), juce::PropertiesFile::Options());
  return file;
}
} // namespace

const std::vector<Language>& all()
{
  static const std::vector<Language> languages = {
    {"pt-BR", "Português (Brasil)"},
    {"en", "English"},
    {"es", "Español"},
    {"fr", "Français"},
    {"it", "Italiano"},
    {"de", "Deutsch"},
    {"ja", "日本語"},
    {"zh", "中文（简体）"},
    {"ko", "한국어"},
    {"hi", "हिन्दी"},
    {"ar", "العربية"},
    {"bn", "বাংলা"},
    {"ru", "Русский"},
    {"id", "Bahasa Indonesia"},
    {"tr", "Türkçe"},
  };
  return languages;
}

juce::String saved()
{
  return preferences().getValue("idioma");
}

juce::String fromSystem()
{
  const auto lang = juce::SystemStats::getUserLanguage().toLowerCase(); // "pt", "en", "zh"...
  if (lang.startsWith("pt"))
    return "pt-BR";
  for (const auto& l : all())
    if (lang.startsWith(juce::String(l.code)))
      return l.code;
  return "en";
}

void apply(const juce::String& code, bool save)
{
  gCurrent = code;
  if (save)
  {
    preferences().setValue("idioma", code);
    preferences().saveIfNeeded();
  }
  if (code == "pt-BR")
  {
    juce::LocalisedStrings::setCurrentMappings(nullptr);
    return;
  }
  int size = 0;
  const auto resource = "idioma_" + code.replaceCharacter('-', '_') + "_txt";
  if (const char* data = BinaryData::getNamedResource(resource.toRawUTF8(), size))
    juce::LocalisedStrings::setCurrentMappings(new juce::LocalisedStrings(juce::String::fromUTF8(data, size), false));
  else
    juce::LocalisedStrings::setCurrentMappings(nullptr);
}

juce::String current()
{
  return gCurrent;
}

void applyStartup()
{
  // Uma vez por programa: várias janelas do plugin compartilham o idioma.
  static bool done = false;
  if (done)
    return;
  done = true;
  const auto code = saved();
  apply(code.isNotEmpty() ? code : fromSystem(), code.isEmpty());
}

bool allowsLetterSpacing()
{
  for (const char* code : {"ar", "hi", "bn", "ja", "zh", "ko"})
    if (gCurrent == code)
      return false;
  return true;
}

bool rightToLeft()
{
  return gCurrent == "ar";
}
} // namespace nexus::ui::language
