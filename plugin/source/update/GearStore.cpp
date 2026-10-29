#include "GearStore.h"

#include <juce_cryptography/juce_cryptography.h>

#include "BinaryData.h"
#include "Updater.h"
#include "../gear/GearCatalog.h"

namespace nexus::update
{
namespace
{
juce::var embeddedIndex()
{
  int size = 0;
  if (const char* data = BinaryData::getNamedResource("indice_json", size))
    return juce::JSON::parse(juce::String::fromUTF8(data, size));
  return {};
}

juce::var piece(const juce::var& index, const juce::String& id)
{
  return index["pecas"][juce::Identifier(id)];
}

juce::String sha256(const juce::MemoryBlock& data)
{
  return juce::SHA256(data.getData(), data.getSize()).toHexString();
}
} // namespace

GearStore::GearStore() : juce::Thread("Nexus equipamentos") {}

GearStore::~GearStore()
{
  stopThread(10000);
}

juce::File GearStore::folder()
{
  return nexus::gear::GearCatalog::folder();
}

juce::String GearStore::baseUrl()
{
  for (const char* name : {"NEXUS_EQUIPAMENTOS_URL", "NEXUS_ARTE_URL"})
    if (const auto forced = juce::SystemStats::getEnvironmentVariable(name, {}); forced.isNotEmpty())
      return forced.trimCharactersAtEnd("/") + "/";
  return "https://raw.githubusercontent.com/disouz4-dev/Nexus-Modeler/main/assets/gear/";
}

juce::String GearStore::unsupported(const juce::var& entry)
{
  const auto minimum = entry["nexus_minimo"].toString();
  if (!meetsMinimum(minimum))
    return juce::String::fromUTF8("Precisa de uma versão mais nova do Nexus");
  // Peça com ficha: o motor tem de ser um que este Nexus toca.
  if (entry["tipo"].toString().isNotEmpty())
  {
    auto* m = new juce::DynamicObject();
    for (const char* key : {"tipo", "motor"})
      m->setProperty(key, entry[key]);
    return nexus::gear::GearCatalog::unsupported(juce::var(m));
  }
  return {};
}

void GearStore::checkIfDue()
{
  if (mChecked.exchange(true))
    return;
  mDownload = false;
  startThread(juce::Thread::Priority::low);
}

void GearStore::checkNow()
{
  const auto s = state();
  if (s == State::checking || s == State::downloading)
    return;
  stopThread(10000);
  mChecked = true;
  mDownload = false;
  {
    std::lock_guard<std::mutex> l(mLock);
    mError.clear();
  }
  startThread(juce::Thread::Priority::low);
}

void GearStore::download()
{
  if (state() != State::available && state() != State::failed)
    return;
  bool nothing;
  {
    std::lock_guard<std::mutex> l(mLock);
    nothing = mPending.isEmpty();
  }
  // Erro na procura (sem internet): tentar de novo é procurar de novo.
  if (nothing)
  {
    checkNow();
    return;
  }
  stopThread(10000);
  mDownload = true;
  startThread(juce::Thread::Priority::low);
}

bool GearStore::downloadOne(const juce::String& id)
{
  const auto entry = remoteEntry(id);
  if (!entry.isObject() || unsupported(entry).isNotEmpty() || state() == State::downloading || state() == State::checking)
    return false;
  stopThread(10000);
  {
    std::lock_guard<std::mutex> l(mLock);
    mPending = juce::StringArray(id);
    mNames = juce::StringArray(entry["nome"].toString().isNotEmpty() ? entry["nome"].toString() : id);
  }
  mCount = 1;
  mDownload = true;
  startThread(juce::Thread::Priority::low);
  return true;
}

juce::StringArray GearStore::names() const
{
  std::lock_guard<std::mutex> l(mLock);
  return mNames;
}

juce::String GearStore::error() const
{
  std::lock_guard<std::mutex> l(mLock);
  return mError;
}

juce::var GearStore::remoteEntry(const juce::String& id) const
{
  std::lock_guard<std::mutex> l(mLock);
  return piece(mRemote, id);
}

void GearStore::setState(State s)
{
  mState = s;
  sendChangeMessage();
}

void GearStore::run()
{
  if (mDownload)
    fetch();
  else
  {
    check();
    if (mAutomatic && state() == State::available && !threadShouldExit())
      fetch();
  }
}

void GearStore::check()
{
  setState(State::checking);
  const auto text = juce::URL(baseUrl() + "indice.json").readEntireTextStream(false);
  const auto remote = juce::JSON::parse(text);
  if (!remote["pecas"].isObject())
  {
    // Sem índice: sem internet (avisa, o clique procura de novo) ou ainda não
    // publicado (fica apagado).
    {
      std::lock_guard<std::mutex> l(mLock);
      mPending.clear();
      mNames.clear();
      if (text.isEmpty())
        mError = juce::String::fromUTF8("Não foi possível procurar equipamentos novos agora (sem conexão?).");
    }
    mCount = 0;
    setState(text.isEmpty() ? State::failed : State::upToDate);
    return;
  }
  const auto local = nexus::gear::GearCatalog::localIndex();
  const auto embedded = embeddedIndex();
  juce::StringArray pending, names;
  for (const auto& p : remote["pecas"].getDynamicObject()->getProperties())
  {
    const auto id = p.name.toString();
    const auto entry = p.value;
    if (unsupported(entry).isNotEmpty())
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

juce::String GearStore::fetchOne(const juce::String& id, const juce::var& entry)
{
  const auto* files = entry["arquivos"].getArray();
  const auto* sums = entry["sha256"].getDynamicObject();
  if (files == nullptr || files->isEmpty())
    return juce::String::fromUTF8("peça sem arquivos no índice: ") + id;
  if (sums == nullptr)
    return juce::String::fromUTF8("índice sem o resumo dos arquivos de ") + id;
  // Baixa a peça inteira numa pasta temporária; só instala se veio tudo e
  // cada arquivo confere com o resumo do índice.
  const auto temp = folder().getChildFile(".baixando").getChildFile(id);
  temp.deleteRecursively();
  temp.createDirectory();
  for (const auto& f : *files)
  {
    if (threadShouldExit())
      return juce::String::fromUTF8("cancelado");
    const auto name = f.toString();
    if (!name.startsWith(id + "_") || name.containsAnyOf("/\\:") || name.contains(".."))
      return juce::String::fromUTF8("arquivo fora da peça no índice: ") + name;
    juce::MemoryBlock data;
    if (!juce::URL(baseUrl() + name).readEntireBinaryStream(data, false) || data.getSize() == 0)
      return juce::String::fromUTF8("Não foi possível baixar ") + name;
    if (sha256(data) != sums->getProperty(juce::Identifier(name)).toString())
      return juce::String::fromUTF8("O arquivo baixado não confere: ") + name;
    if (!temp.getChildFile(name).replaceWithData(data.getData(), data.getSize()))
      return juce::String::fromUTF8("Não foi possível gravar ") + name;
  }
  auto& catalog = nexus::gear::GearCatalog::instance();
  // Arte refeita de uma peça que já existe: o programa aberto continua com a
  // antiga até abrir de novo.
  bool known = false;
  for (auto kind : {nexus::gear::Kind::pedal, nexus::gear::Kind::amp, nexus::gear::Kind::cab})
    known = known || catalog.find(kind, id) >= 0;
  if (known && !temp.getChildFile(id + "_manifesto.json").existsAsFile())
    mNeedsRestart = true;
  else if (known)
    for (auto kind : {nexus::gear::Kind::pedal, nexus::gear::Kind::amp, nexus::gear::Kind::cab})
      if (const auto e = catalog.entry(kind, catalog.find(kind, id)); e && e->origin == nexus::gear::GearEntry::Origin::builtIn)
        mNeedsRestart = true;
  return catalog.installFolder(temp, id, entry);
}

void GearStore::fetch()
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
  int done = 0;
  for (const auto& id : pending)
  {
    if (threadShouldExit())
      return;
    const auto error = fetchOne(id, piece(remote, id));
    folder().getChildFile(".baixando").deleteRecursively();
    if (error.isNotEmpty())
    {
      {
        std::lock_guard<std::mutex> l(mLock);
        mError = error;
      }
      setState(State::failed);
      return;
    }
    mProgress = static_cast<float>(++done) / static_cast<float>(pending.size());
    sendChangeMessage();
  }
  setState(State::ready);
}
} // namespace nexus::update
