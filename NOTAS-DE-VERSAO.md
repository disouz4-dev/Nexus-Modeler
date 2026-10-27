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

### Pedais
- Bloco de pedal antes do amplificador, tocando captura NAM ou pacote de
  ganho (mesmo motor de troca sem clique do amplificador).
- Botões Ganho, Tom e Nível; o pisante liga e desliga em 20 ms, sem estalo.
- Três visuais inspirados em pedais reais, com nomes próprios: Verde Drive,
  Laranja Dist e Ouro.

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

### Ferramentas
- Nexus Gear Studio (`tools/gear_studio`): gera amplificadores, caixas e
  pedais no Blender a partir de receitas e renderiza as imagens do plugin.

## 0.1.0 — prova do motor (não publicada)
- Motor de troca de capturas NAM A2 sem clique, com mistura das vizinhas.
- Equalizador do painel modelado no circuito de tonalidade.
- Plugin VST3, AU, LV2 e versão independente; Windows, Mac e Linux.
