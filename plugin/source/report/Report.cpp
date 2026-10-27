#include "Report.h"

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include "../update/Updater.h"

namespace nexus::report
{
namespace
{
const juce::String kRepo = "https://github.com/disouz4-dev/Nexus-Modeler";
constexpr juce::int64 kMaxLogBytes = 512 * 1024;
// O GitHub recusa endereços muito longos; o resto vai pela área de transferência.
constexpr int kMaxUrlLength = 7000;

juce::CriticalSection& logLock()
{
  static juce::CriticalSection lock;
  return lock;
}

// Tira o nome do usuário dos caminhos.
juce::String anonymize(juce::String text)
{
  const auto home = juce::File::getSpecialLocation(juce::File::userHomeDirectory).getFullPathName();
  if (home.length() > 1)
    text = text.replace(home, "~");
  return text;
}

juce::File crashFile()
{
  return folder().getChildFile("travamento.txt");
}

// Preparado na instalação: o tratador de travamento faz o mínimo possível.
juce::String& crashHeader()
{
  static juce::String header;
  return header;
}

void onCrash(void*)
{
  const auto text = crashHeader() + "\nPilha de chamadas:\n" + anonymize(juce::SystemStats::getStackBacktrace());
  crashFile().replaceWithText(text);
}
} // namespace

juce::File folder()
{
  juce::PropertiesFile::Options o;
  o.applicationName = "registro";
  o.folderName = "Nexus Modeler";
  o.filenameSuffix = ".txt";
  o.osxLibrarySubFolder = "Application Support";
  return o.getDefaultFile().getParentDirectory();
}

juce::File logFile()
{
  return folder().getChildFile("registro.txt");
}

void log(const juce::String& where, const juce::String& what)
{
  const juce::ScopedLock lock(logLock());
  auto file = logFile();
  file.getParentDirectory().createDirectory();
  // Passou do limite: guarda a metade mais nova.
  if (file.getSize() > kMaxLogBytes)
  {
    const auto text = file.loadFileAsString();
    file.replaceWithText(text.substring(text.length() / 2).fromFirstOccurrenceOf("\n", false, false));
  }
  const auto line = juce::Time::getCurrentTime().formatted("%Y-%m-%d %H:%M:%S") + "  [" + where + "] "
                    + anonymize(what).replace("\n", " ") + "\n";
  file.appendText(line, false, false, "\n");
}

juce::String recentLog(int maxLines)
{
  const juce::ScopedLock lock(logLock());
  juce::StringArray lines;
  lines.addLines(logFile().loadFileAsString());
  lines.removeEmptyStrings();
  if (lines.size() > maxLines)
    lines.removeRange(0, lines.size() - maxLines);
  return lines.joinIntoString("\n");
}

juce::String systemInfo()
{
  juce::StringArray s;
  s.add("Nexus Modeler " + nexus::update::currentVersion());
  s.add(juce::SystemStats::getOperatingSystemName() + (juce::SystemStats::isOperatingSystem64Bit() ? " (64 bits)" : ""));
  s.add(juce::SystemStats::getCpuModel().trim() + ", " + juce::String(juce::SystemStats::getNumCpus()) + " núcleos, "
        + juce::String(juce::SystemStats::getMemorySizeInMegabytes() / 1024) + " GB");
  const juce::PluginHostType host;
  s.add(juce::JUCEApplicationBase::isStandaloneApp() ? juce::String::fromUTF8("Programa independente")
                                                     : "Plugin em " + juce::String(host.getHostDescription()));
  return s.joinIntoString("\n");
}

void installCrashHandler()
{
  crashHeader() = systemInfo() + "\nQuando: " + juce::Time::getCurrentTime().toISO8601(true);
  juce::SystemStats::setApplicationCrashHandler(onCrash);
}

juce::String takeCrash()
{
  auto file = crashFile();
  if (!file.existsAsFile())
    return {};
  const auto text = file.loadFileAsString();
  file.moveFileTo(folder().getChildFile("travamento-visto.txt"));
  return text;
}

juce::URL issueUrl(Kind kind, const juce::String& description, const juce::String& details)
{
  const bool crash = kind == Kind::crash;
  const auto firstLine = description.trim().upToFirstOccurrenceOf("\n", false, false).substring(0, 70);
  const auto title = crash ? juce::String::fromUTF8("O Nexus fechou sozinho") : (firstLine.isNotEmpty() ? firstLine : "Problema");
  auto build = [&](const juce::String& registro) {
    return juce::URL(kRepo + "/issues/new")
      .withParameter("template", "problema.yml")
      .withParameter("title", "[" + nexus::update::currentVersion() + "] " + title)
      .withParameter("labels", crash ? "travamento" : "problema")
      .withParameter("descricao", description)
      .withParameter("sistema", systemInfo())
      .withParameter("registro", registro);
  };
  // Corta o registro (das linhas mais velhas) até o endereço caber.
  juce::StringArray lines;
  lines.addLines(details);
  auto url = build(details);
  while (url.toString(true).length() > kMaxUrlLength && lines.size() > 1)
  {
    lines.removeRange(0, juce::jmax(1, lines.size() / 4));
    url = build("(cortado; o texto completo foi copiado)\n" + lines.joinIntoString("\n"));
  }
  return url;
}

void send(Kind kind, const juce::String& description, const juce::String& details)
{
  juce::SystemClipboard::copyTextToClipboard(description + "\n\n" + systemInfo() + "\n\n" + details);
  log("relatorio", kind == Kind::crash ? "enviando relatório de travamento" : "enviando relatório de problema");
  issueUrl(kind, description, details).launchInDefaultBrowser();
}
} // namespace nexus::report
