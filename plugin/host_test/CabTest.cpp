// Testa o bloco da caixa por dentro do plugin:
//   1. com captura só de cabeçote, o IR muda o som;
//   2. com captura full rig, o IR fica trancado: ligar ou desligar a caixa
//      não muda nada no som;
//   3. a tela mostra a caixa nos dois casos (fotos para conferir).
//
// Uso: nexus_cab_test <pasta_dos_pacotes_de_teste> [pasta_das_fotos]

#include <cmath>
#include <csignal>
#include <cstdio>

#include "../source/PluginEditor.h"
#include "../source/PluginProcessor.h"
#include "../source/ui/PackMapper.h"
#include "../source/report/Report.h"
#include "../source/update/Updater.h"
#include "nexus/Pack.h"
#include "nexus/TestSignal.h"

namespace
{
int gFailures = 0;

void report(bool ok, const char* what, const juce::String& detail)
{
  std::printf("  [%s] %s\n         %s\n", ok ? " OK  " : "FALHA", what, detail.toRawUTF8());
  if (!ok)
    ++gFailures;
}

void setParam(NexusProcessor& p, const char* id, float plain)
{
  auto* param = p.params.getParameter(id);
  param->setValueNotifyingHost(param->convertTo0to1(plain));
}

// Toca o sinal de teste e devolve o canal esquerdo da saída.
std::vector<float> play(NexusProcessor& p, const std::vector<double>& signal)
{
  juce::AudioBuffer<float> buffer(2, 128);
  juce::MidiBuffer midi;
  std::vector<float> out;
  for (size_t start = 0; start + 128 <= signal.size(); start += 128)
  {
    for (int ch = 0; ch < 2; ++ch)
      for (int i = 0; i < 128; ++i)
        buffer.setSample(ch, i, static_cast<float>(signal[start + static_cast<size_t>(i)]));
    p.processBlock(buffer, midi);
    out.insert(out.end(), buffer.getReadPointer(0), buffer.getReadPointer(0) + 128);
  }
  return out;
}

// Espera o pacote carregar (e o IR entrar), tocando silêncio.
void settle(NexusProcessor& p, int ms)
{
  juce::AudioBuffer<float> buffer(2, 128);
  juce::MidiBuffer midi;
  for (int t = 0; t < ms; t += 5)
  {
    buffer.clear();
    p.processBlock(buffer, midi);
    juce::MessageManager::getInstance()->runDispatchLoopUntil(5);
  }
}

// Espera o pacote terminar de carregar (até 2 minutos em máquina lenta).
bool waitPack(NexusProcessor& p)
{
  for (int t = 0; t < 120000 && !p.channelLoaded(0, 0); t += 50)
    settle(p, 50);
  settle(p, 300);
  return p.channelLoaded(0, 0);
}

double diffDb(const std::vector<float>& a, const std::vector<float>& b)
{
  double num = 0.0, den = 1e-30;
  for (size_t i = 0; i < std::min(a.size(), b.size()); ++i)
  {
    num += (a[i] - b[i]) * (a[i] - b[i]);
    den += a[i] * a[i];
  }
  return 10.0 * std::log10(num / den + 1e-30);
}

void snapshot(NexusProcessor& p, const juce::File& out)
{
  std::unique_ptr<juce::AudioProcessorEditor> editor(p.createEditor());
  if (auto* e = dynamic_cast<NexusEditor*>(editor.get()))
    e->select({NexusEditor::Kind::cab, 0});
  juce::MessageManager::getInstance()->runDispatchLoopUntil(200);
  const auto image = editor->createComponentSnapshot(editor->getLocalBounds(), true, 2.0f);
  out.deleteFile();
  juce::FileOutputStream stream(out);
  juce::PNGImageFormat().writeImageToStream(image, stream);
}
template <typename T>
void collect(juce::Component& root, std::vector<T*>& out)
{
  for (auto* c : root.getChildren())
  {
    if (auto* t = dynamic_cast<T*>(c))
      out.push_back(t);
    collect(*c, out);
  }
}

juce::MouseEvent mouseAt(juce::Component& c, juce::Point<float> pos, juce::Point<float> downPos, int clicks, bool dragged,
                         juce::ModifierKeys mods = {})
{
  const auto now = juce::Time::getCurrentTime();
  return juce::MouseEvent(juce::Desktop::getInstance().getMainMouseSource(), pos, mods, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, &c, &c, now,
                          downPos, now, clicks, dragged);
}

// Visível dentro do plugin (a janela não abre no teste, então isShowing não serve).
bool visibleIn(juce::Component& c, juce::Component& root)
{
  for (auto* x = &c; x != nullptr && x != &root; x = x->getParentComponent())
    if (!x->isVisible())
      return false;
  return true;
}

// Usa a tela como uma pessoa: arrasta botões, duplo clique, troca de peça.
void testScreen(NexusProcessor& p)
{
  std::unique_ptr<juce::AudioProcessorEditor> editor(p.createEditor());
  auto* e = dynamic_cast<NexusEditor*>(editor.get());
  std::vector<nexus::ui::FilmKnob*> all, knobs;
  collect(*editor, all);
  for (auto* k : all)
    if (visibleIn(*k, *editor))
      knobs.push_back(k);
  report(knobs.size() == 6, "Os 6 botões renderizados do Brit 800 estão na tela",
         juce::String(static_cast<int>(knobs.size())) + juce::String::fromUTF8(" botões encontrados"));
  if (knobs.empty() || e == nullptr)
    return;

  auto* gain = p.params.getParameter("ganho");
  auto& k = *knobs.front();
  const auto c = k.getLocalBounds().toFloat().getCentre();
  gain->setValueNotifyingHost(gain->convertTo0to1(5.0f));
  juce::MessageManager::getInstance()->runDispatchLoopUntil(50);
  k.mouseDown(mouseAt(k, c, c, 1, false));
  k.mouseDrag(mouseAt(k, c.translated(0.0f, -100.0f), c, 1, true));
  k.mouseUp(mouseAt(k, c.translated(0.0f, -100.0f), c, 1, true));
  const float afterDrag = gain->convertFrom0to1(gain->getValue());
  report(std::abs(afterDrag - 9.0f) < 0.05f, "Arrastar o ganho 100 pontos para cima leva de 5 a 9",
         juce::String::fromUTF8("ganho depois de arrastar: ") + juce::String(afterDrag, 2));

  // Ganho tem estágios: arrastar um pouco não sai do inteiro.
  k.mouseDown(mouseAt(k, c, c, 1, false));
  k.mouseDrag(mouseAt(k, c.translated(0.0f, 30.0f), c, 1, true));
  k.mouseUp(mouseAt(k, c.translated(0.0f, 30.0f), c, 1, true));
  const float afterStep = gain->convertFrom0to1(gain->getValue());
  report(std::abs(afterStep - 8.0f) < 0.001f, "Ganho anda em estágios de clique (0 a 10)",
         juce::String::fromUTF8("30 pontos para baixo a partir de 9: ") + juce::String(afterStep, 2));

  // Agudo gira livre; com Shift o ajuste é 5 vezes mais fino.
  auto* treble = p.params.getParameter("agudo");
  nexus::ui::FilmKnob* trebleKnob = nullptr;
  for (auto* kn : knobs)
    if (kn->art().id == "agudo")
      trebleKnob = kn;
  treble->setValueNotifyingHost(treble->convertTo0to1(5.0f));
  juce::MessageManager::getInstance()->runDispatchLoopUntil(50);
  if (trebleKnob != nullptr)
  {
    auto& t = *trebleKnob;
    const auto tc = t.getLocalBounds().toFloat().getCentre();
    t.mouseDown(mouseAt(t, tc, tc, 1, false));
    t.mouseDrag(mouseAt(t, tc.translated(0.0f, -100.0f), tc, 1, true, juce::ModifierKeys::shiftModifier));
    t.mouseUp(mouseAt(t, tc.translated(0.0f, -100.0f), tc, 1, true));
  }
  const float afterFine = treble->convertFrom0to1(treble->getValue());
  report(std::abs(afterFine - 5.8f) < 0.05f, "Agudo gira livre; com Shift o ajuste é 5 vezes mais fino",
         juce::String::fromUTF8("100 pontos para cima com Shift: ") + juce::String(afterFine, 2));

  k.mouseDoubleClick(mouseAt(k, c, c, 2, false));
  const float afterReset = gain->convertFrom0to1(gain->getValue());
  report(std::abs(afterReset - 5.0f) < 0.01f, "Duplo clique volta o botão ao padrão",
         juce::String::fromUTF8("ganho depois do duplo clique: ") + juce::String(afterReset, 2));

  e->select({NexusEditor::Kind::cab, 0});
  std::vector<nexus::ui::Segmented*> tabs, segs;
  collect(*editor, segs);
  for (auto* t : segs)
    if (visibleIn(*t, *editor) && t->getParentComponent() != editor.get() && t->numItems() == 3
        && t->findParentComponentOfClass<nexus::ui::GearBrowser>() == nullptr)
      tabs.push_back(t);
  const bool tabsVisible = tabs.size() == 1 && visibleIn(*tabs.front(), *editor);
  const bool knobsHidden = !visibleIn(*knobs.front(), *editor);
  report(tabsVisible && knobsHidden, "Trocar para a caixa mostra as abas da caixa e esconde o amplificador",
         juce::String::fromUTF8(tabsVisible ? "abas visíveis" : "abas escondidas"));
  e->select({NexusEditor::Kind::amp, 0});
  // A tela do cabeçote monta os botões de novo: os ponteiros antigos não valem mais.
  std::vector<nexus::ui::FilmKnob*> again;
  collect(*editor, again);
  int shown = 0;
  for (auto* kn : again)
    shown += visibleIn(*kn, *editor) ? 1 : 0;
  const bool back = shown == 6 && (tabs.empty() || !visibleIn(*tabs.front(), *editor));
  report(back, "Voltar ao amplificador mostra os botões de novo",
         juce::String::fromUTF8(back ? "botões visíveis, abas escondidas"
                                     : shown == 6 ? "abas da caixa continuam visíveis" : "botões escondidos"));
}
} // namespace

int main(int argc, char** argv)
{
  // Só à mão: NEXUS_TESTE_TRAVAR=1 instala o tratador e trava de propósito,
  // para conferir o travamento.txt (o ctest não usa).
  // Só à mão: NEXUS_TESTE_SEPARAR=<música> separa pelo caminho do programa
  // (baixa o modelo se faltar) e grava 20 s sem a guitarra a 75% em
  // NEXUS_TESTE_SAIDA (wav).
  if (const auto music = juce::SystemStats::getEnvironmentVariable("NEXUS_TESTE_SEPARAR", {}); music.isNotEmpty())
  {
    juce::ScopedJuceInitialiser_GUI gui;
    namespace st = nexus::studio;
    st::Separation job;
    juce::String songId, error;
    bool finished = false;
    const auto t0 = juce::Time::getMillisecondCounterHiRes();
    job.start(juce::File(music), [&](const juce::String& id, const juce::String& e) {
      songId = id;
      error = e;
      finished = true;
    });
    int lastStage = -1, lastPc = -10;
    while (!finished)
    {
      juce::MessageManager::getInstance()->runDispatchLoopUntil(200);
      const int stage = static_cast<int>(job.stage()), pc = static_cast<int>(job.progress() * 100.0f);
      if (stage != lastStage || pc >= lastPc + 10)
      {
        std::printf("  etapa %d  %d%%  (%.0f s)\n", stage, pc, (juce::Time::getMillisecondCounterHiRes() - t0) / 1000.0);
        lastStage = stage;
        lastPc = pc;
      }
    }
    std::printf("Terminou em %.0f s: %s %s\n", (juce::Time::getMillisecondCounterHiRes() - t0) / 1000.0, songId.toRawUTF8(), error.toRawUTF8());
    st::Library lib;
    lib.rescan();
    const auto* song = lib.find(songId);
    if (song == nullptr)
      return 1;
    std::printf("Na biblioteca: %s, %.1f s, forma com %d pontos\n", song->title.toRawUTF8(), song->seconds, static_cast<int>(song->shape.size()));
    const auto outPath = juce::SystemStats::getEnvironmentVariable("NEXUS_TESTE_SAIDA", {});
    if (outPath.isNotEmpty())
    {
      st::Player player;
      player.prepare(48000.0, 512);
      player.open(*song);
      auto mix = song->mix;
      mix.mute[st::guitar] = true;
      mix.speed = 0.75;
      player.setMix(mix);
      std::vector<float> l(512), r(512);
      for (int k = 0; k < 40; ++k)
      {
        player.process(l.data(), r.data(), 512);
        juce::Thread::sleep(5);
      }
      player.seek(std::min(30.0, song->seconds * 0.3));
      player.setPlaying(true);
      juce::AudioBuffer<float> out(2, 20 * 48000);
      out.clear();
      for (int k = 0; k + 512 <= out.getNumSamples(); k += 512)
      {
        player.process(out.getWritePointer(0, k), out.getWritePointer(1, k), 512);
        juce::Thread::sleep(2);
      }
      juce::WavAudioFormat wav;
      juce::File(outPath).deleteFile();
      auto stream = std::make_unique<juce::FileOutputStream>(juce::File(outPath));
      std::unique_ptr<juce::AudioFormatWriter> w(wav.createWriterFor(stream.get(), 48000.0, 2, 24, {}, 0));
      stream.release();
      w->writeFromAudioSampleBuffer(out, 0, out.getNumSamples());
      std::printf("Gravado: %s\n", outPath.toRawUTF8());
    }
    return 0;
  }
  if (juce::SystemStats::getEnvironmentVariable("NEXUS_TESTE_TRAVAR", {}) == "1")
  {
    nexus::report::installCrashHandler();
    std::raise(SIGSEGV);
  }

  if (argc < 2)
  {
    std::printf("Uso: nexus_cab_test <pasta_dos_pacotes_de_teste> [pasta_das_fotos]\n");
    return 2;
  }
  juce::ScopedJuceInitialiser_GUI juce;
  const auto cwd = juce::File::getCurrentWorkingDirectory();
  const juce::File packs = cwd.getChildFile(juce::String::fromUTF8(argv[1]));
  const juce::File photos = argc > 2 ? cwd.getChildFile(juce::String::fromUTF8(argv[2])) : juce::File();
  const juce::File ir = packs.getChildFile("ir_teste.wav");
  const auto signal = nexus::makeGuitarLikeSignal(48000.0, 2.0, 5);

  std::printf("Nexus Modeler — teste do bloco da caixa\n");

  // 1. Cabeçote: o IR tem de mudar o som.
  {
    NexusProcessor p;
    p.setPlayConfigDetails(2, 2, 48000.0, 128);
    p.prepareToPlay(48000.0, 128);
    p.loadAmpPack(0, 0, packs.getChildFile("ganho"));
    if (!waitPack(p))
      report(false, "Pacote de teste carrega", "não carregou em 2 minutos");
    setParam(p, "caixa", 0.0f);
    settle(p, 100);
    const auto dry = play(p, signal);
    p.loadIrAsync(0, ir);
    setParam(p, "caixa", 1.0f);
    settle(p, 1500);
    const auto wet = play(p, signal);
    const double d = diffDb(dry, wet);
    report(!p.packIncludesCab(0) && p.irLoaded(0) && d > -20.0, "Captura de cabeçote: o IR muda o som",
           juce::String::fromUTF8("diferença entre sem e com caixa: ") + juce::String(d, 1) + " dB");
    if (photos != juce::File())
      snapshot(p, photos.getChildFile("caixa_com_ir.png"));
    std::printf("\nTela\n");
    testScreen(p);
  }

  // 2. Full rig: a caixa fica trancada, o IR não toca.
  {
    NexusProcessor p;
    p.setPlayConfigDetails(2, 2, 48000.0, 128);
    p.prepareToPlay(48000.0, 128);
    p.loadIrAsync(0, ir);
    p.loadAmpPack(0, 0, packs.getChildFile("fullrig"));
    if (!waitPack(p))
      report(false, "Pacote full rig carrega", "não carregou em 2 minutos");
    setParam(p, "caixa", 0.0f);
    settle(p, 300);
    const auto off = play(p, signal);
    setParam(p, "caixa", 1.0f);
    settle(p, 300);
    const auto on = play(p, signal);
    const double d = diffDb(off, on);
    report(p.packIncludesCab(0) && d < -100.0, "Captura full rig: ligar o IR não muda nada (caixa trancada)",
           juce::String::fromUTF8("diferença: ") + juce::String(d, 1) + " dB (limite -100 dB)");
    if (photos != juce::File())
      snapshot(p, photos.getChildFile("caixa_full_rig.png"));
  }

  // 3. Pedal: muda o som quando ligado; desligado, o som é o mesmo de sem pedal.
  {
    std::printf("\nPedal\n");
    auto make = [&](bool withPedal, bool pedalOn) {
      auto p = std::make_unique<NexusProcessor>();
      p->setPlayConfigDetails(2, 2, 48000.0, 128);
      p->prepareToPlay(48000.0, 128);
      setParam(*p, "p1_ligado", pedalOn ? 1.0f : 0.0f);
      setParam(*p, "caixa", 0.0f);
      p->prepareToPlay(48000.0, 128);
      p->loadAmpPack(0, 0, packs.getChildFile("ganho"));
      if (withPedal)
      {
        ChainLayout l; // a cadeia nova abre sem pedais
        l.pre = {0};
        p->setLayout(l);
        p->loadPedalPack(0, packs.getChildFile("ganho"));
      }
      for (int t = 0; t < 120000 && !(p->channelLoaded(0, 0) && (!withPedal || p->slotLoaded(0))); t += 50)
        settle(*p, 50);
      // Aquecimento da captura (132 ms de som) + passagem suave (30 ms): cada
      // 5 ms aqui tocam 2,7 ms de som, então 600 ms dão folga.
      settle(*p, 600);
      return p;
    };
    auto none = make(false, true);
    auto on = make(true, true);
    auto off = make(true, false);
    const auto outNone = play(*none, signal);
    const auto outOn = play(*on, signal);
    const auto outOff = play(*off, signal);
    const double dOn = diffDb(outNone, outOn);
    const double dOff = diffDb(outNone, outOff);
    report(dOn > -20.0, "Pedal ligado com captura muda o som",
           juce::String::fromUTF8("diferença para sem pedal: ") + juce::String(dOn, 1) + " dB");
    report(dOff < -100.0, "Pedal desligado deixa o som igual ao de sem pedal",
           juce::String::fromUTF8("diferença: ") + juce::String(dOff, 1) + " dB (limite -100 dB)");

    // Liga e desliga no meio do som: sem salto (a transição é de 20 ms).
    setParam(*on, "p1_ligado", 0.0f);
    const auto toggled = play(*on, signal);
    float worstJump = 0.0f;
    for (size_t i = 1; i < std::min<size_t>(toggled.size(), 4800); ++i)
      worstJump = std::max(worstJump, std::abs(toggled[i] - toggled[i - 1]));
    float typicalJump = 0.0f;
    for (size_t i = 1; i < outOn.size(); ++i)
      typicalJump = std::max(typicalJump, std::abs(outOn[i] - outOn[i - 1]));
    report(worstJump <= typicalJump * 1.5f, "Desligar o pedal tocando não dá salto no som",
           juce::String::fromUTF8("maior passo na troca: ") + juce::String(worstJump, 4) + juce::String::fromUTF8("; maior passo tocando: ")
             + juce::String(typicalJump, 4));

    // O pisante da imagem liga e desliga.
    std::unique_ptr<juce::AudioProcessorEditor> editor(on->createEditor());
    auto* e = dynamic_cast<NexusEditor*>(editor.get());
    e->select({NexusEditor::Kind::pedal, 0});
    juce::MessageManager::getInstance()->runDispatchLoopUntil(50);
    auto* param = on->params.getParameter("p1_ligado");
    param->setValueNotifyingHost(1.0f);
    juce::Component* stage = nullptr;
    for (auto* c : editor->getChildren())
      if (c->isVisible() && c->getBounds().getHeight() > 300 && dynamic_cast<nexus::ui::GearBrowser*>(c) == nullptr)
        stage = c;
    bool toggledByClick = false;
    if (stage != nullptr)
    {
      std::vector<nexus::ui::FilmKnob*> pedalKnobs;
      collect(*stage, pedalKnobs);
      report(pedalKnobs.size() == 3, "O pedal mostra os 3 botões renderizados",
             juce::String(static_cast<int>(pedalKnobs.size())) + juce::String::fromUTF8(" botões"));
      // Procura o pisante varrendo a metade de baixo do palco.
      for (int y = stage->getHeight() / 2; y < stage->getHeight() && !toggledByClick; y += 6)
      {
        const juce::Point<float> pt(stage->getWidth() * 0.5f, static_cast<float>(y));
        stage->mouseUp(mouseAt(*stage, pt, pt, 1, false));
        toggledByClick = param->getValue() < 0.5f;
      }
    }
    report(toggledByClick, "Clicar no pisante desliga o pedal", toggledByClick ? juce::String("desligou") : juce::String("nada aconteceu"));
    if (photos != juce::File())
    {
      const auto image = editor->createComponentSnapshot(editor->getLocalBounds(), true, 2.0f);
      juce::File out = photos.getChildFile("pedal.png");
      out.deleteFile();
      juce::FileOutputStream stream(out);
      juce::PNGImageFormat().writeImageToStream(image, stream);
    }
  }

  // 4. Cabeçote de canais: só os canais 1 e 3 têm pacote.
  {
    std::printf("\nCanais do cabeçote\n");
    NexusProcessor p;
    p.setPlayConfigDetails(2, 2, 48000.0, 128);
    p.prepareToPlay(48000.0, 128);
    setParam(p, "caixa", 0.0f);
    setParam(p, "ganho", 0.0f); // canal 1 limpo; o canal 3 é o A2 cheio
    p.loadAmpPack(0, 0, packs.getChildFile("ganho"));
    p.loadAmpPack(0, 2, packs.getChildFile("fullrig"));
    for (int t = 0; t < 120000 && !(p.channelLoaded(0, 0) && p.channelLoaded(0, 2)); t += 50)
      settle(p, 50);
    settle(p, 300);
    report(p.channelLoaded(0, 0) && !p.channelLoaded(0, 1) && p.channelLoaded(0, 2), "Cabeçote com 2 canais: o canal do meio fica vazio",
           "canais com pacote: 1 e 3");
    const auto ch1 = play(p, signal);

    setParam(p, "canal", 1.0f); // canal vazio: o som não pode mudar
    settle(p, 300);
    const auto stay = play(p, signal);
    // Mesmo sinal, mesmo canal: compara com uma segunda passada no canal 1.
    setParam(p, "canal", 0.0f);
    settle(p, 300);
    const auto ch1b = play(p, signal);
    report(diffDb(ch1b, stay) < -100.0 && !p.packIncludesCab(0), "Escolher um canal vazio mantém o canal que está tocando",
           juce::String::fromUTF8("diferença: ") + juce::String(diffDb(ch1b, stay), 1) + " dB");

    setParam(p, "canal", 2.0f);
    const auto switching = play(p, signal);
    float worst = 0.0f, typical = 0.0f;
    for (size_t i = 1; i < switching.size(); ++i)
      worst = std::max(worst, std::abs(switching[i] - switching[i - 1]));
    for (size_t i = 1; i < ch1.size(); ++i)
      typical = std::max(typical, std::abs(ch1[i] - ch1[i - 1]));
    const auto ch3 = play(p, signal);
    float typical3 = 0.0f;
    for (size_t i = 1; i < ch3.size(); ++i)
      typical3 = std::max(typical3, std::abs(ch3[i] - ch3[i - 1]));
    report(worst <= std::max(typical, typical3) * 1.5f, "Trocar de canal tocando não dá salto",
           juce::String::fromUTF8("maior passo na troca: ") + juce::String(worst, 4) + "; tocando: " + juce::String(std::max(typical, typical3), 4));
    report(diffDb(ch1, ch3) > -20.0, "O canal 3 soa diferente do canal 1",
           juce::String::fromUTF8("diferença: ") + juce::String(diffDb(ch1, ch3), 1) + " dB");
    report(p.packIncludesCab(0), "A trava da caixa segue o canal (canal 3 é full rig)",
           p.packIncludesCab(0) ? juce::String("caixa trancada") : juce::String("caixa livre"));

    std::unique_ptr<juce::AudioProcessorEditor> editor(p.createEditor());
    juce::MessageManager::getInstance()->runDispatchLoopUntil(100);
    std::vector<nexus::ui::Segmented*> segs;
    collect(*editor, segs);
    int visibleChannelSelectors = 0;
    for (auto* sgm : segs)
      if (visibleIn(*sgm, *editor) && sgm->numItems() == 1)
        ++visibleChannelSelectors;
    report(visibleChannelSelectors == 0, "Brit 800 tem um canal, como o original: a tela não mostra seletor de canais",
           juce::String(visibleChannelSelectors) + " seletor(es) de canal visíveis");
    if (photos != juce::File())
    {
      const auto image = editor->createComponentSnapshot(editor->getLocalBounds(), true, 2.0f);
      juce::File out = photos.getChildFile("canais.png");
      out.deleteFile();
      juce::FileOutputStream stream(out);
      juce::PNGImageFormat().writeImageToStream(image, stream);
    }
  }

  // 5. MIDI.
  {
    std::printf("\nMIDI\n");
    NexusProcessor p;
    p.setPlayConfigDetails(2, 2, 48000.0, 128);
    p.prepareToPlay(48000.0, 128);
    juce::AudioBuffer<float> buffer(2, 128);
    auto send = [&](const juce::MidiMessage& m) {
      juce::MidiBuffer midi;
      midi.addEvent(m, 0);
      buffer.clear();
      p.processBlock(buffer, midi);
    };
    auto value = [&](const char* id) { auto* q = p.params.getParameter(id); return q->convertFrom0to1(q->getValue()); };

    send(juce::MidiMessage::programChange(1, 2));
    report(juce::roundToInt(value("canal")) == 2, "Troca de patch 3 escolhe o canal 3", "canal " + juce::String(juce::roundToInt(value("canal")) + 1));

    p.midi().learn("p1_ligado");
    send(juce::MidiMessage::controllerEvent(1, 80, 127));
    const bool learnedOn = p.midi().ccFor("p1_ligado") == 80 && value("p1_ligado") > 0.5f;
    send(juce::MidiMessage::controllerEvent(1, 80, 0));
    report(learnedOn && value("p1_ligado") < 0.5f, "Aprender: CC 80 liga (127) e desliga (0) o pedal",
           "CC " + juce::String(p.midi().ccFor("p1_ligado")));

    p.midi().learn("ganho");
    send(juce::MidiMessage::controllerEvent(1, 21, 127));
    const float top = value("ganho");
    send(juce::MidiMessage::controllerEvent(1, 21, 0));
    report(std::abs(top - 10.0f) < 0.01f && value("ganho") < 0.01f, "CC 21 percorre o ganho de 0 a 10",
           "127 -> " + juce::String(top, 2) + ", 0 -> " + juce::String(value("ganho"), 2));

    p.midi().learn("ganho");
    send(juce::MidiMessage::controllerEvent(1, 22, 64));
    send(juce::MidiMessage::controllerEvent(1, 21, 127)); // o CC antigo não manda mais
    report(p.midi().ccFor("ganho") == 22 && value("ganho") < 6.0f, "Reaprender troca o controle (o antigo para de agir)",
           "agora CC " + juce::String(p.midi().ccFor("ganho")) + ", ganho " + juce::String(value("ganho"), 2));

    // Pedaleira (M-VAVE Chocolate e parecidas): cada pisada manda PC ou nota.
    p.midi().learn("p2_ligado");
    send(juce::MidiMessage::programChange(1, 4)); // PC 5
    const bool pcOn = p.midi().ccFor("p2_ligado") == 128 + 4 && value("p2_ligado") < 0.5f;
    send(juce::MidiMessage::programChange(1, 4));
    report(pcOn && value("p2_ligado") > 0.5f && juce::roundToInt(value("canal")) == 2,
           "Aprender: PC 5 da pedaleira liga e desliga o pedal 2 (e não troca o canal)", MidiMap::describe(p.midi().ccFor("p2_ligado")));
    p.midi().learn("p3_ligado");
    send(juce::MidiMessage::noteOn(1, 60, 1.0f));
    const float afterFirst = value("p3_ligado");
    send(juce::MidiMessage::noteOff(1, 60));
    send(juce::MidiMessage::noteOn(1, 60, 1.0f));
    report(p.midi().ccFor("p3_ligado") == 256 + 60 && std::abs(afterFirst - value("p3_ligado")) > 0.5f,
           "Aprender: nota 60 alterna o pedal 3 a cada pisada", MidiMap::describe(p.midi().ccFor("p3_ligado")));

    // Pedaleira que manda o banco (CC 0) junto com a troca de patch: aprende a
    // pisada (o PC), e outra pisada não mexe no pedal.
    {
      auto sendPair = [&](int program) {
        juce::MidiBuffer midi;
        midi.addEvent(juce::MidiMessage::controllerEvent(1, 0, 0), 0);
        midi.addEvent(juce::MidiMessage::programChange(1, program), 1);
        buffer.clear();
        p.processBlock(buffer, midi);
      };
      p.midi().learn("p4_ligado");
      sendPair(10);
      const float first = value("p4_ligado");
      sendPair(11); // outra pisada
      const bool untouched = std::abs(first - value("p4_ligado")) < 0.01f;
      sendPair(10);
      report(p.midi().ccFor("p4_ligado") == 128 + 10 && untouched && std::abs(first - value("p4_ligado")) > 0.5f,
             "Aprender: com banco + troca de patch, só a pisada aprendida liga o pedal 4", MidiMap::describe(p.midi().ccFor("p4_ligado")));
    }

    // Tap tempo: quatro toques a cada 0,5 s = 120 BPM; um toque pisado pela pedaleira também conta.
    for (int t = 0; t < 4; ++t)
    {
      auto* tapParam = p.params.getParameter("tap");
      tapParam->setValueNotifyingHost(tapParam->getValue() > 0.5f ? 0.0f : 1.0f);
      juce::Thread::sleep(500);
    }
    report(std::abs(p.tempoBpm() - 120.0) < 3.0, "Tap tempo: quatro toques a cada meio segundo dão 120 BPM",
           juce::String(p.tempoBpm(), 1) + " BPM");
    report(std::abs(NexusProcessor::divisionBeats(NexusProcessor::divisionFor(5.0f)) * 60.0 / 120.0 - 1.0 / 3.0) < 1e-6
             && juce::String(NexusProcessor::divisionName(NexusProcessor::divisionFor(0.0f))) == "1/1"
             && juce::String(NexusProcessor::divisionName(NexusProcessor::divisionFor(10.0f))) == "1/32",
           "Divisões do andamento (meio do botão = 1/6 a 120 BPM = 333 ms)", "1/1 a 1/32");

    juce::MemoryBlock state;
    p.getStateInformation(state);
    NexusProcessor q;
    q.setStateInformation(state.getData(), static_cast<int>(state.getSize()));
    report(q.midi().ccFor("p1_ligado") == 80 && q.midi().ccFor("ganho") == 22 && q.midi().ccFor("p2_ligado") == 132
             && q.midi().ccFor("p3_ligado") == 316,
           "O mapa MIDI volta ao abrir o projeto (CC, PC e nota)",
           "pedal CC " + juce::String(q.midi().ccFor("p1_ligado")) + ", ganho CC " + juce::String(q.midi().ccFor("ganho")));
  }

  // 6. Cadeia: lugar do pedal, dois cabeçotes com pedal A/B.
  {
    std::printf("\nCadeia de sinal\n");
    auto make = [&](const ChainLayout& layout) {
      auto p = std::make_unique<NexusProcessor>();
      p->setPlayConfigDetails(2, 2, 48000.0, 128);
      p->prepareToPlay(48000.0, 128);
      setParam(*p, "caixa", 0.0f);
      setParam(*p, "caixa_b", 0.0f);
      // Pedal e cabeçote usam a mesma captura de teste: em ganhos diferentes,
      // a ordem muda o som (f(g(x)) é diferente de g(f(x))).
      setParam(*p, "p1_drive", 0.0f);
      setParam(*p, "ganho", 10.0f);
      setParam(*p, "ganho_b", 0.0f);
      p->setLayout(layout);
      p->loadAmpPack(0, 0, packs.getChildFile("ganho"));
      p->loadAmpPack(1, 0, packs.getChildFile("fullrig"));
      p->loadPedalPack(0, packs.getChildFile("ganho"));
      for (int t = 0; t < 120000 && !(p->channelLoaded(0, 0) && p->channelLoaded(1, 0) && p->slotLoaded(0)); t += 50)
        settle(*p, 50);
      // Aquecimento da captura (132 ms de som) + passagem suave (30 ms): cada
      // 5 ms aqui tocam 2,7 ms de som, então 600 ms dão folga.
      settle(*p, 600);
      return p;
    };
    ChainLayout before;
    before.pre = {0};
    ChainLayout after;
    after.post = {0};
    ChainLayout between;
    between.loop[0] = {0};
    auto pBefore = make(before);
    auto pAfter = make(after);
    auto pBetween = make(between);
    const auto oBefore = play(*pBefore, signal);
    const auto oAfter = play(*pAfter, signal);
    const auto oBetween = play(*pBetween, signal);
    report(diffDb(oBefore, oAfter) > -20.0, "Pedal antes ou depois do cabeçote soa diferente",
           juce::String::fromUTF8("diferença: ") + juce::String(diffDb(oBefore, oAfter), 1) + " dB");
    report(diffDb(oBefore, oBetween) > -20.0, "Pedal entre cabeçote e caixa soa diferente de antes",
           juce::String::fromUTF8("diferença: ") + juce::String(diffDb(oBefore, oBetween), 1) + " dB");

    // Primeira abertura: só o Brit 800 com a caixa dele, sem pedais.
    {
      NexusProcessor fresh;
      const int head = juce::roundToInt(fresh.params.getRawParameterValue("cabecote")->load());
      const int cab = juce::roundToInt(fresh.params.getRawParameterValue("gabinete")->load());
      report(fresh.layout().count() == 0 && head == 0 && cab == 0, "Primeira abertura: Brit 800 e a caixa dele, sem pedais",
             juce::String(fresh.layout().count()) + " pedais, cabeçote " + juce::String(head) + ", caixa " + juce::String(cab));
    }
    // Cherry Drop: meio-dia não muda o tom; as pontas são -12 e +12.
    report(NexusProcessor::dropSemitones(5.0f) == 0 && NexusProcessor::dropSemitones(0.0f) == -12 && NexusProcessor::dropSemitones(10.0f) == 12
             && NexusProcessor::dropSemitones(5.0f + 5.0f / 12.0f) == 1,
           "Cherry Drop: meio-dia em 0, de -12 a +12 semitons", juce::String(NexusProcessor::dropSemitones(5.0f)));

    // A cadeia volta igual ao reabrir o projeto.
    juce::MemoryBlock state;
    pBetween->getStateInformation(state);
    NexusProcessor q;
    q.setStateInformation(state.getData(), static_cast<int>(state.getSize()));
    report(q.layout().loop[0] == std::vector<int>{0} && q.layout().pre.empty(), "A ordem da cadeia volta ao abrir o projeto",
           juce::String::fromUTF8(q.layout().loop[0].size() == 1 ? "pedal entre cabeçote e caixa" : "ordem perdida"));

    // Dois cabeçotes: o pedal A/B troca o lado sem salto.
    ChainLayout dual;
    dual.dual = true;
    auto pA = make(dual);
    auto pB = make(dual);
    setParam(*pB, "ab", 1.0f);
    const auto onlyA = play(*pA, signal);
    const auto onlyB = play(*pB, signal);
    report(diffDb(onlyA, onlyB) > -20.0, "Lado A e lado B soam diferentes",
           juce::String::fromUTF8("diferença: ") + juce::String(diffDb(onlyA, onlyB), 1) + " dB");
    setParam(*pA, "ab", 1.0f);
    const auto switched = play(*pA, signal);
    float worst = 0.0f, typical = 0.0f;
    for (size_t i = 1; i < std::min<size_t>(switched.size(), 4800); ++i)
      worst = std::max(worst, std::abs(switched[i] - switched[i - 1]));
    for (size_t i = 1; i < onlyB.size(); ++i)
      typical = std::max(typical, std::max(std::abs(onlyB[i] - onlyB[i - 1]), std::abs(onlyA[i] - onlyA[i - 1])));
    report(worst <= typical * 1.5f, "Pisar no A/B tocando não dá salto no som",
           juce::String::fromUTF8("maior passo na troca: ") + juce::String(worst, 4) + juce::String::fromUTF8("; tocando: ") + juce::String(typical, 4));
    // Referência: o lado B já assentado (o ganho dele desliza ao mudar).
    const auto onlyBSettled = play(*pB, signal);
    const auto settled = play(*pA, signal);
    report(diffDb(onlyBSettled, settled) < -60.0, "Depois da troca toca só o lado B",
           juce::String::fromUTF8("diferença para só B: ") + juce::String(diffDb(onlyBSettled, settled), 1) + " dB");
  }

  // 8. Pedais nativos: chorus, delay e reverb.
  {
    std::printf("\nEfeitos do Nexus (chorus, delay, reverb)\n");
    auto make = [&](int look, bool on) {
      auto p = std::make_unique<NexusProcessor>();
      p->setPlayConfigDetails(2, 2, 48000.0, 128);
      p->prepareToPlay(48000.0, 128);
      setParam(*p, "p1_visual", static_cast<float>(look));
      setParam(*p, "p1_ligado", on ? 1.0f : 0.0f);
      ChainLayout l;
      l.pre = {0};
      p->setLayout(l);
      settle(*p, 100);
      return p;
    };
    // Sem cabeçote carregado o sinal passa direto pelo plugin: dá para ouvir só o efeito.
    auto dry = make(4, false);
    const auto outDry = play(*dry, signal);
    for (int look : {4, 5, 6})
    {
      auto on = make(look, true);
      const auto out = play(*on, signal);
      const char* names[] = {"Chorus", "Delay", "Reverb"};
      report(diffDb(outDry, out) > -25.0, (juce::String(names[look - 4]) + " ligado muda o som").toRawUTF8(),
             juce::String::fromUTF8("diferença para o som seco: ") + juce::String(diffDb(outDry, out), 1) + " dB");
    }
    // Delay: depois de desligar, as repetições continuam por um tempo (rastro).
    auto delay = make(5, true);
    play(*delay, signal);
    setParam(*delay, "p1_ligado", 0.0f);
    std::vector<float> silence(48000, 0.0f);
    juce::AudioBuffer<float> buffer(2, 128);
    juce::MidiBuffer midi;
    double tailEnergy = 0.0;
    for (int b = 0; b < 48000 / 128; ++b)
    {
      buffer.clear();
      delay->processBlock(buffer, midi);
      for (int i = 0; i < 128; ++i)
        tailEnergy += buffer.getSample(0, i) * buffer.getSample(0, i);
    }
    report(tailEnergy > 1e-4, "Delay desligado deixa as repetições terminarem (rastro)",
           juce::String::fromUTF8("energia no segundo seguinte: ") + juce::String(tailEnergy, 4));
    // Rastro acabou: o som volta a ser exatamente o seco.
    for (int b = 0; b < 8 * 48000 / 128; ++b)
    {
      buffer.clear();
      delay->processBlock(buffer, midi);
    }
    const auto after = play(*delay, signal);
    report(diffDb(outDry, after) < -100.0, "Efeito desligado, sem rastro, deixa o som intacto",
           juce::String::fromUTF8("diferença: ") + juce::String(diffDb(outDry, after), 1) + " dB");
  }

  // 8b. Pedais de vários modos: Juniper Sky (reverb), Lime Echo (delay) e Plum Mod (modulação).
  {
    std::printf("\nPedais de vários modos (Juniper Sky, Lime Echo, Plum Mod)\n");
    auto make = [&](int look, int mode, bool on) {
      auto p = std::make_unique<NexusProcessor>();
      p->setPlayConfigDetails(2, 2, 48000.0, 128);
      p->prepareToPlay(48000.0, 128);
      setParam(*p, "p1_visual", static_cast<float>(look));
      setParam(*p, "p1_modo", static_cast<float>(mode));
      setParam(*p, "p1_ligado", on ? 1.0f : 0.0f);
      ChainLayout l;
      l.pre = {0};
      p->setLayout(l);
      settle(*p, 20);
      return p;
    };
    auto rmsDb = [](const std::vector<float>& v) {
      double s = 1e-30;
      for (float x : v)
        s += static_cast<double>(x) * x;
      return 10.0 * std::log10(s / static_cast<double>(std::max<size_t>(1, v.size())));
    };
    const auto outDry = play(*make(19, 0, false), signal);
    const char* const kinds[3] = {"Juniper Sky", "Lime Echo", "Plum Mod"};
    for (int k = 0; k < 3; ++k)
    {
      int finite = 0, changed = 0, level = 0;
      double worstLevel = 0.0;
      juce::String odd;
      for (int mode = 0; mode < 11; ++mode)
      {
        auto p = make(19 + k, mode, true);
        const auto out = play(*p, signal);
        const bool ok = std::all_of(out.begin(), out.end(), [](float x) { return std::isfinite(x) && std::abs(x) < 8.0f; });
        const double gap = rmsDb(out) - rmsDb(outDry);
        finite += ok ? 1 : 0;
        changed += diffDb(outDry, out) > -30.0 ? 1 : 0;
        level += std::abs(gap) < 3.0 ? 1 : 0;
        if (std::abs(gap) > std::abs(worstLevel))
          worstLevel = gap;
        if (!ok || std::abs(gap) >= 3.0 || diffDb(outDry, out) <= -30.0)
          odd += NexusProcessor::modeName(19 + k, static_cast<float>(mode)) + " ";
      }
      report(finite == 11, (juce::String(kinds[k]) + ": os 11 modos sem valores infinitos nem estouro").toRawUTF8(),
             juce::String(finite) + " de 11" + (odd.isEmpty() ? juce::String() : ", fora: " + odd));
      report(changed == 11, (juce::String(kinds[k]) + ": cada modo ligado muda o som").toRawUTF8(), juce::String(changed) + " de 11");
      report(level == 11, (juce::String(kinds[k]) + ": volume ligado parecido com desligado (até 3 dB)").toRawUTF8(),
             juce::String::fromUTF8("maior diferença: ") + juce::String(worstLevel, 1) + " dB");
    }

    // Tudo no máximo por 20 s (shimmer realimentado, repetições no fim): continua finito.
    {
      int bounded = 0;
      const auto longSignal = nexus::makeGuitarLikeSignal(48000.0, 20.0, 9);
      for (int k = 0; k < 3; ++k)
        for (int mode : {0, 4, 6, 10})
        {
          auto p = make(19 + k, mode, true);
          for (const char* id : {"p1_a", "p1_b", "p1_c", "p1_d", "p1_tom", "p1_nivel"})
            setParam(*p, id, 10.0f);
          const auto out = play(*p, longSignal);
          const bool ok = std::all_of(out.begin(), out.end(), [](float x) { return std::isfinite(x) && std::abs(x) < 8.0f; });
          bounded += ok ? 1 : 0;
        }
      report(bounded == 12, juce::String::fromUTF8("Botões no máximo por 20 s: nenhum modo dispara").toRawUTF8(),
             juce::String(bounded) + " de 12");
    }

    // Lime Echo no andamento: 1/4 a 120 BPM repete meio segundo depois.
    {
      auto firstEcho = [](const std::vector<float>& out) {
        size_t best = 4000;
        for (size_t i = 4000; i < out.size(); ++i)
          if (std::abs(out[i]) > std::abs(out[best]))
            best = i;
        return (static_cast<double>(best) - 1024.0) / 48.0;
      };
      auto p = make(20, 0, true);
      setParam(*p, "andamento", 120.0f);
      setParam(*p, "p1_sinc", 1.0f);
      setParam(*p, "p1_a", 2.5f); // terceira divisão: 1/4
      setParam(*p, "p1_b", 0.0f);
      setParam(*p, "p1_c", 10.0f);
      std::vector<double> click(48000, 0.0);
      for (int i = 0; i < 48; ++i)
        click[static_cast<size_t>(1000 + i)] = 0.5 * std::sin(i / 48.0 * 3.14159);
      const double ms = firstEcho(play(*p, click));
      report(std::abs(ms - 500.0) < 5.0, "Lime Echo sincronizado: 1/4 a 120 BPM repete em 500 ms",
             juce::String(ms, 1) + " ms (" + NexusProcessor::divisionName(NexusProcessor::divisionFor(2.5f)) + ")");
      setParam(*p, "andamento", 90.0f);
      play(*p, std::vector<double>(96000, 0.0)); // o tempo desliza até o novo
      const double ms90 = firstEcho(play(*p, click));
      report(std::abs(ms90 - 666.7) < 8.0, "Mudou o andamento (90 BPM): a repetição acompanha", juce::String(ms90, 1) + " ms");
    }

    // Shimmer oitava acima: uma nota de 220 Hz ganha energia em 440 Hz na cauda.
    {
      auto energyAt = [](const std::vector<float>& v, size_t from, double hz) {
        double re = 0.0, im = 0.0;
        for (size_t i = from; i < v.size(); ++i)
        {
          re += v[i] * std::cos(2.0 * 3.14159265358979 * hz * static_cast<double>(i) / 48000.0);
          im += v[i] * std::sin(2.0 * 3.14159265358979 * hz * static_cast<double>(i) / 48000.0);
        }
        return std::sqrt(re * re + im * im);
      };
      std::vector<double> note(48000 * 4, 0.0);
      for (size_t i = 0; i < 48000; ++i)
        note[i] = 0.3 * std::sin(2.0 * 3.14159265358979 * 220.0 * static_cast<double>(i) / 48000.0);
      double ratio[2] = {};
      for (int s = 0; s < 2; ++s)
      {
        auto p = make(19, s == 0 ? 0 : 6, true); // hall e shimmer oitava acima
        setParam(*p, "p1_a", 7.0f);
        setParam(*p, "p1_c", 10.0f);
        setParam(*p, "p1_nivel", 8.0f);
        const auto out = play(*p, note);
        ratio[s] = energyAt(out, 48000 * 2, 440.0) / (energyAt(out, 48000 * 2, 220.0) + 1e-9);
      }
      report(ratio[1] > ratio[0] * 3.0, "Shimmer: a cauda sobe uma oitava (440 Hz) e o hall não",
             "440/220 no shimmer: " + juce::String(ratio[1], 2) + ", no hall: " + juce::String(ratio[0], 2));
    }

    // 8c. Blueberry Chain: delays em cadeia, com as chaves 2 | 3 e x | /.
    std::printf("\nBlueberry Chain (delays em cadeia)\n");
    {
      auto p = make(22, 0, true);
      const auto out = play(*p, signal);
      const bool ok = std::all_of(out.begin(), out.end(), [](float x) { return std::isfinite(x) && std::abs(x) < 8.0f; });
      const double gap = rmsDb(out) - rmsDb(outDry);
      report(ok && diffDb(outDry, out) > -30.0, "Blueberry Chain ligado: muda o som, sem valores infinitos",
             juce::String::fromUTF8("diferença: ") + juce::String(diffDb(outDry, out), 1) + " dB");
      report(std::abs(gap) < 3.0, "Blueberry Chain: volume ligado parecido com desligado (até 3 dB)",
             juce::String::fromUTF8("diferença: ") + juce::String(gap, 1) + " dB");
    }
    {
      // 120 BPM, tempo em 1/4 (500 ms): os três delays no mesmo tempo, então
      // toda repetição cai em múltiplos de 500 ms. A chave x | / muda quantas
      // vezes cada um repete: no x o rastro dura muito mais que no /.
      auto chain = [&](bool multiply, bool three, float feedback) {
        auto p = make(22, 0, true);
        setParam(*p, "andamento", 120.0f);
        setParam(*p, "p1_sinc", 1.0f);
        setParam(*p, "p1_a", 2.5f);
        setParam(*p, "p1_b", feedback);
        setParam(*p, "p1_c", 10.0f);
        setParam(*p, "p1_multiplica", multiply ? 1.0f : 0.0f);
        setParam(*p, "p1_tres", three ? 1.0f : 0.0f);
        play(*p, std::vector<double>(48000, 0.0));
        std::vector<double> click(192000, 0.0);
        for (int i = 0; i < 48; ++i)
          click[static_cast<size_t>(1000 + i)] = 0.3 * std::sin(i / 48.0 * 3.14159);
        return play(*p, click);
      };
      auto peakAt = [](const std::vector<float>& out, double ms) {
        const auto c = static_cast<size_t>(1024.0 + ms * 48.0);
        float best = 0.0f;
        for (size_t i = c - 240; i < c + 240 && i < out.size(); ++i)
          best = std::max(best, std::abs(out[i]));
        return best;
      };
      auto tailDb = [](const std::vector<float>& out) {
        double sum = 0.0;
        for (size_t i = 96000; i < 144000; ++i)
          sum += static_cast<double>(out[i]) * out[i];
        return 10.0 * std::log10(sum / 48000.0 + 1e-20);
      };
      const auto grid = chain(true, true, 0.0f);
      const float onGrid = std::min({peakAt(grid, 500.0), peakAt(grid, 1000.0), peakAt(grid, 1500.0)});
      const float offGrid = std::max(peakAt(grid, 250.0), peakAt(grid, 750.0));
      report(onGrid > 0.01f && offGrid < onGrid * 0.1f, "Os três delays no mesmo tempo: repetições em 500, 1000 e 1500 ms",
             juce::String::fromUTF8("menor pico: ") + juce::String(onGrid, 3) + ", fora do tempo: " + juce::String(offGrid, 4));
      const double times = tailDb(chain(true, true, 3.0f));
      const double divide = tailDb(chain(false, true, 3.0f));
      report(times - divide > 10.0, "Chave x repete muito mais que a chave / (rastro de 2 a 3 s)",
             juce::String::fromUTF8("x: ") + juce::String(times, 1) + " dB, /: " + juce::String(divide, 1) + " dB");
      const double two = tailDb(chain(true, false, 3.0f));
      report(times - two > 3.0, "Chave em 2: sem o terceiro delay o rastro acaba antes",
             juce::String::fromUTF8("3: ") + juce::String(times, 1) + " dB, 2: " + juce::String(two, 1) + " dB");
    }
    {
      auto p = make(22, 0, true);
      for (const char* id : {"p1_a", "p1_b", "p1_c"})
        setParam(*p, id, 10.0f);
      const auto out = play(*p, nexus::makeGuitarLikeSignal(48000.0, 20.0, 9));
      const bool ok = std::all_of(out.begin(), out.end(), [](float x) { return std::isfinite(x) && std::abs(x) < 8.0f; });
      report(ok, "Blueberry Chain com tudo no máximo por 20 s: não dispara", ok ? "ok" : "estourou");
    }
  }

  // 9. Presets: o estado salvo volta inteiro, inclusive "sem pacote".
  {
    std::printf("\nPresets\n");
    NexusProcessor p;
    p.setPlayConfigDetails(2, 2, 48000.0, 128);
    p.prepareToPlay(48000.0, 128);
    p.loadAmpPack(0, 0, packs.getChildFile("ganho"));
    waitPack(p);
    ChainLayout l;
    l.pre = {0, 1};
    l.post = {2};
    p.setLayout(l);
    setParam(p, "p2_visual", 5.0f);
    juce::MemoryBlock withPack;
    p.getStateInformation(withPack);

    NexusProcessor empty;
    juce::MemoryBlock withoutPack;
    empty.getStateInformation(withoutPack);
    p.setStateInformation(withoutPack.getData(), static_cast<int>(withoutPack.getSize()));
    settle(p, 200);
    report(!p.channelLoaded(0, 0), "Preset sem pacote tira o pacote do cabeçote",
           p.channelLoaded(0, 0) ? juce::String("continuou carregado") : juce::String("vazio"));
    p.setStateInformation(withPack.getData(), static_cast<int>(withPack.getSize()));
    waitPack(p);
    const auto back = p.layout();
    report(p.channelLoaded(0, 0) && back.pre == std::vector<int>{0, 1} && back.post == std::vector<int>{2} && p.pedalLook(1) == 5,
           "Voltar ao preset traz pacote, cadeia e pedais",
           juce::String::fromUTF8(p.channelLoaded(0, 0) ? "pacote de volta" : "sem pacote") + ", " + juce::String(static_cast<int>(back.count()))
             + " pedais, pedal 2 = visual " + juce::String(p.pedalLook(1)));
  }

  // 7. TONE3000: escolha das capturas pelos nomes reais dos pacotes.
  {
    std::printf("\nTONE3000: escolha das capturas\n");
    auto models = [](std::initializer_list<const char*> names) {
      std::vector<std::pair<juce::String, juce::String>> v;
      for (auto* n : names)
        v.emplace_back(juce::String(n), juce::String("https://exemplo/") + n);
      return v;
    };
    auto positions = [](const std::vector<nexus::t3k::Capture>& c) {
      juce::StringArray s;
      for (const auto& x : c)
        s.add(juce::String(x.position, 1));
      return s.joinIntoString(" ");
    };
    // Recomendada com série: só o master 6, uma captura por ganho.
    const auto rec = nexus::t3k::recommended("brit800");
    const auto jcm = models({"JCM800 2203 - P5 B5 M5 T5 MV6 G5 - AZG - 700", "JCM800 2203 - P5 B5 M5 T5 MV5 G9 - AZG - 700",
                             "JCM800 2203 - P5 B5 M5 T5 MV6 G1 - AZG - 700", "JCM800 2203 - P5 B5 M5 T5 MV6 G10 - AZG - 700",
                             "JCM800 2203 - P5 B5 M5 T5 MV7 G5 - AZG - 700"});
    const auto a = nexus::t3k::chooseCaptures(jcm, rec.empty() ? nexus::t3k::Tone{} : rec.front());
    report(!rec.empty() && positions(a) == "1.0 5.0 10.0", "Recomendada do Brit 800: série master 6, ganho 1, 5 e 10", positions(a));
    // Sem receita (favorito qualquer): acha "D3", "Drive 7" e fica com a maior série.
    const auto ts = nexus::t3k::chooseCaptures(models({"Ibanez-TS9-D3-T5-L5", "Ibanez-TS9-D0-T6-L10", "Ibanez-TS9-D10-T7-L5"}), {});
    report(positions(ts) == "0.0 3.0 10.0", "Favorito sem receita: lê D0, D3 e D10", positions(ts));
    const auto zw = nexus::t3k::chooseCaptures(models({"ZW - LOW - G3", "ZW - LOW - G5", "ZW - LOW - G7", "ZW - HIGH - G4", "ZW - HIGH - G6"}), {});
    report(positions(zw) == "3.0 5.0 7.0", "Pacote com duas séries: fica a que tem mais posições", positions(zw));
    int all = 0;
    for (const auto* g : {"brit800", "lemon_drive", "orange_drive", "banana_overdrive", "gold_centaur"})
      all += static_cast<int>(nexus::t3k::recommended(g).size());
    report(all >= 12, "Cada equipamento tem recomendadas", juce::String(all) + " tons recomendados");
  }

  // 10. Pacotes inseridos à mão: .zip, capturas soltas e o mapeamento.
  {
    std::printf("\nPacotes inseridos e mapeamento\n");
    namespace pk = nexus::ui::packs;
    const juce::String gear = juce::String::fromUTF8("Teste do Nexus (pode apagar)");
    const auto root = nexus::t3k::Connection::packsFolder(gear);
    root.deleteRecursively();
    juce::TemporaryFile tempDir;
    const auto work = tempDir.getFile();
    work.createDirectory();

    // Capturas com número no nome: mapeia sozinho.
    const auto source = packs.getChildFile("ganho");
    bool needs = true;
    auto found = pk::analyze(source, needs);
    report(!needs && found.size() == 11, "Nomes com número: o Nexus mapeia sozinho", juce::String(static_cast<int>(found.size())) + " capturas");

    // .zip com uma pasta por dentro e nomes sem número.
    const auto loose = work.getChildFile("Meu Amp");
    loose.createDirectory();
    const char* names[] = {"limpo", "crunch", "saturado"};
    const char* from[] = {"Ganho 0.nam", "Ganho 5.nam", "Ganho 10.nam"};
    for (int i = 0; i < 3; ++i)
      source.getChildFile(from[i]).copyFileTo(loose.getChildFile(juce::String(names[i]) + ".nam"));
    const auto zipFile = work.getChildFile("Meu Amp.zip");
    {
      juce::ZipFile::Builder builder;
      for (const auto& f : loose.findChildFiles(juce::File::findFiles, false, "*.nam"))
        builder.addFile(f, 6, "Meu Amp/" + f.getFileName());
      juce::FileOutputStream out(zipFile);
      builder.writeToStream(out, nullptr);
    }
    juce::String error;
    const auto unzipped = pk::importInto({zipFile}, gear, error);
    report(unzipped.isAChildOf(root) && unzipped.findChildFiles(juce::File::findFiles, false, "*.nam").size() == 3,
           ".zip descompactado na pasta do equipamento", error.isEmpty() ? unzipped.getFullPathName() : error);
    report(unzipped.getParentDirectory() == root, "Sem pasta repetida dentro do pacote", unzipped.getFileName());
    found = pk::analyze(unzipped, needs);
    report(needs, "Nomes sem número: pede o mapeamento ao usuário", juce::String::fromUTF8(needs ? "pede" : "não pediu"));

    // O usuário mapeia: limpo 0, crunch 5, saturado 10.
    for (auto& c : found)
      c.position = c.file.getFileNameWithoutExtension() == "limpo" ? 0.0 : c.file.getFileNameWithoutExtension() == "crunch" ? 5.0 : 10.0;
    const bool saved = pk::saveMapping(unzipped, found, "Ganho");
    found = pk::analyze(unzipped, needs);
    report(saved && !needs, "Mapeamento gravado no pack.json", unzipped.getChildFile("pack.json").loadFileAsString().substring(0, 120));
    nexus::PackOptions options;
    try
    {
      const auto pack = nexus::loadPack(unzipped.getFullPathName().toStdString(), options);
      report(pack->captures.size() == 3 && pack->captures[1].name.find("crunch") != std::string::npos,
             "O motor abre o pacote mapeado na ordem certa", juce::String(pack->captures[1].name));
    }
    catch (const std::exception& e)
    {
      report(false, "O motor abre o pacote mapeado na ordem certa", e.what());
    }

    // Capturas soltas: vão juntas para uma pasta nova.
    const auto copied = pk::importInto({loose.getChildFile("limpo.nam"), loose.getChildFile("crunch.nam")}, gear, error);
    report(copied.isAChildOf(root) && copied.findChildFiles(juce::File::findFiles, false, "*.nam").size() == 2,
           "Capturas soltas copiadas para a pasta do equipamento", copied.getFullPathName());
    // Duas capturas na mesma posição pedem conferência.
    juce::Array<juce::File> twins{source.getChildFile("Ganho 5.nam")};
    const auto twinFolder = pk::importInto(twins, gear, error);
    source.getChildFile("Ganho 5.nam").copyFileTo(twinFolder.getChildFile("Drive 5 outro.nam"));
    twinFolder.getChildFile("pack.json").deleteFile(); // só os nomes decidem
    pk::analyze(twinFolder, needs);
    report(needs, "Duas capturas na mesma posição: pede o mapeamento", juce::String::fromUTF8(needs ? "pede" : "não pediu"));
    root.deleteRecursively();
  }

  // 11. Atualização: comparar versões e achar o instalador de cada sistema.
  {
    std::printf("\nAtualização\n");
    namespace up = nexus::update;
    report(up::compareVersions("0.2.0", "0.1.9") > 0 && up::compareVersions("v1.0", "1.0.0") == 0
             && up::compareVersions("0.10.0", "0.9.5") > 0 && up::compareVersions("0.2.0-beta", "0.2.0") < 0
             && up::compareVersions("0.2.0-beta.2", "0.2.0-beta.1") > 0 && up::compareVersions("0.2.0-beta.10", "0.2.0-beta.9") > 0
             && up::compareVersions("0.2.0-beta.1", "0.1.0-beta.1") > 0 && up::compareVersions("v0.2.0-beta.1", "0.2.0-beta.1") == 0,
           "Comparação de versões", "0.2.0 > 0.1.9, v1.0 = 1.0.0, 0.10.0 > 0.9.5, final > beta, beta.2 > beta.1, beta.10 > beta.9");
    const juce::String json = R"({"tag_name":"v0.3.0","html_url":"https://github.com/disouz4-dev/Nexus-Modeler/releases/tag/v0.3.0","body":"Novidades",
      "assets":[{"name":"NexusModeler-0.3.0-Windows-instalador.exe","browser_download_url":"https://x/w.exe","size":10},
                {"name":"NexusModeler-0.3.0-Windows.zip","browser_download_url":"https://x/w.zip","size":9},
                {"name":"NexusModeler-0.3.0-Mac.dmg","browser_download_url":"https://x/m.dmg","size":11},
                {"name":"NexusModeler-0.3.0-Mac.zip","browser_download_url":"https://x/m.zip","size":8},
                {"name":"NexusModeler-0.3.0-Linux-amd64.deb","browser_download_url":"https://x/l.deb","size":12},
                {"name":"NexusModeler-0.3.0-Linux-x86_64.tar.gz","browser_download_url":"https://x/l.tgz","size":13}]})";
    const auto w = up::parseRelease(json, "windows"), m = up::parseRelease(json, "mac");
    const auto d = up::parseRelease(json, "linux-deb"), t = up::parseRelease(json, "linux");
    report(w.version == "0.3.0" && w.assetName.endsWith("instalador.exe") && m.assetName.endsWith("Mac.zip") && up::parseRelease(json.replace("Mac.zip", "Mac.txt"), "mac").assetName.endsWith(".dmg")
             && d.assetName.endsWith(".deb") && t.assetName.endsWith(".tar.gz") && w.assetSize == 10,
           "Instalador certo para cada sistema", w.assetName + ", " + m.assetName + ", " + d.assetName + ", " + t.assetName);
    report(up::compareVersions(up::currentVersion(), "0.0.1") > 0, juce::String::fromUTF8("O programa sabe a própria versão").toRawUTF8(), up::currentVersion());
  }

  // 12. Testar o som: a gravação entra no lugar da guitarra; gravar a própria guitarra.
  {
    std::printf("\nTestar o som\n");
    NexusProcessor p;
    p.setPlayConfigDetails(2, 2, 44100.0, 128);
    p.prepareToPlay(44100.0, 128);
    auto& test = p.testSource();
    auto rms = [&](int blocks, float inputLevel) {
      juce::AudioBuffer<float> b(2, 128);
      juce::MidiBuffer m;
      double sum = 0.0;
      for (int k = 0; k < blocks; ++k)
      {
        for (int i = 0; i < 128; ++i)
          for (int ch = 0; ch < 2; ++ch)
            b.setSample(ch, i, inputLevel * std::sin(0.05f * static_cast<float>(k * 128 + i)));
        p.processBlock(b, m);
        for (int i = 0; i < 128; ++i)
          sum += b.getSample(0, i) * b.getSample(0, i);
      }
      return 10.0 * std::log10(sum / (blocks * 128) + 1e-20);
    };
    const double silent = rms(200, 0.0f);
    bool allStyles = true;
    juce::String levels;
    for (auto s : {nexus::TestSource::clean, nexus::TestSource::blues, nexus::TestSource::rock, nexus::TestSource::heavy})
    {
      test.setStyle(s);
      test.setPlaying(true);
      const double level = rms(700, 0.0f);
      levels << juce::String(level, 1) << " ";
      allStyles = allStyles && level > -45.0;
    }
    report(silent < -100.0 && allStyles, "Cada estilo toca pela cadeia, sem guitarra ligada (44,1 kHz)", "dB: " + levels);
    test.setPlaying(false);
    report(rms(100, 0.0f) < -100.0, "Parar o teste volta para a guitarra", juce::String::fromUTF8("silêncio"));

    // Gravar: 15 s de "guitarra" (um seno) e ela passa a tocar sozinha.
    test.startRecording();
    rms(static_cast<int>(16.0 * 44100.0 / 128.0), 0.3f);
    test.poll();
    const bool mine = test.hasRecording() && test.style() == nexus::TestSource::mine && test.playing();
    const double played = rms(300, 0.0f);
    report(mine && played > -20.0, "Minha guitarra: grava 15 s e repete", juce::String(played, 1) + " dB tocando a gravação");
    test.setPlaying(false);
  }

  // 13. Relatório de problemas: registro sem dados pessoais e issue preenchida.
  {
    std::printf("\n13. Relatório de problemas\n");
    namespace rp = nexus::report;
    const auto home = juce::File::getSpecialLocation(juce::File::userHomeDirectory).getFullPathName();
    const auto mark = "teste-" + juce::String(juce::Random::getSystemRandom().nextInt(1000000));
    rp::log("teste", mark + " " + home + "/Documents/Nexus Modeler/pacote");
    const auto recent = rp::recentLog(5);
    report(recent.contains(mark) && !recent.contains(home) && recent.contains("~/Documents"),
           "Registro guarda o erro e troca a pasta do usuário por ~", recent.fromLastOccurrenceOf("\n", false, false));

    // Registro enorme: o endereço da issue continua dentro do limite.
    juce::String big;
    for (int i = 0; i < 400; ++i)
      big << "linha " << i << juce::String::fromUTF8(": erro de teste com acentuação e um caminho ~/Documents/Nexus Modeler\n");
    const auto url = rp::issueUrl(rp::Kind::problem, juce::String::fromUTF8("O som some quando troco de pedal"), big).toString(true);
    report(url.length() <= 7000 && url.contains("template=problema.yml") && url.contains("labels=problema")
             && url.contains("linha%20399"),
           "Issue preenchida cabe no endereço (o registro é cortado das linhas mais velhas)",
           juce::String(url.length()) + " caracteres");

    // Travamento salvo: é oferecido uma vez só.
    const auto crash = rp::folder().getChildFile("travamento.txt");
    crash.replaceWithText("Nexus Modeler teste\nPilha de chamadas:\n0 nexus");
    const auto first = rp::takeCrash();
    const auto second = rp::takeCrash();
    report(first.contains("Pilha de chamadas") && second.isEmpty(), "Travamento da última vez é oferecido uma vez", juce::String(first.length()) + " caracteres");
    rp::folder().getChildFile("travamento-visto.txt").deleteFile();
  }

  // 14. Estúdio: biblioteca e player (faixas de teste: um tom diferente em cada).
  {
    std::printf("\n14. Estúdio\n");
    namespace st = nexus::studio;
    const auto folder = st::Library::root().getChildFile("Teste do Nexus (pode apagar)");
    folder.deleteRecursively();
    folder.createDirectory();
    const double sr = st::kSeparationRate;
    const int frames = static_cast<int>(3.0 * sr);
    juce::FlacAudioFormat flac;
    for (int s = 0; s < st::kStems; ++s)
    {
      juce::AudioBuffer<float> b(2, frames);
      for (int i = 0; i < frames; ++i)
      {
        const float v = 0.2f * static_cast<float>(std::sin(2.0 * juce::MathConstants<double>::pi * 110.0 * (s + 1) * i / sr));
        b.setSample(0, i, v);
        b.setSample(1, i, v);
      }
      auto out = std::make_unique<juce::FileOutputStream>(folder.getChildFile(st::stemFile(s)));
      std::unique_ptr<juce::AudioFormatWriter> w(flac.createWriterFor(out.get(), sr, 2, 16, {}, 0));
      out.release();
      w->writeFromAudioSampleBuffer(b, 0, frames);
    }
    folder.getChildFile("musica.json").replaceWithText(R"({"titulo":"Teste","segundos":3.0})");
    st::Library lib;
    lib.rescan();
    const auto* song = lib.find(folder.getFileName());
    report(song != nullptr && std::abs(song->seconds - 3.0) < 0.01, "Biblioteca acha a música separada", folder.getFileName());

    st::Player player;
    player.prepare(48000.0, 256);
    const bool opened = song != nullptr && player.open(*song);
    // Toca "seconds" segundos e devolve o lado esquerdo.
    auto run = [&](double seconds) {
      std::vector<float> l(256), r(256), all;
      for (int k = 0; k < static_cast<int>(seconds * 48000.0 / 256); ++k)
      {
        std::fill(l.begin(), l.end(), 0.0f);
        std::fill(r.begin(), r.end(), 0.0f);
        player.process(l.data(), r.data(), 256);
        all.insert(all.end(), l.begin(), l.end());
        juce::Thread::sleep(1); // dá tempo da leitura em segundo plano
      }
      return all;
    };
    // Frequência principal pelo número de cruzamentos por zero.
    auto pitch = [](const std::vector<float>& x, size_t from) {
      int crossings = 0;
      for (size_t i = from + 1; i < x.size(); ++i)
        crossings += (x[i - 1] < 0.0f) != (x[i] < 0.0f);
      return crossings * 0.5 * 48000.0 / static_cast<double>(x.size() - from);
    };
    auto rmsDb = [](const std::vector<float>& x, size_t from) {
      double e = 0.0;
      for (size_t i = from; i < x.size(); ++i)
        e += x[i] * x[i];
      return 10.0 * std::log10(e / static_cast<double>(x.size() - from) + 1e-20);
    };

    st::Mix mix;
    mix.solo[st::guitar] = true; // guitarra: 550 Hz
    player.setMix(mix);
    run(0.3); // a leitura em segundo plano começa
    player.seek(0.0);
    player.setPlaying(true);
    auto a = run(1.0);
    const double f1 = pitch(a, 12000);
    report(opened && rmsDb(a, 12000) > -30.0 && std::abs(f1 - 550.0) < 25.0, "Solo da guitarra toca só ela, no tom certo (placa a 48 kHz)",
           juce::String(f1, 0) + " Hz, " + juce::String(rmsDb(a, 12000), 1) + " dB");

    mix.speed = 0.5;
    player.setMix(mix);
    player.seek(0.5);
    auto b = run(1.0);
    const double moved = player.position() - 0.5, f2 = pitch(b, 12000);
    report(std::abs(moved - 0.5) < 0.08 && std::abs(f2 - 550.0) < 25.0, "Velocidade 50%: anda na metade do tempo sem mudar o tom",
           juce::String(moved, 2) + " s em 1 s, " + juce::String(f2, 0) + " Hz");

    mix.speed = 1.0;
    mix.semitones = 6;
    player.setMix(mix);
    auto c = run(0.8);
    const double f3 = pitch(c, 12000);
    report(std::abs(f3 - 550.0 * std::pow(2.0, 0.5)) < 40.0, "Tom +6: a nota sobe meia oitava sem mudar a velocidade", juce::String(f3, 0) + " Hz");

    mix.semitones = 0;
    mix.solo = {};
    for (auto& m : mix.mute)
      m = true;
    player.setMix(mix);
    auto d = run(0.5);
    report(rmsDb(d, 12000) < -80.0, "Tudo no mudo: silêncio", juce::String(rmsDb(d, 12000), 1) + " dB");

    mix.mute = {};
    mix.loop = true;
    mix.loopStart = 1.0;
    mix.loopEnd = 1.6;
    player.setMix(mix);
    player.seek(1.0);
    double lowest = 10.0, highest = 0.0;
    for (int k = 0; k < 10; ++k)
    {
      run(0.2);
      lowest = std::min(lowest, player.position());
      highest = std::max(highest, player.position());
    }
    report(lowest >= 0.99 && highest <= 1.62 && player.playing(), "Repetir A–B fica dentro do trecho",
           juce::String(lowest, 2) + " a " + juce::String(highest, 2) + " s");

    mix.loop = false;
    player.setMix(mix);
    player.seek(2.8);
    run(0.6);
    report(!player.playing() && player.takeFinished(), "No fim da música para e avisa (para a playlist seguir)", {});

    const int pl = lib.addPlaylist("Teste");
    lib.addToPlaylist(pl, folder.getFileName());
    st::Library again;
    again.rescan();
    const bool saved = static_cast<int>(again.playlists().size()) > pl && again.playlists()[static_cast<size_t>(pl)].songs.contains(folder.getFileName());
    again.removePlaylist(pl);
    report(saved, "Playlist fica gravada", {});
    player.close();
    folder.deleteRecursively();
  }

  std::printf("\n%s (%d falha%s)\n", gFailures ? "REPROVADO" : "APROVADO", gFailures, gFailures == 1 ? "" : "s");
  return gFailures ? 1 : 0;
}
