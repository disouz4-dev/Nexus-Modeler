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
- Imagens com proporção > 1 viram quadradas e abrem a vista na largura: o `build.py`
  só corrige o enquadramento de imagem em pé para as caixas.

## Mudanças em arquivos compartilhados

- `amp_head.py` (desta frente): `import amp_fiel` e despacho no `build`.
- `build.py`, `studio.py`, `knobs.py`: nenhuma mudança.

## Etapas

- [x] 1. Recursos novos no gerador (prévias de teste: brit800, mark5,
      silvertwin, topboost15, britrack1, tiny15, vier100, tri100)
- [x] 2. Receitas em lotes (~10) + render de cada lote: os 94 convertidos e pedidos na fila
- [ ] 3. Conferir na fila que todos ficaram "pronta" (a fila anda ~10 min por cabeçote;
      conferir com `ssh lab2 '~/nexus-gear/fila3d.sh ver'` e repedir o que der erro)
- [ ] 3. Conferir na fila que todos ficaram "pronta"

## Lotes

| lote | cabeçotes | commit | render |
|------|-----------|--------|--------|
| 1 | brit800 mark5 silvertwin topboost15 britrack1 tiny15 vier100 tri100 | f063a68 | pedido 04:26 |
| 2 | recto_dual recto_triple sl100 block5150 block5150iii britplexi britjubilee rocker50 be100 power100 | e316e2d | pedido 04:35 |
| 3 | uber100 sonic60 tweed59 britbass britplexi50 brit45 brit900 brit410 brit2000 britds20 | 5f6be9d | pedido |
| 4 | mark2plus mark3 mark4 markjp badland100 lonestar100 citrus50 citrus30 topboost30 match30 | 8fcc4a4 | pedido |
| 5 | blackcountry100 iron120 fire100 feral120 steel20 herb180 bliss100 goddess80 dirty40 special100 | 372af61 | pedido |
| 6 | doublestone50 blackcat30 hi103 darkstar20 gen120 nova100 emperor20 rg100 arch100 kraken50 magick25 deliver120 | 0852437 | pedido |
| 7 | silverdeluxe silversuper silverprince tweed57 champion5 hotrod40 jazz120 bebop15 vault210 fliptop15 fliptop18 (+ tweed59 de novo) | dfa2284 | pedido |
| 8 | vault300 vaultv4 citrusbass200 granite800 hart1000 garden800 glass500 solart bassline100 eagle751 littletube vault4pro | 17e1504 | pedido |
| 9 | workhorse350 underground800 acoustica360 tinybass tracer7 ash500 maple400 nord650 wind300 bassboxv eaglehammer500 (+ britrack1 de novo) | 984ca22 | pedido |

## Decisões para o Diego

Regra usada quando o original não tem um botão que o Nexus tem: primeiro o botão
do original de função mais próxima (com o rótulo do original); se não houver, um
botão pequeno com o rótulo em português, no canto mais discreto. LEDs de canal que
o original não tem ficam pequenos, perto da chave de canal. Rótulos novos (MASTER,
PRE-AMP, TREBLE...) aparecem como estão em todos os idiomas: para traduzir, entram
na tabela de idiomas (fora desta frente).

Por cabeçote (`notas_modelagem` de cada receita):

- acoustica360: O 360 não tem médio nem presença: o médio ficou no VARIAMP (balanço) e a presença num botão pequeno; o ganho é o FUZZ GAIN.
- arch100: Medidas de loja (o site da PRS bloqueou). O ganho do Nexus é o VOLUME do canal Lead (no Archon é o ganho do canal).
- ash500: Os quatro deslizantes do EQ ficaram desenhados fixos. A presença do Nexus é o COMPRESSION.
- badland100: O Badlander tem LED só nos canais 1 e 2; o terceiro LED é para a tela (Limpo, Crunch, Crush).
- bassboxv: A presença do Nexus ficou no REVERB/ATTACK; o MASTER do original ficou fixo e o volume do Nexus é o VOLUME.
- bassline100: O Bassman não tem médio, presença nem master: o volume do Nexus é o VOLUME do canal Normal; médio e presença em botões pequenos.
- bebop15: Medidas de anúncio (sem fonte do fabricante). A presença do Nexus é o CONTOUR.
- bliss100: Confiança baixa nas medidas (largura estimada) e na posição exata das minichaves do Ecstasy.
- block5150iii: A grade do original tem as listras EVH em barras por cima; aqui ficou só a grade perfurada preta.
- brit410: As teclas do JVM acendem na cor do modo; aqui três LEDs entre elas mostram o canal ativo na tela (Limpo, Crunch, Overdrive).
- brit45: O JTM45 não tem master: o ganho do Nexus é o LOUDNESS 1 (HIGH TREBLE) e o volume o LOUDNESS 2 (NORMAL).
- brit900: Os LEDs de canal A e B entre o GAIN e a entrada são para a tela; no JCM900 a troca é pelo pedal.
- britbass: O Super Bass não tem master: o ganho do Nexus é o VOLUME 1 e o volume o VOLUME 2.
- britds20: O DSL20 tem dois LEDs na tecla CHANNEL; o terceiro, embaixo, é para a tela mostrar os três canais.
- britplexi: O 1959 não tem master: o ganho do Nexus é o VOLUME I e o volume é o VOLUME II.
- britplexi50: O 1987 não tem master: o ganho do Nexus é o LOUDNESS 1 (HIGH TREBLE) e o volume o LOUDNESS 2 (NORMAL).
- britrack1: Os botões que tocam ficam no lugar das teclas de cima do JMP-1 (que no original escolhem o parâmetro do botão DATA). Os LEDs de canal são as teclas CLEAN 1 (Limpo) e OD 1 (Overdrive).
- champion5: O Champ só tem VOLUME: agudo, médio, grave, presença e o volume de saída do Nexus ficaram em cinco botões pequenos.
- citrus30: O AD30 não tem presença: ficou num botão pequeno à direita, perto da entrada. Os LEDs de canal não existem no original.
- citrus50: A presença do Nexus ficou no HF DRIVE do OR50 (que realça os agudos do ganho).
- citrusbass200: O AD200 não tem presença: ficou num botão pequeno entre o GAIN e as entradas.
- darkstar20: Feito como o HT-20RH MkIII (decisão do PROGRESSO). A presença do Nexus ficou no ISF.
- doublestone50: A presença do Nexus ficou no CONTOUR (o Two-Rock não tem PRESENCE na frente).
- eagle751: O DB 751 não tem presença: ficou num botão pequeno depois do MASTER.
- eaglehammer500: O Tone Hammer não tem presença: ficou num botão pequeno ao lado do TREBLE. O DRIVE do original ficou fixo (o ganho do Nexus é o GAIN).
- emperor20: O Imperial não tem presença: a presença do Nexus é o TREBLE do canal Rhythm; o grave é o BASS do Rhythm; o ganho, o agudo e o médio são VOLUME, TONE e MID-BITE do Lead.
- fliptop15: Medidas: o chassi (405 × 250 × 300 mm nas fontes) em cima de uma caixa 1×15 estimada; total 560 × 860 × 380. O volume do Nexus é o VOLUME do canal TWO e a presença o TREBLE dele; o médio ficou num botão pequeno.
- fliptop18: Medidas do B-18 não encontradas: estimadas a partir do B-15 com a caixa 1×18 maior (600 × 940 × 400). O volume do Nexus é o VOLUME do canal 2 e a presença o TREBLE dele; o médio ficou num botão pequeno.
- garden800: Profundidade do WT-800 não encontrada (usei 330 mm).
- glass500: O EQ de seis deslizantes ficou desenhado fixo; grave e médio do Nexus são dois botões pequenos ao lado do BLEND, o agudo é o TONE e a presença o BLEND.
- granite800: A presença do Nexus ficou no BOOST do 800RB; o volume no MASTER de 100 W.
- hart1000: O LH1000 não tem master nem presença: o volume do Nexus ficou no BALANCE e a presença num botão pequeno.
- iron120: O Ironheart não tem presença no painel: a presença do Nexus ficou no TONE.
- jazz120: O JC-120 não tem presença: a presença do Nexus é o TREBLE do canal 1.
- littletube: Painel bronze (decisão do PROGRESSO). A presença do Nexus ficou no MID HIGH.
- magick25: O Black Magick só tem VOL 1, VOL 2 e TONE: grave, médio e presença do Nexus ficaram em botões pequenos entre o DEPTH e as chaves. A tela em flecha do Supro ficou retangular.
- maple400: A presença do Nexus ficou no RESONANCE.
- mark2plus: O PRESENCE do IIC+ fica atrás: aqui é um botão pequeno ao lado do LEAD MASTER. Os LEDs de canal não existem no original.
- mark3: O PRESENCE do Mark III fica atrás: aqui é um botão pequeno depois do REVERB. Os três LEDs de canal são para a tela.
- markjp: O segundo EQ gráfico fica desenhado fixo (o Nexus tem um EQ só).
- match30: O TONE do DC-30 é chave de seis posições; aqui é o médio do Nexus, girando contínuo. A presença é o CUT.
- nord650: A presença do Nexus ficou no HIGH MID.
- nova100: Medidas estimadas pela foto (a Fortin não publica).
- rg100: Medidas de loja (Randall não publica).
- rocker50: O Rockerverb não tem presença: ficou num botão pequeno no bloco preto das chaves, à esquerda.
- silverdeluxe: O Deluxe Reverb não tem médio, presença nem master: a presença ficou no INTENSITY e médio e volume em dois botões pequenos depois dele.
- silverprince: O Princeton Reverb não tem médio, presença nem master: a presença ficou no INTENSITY e médio e volume em dois botões pequenos.
- silversuper: O Super Reverb não tem presença nem master: a presença ficou no INTENSITY e o volume de saída no VOLUME do canal Normal.
- silvertwin: O Twin não tem presença nem master: a presença ficou no INTENSITY (vibrato) e o volume de saída no VOLUME do canal Vibrato; o ganho é o VOLUME do canal Normal.
- sl100: O SLO-100 não tem LED de canal: os dois LEDs sobre as minichaves são para a tela mostrar Normal e Overdrive.
- sonic60: O Super-Sonic não tem presença: a presença do Nexus ficou no REVERB (o volume é o VOLUME do canal Burn).
- special100: Dumble não publica medidas: tamanho estimado pela foto. Reverb e tremolo não existem em todas as unidades.
- tiny15: O Tiny Terror só tem VOLUME, TONE e GAIN: o TONE é o agudo; grave, médio e presença do Nexus ficaram em três botões pequenos no canto de cima à direita (no lugar do brasão).
- tinybass: O Terror Bass não tem presença: ficou num botão pequeno à direita do nome.
- topboost15: O AC15 não tem botão de médio: o médio do Nexus ficou no VOLUME do canal Normal; a presença é o TONE CUT (no original corta agudo no power). Os dois LEDs de canal sobre as entradas não existem no original: são para a tela mostrar o canal ativo.
- topboost30: O AC30 não tem médio: o médio do Nexus ficou no NORMAL VOLUME; a presença é o TONE CUT.
- tracer7: O EQ de sete deslizantes ficou desenhado fixo; grave, médio e agudo do Nexus ficaram em três botões pequenos acima dele. A presença é o COMPRESSOR.
- tri100: O TriAmp não tem presença no canal: a presença do Nexus é o PRESENCE do MASTER (que existe no original).
- tweed57: O 5E3 só tem TONE e dois VOLUMEs: grave, médio e presença do Nexus ficaram em três botões pequenos no painel.
- tweed59: Bassman sem master: o ganho é o VOLUME BRIGHT e o volume o VOLUME NORMAL.
- underground800: A presença do Nexus ficou no HI MID.
- vault210: O BA-210 não tem presença: ficou num botão pequeno depois do AUX LEVEL.
- vault300: Feito como o SVT antigo (decisão do PROGRESSO: painel prata e letras azuis), com os botões do SVT atual. O SVT não tem presença: ficou num botão pequeno depois do MASTER.
- vault4pro: O EQ gráfico de nove faixas ficou desenhado fixo. O SVT-4 Pro não tem presença: ficou num botão pequeno depois do MASTER.
- vaultv4: O V-4B não tem presença: ficou num botão pequeno depois do MASTER.
- wind300: Confiança baixa: o Mark III de baixo não tem medidas nem painel confirmados do fabricante; feito pela foto de época. A presença do Nexus ficou no COMPRESSOR.
- workhorse350: Feito como o Bass 350 de 1994–99 (seções bordô).

