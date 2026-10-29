#include "Updater.h"

#include "../report/Report.h"

#if JUCE_LINUX || JUCE_MAC
#include <unistd.h>
#endif

#ifndef NEXUS_VERSAO
#define NEXUS_VERSAO "0.0.0"
#endif

namespace nexus::update
{
namespace
{
const char* kLatest = "https://api.github.com/repos/disouz4-dev/Nexus-Modeler/releases/latest";
const char* kReleases = "https://github.com/disouz4-dev/Nexus-Modeler/releases";

#if JUCE_LINUX
// Lugares da instalação pelo terminal (os mesmos do packaging/instalar.sh).
juce::File home()
{
  return juce::File::getSpecialLocation(juce::File::userHomeDirectory);
}

bool installedByUser()
{
  const auto exe = juce::File::getSpecialLocation(juce::File::currentExecutableFile);
  // O programa independente roda de ~/.local/bin; no plugin, vale o que está lá.
  // Atalho (link) para /usr/bin: o .deb já juntou as duas, vale a do sistema.
  const auto bin = home().getChildFile(".local/bin/nexus-modeler");
  return exe.isAChildOf(home()) || (bin.existsAsFile() && !bin.isSymbolicLink());
}

// Coloca "from" no lugar de "to" sem deixar o programa sem arquivo no meio:
// copia ao lado e troca pelo nome (o Linux deixa trocar até o programa que
// está rodando).
juce::String replace(const juce::File& from, const juce::File& to)
{
  if (!from.exists())
    return {};
  const auto fresh = to.getSiblingFile(to.getFileName() + ".novo");
  fresh.deleteRecursively();
  const bool copied = from.isDirectory() ? from.copyDirectoryTo(fresh) : from.copyFileTo(fresh);
  if (!copied)
    return juce::String::fromUTF8("Não foi possível copiar para ") + to.getParentDirectory().getFullPathName();
  if (!from.isDirectory())
    fresh.setExecutePermission(true);
  if (to.isDirectory())
    to.deleteRecursively();
  if (!fresh.moveFileTo(to))
    return juce::String::fromUTF8("Não foi possível trocar ") + to.getFullPathName();
  return {};
}

// Abre o .tar.gz e troca o programa, o VST3 e o LV2 que já estavam
// instalados na pasta do usuário.
juce::String installTarball(const juce::File& tarball)
{
  const auto temp = juce::File::getSpecialLocation(juce::File::tempDirectory).getChildFile("nexus-atualizacao");
  temp.deleteRecursively();
  temp.createDirectory();
  juce::ChildProcess tar;
  if (!tar.start(juce::StringArray{"tar", "-xzf", tarball.getFullPathName(), "-C", temp.getFullPathName()})
      || !tar.waitForProcessToFinish(180000) || tar.getExitCode() != 0)
    return juce::String::fromUTF8("Não foi possível abrir o arquivo baixado.");
  auto root = temp.getChildFile("Nexus Modeler");
  if (!root.isDirectory())
    return juce::String::fromUTF8("O arquivo baixado não tem o Nexus Modeler dentro.");
  juce::String error;
  const auto bin = home().getChildFile(".local/bin/nexus-modeler");
  if (bin.existsAsFile())
    error = replace(root.getChildFile("nexus-modeler"), bin);
  for (const auto& [name, folder] : {std::pair<const char*, const char*>{"Nexus Modeler.vst3", ".vst3"}, {"Nexus Modeler.lv2", ".lv2"}})
  {
    const auto installed = home().getChildFile(folder).getChildFile(name);
    if (error.isEmpty() && installed.isDirectory())
      error = replace(root.getChildFile(name), installed);
  }
  temp.deleteRecursively();
  return error;
}
#endif

#if JUCE_MAC
juce::String shellQuoted(const juce::String& s)
{
  return "'" + s.replace("'", "'\\''") + "'";
}

// Lançamento sem o .zip: monta o .dmg e abre direto o instalador .pkg.
bool openMacInstaller(const juce::File& dmg)
{
  juce::ChildProcess attach;
  if (!attach.start(juce::StringArray{"hdiutil", "attach", "-nobrowse", "-noverify", dmg.getFullPathName()}))
    return false;
  const auto out = attach.readAllProcessOutput();
  for (const auto& line : juce::StringArray::fromLines(out))
    if (line.contains("/Volumes/"))
    {
      const juce::File volume(line.fromFirstOccurrenceOf("/Volumes/", true, false).trim());
      for (const auto& pkg : volume.findChildFiles(juce::File::findFiles, false, "*.pkg"))
        return pkg.startAsProcess();
    }
  return false;
}

// Abre o .zip e troca o programa, o VST3 e o AU no lugar onde já estavam
// (pastas do sistema, pelo instalador, ou do usuário, pelo terminal), sem abrir
// instalador. Nas pastas do sistema o Mac pede a senha uma vez, na janela dele.
} // namespace

juce::String installMacZip(const juce::File& zip, const juce::String& version)
{
  const auto temp = juce::File::getSpecialLocation(juce::File::tempDirectory).getChildFile("nexus-atualizacao");
  temp.deleteRecursively();
  temp.createDirectory();
  juce::ChildProcess unzip;
  if (!unzip.start(juce::StringArray{"ditto", "-x", "-k", zip.getFullPathName(), temp.getFullPathName()})
      || !unzip.waitForProcessToFinish(180000) || unzip.getExitCode() != 0)
    return juce::String::fromUTF8("Não foi possível abrir o arquivo baixado.");
  const auto root = temp.getChildFile("Nexus Modeler");
  if (!root.isDirectory())
    return juce::String::fromUTF8("O arquivo baixado não tem o Nexus Modeler dentro.");

  // Para testar sem mexer no Mac de verdade: todas as pastas dentro desta.
  const auto base = juce::SystemStats::getEnvironmentVariable("NEXUS_ATUALIZACAO_RAIZ", {});
  const auto home = base + juce::File::getSpecialLocation(juce::File::userHomeDirectory).getFullPathName();
  juce::StringArray apps{base + "/Applications", home + "/Applications"};
  // O programa aberto agora, onde quer que esteja.
  if (juce::JUCEApplicationBase::isStandaloneApp())
    apps.addIfNotAlreadyThere(juce::File::getSpecialLocation(juce::File::currentApplicationFile).getParentDirectory().getFullPathName());
  const std::vector<std::pair<juce::String, juce::StringArray>> parts{
    {"Nexus Modeler.app", apps},
    {"Nexus Modeler.vst3", {base + "/Library/Audio/Plug-Ins/VST3", home + "/Library/Audio/Plug-Ins/VST3"}},
    {"Nexus Modeler.component", {base + "/Library/Audio/Plug-Ins/Components", home + "/Library/Audio/Plug-Ins/Components"}},
  };

  // Um roteiro só, para pedir a senha (quando precisa) uma vez. Cada item é
  // copiado ao lado e trocado pelo nome, para nunca ficar sem programa no meio.
  juce::String script = "set -e\n";
  bool admin = false, au = false;
  int found = 0;
  for (const auto& [name, folders] : parts)
  {
    const auto fresh = root.getChildFile(name);
    if (!fresh.isDirectory())
      continue;
    for (const auto& folder : folders)
    {
      const auto installed = juce::File(folder).getChildFile(name);
      if (!installed.isDirectory())
        continue;
      ++found;
      au = au || name.endsWith(".component");
      if (!installed.hasWriteAccess() || !installed.getChildFile("Contents").hasWriteAccess()
          || !installed.getParentDirectory().hasWriteAccess())
        admin = true;
      const auto to = shellQuoted(installed.getFullPathName()), next = shellQuoted(installed.getFullPathName() + ".novo");
      script << "rm -rf " << next << "\nditto " << shellQuoted(fresh.getFullPathName()) << " " << next << "\nrm -rf " << to << "\nmv "
             << next << " " << to << "\n";
    }
  }
  if (found == 0)
    return juce::String::fromUTF8("Não achei o Nexus instalado para trocar.");
  // A lista de Audio Units do Mac: para o Logic e o GarageBand verem o AU novo.
  if (au)
    script << "killall -9 AudioComponentRegistrar >/dev/null 2>&1 || true\n";
  const auto file = temp.getChildFile("trocar.sh");
  file.replaceWithText(script, false, false, "\n"); // quebra de linha do Mac, não a do Windows

  juce::StringArray command{"/bin/sh", file.getFullPathName()};
  if (admin)
    command = {"osascript", "-e",
               "do shell script \"/bin/sh \" & quoted form of \"" + file.getFullPathName() + "\" with administrator privileges with prompt \""
                 + juce::String::fromUTF8("O Nexus Modeler vai instalar a versão ") + version + ".\""};
  juce::ChildProcess run;
  const bool ok = run.start(command) && run.waitForProcessToFinish(600000) && run.getExitCode() == 0;
  const auto output = run.readAllProcessOutput();
  temp.deleteRecursively();
  if (ok)
    return {};
  if (output.contains("-128"))
    return juce::String::fromUTF8("A atualização foi cancelada na senha.");
  return juce::String::fromUTF8("Não foi possível trocar os arquivos do Nexus. ") + output.trim().fromLastOccurrenceOf("\n", false, false);
}

namespace
{
#endif

juce::String headers()
{
  return "Accept: application/vnd.github+json\r\nUser-Agent: Nexus-Modeler/" + currentVersion();
}

// Nome do instalador de cada sistema, como o fluxo de pacotes grava.
bool matches(const juce::String& name, const juce::String& system)
{
  const auto n = name.toLowerCase();
  if (system == "windows")
    return n.endsWith("-windows-instalador.exe");
  if (system == "mac")
    return n.endsWith("-mac.zip");
  if (system == "mac-dmg")
    return n.endsWith(".dmg");
  if (system == "linux-deb")
    return n.endsWith(".deb") && n.contains("linux");
  if (system == "linux")
    return n.endsWith(".tar.gz") && n.contains("linux");
  return false;
}
} // namespace

juce::String currentVersion()
{
  // Variável de ambiente só para testar o aviso com uma versão antiga.
  const auto forced = juce::SystemStats::getEnvironmentVariable("NEXUS_VERSAO_ATUAL", {});
  return forced.isNotEmpty() ? forced : juce::String(NEXUS_VERSAO);
}

int compareVersions(const juce::String& a, const juce::String& b)
{
  // "0.2.0" > "0.2.0-beta.2" > "0.2.0-beta.1" > "0.1.0" (versão de teste vem
  // antes da final de mesmo número; entre testes, conta o número no fim).
  auto clean = [](juce::String v) { return v.trim().trimCharactersAtStart("vV"); };
  const auto x = clean(a), y = clean(b);
  const auto px = juce::StringArray::fromTokens(x.upToFirstOccurrenceOf("-", false, false), ".", "");
  const auto py = juce::StringArray::fromTokens(y.upToFirstOccurrenceOf("-", false, false), ".", "");
  for (int i = 0; i < juce::jmax(px.size(), py.size()); ++i)
  {
    const int p = i < px.size() ? px[i].getIntValue() : 0;
    const int q = i < py.size() ? py[i].getIntValue() : 0;
    if (p != q)
      return p < q ? -1 : 1;
  }
  const auto sx = x.fromFirstOccurrenceOf("-", false, false), sy = y.fromFirstOccurrenceOf("-", false, false);
  if (sx.isEmpty() || sy.isEmpty())
    return sx.isEmpty() == sy.isEmpty() ? 0 : (sx.isEmpty() ? 1 : -1);
  const int nx = sx.retainCharacters("0123456789").getIntValue(), ny = sy.retainCharacters("0123456789").getIntValue();
  if (nx != ny)
    return nx < ny ? -1 : 1;
  return sx.compareNatural(sy) < 0 ? -1 : (sx == sy ? 0 : 1);
}

bool meetsMinimum(const juce::String& minimum)
{
  if (minimum.trim().isEmpty())
    return true;
  auto mine = currentVersion();
  if (!minimum.contains("-"))
    mine = mine.upToFirstOccurrenceOf("-", false, false);
  return compareVersions(mine, minimum) >= 0;
}

juce::String thisSystem()
{
#if JUCE_WINDOWS
  return "windows";
#elif JUCE_MAC
  return "mac";
#else
  // Instalado pelo terminal (na pasta do usuário): o .tar.gz, que o próprio
  // Nexus coloca no lugar. Instalado pelo .deb (em /usr): o .deb de novo, pela
  // loja do sistema.
  if (installedByUser())
    return "linux";
  return juce::File("/usr/bin/dpkg").existsAsFile() ? "linux-deb" : "linux";
#endif
}

Release parseRelease(const juce::String& json, const juce::String& system)
{
  Release r;
  const auto v = juce::JSON::parse(json);
  if (!v.isObject())
    return r;
  r.version = v["tag_name"].toString().trimCharactersAtStart("vV");
  r.notes = v["body"].toString();
  r.page = v["html_url"].toString();
  if (auto* assets = v["assets"].getArray())
  {
    // O .deb é o preferido no Linux com dpkg; sem ele, o .tar.gz. No Mac, o
    // .zip, que o próprio Nexus coloca no lugar; sem ele, o .dmg.
    juce::StringArray order{system};
    if (system == "linux-deb")
      order.add("linux");
    if (system == "mac")
      order.add("mac-dmg"); // lançamento sem o .zip: o instalador

    for (const auto& s : order)
    {
      for (const auto& a : *assets)
      {
        if (matches(a["name"].toString(), s))
        {
          r.assetName = a["name"].toString();
          r.assetUrl = a["browser_download_url"].toString();
          r.assetSize = static_cast<juce::int64>(a["size"]);
          return r;
        }
      }
    }
  }
  return r;
}

// ------------------------------------------------------------------ Updater

Updater::Updater() : juce::Thread("Nexus atualizacao")
{
  juce::PropertiesFile::Options o;
  o.applicationName = "atualizacao";
  o.folderName = "Nexus Modeler";
  o.filenameSuffix = "settings";
  o.osxLibrarySubFolder = "Application Support";
  mPrefs = std::make_unique<juce::PropertiesFile>(o);
}

Updater::~Updater()
{
  stopThread(4000);
}

juce::URL Updater::releaseUrl() const
{
  // NEXUS_ATUALIZACAO_URL: outro endereço (ou um arquivo local) para os testes.
  const auto custom = juce::SystemStats::getEnvironmentVariable("NEXUS_ATUALIZACAO_URL", {});
  if (custom.isEmpty())
    return juce::URL(kLatest);
  if (juce::File::isAbsolutePath(custom))
    return juce::URL(juce::File(custom));
  return juce::URL(custom);
}

bool Updater::automatic() const
{
  return mPrefs->getBoolValue("automatica", true);
}

void Updater::setAutomatic(bool on)
{
  mPrefs->setValue("automatica", on);
  mPrefs->saveIfNeeded();
}

void Updater::checkIfDue()
{
  if (!automatic() || mState.load() != State::idle)
    return;
  start(Job::check);
}

void Updater::checkNow()
{
  const auto s = mState.load();
  if (s == State::checking || s == State::downloading)
    return;
  mManual = true;
  mShowSkipped = true; // pedida pelo menu: mostra mesmo uma versão ignorada
  mDismissed = false;
  start(Job::check);
}

void Updater::downloadAndOpen()
{
  const auto r = release();
  if (r.assetUrl.isEmpty())
  {
    openPage();
    return;
  }
  if (mState.load() == State::downloading)
    return;
  start(Job::download);
}

void Updater::openPage() const
{
  const auto page = release().page;
  juce::URL(page.isNotEmpty() ? page : juce::String(kReleases)).launchInDefaultBrowser();
}

void Updater::skipThisVersion()
{
  mPrefs->setValue("ignorar", release().version);
  mPrefs->saveIfNeeded();
  dismiss();
}

bool Updater::takeNewVersion()
{
  // As ferramentas de teste (NEXUS_SEM_PADRAO) não mexem na versão vista.
  if (juce::SystemStats::getEnvironmentVariable("NEXUS_SEM_PADRAO", {}).isNotEmpty())
    return false;
  const juce::ScopedLock lock(mLock);
  if (mPrefs->getValue("versao_vista") == currentVersion())
    return false;
  mPrefs->setValue("versao_vista", currentVersion());
  mPrefs->saveIfNeeded();
  return true;
}

void Updater::dismiss()
{
  mDismissed = true;
  sendChangeMessage();
}

bool Updater::showBadge() const
{
  const auto s = mState.load();
  if (s == State::downloading || s == State::ready)
    return true;
  if (mDismissed.load())
    return false;
  // Falha ao baixar uma versão encontrada: o aviso fica, para tentar de novo.
  if (s == State::failed)
    return release().version.isNotEmpty();
  if (s == State::available)
    return mPrefs->getValue("ignorar") != release().version || mShowSkipped.load();
  return false;
}

Release Updater::release() const
{
  juce::ScopedLock l(mLock);
  return mRelease;
}

juce::String Updater::error() const
{
  juce::ScopedLock l(mLock);
  return mError;
}

juce::File Updater::downloadedFile() const
{
  juce::ScopedLock l(mLock);
  return mFile;
}

void Updater::setState(State s)
{
  mState = s;
  sendChangeMessage();
}

void Updater::start(Job job)
{
  // Espera um trabalho anterior acabar (verificar dura poucos segundos).
  stopThread(10000);
  mJob = job;
  startThread(juce::Thread::Priority::low);
}

void Updater::restartProgram()
{
  const auto exe = juce::File::getSpecialLocation(juce::File::currentExecutableFile);
  if (!juce::JUCEApplicationBase::isStandaloneApp())
    return;
#if JUCE_LINUX || JUCE_MAC
  // Abrir já a cópia nova não dá: ela vê esta ainda aberta e desiste. Um
  // ajudante espera esta fechar e abre o programa no lugar instalado (no
  // Linux, depois da troca, o caminho deste vem com " (deleted)" no fim).
  auto path = exe.getFullPathName();
  if (path.endsWith(" (deleted)"))
    path = path.dropLastCharacters(10);
#if JUCE_MAC
  // No Mac abre o pacote do programa (.app), como o Finder faz.
  const auto quoted = shellQuoted(juce::File::getSpecialLocation(juce::File::currentApplicationFile).getFullPathName());
  const auto script = "while kill -0 " + juce::String((int) getpid()) + " 2>/dev/null; do sleep 0.2; done; exec /usr/bin/open " + quoted;
#else
  const auto quoted = "'" + path.replace("'", "'\\''") + "'";
  const auto script = "while kill -0 " + juce::String((int) getpid()) + " 2>/dev/null; do sleep 0.2; done; exec " + quoted;
#endif
  const auto command = script.toStdString(); // pronto antes de dividir o processo
  const auto pid = fork();
  if (pid == 0)
  {
    setsid(); // solto deste: segue vivo depois que este fecha
    execl("/bin/sh", "sh", "-c", command.c_str(), (char*) nullptr);
    _exit(127);
  }
  if (pid > 0)
    juce::JUCEApplicationBase::quit();
#else
  if (exe.startAsProcess())
    juce::JUCEApplicationBase::quit();
#endif
}

void Updater::run()
{
  const auto job = mJob.exchange(Job::none);
  if (job == Job::check)
    check();
  else if (job == Job::download)
    download();
}

void Updater::check()
{
  setState(State::checking);
  int status = 0;
  juce::String text;
  if (auto stream = releaseUrl().createInputStream(juce::URL::InputStreamOptions(juce::URL::ParameterHandling::inAddress)
                                                     .withExtraHeaders(headers())
                                                     .withConnectionTimeoutMs(10000)
                                                     .withStatusCode(&status)))
    text = stream->readEntireStreamAsString();

  const bool local = releaseUrl().isLocalFile();
  Release r;
  State result = State::failed;
  juce::String error;
  if (!local && status == 404)
    result = State::upToDate; // ainda não há lançamento publicado
  else if (text.isEmpty() || (!local && status != 200))
    error = status == 0 ? juce::String::fromUTF8("Sem conexão com o GitHub.")
                        : juce::String::fromUTF8("O GitHub respondeu com erro ") + juce::String(status) + ".";
  else
  {
    r = parseRelease(text, thisSystem());
    if (r.version.isEmpty())
      error = juce::String::fromUTF8("Resposta do GitHub sem versão.");
    else
    {
      result = compareVersions(r.version, currentVersion()) > 0 ? State::available : State::upToDate;
      // Endereço de teste não vale para a memória do programa.
      if (juce::SystemStats::getEnvironmentVariable("NEXUS_ATUALIZACAO_URL", {}).isEmpty())
      {
        mPrefs->setValue("ultima_verificacao", juce::String(juce::Time::currentTimeMillis()));
        mPrefs->setValue("versao_encontrada", r.version);
        mPrefs->setValue("lancamento", text);
        mPrefs->saveIfNeeded();
      }
    }
  }
  // Sem conexão: uma versão nova encontrada antes continua valendo.
  if (result == State::failed)
  {
    const auto known = parseRelease(mPrefs->getValue("lancamento"), thisSystem());
    if (known.version.isNotEmpty() && compareVersions(known.version, currentVersion()) > 0)
    {
      r = known;
      result = State::available;
    }
  }
  {
    juce::ScopedLock l(mLock);
    mRelease = r;
    mError = error;
  }
  if (error.isNotEmpty())
    nexus::report::log("atualizacao", "procurar: " + error);
  if (mManual.exchange(false))
    mManualResult = true;
  setState(result);
}

void Updater::download()
{
  const auto r = release();
  auto folder = juce::File::getSpecialLocation(juce::File::userHomeDirectory).getChildFile("Downloads");
  if (!folder.isDirectory() && !folder.createDirectory())
    folder = juce::File::getSpecialLocation(juce::File::tempDirectory);
  const auto target = folder.getChildFile(juce::File::createLegalFileName(r.assetName));
  mProgress = 0.0f;
  setState(State::downloading);

  juce::String error;
  // Já baixado antes (mesmo tamanho): só abre.
  if (!(target.existsAsFile() && r.assetSize > 0 && target.getSize() == r.assetSize))
  {
    const auto partial = target.withFileExtension(target.getFileExtension() + ".parcial");
    partial.deleteFile();
    int status = 0;
    auto stream = juce::URL(r.assetUrl).createInputStream(juce::URL::InputStreamOptions(juce::URL::ParameterHandling::inAddress)
                                                            .withExtraHeaders("User-Agent: Nexus-Modeler/" + currentVersion())
                                                            .withConnectionTimeoutMs(15000)
                                                            .withNumRedirectsToFollow(8)
                                                            .withStatusCode(&status));
    if (stream == nullptr)
      error = juce::String::fromUTF8("Não foi possível baixar o instalador.");
    else
    {
      const auto total = r.assetSize > 0 ? r.assetSize : stream->getTotalLength();
      juce::int64 done = 0;
      {
        juce::FileOutputStream out(partial);
        if (!out.openedOk())
          error = juce::String::fromUTF8("Não foi possível gravar em ") + folder.getFullPathName();
        juce::HeapBlock<char> buffer(1 << 16);
        while (error.isEmpty() && !stream->isExhausted())
        {
          if (threadShouldExit())
          {
            error = juce::String::fromUTF8("Download cancelado.");
            break;
          }
          const int n = stream->read(buffer, 1 << 16);
          if (n < 0)
            error = juce::String::fromUTF8("A conexão caiu no meio do download.");
          if (n <= 0)
            break;
          out.write(buffer, static_cast<size_t>(n));
          done += n;
          if (total > 0)
          {
            const float p = static_cast<float>(done) / static_cast<float>(total);
            // Avisa a tela a cada 1%.
            if (static_cast<int>(p * 100.0f) != static_cast<int>(mProgress.load() * 100.0f))
            {
              mProgress = p;
              sendChangeMessage();
            }
          }
        }
        out.flush();
      }
      if (error.isEmpty() && total > 0 && done != total)
        error = juce::String::fromUTF8("O download veio incompleto.");
      if (error.isEmpty())
      {
        target.deleteFile();
        if (!partial.moveFileTo(target))
          error = juce::String::fromUTF8("Não foi possível gravar em ") + folder.getFullPathName();
      }
      if (error.isNotEmpty())
        partial.deleteFile();
    }
  }
  {
    juce::ScopedLock l(mLock);
    mError = error;
    mFile = error.isEmpty() ? target : juce::File();
  }
  if (error.isNotEmpty())
    nexus::report::log("atualizacao", "baixar: " + error);
  if (error.isNotEmpty())
  {
    // O aviso continua, agora com "tentar de novo".
    setState(State::failed);
    return;
  }
#if JUCE_LINUX || JUCE_MAC
  // Linux instalado pelo terminal, ou Mac: o próprio Nexus coloca a versão
  // nova no lugar, sem instalador.
#if JUCE_LINUX
  const bool inPlace = target.getFileName().endsWithIgnoreCase(".tar.gz");
#else
  const bool inPlace = target.hasFileExtension("zip");
#endif
  if (inPlace)
  {
#if JUCE_LINUX
    const auto installError = installTarball(target);
#else
    const auto installError = installMacZip(target, release().version);
#endif
    if (installError.isNotEmpty())
    {
      nexus::report::log("atualizacao", "instalar: " + installError);
      {
        juce::ScopedLock l(mLock);
        mError = installError;
      }
      setState(State::failed);
      return;
    }
    nexus::report::log("atualizacao", "instalada a " + release().version);
    target.deleteFile(); // já está no lugar; não deixa o arquivo na pasta Downloads
    mInPlace = true;
    mProgress = 1.0f;
    setState(State::ready);
    return;
  }
#endif
  mProgress = 1.0f;
  setState(State::ready);
  if (juce::SystemStats::getEnvironmentVariable("NEXUS_ATUALIZACAO_NAO_ABRIR", {}).isNotEmpty())
    return;
#if JUCE_MAC
  // Direto no instalador (sem a pessoa ter que achar o .pkg no .dmg).
  if (target.hasFileExtension("dmg") && openMacInstaller(target))
    return;
#endif
  // Abre o instalador no fio da tela (o sistema prefere assim).
  juce::MessageManager::callAsync([target] {
#if JUCE_WINDOWS
    // Programa independente: sem perguntas, o instalador fecha o Nexus e
    // atualiza; um ajudante (o cmd) espera o instalador terminar e abre o
    // Nexus de novo. Dentro do programa de gravação o instalador abre normal,
    // para não fechar o programa de gravação sem a pessoa salvar o projeto.
    if (target.hasFileExtension("exe") && juce::JUCEApplicationBase::isStandaloneApp())
    {
      const auto exe = juce::File::getSpecialLocation(juce::File::currentExecutableFile);
      const auto command = "cmd.exe /d /s /c \"start \"\" /wait " + target.getFullPathName().quoted()
                           + " /SILENT /SUPPRESSMSGBOXES /CLOSEAPPLICATIONS /NORESTART & start \"\" " + exe.getFullPathName().quoted() + "\"";
      static juce::ChildProcess helper;
      if (helper.start(command, 0))
        return;
    }
#endif
    if (!target.startAsProcess())
      target.revealToUser();
  });
}
} // namespace nexus::update
