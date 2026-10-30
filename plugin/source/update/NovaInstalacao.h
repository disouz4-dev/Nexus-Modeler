#pragma once

#include <juce_data_structures/juce_data_structures.h>

// A 0.2.0-beta.3 é uma instalação nova: não lê nada das versões antigas
// (preferências, equipamentos baixados, presets, índice, capturas da
// TONE3000). Na primeira abertura, o que uma versão antiga deixou vai para
// uma pasta de reserva ao lado ("Nexus Modeler (versões antigas)"), sem
// apagar nada; o programa nunca lê a reserva. Só o idioma da tela continua.
namespace nexus::update
{
// Geração dos dados: muda só quando uma versão nova não pode ler os dados das
// anteriores.
inline constexpr int kDataGeneration = 3;

// Pastas que o Nexus usa (NEXUS_DADOS_PASTA e NEXUS_DOCUMENTOS_PASTA trocam
// a raiz nos testes).
juce::File dataRoot();      // Application Support/Nexus Modeler (Mac)
juce::File documentsRoot(); // Documentos/Nexus Modeler
// Arquivo de preferências na pasta de dados (preferencias.settings,
// tone3000.settings...). Todos os arquivos do Nexus saem destas raízes: os
// testes trocam as duas e nunca tocam nos dados de quem roda o teste.
juce::File settingsFile(const juce::String& name);

// Faz a troca uma vez por computador (a primeira cópia do Nexus que abrir,
// programa independente ou plugin). Devolve a pasta de reserva criada (vazia
// se não havia nada antigo).
juce::File startFresh();
} // namespace nexus::update
