#include "NovaInstalacao.h"

#include "../report/Report.h"

namespace nexus::update
{
namespace
{
constexpr const char* kReserveName = "Nexus Modeler (versões antigas)";
constexpr const char* kMarker = "instalacao.json";
// O que fica nos Documentos: a biblioteca do estúdio é do usuário (gravações
// e músicas), não de uma versão do programa.
constexpr const char* kKeepInDocuments = "Estúdio";

int generationOf(const juce::File& root)
{
  return static_cast<int>(juce::JSON::parse(root.getChildFile(kMarker))["geracao"]);
}

// Pasta nova dentro da reserva (uma por troca, pela data).
juce::File reserveFolder(const juce::File& besides)
{
  const auto reserve = besides.getSiblingFile(juce::String::fromUTF8(kReserveName));
  const auto stamp = juce::Time::getCurrentTime().formatted("%Y-%m-%d %H-%M-%S");
  return reserve.getChildFile(stamp).getNonexistentSibling(false);
}

bool moveAside(const juce::File& item, const juce::File& to)
{
  to.getParentDirectory().createDirectory();
  if (item.moveFileTo(to))
    return true;
  nexus::report::log("instalacao", juce::String::fromUTF8("não deu para mover ") + item.getFullPathName());
  return false;
}
} // namespace

juce::File dataRoot()
{
  if (const auto forced = juce::SystemStats::getEnvironmentVariable("NEXUS_DADOS_PASTA", {}); forced.isNotEmpty())
    return juce::File(forced);
  return juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
#if JUCE_MAC
    .getChildFile("Application Support")
#endif
    .getChildFile("Nexus Modeler");
}

juce::File documentsRoot()
{
  if (const auto forced = juce::SystemStats::getEnvironmentVariable("NEXUS_DOCUMENTOS_PASTA", {}); forced.isNotEmpty())
    return juce::File(forced);
  return juce::File::getSpecialLocation(juce::File::userDocumentsDirectory).getChildFile("Nexus Modeler");
}

juce::File settingsFile(const juce::String& name)
{
  return dataRoot().getChildFile(name);
}

namespace
{
// Pasta de dados que só tem o que o instalador acabou de gravar (o idioma
// escolhido nele): é de quem nunca teve o Nexus, não de versão antiga.
bool onlyInstallerFiles(const juce::File& data)
{
  for (const auto& f : data.findChildFiles(juce::File::findFilesAndDirectories, false))
    if (!f.getFileName().startsWithChar('.') && f.getFileName() != "preferencias.settings")
      return false;
  return true;
}
} // namespace

juce::File startFresh()
{
  const auto data = dataRoot();
  if (generationOf(data) >= kDataGeneration)
    return {};
  // Programa independente e plugins abertos juntos: um faz a troca, os outros esperam.
  juce::InterProcessLock lock("NexusModelerNovaInstalacao");
  if (!lock.enter(10000) || generationOf(data) >= kDataGeneration)
    return {};

  juce::File reserve;
  auto aside = [&reserve, &data](const juce::File& item, const juce::String& name) {
    if (!item.exists())
      return;
    if (reserve == juce::File())
      reserve = reserveFolder(data);
    moveAside(item, reserve.getChildFile(name));
  };
  // Pasta de dados inteira (preferências, Equipamentos, Presets, índice,
  // registro, conta da TONE3000) e as preferências do programa independente,
  // que ficam ao lado dela.
  if (!onlyInstallerFiles(data))
    aside(data, "Dados");
  for (const auto& f : data.getParentDirectory().findChildFiles(juce::File::findFiles, false, "Nexus Modeler.*"))
    aside(f, f.getFileName());
#if JUCE_LINUX
  // No Linux as versões antigas guardavam as preferências em ~/Nexus Modeler.
  if (juce::SystemStats::getEnvironmentVariable("NEXUS_DADOS_PASTA", {}).isEmpty())
    aside(juce::File::getSpecialLocation(juce::File::userHomeDirectory).getChildFile("Nexus Modeler"), "Preferencias");
#endif
  // Capturas da TONE3000 baixadas (Documentos/Nexus Modeler/<equipamento>).
  const auto docs = documentsRoot();
  for (const auto& f : docs.findChildFiles(juce::File::findFilesAndDirectories, false))
    if (f.getFileName() != juce::String::fromUTF8(kKeepInDocuments) && !f.getFileName().startsWithChar('.'))
      aside(f, juce::String("Documentos/") + f.getFileName());

  data.createDirectory();
  // Só o idioma da tela continua: é o que o instalador acabou de gravar (ou
  // a escolha da pessoa), não um dado de versão antiga.
  if (reserve != juce::File())
  {
    juce::PropertiesFile::Options o;
    for (const char* from : {"Dados", "Preferencias"})
    {
      juce::PropertiesFile old(reserve.getChildFile(from).getChildFile("preferencias.settings"), o);
      if (const auto language = old.getValue("idioma"); language.isNotEmpty())
      {
        juce::PropertiesFile fresh(data.getChildFile("preferencias.settings"), o);
        fresh.setValue("idioma", language);
        fresh.saveIfNeeded();
        break;
      }
    }
  }
  auto* marker = new juce::DynamicObject();
  marker->setProperty("geracao", kDataGeneration);
  if (reserve != juce::File())
    marker->setProperty("reserva", reserve.getFullPathName());
  data.getChildFile(kMarker).replaceWithText(juce::JSON::toString(juce::var(marker)));
  if (reserve != juce::File())
    nexus::report::log("instalacao", juce::String::fromUTF8("dados das versões antigas guardados em ") + reserve.getFullPathName());
  return reserve;
}
} // namespace nexus::update
