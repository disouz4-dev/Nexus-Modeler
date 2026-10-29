// Teste dos equipamentos novos sem reinstalar (update/ArteNova): com um
// "GitHub" de mentira numa pasta (file://), confere que o Nexus acha as peças
// mudadas, baixa só elas para a pasta Arte, não acha de novo depois de baixar
// e passa a carregar a arte baixada no lugar da embutida.
//   nexus_arte_test <pasta com indice.json e os arquivos> <peça> <nome esperado>
#include <juce_gui_basics/juce_gui_basics.h>

#include "../source/ui/Gear.h"
#include "../source/update/ArteNova.h"

using nexus::update::ArteNova;

static bool wait(ArteNova& a, std::initializer_list<ArteNova::State> until)
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

int main(int argc, char** argv)
{
  juce::ScopedJuceInitialiser_GUI gui;
  if (argc < 4)
  {
    std::puts("uso: nexus_arte_test <pasta remota> <peça> <nome esperado>");
    return 2;
  }
  setenv("NEXUS_ARTE_URL", ("file://" + juce::String(argv[1])).toRawUTF8(), 1);
  const juce::String id(argv[2]), expected = juce::String::fromUTF8(argv[3]);
  ArteNova::folder().deleteRecursively();
  int failures = 0;
  auto check = [&](bool ok, const juce::String& what) {
    std::printf("%s %s\n", ok ? "ok  " : "FALHOU", what.toRawUTF8());
    failures += ok ? 0 : 1;
  };

  {
    ArteNova a;
    a.checkIfDue();
    check(wait(a, {ArteNova::State::available, ArteNova::State::upToDate}), "verificou o índice");
    check(a.state() == ArteNova::State::available, "achou equipamentos novos: " + a.names().joinIntoString(", "));
    a.download();
    check(wait(a, {ArteNova::State::ready, ArteNova::State::failed}) && a.state() == ArteNova::State::ready, "baixou " + a.error());
    check(ArteNova::folder().getChildFile(id).getChildFile(id + "_mapa.json").existsAsFile(), "a peça ficou em Equipamentos/<id>");
  }
  {
    ArteNova b;
    b.checkIfDue();
    wait(b, {ArteNova::State::available, ArteNova::State::upToDate});
    check(b.state() == ArteNova::State::upToDate, "depois de baixar, não oferece de novo");
  }
  const auto art = nexus::ui::GearArt::load(id);
  check(art.name == expected, "a tela carrega a arte baixada (nome: " + art.name + ")");
  check(art.front.isValid() && !art.knobs.empty(), "frente e botões carregados da pasta");
  ArteNova::folder().deleteRecursively();
  std::printf(failures == 0 ? "tudo certo\n" : "%d falhas\n", failures);
  return failures == 0 ? 0 : 1;
}
