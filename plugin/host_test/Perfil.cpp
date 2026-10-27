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

#include "../../tools/Wav.h"
#include "../source/PluginProcessor.h"
#include "nexus/TestSignal.h"
#include "nexus/Pack.h"
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
  if (argc < 4)
  {
    std::printf("Uso: nexus_perfil <pasta do pedal> <pasta do cabeçote> <ir.wav> [guitarra.wav]\n");
    return 2;
  }
  juce::ScopedJuceInitialiser_GUI gui;
  const double rate = 48000.0;
  const int block = 128;
  const double budgetUs = 1e6 * block / rate;
  std::vector<double> guitar;
  if (argc > 4)
  {
    double r = 0;
    guitar = wav::read(argv[4], r);
  }
  else
    guitar = nexus::makeGuitarLikeSignal(rate, 8.0, 3);

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
