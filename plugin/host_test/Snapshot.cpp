// Monta o plugin por dentro (sem programa de gravação), carrega um pacote,
// toca alguns segundos girando o ganho e salva uma foto da tela.
//
// Uso: nexus_snapshot <pasta do pacote> <foto.png> [posição do ganho] [pedal:<visual> | caixa]

#include <cstdio>

#include "../source/PluginEditor.h"
#include "../source/PluginProcessor.h"
#include "nexus/TestSignal.h"

int main(int argc, char** argv)
{
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
  processor.loadPackAsync(folder);

  const auto signal = nexus::makeGuitarLikeSignal(48000.0, 4.0, 2);
  juce::AudioBuffer<float> buffer(2, 128);
  juce::MidiBuffer midi;
  auto* param = processor.params.getParameter("ganho");
  for (int round = 0; round < 3000 && processor.captureCount() == 0; ++round)
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
    auto* look = processor.params.getParameter("pedal_modelo");
    look->setValueNotifyingHost(look->convertTo0to1(stage.fromFirstOccurrenceOf(":", false, false).getFloatValue()));
  }
  std::unique_ptr<juce::AudioProcessorEditor> editor(processor.createEditor());
  if (auto* e = dynamic_cast<NexusEditor*>(editor.get()))
  {
    if (stage.startsWith("pedal"))
      e->showStage(NexusEditor::Stage::pedal);
    else if (stage == "caixa")
      e->showStage(NexusEditor::Stage::cab);
  }
  // Deixa o relógio da tela atualizar textos e medidor.
  juce::MessageManager::getInstance()->runDispatchLoopUntil(300);
  const auto image = editor->createComponentSnapshot(editor->getLocalBounds(), true, 2.0f);
  out.deleteFile();
  juce::FileOutputStream stream(out);
  juce::PNGImageFormat().writeImageToStream(image, stream);
  std::printf("Foto da tela em %s (%d capturas)\n", out.getFullPathName().toRawUTF8(), processor.captureCount());
  editor.reset();
  return processor.captureCount() > 0 ? 0 : 1;
}
