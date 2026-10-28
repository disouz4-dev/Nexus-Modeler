#include <cstdlib>
// Mede quanto o áudio do Nexus pesa com pacotes reais (Banana Overdrive no
// pedal, Brit 800 no cabeçote e um IR), bloco a bloco, como a placa de som
// pediria: 48 kHz, bloco de 128 (2,67 ms para cada bloco ficar pronto).
//
// Cenários: parado; girando o drive sem a mão no botão (automação); girando
// com a mão no botão (aquecimento antecipado ligado); e os mesmos em A2 Lite.
//
// Uso: nexus_perfil <pasta do pedal> <pasta do cabeçote> <ir.wav> [guitarra.wav]

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <thread>

#include "../../tools/Wav.h"
#include "../source/PluginProcessor.h"
#include "nexus/TestSignal.h"
#include "nexus/Pack.h"
#include "../source/report/Report.h"
#include "../source/fx/PitchDrop.h"
#include "NAM/dsp.h"

namespace
{
void setParam(NexusProcessor& p, const char* id, float plain)
{
  auto* param = p.params.getParameter(id);
  param->setValueNotifyingHost(param->convertTo0to1(plain));
}

struct Result
{
  double mean = 0, p99 = 0, p999 = 0, worst = 0;
  int over = 0, overHalf = 0, blocks = 0;
};
} // namespace

int main(int argc, char** argv)
{
  // Ferramenta: a tela não baixa nem troca peças sozinha (captura padrão desligada).
#ifdef _WIN32
  _putenv_s("NEXUS_SEM_PADRAO", "1");
#else
  setenv("NEXUS_SEM_PADRAO", "1", 1);
#endif
  if (argc < 4)
  {
    std::printf("Uso: nexus_perfil <pasta do pedal> <pasta do cabeçote> <ir.wav> [guitarra.wav]\n");
    return 2;
  }
  juce::ScopedJuceInitialiser_GUI gui;
  const double rate = juce::SystemStats::getEnvironmentVariable("NEXUS_PERFIL_TAXA", "48000").getDoubleValue();
  const int block = juce::SystemStats::getEnvironmentVariable("NEXUS_PERFIL_BLOCO_CADEIA", "128").getIntValue();
  const double budgetUs = 1e6 * block / rate;
  std::vector<double> guitar;
  if (argc > 4)
  {
    double r = 0;
    guitar = wav::read(argv[4], r);
  }
  else
    guitar = nexus::makeGuitarLikeSignal(rate, 8.0, 3);

  // NEXUS_PERFIL_DROP=<pasta>: mudança de tom para baixo (drop) da gravação
  // argv[4] com o Signalsmith Stretch em janelas de tamanhos diferentes. Mostra
  // o atraso de cada uma e grava drop_<janela>_<semitons>.wav para ouvir.
  if (const auto outDir = juce::SystemStats::getEnvironmentVariable("NEXUS_PERFIL_DROP", {}); outDir.isNotEmpty())
  {
    std::vector<float> in(guitar.begin(), guitar.end());
    for (int window : {256, 384, 512, 768, 1024, 2048})
      for (int semis : {-2, -5, -12})
      {
        nexus::fx::PitchDrop drop;
        drop.prepare(rate, block, window);
        drop.setSemitones(static_cast<float>(semis));
        std::vector<double> out;
        std::vector<float> a(static_cast<size_t>(block));
        const double t0 = juce::Time::getMillisecondCounterHiRes();
        for (size_t k = 0; k + static_cast<size_t>(block) <= in.size(); k += static_cast<size_t>(block))
        {
          std::copy(in.begin() + static_cast<long>(k), in.begin() + static_cast<long>(k) + block, a.begin());
          drop.process(a.data(), block);
          for (float v : a)
            out.push_back(v);
        }
        const double ms = juce::Time::getMillisecondCounterHiRes() - t0;
        std::printf("janela %4d, %+3d semitons: atraso %5.1f ms | processador %4.1f%% de um núcleo\n", window, semis,
                    1000.0 * drop.latencySamples() / rate, 100.0 * ms / (1000.0 * static_cast<double>(in.size()) / rate));
        wav::write(juce::File(outDir).getChildFile("drop_" + juce::String(window) + "_" + juce::String(-semis) + ".wav").getFullPathName().toStdString(), out, rate);
      }
    return 0;
  }

  // NEXUS_PERFIL_CHERRY=<arquivo.wav>: o Cherry Drop dentro do Nexus (pedal 1,
  // drop de NEXUS_PERFIL_SEMITONS, padrão 5, mistura 10), só ele na cadeia.
  if (const auto outFile = juce::SystemStats::getEnvironmentVariable("NEXUS_PERFIL_CHERRY", {}); outFile.isNotEmpty())
  {
    const int semis = juce::SystemStats::getEnvironmentVariable("NEXUS_PERFIL_SEMITONS", "5").getIntValue();
    NexusProcessor q;
    q.setPlayConfigDetails(2, 2, rate, block);
    q.prepareToPlay(rate, block);
    ChainLayout l;
    l.pre = {0};
    q.setLayout(l);
    setParam(q, "p1_visual", static_cast<float>(NexusProcessor::kDropLook));
    setParam(q, "p1_ligado", 1.0f);
    setParam(q, "p1_a", (semis >= 12 ? 7.0f : static_cast<float>(semis - 1)) * 10.0f / 8.0f);
    setParam(q, "p1_b", 10.0f);
    juce::AudioBuffer<float> buf(2, block);
    juce::MidiBuffer m;
    for (int t = 0; t < 100; ++t)
    {
      buf.clear();
      q.processBlock(buf, m);
      juce::MessageManager::getInstance()->runDispatchLoopUntil(1);
    }
    std::vector<double> out;
    for (size_t k = 0; k + static_cast<size_t>(block) <= guitar.size(); k += static_cast<size_t>(block))
    {
      for (int c = 0; c < 2; ++c)
        for (int i = 0; i < block; ++i)
          buf.setSample(c, i, static_cast<float>(guitar[k + static_cast<size_t>(i)]));
      q.processBlock(buf, m);
      for (int i = 0; i < block; ++i)
        out.push_back(buf.getSample(0, i));
    }
    wav::write(outFile.toStdString(), out, rate);
    std::printf("gravado %s (%d semitons)\n", outFile.toRawUTF8(), semis);
    return 0;
  }

  // NEXUS_PERFIL_ZERO=1: o JACK do PipeWire avisa taxa e bloco 0 ao religar;
  // o processamento não pode prender. Mede quanto tempo cada bloco leva.
  if (juce::SystemStats::getEnvironmentVariable("NEXUS_PERFIL_ZERO", {}).isNotEmpty())
  {
    NexusProcessor q;
    q.setPlayConfigDetails(2, 2, 48000.0, 128);
    juce::AudioBuffer<float> buf(2, 512);
    juce::MidiBuffer m;
    for (auto [sr, bs] : {std::pair{0.0, 0}, std::pair{48000.0, 0}, std::pair{0.0, 128}, std::pair{44100.0, 64}})
    {
      q.prepareToPlay(sr, bs);
      const double t0 = juce::Time::getMillisecondCounterHiRes();
      for (int k = 0; k < 50; ++k)
      {
        buf.clear();
        q.processBlock(buf, m);
      }
      std::printf("taxa %.0f, bloco %d: 50 blocos em %.1f ms\n", sr, bs, juce::Time::getMillisecondCounterHiRes() - t0);
    }
    return 0;
  }

  // NEXUS_PERFIL_RELATORIO=1: envia um relatório de teste pelo mesmo caminho
  // do botão "Reportar um problema" e mostra o issue criado.
  if (juce::SystemStats::getEnvironmentVariable("NEXUS_PERFIL_RELATORIO", {}).isNotEmpty())
  {
    bool finished = false;
    nexus::report::send(nexus::report::Kind::problem, juce::String::fromUTF8("Teste do botão de relatório (pode fechar)."), "registro de teste",
                        [&](nexus::report::Sent sent) {
                          std::printf("issue %d %s %s\n", sent.number, sent.address.toRawUTF8(), sent.error.toRawUTF8());
                          finished = true;
                        });
    for (int t = 0; t < 600 && !finished; ++t)
      juce::MessageManager::getInstance()->runDispatchLoopUntil(50);
    return finished ? 0 : 1;
  }

  // NEXUS_PERFIL_NAM=<pasta de saída>: prova do motor. A mesma gravação
  // (argv[4]) passa pelo pedal (argv[1], drive 5) e pelo cabeçote (argv[2],
  // canal 1, ganho na primeira captura) de dois jeitos: pelo Nexus (sem caixa
  // nem equalizador) e direto pelo NAM, uma captura ligada na outra. Mostra a
  // diferença entre os dois e grava os dois arquivos.
  if (const auto outDir = juce::SystemStats::getEnvironmentVariable("NEXUS_PERFIL_NAM", {}); outDir.isNotEmpty())
  {
    nexus::PackOptions o;
    o.sampleRate = rate;
    o.maxBlockSize = block;
    o.keepCaptureLevel = true;
    auto pedal = nexus::loadPack(juce::String::fromUTF8(argv[1]).toStdString(), o);
    nexus::PackOptions oa = o;
    oa.keepCaptureLevel = false;
    auto amp = nexus::loadPack(juce::String::fromUTF8(argv[2]).toStdString(), oa);
    const nexus::Capture* pc = &pedal->captures.front();
    for (const auto& c : pedal->captures)
      if (std::abs(c.position - 5.0) < std::abs(pc->position - 5.0))
        pc = &c;
    const auto& ac = amp->captures.front();
    std::printf("pedal: captura %.1f | cabeçote: captura %.1f (ganho de volume %+.1f dB)\n", pc->position, ac.position, 20 * std::log10(ac.levelGain));

    // Direto pelo NAM.
    std::vector<double> in(guitar.begin(), guitar.end()), mid(in.size()), direct(in.size());
    pc->dsp->Reset(rate, block);
    ac.dsp->Reset(rate, block);
    for (size_t k = 0; k + block <= in.size(); k += block)
    {
      double* i = in.data() + k;
      double* m = mid.data() + k;
      double* d = direct.data() + k;
      pc->dsp->process(&i, &m, block);
      ac.dsp->process(&m, &d, block);
    }
    for (auto& v : direct)
      v *= ac.levelGain;

    // Pelo Nexus.
    NexusProcessor q;
    q.setPlayConfigDetails(2, 2, rate, block);
    q.prepareToPlay(rate, block);
    ChainLayout l;
    l.pre = {0};
    q.setLayout(l);
    setParam(q, "p1_drive", static_cast<float>(pc->position));
    setParam(q, "ganho", static_cast<float>(ac.position));
    setParam(q, "caixa", 0.0f);
    q.loadPedalPack(0, juce::File(juce::String::fromUTF8(argv[1])));
    q.loadAmpPack(0, 0, juce::File(juce::String::fromUTF8(argv[2])));
    juce::AudioBuffer<float> buf(2, block);
    juce::MidiBuffer m;
    for (int t = 0; t < 4000 && !(q.slotLoaded(0) && q.channelLoaded(0, 0)); ++t)
    {
      buf.clear();
      q.processBlock(buf, m);
      juce::MessageManager::getInstance()->runDispatchLoopUntil(2);
    }
    for (int t = 0; t < 400; ++t)
    {
      buf.clear();
      q.processBlock(buf, m);
      juce::MessageManager::getInstance()->runDispatchLoopUntil(1);
    }
    std::vector<double> nexusOut;
    for (size_t k = 0; k + block <= in.size(); k += block)
    {
      for (int c = 0; c < 2; ++c)
        for (int i = 0; i < block; ++i)
          buf.setSample(c, i, static_cast<float>(in[k + static_cast<size_t>(i)]));
      q.processBlock(buf, m);
      for (int i = 0; i < block; ++i)
        nexusOut.push_back(buf.getSample(0, i));
    }
    direct.resize(nexusOut.size());
    // Diferença depois de igualar o volume (o Nexus pode ter outro volume final).
    const size_t from = static_cast<size_t>(rate);
    double xy = 0, yy = 0, xx = 0;
    for (size_t i = from; i < nexusOut.size(); ++i)
    {
      xy += nexusOut[i] * direct[i];
      yy += direct[i] * direct[i];
      xx += nexusOut[i] * nexusOut[i];
    }
    const double g = xy / (yy + 1e-30);
    double ee = 0;
    for (size_t i = from; i < nexusOut.size(); ++i)
      ee += (nexusOut[i] - g * direct[i]) * (nexusOut[i] - g * direct[i]);
    std::printf("Nexus %.1f dB | NAM direto %.1f dB | volume do Nexus em relação ao NAM %+.1f dB\n", 10 * std::log10(xx / nexusOut.size() + 1e-30),
                10 * std::log10(yy / nexusOut.size() + 1e-30), 20 * std::log10(std::abs(g) + 1e-30));
    std::printf("diferença entre os dois (depois de igualar o volume): %.1f dB abaixo do som\n", 10 * std::log10(xx / (ee + 1e-30)));
    wav::write(juce::File(outDir).getChildFile("nexus.wav").getFullPathName().toStdString(), nexusOut, rate);
    wav::write(juce::File(outDir).getChildFile("nam_direto.wav").getFullPathName().toStdString(), direct, rate);
    wav::write(juce::File(outDir).getChildFile("so_pedal.wav").getFullPathName().toStdString(), mid, rate);
    return 0;
  }

  // NEXUS_PERFIL_GANHO=1: ganho de volume que o Nexus aplica em cada captura
  // do pacote (argv[1]), com a regra de cabeçote e com a regra de pedal.
  if (juce::SystemStats::getEnvironmentVariable("NEXUS_PERFIL_GANHO", {}).isNotEmpty())
  {
    const auto s = nexus::makeGuitarLikeSignal(rate, 2.0, 7);
    std::printf("sinal de medida: %.1f dB\n", nexus::rmsDb(s.data() + static_cast<int>(rate / 2), static_cast<int>(s.size() - rate / 2)));
    for (int pedal = 0; pedal < 2; ++pedal)
    {
      nexus::PackOptions o;
      o.sampleRate = rate;
      o.maxBlockSize = block;
      o.keepCaptureLevel = pedal == 1;
      auto p = nexus::loadPack(juce::String::fromUTF8(argv[1]).toStdString(), o);
      for (const auto& c : p->captures)
        std::printf("  regra de %s, captura %4.1f: saída medida %6.1f dB, ganho aplicado %+5.1f dB\n", pedal ? "pedal   " : "cabeçote", c.position,
                    c.measuredRmsDb, 20 * std::log10(c.levelGain));
    }
    return 0;
  }

  // NEXUS_PERFIL_PEDAIS=<pasta de saída>: Lemon (pedal 1) e Banana (pedal 2)
  // antes do cabeçote; toca a gravação "Rock" e depois silêncio na entrada.
  // Grava os casos (só Lemon, só Banana, os dois) e mede o que sobra no silêncio.
  if (const auto outDir = juce::SystemStats::getEnvironmentVariable("NEXUS_PERFIL_PEDAIS", {}); outDir.isNotEmpty())
  {
    const juce::File lemon(juce::String::fromUTF8(argv[1]));
    const juce::File banana = lemon.getParentDirectory().getParentDirectory().getChildFile("Banana Overdrive").findChildFiles(juce::File::findDirectories, false)[0];
    // NEXUS_PERFIL_RUIDO=<dB>: soma um chiado branco desse nível na entrada
    // (como o de um microfone ou de uma placa), para ver quanto a cadeia o aumenta.
    const double noiseDb = juce::SystemStats::getEnvironmentVariable("NEXUS_PERFIL_RUIDO", "-400").getDoubleValue();
    const float noiseAmp = static_cast<float>(std::pow(10.0, noiseDb / 20.0) * std::sqrt(3.0));
    juce::Random noise(7);
    for (int which = 0; which < 4; ++which)
    {
      NexusProcessor q;
      q.setPlayConfigDetails(2, 2, rate, block);
      q.prepareToPlay(rate, block);
      ChainLayout l;
      l.pre = which == 0 ? std::vector<int>{0} : which == 1 ? std::vector<int>{1} : which == 2 ? std::vector<int>{0, 1} : std::vector<int>{};
      // NEXUS_PERFIL_GATE=1: o pedal 3 vira o Pear Gate e entra no começo da cadeia.
      const bool gate = juce::SystemStats::getEnvironmentVariable("NEXUS_PERFIL_GATE", {}).isNotEmpty();
      if (gate)
        l.pre.insert(l.pre.begin(), 2);
      q.setLayout(l);
      if (gate)
      {
        setParam(q, "p3_visual", static_cast<float>(NexusProcessor::kPedalLooks - 1));
        setParam(q, "p3_a", juce::SystemStats::getEnvironmentVariable("NEXUS_PERFIL_GATE", "5").getFloatValue());
      }
      setParam(q, "p1_visual", 0.0f);
      setParam(q, "p2_visual", 2.0f);
      setParam(q, "ganho", 6.0f);
      q.loadPedalPack(0, lemon);
      q.loadPedalPack(1, banana);
      const bool noAmp = juce::SystemStats::getEnvironmentVariable("NEXUS_PERFIL_SEM_CABECOTE", {}).isNotEmpty();
      if (!noAmp)
        q.loadAmpPack(0, 0, juce::File(juce::String::fromUTF8(argv[2])));
      q.loadIrAsync(0, juce::File(juce::String::fromUTF8(argv[3])));
      juce::AudioBuffer<float> buf(2, block);
      juce::MidiBuffer m;
      for (int t = 0; t < 4000 && !(q.slotLoaded(0) && q.slotLoaded(1) && (noAmp || q.channelLoaded(0, 0)) && q.irLoaded(0)); ++t)
      {
        buf.clear();
        q.processBlock(buf, m);
        juce::MessageManager::getInstance()->runDispatchLoopUntil(2);
      }
      setParam(q, "caixa", juce::SystemStats::getEnvironmentVariable("NEXUS_PERFIL_SEM_CAIXA", {}).isEmpty() ? 1.0f : 0.0f);
      for (int t = 0; t < 300; ++t)
      {
        buf.clear();
        q.processBlock(buf, m);
        juce::MessageManager::getInstance()->runDispatchLoopUntil(1);
      }
      auto& test = q.testSource();
      // NEXUS_PERFIL_ESTILO=pesado|limpo|blues: estilo do "Testar o som" (padrão rock).
      const auto style = juce::SystemStats::getEnvironmentVariable("NEXUS_PERFIL_ESTILO", "rock");
      test.setStyle(style == "pesado" ? nexus::TestSource::heavy : style == "limpo" ? nexus::TestSource::clean
                    : style == "blues" ? nexus::TestSource::blues : nexus::TestSource::rock);
      test.setPlaying(true);
      std::vector<double> out;
      const double playSec = juce::SystemStats::getEnvironmentVariable("NEXUS_PERFIL_TOCA", "5").getDoubleValue();
      const int playBlocks = static_cast<int>(playSec * rate / block), quietBlocks = static_cast<int>(6.0 * rate / block);
      // NEXUS_PERFIL_LIGAR=1: o pedal 2 começa desligado, liga em 2 s e desliga em 3,5 s.
      const bool toggle = juce::SystemStats::getEnvironmentVariable("NEXUS_PERFIL_LIGAR", {}).isNotEmpty() && which == 2;
      if (toggle)
        setParam(q, "p2_ligado", 0.0f);
      for (int b = 0; b < playBlocks + quietBlocks; ++b)
      {
        if (toggle && b == static_cast<int>(2.0 * rate / block))
          setParam(q, "p2_ligado", 1.0f);
        if (toggle && b == static_cast<int>(3.5 * rate / block))
          setParam(q, "p2_ligado", 0.0f);
        if (b == playBlocks)
          test.setPlaying(false);
        buf.clear();
        for (int c = 0; c < 2; ++c)
          for (int i = 0; i < block; ++i)
            buf.setSample(c, i, noiseAmp * (2.0f * noise.nextFloat() - 1.0f));
        q.processBlock(buf, m);
        for (int i = 0; i < block; ++i)
          out.push_back(buf.getSample(0, i));
      }
      // Silêncio: depois de 1 s do fim (a cauda do som já acabou).
      double e = 0, pk = 0, jump = 0;
      const size_t from = static_cast<size_t>((playSec + 1.0) * rate);
      // Onde, no silêncio, aparece o maior estouro.
      size_t worst = from;
      for (size_t i = from; i < out.size(); ++i)
        if (std::abs(out[i]) > std::abs(out[worst]))
          worst = i;
      std::printf("   estouro em %.2f s (%.2f s depois do fim do som)\n", worst / rate, worst / rate - playSec);
      for (size_t i = from; i < out.size(); ++i)
      {
        e += out[i] * out[i];
        pk = std::max(pk, std::abs(out[i]));
        jump = std::max(jump, std::abs(out[i] - out[i - 1]));
      }
      double playE = 0;
      for (size_t i = static_cast<size_t>(rate); i < static_cast<size_t>(5.0 * rate); ++i)
        playE += out[i] * out[i];
      for (int sl : {0, 1, NexusProcessor::ampSlot(0, 0)})
        std::printf("   espaço %d: %s\n", sl, q.statusText(sl).toRawUTF8());
      if (toggle)
      {
        // Maior salto de uma amostra para a outra perto de cada troca, comparado ao som normal.
        auto jumpIn = [&](double t0, double t1) {
          double j = 0;
          for (size_t i = static_cast<size_t>(t0 * rate) + 1; i < static_cast<size_t>(t1 * rate); ++i)
            j = std::max(j, std::abs(out[i] - out[i - 1]));
          return 20 * std::log10(j + 1e-12);
        };
        std::printf("   saltos: tocando normal %.1f dB | ao ligar %.1f dB | ao desligar %.1f dB\n", jumpIn(1.0, 1.9), jumpIn(1.99, 2.4),
                    jumpIn(3.49, 3.9));
      }
      const char* names[4] = {"so_lemon", "so_banana", "lemon_e_banana", "so_cabecote"};
      std::printf("%-15s tocando %.1f dB | silêncio: ruído %.1f dB, pico %.1f dB, maior salto %.1f dB\n", names[which],
                  10 * std::log10(playE / (4.0 * rate) + 1e-20), 10 * std::log10(e / (out.size() - from) + 1e-20), 20 * std::log10(pk + 1e-12),
                  20 * std::log10(jump + 1e-12));
      wav::write(juce::File(outDir).getChildFile(juce::String(names[which]) + ".wav").getFullPathName().toStdString(), out, rate);
    }
    return 0;
  }

  // NEXUS_PERFIL_ESTUDIO_CADEIA=<pasta da música>: pedal, cabeçote e caixa
  // (argv) tocando a guitarra, primeiro sem a música e depois com a música do
  // estúdio junto; mede cada bloco inteiro no ritmo da placa (NEXUS_PERFIL_BLOCO).
  if (const auto songId = juce::SystemStats::getEnvironmentVariable("NEXUS_PERFIL_ESTUDIO_CADEIA", {}); songId.isNotEmpty())
  {
    const int blk = juce::SystemStats::getEnvironmentVariable("NEXUS_PERFIL_BLOCO", "128").getIntValue();
    const double blkUs = 1e6 * blk / rate;
    nexus::studio::Library lib;
    lib.rescan();
    const auto* song = lib.find(songId);
    if (song == nullptr)
      return 1;
    NexusProcessor q;
    q.setPlayConfigDetails(2, 2, rate, blk);
    q.prepareToPlay(rate, blk);
    ChainLayout l;
    l.pre = {0};
    q.setLayout(l);
    setParam(q, "p1_visual", 2.0f);
    setParam(q, "p1_drive", 4.0f);
    setParam(q, "ganho", 7.0f);
    setParam(q, "caixa", 1.0f);
    q.loadPedalPack(0, juce::File(juce::String::fromUTF8(argv[1])));
    q.loadAmpPack(0, 0, juce::File(juce::String::fromUTF8(argv[2])));
    q.loadIrAsync(0, juce::File(juce::String::fromUTF8(argv[3])));
    juce::AudioBuffer<float> buf(2, blk);
    juce::MidiBuffer m;
    size_t at = 0;
    auto feed = [&] {
      for (int i = 0; i < blk; ++i)
      {
        buf.setSample(0, i, static_cast<float>(guitar[at]));
        buf.setSample(1, i, static_cast<float>(guitar[at]));
        at = (at + 1) % guitar.size();
      }
    };
    for (int t = 0; t < 4000 && !(q.slotLoaded(0) && q.channelLoaded(0, 0) && q.irLoaded(0)); ++t)
    {
      feed();
      q.processBlock(buf, m);
      juce::MessageManager::getInstance()->runDispatchLoopUntil(2);
    }
    for (int t = 0; t < 400; ++t)
    {
      feed();
      q.processBlock(buf, m);
      juce::MessageManager::getInstance()->runDispatchLoopUntil(1);
    }
    auto& player = q.studio();
    player.open(*song);
    player.setVolume(0.8f);
    auto measure = [&](const char* name, double seconds) {
      std::vector<double> us;
      auto next = std::chrono::steady_clock::now();
      for (int b = 0; b < static_cast<int>(seconds * rate / blk); ++b)
      {
        feed();
        const auto t0 = std::chrono::steady_clock::now();
        q.processBlock(buf, m);
        us.push_back(std::chrono::duration<double, std::micro>(std::chrono::steady_clock::now() - t0).count());
        juce::MessageManager::getInstance()->runDispatchLoopUntil(0);
        next += std::chrono::microseconds(static_cast<long long>(blkUs));
        std::this_thread::sleep_until(next);
      }
      auto s = us;
      std::sort(s.begin(), s.end());
      double sum = 0;
      int over = 0, overHalf = 0;
      for (auto v : us)
      {
        sum += v;
        over += v > blkUs ? 1 : 0;
        overHalf += v > blkUs * 0.5 ? 1 : 0;
      }
      std::printf("%-28s bloco %d (%.0f us): média %4.0f us (%4.1f%%)  99%% %5.0f  99,9%% %5.0f  pior %5.0f  estouros %d, acima da metade %d, de %zu\n", name, blk,
                  blkUs, sum / us.size(), 100.0 * sum / us.size() / blkUs, s[s.size() * 99 / 100], s[s.size() * 999 / 1000], s.back(), over, overHalf,
                  us.size());
    };
    const double secs = juce::SystemStats::getEnvironmentVariable("NEXUS_PERFIL_TOCA", "8").getDoubleValue();
    // NEXUS_PERFIL_SEPARAR=<arquivo>: separa essa música ao mesmo tempo (como
    // o botão "Importar música"). Espera a separação chegar à etapa pesada.
    if (const auto file = juce::SystemStats::getEnvironmentVariable("NEXUS_PERFIL_SEPARAR", {}); file.isNotEmpty())
    {
      measure("guitarra, antes de separar", secs);
      q.separation().start(juce::File(file), [](const juce::String&, const juce::String& e) { std::printf("separação: %s\n", e.isEmpty() ? "pronta" : e.toRawUTF8()); });
      for (int t = 0; t < 6000 && q.separation().stage() != nexus::studio::Separation::Stage::separating; ++t)
      {
        feed();
        q.processBlock(buf, m);
        juce::MessageManager::getInstance()->runDispatchLoopUntil(5);
      }
      measure("guitarra separando", secs);
      player.setPlaying(true);
      measure("guitarra + música, separando", secs);
      q.separation().cancel();
      for (int t = 0; t < 2000 && q.separation().busy(); ++t)
        juce::MessageManager::getInstance()->runDispatchLoopUntil(5);
      return 0;
    }
    measure("guitarra sem a música", secs);
    player.setPlaying(true);
    measure("guitarra + música", secs);
    nexus::studio::Mix mix = song->mix;
    mix.speed = 0.75;
    player.setMix(mix);
    measure("guitarra + música a 75%", secs);
    return 0;
  }

  // NEXUS_PERFIL_ESTUDIO=<pasta da música>: abre no player do estúdio, dá
  // play e mede o que sai (bloco de NEXUS_PERFIL_BLOCO, padrão 64).
  if (const auto songId = juce::SystemStats::getEnvironmentVariable("NEXUS_PERFIL_ESTUDIO", {}); songId.isNotEmpty())
  {
    const int blk = juce::SystemStats::getEnvironmentVariable("NEXUS_PERFIL_BLOCO", "64").getIntValue();
    nexus::studio::Library lib;
    lib.rescan();
    const auto* song = lib.find(songId);
    std::printf("Música: %s\n", song ? song->title.toRawUTF8() : "(não achada)");
    if (song == nullptr)
      return 1;
    nexus::studio::Player player;
    player.prepare(rate, blk);
    std::printf("Abriu: %s\n", player.open(*song) ? "sim" : "não");
    player.setVolume(0.8f);
    player.setPlaying(true);
    std::printf("Tocando: %s, duração %.1f s\n", player.playing() ? "sim" : "não", player.length());
    // No ritmo da placa: cada bloco espera o seu tempo, e mede quanto o
    // player gasta (o orçamento é o bloco inteiro, com a cadeia junto).
    const double blkUs = 1e6 * blk / rate;
    const int seconds = juce::SystemStats::getEnvironmentVariable("NEXUS_PERFIL_TOCA", "8").getIntValue();
    std::vector<float> l(static_cast<size_t>(blk)), r(static_cast<size_t>(blk));
    std::vector<double> us;
    int silent = 0;
    auto next = std::chrono::steady_clock::now();
    for (int sec = 0; sec < seconds; ++sec)
    {
      double peak = 0;
      for (int b = 0; b < static_cast<int>(rate) / blk; ++b)
      {
        std::fill(l.begin(), l.end(), 0.0f);
        std::fill(r.begin(), r.end(), 0.0f);
        const auto t0 = std::chrono::steady_clock::now();
        player.process(l.data(), r.data(), blk);
        us.push_back(std::chrono::duration<double, std::micro>(std::chrono::steady_clock::now() - t0).count());
        float blockPeak = 0.0f;
        for (auto v : l)
          blockPeak = std::max(blockPeak, std::abs(v));
        silent += sec > 0 && blockPeak == 0.0f ? 1 : 0;
        peak = std::max(peak, static_cast<double>(blockPeak));
        next += std::chrono::microseconds(static_cast<long long>(blkUs));
        std::this_thread::sleep_until(next);
      }
      std::printf("  %d s: pico %.1f dB, posição %.2f s\n", sec, 20 * std::log10(peak + 1e-12), player.position());
    }
    auto s = us;
    std::sort(s.begin(), s.end());
    double sum = 0;
    int overQuarter = 0, overHalf = 0;
    for (auto v : us)
    {
      sum += v;
      overQuarter += v > blkUs * 0.25 ? 1 : 0;
      overHalf += v > blkUs * 0.5 ? 1 : 0;
    }
    std::printf("Player, bloco %d (%.0f us): média %.0f us (%.1f%%)  99%% %.0f  99,9%% %.0f  pior %.0f  acima de 1/4 do bloco %d, acima da metade %d, de %zu | blocos mudos %d\n",
                blk, blkUs, sum / us.size(), 100.0 * sum / us.size() / blkUs, s[s.size() * 99 / 100], s[s.size() * 999 / 1000], s.back(),
                overQuarter, overHalf, us.size(), silent);
    return 0;
  }

  // NEXUS_PERFIL_ESTADO=<arquivo .settings do programa>: carrega o estado
  // salvo (pedais, cabeçote, caixa, botões), liga o "Testar o som" e mede o
  // nível da saída com o bloco dado em NEXUS_PERFIL_BLOCO (padrão 64).
  if (const auto state = juce::SystemStats::getEnvironmentVariable("NEXUS_PERFIL_ESTADO", {}); state.isNotEmpty())
  {
    const int blk = juce::SystemStats::getEnvironmentVariable("NEXUS_PERFIL_BLOCO", "64").getIntValue();
    juce::PropertiesFile::Options o;
    juce::PropertiesFile props(juce::File(state), o);
    juce::MemoryBlock data;
    data.fromBase64Encoding(props.getValue("filterState"));
    NexusProcessor q;
    q.setPlayConfigDetails(2, 2, rate, blk);
    q.setStateInformation(data.getData(), static_cast<int>(data.getSize()));
    q.prepareToPlay(rate, blk);
    // NEXUS_PERFIL_SEM_PEDAIS=1: desliga todos os pedais (para comparar).
    for (auto* prm : q.getParameters())
      if (auto* w = dynamic_cast<juce::AudioProcessorParameterWithID*>(prm); w != nullptr && w->paramID.matchesWildcard("p*_ligado", true))
      {
        if (juce::SystemStats::getEnvironmentVariable("NEXUS_PERFIL_SEM_PEDAIS", {}).isNotEmpty())
          prm->setValueNotifyingHost(0.0f);
        // NEXUS_PERFIL_PEDAL=<n>: liga o pedal n (1 ou 2) mesmo se estava desligado.
        if (w->paramID == "p" + juce::SystemStats::getEnvironmentVariable("NEXUS_PERFIL_PEDAL", "x") + "_ligado")
          prm->setValueNotifyingHost(1.0f);
        if (prm->getValue() > 0.5f)
          std::printf("  %s ligado\n", w->paramID.toRawUTF8());
      }
    {
      const auto l = q.layout();
      std::printf("  pedais antes:");
      for (int p : l.pre)
        std::printf(" %d", p);
      std::printf(" | depois:");
      for (int p : l.post)
        std::printf(" %d", p);
      std::printf("\n");
    }
    juce::AudioBuffer<float> buf(2, blk);
    juce::MidiBuffer m;
    // NEXUS_PERFIL_GRAVAR=<arquivo .wav>: grava o "Testar o som" e o silêncio depois.
    std::vector<double> rec;
    bool recording = false;
    auto level = [&](int blocks, bool silentInput) {
      double peak = 0;
      for (int i = 0; i < blocks; ++i)
      {
        buf.clear();
        if (!silentInput)
          for (int k = 0; k < blk; ++k)
            buf.setSample(0, k, 0.0f);
        q.processBlock(buf, m);
        if (recording)
          for (int k = 0; k < blk; ++k)
            rec.push_back(buf.getSample(0, k));
        peak = std::max(peak, static_cast<double>(buf.getMagnitude(0, 0, blk)));
        juce::MessageManager::getInstance()->runDispatchLoopUntil(0);
      }
      return 20 * std::log10(peak + 1e-12);
    };
    const double t0 = juce::Time::getMillisecondCounterHiRes();
    for (int t = 0; t < 4000; ++t)
    {
      level(10, true);
      juce::Thread::sleep(5);
      bool loading = false;
      for (int s = 0; s < NexusProcessor::kSlots; ++s)
        loading = loading || q.statusText(s).startsWith("Carregando");
      if (!loading)
        break;
    }
    std::printf("Carregou tudo em %.1f s\n", (juce::Time::getMillisecondCounterHiRes() - t0) / 1000.0);
    for (int s = 0; s < NexusProcessor::kSlots; ++s)
      if (q.statusText(s).isNotEmpty() && !q.statusText(s).startsWith(juce::String::fromUTF8("Nenhum")))
        std::printf("  espaço %d: %s\n", s, q.statusText(s).toRawUTF8());
    for (auto* prm : q.getParameters())
      if (auto* w = dynamic_cast<juce::AudioProcessorParameterWithID*>(prm))
        if (w->paramID == "entrada" || w->paramID == "entrada_mudo" || w->paramID == "volume" || w->paramID == "caixa" || w->paramID == "ganho")
          std::printf("  %s = %s\n", w->paramID.toRawUTF8(), prm->getCurrentValueAsText().toRawUTF8());
    for (int k = 0; k < 6; ++k)
      std::printf("Sem teste (entrada em silêncio), %d s: pico %.1f dB\n", k, level(static_cast<int>(rate / blk), true));
    // Média (componente contínua) da saída em silêncio.
    {
      double sum = 0;
      for (int i = 0; i < 100; ++i)
      {
        buf.clear();
        q.processBlock(buf, m);
        for (int k = 0; k < blk; ++k)
          sum += buf.getSample(0, k);
      }
      std::printf("Média da saída em silêncio: %.5f\n", sum / (100.0 * blk));
    }
    auto& test = q.testSource();
    test.setStyle(nexus::TestSource::rock);
    test.setPlaying(true);
    recording = true;
    std::printf("Testar o som (rock), bloco %d: pico %.1f dB\n", blk, level(static_cast<int>(8.0 * rate / blk), true));
    test.setPlaying(false);
    // Depois do som: o que sobra a cada quarto de segundo.
    for (int k = 0; k < 24; ++k)
    {
      const double pk = level(static_cast<int>(0.25 * rate / blk), true);
      if (pk > -90.0)
        std::printf("  silêncio %.2f s: pico %.1f dB\n", (k + 1) * 0.25, pk);
    }
    std::printf("Fim da medida do silêncio.\n");
    if (const auto f = juce::SystemStats::getEnvironmentVariable("NEXUS_PERFIL_GRAVAR", {}); f.isNotEmpty())
      wav::write(f.toStdString(), rec, rate);
    return 0;
  }

  // NEXUS_PERFIL_DRIVE=1: só o pedal, drive de posição em posição; mede o
  // volume e o quanto distorce, e compara com cada captura tocada direto.
  if (juce::SystemStats::getEnvironmentVariable("NEXUS_PERFIL_DRIVE", {}) == "1")
  {
    struct Measure
    {
      double rmsDb, crestDb, brightDb;
    };
    auto measure = [&](const std::vector<double>& x) {
      const size_t w = 2400;
      double e = 0, hi = 0, prev = 0;
      std::vector<double> crests;
      for (size_t k = 0; k + w <= x.size(); k += w)
      {
        double pk = 0, ee = 0;
        for (size_t i = k; i < k + w; ++i)
        {
          pk = std::max(pk, std::abs(x[i]));
          ee += x[i] * x[i];
        }
        if (ee / w > 1e-8)
          crests.push_back(20 * std::log10(pk / std::sqrt(ee / w)));
      }
      for (auto v : x)
      {
        e += v * v;
        const double d = v - prev; // diferença: pesa os agudos (harmônicos da distorção)
        hi += d * d;
        prev = v;
      }
      std::sort(crests.begin(), crests.end());
      return Measure{10 * std::log10(e / x.size() + 1e-20), crests.empty() ? 0.0 : crests[crests.size() / 2], 10 * std::log10(hi / (e + 1e-20))};
    };
    const juce::File folder(juce::String::fromUTF8(argv[1]));
    std::printf("Direto pelo NAM (cada captura, sem o Nexus), mesma guitarra:\n");
    nexus::PackOptions opt;
    opt.sampleRate = rate;
    opt.maxBlockSize = block;
    auto pack = nexus::loadPack(folder.getFullPathName().toStdString(), opt);
    for (auto& c : pack->captures)
    {
      c.dsp->Reset(rate, block);
      std::vector<double> out(guitar.size());
      std::vector<double> in(guitar.begin(), guitar.end());
      for (size_t k = 0; k + block <= in.size(); k += block)
      {
        double* i = in.data() + k;
        double* o = out.data() + k;
        c.dsp->process(&i, &o, block);
      }
      out.erase(out.begin(), out.begin() + 12000);
      const auto m = measure(out);
      std::printf("  captura %4.1f  volume %6.1f dB  (com a correção de volume %6.1f dB)  achatamento %4.1f dB  agudos %6.1f dB\n", c.position, m.rmsDb,
                  m.rmsDb + 20 * std::log10(c.levelGain), m.crestDb, m.brightDb);
    }
    NexusProcessor q;
    q.setPlayConfigDetails(2, 2, rate, block);
    q.prepareToPlay(rate, block);
    ChainLayout l;
    l.pre = {0};
    q.setLayout(l);
    setParam(q, "p1_visual", 2.0f);
    setParam(q, "p1_ligado", 1.0f);
    q.loadPedalPack(0, folder);
    juce::AudioBuffer<float> buf(2, block);
    juce::MidiBuffer m;
    for (int t = 0; t < 3000 && !q.slotLoaded(0); ++t)
    {
      buf.clear();
      q.processBlock(buf, m);
      juce::Thread::sleep(2);
    }
    std::printf("Pelo Nexus (só o pedal, sem cabeçote):\n");
    for (int d = 1; d <= 8; ++d)
    {
      setParam(q, "p1_drive", static_cast<float>(d));
      std::vector<double> out;
      for (size_t k = 0; k + block <= guitar.size(); k += block)
      {
        for (int i = 0; i < block; ++i)
        {
          buf.setSample(0, i, static_cast<float>(guitar[k + i]));
          buf.setSample(1, i, static_cast<float>(guitar[k + i]));
        }
        q.processBlock(buf, m);
        for (int i = 0; i < block; ++i)
          out.push_back(buf.getSample(0, i));
      }
      out.erase(out.begin(), out.begin() + 12000);
      const auto r = measure(out);
      std::printf("  drive %d  volume %6.1f dB  achatamento %4.1f dB  agudos %6.1f dB\n", d, r.rmsDb, r.crestDb, r.brightDb);
    }
    return 0;
  }

  // NEXUS_PERFIL_EQ=1: cabeçote + caixa; cada botão do equalizador em 0, 5 e
  // 10; mede o nível em faixas de frequência da saída.
  if (juce::SystemStats::getEnvironmentVariable("NEXUS_PERFIL_EQ", {}) == "1")
  {
    NexusProcessor q;
    q.setPlayConfigDetails(2, 2, rate, block);
    q.prepareToPlay(rate, block);
    q.setLayout(ChainLayout{});
    setParam(q, "ganho", 6.0f);
    q.loadAmpPack(0, 0, juce::File(juce::String::fromUTF8(argv[2])));
    q.loadIrAsync(0, juce::File(juce::String::fromUTF8(argv[3])));
    juce::AudioBuffer<float> buf(2, block);
    juce::MidiBuffer m;
    for (int t = 0; t < 3000 && !(q.channelLoaded(0, 0) && q.irLoaded(0)); ++t)
    {
      buf.clear();
      q.processBlock(buf, m);
      juce::Thread::sleep(2);
    }
    setParam(q, "caixa", 1.0f);
    // Nível (dB) por faixa: graves 60-200, médios 400-1200, agudos 2-4 k, presença 4-8 k.
    auto bands = [&]() {
      std::vector<double> out;
      for (int pass = 0; pass < 2; ++pass)
      {
        out.clear();
        for (size_t k = 0; k + block <= guitar.size(); k += block)
        {
          for (int i = 0; i < block; ++i)
          {
            buf.setSample(0, i, static_cast<float>(guitar[k + i]));
            buf.setSample(1, i, static_cast<float>(guitar[k + i]));
          }
          q.processBlock(buf, m);
          for (int i = 0; i < block; ++i)
            out.push_back(buf.getSample(0, i));
        }
      }
      const int N = 1 << 15;
      juce::dsp::FFT fft(15);
      std::vector<float> acc(N / 2, 0.0f), w(2 * N);
      int frames = 0;
      for (size_t k = 0; k + N <= out.size(); k += N / 2, ++frames)
      {
        std::fill(w.begin(), w.end(), 0.0f);
        for (int i = 0; i < N; ++i)
          w[static_cast<size_t>(i)] = static_cast<float>(out[k + i] * (0.5 - 0.5 * std::cos(2 * 3.14159265 * i / N)));
        fft.performFrequencyOnlyForwardTransform(w.data());
        for (int i = 0; i < N / 2; ++i)
          acc[static_cast<size_t>(i)] += w[static_cast<size_t>(i)] * w[static_cast<size_t>(i)];
      }
      auto band = [&](double lo, double hi) {
        double e = 0;
        for (int i = static_cast<int>(lo * N / rate); i < static_cast<int>(hi * N / rate); ++i)
          e += acc[static_cast<size_t>(i)];
        return 10 * std::log10(e + 1e-20);
      };
      return std::array<double, 4>{band(60, 200), band(400, 1200), band(2000, 4000), band(4000, 8000)};
    };
    // NEXUS_PERFIL_TOM=<pasta do pedal>: o pedal antes do cabeçote; mede o TOM.
    if (const auto pedal = juce::SystemStats::getEnvironmentVariable("NEXUS_PERFIL_TOM", {}); pedal.isNotEmpty())
    {
      ChainLayout l;
      l.pre = {0};
      q.setLayout(l);
      setParam(q, "p1_visual", 2.0f);
      setParam(q, "p1_drive", 5.0f);
      q.loadPedalPack(0, juce::File(pedal));
      // O relógio do processador recolhe a cadeia antiga; sem ele a nova não entra.
      for (int t = 0; t < 3000 && !q.slotLoaded(0); ++t)
      {
        buf.clear();
        q.processBlock(buf, m);
        juce::MessageManager::getInstance()->runDispatchLoopUntil(2);
      }
      for (int t = 0; t < 50; ++t)
      {
        buf.clear();
        q.processBlock(buf, m);
        juce::MessageManager::getInstance()->runDispatchLoopUntil(2);
      }
      const auto base = bands();
      std::printf("TOM do pedal, diferença para o 5 (dB): graves | médios | agudos | presença\n");
      for (float v : {0.0f, 10.0f})
      {
        setParam(q, "p1_tom", v);
        const auto b = bands();
        std::printf("  tom %4.0f   %+6.1f  %+6.1f  %+6.1f  %+6.1f\n", v, b[0] - base[0], b[1] - base[1], b[2] - base[2], b[3] - base[3]);
      }
      return 0;
    }
    const auto flat = bands();
    std::printf("Diferença para tudo em 5 (dB) nas faixas: graves | médios | agudos | presença\n");
    for (const char* knob : {"grave", "medio", "agudo", "presenca"})
      for (float v : {0.0f, 10.0f})
      {
        setParam(q, knob, v);
        const auto b = bands();
        std::printf("  %-9s %4.0f   %+6.1f  %+6.1f  %+6.1f  %+6.1f\n", knob, v, b[0] - flat[0], b[1] - flat[1], b[2] - flat[2], b[3] - flat[3]);
        setParam(q, knob, 5.0f);
      }
    return 0;
  }

  NexusProcessor p;
  p.setPlayConfigDetails(2, 2, rate, block);
  p.prepareToPlay(rate, block);
  ChainLayout layout;
  layout.pre = {0};
  p.setLayout(layout);
  setParam(p, "p1_visual", 2.0f);
  setParam(p, "p1_drive", 4.0f);
  setParam(p, "ganho", 7.0f);
  p.loadPedalPack(0, juce::File(juce::String::fromUTF8(argv[1])));
  p.loadAmpPack(0, 0, juce::File(juce::String::fromUTF8(argv[2])));
  p.loadIrAsync(0, juce::File(juce::String::fromUTF8(argv[3])));
  juce::AudioBuffer<float> buffer(2, block);
  juce::MidiBuffer midi;
  size_t at = 0;
  auto feed = [&] {
    for (int i = 0; i < block; ++i)
    {
      const float v = static_cast<float>(guitar[at]);
      buffer.setSample(0, i, v);
      buffer.setSample(1, i, v);
      at = (at + 1) % guitar.size();
    }
  };
  for (int t = 0; t < 3000 && !(p.channelLoaded(0, 0) && p.slotLoaded(0) && p.irLoaded(0)); ++t)
  {
    feed();
    p.processBlock(buffer, midi);
    juce::Thread::sleep(2);
  }
  setParam(p, "caixa", 1.0f);
  for (int i = 0; i < 400; ++i)
  {
    feed();
    p.processBlock(buffer, midi);
  }

  auto* drive = p.params.getParameter("p1_drive");
  // seconds de áudio; "sweep": drive vai de 1 a 8 e volta.
  auto run = [&](double seconds, bool sweep) {
    Result r;
    std::vector<double> us;
    const int n = static_cast<int>(seconds * rate / block);
    for (int b = 0; b < n; ++b)
    {
      if (sweep)
      {
        const double ph = static_cast<double>(b) / n;
        const double d = 1.0 + 7.0 * (ph < 0.5 ? ph * 2.0 : 2.0 - ph * 2.0);
        drive->setValueNotifyingHost(drive->convertTo0to1(static_cast<float>(d)));
      }
      feed();
      const auto t0 = std::chrono::steady_clock::now();
      p.processBlock(buffer, midi);
      us.push_back(std::chrono::duration<double, std::micro>(std::chrono::steady_clock::now() - t0).count());
      // A placa entrega o próximo bloco só depois de 2,67 ms (como na vida real).
      juce::MessageManager::getInstance()->runDispatchLoopUntil(0);
    }
    std::vector<double> s = us;
    std::sort(s.begin(), s.end());
    double sum = 0;
    for (auto v : us)
    {
      sum += v;
      r.over += v > budgetUs ? 1 : 0;
      r.overHalf += v > budgetUs * 0.5 ? 1 : 0;
    }
    r.blocks = static_cast<int>(us.size());
    r.mean = sum / us.size();
    r.p99 = s[static_cast<size_t>(s.size() * 0.99)];
    r.p999 = s[static_cast<size_t>(s.size() * 0.999)];
    r.worst = s.back();
    return r;
  };
  auto show = [&](const char* name, const Result& r) {
    std::printf("%-46s média %5.0f us (%4.1f%%)  99%% %5.0f  99,9%% %5.0f  pior %6.0f  estouros %d de %d (acima da metade: %d)\n", name, r.mean,
                100.0 * r.mean / budgetUs, r.p99, r.p999, r.worst, r.over, r.blocks, r.overHalf);
  };
  std::printf("Orçamento por bloco: %.0f us\n", budgetUs);
  // Peça por peça (A2 Full, parado): o que cada uma custa.
  {
    std::printf("\nPeça por peça (A2 Full, parado)\n");
    auto settle = [&] {
      for (int i = 0; i < 300; ++i)
      {
        feed();
        p.processBlock(buffer, midi);
      }
    };
    p.setLayout(ChainLayout{});
    setParam(p, "caixa", 0.0f);
    settle();
    show("só o cabeçote", run(3.0, false));
    setParam(p, "caixa", 1.0f);
    settle();
    show("cabeçote + caixa (IR)", run(3.0, false));
    p.setLayout(layout);
    settle();
    show("pedal + cabeçote + caixa", run(3.0, false));
    setParam(p, "ganho", 7.5f);
    settle();
    show("idem, ganho entre duas capturas (7,5)", run(3.0, false));
    setParam(p, "ganho", 7.0f);
    settle();
  }
  for (int lite = 0; lite < 2; ++lite)
  {
    setParam(p, "lite", static_cast<float>(lite));
    setParam(p, "p1_lite", static_cast<float>(lite));
    for (int i = 0; i < 300; ++i)
    {
      feed();
      p.processBlock(buffer, midi);
      juce::MessageManager::getInstance()->runDispatchLoopUntil(1);
    }
    std::printf("\n%s\n", lite ? "A2 Lite" : "A2 Full");
    show("parado (drive 4, ganho 7)", run(4.0, false));
    show("girando o drive sem a mão (automação)", run(4.0, true));
    drive->beginChangeGesture();
    show("girando o drive com a mão no botão", run(4.0, true));
    drive->endChangeGesture();
    show("logo depois de soltar", run(2.0, false));
  }
  return 0;
}
