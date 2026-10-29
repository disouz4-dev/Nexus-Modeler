#include "GearCatalog.h"

#include "BinaryData.h"
#include "../update/Updater.h"

#include <algorithm>

namespace nexus::gear
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

// Versões antigas baixavam só a arte, solta numa pasta "Arte" (um arquivo
// <id>_* por peça e o indice.json). Passa tudo para Equipamentos/<id>/.
void migrateArte(const juce::File& target)
{
  const auto old = target.getSiblingFile("Arte");
  if (!old.isDirectory())
    return;
  target.createDirectory();
  const auto index = juce::JSON::parse(old.getChildFile("indice.json"));
  if (auto* pieces = index["pecas"].getDynamicObject())
    for (const auto& p : pieces->getProperties())
      if (const auto* files = p.value["arquivos"].getArray())
      {
        const auto dir = target.getChildFile(p.name.toString());
        dir.createDirectory();
        for (const auto& f : *files)
          old.getChildFile(f.toString()).moveFileTo(dir.getChildFile(f.toString()));
      }
  if (index.isObject() && !target.getChildFile("indice.json").existsAsFile())
    old.getChildFile("indice.json").moveFileTo(target.getChildFile("indice.json"));
  old.deleteRecursively();
}

// Antes de ler a pasta: apaga o que ficou para trás (peça que veio igual ou
// mais nova dentro do programa atualizado, ou que esta versão não entende).
void prune(const juce::File& target)
{
  const auto indexFile = target.getChildFile("indice.json");
  if (!indexFile.existsAsFile())
    return;
  auto local = juce::JSON::parse(indexFile);
  auto* pieces = local["pecas"].getDynamicObject();
  if (pieces == nullptr)
    return;
  const auto embedded = embeddedIndex();
  juce::StringArray gone;
  for (const auto& p : pieces->getProperties())
  {
    const auto mine = p.value;
    const auto inside = embedded["pecas"][p.name];
    const auto minimum = mine["nexus_minimo"].toString();
    const bool understood = minimum.isEmpty() || update::compareVersions(update::currentVersion(), minimum) >= 0;
    if ((inside.isObject() && inside["data"].toString() >= mine["data"].toString()) || !understood)
    {
      target.getChildFile(p.name.toString()).deleteRecursively();
      gone.add(p.name.toString());
    }
  }
  for (const auto& id : gone)
    pieces->removeProperty(juce::Identifier(id));
  if (!gone.isEmpty())
    indexFile.replaceWithText(juce::JSON::toString(local));
}
} // namespace

int pedalEngine(const juce::String& name)
{
  for (int i = 0; i < kPedalEngineCount; ++i)
    if (name == kPedalEngines[i])
      return i;
  return -1;
}

std::optional<Kind> kindFromText(const juce::String& text)
{
  if (text == "pedal")
    return Kind::pedal;
  if (text == "cabecote")
    return Kind::amp;
  if (text == "caixa")
    return Kind::cab;
  return std::nullopt;
}

juce::String kindText(Kind kind)
{
  return kind == Kind::pedal ? "pedal" : kind == Kind::amp ? "cabecote" : "caixa";
}

GearCatalog& GearCatalog::instance()
{
  static GearCatalog catalog;
  return catalog;
}

juce::File GearCatalog::folder()
{
  const auto forced = juce::SystemStats::getEnvironmentVariable("NEXUS_EQUIPAMENTOS_PASTA", {});
  if (forced.isNotEmpty())
    return juce::File(forced);
  return juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
#if JUCE_MAC
    .getChildFile("Application Support")
#endif
    .getChildFile("Nexus Modeler")
    .getChildFile("Equipamentos");
}

juce::String GearCatalog::unsupported(const juce::var& manifest)
{
  const auto kind = kindFromText(manifest["tipo"].toString());
  if (!kind)
    return juce::String::fromUTF8("tipo desconhecido");
  const auto engine = manifest["motor"].toString();
  if ((*kind == Kind::pedal && pedalEngine(engine) < 0) || (*kind == Kind::amp && engine != "captura")
      || (*kind == Kind::cab && engine != "caixa"))
    return juce::String::fromUTF8("Precisa de uma versão mais nova do Nexus");
  const auto minimum = manifest["versao_minima_do_programa"].toString();
  if (minimum.isNotEmpty() && update::compareVersions(update::currentVersion(), minimum) < 0)
    return juce::String::fromUTF8("Precisa de uma versão mais nova do Nexus");
  return {};
}

std::optional<GearEntry> GearCatalog::parse(const juce::var& m)
{
  const auto kind = kindFromText(m["tipo"].toString());
  const auto id = m["id"].toString();
  if (!kind || id.isEmpty() || id.containsAnyOf("/\\. ") || unsupported(m).isNotEmpty())
    return std::nullopt;
  GearEntry e;
  e.id = id;
  e.kind = *kind;
  e.name = m["nome"].toString();
  if (e.name.isEmpty())
    e.name = id;
  e.inspiration = m["inspiracao"].toString();
  e.category = m["categoria"].toString();
  e.source = m["fonte"].toString();
  e.engine = m["motor"].toString();
  e.engineCode = *kind == Kind::pedal ? pedalEngine(e.engine) : 0;
  e.tuning = m["regulagem"];
  e.captures = m["capturas"];
  e.cabId = m["caixa_padrao"].toString();
  e.graphicEq = static_cast<bool>(m["equalizador_grafico"]);
  e.version = m["versao"].toString();
  e.minimumVersion = m["versao_minima_do_programa"].toString();
  e.builtInIndex = m.hasProperty("indice_de_fabrica") ? static_cast<int>(m["indice_de_fabrica"]) : -1;
  return e;
}

GearCatalog::GearCatalog()
{
  for (auto& c : mCount)
    c.store(0);
  loadEmbedded();
  const auto dir = folder();
  migrateArte(dir);
  prune(dir);
  reload();
}

void GearCatalog::loadEmbedded()
{
  // As de fábrica primeiro, na posição antiga; depois as embutidas sem índice.
  std::vector<GearEntry> loose;
  std::array<std::vector<GearEntry>, 3> fixed;
  for (int i = 0; i < BinaryData::namedResourceListSize; ++i)
  {
    const juce::String name(BinaryData::namedResourceList[i]);
    if (!name.endsWith("_manifesto_json"))
      continue;
    int size = 0;
    const char* data = BinaryData::getNamedResource(BinaryData::namedResourceList[i], size);
    auto e = parse(juce::JSON::parse(juce::String::fromUTF8(data, size)));
    if (!e)
      continue;
    if (e->builtInIndex >= 0)
      fixed[static_cast<size_t>(e->kind)].push_back(std::move(*e));
    else
      loose.push_back(std::move(*e));
  }
  for (auto& list : fixed)
  {
    std::sort(list.begin(), list.end(), [](const GearEntry& a, const GearEntry& b) { return a.builtInIndex < b.builtInIndex; });
    for (auto& e : list)
    {
      // Índices de fábrica contínuos: a posição é o índice do parâmetro.
      jassert(e.builtInIndex == count(e.kind));
      add(std::move(e));
    }
  }
  std::sort(loose.begin(), loose.end(), [](const GearEntry& a, const GearEntry& b) { return a.id < b.id; });
  for (auto& e : loose)
    add(std::move(e));
}

void GearCatalog::add(GearEntry e)
{
  const auto k = static_cast<size_t>(e.kind);
  const int look = static_cast<int>(mEntries[k].size());
  if (look >= kMaxLooks)
    return;
  e.look = look;
  if (e.kind == Kind::pedal)
    mPedalEngine[static_cast<size_t>(look)].store(e.engineCode);
  if (e.kind == Kind::amp)
    mGraphicEq[static_cast<size_t>(look)].store(e.graphicEq);
  mEntries[k].push_back(std::move(e));
  mCount[k].store(look + 1);
}

void GearCatalog::reload()
{
  std::vector<GearEntry> found;
  for (const auto& dir : folder().findChildFiles(juce::File::findDirectories, false))
  {
    const auto id = dir.getFileName();
    if (id.startsWithChar('.'))
      continue;
    const auto file = dir.getChildFile(id + "_manifesto.json");
    if (!file.existsAsFile())
      continue; // só arte nova de uma peça de fábrica (GearArt::load lê direto)
    auto e = parse(juce::JSON::parse(file));
    if (!e || e->id != id)
      continue;
    e->origin = GearEntry::Origin::downloaded;
    e->builtInIndex = -1; // baixado nunca entra nos parâmetros
    e->folder = dir;
    found.push_back(std::move(*e));
  }
  std::sort(found.begin(), found.end(), [](const GearEntry& a, const GearEntry& b) { return a.id < b.id; });
  bool added = false;
  {
    std::lock_guard<std::mutex> l(mLock);
    for (auto& e : found)
    {
      auto& list = mEntries[static_cast<size_t>(e.kind)];
      auto same = std::find_if(list.begin(), list.end(), [&](const GearEntry& x) { return x.id == e.id; });
      if (same == list.end())
      {
        add(std::move(e));
        added = true;
      }
      // Peça que já estava no catálogo (atualização de uma baixada, ou arte
      // nova de uma de fábrica): não troca no meio do uso; vale a partir da
      // próxima abertura. Nas de fábrica vale sempre a ficha embutida.
    }
  }
  if (added)
    sendChangeMessage();
}

// ================================================================ Instalação

namespace
{
constexpr juce::int64 kMaxFile = 200 * 1024 * 1024;

bool allowedName(const juce::String& name, const juce::String& id)
{
  static const juce::StringArray extensions{".json", ".png", ".jpg", ".wav", ".nam"};
  return name.startsWith(id + "_") && !name.containsAnyOf("/\\:") && !name.contains("..")
         && extensions.contains(name.fromLastOccurrenceOf(".", true, false).toLowerCase());
}
} // namespace

juce::var GearCatalog::localIndex()
{
  const auto parsed = juce::JSON::parse(folder().getChildFile("indice.json"));
  return parsed.isObject() ? parsed : juce::var(new juce::DynamicObject());
}

juce::String GearCatalog::validate(const juce::File& staged, const juce::String& id)
{
  if (id.isEmpty() || id.containsAnyOf("/\\. ") || !staged.isDirectory())
    return juce::String::fromUTF8("peça sem nome válido");
  if (!staged.findChildFiles(juce::File::findDirectories, false).isEmpty())
    return juce::String::fromUTF8("pasta dentro do pacote");
  for (const auto& f : staged.findChildFiles(juce::File::findFiles, false))
  {
    if (!allowedName(f.getFileName(), id))
      return juce::String::fromUTF8("arquivo não aceito: ") + f.getFileName();
    if (f.getSize() > kMaxFile)
      return juce::String::fromUTF8("arquivo grande demais: ") + f.getFileName();
  }
  const auto manifestFile = staged.getChildFile(id + "_manifesto.json");
  if (manifestFile.existsAsFile())
  {
    const auto m = juce::JSON::parse(manifestFile);
    if (m["id"].toString() != id)
      return juce::String::fromUTF8("ficha de outra peça");
    if (const auto why = unsupported(m); why.isNotEmpty())
      return why;
    if (!parse(m))
      return juce::String::fromUTF8("ficha incompleta");
  }
  const auto mapFile = staged.getChildFile(id + "_mapa.json");
  if (!mapFile.existsAsFile())
    return juce::String::fromUTF8("falta o mapa da peça");
  const auto map = juce::JSON::parse(mapFile);
  if (!map.isObject())
    return juce::String::fromUTF8("mapa com defeito");
  if (!staged.getChildFile(id + "_frente.png").existsAsFile()
      && !(staged.getChildFile(id + "_frente.jpg").existsAsFile() && staged.getChildFile(id + "_frente_alfa.png").existsAsFile()))
    return juce::String::fromUTF8("falta a frente da peça");
  if (const auto* knobs = map["botoes"].getArray())
    for (const auto& k : *knobs)
      if (!staged.getChildFile(id + "_botao_" + k["id"].toString() + ".png").existsAsFile())
        return juce::String::fromUTF8("falta a tira do botão ") + k["id"].toString();
  return {};
}

juce::String GearCatalog::installFolder(const juce::File& staged, const juce::String& id, const juce::var& indexEntry)
{
  if (const auto error = validate(staged, id); error.isNotEmpty())
    return error;
  const auto dir = folder();
  dir.createDirectory();
  const auto target = dir.getChildFile(id);
  const auto old = dir.getChildFile(".velho-" + id);
  old.deleteRecursively();
  if (target.exists() && !target.moveFileTo(old))
    return juce::String::fromUTF8("não deu para trocar a peça instalada");
  if (!staged.moveFileTo(target))
  {
    old.moveFileTo(target); // volta a que estava
    return juce::String::fromUTF8("não deu para instalar a peça");
  }
  old.deleteRecursively();
  auto index = localIndex();
  if (auto* root = index.getDynamicObject())
  {
    if (!root->getProperty("pecas").isObject())
      root->setProperty("pecas", juce::var(new juce::DynamicObject()));
    root->getProperty("pecas").getDynamicObject()->setProperty(juce::Identifier(id), indexEntry);
    dir.getChildFile("indice.json").replaceWithText(juce::JSON::toString(index));
  }
  reload();
  return {};
}

juce::String GearCatalog::installPackage(const juce::File& package)
{
  juce::ZipFile zip(package);
  if (zip.getNumEntries() == 0)
    return juce::String::fromUTF8("pacote vazio ou com defeito");
  juce::String id;
  for (int i = 0; i < zip.getNumEntries(); ++i)
    if (const auto name = zip.getEntry(i)->filename; name.endsWith("_manifesto.json"))
      id = name.dropLastCharacters(juce::String("_manifesto.json").length());
  if (id.isEmpty())
    return juce::String::fromUTF8("pacote sem ficha");
  for (int i = 0; i < zip.getNumEntries(); ++i)
  {
    const auto* e = zip.getEntry(i);
    if (!allowedName(e->filename, id) || e->uncompressedSize > kMaxFile)
      return juce::String::fromUTF8("arquivo não aceito no pacote: ") + e->filename;
  }
  const auto staged = folder().getChildFile(".baixando").getChildFile(id);
  staged.deleteRecursively();
  staged.createDirectory();
  if (zip.uncompressTo(staged, true).failed())
  {
    staged.deleteRecursively();
    return juce::String::fromUTF8("não deu para abrir o pacote");
  }
  auto* entry = new juce::DynamicObject();
  entry->setProperty("nome", juce::JSON::parse(staged.getChildFile(id + "_manifesto.json"))["nome"]);
  entry->setProperty("hash", "pacote");
  entry->setProperty("data", juce::Time::getCurrentTime().toISO8601(true));
  entry->setProperty("pacote", package.getFileName());
  const auto error = installFolder(staged, id, juce::var(entry));
  folder().getChildFile(".baixando").deleteRecursively();
  return error;
}

int GearCatalog::pedalEngineAt(int look) const
{
  return look >= 0 && look < count(Kind::pedal) ? mPedalEngine[static_cast<size_t>(look)].load() : 0;
}

bool GearCatalog::graphicEqAt(int look) const
{
  return look >= 0 && look < count(Kind::amp) && mGraphicEq[static_cast<size_t>(look)].load();
}

std::vector<GearEntry> GearCatalog::entries(Kind kind) const
{
  std::lock_guard<std::mutex> l(mLock);
  return mEntries[static_cast<size_t>(kind)];
}

std::optional<GearEntry> GearCatalog::entry(Kind kind, int look) const
{
  std::lock_guard<std::mutex> l(mLock);
  const auto& list = mEntries[static_cast<size_t>(kind)];
  if (look < 0 || look >= static_cast<int>(list.size()))
    return std::nullopt;
  return list[static_cast<size_t>(look)];
}

int GearCatalog::find(Kind kind, const juce::String& id) const
{
  std::lock_guard<std::mutex> l(mLock);
  for (const auto& e : mEntries[static_cast<size_t>(kind)])
    if (e.id == id)
      return e.look;
  return -1;
}

juce::String GearCatalog::idOf(Kind kind, int look) const
{
  std::lock_guard<std::mutex> l(mLock);
  const auto& list = mEntries[static_cast<size_t>(kind)];
  return look >= 0 && look < static_cast<int>(list.size()) ? list[static_cast<size_t>(look)].id : juce::String();
}

int GearCatalog::fallbackFor(Kind kind, int look) const
{
  std::lock_guard<std::mutex> l(mLock);
  const auto& list = mEntries[static_cast<size_t>(kind)];
  if (look < 0 || look >= static_cast<int>(list.size()))
    return 0;
  const auto& e = list[static_cast<size_t>(look)];
  if (e.builtInIndex >= 0)
    return e.builtInIndex;
  for (const auto& x : list)
    if (x.builtInIndex >= 0 && x.engine == e.engine)
      return x.builtInIndex;
  return 0;
}
} // namespace nexus::gear
