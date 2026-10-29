#pragma once

#include <juce_core/juce_core.h>

#include <vector>

// Microfones de um IR. O arquivo WAV não tem lugar para isso: quem grava
// escreve no nome ("Engl_SM57_Cone.wav", "M25 UL 1960TV 4x12 M201 1.25in
// 0.0in SA73.wav", "Shiva 212_V30-R121 01.wav"). A tela da caixa desenha cada
// microfone na frente do falante, na posição em que o IR foi gravado.
namespace nexus::ui
{
// Os cinco modelos 3D de microfone (a ordem é a das imagens "microfone_*").
enum class MicKind
{
  dynamicStick, // dinâmico de bastão (SM57, M201...)
  dynamicSide,  // dinâmico lateral (MD421, e906...)
  largeCondenser,
  pencilCondenser,
  ribbon
};
constexpr int kMicKinds = 5;

struct MicShot
{
  MicKind kind = MicKind::dynamicStick;
  juce::String model; // como veio no nome do arquivo ("SM57")
  // Falante da caixa, lido da esquerda para a direita e de cima para baixo
  // (UL = 0, UR = 1, LL = 2, LR = 3); -1 = o nome não diz.
  int speaker = -1;
  // Distância do centro do falante: em polegadas, quando o nome diz ("1.25in");
  // senão pela palavra (centro, borda da calota, cone, borda), em fração do
  // raio do falante. -1 = o nome não diz.
  float inches = -1.0f;
  float fraction = -1.0f;
};

// Microfones achados no nome do arquivo (vazio = o nome não diz).
std::vector<MicShot> micsOfIr(const juce::String& fileName);
juce::String micKindName(MicKind kind);
} // namespace nexus::ui
