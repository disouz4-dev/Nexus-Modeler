// Teste da conexão com a TONE3000 de ponta a ponta, na conta de verdade:
// login pelo navegador, listas, download de um tom recomendado e o pacote
// baixado tocando no motor. Manual: precisa de internet e de alguém para
// aprovar o login no navegador na primeira vez.
//
// Uso: nexus_t3k_test [número do tom recomendado; padrão: o primeiro do Brit 800]

#include <cstdio>

#include "../source/tone3000/Tone3000.h"
#include "nexus/Pack.h"

namespace
{
int gFailures = 0;

void report(bool ok, const juce::String& what, const juce::String& detail)
{
  std::printf("  [%s] %s\n         %s\n", ok ? " OK  " : "FALHA", what.toRawUTF8(), detail.toRawUTF8());
  if (!ok)
    ++gFailures;
}

// Espera a resposta rodando o laço de mensagens (até "seconds").
template <typename Done>
void waitFor(Done&& done, int seconds)
{
  for (int t = 0; t < seconds * 10 && !done(); ++t)
    juce::MessageManager::getInstance()->runDispatchLoopUntil(100);
}
} // namespace

int main(int argc, char** argv)
{
  juce::ScopedJuceInitialiser_GUI gui;
  nexus::t3k::Connection t3k;
  std::printf("Nexus Modeler — teste da TONE3000\n");
  report(t3k.hasKey(), "O programa tem a chave pública da TONE3000", t3k.hasKey() ? "sim" : "não");
  if (!t3k.hasKey())
    return 1;

  // 1. Login (só abre o navegador se ainda não houver conta salva).
  if (!t3k.signedIn())
  {
    std::printf("\nAbrindo o navegador para entrar na TONE3000...\n");
    bool finished = false;
    juce::String error;
    t3k.signIn([&](juce::String e) {
      error = e;
      finished = true;
    });
    waitFor([&] { return finished; }, 300);
    report(finished && error.isEmpty(), "Login pelo navegador", finished ? (error.isEmpty() ? "conectado" : error) : "tempo esgotado");
  }
  report(t3k.signedIn(), "Conta salva", "@" + t3k.userName());
  if (!t3k.signedIn())
    return 1;

  // 2. Listas.
  auto list = [&](nexus::t3k::Connection::List which, const char* gear, const char* word, const char* name) {
    bool finished = false;
    std::vector<nexus::t3k::Tone> tones;
    juce::String error;
    t3k.list(which, gear, word, [&](std::vector<nexus::t3k::Tone> t, juce::String e) {
      tones = std::move(t);
      error = e;
      finished = true;
    });
    waitFor([&] { return finished; }, 60);
    juce::String detail = error.isNotEmpty() ? error : juce::String(static_cast<int>(tones.size())) + " tons";
    if (!tones.empty())
      detail << ": " << tones.front().title << " (@" << tones.front().creator << ")";
    report(finished && error.isEmpty(), name, detail);
  };
  std::printf("\nListas\n");
  list(nexus::t3k::Connection::List::trending, "amp", "", "Em alta (cabeçotes)");
  list(nexus::t3k::Connection::List::favorites, "amp", "JCM800", "Seus favoritos de JCM800");
  list(nexus::t3k::Connection::List::downloaded, "pedal", "", "Seus downloads (pedais)");

  // 3. Download de um tom recomendado.
  auto recommended = nexus::t3k::recommended("brit800");
  nexus::t3k::Tone tone = recommended.front();
  if (argc > 1)
    for (const auto& r : recommended)
      if (r.id == juce::String(argv[1]).getIntValue())
        tone = r;
  std::printf("\nDownload: %s (%d)\n", tone.title.toRawUTF8(), tone.id);
  bool finished = false;
  juce::File folder;
  juce::String error;
  t3k.download(
    tone, "amp_amp-cab", [](juce::String s) { std::printf("  ... %s\n", s.toRawUTF8()); },
    [&](juce::File f, juce::String e) {
      folder = f;
      error = e;
      finished = true;
    });
  waitFor([&] { return finished; }, 600);
  report(finished && error.isEmpty() && folder.isDirectory(), "Tom baixado e pacote montado",
         finished ? (error.isEmpty() ? folder.getFullPathName() : error) : "tempo esgotado");
  if (!folder.isDirectory())
    return 1;

  // 4. O pacote toca no motor, uma captura por posição do botão.
  try
  {
    nexus::PackOptions options;
    const auto pack = nexus::loadPack(folder.getFullPathName().toStdString(), options);
    juce::StringArray positions;
    for (const auto& c : pack->captures)
      positions.add(juce::String(c.position, 1));
    report(pack->captures.size() >= 3, "O motor abre o pacote baixado",
           juce::String(static_cast<int>(pack->captures.size())) + " capturas, ganho " + positions.joinIntoString(" "));
  }
  catch (const std::exception& e)
  {
    report(false, "O motor abre o pacote baixado", e.what());
  }

  std::printf("\n%s (%d falha%s)\n", gFailures ? "REPROVADO" : "APROVADO", gFailures, gFailures == 1 ? "" : "s");
  return gFailures ? 1 : 0;
}
