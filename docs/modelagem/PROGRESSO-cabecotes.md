# Modelagem fiel: cabeçotes (94)

Ramo `fiel-cabecotes` (saiu de `modelagem-fiel` em 009655c), pasta
`Nexus Modeler-fiel-cabecotes`. Ponto de retomada desta frente: se a luz cair,
leia este arquivo, `git log --oneline -5` e siga da primeira etapa sem [x].

## Como funciona

- `tools/gear_studio/amp_fiel.py`: gerador novo (cabeçote, combo, rack,
  caixinha). Receita com `fileiras` vai para ele; as antigas continuam no
  `amp_head.py` (o único acréscimo lá é o despacho no começo do `build`).
- `tools/gear_studio/fichas_cabecotes/loteN.py`: a ficha de cada cabeçote
  (formato, medidas, cores, painel, botões em ordem, chaves).
- `python3 tools/gear_studio/cabecotes_fiel.py [id...]` escreve
  `recipes/<id>.json` a partir da ficha, guardando nome, família e canais da
  receita, e confere que os botões que tocam (ganho, grave, médio, agudo,
  presença, volume e os deslizantes eq*) continuam todos lá com o mesmo id e
  que há um LED de canal por canal.
- Botões e chaves do original que não tocam entram com `"fixo": true`
  (desenhados, fora do mapa). Rótulos: os que a tela já traduz (GANHO,
  GRAVE...) quando o original diz a mesma coisa; senão o texto do original
  (MASTER, PRE-AMP, TREBLE...), que a tela mostra como está.
- `notas_modelagem` na receita: o que ficou diferente do original e precisa
  de decisão do Diego.

## Prévia e render

- Mandar: `tools/gear_studio/lab2_sync.sh repo-fiel-cabecotes` (a marca SVG
  vai à parte: `scp assets/marca/nexus_marca.svg lab2:nexus-gear/repo-fiel-cabecotes/assets/marca/`).
- Prévia: ver o comando no pedido; final pela `fila3d.sh pedir --repo ~/nexus-gear/repo-fiel-cabecotes`.

## Mudanças em arquivos compartilhados

- `amp_head.py` (desta frente): `import amp_fiel` e despacho no `build`.
- `build.py`, `studio.py`, `knobs.py`: nenhuma mudança.

## Etapas

- [x] 1. Recursos novos no gerador (prévias de teste: brit800, mark5,
      silvertwin, topboost15, britrack1, tiny15, vier100, tri100)
- [ ] 2. Receitas em lotes (~10) + render de cada lote
- [ ] 3. Conferir na fila que todos ficaram "pronta"

## Lotes

| lote | cabeçotes | commit | render |
|------|-----------|--------|--------|
| 1 | brit800 mark5 silvertwin topboost15 britrack1 tiny15 vier100 tri100 | | |

## Decisões para o Diego

(ver `notas_modelagem` de cada receita; resumo no fim de cada lote)
