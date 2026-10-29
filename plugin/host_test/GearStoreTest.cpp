// Teste de ponta a ponta dos equipamentos novos sem reinstalar
// (update/GearStore), com uma pasta local no lugar do GitHub (file://):
// publicado um equipamento que o programa não conhece, o Nexus acha no
// índice, baixa sozinho conferindo o sha256 de cada arquivo, a peça entra no catálogo,
// a arte carrega da pasta e um preset com o id dela abre com ela. Um arquivo
// adulterado no "GitHub" é recusado sem instalar nada.
//   nexus_arte_test <pasta com indice.json e os arquivos> <peça nova> <nome esperado>
// A pasta sai de tools/gear_studio/montar_equipamento.py rodado numa cópia do
// projeto (ver docs/equipamentos-PROGRESSO.md).
#include <cstdio>
#include <cstdlib>

#ifdef _WIN32
// O Windows não tem setenv.
static int setenv(const char* name, const char* value, int) { return _putenv_s(name, value); }
#endif

#include "../source/PluginProcessor.h"
#include "../source/ui/Gear.h"
#include "../source/update/GearStore.h"

using nexus::gear::GearCatalog;
using nexus::gear::Kind;
using nexus::update::GearStore;

static juce::String U(const char* text) { return juce::String::fromUTF8(text); }

static int failures = 0;
static void check(bool ok, const juce::String& what)
{
  std::printf("%s %s\n", ok ? "ok    " : "FALHOU", what.toRawUTF8());
  failures += ok ? 0 : 1;
}

static bool wait(GearStore& a, std::initializer_list<GearStore::State> until)
{
  for (int i = 0; i < 600; ++i)
  {
    for (auto s : until)
      if (a.state() == s)
        return true;
    juce::Thread::sleep(50);
  }
  return false;
}

static void useRemote(const juce::File& dir)
{
  setenv("NEXUS_EQUIPAMENTOS_URL", ("file://" + dir.getFullPathName()).toRawUTF8(), 1);
}

// Estado (preset) com o pedal 1 sendo a peça dada pelo id.
static juce::MemoryBlock presetWith(const juce::String& id)
{
  NexusProcessor p;
  juce::MemoryBlock m;
  p.getStateInformation(m);
  auto xml = juce::AudioProcessor::getXmlFromBinary(m.getData(), static_cast<int>(m.getSize()));
  xml->setAttribute("p1_visual_id", id);
  juce::MemoryBlock out;
  juce::AudioProcessor::copyXmlToBinary(*xml, out);
  return out;
}

int main(int argc, char** argv)
{
  juce::ScopedJuceInitialiser_GUI gui;
  if (argc < 4)
  {
    std::puts("uso: nexus_arte_test <pasta remota> <peça nova> <nome esperado>");
    return 2;
  }
  const juce::File remote(U(argv[1]));
  const juce::String id(U(argv[2])), expected = U(argv[3]);
  const auto temp = juce::File::getSpecialLocation(juce::File::tempDirectory).getChildFile("nexus_arte_test");
  temp.deleteRecursively();
  setenv("NEXUS_EQUIPAMENTOS_PASTA", temp.getChildFile("Equipamentos").getFullPathName().toRawUTF8(), 1);
  auto& catalog = GearCatalog::instance();

  check(catalog.find(Kind::pedal, id) < 0, U("o programa não conhece ") + id);
  const auto preset = presetWith(id);
  NexusProcessor before;
  before.setStateInformation(preset.getData(), static_cast<int>(preset.getSize()));
  check(before.missingGear(Kind::pedal, 0) == id, U("preset com a peça abre com o cartão \"não está instalado\""));

  // "GitHub" com um arquivo adulterado depois do índice: recusa e não instala.
  {
    const auto bad = temp.getChildFile("remoto_adulterado");
    bad.createDirectory();
    remote.getChildFile("indice.json").copyFileTo(bad.getChildFile("indice.json"));
    for (const auto& f : remote.findChildFiles(juce::File::findFiles, false, id + "_*"))
      f.copyFileTo(bad.getChildFile(f.getFileName()));
    bad.getChildFile(id + "_mapa.json").appendText(" ");
    useRemote(bad);
    GearStore s;
    s.setAutomatic(false); // aqui, passo a passo
    s.checkIfDue();
    wait(s, {GearStore::State::available, GearStore::State::upToDate});
    check(s.downloadOne(id), U("acha a peça no índice e começa a baixar"));
    check(wait(s, {GearStore::State::ready, GearStore::State::failed}) && s.state() == GearStore::State::failed,
          U("arquivo adulterado é recusado (") + s.error() + ")");
    check(!GearStore::folder().getChildFile(id).exists() && catalog.find(Kind::pedal, id) < 0, U("nada foi instalado"));
  }

  useRemote(remote);
  {
    // Como ao abrir o Nexus: procura sozinho e o ícone acende; nada baixa
    // antes do clique. O clique (download) baixa.
    GearStore a;
    a.checkIfDue();
    check(wait(a, {GearStore::State::available, GearStore::State::failed, GearStore::State::upToDate}) && a.state() == GearStore::State::available,
          U("ao abrir, procurou e acendeu sem baixar ") + a.error());
    check(!GearStore::folder().getChildFile(id).exists(), U("antes do clique nada foi baixado"));
    a.download();
    check(wait(a, {GearStore::State::ready, GearStore::State::failed}) && a.state() == GearStore::State::ready,
          U("o clique baixou ") + a.error());
    check(a.count() == 1 && a.names().contains(expected), U("chegou só o equipamento novo: ") + a.names().joinIntoString(", "));
    check(a.remoteEntry(id)["tipo"].toString() == "pedal" && a.remoteEntry(id)["motor"].toString() == "captura",
          U("o índice traz a ficha (tipo e motor)"));
    check(!a.needsRestart(), U("equipamento novo não pede para abrir o Nexus de novo"));
    // Clique no ícone apagado: procura de novo e não acha mais nada.
    a.checkNow();
    check(wait(a, {GearStore::State::available, GearStore::State::upToDate, GearStore::State::failed}) && a.state() == GearStore::State::upToDate,
          U("procurar de novo depois de baixar fica apagado"));
  }
  const auto dir = GearStore::folder().getChildFile(id);
  check(dir.getChildFile(id + "_manifesto.json").existsAsFile() && dir.getChildFile(id + "_mapa.json").existsAsFile(),
        U("a peça ficou em Equipamentos/") + id);
  const int look = catalog.find(Kind::pedal, id);
  check(look >= NexusProcessor::kPedalLooks, U("entrou no catálogo sem reinstalar (posição ") + juce::String(look) + ")");
  const auto entry = catalog.entry(Kind::pedal, look);
  check(entry && entry->name == expected && entry->origin == nexus::gear::GearEntry::Origin::downloaded,
        U("ficha lida da pasta: ") + (entry ? entry->name + " / " + entry->inspiration + " / " + entry->category : juce::String()));
  const auto art = nexus::ui::GearArt::load(id);
  check(art.name == expected && art.front.isValid() && !art.knobs.empty(),
        U("a arte carrega da pasta (") + art.name + ", " + juce::String(art.knobs.size()) + U(" botões)"));
  {
    GearStore b;
    b.checkIfDue();
    wait(b, {GearStore::State::available, GearStore::State::upToDate});
    check(b.state() == GearStore::State::upToDate, U("depois de baixar, não oferece de novo"));
  }

  // O preset que pedia a peça: o aberto antes passa a tocá-la ao instalar, e
  // um aberto agora já abre com ela.
  before.gearInstalled();
  check(before.missingGear(Kind::pedal, 0).isEmpty() && before.pedalLook(0) == look, U("o preset já aberto passa a usar a peça instalada"));
  NexusProcessor after;
  after.setStateInformation(preset.getData(), static_cast<int>(preset.getSize()));
  check(after.pedalLook(0) == look && after.missingGear(Kind::pedal, 0).isEmpty()
          && NexusProcessor::pedalKind(after.pedalLook(0)) == NexusProcessor::PedalKind::capture,
        U("um preset com o id abre com a peça nova (motor captura)"));
  juce::MemoryBlock again;
  after.getStateInformation(again);
  auto xml = juce::AudioProcessor::getXmlFromBinary(again.getData(), static_cast<int>(again.getSize()));
  double visual = -1.0;
  for (auto* e : xml->getChildWithTagNameIterator("PARAM"))
    if (e->getStringAttribute("id") == "p1_visual")
      visual = e->getDoubleAttribute("value");
  check(xml->getStringAttribute("p1_visual_id") == id && visual == 0.0,
        U("salvo de novo: id da peça nova e o parâmetro no Greengage Drive (programa velho toca algo parecido)"));

  temp.deleteRecursively();
  std::printf(failures == 0 ? "tudo certo\n" : "%d falhas\n", failures);
  return failures == 0 ? 0 : 1;
}
