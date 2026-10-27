# Nexus Modeler — famílias de modelos 3D e ferramenta de criação

Estado: proposta para aprovação (26/10/2026).

## 1. A ideia: uma família visual, vários amplificadores

Não precisamos de um modelo 3D para cada amplificador do mundo. Precisamos de
um modelo 3D **por família visual**: amplificadores que, olhando de frente,
parecem o mesmo equipamento.

- Dual Rectifier, Triple Rectifier e Roadster → **uma** família ("Rectifier").
- Mark IIC+, Mark IV, Mark V e JP-2C → **outra** família ("Mark"), embora
  também seja Mesa/Boogie: frente diferente, equalizador gráfico de barrinhas.

Cada família tem variações baratas que não exigem modelo novo: cor do couro,
cor do painel, número de botões, nome na placa. Ex.: JCM800 e JCM900 são a
mesma família; o Silver Jubilee é a mesma família com painel prateado.

Todos os simulacros têm **nome próprio e nenhum logo de marca** (ver
ARQUITETURA.md, seção 3.3). O nome sugerido abaixo é só ponto de partida.

## 2. Catálogo de famílias de amplificador

Critério: o que é mais capturado e baixado hoje na TONE3000 (os tons em alta
incluem Fender Super Reverb, Marshall JCM800, Tone King Imperial, Two-Rock,
Randall Satan, EVH 5150III, 6505+, Marshall JVM) mais os clássicos que todo
simulador profissional tem (AmpliTube, ToneX, Neural DSP, Fractal).

Prioridade: **1** = primeira leva (inclui todos os que você citou),
**2** = segunda leva, **3** = depois.

### Britânicos
| # | Família (simulacro) | Cobre | Visual que define a família | Prior. |
|---|---|---|---|---|
| 1 | **Brit Plexi** | Marshall 1959 Super Lead, 1987X, JTM45, JMP | Cabeçote alto, painel dourado de acrílico, tecido quadriculado bege, botões pretos com tampa dourada | 1 |
| 2 | **Brit 800** | Marshall JCM800 2203/2204, JCM900, 2555 Silver Jubilee (variação prata) | Painel dourado escovado, couro preto, friso branco, logo em script | 1 |
| 3 | **Brit Modern** | Marshall JVM410H, DSL100, JCM2000 | Mais botões, chaves e luzes por canal | 2 |
| 4 | **Class A 30** | Vox AC30, AC15 | Combo com grade em losango, painel cobre/dourado em cima, couro bege ou preto | 1 |
| 5 | **Orange Box** | Orange Rockerverb, OR50/120, Thunderverb, Dual Terror | Couro laranja, painel branco com desenhos no lugar de palavras | 2 |
| 6 | **Hi-Power** | Hiwatt DR103 | Painel cinza fosco, letras brancas, botões grandes | 3 |

### Americanos limpos e vintage
| # | Família | Cobre | Visual | Prior. |
|---|---|---|---|---|
| 7 | **Blackface** | Fender Twin Reverb, Deluxe Reverb, Super Reverb, Princeton | Painel preto, botões com saia numerada, tela prateada; Silverface como variação | 1 |
| 8 | **Tweed** | Fender Bassman 5F6A, Deluxe 5E3, Champ | Tecido tweed envernizado, tela marrom, painel cromado em cima | 1 |
| 9 | **Boutique Clean** | Two-Rock, Dumble Overdrive Special e clones | Cabeçote baixo e largo, couro preto ou cores sóbrias, painel metálico | 2 |
| 10 | **Vintage Combo Luxo** | Tone King Imperial, Fender Vibrolux | Combo com duas cores e grade clara | 3 |
| 11 | **Jazz Chorus** | Roland JC-120 | Transistorizado, grade preta, painel prateado com chaves | 2 |
| 12 | **Class A Boutique** | Matchless DC30, Dr. Z, Bad Cat | Grade com faixas, logo iluminado, botões tipo frango | 3 |

### Mesa/Boogie
| # | Família | Cobre | Visual | Prior. |
|---|---|---|---|---|
| 13 | **Rectifier** | Dual Rectifier, Triple Rectifier, Roadster, Road King | Chapa de alumínio xadrez na frente, couro preto, botões cromados | 1 |
| 14 | **Mark** | Mark IIC+, Mark IV, Mark V, JP-2C | Grade de vime ou couro, **equalizador gráfico de 5 barrinhas**, muitos puxadores | 1 |
| 15 | **Lone Star** | Lone Star, Express, Fillmore | Visual mais vintage da Mesa | 3 |

### Alto ganho clássico
| # | Família | Cobre | Visual | Prior. |
|---|---|---|---|---|
| 16 | **5150 Classic** | Peavey 5150, 5150 II, 6505, 6505+ | Painel preto e prata, letras em bloco, couro preto, cantos de metal | 1 |
| 17 | **5150 III** | EVH 5150III (50 W, 100 W, Stealth, Ivory) | Mais arredondado, faixas, variação marfim | 1 |
| 18 | **SLO** | Soldano SLO-100, SLO-30, Hot Rod 50 | **Frente inteira cromada espelhada**, botões pretos | 1 |
| 19 | **Jet** | Jet City JCA100H, JCA50, JCA22 (projeto do Soldano) | Couro preto, painel com faixas vermelhas | 1 |
| 20 | **Friedman** | BE-100, Dirty Shirley, JJ Junior | Derivado do Plexi, painel preto/prata, couro preto | 2 |
| 21 | **Bogner** | Ecstasy, Uberschall, Shiva | Frente de madeira ou couro bege, painel com muitos botões | 2 |
| 22 | **Splawn** | Nitro, Quickrod | Hot rod britânico, couro colorido | 3 |

### Alto ganho moderno
| # | Família | Cobre | Visual | Prior. |
|---|---|---|---|---|
| 23 | **Diezel** | VH4, Herbert, Hagen | Painel azul, muitos botões em fila dupla | 2 |
| 24 | **ENGL** | Powerball, Fireball, Savage, Invader | Couro preto, painel prateado com detalhes vermelhos | 2 |
| 25 | **Satan** | Randall Satan (Ola Englund) | Preto agressivo, logo grande iluminado | 2 |
| 26 | **Fortin** | Fortin Natas, Meshuggah, Cali | Preto minimalista, metal escovado | 3 |
| 27 | **Revv** | Revv Generator 120, D20 | Moderno, luzes coloridas por canal | 3 |
| 28 | **Victory** | Victory Kraken, VX, Sheriff | Britânico moderno, botões grandes | 3 |
| 29 | **PRS** | PRS Archon, MT15 (Mark Tremonti) | Moderno, logo pássaro trocado por símbolo próprio | 3 |

### Baixo (pedido futuro)
| # | Família | Cobre | Prior. |
|---|---|---|---|
| 30 | **Bass Tube** | Ampeg SVT, B-15 | 3 |
| 31 | **Bass Modern** | Darkglass Microtubes, Aguilar | 3 |

**Primeira leva (prioridade 1): 13 famílias** — Brit Plexi, Brit 800, Class
A 30, Blackface, Tweed, Rectifier, Mark, 5150 Classic, 5150 III, SLO, Jet e,
para fechar os estilos de uso diário, as duas famílias de caixa e o pedal
verde abaixo.

Quando o usuário importa um tom da TONE3000, o Nexus escolhe a família pela
marca e modelo do tom (ex.: "Mesa Boogie" + "Mark IV" → Mark). Se não
reconhecer, usa um cabeçote genérico.

## 3. Equalização de cada família

Todo amplificador do Nexus tem equalizador, e o equalizador de cada família
imita o **circuito de tonalidade do amplificador real** (a "pilha de
tonalidade": os botões de grave, médio e agudo e as peças eletrônicas por trás
deles). Por isso o médio de um Marshall não responde igual ao de um Fender,
e um Mark tem as 5 barrinhas que o original tem.

### Onde fica
O equalizador é **o próprio painel do amplificador 3D**: os botões de grave,
médio, agudo, presença (e as barrinhas do Mark, o corte do Vox, o botão único
do Tweed Deluxe) são desenhados no modelo, no mesmo lugar e com o mesmo estilo
do amplificador real, e girar esses botões muda o som. Não existe um bloco de
equalizador separado nem um equalizador genérico igual para todos: cada
amplificador tem exatamente os controles de tonalidade que o original tem.

### Como funciona
- A captura NAM é uma fotografia com o equalizador numa posição fixa
  (normalmente tudo no meio, 5). O Nexus calcula a resposta do circuito
  original nessa posição e na posição escolhida pelo usuário, e aplica só a
  **diferença** entre as duas. Com os botões na posição da captura, o som é
  exatamente o da captura.
- A posição em que a captura foi feita vem da descrição do pacote; se o
  criador não informou, vale tudo no 5, e o usuário pode corrigir na tela de
  mapa.
- **Presença** e **profundidade/ressonância** imitam o ajuste do estágio de
  potência (realce de agudos e graves no final), também por família.
- Limite honesto: no amplificador real o equalizador fica **entre** os
  estágios de distorção, então mexer nele também muda um pouco a distorção. O
  Nexus aplica depois da captura, como o plugin do NAM e a maioria dos
  simuladores baseados em captura. Se o pacote tiver capturas com o
  equalizador em posições diferentes, o motor de troca usa essas capturas no
  lugar da aproximação.

### Tipo de circuito por família
| Circuito | Famílias | Botões |
|---|---|---|
| Fender/Marshall clássico, valores Marshall | Brit Plexi, Brit 800, Brit Modern, Friedman, Splawn, Victory | grave, médio, agudo, presença |
| Fender/Marshall clássico, valores Fender | Blackface, Vintage Combo Luxo, Tweed Bassman | grave, (médio), agudo, (presença) |
| Um botão de tom só | Tweed Deluxe/Champ | tom |
| Vox "Top Boost" com corte | Class A 30 | agudo, grave, corte |
| Fender/Marshall, valores de alto ganho | Rectifier, 5150 Classic, 5150 III, SLO, Jet, Bogner, ENGL, Diezel, Satan, Fortin, Revv, PRS | grave, médio, agudo, presença, ressonância/profundidade |
| Fender/Marshall + **gráfico de 5 faixas** (80, 240, 750, 2200, 6600 Hz) | Mark | grave, médio, agudo, presença + 5 barrinhas |
| Orange | Orange Box | grave, médio, agudo |
| Hiwatt | Hi-Power | grave, agudo, presença (médio no circuito do Hiwatt) |
| Boutique (D-style) | Boutique Clean | grave, médio, agudo, presença |
| Transistorizado ativo | Jazz Chorus | grave, médio, agudo |

Os valores das peças de cada família ficam na receita (seção 5.2), junto com
o visual, para o botão desenhado e o som andarem juntos.

## 4. Caixas e pedais (mesma lógica de famílias)

**Caixas:** 4×12 britânica (inclinada e reta, tela quadriculada), 4×12
Rectifier (chapa xadrez), 4×12 laranja, 2×12 fechada, 2×12 aberta, 1×12
combo, 4×10 tweed, 2×12 Vox.

**Pedais:** verde de overdrive (estilo Tube Screamer), dourado de boost
(estilo Klon), compacto padrão (estilo Boss, várias cores), preto de
distorção (estilo RAT), grande de fuzz (estilo Big Muff), redondo de fuzz
(estilo Fuzz Face), pequeno de uma cor (estilo MXR), boutique de alumínio.

## 5. A ferramenta: Nexus Gear Studio

Fazer 30 amplificadores, 8 caixas e 8 pedais à mão seria lento e
inconsistente. A **ferramenta gera os modelos a partir de uma receita**.

**Já funciona** (`tools/gear_studio`): famílias "cabeçote", "caixa" e
"pedal", com o Brit 800, a caixa Brit 4x12 e três pedais prontos:

| Pedal | Inspirado em | Formato |
|---|---|---|
| Lemon Drive | overdrive verde clássico de três botões (estilo Tube Screamer) | `classico`: tampa grande de pisar |
| Laranja Dist | distorção compacta laranja (estilo DS-1) | `compacto`: pedal basculante com borracha frisada |
| Ouro | overdrive dourado de luxo (estilo Klon Centaur) | `padrao`: pisante de metal |

Nomes próprios, sem logos de marca. Pedal novo é uma receita nova em
`tools/gear_studio/recipes/`. Para gerar:

```bash
blender -b --factory-startup -P tools/gear_studio/build.py -- --receita tools/gear_studio/recipes/brit800.json --saida build/gear/brit800
```

- `--previa` faz meia resolução e 11 posições por botão, para ajustar rápido.
- A saída (`frente.png`, `botao_<id>.png`, `mapa.json`) é copiada para
  `assets/gear/` com o nome da família na frente (`brit800_frente.png`…) e
  entra no plugin na compilação.
- Sem placa de vídeo compatível, o Blender usa o processador: no Mac Intel de
  8 núcleos, a frente leva cerca de 7 minutos e as 101 posições dos botões,
  cerca de 40.

### 5.1 Como funciona
```
receita da família (arquivo de texto)
        │
        ▼
Gerador no Blender (scripts Python)  ←  biblioteca de peças e materiais
        │                                (botões, chaves, luzes, couro,
        ▼                                 telas, chapas, cantos, alças)
modelo 3D completo
        │
        ▼
Estúdio de render (luz, câmera, sombras sempre iguais)
        │
        ▼
pacote visual pronto para o plugin
  frente.png, botões (101 imagens cada, de 0,0 a 10,0),
  chaves (ligada/desligada), luzes (acesa/apagada) e um arquivo com a
  posição exata de cada controle na imagem
```

### 5.2 A receita
Cada família é descrita por um arquivo de texto curto. A receita real do Brit
800 está em `tools/gear_studio/recipes/brit800.json`. Exemplo (planejado) do
Mark:
```json
{
  "familia": "mark",
  "nome": "Mark Series",
  "formato": "cabecote",
  "medidas_cm": { "largura": 49, "altura": 24, "profundidade": 27 },
  "couro": { "tipo": "tolex_fino", "cor": "#111111" },
  "frente": { "tipo": "vime", "cor": "#c9b48a" },
  "painel": { "material": "preto_fosco", "letras": "branco" },
  "linhas_de_controles": [
    { "tipo": "botao", "estilo": "mesa_cromado", "rotulos": ["Volume","Treble","Bass","Middle","Master","Lead Drive","Lead Master","Presence"] },
    { "tipo": "eq_grafico", "faixas": ["80","240","750","2200","6600"] },
    { "tipo": "chave", "rotulos": ["Rhythm","Lead"] }
  ],
  "equalizacao": {
    "circuito": "fmv_alto_ganho",
    "pecas": { "R1": "250k", "R2": "1M", "R3": "25k", "R4": "47k", "C1": "470p", "C2": "22n", "C3": "22n" },
    "grafico_5_faixas": true,
    "presenca": true,
    "posicao_da_captura": { "grave": 5, "medio": 5, "agudo": 5, "presenca": 5 }
  },
  "luz_piloto": "verde",
  "plaqueta": "MARK"
}
```
Trocar couro, cor ou número de botões é editar a receita e gerar de novo.
Uma família nova é uma receita nova, reaproveitando as peças.

### 5.3 Biblioteca de peças (o que dá o realismo)
- **Botões:** tampa dourada (Plexi/800), saia numerada (Fender), cromado (Mesa),
  frango (boutique), grande liso (Hiwatt), moderno com aro.
- **Materiais:** couro com textura de relevo, tweed envernizado, tela
  quadriculada, tela em losango (Vox), vime, chapa de alumínio xadrez,
  cromo espelhado, metal escovado, acrílico dourado, plástico.
- **Detalhes:** parafusos, cantos de metal, alças, friso, pés de borracha,
  luz piloto de vidro, entradas de cabo, desgaste leve nas bordas.

Todos os materiais são **procedurais** (gerados por fórmula no Blender), sem
fotografias de terceiros, então não há problema de direitos.

### 5.4 Ajuste visual
Cada geração salva também `cena.blend`: abrindo no Blender, dá para girar,
aproximar e conferir antes do render final. (Um painel próprio dentro do
Blender, com os campos da receita, fica para depois.)

### 5.5 Referência de qualidade
O ToneX 2 e o AmpliTube 5 são a **régua de qualidade**: enquadramento,
iluminação de estúdio, nitidez, peso dos materiais, brilho dos metais. As
imagens deles servem para comparar lado a lado durante o ajuste, mas **não
são copiadas nem usadas** no Nexus (são da IK Multimedia).

### 5.6 Onde renderizar
101 imagens por botão × ~10 botões × 30 famílias é muito trabalho de render.
Vai para a placa de vídeo do lab2 (Cycles com CUDA). O Blender 4.5 não usa a
placa AMD do Mac Intel, então no Mac o render é no processador.

## 6. Ordem sugerida
1. Biblioteca de peças e estúdio de render.
2. **Brit 800** como primeira família, iterando até o nível do ToneX/AmpliTube.
3. Mark e Rectifier (provam que a ferramenta resolve frentes bem diferentes).
4. Resto da prioridade 1, depois caixas e pedais.
5. Prioridades 2 e 3.

## Fontes
- Tons em alta e catálogo: https://www.tone3000.com/ e https://www.tone3000.com/plugin
- Pacotes populares de alto ganho: https://www.tone3000.com/ampspedalspickups
- Lançamento do plugin TONE3000: https://guitar.com/news/tone-3000-plugin-neural-amp-modeler/
