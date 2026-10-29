# Modelagem fiel: cabinets (progresso)

Ramo `fiel-cabinets` (saiu de `modelagem-fiel`), pasta `Nexus Modeler-fiel-cabinets`.
Ponto de retomada desta frente: ver a última etapa marcada e seguir da próxima.
Cada etapa termina com um commit só com os arquivos desta frente
(`tools/gear_studio/cab.py`, receitas de tipo "caixa", este arquivo).

## Regra do Diego (29/10, 05h40)

Nada angulado: nenhuma cabinet inclinada (sem o campo `inclinada` nas receitas) e câmera
reta de frente em todas (`quadro.elevacao` = 0). O recurso `inclinada` continua no cab.py,
mas não deve ser usado. Commits do coordenador: e3f343b e 0e863d4.

## Escopo

As 27 caixas com veredito "trocar" em `docs/modelagem/verificado_caixas.json`.
`solar_412` e `fliptop_115` têm veredito "ok" (confiança baixa) e ficam como estão.

## Etapas

- [x] 1. Recursos novos no `cab.py` (com prévias no lab2)
- [x] 2a. Lote 1: brit412, mark5_412, block_412, sl_412, be_412, citrus_412, star_412
- [x] 2b. Lote 2: blue_212, twin_212, tweed_410, match_212, silver_112, mark_112, jazz_212
- [x] 2c. Lote 3: power_412, uber_412, country_412, vier_412, hi_412, rg_412, glass_412
- [x] 2d. Lote 4: vault_810, granite_410, hart_410, glass_410, bassline_215, solar_215
- [ ] 3. Conferir na fila que todas ficaram "pronta"

## Recursos do cab.py (etapa 1)

Todos opcionais; sem eles a receita sai igual ao gerador antigo (conferido com brit412).
- `inclinada` {recuo_m, inicio_rel}: parte de cima da frente inclinada (slant/"A").
  A frente é montada reta e dobrada na dobradiça (tela, friso, logo, grade, tachas).
- `tela.tipo: "grade"`: grade de metal com furos transparentes e falantes de verdade
  atrás (aro, suspensão, cone, calota), em `falantes` linhas x colunas:
  `grade` "perfurada" (chapa), "losango" (metal expandido), "quadrada" (malha soldada);
  `cones: "aluminio"` (Hartke). O rebaixo da frente fica fundo para os cones aparecerem.
- `tela.padrao`: "cesto" (padrão), "losango" (Vox), "listrada" (oxblood tweed,
  prata Fender), "xadrez" (preto e branco fino); `cor_linha`, `passo_mm`, `largura_rel`.
- `couro.tipo`: "levant" (padrão), "tweed" (sarja diagonal laqueada), "carbono", "carpete".
- `friso_m`, `friso_metal`, `friso_onde` ("tela", "abertura", "ambos").
- `painel` (tolex ou chapa de controle com botões) no topo da frente; `logo.no_painel`.
- `faixas` metálicas ou douradas acima/abaixo da tela.
- `cantos` "preto" | "cromo" | "nenhum", `cantos_m`, `cantos_onde` ("todos", "cima", "baixo").
- `tachas`, `rodizios`, `pes` (contam na altura total), `chanfro_m`, `aba_topo_m`.
- `logo.placa` (plaqueta atrás da marca, com borda), `selo.lado` e `selo.placa`.

## Observações

- `falantes` (linhas, colunas, polegadas) vai em todas as receitas; a tela do programa
  espalha os falantes por igual na área dentro da aba de 3 cm. Nas caixas com painel no
  topo (Vox, Matchless, Fender combo, JC) os falantes ficam só na parte da tela: a tela
  do programa ainda não sabe disso (fica para quem mexer no plugin).
- Nada mudou em `studio.py` nem em `build.py`.
- `lab2_sync.sh` não manda `assets/marca/` (a marca NEXUS); sem ela o build falha.
  Copiei à mão: `scp -r assets/marca lab2:nexus-gear/repo-fiel-cabinets/assets/`.

## Registro

- 29/10: etapa 1 (recursos novos no cab.py).
- 29/10: lote 1 nas receitas, prévias conferidas com as fotos; render final pedido na fila.
  Plaqueta atrás da marca passou a ter o tamanho da marca (cab.py).
- 29/10: lote 2 (Vox, Fender Twin/Deluxe/Bassman tweed, Matchless, Mesa 1x12, JC-120), render pedido.
  Prévias agora numa cópia separada no lab2 (`repo-fiel-cabinets-previa`), para não mexer no que a fila está renderizando.
- 29/10: lote 3 (ENGL, Bogner, Laney, Diezel, Hiwatt, Randall, Glass 4x12), render pedido.
  Laney: 765 de caixa mais 60 mm de rodízios (a foto mostra rodízios; a ficha dá a caixa). Randall: os 696 mm já contam os rodízios (incerto).
- 29/10: lote 4 (Ampeg 8x10, GK, Hartke, Darkglass 4x10, Bassman 2x15, Sunn 2x15), render pedido.

## Incertezas (confiança baixa)

- `bassline_215` (Bassman 2x15 silverface): cores, moldura de alumínio e medidas 749 x 1016 x 292
  vêm da triagem de memória, sem página da Fender. Cantoneiras pretas mantidas (sem dado).
- `solar_215` (Sunn 200S): só o tamanho mudou (610 x 1016 x 381, de memória); tela, friso e
  cantoneiras ficaram como estavam, por falta de foto ou ficha confiável.
- `solar_412` e `fliptop_115`: veredito "ok" na conferência; não mexi.
- `hart_410`: a Hartke não diz a ordem dos eixos (25 x 24.5 x 15 pol.); usei 635 L x 622 A.
