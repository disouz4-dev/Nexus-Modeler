// Teste sem janela do catálogo de equipamentos (gear/GearCatalog) e do id em
// texto no estado (NexusProcessor::getStateInformation).
//   nexus_gear_test <pasta assets/gear>
// Confere que o catálogo tem as mesmas peças, na mesma ordem e com os mesmos
// dados das fichas embutidas (que vieram das tabelas antigas: manifestos.py --conferir).
#include <cstdio>
#include <cstdlib>

#include "../source/PluginProcessor.h"

using nexus::gear::GearCatalog;
using nexus::gear::Kind;

static juce::String U(const char* text) { return juce::String::fromUTF8(text); }

static int failures = 0;
static void check(bool ok, const juce::String& what)
{
  std::printf("%s %s\n", ok ? "ok    " : "FALHOU", what.toRawUTF8());
  failures += ok ? 0 : 1;
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
        "motores dos pedais (Cherry Drop, Pear Gate, Blueberry Chain, Acerola EQ, Apple Comp, Lemon Drive)");
  bool eq = true;
  for (int i = 0; i < NexusProcessor::kAmpLooks; ++i)
    eq = eq && NexusProcessor::hasGraphicEq(i) == (i == 1 || (i >= 50 && i <= 53));
  check(eq, U("equalizador gráfico só no Mark Five e nos Mark 2+, 3, 4 e JP"));
  const auto brit = c.entry(Kind::amp, 0);
  check(brit && brit->cabId == "brit412" && c.find(Kind::cab, "brit412") == 0, U("Brit 800 abre com a Brit 4x12"));
}

int main(int argc, char** argv)
{
  juce::ScopedJuceInitialiser_GUI gui;
  if (argc < 2)
  {
    std::puts("uso: nexus_gear_test <pasta assets/gear>");
    return 2;
  }
  // Pasta vazia no lugar da de equipamentos baixados: o teste não depende do Mac.
  const auto temp = juce::File::getSpecialLocation(juce::File::tempDirectory).getChildFile("nexus_gear_test");
  temp.deleteRecursively();
  temp.getChildFile("Equipamentos").createDirectory();
  setenv("NEXUS_EQUIPAMENTOS_PASTA", temp.getChildFile("Equipamentos").getFullPathName().toRawUTF8(), 1);
  testCatalog(juce::File(U(argv[1])));
  temp.deleteRecursively();
  std::printf(failures == 0 ? "tudo certo\n" : "%d falhas\n", failures);
  return failures == 0 ? 0 : 1;
}
