#pragma once

#include <juce_core/juce_core.h>

#include <vector>

// Idiomas da tela. O português do Brasil é o texto de origem; os outros vêm
// do pacote de idiomas (assets/idiomas/<código>.txt, embutido no plugin e
// também instalado pelo instalador). A escolha fica nas preferências do
// usuário, comuns ao plugin e ao programa independente; na primeira vez vale
// o idioma do sistema.
namespace nexus::ui::language
{
struct Language
{
  const char* code;   // "pt-BR", "en", "ja"...
  const char* native; // nome na própria língua, em UTF-8
};

const std::vector<Language>& all();

// Código guardado nas preferências; vazio na primeira vez.
juce::String saved();
// Idioma do sistema, entre os disponíveis (inglês se não houver).
juce::String fromSystem();
// Aplica e guarda.
void apply(const juce::String& code, bool save = true);
juce::String current();
// Aplica o guardado ou, na primeira vez, o do sistema.
void applyStartup();
// Arábico se escreve da direita para a esquerda.
bool rightToLeft();
// Escritas que aceitam espaço extra entre letras (latina, cirílica). Árabe,
// hindi e bengali ligam as letras; japonês, chinês e coreano não usam.
bool allowsLetterSpacing();
} // namespace nexus::ui::language
