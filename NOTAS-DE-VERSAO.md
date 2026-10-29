# Notas de versão

## 0.2.0-beta.1 (em preparação)

### Equipamentos novos (nomes ainda provisórios, aguardando aprovação)
- 31 cabeçotes novos, cada um com a sua caixa. Guitarra: Top Boost 30 (AC30),
  Match 30 (Matchless DC-30), Tiny 15 (Tiny Terror), Rocker 50 (Rockerverb),
  Recto Triple, Power 100 (ENGL Powerball), Uber 100 (Uberschall), Sonic 60
  (Super-Sonic), Silver Twin (Twin Reverb), Tweed 59 (Bassman), Brit Plexi
  (Super Lead 1959), Brit Jubilee (Silver Jubilee) e Jazz 120 (JC-120).
- 18 cabeçotes de baixo: Vault 300 (Ampeg SVT), Vault V4 (Ampeg V-4B),
  Fliptop 15 (Ampeg B-15), Citrus Bass 200 (Orange AD200), Tiny Bass (Orange
  Terror Bass), Granite 800 (Gallien-Krueger 800RB), Hart 1000 (Hartke
  LH1000), Garden 800 (Eden), Glass 500 (Darkglass Microtubes 500), Solar T
  (Sunn Model T), Bassline 100 (Fender Bassman 100T), Eagle 751 (Aguilar
  DB751), Little Tube (Markbass Little Mark Tube), Workhorse 350 (SWR),
  Underground 800 (Mesa Bass 800D), Acoustica 360 (Acoustic 360), Tracer 7
  (Trace Elliot GP7) e Brit Bass (Marshall Super Bass).
- 10 caixas novas: Citrus 4x12, Blue 2x12, Twin 2x12, Tweed 4x10, Match 2x12,
  Power 4x12, Uber 4x12, Jazz 2x12, Vault 8x10 e Granite 4x10. A lista das
  caixas agora é agrupada pelo tamanho (4x12, 2x12, 4x10, 8x10).
- Pedais de modulação feitos pelo próprio Nexus: Tangerine Phaser (Phase 90),
  Dragonfruit Flanger (Electric Mistress), Kiwi Tremolo (TR-2) e Blackberry Vibe
  (Uni-Vibe).
- Pedais de ambiente de vários modos, feitos pelo próprio Nexus, com visor
  que mostra o modo:
  - Juniper Sky (no espírito do BigSky): hall, sala, placa, mola, nuvem,
    swell e cinco shimmers (oitava acima, quinta + oitava, oitava abaixo, só
    na cauda e duplo). Botões: decaimento, pré-atraso, modulação, tom,
    shimmer e mistura.
  - Lime Echo (no espírito do DL4): digital, analógico, fita, multi-cabeça,
    ping-pong, reverso, modulado, lo-fi, slapback, swell e dueto. Segue o tap
    tempo e as divisões.
  - Plum Mod (no espírito do MM4): rotary, tremolo harmônico, vibrato, filtro
    de envelope, wah automático, modulador em anel, chorus triplo, tremolo
    picotado, phaser de 12 estágios, flanger por zero e detune. Segue o tap
    tempo.
- Blueberry Chain: delays digitais em cadeia (um alimenta o outro). Botões
  de tempo, feedback e repetição; chave 2 | 3 escolhe dois ou três delays e
  chave x | / faz cada delay repetir o dobro ou a metade de vezes do anterior
  (4, 8 e 16 repetições ou 4, 2 e 1). Todos no mesmo tempo; segue o tap tempo.
- Pedais com captura da TONE3000: Açaí Rat (RAT), Jabuticaba Buzzsaw (HM-2),
  Lychee Fuzz (Big Muff), Avocado Metal (MT-2), Pomegranate Mud (Mudhoney) e
  Snow Pear Treble (Rangemaster).
- Cores e detalhes de cada peça seguem o equipamento original; o nome do
  pedal é uma fruta da mesma cor. Em frente clara, a marca tem letras pretas.

### Captura padrão
- Ao escolher uma peça, o Nexus baixa sozinho a captura mais curtida pela
  comunidade da TONE3000. "Trocar pacote" abre a lista ("Mais curtidas" é a
  primeira aba) para escolher outra.

### Atualização
- Ícone próprio no topo (nuvem com seta para baixo) quando há versão nova.
  Toda vez que o Nexus abre, ele procura e pergunta se a pessoa quer
  atualizar; "Agora não" deixa o ícone lá para depois.
- No Mac, a atualização troca o programa e os plugins no lugar onde já estão,
  sem abrir instalador. Se estiverem nas pastas do sistema, o Mac pede a
  senha uma vez, na janela dele.
- No Windows, a atualização instala sem perguntas e abre o Nexus de novo
  sozinha. Dentro do programa de gravação, o instalador abre normal, para a
  pessoa salvar o projeto antes.
- No Linux (instalação pelo terminal), "Reabrir agora" fecha e abre o Nexus
  já na versão nova.
- Os instaladores do Mac e do Windows trazem o número completo da versão
  (com -beta.N), para as versões seguintes aparecerem.
- Notas de versão na tela: o aviso de versão nova mostra o que entra nela,
  e na primeira abertura depois de atualizar o Nexus mostra as novidades da
  versão. Para ler de novo: menu "i", "Novidades desta versão".

### Afinador
- Afinador de rack fixo no começo da cadeia: nota, oitava, cents e
  frequência, fileira de luzes (verde no meio = afinada), lá de referência de
  430 a 450 Hz e "mudo ao afinar".

### Phaser
- Tangerine Phaser refeito no molde do circuito do MXR Phase 90: quatro
  estágios varridos por um oscilador triangular, som limpo e defasado meio a
  meio. Velocidade de 0,1 a 10 Hz (o botão mostra em Hz), profundidade é a
  largura da varredura, intensidade é a realimentação do Phase 90 de logotipo
  em bloco (zerada, é o "script").

### Pedaleira (MIDI)
- Aprender uma pisada: a pedaleira que manda o banco junto com a troca de
  patch ligava qualquer pisada ao pedal. Agora fica só a pisada aprendida, e
  o registro anota o que foi aprendido.

### Pedaleira na tela

- Ao escolher um pedal, o palco mostra a pedaleira inteira (modelada em 3D no
  Blender): todos os pedais da cadeia, na ordem do som, ligados por cabos.
  O LED aceso mostra quem está ligado; pisar no pisante de qualquer pedal liga
  e desliga, e clicar no pedal escolhe ele para mexer nos botões.
- Os pedais ficam numa fileira só enquanto cabem bem; quando apertar a ponto
  de ficarem menores do que em duas, abre a fileira da frente (a de trás com
  o começo do som, a da frente com o resto).
- Cada pedal aparece no tamanho de verdade em relação aos outros: um pedal
  pequeno fica menor que um comum, e o Lime Echo ficou mais compacto
  (15 x 10,5 cm).
- Cabeçote ou caixa: a pedaleira some e aparece só a peça.
- A cadeia de baixo mostra sempre os mesmos pedais, na mesma ordem.

### Microfones na frente da caixa

- Com IR próprio, a caixa mostra o microfone com que o IR foi gravado, em 3D,
  na frente do falante e na posição da gravação (centro, borda da calota,
  cone, borda ou a distância em polegadas), com uma etiqueta do modelo e do
  tipo. Um IR que mistura dois microfones mostra os dois, lado a lado.
- O arquivo de IR não guarda o microfone por dentro: o Nexus lê o nome do
  arquivo (ex.: "Engl_MD421_Cone", "UL 4x12 M201 1.25in"). Quando o nome não
  diz, a caixa aparece sozinha, como antes.
- Cinco modelos de microfone, sem marca: dinâmico de bastão (estilo SM57),
  dinâmico lateral (estilo MD421), condensador grande (estilo U87),
  condensador de lápis (estilo KM184) e fita (estilo R121).
- Os nomes dos tipos de microfone ainda aparecem só em português.

### Primeira abertura
- A cadeia de sinal começa só com o Brit 800 e a caixa dele, sem pedais. Nas
  vezes seguintes, abre como a pessoa deixou.

### Cherry Drop
- Visual novo: visor azul sempre aceso mostrando os semitons, estampa de
  ondas e aro cromado. O LED mostra se o pedal está ligado.
- Botão do drop com o meio-dia sem mudar o tom: para a esquerda desce até
  −12 semitons, para a direita sobe até +12. O segundo botão é o brilho, para
  o som não ficar opaco quando desce.

### Menor
- Imagens dos botões em 256 cores: a arte dos equipamentos caiu de 284 MB
  para cerca de 100 MB, mesmo com o dobro de peças.

## 0.1.0-beta.1


### Tela
- Tela nova com identidade própria: marca do Nexus (triângulo impossível
  azul), cor de destaque ciano, palco com foco de luz, nome grande do
  equipamento com o pacote e o trilho das capturas, e o caminho do sinal no
  rodapé (entrada, pedal, amplificador, caixa, saída).
- Amplificador Brit 800 e caixa Brit 4x12 modelados e renderizados em 3D.
  Cada botão tem 101 imagens: arrastar, Shift para ajuste fino, duplo clique
  volta ao padrão, roda do mouse.
- Controles deslizantes de entrada e saída com medidor de pico.
- Coluna de equipamentos à direita (Pedais, Cabeçotes, Caixas): cada peça com
  a imagem 3D, o clássico que inspirou e de onde vem o som. Clique troca a
  peça escolhida; arrastar para a cadeia acrescenta onde soltar. Os
  equipamentos planejados aparecem em "Em breve" (lista em docs/CATALOGO.md).
- Presets na barra de cima: o som inteiro (cadeia, pedais, cabeçotes,
  botões, capturas, IRs e MIDI) num arquivo; setas para o anterior e o
  próximo, salvar, salvar como, renomear e apagar (vai para a lixeira).
- Uso do processador de cada pedal e cabeçote embaixo de cada peça da cadeia.
- Ícone do programa com o triângulo do logo.

### Cadeia de sinal
- Até 12 pedais, montados por clique e arrasta: antes do cabeçote, entre o
  cabeçote e a caixa, ou depois da caixa. "+" adiciona; botão direito move,
  remove e liga ao MIDI.
- Dois cabeçotes com pedal A/B no fim dos pedais: cada lado tem cabeçote,
  pedais próprios e caixa. A troca de lado aquece o outro e cruza em 30 ms,
  sem estalo.
- A ordem da cadeia é salva no projeto.

### Capturas da TONE3000
- "Abrir pacote" abre a janela das capturas daquele equipamento, dentro do
  Nexus e com a TONE3000 como fonte: recomendadas (Brit 800, Lemon Drive,
  DS-1 laranja e Klon dourado), favoritos e downloads da conta filtrados pelo
  equipamento, em alta, catálogo completo e "inserir manualmente".
- O Nexus baixa só as capturas A2 da série certa, uma por posição do botão,
  e monta o pacote sozinho.
- Com a conta conectada, "Carregar" baixa direto, sem abrir o site. Os
  pacotes ficam em Documentos/Nexus Modeler/<equipamento>/<pacote>.
- "Inserir manualmente" aceita pasta, `.zip` ou capturas `.nam` soltas: copia
  ou descompacta para a pasta do equipamento.
- Mapeamento: quando o Nexus não acha a posição do botão no nome das
  capturas (ou duas caem na mesma posição), abre a tela "Mapear as capturas"
  e grava o resultado no `pack.json`.

### Testar o som
- Barra "Testar o som" em cima da entrada: toca uma gravação de guitarra
  direta pela cadeia, sem tocar, em quatro estilos (Limpo, Blues, Rock,
  Pesado), cada um para avaliar o tipo de equipamento que combina com ele.
  São gravações de guitarra limpa, direto na interface (sem amplificador nem
  efeito), cortadas em compassos inteiros para a repetição cair no tempo.
- "Gravar 15 s": grava a sua guitarra e repete enquanto você ajusta os botões.

### Equalizador do fim da cadeia
- Um equalizador fixo depois do cabeçote e da caixa: todo o som passa por ele
  (sem faixas, passa intacto). Estilo FabFilter Pro-Q: até 24 faixas; duplo
  clique na curva cria uma faixa, arrastar muda frequência e ganho, a roda do
  mouse muda a largura e o botão direito troca o tipo (sino, prateleiras,
  cortes de 12 a 48 dB por oitava, notch) ou apaga.
- Na cadeia, o ícone mostra cinco deslizantes que acompanham a curva.

### Equipamentos novos
- Mark Five (inspirado no Mark V): três canais, cada um com a sua captura.
  Ao escolher o cabeçote, os pacotes "Canal 1", "Canal 2" e "Canal 3" da
  pasta dele entram sozinhos no canal certo.
- Mark 4x12: caixa que acompanha o Mark Five (cada cabeçote abre com a sua
  caixa; dá para trocar na lista de caixas).
- Pear Gate: redutor de ruído de caixinha branca, com um botão grande de
  sensibilidade e dois pequenos de ataque e release.
- Os equipamentos 3D passam a ser gerados na placa de vídeo (lab2, RTX 3050):
  um pedal completo em 26 minutos, antes 2 a 3 horas no processador do Mac.

### Som com duas capturas, Windows e Linux
- Pedal entra no cabeçote no nível da própria captura, como no NAM. Antes,
  cada pedal era levado a -18 dB: o TS9 da TONE3000 entrava 15 dB mais forte
  que o pedal real, com muito mais distorção e chiado ao ligar duas capturas.
  Medido com o Lemon na frente: chiado do Brit 800 de -68 para -82 dB. O
  botão nível ajusta até +12 dB.
- Prova do motor: com as mesmas capturas, o Nexus dá o mesmo som do NAM
  oficial (diferença 152 dB abaixo do som).
- Mark Five, canal Lead: a captura padrão passa a ser a Mikko IIC+ (pacote
  2399). As de alto ganho do pacote 57410 deixavam as pausas quase no volume
  das notas.
- Captura padrão automática: peça vazia (pedal, canal do cabeçote, caixa)
  recebe a captura padrão sozinha, do computador ou baixada da TONE3000 com
  a conta conectada. Trocar continua pelo pacote.
- Windows: o programa e o plugin não dependem mais das bibliotecas do Visual
  C++ (num Windows limpo não abriam); a janela se reduz em telas menores que
  1400x800.
- Linux: com o pipewire-jack instalado, o Nexus usa o JACK do PipeWire (cada
  placa pelo nome, como a Focusrite, e o bloco pedido ao PipeWire: 128
  amostras, 5,2 ms de ida e volta na Scarlett 2i2). O dispositivo padrão do
  ALSA aparece como "Padrão do sistema".

### Som e uso ao vivo
- TOM dos pedais refeito: agora é uma prateleira de agudos a partir de
  1,2 kHz, ±12 dB, como no TS9 e no SD-1. Antes ficava acima de 4 kHz e,
  depois do cabeçote e da caixa, mexia só 0,1 dB no som (medido). Agora,
  com o Brit 800 e a caixa: TOM em 10 reforça os agudos em 6 a 8 dB; em 0,
  escurece 1,5 a 3 dB (o cabeçote distorcido recria parte dos agudos).
- "Testar o som" não passa mais pelo controle de entrada (com a entrada no
  mínimo ele saía 24 dB mais baixo, quase mudo).
- Equalizador universal: grave, médio, agudo e presença agora ficam depois
  da captura e da caixa, iguais em todo cabeçote, e mexem de verdade no som
  (até ±12 dB; medido com o Brit 800 e a caixa Greenback: grave ±9 dB,
  médio ±8, agudo e presença ±9 a 10 nas suas faixas). Antes era o
  equalizador do circuito aplicado só como diferença antes da caixa, sutil
  demais. Com tudo em 5 o som é o da captura, sem mudar nada.
- Drive dos pedais medido captura por captura (`NEXUS_PERFIL_DRIVE=1
  nexus_perfil`): o Nexus toca cada captura igual ao NAM oficial. O pacote
  do SD-1 varia pouco depois do drive 3 (é o próprio pacote).
- Bem mais leve: as capturas eram preparadas para pedaços de 2048 amostras e
  a placa manda 128; isso deixava cada captura A2 duas vezes mais lenta.
  Agora são preparadas no tamanho do bloco da placa (e recarregadas sozinhas
  se ele mudar). Medido com Banana Overdrive + Brit 800 + caixa em A2 Full,
  48 kHz, bloco 128, num Mac Intel i9: parado, de 70% do tempo do bloco e
  211 engasgos em 1500 blocos para 21% e nenhum; girando o drive com a mão
  no botão, de 452% para 51%, sem engasgo. Ferramenta: `nexus_perfil`.
- Mão no botão, capturas prontas: ao pegar o drive de um pedal ou o ganho de
  um cabeçote (antes de girar), a captura vizinha de cada lado daquele
  equipamento começa a aquecer. O giro entra
  na captura nova sem esperar os 132 ms de aquecimento de uma A2 e sem
  engasgo. Ao soltar, depois de 1,5 s, elas voltam a descansar para poupar o
  processador.
- Entrada no mudo: botão "Mudo" ao lado da entrada. O controle todo para
  baixo (-24 dB) também corta de vez, porque com ganho alto o -24 dB ainda
  apitava com o microfone do notebook. O "Testar o som" toca mesmo com a
  entrada no mudo.
- Caixa: "escolher IR" abre os IRs da TONE3000 (recomendadas para a 4x12:
  Greenback 1960TV, JCM900 1960, G12M e JCM800 1960; favoritos, downloads,
  catálogo). O pacote inteiro vai para Documentos/Nexus Modeler/Brit 4x12 e
  "trocar IR" lista os microfones e posições do pacote, além de "IRs da
  TONE3000..." e "Arquivo do computador...".
- Mac: o ícone do programa tem agora todos os tamanhos (16 a 1024); antes o
  Dock e o Finder mostravam o ícone em branco.

### Estúdio (primeira versão)
- Chave "Equipamento | Estúdio" no alto. A guitarra continua passando pela
  cadeia e se mistura com a música.
- Importar música (MP3, WAV, FLAC, M4A, AAC, OGG, AIFF; ou arrastar o
  arquivo): separa em voz, guitarra, baixo, bateria, piano e resto com o
  Demucs de 6 faixas (demucs.cpp), em segundo plano, com andamento e
  "Cancelar". O modelo (55 MB) é baixado na primeira separação. A mesma
  música não é separada duas vezes. A barra mostra o trabalho inteiro (ler,
  separar e gravar), com a porcentagem e quanto falta.
- A separação roda em prioridade baixa, em no máximo 6 linhas e sem as
  linhas extras do Accelerate: o áudio e a tela seguem leves enquanto separa
  (medido: o áudio cabe no tempo do bloco com uma separação rodando).
- Player: tocar/pausar (barra de espaço), voltar ao começo, forma da música
  (clique pula; arrastar marca o trecho A–B), "Repetir A–B", volume, mudo e
  solo por faixa com medidor, velocidade 50–100% sem mudar o tom, tom −6 a +6
  semitons, volume da música. A mistura de cada música fica guardada.
- Músicas e playlists na coluna da esquerda: criar, renomear, apagar, pôr e
  tirar músicas, subir e descer; "Tocar a playlist" segue sozinha.
- Faixas em FLAC em `Documentos/Nexus Modeler/Estúdio/<música>/`.
- Ainda falta: separação pela placa de vídeo (hoje só processador, cerca de
  3 vezes a duração da música num Mac Intel de 8 núcleos) e contagem antes
  de começar.

### Relatório de problemas
- Menu "i" → "Reportar um problema...": a pessoa conta o que aconteceu e o
  Nexus abre uma issue nova no GitHub, no modelo "Problema ou travamento", já
  com a versão, o sistema e os últimos erros registrados. O texto completo
  também vai para a área de transferência.
- Registro de erros (`registro.txt`, na pasta de ajustes): pacote que não
  abre, IR não encontrado, falhas da TONE3000, da placa de som e da
  atualização. A pasta do usuário vira "~"; senhas e chaves nunca entram.
- Travamento: se o programa independente fechar sozinho, o relatório (versão,
  sistema e onde parou) fica guardado e é oferecido na abertura seguinte.
- Windows: corrigido o erro de compilação no download da TONE3000.

### Atualização automática
- Ao abrir, o Nexus procura uma versão nova no GitHub (no máximo uma vez por
  dia). O aviso "Nova versão" aparece no topo; um clique baixa o instalador do
  sistema para a pasta Downloads e o abre. No menu "i": procurar agora e
  ligar ou desligar a procura automática.

### Marca
- Logo nova: triângulo de Penrose clássico (três fitas em L) nas cores do
  Nexus, com trilhas de circuito. Um só arquivo (`assets/marca/nexus_marca.svg`,
  gerado por `tools/marca/logo.py`) para a tela, o ícone e o site.

### Pedais
- Bloco de pedal antes do amplificador, tocando captura NAM ou pacote de
  ganho (mesmo motor de troca sem clique do amplificador).
- Botões Ganho, Tom e Nível; o pisante liga e desliga em 20 ms, sem estalo.
- Pedais vistos de frente, retos (câmera bem em cima), como na pedaleira,
  com as medidas dos pedais reais e luz de frente.
- Ligado ou desligado, o pedal tem a mesma luz: só o LED vermelho acende.
- Drive em degraus de 0 a 10 (cada degrau é uma captura); tom e nível livres.
- Pedais de captura inspirados em clássicos: Lemon Drive (TS9), Orange Drive
  (DS-1), Banana Overdrive (SD-1) e Gold Centaur (Klon).
- Efeitos feitos pelo próprio Nexus, sem captura: Blueberry Chorus (CE-2),
  Coconut Delay (DD-3) e Grape Reverb (RV-6). Desligados, o delay e o reverb
  terminam de soar (rastro) em vez de cortar.
- Motor A2 Lite por pedal (botão direito no pedal da cadeia).

### Caixa
- Abas Desligada / Captura / IR próprio. IR em `.wav` ou `.aiff`, com
  transição de 30 ms.
- Captura full rig (cabeçote + caixa): a caixa se tranca em "Captura" e o IR
  não toca. Reconhecida pelo tipo gravado no `.nam` (`amp_cab`) ou por
  `"inclui_caixa": true` no `pack.json`.

### Áudio e MIDI
- Programa independente com janela própria e painel de áudio: ASIO (kit da
  Steinberg, GPL v3), WASAPI, DirectSound, Core Audio, JACK e ALSA; latência
  de ida e volta, uso do processador e medidor da guitarra.
- MIDI por USB e Bluetooth (WinRT MIDI no Windows, pareamento no Mac):
  troca de patch escolhe o canal, "Aprender MIDI" em qualquer controle.
- Cabeçote com até 3 canais, conforme o amplificador original.
- O plugin recebe MIDI do programa de gravação (o AU passa a ser "efeito com
  MIDI").

### Idiomas
- Tela em 15 idiomas, com seletor (globo) e o idioma do sistema na primeira
  vez; árabe da direita para a esquerda; fontes do sistema para japonês,
  chinês, coreano, hindi, bengali e árabe.
- Rótulos dos painéis viraram caixas de texto traduzidas por cima do 3D (sem
  render por idioma).
- Os instaladores trazem o seletor de idioma (veja "Instaladores").

### Instaladores
- Windows: instalador `.exe` (Inno Setup) com VST3, programa independente e
  atalho; escolha do idioma do instalador e página "Idioma do Nexus" com os
  15 idiomas, já marcando o do instalador. Instala para todos os usuários
  (administrador) ou só para quem instala.
- Mac: `.dmg` com instalador `.pkg` de componentes escolhíveis (VST3, AU e
  programa independente). A tela do Nexus começa no idioma do Mac. Ainda sem
  assinatura da Apple.
- Linux: pacote `.deb` com VST3, LV2, programa `nexus-modeler`, atalho no
  menu e ícone.
- Instalação pelo terminal, sem administrador: `packaging/instalar.sh`
  (Linux e Mac) e `packaging/instalar.ps1` (Windows), com a pergunta do
  idioma ou `--idioma`.
- Fluxo `pacotes.yml`: gera os pacotes nos três sistemas e, numa etiqueta
  `v*`, anexa ao lançamento do GitHub.

### Ferramentas
- Nexus Gear Studio (`tools/gear_studio`): gera amplificadores, caixas e
  pedais no Blender a partir de receitas e renderiza as imagens do plugin.

## 0.1.0 — prova do motor (não publicada)
- Motor de troca de capturas NAM A2 sem clique, com mistura das vizinhas.
- Equalizador do painel modelado no circuito de tonalidade.
- Plugin VST3, AU, LV2 e versão independente; Windows, Mac e Linux.
