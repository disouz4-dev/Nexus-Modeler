#pragma once

#include <juce_core/juce_core.h>

// Relatório de problemas para os issues do GitHub.
//
// - Registro: os erros do programa (pacote que não abre, IR, TONE3000, placa
//   de som, atualização) vão para "registro.txt" na pasta de ajustes do Nexus.
//   Sem senhas nem chaves; a pasta do usuário vira "~".
// - Travamento: no programa independente, se o Nexus fechar sozinho, o
//   relatório (versão, sistema e a pilha de chamadas) fica em
//   "travamento.txt"; na abertura seguinte o Nexus oferece enviar.
// - Enviar: abre a página de issue nova do GitHub já preenchida (modelo
//   .github/ISSUE_TEMPLATE/problema.yml). A pessoa só confere e envia; precisa
//   de uma conta no GitHub. O texto completo também vai para a área de
//   transferência, para colar se a página cortar.
namespace nexus::report
{
// Pasta de ajustes do Nexus (a mesma das preferências).
juce::File folder();
juce::File logFile();

// Guarda uma linha no registro. Pode ser chamada de qualquer linha de
// execução, menos a do áudio.
void log(const juce::String& where, const juce::String& what);

// Últimas linhas do registro (para o relatório).
juce::String recentLog(int maxLines);

// Versão, sistema, processador, memória e programa de gravação.
juce::String systemInfo();

// Programa independente: grava o relatório se o Nexus fechar sozinho.
void installCrashHandler();
// Relatório de travamento da última vez, se houver (e marca como visto).
juce::String takeCrash();

enum class Kind
{
  problem, // relatado pela pessoa
  crash,   // o Nexus fechou sozinho
};

// Endereço da issue nova já preenchida.
juce::URL issueUrl(Kind kind, const juce::String& description, const juce::String& details);

// Abre a issue no navegador e copia o texto completo.
void send(Kind kind, const juce::String& description, const juce::String& details);
} // namespace nexus::report
