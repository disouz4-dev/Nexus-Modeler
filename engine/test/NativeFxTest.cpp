// Teste dos pedais nativos novos (wah, leslie, delay de balde, oitavador e
// compressor), só com o motor: silêncio continua silêncio, nada explode com
// os botões nos extremos e cada efeito faz o que promete.
//
//   c++ -std=c++20 -O2 -Iengine/include engine/src/Effects.cpp engine/test/NativeFxTest.cpp -o nexus_fx_test
#include "nexus/Effects.h"

#include <cmath>
#include <cstdio>
#include <random>
#include <vector>

namespace
{
constexpr double kRate = 48000.0; // taxa do motor
constexpr int kBlock = 256;
constexpr double kPi = 3.14159265358979323846;
using Type = nexus::NativePedal::Type;
int gFalhas = 0;

void confere(bool ok, const char* nome, const char* fmt, double v)
{
  std::printf("  [%s] %-46s ", ok ? " ok " : "FALHA", nome);
  std::printf(fmt, v);
  std::printf("\n");
  if (!ok)
    ++gFalhas;
}

// Passa o sinal pelo pedal em blocos.
void roda(nexus::NativePedal& p, Type t, bool on, std::vector<double>& x, double a, double b, double c)
{
  for (size_t i = 0; i < x.size(); i += kBlock)
    p.process(t, on, x.data() + i, static_cast<int>(std::min<size_t>(kBlock, x.size() - i)), a, b, c);
}

double rms(const std::vector<double>& x, size_t from, size_t to)
{
  double s = 0.0;
  for (size_t i = from; i < to; ++i)
    s += x[i] * x[i];
  return std::sqrt(s / static_cast<double>(to - from));
}

double pico(const std::vector<double>& x, size_t from, size_t to)
{
  double m = 0.0;
  for (size_t i = from; i < to; ++i)
    m = std::max(m, std::abs(x[i]));
  return m;
}

// Amplitude da componente em hz (Goertzel).
double energia(const std::vector<double>& x, size_t from, size_t to, double hz)
{
  const double w = 2.0 * kPi * hz / kRate;
  double re = 0.0, im = 0.0;
  for (size_t i = from; i < to; ++i)
  {
    re += x[i] * std::cos(w * static_cast<double>(i));
    im += x[i] * std::sin(w * static_cast<double>(i));
  }
  return 2.0 * std::sqrt(re * re + im * im) / static_cast<double>(to - from);
}

// Guitarra de mentira: notas palhetadas com harmônicos que morrem.
std::vector<double> guitarra(double seconds, double peak)
{
  std::vector<double> x(static_cast<size_t>(seconds * kRate));
  const double notas[] = {82.4, 110.0, 146.8, 196.0, 246.9, 329.6};
  const size_t nota = static_cast<size_t>(0.5 * kRate);
  for (size_t i = 0; i < x.size(); ++i)
  {
    const double f = notas[(i / nota) % 6];
    const double t = static_cast<double>(i % nota) / kRate;
    double v = 0.0;
    for (int h = 1; h <= 6; ++h)
      v += std::sin(2.0 * kPi * f * h * t) / h * std::exp(-t * (3.0 + h));
    x[i] = v * peak / 1.6;
  }
  return x;
}

const char* nomeDe(Type t)
{
  switch (t)
  {
    case Type::wah: return "wah";
    case Type::rotary: return "rotary";
    case Type::analogdelay: return "analogdelay";
    case Type::octave: return "octave";
    case Type::comp: return "comp";
    default: return "?";
  }
}

// Equalizador gráfico (Acerola EQ): o tom de hz passa com os botões e o nível
// dados; devolve o ganho em dB do tom depois que tudo assentou.
double ganhoEq(const nexus::NativePedal::Knobs& k, double nivel, double hz)
{
  nexus::NativePedal p;
  p.prepare(kRate, kBlock);
  std::vector<double> x(static_cast<size_t>(1.0 * kRate));
  for (size_t i = 0; i < x.size(); ++i)
    x[i] = 0.1 * std::sin(2.0 * kPi * hz * static_cast<double>(i) / kRate);
  for (size_t i = 0; i < x.size(); i += kBlock)
    p.process(Type::geq, true, x.data() + i, static_cast<int>(std::min<size_t>(kBlock, x.size() - i)), k, nivel);
  const size_t from = static_cast<size_t>(0.5 * kRate);
  return 20.0 * std::log10(energia(x, from, x.size(), hz) / 0.1);
}

void testaEq()
{
  std::printf("geq\n");
  const nexus::NativePedal::Knobs plano{0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5};
  // 1. Tudo no meio: o som passa intacto (guitarra entra e sai igual).
  {
    nexus::NativePedal p;
    p.prepare(kRate, kBlock);
    const auto seco = guitarra(2.0, 0.5);
    auto x = seco;
    for (size_t i = 0; i < x.size(); i += kBlock)
      p.process(Type::geq, true, x.data() + i, static_cast<int>(std::min<size_t>(kBlock, x.size() - i)), plano, 0.5);
    double dif = 0.0;
    for (size_t i = 0; i < x.size(); ++i)
      dif = std::max(dif, std::abs(x[i] - seco[i]));
    confere(dif < 1e-9, "tudo no meio: transparente (maior diferenca)", "%g", dif);
  }
  // 2. 800 Hz no máximo (+15 dB): o tom de 800 Hz sobe ~15 dB; 100 Hz e
  // 6,4 kHz quase não mudam.
  auto k800 = plano;
  k800[3] = 1.0;
  const double g800 = ganhoEq(k800, 0.5, 800.0);
  confere(std::abs(g800 - 15.0) < 0.5, "800 Hz em +15 dB: ganho em 800 Hz (dB)", "%.2f", g800);
  const double g100 = ganhoEq(k800, 0.5, 100.0);
  confere(std::abs(g100) < 1.0, "800 Hz em +15 dB: ganho em 100 Hz (dB)", "%.2f", g100);
  const double g6k = ganhoEq(k800, 0.5, 6400.0);
  confere(std::abs(g6k) < 1.0, "800 Hz em +15 dB: ganho em 6,4 kHz (dB)", "%.2f", g6k);
  // Corte também: 800 Hz no mínimo tira ~15 dB.
  auto k800c = plano;
  k800c[3] = 0.0;
  const double c800 = ganhoEq(k800c, 0.5, 800.0);
  confere(std::abs(c800 + 15.0) < 0.5, "800 Hz em -15 dB: ganho em 800 Hz (dB)", "%.2f", c800);
  // 3. Prateleira de 6,4 kHz: reforça o agudo e deixa o grave.
  auto kAgudo = plano;
  kAgudo[6] = 1.0;
  const double a10k = ganhoEq(kAgudo, 0.5, 12000.0);
  confere(a10k > 13.0 && a10k < 15.5, "6,4 kHz em +15 dB: ganho em 12 kHz (dB)", "%.2f", a10k);
  const double a100 = ganhoEq(kAgudo, 0.5, 100.0);
  confere(std::abs(a100) < 0.5, "6,4 kHz em +15 dB: ganho em 100 Hz (dB)", "%.2f", a100);
  // 4. Nível: +15 dB e -15 dB com as faixas planas.
  const double nMais = ganhoEq(plano, 1.0, 1000.0);
  confere(std::abs(nMais - 15.0) < 0.1, "nivel no maximo: ganho (dB)", "%.2f", nMais);
  const double nMenos = ganhoEq(plano, 0.0, 1000.0);
  confere(std::abs(nMenos + 15.0) < 0.1, "nivel no minimo: ganho (dB)", "%.2f", nMenos);
}
} // namespace

int main()
{
  const Type tipos[] = {Type::wah, Type::rotary, Type::analogdelay, Type::octave, Type::comp};
  for (Type t : tipos)
  {
    std::printf("%s\n", nomeDe(t));
    nexus::NativePedal p;
    p.prepare(kRate, kBlock);

    // 1. Silêncio entra, silêncio sai (de 0 e de 1 nos botões).
    double maxSilencio = 0.0;
    for (double k : {0.0, 0.5, 1.0})
    {
      p.reset();
      std::vector<double> x(static_cast<size_t>(2.0 * kRate), 0.0);
      roda(p, t, true, x, k, k, k);
      maxSilencio = std::max(maxSilencio, pico(x, 0, x.size()));
    }
    confere(maxSilencio == 0.0, "silencio -> silencio (pico)", "%g", maxSilencio);

    // 2. 20 s de sinal alto com cada combinação dos botões em 0 e 1.
    std::mt19937 rng(7);
    std::normal_distribution<double> ruido(0.0, 0.3);
    double maxSaida = 0.0;
    bool finito = true;
    for (int combo = 0; combo < 8; ++combo)
    {
      p.reset();
      const double a = combo & 1, b = (combo >> 1) & 1, c = (combo >> 2) & 1;
      std::vector<double> x(static_cast<size_t>(20.0 * kRate));
      for (size_t i = 0; i < x.size(); ++i)
        x[i] = 1.5 * std::sin(2.0 * kPi * 110.0 * static_cast<double>(i) / kRate) + ruido(rng);
      roda(p, t, true, x, a, b, c);
      for (double v : x)
        finito = finito && std::isfinite(v);
      maxSaida = std::max(maxSaida, pico(x, 0, x.size()));
    }
    confere(finito && maxSaida < 40.0, "20 s alto, botoes 0/1: pico da saida", "%.2f", maxSaida);

    // 3. Desligar: depois do fim, volta ao seco (ou o rastro morre) e o
    // reset deixa tudo limpo.
    {
      p.reset();
      auto x = guitarra(2.0, 0.5);
      roda(p, t, true, x, 0.5, 0.5, 0.5);
      auto seco = guitarra(1.0, 0.5);
      auto y = seco;
      roda(p, t, false, y, 0.5, 0.5, 0.5);
      // Pulo na hora de desligar (sem estalo): diferença entre amostras vizinhas.
      double pulo = 0.0;
      for (size_t i = 1; i < 2000; ++i)
        pulo = std::max(pulo, std::abs(y[i] - y[i - 1]));
      double resto = 0.0;
      const size_t depois = static_cast<size_t>(0.1 * kRate);
      for (size_t i = depois; i < y.size(); ++i)
        resto = std::max(resto, std::abs(y[i] - seco[i]));
      if (t == Type::analogdelay)
        confere(true, "desligado: rastro depois de 100 ms (pico)", "%.4f", resto);
      else
        confere(resto < 1e-12, "desligado: igual ao seco depois de 100 ms", "%g", resto);
      confere(pulo < 0.2, "desligar sem estalo (maior passo)", "%.4f", pulo);
      // Silêncio depois do som: morre e não fica DC.
      std::vector<double> s(static_cast<size_t>(8.0 * kRate), 0.0);
      roda(p, t, false, s, 0.5, 0.5, 0.5);
      confere(pico(s, s.size() - static_cast<size_t>(kRate), s.size()) < 1e-6, "silencio depois: ultimo segundo", "%g",
              pico(s, s.size() - static_cast<size_t>(kRate), s.size()));
      auto z = guitarra(1.0, 0.5);
      roda(p, t, true, z, 1.0, 1.0, 1.0);
      p.reset();
      std::vector<double> q(static_cast<size_t>(kRate), 0.0);
      roda(p, t, true, q, 1.0, 1.0, 1.0);
      confere(pico(q, 0, q.size()) == 0.0, "reset: silencio depois", "%g", pico(q, 0, q.size()));
    }

    // 4. O efeito de cada um.
    if (t == Type::wah)
    {
      // Frequência de maior ganho com o pedal no calcanhar, no meio e na ponta.
      for (double pos : {0.0, 0.5, 1.0})
      {
        double melhor = 0.0, melhorHz = 0.0;
        for (double hz = 200.0; hz < 4000.0; hz *= 1.03)
        {
          nexus::NativePedal w;
          w.prepare(kRate, kBlock);
          std::vector<double> x(static_cast<size_t>(0.2 * kRate));
          for (size_t i = 0; i < x.size(); ++i)
            x[i] = 0.1 * std::sin(2.0 * kPi * hz * static_cast<double>(i) / kRate);
          roda(w, t, true, x, pos, 0.5, 0.5);
          const double g = rms(x, x.size() / 2, x.size()) / (0.1 / std::sqrt(2.0));
          if (g > melhor)
          {
            melhor = g;
            melhorHz = hz;
          }
        }
        char nome[64];
        std::snprintf(nome, sizeof nome, "pedal %.1f: pico em Hz (ganho %.1f dB)", pos, 20.0 * std::log10(melhor));
        const double esperado = nexus::Wah::centerHz(pos, 0.5);
        confere(std::abs(std::log2(melhorHz / esperado)) < 0.15, nome, "%.0f", melhorHz);
      }
    }
    if (t == Type::octave)
    {
      for (int qual = 1; qual <= 2; ++qual)
      {
        p.reset();
        std::vector<double> x(static_cast<size_t>(2.0 * kRate));
        for (size_t i = 0; i < x.size(); ++i)
          x[i] = 0.3 * std::sin(2.0 * kPi * 110.0 * static_cast<double>(i) / kRate);
        roda(p, t, true, x, 0.0, qual == 1 ? 1.0 : 0.0, qual == 2 ? 1.0 : 0.0);
        const size_t de = static_cast<size_t>(kRate), ate = x.size();
        const double sub = energia(x, de, ate, qual == 1 ? 55.0 : 27.5);
        const double orig = energia(x, de, ate, 110.0);
        char nome[64];
        std::snprintf(nome, sizeof nome, "%s: amplitude em %s Hz (110 Hz: %.3f)", qual == 1 ? "-1 oit" : "-2 oit",
                      qual == 1 ? "55" : "27,5", orig);
        confere(sub > 0.1 && sub > 3.0 * orig, nome, "%.3f", sub);
      }
    }
    if (t == Type::comp)
    {
      const auto seco = guitarra(6.0, 0.5);
      const size_t de = static_cast<size_t>(0.5 * kRate);
      const double crestSeco = pico(seco, de, seco.size()) / rms(seco, de, seco.size());
      auto x = seco;
      p.reset();
      roda(p, t, true, x, 0.5, 1.0, 0.0);
      const double crest = pico(x, de, x.size()) / rms(x, de, x.size());
      char nome[64];
      std::snprintf(nome, sizeof nome, "pico/media em dB, sens 1 (seco %.1f dB)", 20.0 * std::log10(crestSeco));
      confere(crest < crestSeco / 1.2, nome, "%.1f", 20.0 * std::log10(crest));
      for (double peak : {0.25, 0.355, 0.5})
      {
        const auto s = guitarra(6.0, peak);
        auto y = s;
        p.reset();
        roda(p, t, true, y, 0.5, 0.5, 0.5);
        const double diff = 20.0 * std::log10(rms(y, de, y.size()) / rms(s, de, s.size()));
        std::snprintf(nome, sizeof nome, "botoes no meio, pico %.0f dB: ligado - bypass (dB)", 20.0 * std::log10(peak));
        // O nível típico (-9 dB) precisa ficar a 2 dB; os outros, a 3 dB.
        confere(std::abs(diff) < (peak == 0.355 ? 2.0 : 3.0), nome, "%+.2f", diff);
      }
    }
    if (t == Type::analogdelay)
    {
      // O toque vem depois de 50 ms: a entrada do pedal leva 20 ms para abrir.
      const size_t kToque = static_cast<size_t>(0.05 * kRate);
      for (double tempo : {0.0, 0.5, 1.0})
      {
        p.reset();
        std::vector<double> x(static_cast<size_t>(1.5 * kRate), 0.0);
        for (size_t i = 0; i < 96; ++i)
          x[kToque + i] = 0.8 * std::sin(2.0 * kPi * 500.0 * static_cast<double>(i) / kRate);
        auto seco = x;
        roda(p, t, true, x, tempo, 0.0, 1.0);
        // Onde está o eco: o maior pico depois do toque.
        double m = 0.0;
        size_t onde = 0;
        for (size_t i = kToque + 200; i < x.size(); ++i)
          if (std::abs(x[i] - seco[i]) > m)
          {
            m = std::abs(x[i] - seco[i]);
            onde = i;
          }
        const double esperado = nexus::AnalogDelay::seconds(tempo) * 1000.0;
        const double achado = static_cast<double>(onde - kToque) / kRate * 1000.0;
        char nome[64];
        std::snprintf(nome, sizeof nome, "tempo %.1f: eco em ms (esperado %.0f, pico %.2f)", tempo, esperado, m);
        confere(std::abs(achado - esperado) < 3.0 && m > 0.1, nome, "%.1f", achado);
      }
      // Andamento: 375 ms (colcheia a 80 bpm).
      p.reset();
      p.setSync(0.0, 0.375);
      std::vector<double> x(static_cast<size_t>(1.0 * kRate), 0.0);
      for (size_t i = 0; i < 96; ++i)
        x[kToque + i] = 0.8 * std::sin(2.0 * kPi * 500.0 * static_cast<double>(i) / kRate);
      auto seco = x;
      roda(p, t, true, x, 0.0, 0.0, 1.0);
      size_t onde = 0;
      double m = 0.0;
      for (size_t i = kToque + 200; i < x.size(); ++i)
        if (std::abs(x[i] - seco[i]) > m)
        {
          m = std::abs(x[i] - seco[i]);
          onde = i;
        }
      confere(std::abs(static_cast<double>(onde - kToque) / kRate - 0.375) < 0.003, "andamento 375 ms: eco em ms", "%.1f",
              static_cast<double>(onde - kToque) / kRate * 1000.0);
      p.setSync(0.0, 0.0);
    }
    if (t == Type::rotary)
    {
      // Inércia: de lento para rápido, a corneta chega antes do tambor.
      nexus::Rotary r;
      r.prepare(kRate);
      std::vector<double> x(static_cast<size_t>(kRate), 0.0);
      r.process(x.data(), 1, 0.0, 0.5, 0.5);
      r.process(x.data(), static_cast<int>(x.size()), 1.0, 0.5, 0.5);
      std::printf("         depois de 1 s no rapido: corneta %.2f Hz, tambor %.2f Hz\n", r.hornHz(), r.drumHz());
      confere(r.hornHz() > 5.0 && r.drumHz() < 3.5, "inercia: corneta mais rapida que o tambor", "%.2f",
              r.hornHz() - r.drumHz());
      // Modulação: o volume de um tom agudo balança.
      p.reset();
      std::vector<double> y(static_cast<size_t>(3.0 * kRate));
      for (size_t i = 0; i < y.size(); ++i)
        y[i] = 0.3 * std::sin(2.0 * kPi * 2000.0 * static_cast<double>(i) / kRate);
      roda(p, t, true, y, 0.0, 0.0, 1.0);
      double lo = 1e9, hi = 0.0;
      for (size_t i = static_cast<size_t>(kRate); i + 480 < y.size(); i += 480)
      {
        const double v = rms(y, i, i + 480);
        lo = std::min(lo, v);
        hi = std::max(hi, v);
      }
      confere(hi / lo > 1.4, "corneta: balanco de volume (dB)", "%.1f", 20.0 * std::log10(hi / lo));
    }
  }
  testaEq();
  std::printf(gFalhas == 0 ? "\ntudo certo\n" : "\n%d falha(s)\n", gFalhas);
  return gFalhas == 0 ? 0 : 1;
}
