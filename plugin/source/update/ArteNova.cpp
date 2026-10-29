#include "ArteNova.h"

#include "BinaryData.h"
#include "Updater.h"

namespace nexus::update
{
namespace
{
// Onde o índice e a arte ficam no GitHub (a pasta assets/gear do ramo main).
// NEXUS_ARTE_URL troca a origem para testar (aceita file://).
juce::String baseUrl()
{
  const auto forced = juce::SystemStats::getEnvironmentVariable("NEXUS_ARTE_URL", {});
  return forced.isNotEmpty() ? forced.trimCharactersAtEnd("/") + "/"
                             : "https://raw.githubusercontent.com/disouz4-dev/Nexus-Modeler/main/assets/gear/";
}

juce::var embeddedIndex()
{
  int size = 0;
  if (const char* data = BinaryData::getNamedResource("indice_json", size))
    return juce::JSON::parse(juce::String::fromUTF8(data, size));
  return {};
}

juce::File localIndexFile()
{
  return ArteNova::folder().getChildFile("indice.json");
}

juce::var localIndex()
{
  const auto parsed = juce::JSON::parse(localIndexFile());
  return parsed.isObject() ? parsed : juce::var(new juce::DynamicObject());
}

void saveLocalIndex(const juce::var& index)
{
  localIndexFile().replaceWithText(juce::JSON::toString(index));
}

juce::var piece(const juce::var& index, const juce::String& id)
{
  return index["pecas"][juce::Identifier(id)];
}

juce::DynamicObject* pieces(juce::var& index)
{
  auto* root = index.getDynamicObject();
  if (root == nullptr)
    return nullptr;
  if (!root->getProperty("pecas").isObject())
    root->setProperty("pecas", juce::var(new juce::DynamicObject()));
  return root->getProperty("pecas").getDynamicObject();
}

bool understands(const juce::var& entry)
{
  const auto minimum = entry["nexus_minimo"].toString();
  return minimum.isEmpty() || compareVersions(currentVersion(), minimum) >= 0;
}

void removeFiles(const juce::var& entry)
{
  if (const auto* files = entry["arquivos"].getArray())
    for (const auto& f : *files)
      ArteNova::folder().getChildFile(f.toString()).deleteFile();
}
} // namespace

ArteNova::ArteNova() : juce::Thread("Nexus arte nova") {}

ArteNova::~ArteNova()
{
  stopThread(10000);
}

juce::File ArteNova::folder()
{
  return juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
#if JUCE_MAC
    .getChildFile("Application Support")
#endif
    .getChildFile("Nexus Modeler")
    .getChildFile("Arte");
}

void ArteNova::prune()
{
  if (!localIndexFile().existsAsFile())
    return;
  auto local = localIndex();
  const auto embedded = embeddedIndex();
  auto* list = pieces(local);
  if (list == nullptr)
    return;
  juce::StringArray gone;
  for (const auto& p : list->getProperties())
  {
    const auto mine = p.value;
    const auto inside = piece(embedded, p.name.toString());
    // Programa atualizado trouxe a mesma peça ou uma mais nova; ou esta
    // versão do Nexus não entende a peça baixada.
    if ((inside.isObject() && inside["data"].toString() >= mine["data"].toString()) || !understands(mine))
    {
      removeFiles(mine);
      gone.add(p.name.toString());
    }
  }
  for (const auto& id : gone)
    list->removeProperty(juce::Identifier(id));
  if (!gone.isEmpty())
    saveLocalIndex(local);
}

void ArteNova::checkIfDue()
{
  if (mChecked.exchange(true))
    return;
  mDownload = false;
  startThread(juce::Thread::Priority::low);
}

void ArteNova::download()
{
  if (state() != State::available && state() != State::failed)
    return;
  stopThread(10000);
  mDownload = true;
  startThread(juce::Thread::Priority::low);
}

juce::StringArray ArteNova::names() const
{
  std::lock_guard<std::mutex> l(mLock);
  return mNames;
}

juce::String ArteNova::error() const
{
  std::lock_guard<std::mutex> l(mLock);
  return mError;
}

void ArteNova::setState(State s)
{
  mState = s;
  sendChangeMessage();
}

void ArteNova::run()
{
  if (mDownload)
    fetch();
  else
    check();
}

void ArteNova::check()
{
  setState(State::checking);
  const auto text = juce::URL(baseUrl() + "indice.json").readEntireTextStream(false);
  const auto remote = juce::JSON::parse(text);
  if (!remote["pecas"].isObject())
  {
    // Sem índice (sem internet, ou ainda não publicado): fica quieto.
    setState(State::upToDate);
    return;
  }
  const auto local = localIndex();
  const auto embedded = embeddedIndex();
  juce::StringArray pending, names;
  for (const auto& p : remote["pecas"].getDynamicObject()->getProperties())
  {
    const auto id = p.name.toString();
    const auto entry = p.value;
    if (!understands(entry))
      continue;
    auto have = piece(local, id);
    if (!have.isObject())
      have = piece(embedded, id);
    // Peça nova, ou mudou e é mais nova que a que o Nexus tem.
    if (!have.isObject() || (have["hash"] != entry["hash"] && entry["data"].toString() > have["data"].toString()))
    {
      pending.add(id);
      names.add(entry["nome"].toString().isNotEmpty() ? entry["nome"].toString() : id);
    }
  }
  {
    std::lock_guard<std::mutex> l(mLock);
    mRemote = remote;
    mPending = pending;
    mNames = names;
  }
  mCount = pending.size();
  setState(pending.isEmpty() ? State::upToDate : State::available);
}

void ArteNova::fetch()
{
  juce::var remote;
  juce::StringArray pending;
  {
    std::lock_guard<std::mutex> l(mLock);
    remote = mRemote;
    pending = mPending;
  }
  folder().createDirectory();
  mProgress = 0.0f;
  setState(State::downloading);
  auto local = localIndex();
  int done = 0;
  for (const auto& id : pending)
  {
    if (threadShouldExit())
      return;
    const auto entry = piece(remote, id);
    const auto* files = entry["arquivos"].getArray();
    if (files == nullptr)
      continue;
    // Baixa a peça inteira numa pasta temporária; só troca se veio tudo.
    const auto temp = folder().getChildFile(".baixando");
    temp.deleteRecursively();
    temp.createDirectory();
    bool ok = true;
    for (const auto& f : *files)
    {
      const auto name = f.toString();
      juce::MemoryBlock data;
      if (!juce::URL(baseUrl() + name).readEntireBinaryStream(data, false) || data.getSize() == 0
          || !temp.getChildFile(name).replaceWithData(data.getData(), data.getSize()))
      {
        ok = false;
        std::lock_guard<std::mutex> l(mLock);
        mError = juce::String::fromUTF8("Não foi possível baixar ") + name;
        break;
      }
    }
    if (ok)
    {
      removeFiles(piece(local, id));
      for (const auto& f : *files)
        temp.getChildFile(f.toString()).moveFileTo(folder().getChildFile(f.toString()));
      if (auto* list = pieces(local))
        list->setProperty(juce::Identifier(id), entry);
      saveLocalIndex(local);
    }
    temp.deleteRecursively();
    if (!ok)
    {
      setState(State::failed);
      return;
    }
    mProgress = static_cast<float>(++done) / static_cast<float>(pending.size());
    sendChangeMessage();
  }
  setState(State::ready);
}
} // namespace nexus::update
