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
    // O pedal 1 recebe o pacote no lugar do cabeçote.
    processor.loadPedalPack(0, folder);
    for (int t = 0; t < 3000 && !processor.slotLoaded(0); ++t)
    {
      buffer.clear();
      processor.processBlock(buffer, midi);
      juce::Thread::sleep(2);
    }
    auto* look = processor.params.getParameter("p1_visual");
    look->setValueNotifyingHost(look->convertTo0to1(stage.fromFirstOccurrenceOf(":", false, false).getFloatValue()));
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
  std::unique_ptr<juce::AudioProcessorEditor> editor(processor.createEditor());
  if (auto* e = dynamic_cast<NexusEditor*>(editor.get()))
  {
    if (stage.startsWith("pedal"))
      e->select({NexusEditor::Kind::pedal, 0});
    else if (stage == "caixa")
      e->select({NexusEditor::Kind::cab, 0});
    else if (stage == "capturas")
      e->showCaptures();
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
