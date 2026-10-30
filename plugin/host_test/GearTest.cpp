// Teste sem janela do catálogo de equipamentos (gear/GearCatalog) e do id em
// texto no estado (NexusProcessor::getStateInformation).
//   nexus_gear_test <pasta assets/gear> [pacote.equipamento]
// Confere:
//  1. o catálogo tem as mesmas peças, na mesma posição e com os mesmos dados
//     das fichas embutidas, e as posições de fábrica das peças que saíram do
//     programa (só entra peça revisada) ficam vagas;
//  2. sessão nova sem cabeçote e sem caixa (abre, toca limpo, salva vazia);
//     um preset salvo antes do id (só os índices) abre igual, um preset novo
//     guarda e devolve os ids, e um id que não está instalado fica guardado;
//     a peça que saiu do programa fica vazia (não toca e não ganha cartão);
//  3. a peça revisada volta pelo botão para a mesma posição (as automações
//     antigas continuam valendo), com a caixa dela, o equalizador gráfico e as
//     capturas recomendadas da ficha, e a sessão que tinha o id dela a toca;
//  4. a primeira abertura da instalação nova guarda os dados das versões
//     antigas numa reserva, sem apagar nada, e só faz isso uma vez;
//  5. (com o pacote) um equipamento novo instala, aparece e vai num preset.
#include <array>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <memory>

#ifdef _WIN32
// O Windows não tem setenv.
static int setenv(const char* name, const char* value, int) { return _putenv_s(name, value); }
#endif

#include "../source/PluginProcessor.h"
#include "../source/tone3000/Tone3000.h"
#include "../source/ui/Gear.h"
#include "../source/update/NovaInstalacao.h"

using nexus::gear::GearCatalog;
using nexus::gear::Kind;

static juce::String U(const char* text) { return juce::String::fromUTF8(text); }

static int failures = 0;
static juce::File gRepo; // raiz do projeto (a pasta assets/gear vem pela linha de comando)
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
  // Todas as posições de fábrica existem, com peça ou vagas.
  check(c.count(Kind::pedal) == NexusProcessor::kPedalLooks, U("posições de pedal: ") + juce::String(c.count(Kind::pedal)));
  check(c.count(Kind::amp) == NexusProcessor::kAmpLooks, U("posições de cabeçote: ") + juce::String(c.count(Kind::amp)));
  check(c.count(Kind::cab) == NexusProcessor::kCabLooks, U("posições de caixa: ") + juce::String(c.count(Kind::cab)));
  int compared = 0, wrong = 0;
  std::array<int, 3> embedded{};
  for (const auto& f : gear.findChildFiles(juce::File::findFiles, false, "*_manifesto.json"))
  {
    const auto m = juce::JSON::parse(f);
    const auto kind = nexus::gear::kindFromText(m["tipo"].toString());
    const int look = c.find(*kind, m["id"].toString());
    const auto e = c.entry(*kind, look);
    ++compared;
    ++embedded[static_cast<size_t>(*kind)];
    const bool same = e && !e->vacant && c.presentAt(*kind, look) && look == static_cast<int>(m["indice_de_fabrica"])
                      && e->name == m["nome"].toString() && e->inspiration == m["inspiracao"].toString()
                      && e->category == m["categoria"].toString() && e->source == m["fonte"].toString()
                      && e->engine == m["motor"].toString() && e->cabId == m["caixa_padrao"].toString()
                      && e->graphicEq == static_cast<bool>(m["equalizador_grafico"]);
    if (!same)
    {
      ++wrong;
      std::printf("   diferente: %s\n", m["id"].toString().toRawUTF8());
    }
  }
  check(compared > 0 && wrong == 0, U("fichas iguais ao catálogo: ") + juce::String(compared) + " comparadas, " + juce::String(wrong) + " diferentes");
  // As vagas: tudo o que não veio embutido.
  for (const auto kind : {Kind::pedal, Kind::amp, Kind::cab})
  {
    int vacant = 0, hidden = 0;
    for (const auto& e : c.entries(kind))
      if (e.vacant)
      {
        ++vacant;
        hidden += c.find(kind, e.id) < 0 && !c.presentAt(kind, e.look) && c.idOf(kind, e.look) == e.id ? 1 : 0;
      }
    check(vacant == c.count(kind) - embedded[static_cast<size_t>(kind)] && hidden == vacant,
          U("vagas de ") + nexus::gear::kindText(kind) + ": " + juce::String(vacant) + U(" (não instaladas, com o id guardado)"));
  }
  check(embedded[static_cast<size_t>(Kind::amp)] == 0 && embedded[static_cast<size_t>(Kind::cab)] == 0,
        U("nenhum cabeçote e nenhuma caixa embutidos (só peça revisada)"));
  check(c.idOf(Kind::amp, 0) == "brit800" && c.entry(Kind::amp, 0)->name == "brit800",
        U("a vaga 0 de cabeçote guarda só o id do Brit 800 (nenhum nome antigo no programa)"));
  // Motores como eram nas tabelas antigas (as vagas guardam o motor também).
  check(NexusProcessor::pedalKind(8) == NexusProcessor::PedalKind::drop && NexusProcessor::pedalKind(7) == NexusProcessor::PedalKind::gate
          && NexusProcessor::pedalKind(22) == NexusProcessor::PedalKind::triple && NexusProcessor::pedalKind(57) == NexusProcessor::PedalKind::geq
          && NexusProcessor::pedalKind(62) == NexusProcessor::PedalKind::comp && NexusProcessor::pedalKind(0) == NexusProcessor::PedalKind::capture,
        "motores dos pedais (Cherry Drop, Marcasite Gate, Blueberry Chain, Selenite EQ, Apple Comp, Greengage Drive)");
}

// Toca alguns blocos de um seno e devolve a força do que saiu sobre a do que
// entrou (0: silêncio; NaN ou infinito: quebrou).
static double playRatio(NexusProcessor& p)
{
  p.prepareToPlay(48000.0, 256);
  juce::AudioBuffer<float> buffer(2, 256);
  juce::MidiBuffer midi;
  double in = 0.0, out = 0.0;
  for (int block = 0; block < 80; ++block)
  {
    for (int i = 0; i < 256; ++i)
    {
      const float x = 0.1f * std::sin(static_cast<float>(block * 256 + i) * 0.0575f);
      buffer.setSample(0, i, x);
      buffer.setSample(1, i, x);
      in += block >= 40 ? x * x : 0.0;
    }
    p.processBlock(buffer, midi);
    if (block >= 40)
      for (int i = 0; i < 256; ++i)
        out += static_cast<double>(buffer.getSample(0, i)) * buffer.getSample(0, i);
  }
  p.releaseResources();
  return std::sqrt(out / in);
}

static void testState()
{
  // Sessão nova: sem cabeçote e sem caixa, colunas vazias, sem cartão.
  {
    auto freshBox = std::make_unique<NexusProcessor>(); // no heap: a pilha do Windows tem 1 MB
    auto& fresh = *freshBox;
    bool empty = true;
    for (int r = 0; r < NexusProcessor::kRigs; ++r)
      for (const auto kind : {Kind::amp, Kind::cab})
        empty = empty && fresh.gearEmpty(kind, r) && fresh.missingGear(kind, r).isEmpty() && !fresh.gearPlays(kind, r);
    check(empty, U("sessão nova: cabeçote e caixa vazios, sem o cartão \"não está instalado\""));
    const double ratio = playRatio(fresh);
    check(std::isfinite(ratio) && ratio > 0.2 && ratio < 5.0, U("sessão nova toca limpo, sem cabeçote e sem caixa (saída/entrada ") + juce::String(ratio, 2) + ")");
    const auto saved = stateOf(fresh);
    auto xml = juce::AudioProcessor::getXmlFromBinary(saved.getData(), static_cast<int>(saved.getSize()));
    check(xml->hasAttribute("cabecote_id") && xml->getStringAttribute("cabecote_id").isEmpty()
            && xml->hasAttribute("gabinete_id") && xml->getStringAttribute("gabinete_id").isEmpty(),
          U("sessão vazia salva o cabeçote e a caixa em branco"));
    auto againBox = std::make_unique<NexusProcessor>();
    auto& again = *againBox;
    setParam(again, "cabecote", 6.0f); // estado anterior qualquer
    again.setStateInformation(saved.getData(), static_cast<int>(saved.getSize()));
    check(again.gearEmpty(Kind::amp, 0) && again.missingGear(Kind::amp, 0).isEmpty() && again.gearEmpty(Kind::cab, 0),
          U("sessão salva vazia abre vazia"));
    // Automação do programa de gravação numa vaga: fica vazio, sem nome nem cartão.
    setParam(again, "cabecote", 6.0f);
    check(again.missingGear(Kind::amp, 0).isEmpty() && !again.gearPlays(Kind::amp, 0),
          U("automação que escolhe uma peça que saiu: lugar vazio, sem cartão, sem cabeçote tocando"));
  }

  // Preset com um pedal de cada motor, cabeçote e caixa fora do começo da lista.
  auto aBox = std::make_unique<NexusProcessor>();
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
          U("o estado guarda o id de cada peça, também a da vaga (p2_visual_id=") + xml->getStringAttribute("p2_visual_id")
            + ", cabecote_id=" + xml->getStringAttribute("cabecote_id") + ")");
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
  check(b.missingGear(Kind::amp, 0).isEmpty() && b.missingGear(Kind::cab, 1).isEmpty() && !b.gearPlays(Kind::amp, 0)
          && !b.gearPlays(Kind::cab, 0) && !b.gearPlays(Kind::amp, 1) && !b.gearPlays(Kind::cab, 1),
        U("cabeçote e caixa que saíram do programa: lugar vazio, sem cartão, e o som passa direto"));
  check(std::isfinite(playRatio(b)), U("preset antigo com peças que saíram toca sem quebrar"));
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
  check(same && c.ampLook(0) == 50 && c.cabLook(1) == 28 && c.missingGear(Kind::amp, 0) == "mark2plus", "preset novo (com id) abre igual");

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
  check(d.missingGear(Kind::pedal, 0) == "fruta_que_nao_existe" && !d.gearPlays(Kind::pedal, 0),
        U("peça não instalada fica marcada para o cartão \"não está instalado\""));
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
  setParam(before, NexusProcessor::pedalParam(1, "visual"), static_cast<float>(catalog.find(Kind::pedal, "agate_wylde")));
  const auto oldState = stateOf(before);
  auto oldXml = juce::AudioProcessor::getXmlFromBinary(oldState.getData(), static_cast<int>(oldState.getSize()));
  oldXml->setAttribute("p1_visual_id", "lemon_drive");
  oldXml->setAttribute("p2_visual_id", "cranberry_wylde");
  juce::MemoryBlock renamed;
  juce::AudioProcessor::copyXmlToBinary(*oldXml, renamed);
  auto eBox = std::make_unique<NexusProcessor>();
  auto& e = *eBox;
  e.setStateInformation(renamed.getData(), static_cast<int>(renamed.getSize()));
  check(e.missingGear(Kind::pedal, 0).isEmpty() && e.missingGear(Kind::pedal, 1).isEmpty()
            && e.pedalLook(0) == catalog.find(Kind::pedal, "greengage_drive") && e.pedalLook(1) == catalog.find(Kind::pedal, "agate_wylde"),
        U("preset com código antigo (lemon_drive, cranberry_wylde) abre com o pedal renomeado"));
  check(e.gearId(Kind::pedal, 0) == "greengage_drive" && e.gearId(Kind::pedal, 1) == "agate_wylde", U("e salva de novo com o código novo"));
}

// Peça mínima numa pasta (ficha, mapa sem botões e frente), como o botão de
// equipamentos monta antes de instalar.
static juce::File stagePiece(const juce::String& id, const juce::String& manifest)
{
  const auto dir = GearCatalog::folder().getSiblingFile("montando").getChildFile(id);
  dir.deleteRecursively();
  dir.createDirectory();
  dir.getChildFile(id + "_manifesto.json").replaceWithText(manifest);
  dir.getChildFile(id + "_mapa.json").replaceWithText("{\"nome\": \"" + id + "\", \"largura\": 64, \"altura\": 32, \"botoes\": []}");
  juce::Image front(juce::Image::ARGB, 64, 32, true);
  front.clear(front.getBounds(), juce::Colours::darkgrey);
  juce::FileOutputStream out(dir.getChildFile(id + "_frente.png"));
  juce::PNGImageFormat().writeImageToStream(front, out);
  return dir;
}

static void testReturn()
{
  // Sessão da beta.2 com o Mark Two Plus e a caixa dele, e uma automação antiga.
  auto& catalog = GearCatalog::instance();
  const int cabLook = catalog.entries(Kind::cab)[16].id == "mark_112" ? 16 : -1;
  check(cabLook == 16 && catalog.find(Kind::amp, "mark2plus") < 0 && catalog.find(Kind::cab, "mark_112") < 0,
        U("antes: Mark Two Plus (vaga 50) e Mark 1x12 (vaga 16) não estão instalados"));
  auto aBox = std::make_unique<NexusProcessor>(); // no heap: a pilha do Windows tem 1 MB
  auto& a = *aBox;
  setParam(a, "cabecote", 50.0f);
  setParam(a, "gabinete", 16.0f);
  const auto saved = stateOf(a);
  auto bBox = std::make_unique<NexusProcessor>();
  auto& b = *bBox;
  b.setStateInformation(saved.getData(), static_cast<int>(saved.getSize()));
  check(!b.gearPlays(Kind::amp, 0) && !b.gearPlays(Kind::cab, 0) && b.gearId(Kind::amp, 0) == "mark2plus",
        U("a sessão abre sem as duas peças, guardando o id delas"));
  check(!nexus::t3k::defaultTone("mark2plus", 0) && !nexus::t3k::defaultTone("topboost30", 0),
        U("nenhuma recomendação de captura para cabeçote que saiu"));

  // As recomendadas voltam na ficha da peça revisada (como o programa as lê: ficha antes do embutido).
  nexus::t3k::setRecommendedLookup([](const juce::String& id) -> juce::var {
    for (auto kind : {Kind::pedal, Kind::amp, Kind::cab})
      if (const auto e = GearCatalog::instance().entry(kind, GearCatalog::instance().find(kind, id)))
        return e->recommended();
    return {};
  });
  const auto reviewed = juce::JSON::parse(gRepo.getChildFile("tools/gear_studio/recomendadas_em_revisao.json"))["mark2plus"];
  check(reviewed.isObject(), U("as recomendadas do Mark Two Plus esperam a revisão em tools/gear_studio"));
  auto* manifest = new juce::DynamicObject();
  for (const auto& [key, value] : std::initializer_list<std::pair<const char*, juce::var>>{
         {"formato", 1}, {"id", "mark2plus"}, {"versao", "1.1.0"}, {"tipo", "cabecote"}, {"nome", "Mark Two Plus"},
         {"versao_minima_do_programa", "0.2.0"}, {"motor", "captura"}, {"indice_de_fabrica", 50},
         {"caixa_padrao", "mark_112"}, {"equalizador_grafico", true}})
    manifest->setProperty(key, value);
  auto* captures = new juce::DynamicObject();
  captures->setProperty("origem", "tone3000");
  captures->setProperty("recomendadas", reviewed);
  manifest->setProperty("capturas", juce::var(captures));
  const auto amp = stagePiece("mark2plus", juce::JSON::toString(juce::var(manifest)));
  const auto cab = stagePiece("mark_112", R"({"formato": 1, "id": "mark_112", "versao": "1.1.0", "tipo": "caixa", "nome": "Mark 1x12",
    "versao_minima_do_programa": "0.2.0", "motor": "caixa", "indice_de_fabrica": 16})");
  const auto e1 = catalog.installFolder(amp, "mark2plus", juce::var(new juce::DynamicObject()));
  const auto e2 = catalog.installFolder(cab, "mark_112", juce::var(new juce::DynamicObject()));
  check(e1.isEmpty() && e2.isEmpty(), U("as duas peças revisadas instalam pelo botão ") + e1 + e2);
  check(catalog.find(Kind::amp, "mark2plus") == 50 && catalog.find(Kind::cab, "mark_112") == 16 && catalog.presentAt(Kind::amp, 50)
          && catalog.count(Kind::amp) == NexusProcessor::kAmpLooks && catalog.count(Kind::cab) == NexusProcessor::kCabLooks,
        U("voltam para a mesma posição de parâmetro (cabeçote 50, caixa 16), sem crescer o catálogo"));
  const auto entry = catalog.entry(Kind::amp, 50);
  check(entry && entry->cabId == "mark_112" && catalog.find(Kind::cab, entry->cabId) == 16 && NexusProcessor::hasGraphicEq(50),
        U("o cabeçote abre com a caixa dele e tem o equalizador gráfico"));
  check(nexus::t3k::defaultTone("mark2plus", 0).has_value(), U("as capturas recomendadas vêm com a ficha do cabeçote"));
  b.gearInstalled();
  check(b.missingGear(Kind::amp, 0).isEmpty() && b.missingGear(Kind::cab, 0).isEmpty() && b.ampLook(0) == 50 && b.cabLook(0) == 16
          && b.gearPlays(Kind::amp, 0) && b.gearPlays(Kind::cab, 0),
        U("a sessão que esperava passa a tocar as duas peças, sem o cartão"));
  check(juce::roundToInt(b.params.getRawParameterValue("cabecote")->load()) == 50, U("o parâmetro continua no 50: a automação antiga vale"));
  // Automação de antes da peça sair: 50/93 escolhe o Mark Two Plus.
  auto cBox = std::make_unique<NexusProcessor>();
  auto& c = *cBox;
  c.params.getParameter("cabecote_b")->setValueNotifyingHost(50.0f / 93.0f);
  check(c.ampLook(1) == 50 && c.gearPlays(Kind::amp, 1) && c.gearId(Kind::amp, 1) == "mark2plus", U("automação 50/93 escolhe o Mark Two Plus de novo"));
  check(std::isfinite(playRatio(b)), U("toca com a peça que voltou"));
  // Sessão nova continua vazia: a peça que voltou não entra sozinha.
  auto dBox = std::make_unique<NexusProcessor>();
  check(dBox->gearEmpty(Kind::amp, 0) && dBox->gearEmpty(Kind::cab, 0), U("sessão nova continua sem cabeçote (a vaga 0 segue vazia)"));
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

static void testFreshStart(const juce::File& temp)
{
  // Uma instalação antiga: dados, preferências do programa independente,
  // capturas nos Documentos e a biblioteca do estúdio.
  const auto data = temp.getChildFile("Suporte").getChildFile("Nexus Modeler");
  const auto docs = temp.getChildFile("Documentos").getChildFile("Nexus Modeler");
  for (const auto& dir : {data.getChildFile("Presets"), docs.getChildFile("Top Boost 30").getChildFile("26584 clean"), docs.getChildFile(U("Estúdio"))})
    dir.createDirectory();
  data.getChildFile("Equipamentos").getChildFile("brit800").getChildFile("brit800_frente.png").create();
  data.getChildFile("Presets").getChildFile("Meu som.nexuspreset").replaceWithText("x");
  data.getChildFile("tone3000.settings").replaceWithText("conta");
  {
    juce::PropertiesFile::Options o;
    juce::PropertiesFile prefs(data.getChildFile("preferencias.settings"), o);
    prefs.setValue("idioma", "ja");
    prefs.setValue("outra_coisa", "antiga");
  }
  data.getSiblingFile("Nexus Modeler.settings").replaceWithText("interface");
  docs.getChildFile("Top Boost 30").getChildFile("26584 clean").getChildFile("pack.json").replaceWithText("{}");
  docs.getChildFile(U("Estúdio")).getChildFile(U("música.wav")).replaceWithText("som");
  setenv("NEXUS_DADOS_PASTA", data.getFullPathName().toRawUTF8(), 1);
  setenv("NEXUS_DOCUMENTOS_PASTA", docs.getFullPathName().toRawUTF8(), 1);

  const auto reserve = nexus::update::startFresh();
  check(reserve.isDirectory() && reserve.getParentDirectory().getFileName() == U("Nexus Modeler (versões antigas)"),
        U("os dados antigos vão para a reserva ao lado (") + reserve.getFullPathName() + ")");
  check(reserve.getChildFile("Dados/Presets/Meu som.nexuspreset").existsAsFile() && reserve.getChildFile("Dados/tone3000.settings").existsAsFile()
          && reserve.getChildFile("Dados/Equipamentos/brit800/brit800_frente.png").existsAsFile()
          && reserve.getChildFile("Nexus Modeler.settings").existsAsFile()
          && reserve.getChildFile("Documentos/Top Boost 30/26584 clean/pack.json").existsAsFile(),
        U("nada foi apagado: presets, preferências, equipamentos e capturas estão na reserva"));
  check(!data.getChildFile("Presets").exists() && !data.getChildFile("Equipamentos").exists() && !data.getSiblingFile("Nexus Modeler.settings").exists()
          && !docs.getChildFile("Top Boost 30").exists(),
        U("o programa não vê mais nada antigo"));
  check(docs.getChildFile(U("Estúdio")).getChildFile(U("música.wav")).existsAsFile(), U("a biblioteca do estúdio (do usuário) fica onde está"));
  {
    juce::PropertiesFile::Options o;
    juce::PropertiesFile prefs(data.getChildFile("preferencias.settings"), o);
    check(prefs.getValue("idioma") == "ja" && !prefs.containsKey("outra_coisa"), U("só o idioma da tela continua nas preferências"));
  }
  // Segunda abertura: não mexe mais (o que a beta.3 criou fica).
  data.getChildFile("Presets").getChildFile("Novo.nexuspreset").create();
  check(nexus::update::startFresh() == juce::File() && data.getChildFile("Presets").getChildFile("Novo.nexuspreset").existsAsFile(),
        U("só na primeira abertura: o que a versão nova criou fica"));
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
  gRepo = juce::File(U(argv[1])).getParentDirectory().getParentDirectory();
  testCatalog(juce::File(U(argv[1])));
  testState();
  testReturn();
  testFreshStart(temp);
  if (argc > 2)
    testPackage(juce::File(U(argv[2])));
  temp.deleteRecursively();
  std::printf(failures == 0 ? "tudo certo\n" : "%d falhas\n", failures);
  return failures == 0 ? 0 : 1;
}
