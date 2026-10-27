#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include <functional>
#include <memory>
#include <vector>

// Pacotes no disco e o mapeamento das capturas.
//
// Todo pacote fica em Documentos/Nexus Modeler/<equipamento>/<pacote>: os
// baixados da TONE3000 e os inseridos à mão (pasta, .zip ou arquivos .nam,
// que são copiados ou descompactados para lá).
//
// O Nexus tenta descobrir sozinho a posição do botão de cada captura (pelo
// pack.json ou pelo número no nome: "Gain 7" -> 7). Quando não consegue (nome
// sem número, duas capturas na mesma posição, ou o pacote marcado para
// mapear), abre a tela de mapeamento: cada captura ganha a sua posição de 0 a
// 10, e o pack.json é gravado com o resultado.
namespace nexus::ui
{
struct MappedCapture
{
  juce::File file;
  double position = -1.0; // -1: não usar
};

namespace packs
{
// Lê as capturas da pasta e a posição de cada uma. needsMapping fica
// verdadeiro quando o usuário precisa conferir o mapeamento.
std::vector<MappedCapture> analyze(const juce::File& folder, bool& needsMapping);
// Leva a escolha do usuário (pasta, .zip ou arquivos .nam) para
// Documentos/Nexus Modeler/<equipamento>/ e devolve a pasta do pacote.
// Erro: pasta inválida e "error" preenchido.
juce::File importInto(const juce::Array<juce::File>& chosen, const juce::String& gearName, juce::String& error);
// Grava o mapeamento no pack.json da pasta (mantém o resto do arquivo).
bool saveMapping(const juce::File& folder, const std::vector<MappedCapture>& captures, const juce::String& knobName);
} // namespace packs

class PackMapper : public juce::Component
{
public:
  PackMapper();
  ~PackMapper() override;

  // Abre para a pasta; knobName: "Ganho" no cabeçote, "Drive" no pedal.
  void open(const juce::File& folder, const juce::String& gearName, const juce::String& knobName);
  void retranslate();

  void paint(juce::Graphics&) override;
  void resized() override;
  void mouseUp(const juce::MouseEvent&) override;

  // Mapeamento gravado: carregar o pacote.
  std::function<void(const juce::File& folder)> onSaved;
  std::function<void()> onClose;

private:
  class Row;
  juce::Rectangle<int> card() const;
  void validate();
  void spread();

  juce::File mFolder;
  juce::String mGearName, mKnobName;
  std::vector<MappedCapture> mCaptures;
  juce::Component mRows;
  juce::Viewport mViewport;
  std::vector<std::unique_ptr<Row>> mRowViews;
  juce::String mProblem;
  juce::TextButton mSpread{juce::String::fromUTF8("Distribuir de 0 a 10")};
  juce::TextButton mSave{"Salvar e carregar"};
  juce::TextButton mCancel{"Cancelar"};
};
} // namespace nexus::ui
