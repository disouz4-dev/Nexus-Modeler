# Notas de versão

## Próxima versão (em desenvolvimento, ainda não publicada)

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
