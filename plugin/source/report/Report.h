#pragma once

#include <juce_core/juce_core.h>

#include <functional>

// Relatório de problemas para os issues do GitHub.
//
// - Registro: os erros do programa (pacote que não abre, IR, TONE3000, placa
//   de som, atualização) vão para "registro.txt" na pasta de ajustes do Nexus.
//   Sem senhas nem chaves; a pasta do usuário vira "~".
// - Travamento: no programa independente, se o Nexus fechar sozinho, o
//   relatório (versão, sistema e a pilha de chamadas) fica em
//   "travamento.txt"; na abertura seguinte o Nexus oferece enviar.
// - Enviar: manda o relatório para a ponte (tools/relatorio, na Cloudflare),
//   que cria o issue no GitHub sem a pessoa precisar de conta. Sem a ponte
//   (sem internet ou ponte fora do ar), abre a página de issue nova do GitHub
//   já preenchida e copia o texto completo.
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

struct Sent
{
  int number = 0;           // número do issue criado (0: não criou)
  juce::String address;     // endereço do issue
  juce::String error;
  bool openedBrowser = false; // caiu no plano B (GitHub no navegador)
};

// Envia em segundo plano; "done" roda na linha da tela.
void send(Kind kind, const juce::String& description, const juce::String& details, std::function<void(Sent)> done = {});
} // namespace nexus::report
