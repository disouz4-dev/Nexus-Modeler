// Teste sem janela do catálogo de equipamentos (gear/GearCatalog) e do id em
// texto no estado (NexusProcessor::getStateInformation).
//   nexus_gear_test <pasta assets/gear> [pacote.equipamento]
// Confere:
//  1. o catálogo tem as mesmas peças, na mesma ordem e com os mesmos dados
//     das fichas embutidas (que vieram das tabelas antigas: manifestos.py --conferir);
//  2. um preset salvo antes do id (só os índices) abre igual, um preset novo
//     guarda e devolve os ids, e um id que não está instalado fica guardado;
//  3. (com o pacote) um equipamento novo instala, aparece e vai num preset.
#include <cstdio>
#include <cstdlib>
#include <memory>

#ifdef _WIN32
// O Windows não tem setenv.
static int setenv(const char* name, const char* value, int) { return _putenv_s(name, value); }
#endif

#include "../source/PluginProcessor.h"
#include "../source/ui/Gear.h"

using nexus::gear::GearCatalog;
using nexus::gear::Kind;

static juce::String U(const char* text) { return juce::String::fromUTF8(text); }

static int failures = 0;
static void check(bool ok, const juce::String& what)
{
  std::printf("%s %s\n", ok ? "ok    " : "FALHOU", what.toRawUTF8());
  failures += ok ? 0 : 1;
}

static void setParam(NexusProcessor& p, const juce::String& id, float value)
{
  auto* param = p.params.getParameter(id);
  param->setValueNotifyingHost(param->convertTo0to1(value));
}

static juce::MemoryBlock stateOf(NexusProcessor& p)
{
  juce::MemoryBlock m;
  p.getStateInformation(m);
  return m;
}

// Estado como a versão antiga salvava: sem os *_id.
static juce::MemoryBlock withoutIds(const juce::MemoryBlock& m)
{
  auto xml = juce::AudioProcessor::getXmlFromBinary(m.getData(), static_cast<int>(m.getSize()));
  juce::StringArray drop;
  for (int i = 0; i < xml->getNumAttributes(); ++i)
    if (xml->getAttributeName(i).endsWith("_id") || xml->getAttributeName(i).contains("_id_"))
      drop.add(xml->getAttributeName(i));
  for (const auto& a : drop)
    xml->removeAttribute(a);
  juce::MemoryBlock out;
  juce::AudioProcessor::copyXmlToBinary(*xml, out);
  return out;
}

static void testCatalog(const juce::File& gear)
{
  auto& c = GearCatalog::instance();
  check(c.count(Kind::pedal) == NexusProcessor::kPedalLooks, U("pedais de fábrica: ") + juce::String(c.count(Kind::pedal)));
  check(c.count(Kind::amp) == NexusProcessor::kAmpLooks, U("cabeçotes de fábrica: ") + juce::String(c.count(Kind::amp)));
  check(c.count(Kind::cab) == NexusProcessor::kCabLooks, U("caixas de fábrica: ") + juce::String(c.count(Kind::cab)));
  int compared = 0, wrong = 0;
  for (const auto& f : gear.findChildFiles(juce::File::findFiles, false, "*_manifesto.json"))
  {
    const auto m = juce::JSON::parse(f);
    const auto kind = nexus::gear::kindFromText(m["tipo"].toString());
    const int look = c.find(*kind, m["id"].toString());
    const auto e = c.entry(*kind, look);
    ++compared;
    const bool same = e && look == static_cast<int>(m["indice_de_fabrica"]) && e->name == m["nome"].toString()
                      && e->inspiration == m["inspiracao"].toString() && e->category == m["categoria"].toString()
                      && e->source == m["fonte"].toString() && e->engine == m["motor"].toString()
                      && e->cabId == m["caixa_padrao"].toString() && e->graphicEq == static_cast<bool>(m["equalizador_grafico"]);
    if (!same)
    {
      ++wrong;
      std::printf("   diferente: %s\n", m["id"].toString().toRawUTF8());
    }
  }
  check(compared == 186 && wrong == 0, U("fichas iguais ao catálogo: ") + juce::String(compared) + " comparadas, " + juce::String(wrong) + " diferentes");
  // Motores e equalizador gráfico como eram nas tabelas antigas.
  check(NexusProcessor::pedalKind(8) == NexusProcessor::PedalKind::drop && NexusProcessor::pedalKind(7) == NexusProcessor::PedalKind::gate
          && NexusProcessor::pedalKind(22) == NexusProcessor::PedalKind::triple && NexusProcessor::pedalKind(57) == NexusProcessor::PedalKind::geq
          && NexusProcessor::pedalKind(62) == NexusProcessor::PedalKind::comp && NexusProcessor::pedalKind(0) == NexusProcessor::PedalKind::capture,
        "motores dos pedais (Cherry Drop, Marcasite Gate, Blueberry Chain, Selenite EQ, Apple Comp, Greengage Drive)");
  bool eq = true;
  for (int i = 0; i < NexusProcessor::kAmpLooks; ++i)
    eq = eq && NexusProcessor::hasGraphicEq(i) == (i == 1 || (i >= 50 && i <= 53));
  check(eq, U("equalizador gráfico só no Mark Five e nos Mark 2+, 3, 4 e JP"));
  const auto brit = c.entry(Kind::amp, 0);
  check(brit && brit->cabId == "brit412" && c.find(Kind::cab, "brit412") == 0, U("Brit 800 abre com a Brit 4x12"));
}

static void testState()
{
  // Preset com um pedal de cada motor, cabeçote e caixa fora do começo da lista.
  auto aBox = std::make_unique<NexusProcessor>(); // no heap: a pilha do Windows tem 1 MB
  auto& a = *aBox;
  const int looks[] = {0, 8, 22, 57, 62, 7};
  for (int i = 0; i < 6; ++i)
    setParam(a, NexusProcessor::pedalParam(i, "visual"), static_cast<float>(looks[i]));
  setParam(a, "cabecote", 50.0f);
  setParam(a, "gabinete", 13.0f);
  setParam(a, "cabecote_b", 93.0f);
  setParam(a, "gabinete_b", 28.0f);
  const auto saved = stateOf(a);
  {
    auto xml = juce::AudioProcessor::getXmlFromBinary(saved.getData(), static_cast<int>(saved.getSize()));
    check(xml->getStringAttribute("p2_visual_id") == "cherry_drop" && xml->getStringAttribute("cabecote_id") == "mark2plus"
            && xml->getStringAttribute("gabinete_id") == "vault_810" && xml->getStringAttribute("cabecote_id_b") == "vault210",
          U("o estado guarda o id de cada peça (p2_visual_id=") + xml->getStringAttribute("p2_visual_id") + ", cabecote_id="
            + xml->getStringAttribute("cabecote_id") + ")");
  }
  // Preset da versão antiga: só os índices.
  auto bBox = std::make_unique<NexusProcessor>();
  auto& b = *bBox;
  const auto old = withoutIds(saved);
  b.setStateInformation(old.getData(), static_cast<int>(old.getSize()));
  bool same = true;
  for (int i = 0; i < 6; ++i)
    same = same && b.pedalLook(i) == looks[i];
  same = same && b.ampLook(0) == 50 && b.cabLook(0) == 13 && b.ampLook(1) == 93 && b.cabLook(1) == 28;
  check(same, U("preset antigo (sem id) abre com os mesmos pedais, cabeçotes e caixas"));
  check(b.gearId(Kind::pedal, 1) == "cherry_drop" && b.gearId(Kind::amp, 0) == "mark2plus", U("preset antigo ganha o id pela tabela de fábrica"));
  // O estado salvo pela versão antiga e o salvo de novo agora têm os mesmos parâmetros.
  const auto again = withoutIds(stateOf(b));
  auto x1 = juce::AudioProcessor::getXmlFromBinary(old.getData(), static_cast<int>(old.getSize()));
  auto x2 = juce::AudioProcessor::getXmlFromBinary(again.getData(), static_cast<int>(again.getSize()));
  check(x1->isEquivalentTo(x2.get(), false), U("salvar de novo o preset antigo não muda nenhum parâmetro"));

  // Preset novo, com id: abre igual.
  auto cBox = std::make_unique<NexusProcessor>();
  auto& c = *cBox;
  c.setStateInformation(saved.getData(), static_cast<int>(saved.getSize()));
  same = true;
  for (int i = 0; i < 6; ++i)
    same = same && c.pedalLook(i) == looks[i];
  check(same && c.ampLook(0) == 50 && c.cabLook(1) == 28, "preset novo (com id) abre igual");

  // Automação de um programa de gravação: o valor normalizado de cada opção
  // é o mesmo de antes (as listas dos parâmetros não mudaram de tamanho).
  check(dynamic_cast<juce::AudioParameterChoice*>(c.params.getParameter("p1_visual"))->choices.size() == 63
          && dynamic_cast<juce::AudioParameterChoice*>(c.params.getParameter("cabecote"))->choices.size() == 94
          && dynamic_cast<juce::AudioParameterChoice*>(c.params.getParameter("gabinete"))->choices.size() == 29,
        U("parâmetros de escolha congelados (63 pedais, 94 cabeçotes, 29 caixas)"));
  c.params.getParameter("p1_visual")->setValueNotifyingHost(8.0f / 62.0f);
  check(c.pedalLook(0) == 8, U("automação do visual escolhe o mesmo pedal (8/62 = Cherry Drop)"));

  // Preset de outra pessoa, com um pedal que não está instalado.
  auto xml = juce::AudioProcessor::getXmlFromBinary(saved.getData(), static_cast<int>(saved.getSize()));
  xml->setAttribute("p1_visual_id", "fruta_que_nao_existe");
  juce::MemoryBlock foreign;
  juce::AudioProcessor::copyXmlToBinary(*xml, foreign);
  auto dBox = std::make_unique<NexusProcessor>();
  auto& d = *dBox;
  d.setStateInformation(foreign.getData(), static_cast<int>(foreign.getSize()));
  check(d.missingGear(Kind::pedal, 0) == "fruta_que_nao_existe", U("peça não instalada fica marcada para o cartão \"não está instalado\""));
  const auto resaved = stateOf(d);
  auto x3 = juce::AudioProcessor::getXmlFromBinary(resaved.getData(), static_cast<int>(resaved.getSize()));
  check(x3->getStringAttribute("p1_visual_id") == "fruta_que_nao_existe", U("salvar de novo não perde o id da peça que falta"));
  d.choosePedal(0, 3);
  check(d.missingGear(Kind::pedal, 0).isEmpty() && d.gearId(Kind::pedal, 0) == "gold_centaur", U("escolher outra peça tira a marca"));

  // Preset salvo antes da troca de código dos 34 pedais (Lemon Drive virou
  // Greengage Drive): abre com a peça, sem o cartão, e salva o código novo.
  const auto& catalog = GearCatalog::instance();
  auto beforeBox = std::make_unique<NexusProcessor>();
  auto& before = *beforeBox;
  setParam(before, NexusProcessor::pedalParam(0, "visual"), static_cast<float>(catalog.find(Kind::pedal, "greengage_drive")));
  setParam(before, NexusProcessor::pedalParam(1, "visual"), static_cast<float>(catalog.find(Kind::pedal, "aquamarine_sky")));
  const auto oldState = stateOf(before);
  auto oldXml = juce::AudioProcessor::getXmlFromBinary(oldState.getData(), static_cast<int>(oldState.getSize()));
  oldXml->setAttribute("p1_visual_id", "lemon_drive");
  oldXml->setAttribute("p2_visual_id", "juniper_sky");
  juce::MemoryBlock renamed;
  juce::AudioProcessor::copyXmlToBinary(*oldXml, renamed);
  auto eBox = std::make_unique<NexusProcessor>();
  auto& e = *eBox;
  e.setStateInformation(renamed.getData(), static_cast<int>(renamed.getSize()));
  check(e.missingGear(Kind::pedal, 0).isEmpty() && e.missingGear(Kind::pedal, 1).isEmpty()
            && e.pedalLook(0) == catalog.find(Kind::pedal, "greengage_drive") && e.pedalLook(1) == catalog.find(Kind::pedal, "aquamarine_sky"),
        U("preset com código antigo (lemon_drive, juniper_sky) abre com o pedal renomeado"));
  check(e.gearId(Kind::pedal, 0) == "greengage_drive" && e.gearId(Kind::pedal, 1) == "aquamarine_sky", U("e salva de novo com o código novo"));
}

// Pacote montado por tools/gear_studio/montar_equipamento.py --pacote, com
// uma peça que o programa não conhece: instala, entra no catálogo, a arte
// carrega da pasta, toca num pedal e vai e volta num preset.
static void testPackage(const juce::File& package)
{
  auto& catalog = GearCatalog::instance();
  const auto id = package.getFileName().upToFirstOccurrenceOf("-", false, false);
  check(catalog.find(Kind::pedal, id) < 0, U("antes de instalar, o catálogo não conhece ") + id);
  // Pacote com arquivo de outra peça: recusado.
  {
    const auto bad = package.getSiblingFile("ruim.equipamento");
    juce::ZipFile::Builder b;
    b.addEntry(new juce::MemoryInputStream(juce::String("{}").toUTF8(), 2, false), 0, id + "_manifesto.json", juce::Time());
    b.addEntry(new juce::MemoryInputStream(juce::String("x").toUTF8(), 1, false), 0, "greengage_drive_mapa.json", juce::Time());
    bad.deleteFile();
    juce::FileOutputStream out(bad);
    b.writeToStream(out, nullptr);
    out.flush();
    const auto error = catalog.installPackage(bad);
    check(error.isNotEmpty() && catalog.find(Kind::pedal, id) < 0, U("pacote com arquivo de outra peça é recusado (") + error + ")");
    bad.deleteFile();
  }
  const auto error = catalog.installPackage(package);
  check(error.isEmpty(), U("pacote instalado ") + error);
  const int look = catalog.find(Kind::pedal, id);
  check(look >= NexusProcessor::kPedalLooks, U("a peça nova entra no catálogo depois das de fábrica (posição ") + juce::String(look) + ")");
  check(GearCatalog::folder().getChildFile(id).getChildFile(id + "_manifesto.json").existsAsFile(), U("ficou em Equipamentos/") + id);
  const auto art = nexus::ui::GearArt::load(id);
  check(art.valid() && art.knobs.size() == 3 && art.name.isNotEmpty(), U("a arte carrega da pasta (") + art.name + ", " + juce::String(art.knobs.size()) + U(" botões)"));
  auto aBox = std::make_unique<NexusProcessor>(); // no heap: a pilha do Windows tem 1 MB
  auto& a = *aBox;
  a.choosePedal(0, look);
  check(a.pedalLook(0) == look && NexusProcessor::pedalKind(look) == NexusProcessor::PedalKind::capture
          && juce::roundToInt(a.params.getRawParameterValue("p1_visual")->load()) == 0,
        U("escolhida num pedal: toca com o motor captura e o parâmetro fica no Greengage Drive"));
  const auto saved = stateOf(a);
  auto bBox = std::make_unique<NexusProcessor>();
  auto& b = *bBox;
  b.setStateInformation(saved.getData(), static_cast<int>(saved.getSize()));
  check(b.pedalLook(0) == look && b.gearId(Kind::pedal, 0) == id, U("preset com a peça nova abre com ela (") + b.gearId(Kind::pedal, 0) + ")");
}

int main(int argc, char** argv)
{
  juce::ScopedJuceInitialiser_GUI gui;
  if (argc < 2)
  {
    std::puts("uso: nexus_gear_test <pasta assets/gear> [pacote.equipamento]");
    return 2;
  }
  // Pasta vazia no lugar da de equipamentos baixados: o teste não depende do Mac.
  const auto temp = juce::File::getSpecialLocation(juce::File::tempDirectory).getChildFile("nexus_gear_test");
  temp.deleteRecursively();
  temp.getChildFile("Equipamentos").createDirectory();
  setenv("NEXUS_EQUIPAMENTOS_PASTA", temp.getChildFile("Equipamentos").getFullPathName().toRawUTF8(), 1);
  testCatalog(juce::File(U(argv[1])));
  testState();
  if (argc > 2)
    testPackage(juce::File(U(argv[2])));
  temp.deleteRecursively();
  std::printf(failures == 0 ? "tudo certo\n" : "%d falhas\n", failures);
  return failures == 0 ? 0 : 1;
}
