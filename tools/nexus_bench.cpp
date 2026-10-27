// Ferramenta de medida do motor de troca de capturas e do equalizador.
//
// Uso: nexus_bench <pasta_dos_pacotes_de_teste> [pasta_de_saida] [entrada.wav]
//
// Prova, com números, que:
//  1. o equalizador segue o circuito real (análise nodal) e não mexe no som
//     quando está na posição da captura;
//  2. a troca entre capturas não deixa resto de estado (capturas iguais devem
//     soar exatamente como uma só, mesmo com o botão andando);
//  3. a troca não dá clique (comparado com a troca seca, que dá);
//  4. o áudio não reserva memória e cabe no tempo de cada bloco.
// Sai com código 1 se alguma verificação falhar.

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <complex>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <functional>
#include <new>
#include <string>
#include <vector>

#include "NAM/dsp.h"
#include "NAM/slimmable.h"
#include "Wav.h"
#include "nexus/AmpVoice.h"
#include "nexus/Pack.h"
#include "nexus/TestSignal.h"
#include "nexus/ToneStack.h"
#include "nexus/UniversalEq.h"

// ---------------------------------------------------------------------------
// Contador de reservas de memória (só conta enquanto gCounting está ligado).

static std::atomic<long> gAllocations{0};
static thread_local bool gCounting = false;

static void* countedAlloc(std::size_t n, std::size_t align)
{
  if (gCounting)
    ++gAllocations;
  void* p = nullptr;
#if defined(_MSC_VER)
  p = _aligned_malloc(n ? n : 1, align);
#else
  if (align <= alignof(std::max_align_t))
    p = std::malloc(n ? n : 1);
  else if (posix_memalign(&p, align, n ? n : 1) != 0)
    p = nullptr;
#endif
  if (!p)
    throw std::bad_alloc();
  return p;
}
static void countedFree(void* p) noexcept
{
#if defined(_MSC_VER)
  _aligned_free(p);
#else
  std::free(p);
#endif
}
void* operator new(std::size_t n) { return countedAlloc(n, alignof(std::max_align_t)); }
void* operator new[](std::size_t n) { return countedAlloc(n, alignof(std::max_align_t)); }
void* operator new(std::size_t n, std::align_val_t a) { return countedAlloc(n, static_cast<std::size_t>(a)); }
void* operator new[](std::size_t n, std::align_val_t a) { return countedAlloc(n, static_cast<std::size_t>(a)); }
void operator delete(void* p) noexcept { countedFree(p); }
void operator delete[](void* p) noexcept { countedFree(p); }
void operator delete(void* p, std::size_t) noexcept { countedFree(p); }
void operator delete[](void* p, std::size_t) noexcept { countedFree(p); }
void operator delete(void* p, std::align_val_t) noexcept { countedFree(p); }
void operator delete[](void* p, std::align_val_t) noexcept { countedFree(p); }
void operator delete(void* p, std::size_t, std::align_val_t) noexcept { countedFree(p); }
void operator delete[](void* p, std::size_t, std::align_val_t) noexcept { countedFree(p); }

// ---------------------------------------------------------------------------

using namespace nexus;
namespace fs = std::filesystem;

namespace
{

constexpr double kFs = 48000.0;
int gFailures = 0;

void report(bool ok, const std::string& what, const std::string& detail)
{
  std::printf("  [%s] %s\n         %s\n", ok ? " OK  " : "FALHA", what.c_str(), detail.c_str());
  if (!ok)
    ++gFailures;
}

std::string fmt(const char* f, double a, double b = 0, double c = 0)
{
  char buf[256];
  std::snprintf(buf, sizeof(buf), f, a, b, c);
  return buf;
}

double db(double x) { return 20.0 * std::log10(std::max(x, 1e-300)); }

// ---------------------------------------------------------------------------
// 1. Equalizador

// Resposta exata do circuito por análise nodal (sem usar a fórmula fechada).
std::complex<double> fmvNodal(const FmvCircuit& c, const ToneKnobs& k, double freq)
{
  using cd = std::complex<double>;
  const auto f = potFractions(k);
  const cd s(0.0, 2.0 * 3.14159265358979323846 * freq);
  // Nós: 0 = topo do agudo (T), 1 = cursor/saída (W), 2 = base do agudo (L),
  //      3 = base do grave/topo do médio (M), 4 = junção do resistor (A),
  //      5 = cursor do médio, onde chega o capacitor do médio.
  constexpr int N = 6;
  cd G[N][N] = {};
  cd I[N] = {};
  auto add = [&](int a, int b, cd y) {
    if (a >= 0) G[a][a] += y;
    if (b >= 0) G[b][b] += y;
    if (a >= 0 && b >= 0)
    {
      G[a][b] -= y;
      G[b][a] -= y;
    }
  };
  auto toInput = [&](int a, cd y) { // elemento ligado à entrada (1 V)
    G[a][a] += y;
    I[a] += y;
  };
  toInput(0, s * c.C1);
  add(0, 1, 1.0 / ((1.0 - f.t) * c.R1));
  add(1, 2, 1.0 / (f.t * c.R1));
  add(4, 2, s * c.C2);
  add(2, 3, 1.0 / (f.l * c.R2));
  add(4, 5, s * c.C3);
  add(3, 5, 1.0 / ((1.0 - f.m) * c.R3));
  add(5, -1, 1.0 / (f.m * c.R3));
  toInput(4, 1.0 / c.R4);
  // Eliminação de Gauss.
  for (int col = 0; col < N; ++col)
  {
    int piv = col;
    for (int r = col + 1; r < N; ++r)
      if (std::abs(G[r][col]) > std::abs(G[piv][col]))
        piv = r;
    std::swap(G[col], G[piv]);
    std::swap(I[col], I[piv]);
    for (int r = 0; r < N; ++r)
    {
      if (r == col)
        continue;
      const cd m = G[r][col] / G[col][col];
      for (int k2 = col; k2 < N; ++k2)
        G[r][k2] -= m * G[col][k2];
      I[r] -= m * I[col];
    }
  }
  return I[1] / G[1][1];
}

std::complex<double> fmvFormula(const FmvCircuit& c, const ToneKnobs& k, double freq)
{
  const auto p = fmvPolynomials(c, k);
  const std::complex<double> s(0.0, 2.0 * 3.14159265358979323846 * freq);
  const auto num = s * (p.num[0] + p.num[1] * s + p.num[2] * s * s);
  const auto den = p.den[0] + p.den[1] * s + p.den[2] * s * s + p.den[3] * s * s * s;
  return num / den;
}

std::vector<double> logFreqs(double lo, double hi, int n)
{
  std::vector<double> f(n);
  for (int i = 0; i < n; ++i)
    f[i] = lo * std::pow(hi / lo, double(i) / (n - 1));
  return f;
}

double responseDbAt(const std::vector<double>& impulse, double freq)
{
  std::complex<double> acc = 0.0;
  const double w = 2.0 * 3.14159265358979323846 * freq / kFs;
  for (size_t n = 0; n < impulse.size(); ++n)
    acc += impulse[n] * std::polar(1.0, -w * double(n));
  return db(std::abs(acc));
}

void testToneStack()
{
  std::printf("\n1. Equalizador do painel\n");
  const double knobs[] = {0.0, 2.5, 5.0, 7.5, 10.0};
  for (const char* name : {"marshall", "fender", "alto_ganho"})
  {
    const FmvCircuit c = circuitByName(name);
    double worst = 0.0;
    for (double b : knobs)
      for (double m : knobs)
        for (double t : knobs)
          for (double f : logFreqs(20.0, 20000.0, 40))
          {
            const ToneKnobs k{b, m, t};
            worst = std::max(worst, std::abs(db(std::abs(fmvFormula(c, k, f))) - db(std::abs(fmvNodal(c, k, f)))));
          }
    report(worst < 0.01, std::string("Fórmula do circuito confere com a análise nodal (") + name + ")",
           fmt("maior diferença em 125 posições x 40 frequências: %.6f dB (limite 0,01 dB)", worst));
  }

  // Na posição da captura o sinal sai intacto.
  {
    RelativeToneStack ts;
    ts.setCircuit(circuitByName("marshall"));
    ts.setCaptureKnobs({4.0, 6.0, 7.0});
    ts.prepare(kFs);
    ts.setUserKnobs({4.0, 6.0, 7.0});
    auto x = makeGuitarLikeSignal(kFs, 1.0);
    auto y = x;
    ts.process(y.data(), static_cast<int>(y.size()));
    double diff = 0.0;
    for (size_t i = 0; i < x.size(); ++i)
      diff = std::max(diff, std::abs(x[i] - y[i]));
    report(diff == 0.0, "Na posição da captura o equalizador não muda nada",
           fmt("maior diferença amostra a amostra: %.3g", diff));
  }

  // A versão digital segue a razão entre as duas posições do circuito.
  {
    const FmvCircuit c = circuitByName("marshall");
    const ToneKnobs cap{5.0, 5.0, 5.0}, user{10.0, 0.0, 8.0};
    RelativeToneStack ts;
    ts.setCircuit(c);
    ts.setCaptureKnobs(cap);
    ts.prepare(kFs);
    ts.setUserKnobs(user);
    std::vector<double> imp(static_cast<size_t>(kFs), 0.0);
    imp[0] = 1.0;
    ts.process(imp.data(), static_cast<int>(imp.size()));
    double worst = 0.0, swing = 0.0;
    for (double f : logFreqs(30.0, 5000.0, 30))
    {
      const double expected = db(std::abs(fmvFormula(c, user, f) / fmvFormula(c, cap, f)));
      swing = std::max(swing, std::abs(expected));
      worst = std::max(worst, std::abs(responseDbAt(imp, f) - expected));
    }
    report(worst < 0.3, "Equalizador digital segue o circuito (grave 10, médio 0, agudo 8)",
           fmt("diferença máxima até 5 kHz: %.3f dB (limite 0,3 dB); mudança aplicada até %.1f dB", worst, swing));
  }
}

// ---------------------------------------------------------------------------
// Utilidades de render

void resetPack(Pack& pack, int block, double slim = 1.0)
{
  for (auto& c : pack.captures)
  {
    c.dsp->Reset(kFs, block);
    if (auto* s = dynamic_cast<nam::SlimmableModel*>(c.dsp.get()))
      s->SetSlimmableSize(slim);
  }
}

struct Timing
{
  double totalSec = 0.0;
  std::vector<double> blockUs;
  long allocations = 0;
  int maxRunning = 0;
};

using Automation = std::function<void(double t, AmpVoice::Controls&)>;

// Por bloco: quais capturas estão na mistura (um bit por captura).
using Masks = std::vector<uint64_t>;

std::vector<double> render(std::shared_ptr<Pack> pack, const std::vector<double>& in, int block,
                           const Automation& automation, Masks* masks = nullptr,
                           Timing* timing = nullptr, double slim = 1.0)
{
  resetPack(*pack, block, slim);
  AmpVoice voice;
  AmpVoice::Controls c;
  c.tone = pack->captureTone;
  c.presence = pack->capturePresence;
  automation(0.0, c);
  voice.setControls(c);
  voice.prepare(pack, kFs, block);

  std::vector<double> out(in.size());
  if (masks)
    masks->clear();
  for (size_t pos = 0; pos < in.size(); pos += block)
  {
    const int n = static_cast<int>(std::min<size_t>(block, in.size() - pos));
    automation(pos / kFs, c);
    voice.setControls(c);
    uint64_t before = 0;
    for (size_t k = 0; k < pack->captures.size(); ++k)
      if (voice.morph().weightOf(int(k)) > 0.0)
        before |= uint64_t(1) << k;
    const auto t0 = std::chrono::steady_clock::now();
    gCounting = true;
    voice.process(in.data() + pos, out.data() + pos, n);
    gCounting = false;
    const auto t1 = std::chrono::steady_clock::now();
    if (timing)
    {
      const double us = std::chrono::duration<double, std::micro>(t1 - t0).count();
      timing->blockUs.push_back(us);
      timing->totalSec += us * 1e-6;
      timing->maxRunning = std::max(timing->maxRunning, voice.morph().runningCount());
    }
    if (masks)
    {
      uint64_t mask = before;
      for (size_t k = 0; k < pack->captures.size(); ++k)
        if (voice.morph().weightOf(int(k)) > 0.0)
          mask |= uint64_t(1) << k;
      masks->push_back(mask);
    }
  }
  if (timing)
    timing->allocations = gAllocations.exchange(0);
  else
    gAllocations = 0;
  return out;
}

// Energia de "aspereza" por bloco: terceira diferença ao quadrado (realça o
// extremo agudo, onde mora o clique). Um clique é uma explosão dela que as
// capturas paradas não têm no mesmo instante.
std::vector<double> roughness(const std::vector<double>& y, int frame)
{
  std::vector<double> e((y.size() + frame - 1) / frame, 0.0);
  for (size_t n = 3; n < y.size(); ++n)
  {
    const double d = y[n] - 3.0 * y[n - 1] + 3.0 * y[n - 2] - y[n - 3];
    e[n / frame] += d * d;
  }
  return e;
}

struct ClickResult
{
  double worstDb;
  double atSec;
};

// Compara a aspereza da saída com a das capturas que estavam na mistura
// naquele bloco, tocando sozinhas. Misturar duas capturas nunca deixa o som
// mais áspero que a mais áspera delas; o que passar disso é clique.
ClickResult clickExcess(const std::vector<double>& out, const Masks& masks,
                        const std::vector<std::vector<double>>& staticRough, int frame)
{
  const auto e = roughness(out, frame);
  double floor = 0.0;
  for (const auto& s : staticRough)
    for (double v : s)
      floor += v;
  floor = floor / (staticRough.size() * staticRough[0].size()) * 1e-3;

  ClickResult r{-1e9, 0.0};
  const size_t skip = static_cast<size_t>(0.2 * kFs / frame);
  for (size_t f = skip; f < e.size() && f < masks.size(); ++f)
  {
    const uint64_t mask = masks[f] | masks[f - 1] | (f + 1 < masks.size() ? masks[f + 1] : 0);
    double ref = 0.0;
    for (size_t k = 0; k < staticRough.size(); ++k)
      if (mask & (uint64_t(1) << k))
        for (size_t g = f - 1; g <= std::min(f + 1, staticRough[k].size() - 1); ++g)
          ref = std::max(ref, staticRough[k][g]);
    const double excess = 10.0 * std::log10((e[f] + floor) / (ref + floor));
    if (excess > r.worstDb)
      r = {excess, f * double(frame) / kFs};
  }
  return r;
}

// Troca seca, para mostrar que o medidor pega clique: sempre a captura mais
// próxima, sem transição. "fria" = a captura que entra estava parada.
std::vector<double> renderNaive(Pack& pack, const std::vector<double>& in, int block, const Automation& automation,
                                bool keepAllRunning, Masks* masks)
{
  resetPack(pack, block);
  std::vector<double> out(in.size()), buf(block), inCopy(block);
  masks->clear();
  int current = -1;
  for (size_t pos = 0; pos < in.size(); pos += block)
  {
    const int n = static_cast<int>(std::min<size_t>(block, in.size() - pos));
    AmpVoice::Controls c;
    automation(pos / kFs, c);
    int best = 0;
    for (int k = 0; k < static_cast<int>(pack.captures.size()); ++k)
      if (std::abs(pack.captures[k].position - c.main) < std::abs(pack.captures[best].position - c.main))
        best = k;
    current = best;
    for (int k = 0; k < static_cast<int>(pack.captures.size()); ++k)
    {
      if (k != current && !keepAllRunning)
        continue;
      std::copy(in.begin() + pos, in.begin() + pos + n, inCopy.begin());
      double* ip = inCopy.data();
      double* op = buf.data();
      pack.captures[k].dsp->process(&ip, &op, n);
      if (k == current)
        for (int i = 0; i < n; ++i)
          out[pos + i] = buf[i] * pack.captures[k].levelGain;
    }
    masks->push_back(uint64_t(1) << current);
  }
  return out;
}

// ---------------------------------------------------------------------------

void testIdentical(const fs::path& packsDir, const std::vector<double>& in)
{
  std::printf("\n2. Troca entre capturas iguais (prova do aquecimento)\n");
  PackOptions opt;
  opt.maxBlockSize = 128;
  auto pack = std::shared_ptr<Pack>(loadPack(packsDir / "iguais", opt));

  const auto still = render(pack, in, 128, [](double, AmpVoice::Controls& c) { c.main = 0.0; });
  double peak = 0.0;
  for (double v : still)
    peak = std::max(peak, std::abs(v));

  struct Case
  {
    const char* name;
    Automation a;
  };
  const Case cases[] = {
    {"varredura lenta 0 -> 10 -> 0", [](double t, AmpVoice::Controls& c) { c.main = 10.0 * (1.0 - std::abs(1.0 - t / 4.0)); }},
    {"saltos 0 -> 10 -> 5 -> 0", [](double t, AmpVoice::Controls& c) { c.main = t < 2 ? 0 : t < 4 ? 10 : t < 6 ? 5 : 0; }},
    {"modo degraus com vaivém rápido", [](double t, AmpVoice::Controls& c) {
       c.stepped = true;
       c.main = 5.0 + 5.0 * std::sin(t * 6.0);
     }},
  };
  for (const auto& cs : cases)
  {
    const auto moving = render(pack, in, 128, cs.a);
    double diff = 0.0;
    for (size_t i = 0; i < in.size(); ++i)
      diff = std::max(diff, std::abs(moving[i] - still[i]));
    const double rel = db(diff / peak);
    report(rel < -100.0, std::string("Capturas iguais soam como uma só: ") + cs.name,
           fmt("maior diferença: %.1f dB abaixo do pico (limite -100 dB)", rel));
  }
}

void testClicks(const fs::path& packsDir, const std::vector<double>& in, const fs::path& outDir,
                const std::vector<double>& listen)
{
  std::printf("\n3. Clique na troca de capturas e no equalizador\n");
  PackOptions opt;
  opt.maxBlockSize = 64;
  auto pack = std::shared_ptr<Pack>(loadPack(packsDir / "ganho", opt));
  const int frame = 64;

  std::vector<std::vector<double>> staticRough;
  for (const auto& c : pack->captures)
  {
    const double p = c.position;
    staticRough.push_back(roughness(render(pack, in, 64, [p](double, AmpVoice::Controls& k) { k.main = p; }), frame));
  }

  Masks positions;
  const Automation sweep = [](double t, AmpVoice::Controls& c) { c.main = std::clamp((t - 0.5) * 2.5, 0.0, 10.0); };
  const Automation jumps = [](double t, AmpVoice::Controls& c) {
    const double seq[] = {2, 8, 3, 10, 0, 6, 1, 9};
    c.main = seq[static_cast<int>(t / 1.0) % 8];
  };
  const Automation wiggle = [](double t, AmpVoice::Controls& c) { c.main = 5.0 + 4.5 * std::sin(t * 9.0); };
  const Automation stepped = [](double t, AmpVoice::Controls& c) {
    c.stepped = true;
    c.main = 5.0 + 5.0 * std::sin(t * 2.0);
  };

  // Primeiro a troca seca, para mostrar que o medidor enxerga clique.
  {
    const auto naive = renderNaive(*pack, in, 64, jumps, false, &positions);
    const auto r = clickExcess(naive, positions, staticRough, frame);
    std::printf("  (referência) troca seca com captura fria: pior excesso %+.1f dB em %.2f s\n", r.worstDb, r.atSec);
    const auto naiveWarm = renderNaive(*pack, in, 64, jumps, true, &positions);
    const auto r2 = clickExcess(naiveWarm, positions, staticRough, frame);
    std::printf("  (referência) troca seca com todas rodando: pior excesso %+.1f dB em %.2f s\n", r2.worstDb, r2.atSec);
    report(std::min(r.worstDb, r2.worstDb) > 6.0, "O medidor enxerga o clique da troca seca",
           fmt("troca seca fria %+.1f dB, quente %+.1f dB (as duas precisam passar de +6 dB para o medidor valer)",
               r.worstDb, r2.worstDb));
    wav::write((outDir / "troca_seca_fria.wav").string(), renderNaive(*pack, listen, 64, jumps, false, &positions), kFs);
  }

  struct Case
  {
    const char* name;
    const Automation* a;
    const char* file;
  };
  const Case cases[] = {
    {"varredura 0 -> 10 em 4 s", &sweep, "nexus_varredura.wav"},
    {"saltos de automação (2, 8, 3, 10, 0...)", &jumps, "nexus_saltos.wav"},
    {"vaivém rápido do botão", &wiggle, "nexus_vaivem.wav"},
    {"modo degraus", &stepped, "nexus_degraus.wav"},
  };
  for (const auto& cs : cases)
  {
    const auto out = render(pack, in, 64, *cs.a, &positions);
    const auto r = clickExcess(out, positions, staticRough, frame);
    report(r.worstDb < 3.0, std::string("Sem clique: ") + cs.name,
           fmt("pior excesso de aspereza: %+.1f dB em %.2f s (limite +3 dB)", r.worstDb, r.atSec));
    wav::write((outDir / cs.file).string(), render(pack, listen, 64, *cs.a), kFs);
  }

  // Equalizador: saltos de 0 a 10 em cada botão, comparados com o som parado
  // nas duas pontas.
  struct Knob
  {
    const char* name;
    std::function<void(AmpVoice::Controls&, double)> set;
  };
  const Knob knobs[] = {
    {"grave", [](AmpVoice::Controls& c, double v) { c.tone.bass = v; }},
    {"médio", [](AmpVoice::Controls& c, double v) { c.tone.mid = v; }},
    {"agudo", [](AmpVoice::Controls& c, double v) { c.tone.treble = v; }},
    {"presença", [](AmpVoice::Controls& c, double v) { c.presence = v; }},
  };
  for (const auto& k : knobs)
  {
    auto atValue = [&](double v) {
      return roughness(render(pack, in, 64, [&](double, AmpVoice::Controls& c) { c.main = 5.0; k.set(c, v); }), frame);
    };
    const auto lo = atValue(0.0), hi = atValue(10.0);
    std::vector<std::vector<double>> refs;
    for (size_t i = 0; i < pack->captures.size(); ++i)
    {
      std::vector<double> m(lo.size());
      for (size_t f = 0; f < m.size(); ++f)
        m[f] = std::max(lo[f], hi[f]);
      refs.push_back(m);
    }
    const Automation a = [&](double t, AmpVoice::Controls& c) {
      c.main = 5.0;
      k.set(c, (static_cast<int>(t / 0.5) % 2) ? 10.0 : 0.0);
    };
    const auto out = render(pack, in, 64, a, &positions);
    const auto r = clickExcess(out, positions, refs, frame);
    report(r.worstDb < 3.0, std::string("Sem clique ao saltar o ") + k.name + " de 0 a 10",
           fmt("pior excesso de aspereza: %+.1f dB em %.2f s (limite +3 dB)", r.worstDb, r.atSec));
  }
  wav::write((outDir / "nexus_equalizador.wav").string(),
             render(pack, listen, 64,
                    [](double t, AmpVoice::Controls& c) {
                      c.main = 6.0;
                      c.tone.bass = 5.0 + 5.0 * std::sin(t * 1.3);
                      c.tone.treble = 5.0 + 5.0 * std::sin(t * 0.9 + 1.0);
                      c.tone.mid = 5.0 + 5.0 * std::sin(t * 0.7 + 2.0);
                    }),
             kFs);
}

void testPerformance(const fs::path& packsDir, const std::vector<double>& in)
{
  std::printf("\n4. Tempo real: memória e processador\n");
  for (int block : {64, 128})
  {
    PackOptions opt;
    opt.maxBlockSize = block;
    auto pack = std::shared_ptr<Pack>(loadPack(packsDir / "ganho", opt));
    const double budgetUs = block / kFs * 1e6;
    struct Case
    {
      const char* name;
      Automation a;
    };
    const Case cases[] = {
      {"parado numa captura", [](double, AmpVoice::Controls& c) { c.main = 5.0; }},
      {"parado entre duas (2,5)", [](double, AmpVoice::Controls& c) { c.main = 2.5; }},
      {"varrendo 0 -> 10 -> 0", [](double t, AmpVoice::Controls& c) { c.main = 10.0 * (1.0 - std::abs(1.0 - t / 4.0)); }},
    };
    for (const auto& cs : cases)
    {
      Timing t;
      render(pack, in, block, cs.a, nullptr, &t);
      auto sorted = t.blockUs;
      std::sort(sorted.begin(), sorted.end());
      const double p999 = sorted[static_cast<size_t>(sorted.size() * 0.999)];
      const double load = t.totalSec / (in.size() / kFs) * 100.0;
      std::printf("  bloco %3d, %-26s uso médio %5.1f%%  bloco típico 99,9%%: %6.0f us de %4.0f us  "
                  "capturas rodando no máx.: %d\n",
                  block, cs.name, load, p999, budgetUs, t.maxRunning);
      report(t.allocations == 0, std::string("Sem reservar memória no áudio (bloco ") + std::to_string(block) + ", " + cs.name + ")",
             fmt("reservas durante o processamento: %.0f", double(t.allocations)));
      // Máquinas compartilhadas da compilação automática não têm tempo
      // garantido: lá o tempo é só informado.
      if (std::getenv("NEXUS_CI"))
        std::printf("  [INFO ] tempo por bloco medido em máquina compartilhada, não reprova\n");
      else
        report(p999 < budgetUs, std::string("Cabe no tempo do bloco (bloco ") + std::to_string(block) + ", " + cs.name + ")",
               fmt("99,9%% dos blocos em até %.0f us; limite %.0f us", p999, budgetUs));
    }
  }

  // A2 Full contra A2 Lite (mesmo arquivo).
  for (double slim : {1.0, 0.0})
  {
    PackOptions opt;
    opt.maxBlockSize = 128;
    opt.slimmableSize = slim;
    auto pack = std::shared_ptr<Pack>(loadPack(packsDir / "iguais", opt));
    Timing t;
    render(pack, in, 128, [](double, AmpVoice::Controls& c) { c.main = 0.0; }, nullptr, &t, slim);
    std::printf("  A2 %s: uso médio de uma captura %.1f%% do processador (um núcleo)\n", slim > 0.5 ? "Full" : "Lite",
                t.totalSec / (in.size() / kFs) * 100.0);
  }
}

// Mão no botão: as vizinhas aquecem antes do giro, e o giro não espera.
void testAnticipate(const fs::path& packsDir, const std::vector<double>& in)
{
  std::printf("\n7. Aquecimento antecipado (mão no botão)\n");
  PackOptions opt;
  opt.sampleRate = kFs;
  opt.maxBlockSize = 128;
  auto pack = std::shared_ptr<Pack>(loadPack(packsDir / "ganho", opt));
  resetPack(*pack, 128);
  MorphEngine m;
  m.setTargetPosition(5.0);
  m.prepare(pack, kFs, 128);
  std::vector<double> out(128);
  size_t at = 0;
  auto run = [&](double seconds) {
    for (int b = 0; b < static_cast<int>(seconds * kFs / 128); ++b, at = (at + 128) % (in.size() - 128))
      m.process(in.data() + at, out.data(), 128);
  };
  run(0.5);
  const int before = m.runningCount();
  m.setAnticipate(true);
  run(0.2);
  const int during = m.runningCount();
  // Giro de 5 para 6 depois de aquecer: a 6 já está pronta, então o peso dela
  // sobe já no primeiro bloco (sem segurar a mistura antiga).
  m.setTargetPosition(6.0);
  run(0.3);
  int seven = 0;
  for (int i = 0; i < static_cast<int>(pack->captures.size()); ++i)
    if (std::abs(pack->captures[static_cast<size_t>(i)].position - 6.0) < std::abs(pack->captures[static_cast<size_t>(seven)].position - 6.0))
      seven = i;
  const double w7 = m.weightOf(seven);
  m.setAnticipate(false);
  run(1.0);
  const int after = m.runningCount();
  report(before <= 2 && during >= 3 && during <= 4 && after <= 2, "Com a mão no botão as vizinhas ficam aquecidas; ao soltar, voltam a descansar",
         "rodando: " + std::to_string(before) + " parado, " + std::to_string(during) + " com a mão, " + std::to_string(after) + " depois");
  report(w7 > 0.9, "Girando com as vizinhas aquecidas, a captura nova entra sem esperar", "peso da captura 6 em 300 ms: " + std::to_string(w7));
}

// Equalizador universal (depois da caixa): em 5 não mexe; em 0 e 10 corta e
// reforça cada faixa no lugar certo.
void testUniversalEq()
{
  std::printf("\n8. Equalizador universal\n");
  UniversalEq eq;
  eq.prepare(kFs);
  std::vector<double> x(4096), y;
  for (size_t i = 0; i < x.size(); ++i)
    x[i] = std::sin(0.013 * static_cast<double>(i)) + 0.3 * std::sin(0.31 * static_cast<double>(i));
  y = x;
  eq.process(y.data(), static_cast<int>(y.size()));
  double diff = 0.0;
  for (size_t i = 0; i < x.size(); ++i)
    diff = std::max(diff, std::abs(y[i] - x[i]));
  report(diff == 0.0, "Tudo em 5: o som da captura passa intacto", "maior diferença: " + std::to_string(diff));
  struct Case
  {
    const char* name;
    double UniversalEq::Knobs::*knob;
    double inside, outside;
  };
  const Case cases[] = {{"grave", &UniversalEq::Knobs::bass, 80.0, 3000.0},
                        {"médio", &UniversalEq::Knobs::mid, 750.0, 80.0},
                        {"agudo", &UniversalEq::Knobs::treble, 8000.0, 200.0},
                        {"presença", &UniversalEq::Knobs::presence, 5500.0, 300.0}};
  for (const auto& c : cases)
  {
    UniversalEq::Knobs k;
    k.*(c.knob) = 10.0;
    eq.setKnobs(k);
    eq.prepare(kFs);
    const double up = eq.responseDb(c.inside), side = eq.responseDb(c.outside);
    k.*(c.knob) = 0.0;
    eq.setKnobs(k);
    eq.prepare(kFs);
    const double down = eq.responseDb(c.inside);
    report(up > 8.0 && down < -8.0 && std::abs(side) < 3.0, std::string("Botão de ") + c.name + " mexe na faixa dele",
           "+" + std::to_string(up).substr(0, 4) + " dB / " + std::to_string(down).substr(0, 5) + " dB; fora da faixa " + std::to_string(side).substr(0, 4) + " dB");
  }
}

void testFileNames()
{
  std::printf("\n5. Posição pelo nome do arquivo\n");
  struct Case
  {
    const char* name;
    bool ok;
    double pos;
  };
  const Case cases[] = {{"Ganho 7", true, 7}, {"JCM800 Gain 7", true, 7}, {"JCM800 G07", true, 7},
                        {"drive_7.5", true, 7.5}, {"5150 - 3", true, 3}, {"Gain 2,5 crunch", true, 2.5},
                        {"Crunch", false, 0}};
  int wrong = 0;
  for (const auto& c : cases)
  {
    double p = -1;
    const bool ok = positionFromFileName(c.name, p);
    if (ok != c.ok || (ok && std::abs(p - c.pos) > 1e-9))
    {
      ++wrong;
      std::printf("  \"%s\" -> %s %.2f (esperado %.2f)\n", c.name, ok ? "posição" : "sem número", p, c.pos);
    }
  }
  report(wrong == 0, "Nomes como \"JCM800 Gain 7\", \"G07\", \"drive_7.5\" viram a posição certa",
         fmt("%.0f de 7 nomes errados", double(wrong)));
}

void testFullRig(const fs::path& packsDir)
{
  std::printf("\n6. Captura com caixa (full rig)\n");
  report(captureIncludesCab(packsDir / "fullrig" / "Rig Gain 5.nam"), "Captura marcada \"amp_cab\" no .nam é reconhecida como full rig",
         "tipo lido do próprio arquivo");
  report(!captureIncludesCab(packsDir / "ganho" / "Ganho 5.nam"), "Captura sem tipo de equipamento não tranca a caixa",
         "sem informação, a caixa fica livre");
  PackOptions opt;
  auto rig = loadPack(packsDir / "fullrig", opt);
  auto amp = loadPack(packsDir / "ganho", opt);
  report(rig->includesCab && !amp->includesCab, "Pacote full rig tranca a caixa; pacote de cabeçote não",
         std::string("full rig: ") + (rig->includesCab ? "trancada" : "livre") + "; cabeçote: "
           + (amp->includesCab ? "trancada" : "livre"));
}

} // namespace

int main(int argc, char** argv)
{
  if (argc < 2)
  {
    std::printf("Uso: nexus_bench <pasta_dos_pacotes_de_teste> [pasta_de_saida] [entrada.wav]\n");
    return 2;
  }
  const fs::path packsDir = argv[1];
  const fs::path outDir = argc > 2 ? fs::path(argv[2]) : fs::path("medidas");
  fs::create_directories(outDir);

  const auto in = makeGuitarLikeSignal(kFs, 8.0, 3);
  std::vector<double> listen = in;
  if (argc > 3)
  {
    double sr = 0;
    auto di = wav::read(argv[3], sr);
    listen.clear();
    while (listen.size() < static_cast<size_t>(8 * kFs))
      listen.insert(listen.end(), di.begin(), di.end());
    listen.resize(static_cast<size_t>(8 * kFs));
  }

  std::printf("Nexus Modeler — medida do motor de troca de capturas\n");
  try
  {
    testToneStack();
    testIdentical(packsDir, in);
    testClicks(packsDir, in, outDir, listen);
    testPerformance(packsDir, in);
    testFileNames();
    testFullRig(packsDir);
    testAnticipate(packsDir, in);
    testUniversalEq();
  }
  catch (const std::exception& e)
  {
    std::printf("\nERRO: %s\n", e.what());
    return 1;
  }
  std::printf("\n%s (%d falha%s). Arquivos para ouvir em %s\n", gFailures ? "REPROVADO" : "APROVADO", gFailures,
              gFailures == 1 ? "" : "s", outDir.string().c_str());
  return gFailures ? 1 : 0;
}
