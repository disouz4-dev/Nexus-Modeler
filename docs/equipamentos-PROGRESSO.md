# Equipamentos baixáveis: progresso

Plano: `docs/equipamentos-baixaveis.md` (commit `4d83ec9`, ramo
`claude/vibrant-heisenberg-8038e5`), etapas 1, 2, 4 e 5 da seção 6. Ramo de
trabalho: `equipamentos-novos`, pasta `Nexus Modeler-equipamentos`, compilação
em `build-teste/`.

Decisões do dono que valem sobre o plano:

- gratuitos no próprio `disouz4-dev/Nexus-Modeler`, ramo `main`, lidos por
  `raw.githubusercontent.com`; capturas apontam para a TONE3000;
- um baixador só (o `ArteNova` de 3901d61, estendido), um índice só
  (`assets/gear/indice.json`) e uma pasta só (`Equipamentos/<id>/`);
- sem assinatura digital por ora: o índice traz o sha256 de cada arquivo e o
  programa confere antes de instalar;
- ficha de cada peça em `assets/gear/<id>_manifesto.json` (padrão `<id>_*`
  para o `tools/renomear_pedais.py`);
- a automação que empacota cada peça roda no lab2 (Linux, python3 + Pillow).

## Etapas

- [x] **1. Catálogo de fábrica unificado.** `plugin/source/gear/GearCatalog`
  lê as fichas embutidas (186: 63 pedais, 94 cabeçotes, 29 caixas) e as da
  pasta `Equipamentos/`. Editor, navegador e motor dos pedais leem do
  catálogo; as tabelas `kPedalPrefixes`, `kPedalInfo`, `kPedalKinds`,
  `kAmpPrefixes`, `kAmpInfo`, `kCabOfAmp`, `kCabPrefixes` e `kCabInfo` saíram.
  Fichas geradas por `tools/gear_studio/manifestos.py` a partir das tabelas de
  3901d61 (`--conferir` compara). `tools/site/catalogo.py` lê as fichas.
  Teste: `nexus_gear_test assets/gear` (catálogo = fichas) e fotos da tela
  (`nexus_snapshot`) iguais às de antes, fora o texto de uso do processador.
- [x] **2. Id em texto no estado.** `p<n>_visual_id`, `cabecote_id[_b]`,
  `gabinete_id[_b]` no estado; parâmetros de escolha congelados; peça baixada
  guarda a escolha à parte e deixa o parâmetro no de fábrica do mesmo motor;
  id não instalado fica marcado (pedal passa direto) e não se perde ao salvar.
  Teste: `nexus_gear_test` monta o estado da versão antiga (sem ids) e confere.
- [x] **4. Pacote no disco e instalação** (sem assinatura). Pasta única
  `Equipamentos/<id>/` com os arquivos `<id>_*` (a `Arte/` antiga é migrada
  pelo catálogo ao abrir); `GearArt::loadFolder`; `GearCatalog::validate`,
  `installFolder` (troca por renomear) e `installPackage` (zip
  `<id>-<versao>.equipamento`, recusa nome de outra peça, `..`, extensão fora
  de `.json .png .jpg .wav .nam` e arquivo acima de 200 MB); arrastar o
  pacote para a janela instala; cartão "<nome> não está instalado" no título
  da posição. Ferramenta única `tools/gear_studio/montar_equipamento.py <id>
  --saida <render>` (python3 + Pillow, roda no lab2): arte como as outras,
  ficha, índice, `--pacote` e `--commit`. `indice_arte.py` passou a pôr a
  ficha, o sha256 e o tamanho de cada arquivo no índice.
  Teste: `nexus_gear_test assets/gear <pacote>` com um pedal novo
  (`caqui_drive`, montado de um render de mentira) e `nexus_arte_test`.
- [x] **5. Busca e download no mesmo repositório.** `update/ArteNova` virou
  `update/GearStore` (mesmo índice, mesma pasta): lê
  `raw.githubusercontent.com/disouz4-dev/Nexus-Modeler/main/assets/gear/indice.json`
  (ou `NEXUS_EQUIPAMENTOS_URL` / `NEXUS_ARTE_URL`), pula o que pede Nexus
  mais novo ou motor desconhecido, e **procura e baixa sozinho ao abrir**,
  conferindo o sha256 de cada arquivo, e instala pelo catálogo. Equipamento
  novo entra na lista na hora; peça que já existia (pode estar tocando) troca
  ao abrir de novo. O ícone saiu da barra de cima e foi para a coluna de
  equipamentos, ao lado da pesquisa (`GearBrowser::setAccessory`), e só mostra
  o estado (procurando, baixando com andamento, quantos chegaram, erro; clique
  só no erro, para tentar de novo). O cartão "não está instalado" ganhou o
  nome do índice e o link "Baixar" (ou "Precisa de uma versão mais nova").
  Teste de ponta a ponta: `nexus_arte_test <remoto> caqui_drive "Caqui Drive"`.

## Teste de ponta a ponta (como foi rodado)

```
# cópia do projeto como "GitHub" local
mkdir -p /tmp/remoto/tools /tmp/remoto/assets
cp -R tools/gear_studio /tmp/remoto/tools/ && cp -R assets/gear /tmp/remoto/assets/ && cp CMakeLists.txt /tmp/remoto/
# render de mentira (qualquer saída do build.py serve) e a peça nova
cd /tmp/remoto && python3 tools/gear_studio/montar_equipamento.py caqui_drive --tipo pedal \
  --saida <render> --inspiracao "Fulltone OCD" --categoria "Drive e distorção" --busca OCD --pacote /tmp/pacotes
build-teste/plugin/nexus_arte_test_artefacts/Release/nexus_arte_test /tmp/remoto/assets/gear caqui_drive "Caqui Drive"
build-teste/plugin/nexus_gear_test_artefacts/Release/nexus_gear_test assets/gear /tmp/pacotes/caqui_drive-1.0.0.equipamento
```

## Para retomar

1. `ninja -C build-teste nexus_gear_test nexus_arte_test nexus_snapshot`
2. `build-teste/plugin/nexus_gear_test_artefacts/Release/nexus_gear_test assets/gear`
3. Seguir da primeira etapa sem `[x]`.
