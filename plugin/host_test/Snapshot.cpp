#include <cstdlib>
// Monta o plugin por dentro (sem programa de gravação), carrega um pacote,
// toca alguns segundos girando o ganho e salva uma foto da tela.
//
// Uso: nexus_snapshot <pasta do pacote> <foto.png> [posição do ganho] [pedal:<visual> | caixa]

#include <cstdio>

#include "../source/PluginEditor.h"
#include "../source/PluginProcessor.h"
#include "nexus/TestSignal.h"
#include "../source/ui/Language.h"
#include "../source/ui/Brand.h"

// Ícone do programa: o triângulo impossível do logo num quadrado escuro.
static int writeIcon(const juce::File& out, int size)
{
  juce::Image img(juce::Image::ARGB, size, size, true);
  {
    juce::Graphics g(img);
    const float s = static_cast<float>(size);
    const auto box = juce::Rectangle<float>(s, s).reduced(s * 0.09f); // margem do padrão de ícones do Mac
    juce::ColourGradient bg(juce::Colour(0xff1b1e24), box.getX(), box.getY(), juce::Colour(0xff0a0b0d), box.getX(), box.getBottom(), false);
    g.setGradientFill(bg);
    g.fillRoundedRectangle(box, box.getWidth() * 0.225f);
    g.setColour(juce::Colour(0x3329b6ff));
    g.drawRoundedRectangle(box.reduced(s * 0.004f), box.getWidth() * 0.225f, s * 0.006f);
    const auto tri = box.reduced(box.getWidth() * 0.12f).translated(0.0f, box.getHeight() * 0.015f);
    nexus::ui::brand::drawPenrose(g, tri);
  }
  out.deleteFile();
  juce::FileOutputStream stream(out);
  return juce::PNGImageFormat().writeImageToStream(img, stream) ? 0 : 1;
}

int main(int argc, char** argv)
{
  // Ferramenta: a tela não baixa nem troca peças sozinha (captura padrão desligada).
#ifdef _WIN32
  _putenv_s("NEXUS_SEM_PADRAO", "1");
#else
  setenv("NEXUS_SEM_PADRAO", "1", 1);
#endif
  // NEXUS_ICONE=<arquivo.png>: só gera o ícone do programa.
  if (const char* icon = std::getenv("NEXUS_ICONE"))
  {
    juce::ScopedJuceInitialiser_GUI init;
    return writeIcon(juce::File::getCurrentWorkingDirectory().getChildFile(icon), 1024);
  }
  if (argc < 3)
  {
    std::printf("Uso: nexus_snapshot <pasta_do_pacote> <foto.png> [ganho]\n");
    return 2;
  }
  juce::ScopedJuceInitialiser_GUI juce;
  const juce::File folder(juce::File::getCurrentWorkingDirectory().getChildFile(juce::String::fromUTF8(argv[1])));
  const juce::File out(juce::File::getCurrentWorkingDirectory().getChildFile(juce::String::fromUTF8(argv[2])));
  const float gain = argc > 3 ? static_cast<float>(std::atof(argv[3])) : 3.5f;

  NexusProcessor processor;
  processor.setPlayConfigDetails(2, 2, 48000.0, 128);
  processor.prepareToPlay(48000.0, 128);
  processor.loadAmpPack(0, 0, folder);

  const auto signal = nexus::makeGuitarLikeSignal(48000.0, 4.0, 2);
  juce::AudioBuffer<float> buffer(2, 128);
  juce::MidiBuffer midi;
  auto* param = processor.params.getParameter("ganho");
  for (int round = 0; round < 3000 && processor.captureCount(NexusProcessor::ampSlot(0, 0)) == 0; ++round)
  {
    buffer.clear();
    processor.processBlock(buffer, midi);
    juce::Thread::sleep(2);
  }
  param->setValueNotifyingHost(param->convertTo0to1(gain));
  for (size_t start = 0; start + 128 <= signal.size(); start += 128)
  {
    for (int ch = 0; ch < 2; ++ch)
      for (int i = 0; i < 128; ++i)
        buffer.setSample(ch, i, static_cast<float>(signal[start + static_cast<size_t>(i)]));
    processor.processBlock(buffer, midi);
  }

  // Quinto argumento opcional: "pedal:<visual>" ou "caixa" escolhe o palco.
  const juce::String stage = argc > 4 ? juce::String(argv[4]) : juce::String();
  if (stage.startsWith("pedal:"))
  {
    // O pedal 1 recebe o pacote no lugar do cabeçote (a cadeia abre sem pedais).
    ChainLayout l;
    l.pre = {0};
    processor.setLayout(l);
    processor.loadPedalPack(0, folder);
    for (int t = 0; t < 3000 && !processor.slotLoaded(0); ++t)
    {
      buffer.clear();
      processor.processBlock(buffer, midi);
      juce::Thread::sleep(2);
    }
    // Mais alguns blocos: o áudio troca para o pacote novo e publica as capturas.
    for (int t = 0; t < 100; ++t)
    {
      buffer.clear();
      processor.processBlock(buffer, midi);
    }
    auto* look = processor.params.getParameter("p1_visual");
    look->setValueNotifyingHost(look->convertTo0to1(stage.fromFirstOccurrenceOf(":", false, false).getFloatValue()));
    // NEXUS_MODO=6 (por exemplo): modo dos pedais de vários modos (Juniper Sky, Lime Echo, Plum Mod).
    if (const char* mode = std::getenv("NEXUS_MODO"))
    {
      auto* m = processor.params.getParameter("p1_modo");
      m->setValueNotifyingHost(m->convertTo0to1(static_cast<float>(std::atof(mode))));
    }
  }
  // NEXUS_IDIOMA=ja (por exemplo) tira a foto naquele idioma, sem mudar a preferência salva.
  if (const char* lang = std::getenv("NEXUS_IDIOMA"))
  {
    nexus::ui::language::applyStartup();
    nexus::ui::language::apply(juce::String(lang), false);
  }
  // NEXUS_CADEIA=dupla: dois cabeçotes, um pedal antes e dois entre cabeçote e caixa.
  if (const char* chain = std::getenv("NEXUS_CADEIA"); chain != nullptr && juce::String(chain) == "dupla")
  {
    ChainLayout l;
    l.dual = true;
    l.pre = {0};
    l.loop[0] = {1};
    l.loop[1] = {2};
    l.post = {3};
    processor.setLayout(l);
  }
  // Quarto argumento "estudio:<pasta da música>": abre o estúdio tocando essa
  // música (sem a guitarra, como para tocar junto), um minuto adentro.
  if (stage.startsWith("estudio:"))
  {
    nexus::studio::Library lib;
    lib.rescan();
    if (const auto* song = lib.find(stage.fromFirstOccurrenceOf(":", false, false)))
    {
      auto& player = processor.studio();
      player.open(*song);
      auto mix = song->mix;
      mix.mute[nexus::studio::guitar] = true;
      mix.loopStart = 52.0;
      mix.loopEnd = 71.0;
      mix.loop = true;
      mix.speed = 0.75;
      player.setMix(mix);
      lib.saveMix(song->id(), mix); // a tela mostra a mistura guardada (volta no fim)
      player.seek(60.0);
      player.setPlaying(true);
      for (int i = 0; i < 48000 / 128; ++i)
      {
        buffer.clear();
        processor.processBlock(buffer, midi);
        juce::Thread::sleep(1);
      }
    }
  }
  // Quarto argumento "mark5:<pasta do pacote>": cabeçote Mark Five no canal 3
  // com esse pacote e a curva em "V" no equalizador gráfico.
  if (stage.startsWith("mark5:"))
  {
    auto set = [&](const char* id, float v) { processor.params.getParameter(id)->setValueNotifyingHost(processor.params.getParameter(id)->convertTo0to1(v)); };
    set("cabecote", 1.0f);
    set("canal", 2.0f);
    set("ganho_3", 7.0f);
    const float curve[5] = {3.0f, 1.0f, -5.0f, 1.5f, 3.0f};
    const char* ids[5] = {"eq80", "eq240", "eq750", "eq2200", "eq6600"};
    for (int k = 0; k < 5; ++k)
      set(ids[k], curve[k]);
    processor.loadAmpPack(0, 2, juce::File(stage.fromFirstOccurrenceOf(":", false, false)));
    for (int t = 0; t < 3000 && !processor.channelLoaded(0, 2); ++t)
    {
      buffer.clear();
      processor.processBlock(buffer, midi);
      juce::Thread::sleep(2);
    }
    for (int t = 0; t < 200; ++t)
    {
      buffer.clear();
      processor.processBlock(buffer, midi);
    }
  }
  std::unique_ptr<juce::AudioProcessorEditor> editor(processor.createEditor());
  if (auto* e = dynamic_cast<NexusEditor*>(editor.get()))
  {
    if (stage.startsWith("look:"))
    {
      // Cabeçote pelo número do visual (foto de cada um).
      auto* look = processor.params.getParameter(NexusProcessor::ampParam(0, "cabecote"));
      look->setValueNotifyingHost(look->convertTo0to1(stage.fromFirstOccurrenceOf(":", false, false).getFloatValue()));
      e->select({NexusEditor::Kind::amp, 0});
    }
    else if (stage == "pedaleira")
    {
      // Pedaleira: cinco pedais (três antes, um entre cabeçote e caixa, um
      // depois), alguns ligados, com o segundo escolhido.
      ChainLayout l;
      l.pre = {0, 1, 2};
      l.loop[0] = {3};
      l.post = {4};
      processor.setLayout(l);
      const int looks[5] = {0, NexusProcessor::kDropLook, 3, 12, 20};
      for (int i = 0; i < 5; ++i)
      {
        auto* look = processor.params.getParameter(NexusProcessor::pedalParam(i, "visual"));
        look->setValueNotifyingHost(look->convertTo0to1(static_cast<float>(looks[i])));
        processor.params.getParameter(NexusProcessor::pedalParam(i, "ligado"))->setValueNotifyingHost(i % 2 == 0 ? 1.0f : 0.0f);
      }
      e->select({NexusEditor::Kind::pedal, 1});
    }
    else if (stage == "gate")
    {
      // Pedal 1 com o visual do Pear Gate.
      auto* look = processor.params.getParameter(NexusProcessor::pedalParam(0, "visual"));
      look->setValueNotifyingHost(look->convertTo0to1(static_cast<float>(NexusProcessor::kGateLook)));
      e->select({NexusEditor::Kind::pedal, 0});
    }
    else if (stage.startsWith("pedal"))
      e->select({NexusEditor::Kind::pedal, 0});
    else if (stage == "caixa")
      e->select({NexusEditor::Kind::cab, 0});
    else if (stage.startsWith("caixa_ir:"))
    {
      // "caixa_ir:<visual>:<arquivo do IR>": caixa com o IR próprio (os microfones do nome aparecem).
      const auto rest = stage.fromFirstOccurrenceOf(":", false, false);
      auto* cab = processor.params.getParameter(NexusProcessor::ampParam(0, "gabinete"));
      cab->setValueNotifyingHost(cab->convertTo0to1(rest.upToFirstOccurrenceOf(":", false, false).getFloatValue()));
      processor.params.getParameter(NexusProcessor::ampParam(0, "caixa"))->setValueNotifyingHost(1.0f);
      processor.loadIrAsync(0, juce::File(rest.fromFirstOccurrenceOf(":", false, false)));
      for (int t = 0; t < 3000 && !processor.irLoaded(0); ++t)
      {
        juce::MessageManager::getInstance()->runDispatchLoopUntil(2);
        buffer.clear();
        processor.processBlock(buffer, midi);
      }
      e->select({NexusEditor::Kind::cab, 0});
    }
    else if (stage == "caixa_mark" || stage.startsWith("caixa:"))
    {
      // Caixa pelo número do visual (caixa_mark é a 1).
      auto* cab = processor.params.getParameter(NexusProcessor::ampParam(0, "gabinete"));
      const float n = stage == "caixa_mark" ? 1.0f : stage.fromFirstOccurrenceOf(":", false, false).getFloatValue();
      cab->setValueNotifyingHost(cab->convertTo0to1(n));
      e->select({NexusEditor::Kind::cab, 0});
    }
    else if (stage.startsWith("afinador"))
    {
      e->select({NexusEditor::Kind::tuner, 0});
      // "afinador:blues": toca o teste de som e lê o afinador (como na tela).
      if (stage.contains(":"))
      {
        processor.testSource().setStyle(nexus::TestSource::blues);
        processor.testSource().setPlaying(true);
        for (int t = 0; t < 400; ++t)
        {
          processor.processBlock(buffer, midi);
          if (t % 10 == 0)
            e->updateTuner();
        }
      }
    }
    else if (stage == "capturas")
      e->showCaptures();
    else if (stage.startsWith("estudio"))
      e->setMode(1);
    else if (stage == "eq")
    {
      // Equalizador do fim com algumas faixas de exemplo.
      auto set = [&](const juce::String& id, float v) { processor.params.getParameter(id)->setValueNotifyingHost(processor.params.getParameter(id)->convertTo0to1(v)); };
      const struct { int type; float f, g, q; } bands[] = {{3, 80.0f, 0.0f, 1.0f}, {0, 220.0f, 3.0f, 1.2f}, {0, 650.0f, -5.0f, 2.0f},
                                                           {5, 1800.0f, 0.0f, 8.0f}, {0, 3200.0f, 4.0f, 1.0f}, {4, 9000.0f, 0.0f, 1.0f}};
      for (int i = 0; i < 6; ++i)
      {
        set(NexusProcessor::eqParam(i, "ligado"), 1.0f);
        set(NexusProcessor::eqParam(i, "tipo"), static_cast<float>(bands[i].type));
        set(NexusProcessor::eqParam(i, "freq"), bands[i].f);
        set(NexusProcessor::eqParam(i, "ganho"), bands[i].g);
        set(NexusProcessor::eqParam(i, "q"), bands[i].q);
      }
      e->select({NexusEditor::Kind::eq, 0});
    }
  }
  // Deixa o relógio da tela atualizar textos e medidor.
  juce::MessageManager::getInstance()->runDispatchLoopUntil(300);
  const auto image = editor->createComponentSnapshot(editor->getLocalBounds(), true, 2.0f);
  out.deleteFile();
  juce::FileOutputStream stream(out);
  juce::PNGImageFormat().writeImageToStream(image, stream);
  std::printf("Foto da tela em %s (%d capturas)\n", out.getFullPathName().toRawUTF8(), processor.captureCount(NexusProcessor::ampSlot(0, 0)));
  editor.reset();
  return processor.captureCount(NexusProcessor::ampSlot(0, 0)) > 0 ? 0 : 1;
}
