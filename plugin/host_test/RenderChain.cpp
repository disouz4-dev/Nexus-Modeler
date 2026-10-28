// Passa uma gravação de guitarra pela cadeia do Nexus, com material da
// TONE3000, e grava o resultado para ouvir:
//   Banana Overdrive (captura do SD-1) -> Brit 800 (captura do JCM800) -> IR.
// Com "-" no lugar do IR, grava só o Brit 800 (para comparar com o oficial).
//
// Baixa as capturas pela conta conectada (o mesmo caminho do botão
// "Carregar" do programa, para Documentos/Nexus Modeler/<equipamento>) e usa o
// mesmo processador do plugin.
//
// Usa as gravações do "Testar o som" (uma por estilo, cada uma com o ajuste
// que combina com ela). O primeiro argumento fica para comparar a guitarra.
//
// Uso: nexus_render <guitarra.wav> <ir.wav> <pasta_de_saida> [ganho_de_entrada_dB]

#include <cmath>
#include <cstdio>

#include "../../tools/Wav.h"
#include "../source/PluginProcessor.h"
#include "../source/tone3000/Tone3000.h"

namespace
{
template <typename Done>
void waitFor(Done&& done, int seconds)
{
  for (int i = 0; i < seconds * 10 && !done(); ++i)
    juce::MessageManager::getInstance()->runDispatchLoopUntil(100);
}

void setParam(NexusProcessor& p, const char* id, float plain)
{
  auto* param = p.params.getParameter(id);
  param->setValueNotifyingHost(param->convertTo0to1(plain));
}

void settle(NexusProcessor& p, int blocks)
{
  juce::AudioBuffer<float> buffer(2, 128);
  juce::MidiBuffer midi;
  for (int i = 0; i < blocks; ++i)
  {
    buffer.clear();
    p.processBlock(buffer, midi);
    juce::Thread::sleep(1);
  }
}

// Baixa um tom recomendado e devolve a pasta do pacote.
juce::File fetch(nexus::t3k::Connection& t3k, const char* prefix, int id, const juce::String& gearName, const char* filter,
                 const juce::String& packName = {})
{
  nexus::t3k::Tone tone;
  for (const auto& r : nexus::t3k::recommended(prefix))
    if (r.id == id && (packName.isEmpty() || r.packName == packName))
      tone = r;
  bool finished = false;
  juce::File folder;
  juce::String error;
  t3k.download(
    tone, filter, gearName, [](juce::String s) { std::printf("  ... %s\n", s.toRawUTF8()); },
    [&](juce::File f, juce::String e) {
      folder = f;
      error = e;
      finished = true;
    });
  waitFor([&] { return finished; }, 600);
  if (!error.isEmpty() || !folder.isDirectory())
    std::printf("  ERRO ao baixar %s: %s\n", tone.title.toRawUTF8(), error.toRawUTF8());
  return folder;
}

struct Setting
{
  const char* file;
  const char* description;
  bool pedal;
  float drive, tone, level;
  float gain, bass, middle, treble, presence, volume;
};
} // namespace

int main(int argc, char** argv)
{
  if (argc < 4)
  {
    std::printf("Uso: nexus_render <guitarra.wav> <ir.wav> <pasta_de_saida> [ganho_de_entrada_dB]\n");
    return 2;
  }
  juce::ScopedJuceInitialiser_GUI juce;
  const auto cwd = juce::File::getCurrentWorkingDirectory();
  const auto input = cwd.getChildFile(juce::String::fromUTF8(argv[1]));
  const auto ir = cwd.getChildFile(juce::String::fromUTF8(argv[2]));
  const auto out = cwd.getChildFile(juce::String::fromUTF8(argv[3]));
  const double inputDb = argc > 4 ? std::atof(argv[4]) : 0.0;
  out.createDirectory();

  double rate = 48000.0;
  auto dry = wav::read(input.getFullPathName().toStdString(), rate);
  double peak = 0.0;
  for (auto& v : dry)
  {
    v *= std::pow(10.0, inputDb / 20.0);
    peak = std::max(peak, std::abs(v));
  }
  std::printf("Guitarra: %.1f s a %.0f Hz, pico %.1f dBFS\n", dry.size() / rate, rate, 20.0 * std::log10(peak + 1e-12));

  nexus::t3k::Connection t3k;
  if (!t3k.signedIn())
  {
    std::printf("Conecte a conta da TONE3000 no Nexus antes.\n");
    return 1;
  }
  // NEXUS_BAIXAR=<id>,<id>,...: só baixa esses tons (cabeçotes) para
  // Documentos/Nexus Modeler/Teste de capturas e termina (para comparar pacotes).
  if (const auto ids = juce::SystemStats::getEnvironmentVariable("NEXUS_BAIXAR", {}); ids.isNotEmpty())
  {
    for (const auto& idText : juce::StringArray::fromTokens(ids, ",", {}))
    {
      nexus::t3k::Tone tone;
      tone.id = idText.getIntValue();
      bool finished = false;
      juce::File folder;
      juce::String error;
      t3k.download(
        tone, juce::SystemStats::getEnvironmentVariable("NEXUS_BAIXAR_FILTRO", "amp_amp-cab"), "Teste de capturas", [](juce::String) {},
        [&](juce::File f, juce::String e) {
          folder = f;
          error = e;
          finished = true;
        });
      waitFor([&] { return finished; }, 600);
      std::printf("%d: %s %s\n", tone.id, folder.getFullPathName().toRawUTF8(), error.toRawUTF8());
    }
    return 0;
  }
  // NEXUS_CONFERIR=<pasta>: baixa cada recomendada (todas as peças) para
  // <pasta>/<prefixo> e confere se as capturas entram com a posição (sem
  // "mapear"). Mostra uma linha por recomendada e o total de problemas.
  if (const auto dir = juce::SystemStats::getEnvironmentVariable("NEXUS_CONFERIR", {}); dir.isNotEmpty())
  {
    // Lista das peças pelo arquivo do projeto (as recomendadas em si vêm do programa).
    const auto all = juce::JSON::parse(juce::File::getCurrentWorkingDirectory().getChildFile("assets/tone3000/recomendadas.json"));
    int problems = 0, total = 0;
    if (auto* obj = all.getDynamicObject())
      for (const auto& prop : obj->getProperties())
      {
        const auto prefix = prop.name.toString();
        // NEXUS_CONFERIR_SO=prefixo,prefixo: só essas peças.
        const auto only = juce::StringArray::fromTokens(juce::SystemStats::getEnvironmentVariable("NEXUS_CONFERIR_SO", {}), ",", "");
        if (prefix.startsWith("_") || (!only.isEmpty() && !only.contains(prefix)))
          continue;
        for (const auto& tone : nexus::t3k::recommended(prefix))
        {
          ++total;
          const juce::String filter = tone.gear == "cab" ? "cab" : tone.gear == "pedal" ? "pedal" : "amp_amp-cab";
          bool finished = false;
          juce::File folder;
          juce::String error;
          t3k.download(tone, filter, "Conferir " + prefix, [](juce::String) {}, [&](juce::File f, juce::String e) {
            folder = f;
            error = e;
            finished = true;
          });
          waitFor([&] { return finished; }, 600);
          const auto pack = juce::JSON::parse(folder.getChildFile("pack.json"));
          const int count = pack["capturas"].getDynamicObject() != nullptr ? pack["capturas"].getDynamicObject()->getProperties().size() : 0;
          const int irs = folder.getNumberOfChildFiles(juce::File::findFiles, "*.wav");
          const bool ok = error.isEmpty() && !static_cast<bool>(pack["mapear"]) && (count > 0 || irs > 0);
          problems += ok ? 0 : 1;
          std::printf("%s %-14s %6d %-28s %s %s\n", ok ? "ok   " : "FALHA", prefix.toRawUTF8(), tone.id, tone.packName.substring(0, 28).toRawUTF8(),
                      filter == "cab" ? (juce::String(irs) + " IRs").toRawUTF8() : (juce::String(count) + " capturas").toRawUTF8(), error.toRawUTF8());
          if (!ok) // nomes das capturas, para escrever a regra da posição
            for (const auto& f : folder.findChildFiles(juce::File::findFiles, true, "*.nam"))
              std::printf("        %s\n", f.getFileName().toRawUTF8());
          folder.deleteRecursively();
        }
      }
    std::printf("%d recomendadas, %d com problema\n", total, problems);
    return problems == 0 ? 0 : 1;
  }
  // NEXUS_BUSCAR=<palavra>:<tipo>: a busca "mais curtidas" (o que a escolha
  // automática usa) e termina.
  if (const auto spec = juce::SystemStats::getEnvironmentVariable("NEXUS_BUSCAR", {}); spec.isNotEmpty())
  {
    bool finished = false;
    t3k.list(nexus::t3k::Connection::List::mostLiked, spec.fromLastOccurrenceOf(":", false, false), spec.upToLastOccurrenceOf(":", false, false),
             [&](std::vector<nexus::t3k::Tone> tones, juce::String error) {
               std::printf("%s: %d tons %s\n", spec.toRawUTF8(), static_cast<int>(tones.size()), error.toRawUTF8());
               for (size_t i = 0; i < std::min<size_t>(6, tones.size()); ++i)
                 std::printf("  %6d favoritos %7d downloads %3d capturas  %d %s\n", tones[i].favorites, tones[i].downloads, tones[i].models,
                             tones[i].id, tones[i].title.toRawUTF8());
               finished = true;
             });
    waitFor([&] { return finished; }, 60);
    return 0;
  }
  // NEXUS_BAIXAR_RECOMENDADA=<prefixo>:<id>:<pacote>:<equipamento>: baixa uma
  // recomendada como o Nexus baixa (mapa e nome do pacote) e termina.
  if (const auto spec = juce::SystemStats::getEnvironmentVariable("NEXUS_BAIXAR_RECOMENDADA", {}); spec.isNotEmpty())
  {
    const auto parts = juce::StringArray::fromTokens(spec, ":", {});
    const auto folder = fetch(t3k, parts[0].toRawUTF8(), parts[1].getIntValue(), parts[3], "amp_amp-cab", parts[2]);
    std::printf("pasta: %s\n", folder.getFullPathName().toRawUTF8());
    for (const auto& f : folder.findChildFiles(juce::File::findFiles, false))
      std::printf("  %s\n", f.getFileName().toRawUTF8());
    return folder.isDirectory() ? 0 : 1;
  }
  std::printf("\nBaixando da TONE3000\n");
  // IR "t3k": a caixa também vem da TONE3000 (o IR indicado do pacote Greenback).
  auto irFile = ir;
  if (std::string(argv[2]) == "t3k")
  {
    const auto cab = fetch(t3k, "brit412", 75087, "Brit 4x12", "cab");
    const auto preferred = juce::JSON::parse(cab.getChildFile("pack.json"))["ir_padrao"].toString();
    irFile = cab.getChildFile(preferred + ".wav");
    std::printf("  caixa: %d IRs, usando %s\n", cab.getNumberOfChildFiles(juce::File::findFiles, "*.wav"), irFile.getFileName().toRawUTF8());
    if (!irFile.existsAsFile())
      return 1;
  }
  const auto banana = fetch(t3k, "banana_overdrive", 82811, "Banana Overdrive", "pedal");
  const auto brit = fetch(t3k, "brit800", 1071, "Brit 800", "amp_amp-cab");
  if (!banana.isDirectory() || !brit.isDirectory())
    return 1;

  // NEXUS_MARK5=1: Mark Five no canal 3 (MK-IIC+), equalizador gráfico em "V".
  if (juce::SystemStats::getEnvironmentVariable("NEXUS_MARK5", {}) == "1")
  {
    const auto mark = fetch(t3k, "mark5", 57410, "Mark Five", "amp_amp-cab", "Canal 3 MK-IIC+");
    if (!mark.isDirectory())
      return 1;
    std::printf("  pacote: %s\n", mark.getFileName().toRawUTF8());
    for (int v = 0; v < 2; ++v)
    {
      NexusProcessor p;
      p.setPlayConfigDetails(2, 2, rate, 128);
      p.prepareToPlay(rate, 128);
      p.setLayout(ChainLayout{});
      setParam(p, "cabecote", 1.0f);
      setParam(p, "canal", 2.0f);
      setParam(p, "ganho_3", 7.0f);
      // A curva em "V" do Mark: graves e agudos para cima, 750 Hz para baixo.
      const float curve[5] = {3.0f, 1.0f, -5.0f, 1.5f, 3.0f};
      const char* ids[5] = {"eq80", "eq240", "eq750", "eq2200", "eq6600"};
      for (int k = 0; k < 5; ++k)
        setParam(p, ids[k], v == 1 ? curve[k] : 0.0f);
      p.loadAmpPack(0, 2, mark);
      p.loadIrAsync(0, irFile);
      for (int t = 0; t < 2400 && !(p.channelLoaded(0, 2) && p.irLoaded(0)); ++t)
        settle(p, 4);
      setParam(p, "caixa", 1.0f);
      settle(p, 400);
      auto& test = p.testSource();
      test.setStyle(nexus::TestSource::heavy);
      test.setPlaying(true);
      std::vector<double> result;
      juce::AudioBuffer<float> buffer(2, 128);
      juce::MidiBuffer midi;
      while (result.size() < static_cast<size_t>(16.0 * rate))
      {
        buffer.clear();
        p.processBlock(buffer, midi);
        for (int i = 0; i < 128; ++i)
          result.push_back(buffer.getSample(0, i));
      }
      double m = 0.0;
      for (auto x : result)
        m = std::max(m, std::abs(x));
      for (auto& x : result)
        x *= m > 0.0 ? std::pow(10.0, -1.0 / 20.0) / m : 1.0;
      const auto file = out.getChildFile(v == 1 ? "mark5_canal3_iic_eq_em_v.wav" : "mark5_canal3_iic_eq_plano.wav");
      wav::write(file.getFullPathName().toStdString(), result, rate);
      std::printf("Gravado: %s\n", file.getFileName().toRawUTF8());
    }
    return 0;
  }

  // IR "-": só o Brit 800 no ganho 5, botões na posição da captura, com a
  // guitarra do primeiro argumento. Serve para comparar com o "render" do
  // NeuralAmpModelerCore (a mesma captura tocada pelo código oficial).
  if (std::string(argv[2]) == "-")
  {
    NexusProcessor p;
    p.setPlayConfigDetails(2, 2, rate, 128);
    p.prepareToPlay(rate, 128);
    p.setLayout(ChainLayout{});
    setParam(p, "ganho", 5.0f);
    p.loadAmpPack(0, 0, brit);
    for (int t = 0; t < 2400 && !p.channelLoaded(0, 0); ++t)
      settle(p, 4);
    settle(p, 400);
    std::vector<double> result;
    juce::AudioBuffer<float> buffer(2, 128);
    juce::MidiBuffer midi;
    for (size_t start = 0; start + 128 <= dry.size(); start += 128)
    {
      for (int i = 0; i < 128; ++i)
        buffer.setSample(0, i, static_cast<float>(dry[start + static_cast<size_t>(i)]));
      buffer.clear(1, 0, 128);
      p.processBlock(buffer, midi);
      for (int i = 0; i < 128; ++i)
        result.push_back(buffer.getSample(0, i));
    }
    wav::write(out.getChildFile("so_amp_g5.wav").getFullPathName().toStdString(), result, rate);
    std::printf("Gravado: so_amp_g5.wav\n");
    return 0;
  }

  // Cada estilo do "Testar o som" com o ajuste que combina com ele.
  struct ByStyle
  {
    nexus::TestSource::Style style;
    Setting s;
  };
  const ByStyle styles[] = {
    {nexus::TestSource::clean, {"1_limpo_brit800_ganho2.wav", "Limpo: Brit 800 no ganho 2, sem pedal", false, 0, 5, 5, 2, 5, 5, 6, 5, 5}},
    {nexus::TestSource::blues, {"2_blues_banana2_brit800_ganho5.wav", "Blues: Banana drive 2 e Brit 800 no ganho 5", true, 2, 5, 6, 5, 5, 6, 6, 5, 5}},
    {nexus::TestSource::rock, {"3_rock_banana4_brit800_ganho7.wav", "Rock: Banana drive 4 e Brit 800 no ganho 7", true, 4, 5, 7, 7, 5, 6, 6, 6, 5}},
    {nexus::TestSource::heavy, {"4_pesado_banana6_brit800_ganho10.wav", "Pesado: Banana drive 6 empurrando o Brit 800 no ganho 10", true, 6, 6, 7, 10, 5, 7, 6, 6, 5}},
  };
  (void) dry;

  for (const auto& [style, s] : styles)
  {
    NexusProcessor p;
    p.setPlayConfigDetails(2, 2, rate, 128);
    p.prepareToPlay(rate, 128);
    ChainLayout layout;
    if (s.pedal)
      layout.pre = {0};
    p.setLayout(layout);
    setParam(p, "p1_visual", 2.0f); // Banana Overdrive
    setParam(p, "p1_drive", s.drive);
    setParam(p, "p1_tom", s.tone);
    setParam(p, "p1_nivel", s.level);
    setParam(p, "ganho", s.gain);
    setParam(p, "grave", s.bass);
    setParam(p, "medio", s.middle);
    setParam(p, "agudo", s.treble);
    setParam(p, "presenca", s.presence);
    p.loadAmpPack(0, 0, brit);
    if (s.pedal)
      p.loadPedalPack(0, banana);
    p.loadIrAsync(0, irFile);
    for (int t = 0; t < 2400 && !(p.channelLoaded(0, 0) && (!s.pedal || p.slotLoaded(0)) && p.irLoaded(0)); ++t)
      settle(p, 4);
    setParam(p, "caixa", 1.0f);
    settle(p, 400);

    // O mesmo "Testar o som" do programa: a gravação entra no lugar da guitarra.
    auto& test = p.testSource();
    test.setStyle(style);
    test.setPlaying(true);
    std::vector<double> result;
    const int total = static_cast<int>(16.0 * rate);
    juce::AudioBuffer<float> buffer(2, 128);
    juce::MidiBuffer midi;
    while (static_cast<int>(result.size()) < total)
    {
      buffer.clear();
      p.processBlock(buffer, midi);
      for (int i = 0; i < 128; ++i)
        result.push_back(buffer.getSample(0, i));
    }
    double m = 0.0;
    for (auto v : result)
      m = std::max(m, std::abs(v));
    const double k = m > 0.0 ? std::pow(10.0, -1.0 / 20.0) / m : 1.0;
    for (auto& v : result)
      v *= k;
    const auto file = out.getChildFile(s.file);
    wav::write(file.getFullPathName().toStdString(), result, rate);
    std::printf("Gravado: %s  (%s)\n", file.getFileName().toRawUTF8(), s.description);
  }
  return 0;
}
