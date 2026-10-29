#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include <functional>

// Notas de versão na tela. O NOTAS-DE-VERSAO.md vai embutido no programa:
// na primeira abertura depois de atualizar, o Nexus mostra a seção da versão
// nova. O aviso de versão nova mostra o texto do lançamento no GitHub, que o
// fluxo de pacotes tira do mesmo arquivo.
namespace nexus::ui::whatsnew
{
// A seção "## <versão>" do texto (sem o título). Sem a versão exata, a
// primeira que começa com ela ("0.2.0" acha "0.2.0-beta.1"); nada: vazio.
juce::String section(const juce::String& markdown, const juce::String& version);
// A seção desta versão, das notas embutidas.
juce::String bundled();

// Janela com as notas roláveis. Sem `cancel`, um botão só. `done` recebe
// verdadeiro quando a pessoa aperta o primeiro botão.
void show(const juce::String& title, const juce::String& notes, const juce::String& ok, const juce::String& cancel,
          std::function<void(bool)> done, juce::Component* around);
} // namespace nexus::ui::whatsnew
