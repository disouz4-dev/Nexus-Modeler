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
// Ponte dos relatórios (tools/relatorio, na Cloudflare): cria o issue sem a
// pessoa precisar de conta no GitHub. Vazio: abre o GitHub preenchido.
const juce::String kRelay = "https://nexus-relatorio.nexusmodeler.workers.dev/relatorio";

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

static Sent sendDirect(Kind kind, const juce::String& description, const juce::String& details)
{
  if (kRelay.isEmpty())
    return {};
  juce::DynamicObject::Ptr body = new juce::DynamicObject();
  body->setProperty("tipo", kind == Kind::crash ? "travamento" : "problema");
  body->setProperty("versao", nexus::update::currentVersion());
  body->setProperty("descricao", description);
  body->setProperty("sistema", systemInfo());
  body->setProperty("registro", details.getLastCharacters(18000));
  int status = 0;
  auto stream = juce::URL(kRelay).withPOSTData(juce::JSON::toString(juce::var(body.get()), true))
                  .createInputStream(juce::URL::InputStreamOptions(juce::URL::ParameterHandling::inPostData)
                                       .withExtraHeaders("Content-Type: application/json\r\nUser-Agent: NexusModeler/" + nexus::update::currentVersion())
                                       .withConnectionTimeoutMs(15000)
                                       .withStatusCode(&status));
  Sent sent;
  if (stream == nullptr)
  {
    sent.error = "sem conexão com a ponte";
    return sent;
  }
  const auto reply = juce::JSON::parse(stream->readEntireStreamAsString());
  sent.number = static_cast<int>(reply["numero"]);
  sent.address = reply["endereco"].toString();
  if (status != 200 || sent.number <= 0)
    sent.error = reply["erro"].toString().isNotEmpty() ? reply["erro"].toString() : "a ponte respondeu " + juce::String(status);
  return sent;
}

void send(Kind kind, const juce::String& description, const juce::String& details, std::function<void(Sent)> done)
{
  log("relatorio", kind == Kind::crash ? "enviando relatório de travamento" : "enviando relatório de problema");
  juce::Thread::launch([kind, description, details, done] {
    auto sent = sendDirect(kind, description, details);
    if (sent.number <= 0)
    {
      // Sem a ponte: abre o GitHub preenchido (precisa de conta) e copia o texto.
      if (sent.error.isNotEmpty())
        log("relatorio", "ponte: " + sent.error);
      juce::MessageManager::callAsync([kind, description, details] {
        juce::SystemClipboard::copyTextToClipboard(description + "\n\n" + systemInfo() + "\n\n" + details);
        issueUrl(kind, description, details).launchInDefaultBrowser();
      });
      sent.openedBrowser = true;
    }
    else
      log("relatorio", "issue #" + juce::String(sent.number));
    juce::MessageManager::callAsync([done, sent] {
      if (done)
        done(sent);
    });
  });
}
} // namespace nexus::report
