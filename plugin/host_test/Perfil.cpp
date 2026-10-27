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
