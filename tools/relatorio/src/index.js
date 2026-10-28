// Ponte dos relatórios do Nexus Modeler.
//
// O programa manda o relatório para cá (sem login nenhum) e a ponte cria o
// issue no GitHub com uma chave que só ela conhece (segredo GITHUB_TOKEN, com
// permissão só de issues deste repositório). Assim a chave nunca vai dentro do
// programa, que tem código aberto.
//
// POST /relatorio  { tipo: "problema" | "travamento", versao, descricao, sistema, registro }
// Resposta: { numero, endereco } ou { erro }

const LIMITES = { versao: 40, descricao: 4000, sistema: 1500, registro: 20000 };

function resposta(corpo, status = 200) {
  return new Response(JSON.stringify(corpo), {
    status,
    headers: { "content-type": "application/json; charset=utf-8" },
  });
}

function texto(v, max) {
  return typeof v === "string" ? v.slice(0, max) : "";
}

// Tira o que pode identificar a pessoa: caminhos de pasta com o nome do
// usuário e endereços de e-mail.
function limpar(s) {
  return s
    .replace(/\/(Users|home)\/[^/\s]+/g, "~")
    .replace(/[A-Za-z]:\\Users\\[^\\\s]+/g, "~")
    .replace(/[\w.+-]+@[\w-]+\.[\w.-]+/g, "(e-mail)");
}

export default {
  async fetch(request, env) {
    const url = new URL(request.url);
    if (request.method !== "POST" || url.pathname !== "/relatorio")
      return resposta({ erro: "nada aqui" }, 404);
    if (!(request.headers.get("user-agent") || "").startsWith("NexusModeler/"))
      return resposta({ erro: "só o Nexus Modeler envia relatórios" }, 403);

    const ip = request.headers.get("cf-connecting-ip") || "desconhecido";
    const { success } = await env.LIMITE.limit({ key: ip });
    if (!success) return resposta({ erro: "muitos relatórios seguidos; tente daqui a um minuto" }, 429);

    let dados;
    try {
      dados = await request.json();
    } catch {
      return resposta({ erro: "relatório inválido" }, 400);
    }
    const tipo = dados.tipo === "travamento" ? "travamento" : "problema";
    const versao = texto(dados.versao, LIMITES.versao).replace(/[^\w.\-+]/g, "");
    const descricao = limpar(texto(dados.descricao, LIMITES.descricao)).trim();
    const sistema = limpar(texto(dados.sistema, LIMITES.sistema));
    const registro = limpar(texto(dados.registro, LIMITES.registro));
    if (tipo === "problema" && descricao.length < 3)
      return resposta({ erro: "conte o que aconteceu" }, 400);

    const primeira = descricao.split("\n")[0].slice(0, 70);
    const titulo = `[${versao || "?"}] ` + (tipo === "travamento" ? "O Nexus fechou sozinho" : primeira || "Problema");
    const corpo = [
      "### O que aconteceu", "", descricao || "_(sem descrição)_", "",
      "### Versão e sistema", "", "```text", sistema, "```", "",
      "### Registro de erros", "", "```text", registro || "(vazio)", "```", "",
      "_Enviado pelo botão de relatório do Nexus Modeler._",
    ].join("\n");

    const gh = await fetch(`https://api.github.com/repos/${env.REPOSITORIO}/issues`, {
      method: "POST",
      headers: {
        authorization: `Bearer ${env.GITHUB_TOKEN}`,
        accept: "application/vnd.github+json",
        "user-agent": "nexus-relatorio",
        "x-github-api-version": "2022-11-28",
      },
      body: JSON.stringify({ title: titulo, body: corpo, labels: [tipo] }),
    });
    if (!gh.ok) return resposta({ erro: `GitHub respondeu ${gh.status}` }, 502);
    const issue = await gh.json();
    return resposta({ numero: issue.number, endereco: issue.html_url });
  },
};
