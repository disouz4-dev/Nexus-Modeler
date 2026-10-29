# Modelagem fiel: cabinets (progresso)

Ramo `fiel-cabinets` (saiu de `modelagem-fiel`), pasta `Nexus Modeler-fiel-cabinets`.
Ponto de retomada desta frente: ver a última etapa marcada e seguir da próxima.
Cada etapa termina com um commit só com os arquivos desta frente
(`tools/gear_studio/cab.py`, receitas de tipo "caixa", este arquivo).

## Escopo

As 27 caixas com veredito "trocar" em `docs/modelagem/verificado_caixas.json`.
`solar_412` e `fliptop_115` têm veredito "ok" (confiança baixa) e ficam como estão.

## Etapas

- [x] 1. Recursos novos no `cab.py` (com prévias no lab2)
- [ ] 2a. Lote 1: brit412, mark5_412, block_412, sl_412, be_412, citrus_412, star_412
- [ ] 2b. Lote 2: blue_212, twin_212, tweed_410, match_212, silver_112, mark_112, jazz_212
- [ ] 2c. Lote 3: power_412, uber_412, country_412, vier_412, hi_412, rg_412, glass_412
- [ ] 2d. Lote 4: vault_810, granite_410, hart_410, glass_410, bassline_215, solar_215
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
