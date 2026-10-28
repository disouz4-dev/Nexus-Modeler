# Ponte dos relatórios (Cloudflare Workers)

O botão "Reportar um problema" do Nexus manda o relatório para esta ponte,
sem login. A ponte cria o issue no GitHub com uma chave guardada só nela.

## Publicar (uma vez)

1. Conta gratuita em https://dash.cloudflare.com/sign-up
2. Chave do GitHub: https://github.com/settings/personal-access-tokens/new
   - Repository access: Only select repositories → Nexus-Modeler
   - Permissions → Repository permissions → Issues: Read and write
   - Nada mais. Copie a chave gerada.
3. Nesta pasta:
   - `npx wrangler login` (abre o navegador para entrar na Cloudflare)
   - `npx wrangler secret put GITHUB_TOKEN` (cole a chave)
   - `npx wrangler deploy` (mostra o endereço da ponte)

## Limites

- 3 relatórios por minuto por endereço de rede.
- Só aceita pedidos com o identificador do Nexus Modeler.
- Caminhos de pasta com nome de usuário e e-mails são trocados antes de ir
  para o GitHub.
