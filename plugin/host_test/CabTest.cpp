// Testa o bloco da caixa por dentro do plugin:
//   1. com captura só de cabeçote, o IR muda o som;
//   2. com captura full rig, o IR fica trancado: ligar ou desligar a caixa
//      não muda nada no som;
//   3. a tela mostra a caixa nos dois casos (fotos para conferir).
//
// Uso: nexus_cab_test <pasta_dos_pacotes_de_teste> [pasta_das_fotos]

#include <cmath>
#include <cstdio>

#include "../source/PluginEditor.h"
#include "../source/PluginProcessor.h"
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
  for (int t = 0; t < 120000 && !p.packLoaded(); t += 50)
    settle(p, 50);
  settle(p, 300);
  return p.packLoaded();
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
    e->showStage(NexusEditor::Stage::cab);
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

  k.mouseDown(mouseAt(k, c, c, 1, false));
  k.mouseDrag(mouseAt(k, c.translated(0.0f, 100.0f), c, 1, true, juce::ModifierKeys::shiftModifier));
  k.mouseUp(mouseAt(k, c.translated(0.0f, 100.0f), c, 1, true));
  const float afterFine = gain->convertFrom0to1(gain->getValue());
  report(std::abs(afterFine - 8.2f) < 0.05f, "Com Shift o ajuste é 5 vezes mais fino",
         juce::String::fromUTF8("100 pontos para baixo com Shift: ") + juce::String(afterFine, 2));

  k.mouseDoubleClick(mouseAt(k, c, c, 2, false));
  const float afterReset = gain->convertFrom0to1(gain->getValue());
  report(std::abs(afterReset - 5.0f) < 0.01f, "Duplo clique volta o botão ao padrão",
         juce::String::fromUTF8("ganho depois do duplo clique: ") + juce::String(afterReset, 2));

  e->showStage(NexusEditor::Stage::cab);
  std::vector<nexus::ui::Segmented*> tabs, segs;
  collect(*editor, segs);
  for (auto* t : segs)
    if (visibleIn(*t, *editor) && t->getParentComponent() != editor.get())
      tabs.push_back(t);
  const bool tabsVisible = tabs.size() == 1 && visibleIn(*tabs.front(), *editor);
  const bool knobsHidden = !visibleIn(*knobs.front(), *editor);
  report(tabsVisible && knobsHidden, "Trocar para a caixa mostra as abas da caixa e esconde o amplificador",
         juce::String::fromUTF8(tabsVisible ? "abas visíveis" : "abas escondidas"));
  e->showStage(NexusEditor::Stage::amp);
  const bool back = visibleIn(*knobs.front(), *editor) && (tabs.empty() || !visibleIn(*tabs.front(), *editor));
  report(back, "Voltar ao amplificador mostra os botões de novo",
         juce::String::fromUTF8(back ? "botões visíveis, abas escondidas" : "estado errado"));
}
} // namespace

int main(int argc, char** argv)
{
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
    p.loadPackAsync(packs.getChildFile("ganho"));
    if (!waitPack(p))
      report(false, "Pacote de teste carrega", "não carregou em 2 minutos");
    setParam(p, "caixa", 0.0f);
    settle(p, 100);
    const auto dry = play(p, signal);
    p.loadIrAsync(ir);
    setParam(p, "caixa", 1.0f);
    settle(p, 1500);
    const auto wet = play(p, signal);
    const double d = diffDb(dry, wet);
    report(!p.packIncludesCab() && p.irLoaded() && d > -20.0, "Captura de cabeçote: o IR muda o som",
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
    p.loadIrAsync(ir);
    p.loadPackAsync(packs.getChildFile("fullrig"));
    if (!waitPack(p))
      report(false, "Pacote full rig carrega", "não carregou em 2 minutos");
    setParam(p, "caixa", 0.0f);
    settle(p, 300);
    const auto off = play(p, signal);
    setParam(p, "caixa", 1.0f);
    settle(p, 300);
    const auto on = play(p, signal);
    const double d = diffDb(off, on);
    report(p.packIncludesCab() && d < -100.0, "Captura full rig: ligar o IR não muda nada (caixa trancada)",
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
      setParam(*p, "pedal_ligado", pedalOn ? 1.0f : 0.0f);
      setParam(*p, "caixa", 0.0f);
      p->prepareToPlay(48000.0, 128);
      p->loadPackAsync(packs.getChildFile("ganho"), NexusProcessor::amp);
      if (withPedal)
        p->loadPackAsync(packs.getChildFile("ganho"), NexusProcessor::pedal);
      for (int t = 0; t < 120000 && !(p->packLoaded() && (!withPedal || p->packLoaded(NexusProcessor::pedal))); t += 50)
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
    setParam(*on, "pedal_ligado", 0.0f);
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
    e->showStage(NexusEditor::Stage::pedal);
    juce::MessageManager::getInstance()->runDispatchLoopUntil(50);
    auto* param = on->params.getParameter("pedal_ligado");
    param->setValueNotifyingHost(1.0f);
    juce::Component* stage = nullptr;
    for (auto* c : editor->getChildren())
      if (c->isVisible() && c->getBounds().getHeight() > 300)
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
    p.loadPackAsync(packs.getChildFile("ganho"), NexusProcessor::amp, 0);
    p.loadPackAsync(packs.getChildFile("fullrig"), NexusProcessor::amp, 2);
    for (int t = 0; t < 120000 && !(p.channelLoaded(0) && p.channelLoaded(2)); t += 50)
      settle(p, 50);
    settle(p, 300);
    report(p.channelLoaded(0) && !p.channelLoaded(1) && p.channelLoaded(2), "Cabeçote com 2 canais: o canal do meio fica vazio",
           "canais com pacote: 1 e 3");
    const auto ch1 = play(p, signal);

    setParam(p, "canal", 1.0f); // canal vazio: o som não pode mudar
    settle(p, 300);
    const auto stay = play(p, signal);
    // Mesmo sinal, mesmo canal: compara com uma segunda passada no canal 1.
    setParam(p, "canal", 0.0f);
    settle(p, 300);
    const auto ch1b = play(p, signal);
    report(diffDb(ch1b, stay) < -100.0 && !p.packIncludesCab(), "Escolher um canal vazio mantém o canal que está tocando",
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
    report(p.packIncludesCab(), "A trava da caixa segue o canal (canal 3 é full rig)",
           p.packIncludesCab() ? juce::String("caixa trancada") : juce::String("caixa livre"));

    std::unique_ptr<juce::AudioProcessorEditor> editor(p.createEditor());
    juce::MessageManager::getInstance()->runDispatchLoopUntil(100);
    std::vector<nexus::ui::Segmented*> segs;
    collect(*editor, segs);
    int channelItems = -1;
    for (auto* sgm : segs)
      if (sgm->isVisible() && sgm->numItems() == 3)
        channelItems = sgm->numItems();
    report(channelItems == 3, "A tela mostra os 2 canais e o \"+ canal\"",
           juce::String(channelItems) + " itens no seletor de canais");
    if (photos != juce::File())
    {
      const auto image = editor->createComponentSnapshot(editor->getLocalBounds(), true, 2.0f);
      juce::File out = photos.getChildFile("canais.png");
      out.deleteFile();
      juce::FileOutputStream stream(out);
      juce::PNGImageFormat().writeImageToStream(image, stream);
    }
  }

  std::printf("\n%s (%d falha%s)\n", gFailures ? "REPROVADO" : "APROVADO", gFailures, gFailures == 1 ? "" : "s");
  return gFailures ? 1 : 0;
}
