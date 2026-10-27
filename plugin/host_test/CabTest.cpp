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
  const bool back = visibleIn(*knobs.front(), *editor) && (tabs.empty() || !visibleIn(*tabs.front(), *editor));
  report(back, "Voltar ao amplificador mostra os botões de novo",
         juce::String::fromUTF8(back ? "botões visíveis, abas escondidas" : "estado errado"));
}
} // namespace

int main(int argc, char** argv)
{
  // Só à mão: NEXUS_TESTE_TRAVAR=1 instala o tratador e trava de propósito,
  // para conferir o travamento.txt (o ctest não usa).
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
        p->loadPedalPack(0, packs.getChildFile("ganho"));
      for (int t = 0; t < 120000 && !(p->channelLoaded(0, 0) && (!withPedal || p->slotLoaded(0))); t += 50)
        settle(*p, 50);
      settle(*p, 300);
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

    juce::MemoryBlock state;
    p.getStateInformation(state);
    NexusProcessor q;
    q.setStateInformation(state.getData(), static_cast<int>(state.getSize()));
    report(q.midi().ccFor("p1_ligado") == 80 && q.midi().ccFor("ganho") == 22, "O mapa MIDI volta ao abrir o projeto",
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
      settle(*p, 300);
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
             && up::compareVersions("0.10.0", "0.9.5") > 0 && up::compareVersions("0.2.0-beta", "0.2.0") == 0,
           "Comparação de versões", "0.2.0 > 0.1.9, v1.0 = 1.0.0, 0.10.0 > 0.9.5");
    const juce::String json = R"({"tag_name":"v0.3.0","html_url":"https://github.com/disouz4-dev/Nexus-Modeler/releases/tag/v0.3.0","body":"Novidades",
      "assets":[{"name":"NexusModeler-0.3.0-Windows-instalador.exe","browser_download_url":"https://x/w.exe","size":10},
                {"name":"NexusModeler-0.3.0-Windows.zip","browser_download_url":"https://x/w.zip","size":9},
                {"name":"NexusModeler-0.3.0-Mac.dmg","browser_download_url":"https://x/m.dmg","size":11},
                {"name":"NexusModeler-0.3.0-Mac.zip","browser_download_url":"https://x/m.zip","size":8},
                {"name":"NexusModeler-0.3.0-Linux-amd64.deb","browser_download_url":"https://x/l.deb","size":12},
                {"name":"NexusModeler-0.3.0-Linux-x86_64.tar.gz","browser_download_url":"https://x/l.tgz","size":13}]})";
    const auto w = up::parseRelease(json, "windows"), m = up::parseRelease(json, "mac");
    const auto d = up::parseRelease(json, "linux-deb"), t = up::parseRelease(json, "linux");
    report(w.version == "0.3.0" && w.assetName.endsWith("instalador.exe") && m.assetName.endsWith(".dmg")
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

  std::printf("\n%s (%d falha%s)\n", gFailures ? "REPROVADO" : "APROVADO", gFailures, gFailures == 1 ? "" : "s");
  return gFailures ? 1 : 0;
}
