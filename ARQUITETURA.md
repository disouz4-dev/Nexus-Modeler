# Nexus Modeler — proposta de arquitetura

Estado (26/10/2026): prova do motor pronta; tela 3D com o Brit 800, a caixa
4x12 (IR e trava do full rig) e três pedais (Verde Drive, Laranja Dist e Ouro)
tocando capturas. O resto é proposta.
Repositório: https://github.com/disouz4-dev/Nexus-Modeler

## 1. O que o Nexus Modeler é

Um plugin e aplicativo de guitarra, **gratuito**, para Windows, Mac e Linux,
com três partes:

1. **Cadeia de equipamentos virtuais:** pedais → amplificador → caixa, todos
   com visual 3D no nível do ToneX 2 e do AmpliTube 5. O som vem de capturas do
   Neural Amp Modeler (NAM) importadas direto da TONE3000.
2. **Motor de troca de capturas:** quando um criador publica várias capturas do
   mesmo equipamento (ganho 0, 1, 2 … 10), o Nexus junta tudo num único
   equipamento virtual. Girar o botão troca de captura por dentro, sem clique
   no áudio e sem o usuário abrir pasta nenhuma.
3. **Player de estudo:** o usuário importa uma música, o Nexus separa as faixas
   (voz, bateria, baixo, guitarra, piano, resto), e ele pode silenciar a
   guitarra original e tocar por cima com o timbre do Nexus. Serve para
   estudar e para gravar.

## 2. O que a pesquisa mostrou

### Motor do NAM (NeuralAmpModelerCore)
- Licença MIT: pode ser usado em projeto aberto ou fechado, só mantendo o aviso
  de autoria. C++. Versão atual 0.6.0.
- Lê capturas A2. O A2 Full e o A2 Lite são **o mesmo arquivo**: um ajuste de
  tamanho (`SetSlimmableSize`, de 0 a 1) escolhe a versão leve ou a mais fiel.
- Lê também as respostas de caixa (IRs, arquivos `.wav`) pelo mesmo caminho.
- Cada captura informa o volume em que foi gravada; isso permite igualar o
  volume entre capturas, e a troca não dá "pulo" de volume.
- Cada captura precisa de um "aquecimento" curto antes de soar certo; o motor
  já tem a função (`prewarm`).

### Plugin oficial do NAM
- Feito com iPlug2. Não tem versão Linux. Troca de captura de forma seca, sem
  transição.

### Plugin oficial da TONE3000 (tone3000-plugin)
- Licença MIT, publicado em 2026, feito com **JUCE 9**. Sai como VST3, AU,
  CLAP, LV2 e versão independente, em Windows, Mac e Linux.
- Já resolve: login na conta da TONE3000 pelo navegador do sistema, busca e
  download de capturas e IRs, A2 Full/Lite, troca de captura sem travar o
  áudio, ajuste de taxa de amostragem.

### Regras da TONE3000 (interface de programação, a "API")
- Login com a conta do usuário. Precisamos criar a chave do aplicativo na conta
  do projeto e cadastrar o endereço de retorno.
- Um "tom" pode ter **vários modelos** dentro — é assim que os pacotes de
  ganho aparecem.
- Tipos de equipamento: `amp` (cabeçote), `amp-cab` (cabeçote + caixa, o
  "full rig"), `pedal`, `cab` (IR de caixa), `outboard`, `space`.
- **Plano gratuito** — o texto da TONE3000 diz: "free and open software or
  hardware with no paid product, or upsell" (software gratuito **e aberto**,
  sem produto pago). Nesse plano o aplicativo só pode usar:
  - a **tela de escolha da própria TONE3000** (`select_tone`), que aceita
    filtro por tipo de equipamento (amp, amp-cab, pedal, cab…), formato e
    arquitetura (A2), **mas não por marca**;
  - a **carga de um tom já conhecido** pelo número (`load_tone`);
  - listas prontas: favoritos, baixados, criados, em alta, recentes.
- A **busca com filtro de marca** ("todos os Marshall") é do endpoint de
  busca, que fica no **plano comercial**.
- Exigências visuais: tela de parceria antes do login, logo da TONE3000 nas
  listas, nome e foto do criador de cada captura, caminho sempre visível para
  o catálogo completo.
- Cada tom tem licença própria. O Nexus **não embute** capturas de terceiros:
  baixa da TONE3000 na conta do próprio usuário.

### JUCE
- JUCE 8 em diante tem o plano **Starter gratuito** (até US$ 20 mil de receita
  por ano, sem tela de abertura obrigatória), que permite código aberto ou
  fechado. Como o Nexus é gratuito, cabe nele. Também pode ser usado como
  código aberto pela licença AGPLv3.

### Separação de faixas
- **Observação de nome:** RVC é uma técnica de **troca de voz** (cantar com a
  voz de outra pessoa), não de separação. Separar a música em faixas é
  "separação de fontes"; os modelos atuais são o **Demucs** (da Meta) e os da
  família **RoFormer** (BS-RoFormer e Mel-RoFormer), os mesmos usados no
  Ultimate Vocal Remover (UVR) e no MVSep. "NUX" provavelmente era MDX, outra
  família usada no UVR.
- **htdemucs_6s** (Demucs de 6 faixas) separa voz, bateria, baixo,
  **guitarra**, piano e resto. Existe em C++ puro (`demucs.cpp`), o que encaixa
  direto no plugin. Os próprios autores chamam a faixa de guitarra de "ok".
- **BS-RoFormer** tem guitarra melhor (numa medição do MVSep: 7,16 contra 5,22
  do Demucs de 6 faixas — números de um único site), mas não tem versão C++
  pronta; roda pelo ONNX Runtime (motor genérico de modelos de inteligência
  artificial que usa a placa de vídeo no Windows, Mac e Linux).

## 3. Arquitetura recomendada

### 3.1 Base técnica
| Peça | Escolha | Por quê |
|---|---|---|
| Linguagem | C++20 | Língua do motor do NAM e do áudio profissional. |
| Biblioteca de plugin | JUCE 9 (plano Starter gratuito) | Um código para Windows, Mac e Linux; VST3, AU, CLAP, LV2 e versão independente. |
| Som das capturas | NeuralAmpModelerCore | Motor oficial, A2 Full/Lite e IR. |
| Conta e downloads | Código adaptado do tone3000-plugin | MIT permite; evita refazer login e download. |
| Separação de faixas | 1º `demucs.cpp` (htdemucs_6s); depois RoFormer via ONNX Runtime na placa de vídeo | Começa simples e embutido; sobe a qualidade da guitarra depois. |
| Compilação | CMake | Padrão do JUCE e do NAM. |
| Modelos 3D | Blender (scripts em Python) | Renders viram imagens dentro do plugin. |
| Site | GitHub Pages | Gratuito no repositório público. |

**Por que C++ e não uma tecnologia de aplicativo comum (Electron, Flutter,
página web):** o plugin roda dentro do programa de gravação (Reaper, Logic,
Cubase, Ableton) como VST3, AU ou LV2, e esses formatos só aceitam código
nativo com latência de poucos milissegundos. ToneX, AmpliTube, o plugin do NAM
e o da TONE3000 são todos feitos assim; o da TONE3000 usa o mesmo JUCE 9. O
JUCE é a base multiplataforma atual desse mercado: um código só gera Windows,
Mac e Linux. O CMake é ferramenta interna do desenvolvedor; o usuário nunca vê.

**Instaladores (etapa 6):** `.exe` no Windows, `.dmg` de arrastar e soltar no
Mac, `.deb` no Linux, e instalação por um comando no terminal nos três
sistemas (no Linux também por repositório `apt`, como no Praxis).

Descartados: iPlug2 (fraco no Linux), Rust (refazer NAM e login), interface
como página de internet embutida (a TONE3000 começou assim e abandonou),
cópia inteira do tone3000-plugin (produto de outro formato).

### 3.2 Camadas do programa
```
┌──────────────────────────────────────────────────────────────┐
│ Interface 3D: cadeia → equipamento aberto → player de estudo │
├──────────────────────────────────────────────────────────────┤
│ Biblioteca de equipamentos (simulacros + mapas de capturas)  │
├──────────────────────────────────────────────────────────────┤
│ Conexão TONE3000: login, importar, baixar, guardar no disco  │
├──────────────────────────────────────────────────────────────┤
│ Separação de faixas (fora do tempo real, em segundo plano)   │
├──────────────────────────────────────────────────────────────┤
│ Motor de som em tempo real                                   │
│  Guitarra → Pedais → Amplificador → Caixa ─┐                 │
│  Player (faixas da música, cada uma com    ├→ Mistura → saída│
│          volume e mudo) ───────────────────┘                 │
└──────────────────────────────────────────────────────────────┘
```

### 3.3 Simulacros de equipamento
- Cada equipamento do Nexus é um **simulacro**: modelo 3D com o espírito do
  original, nome próprio e sem logo da marca. Exemplo: "Brit 800" e
  "Brit 900" no lugar do Marshall JCM800 e JCM900. Usar o nome e o logo da
  Marshall exige licença da marca; a descrição pode dizer "inspirado no
  clássico britânico dos anos 80".
- Começo sugerido: 1 amplificador (Brit 800), 1 pedal de drive (estilo
  overdrive verde), 1 caixa 4×12.
- Equipamento genérico (amplificador, pedal ou caixa sem marca) para qualquer
  captura que não tenha simulacro próprio.

### 3.4 Botão "Importar NAM"
Cada simulacro tem o botão **Importar NAM**. Ele abre:

1. **Lista curada do Nexus** — nós mantemos, para cada simulacro, uma lista de
   tons da TONE3000 daquele equipamento (ex.: todos os JCM800 em A2 que
   encontrarmos). O usuário escolhe e o Nexus carrega pelo número do tom
   (`load_tone`), permitido no plano gratuito. É assim que o "filtro Marshall"
   funciona sem o plano comercial. A lista é atualizada pela internet, sem
   precisar de versão nova do plugin.
2. **Catálogo completo da TONE3000** — abre a tela de escolha deles, já
   filtrada pelo tipo certo (`gears=amp_amp-cab` no amplificador,
   `gears=pedal` no pedal, `gears=cab` na caixa) e A2. Dentro da tela deles o
   usuário pode procurar "Marshall".
3. **Arquivo do computador** — para quem já tem `.nam` ou `.wav` baixados.

Se a TONE3000 liberar a busca (por pedido ou plano comercial), o item 1 passa a
ser uma busca direta por marca dentro do plugin.

### 3.5 Captura "full rig" (cabeçote + caixa)
Quando o tom importado é do tipo `amp-cab`, a captura já inclui a caixa. O
Nexus então:
- **desliga e tranca o bloco da caixa** (não aceita IR), mostrando "Caixa
  incluída na captura";
- mantém pedais antes e efeitos de ambiente depois;
- ao trocar para uma captura só de cabeçote (`amp`), a caixa volta.

Para capturas do computador, o tipo vem das informações gravadas no `.nam`;
se não houver, o usuário marca "inclui caixa".

### 3.6 Motor de troca de capturas
1. Ao abrir um equipamento, **todas** as capturas do pacote são carregadas na
   memória fora do fio do áudio.
2. Botão numa posição com captura: roda **uma**.
3. Botão entre duas posições (ex.: ganho 2,5): roda as duas vizinhas e mistura,
   com volume igualado. Opção "degraus" para economizar processador.
4. Ao girar: a próxima captura é aquecida com o som recente e entra numa
   transição de ~20–50 ms. Sem clique.
5. No fio do áudio nada de abrir arquivo, reservar memória ou esperar outro
   processo: só troca um ponteiro já pronto.
6. Full/Lite: ajuste global e por equipamento.

O mesmo motor vale para pedais (drive 0–10) e caixas (posição de microfone).
**Todo amplificador tem equalizador**, e ele é modelado no circuito de
tonalidade do amplificador real de cada família (Marshall, Fender, Vox, Mesa
Mark com gráfico de 5 faixas etc.). O Nexus aplica a diferença entre a posição
em que a captura foi feita e a posição escolhida pelo usuário. Detalhes em
`docs/MODELOS-3D.md`, seção 3.

### 3.7 Como o Nexus sabe que "modelo 7" é "ganho 7"
1. Simulacros oficiais: o mapa vem pronto na lista curada.
2. Outros pacotes: o Nexus lê os números nos nomes dos modelos e propõe o mapa.
3. Tela simples para o usuário corrigir.

Arquivo de descrição (rascunho):
```json
{
  "id": "brit-800",
  "nome": "Brit 800",
  "tipo": "amp",
  "visual": "skins/brit-800",
  "tons_curados": [
    {
      "tone3000_tone_id": 12345,
      "botao": "ganho",
      "capturas": { "0": "model_id_a", "5": "model_id_f", "10": "model_id_k" }
    }
  ],
  "botoes_extras": ["grave", "medio", "agudo", "presenca", "volume"]
}
```

### 3.8 Player de estudo e separação de faixas
- Importar música: MP3, WAV, FLAC, M4A.
- A separação roda **em segundo plano**, fora do tempo real, com barra de
  progresso; o resultado fica guardado no disco, e a mesma música não é
  separada duas vezes. Na placa de vídeo leva segundos; só no processador,
  alguns minutos por música.
- Player: tocar, pausar, posição, volume e **mudo** por faixa (guitarra, voz,
  baixo, bateria, piano, resto), **repetir um trecho** (A–B),
  **desacelerar sem mudar o tom** (50–100%), **mudar o tom** em semitons,
  contagem antes de começar.
- A guitarra do usuário passa pela cadeia do Nexus e se mistura com a música.
- Na versão independente, tudo vai para a placa de som. Dentro de um programa
  de gravação, o player toca junto e o plugin entrega a mistura (com opção de
  mandar só a guitarra para gravar).
- Primeira versão com htdemucs_6s embutido; segunda etapa: RoFormer de
  guitarra pela placa de vídeo, se a licença dos pesos permitir.

### 3.9 Interface e 3D
- Tela com identidade própria (o ToneX 2 e o AmpliTube 5 servem só de régua
  de acabamento; a disposição não é copiada): barra de cima com a marca, o
  seletor A2 FULL | A2 LITE e os ícones; o palco com o equipamento escolhido
  sob um foco de luz; embaixo dele, o nome grande, o pacote e o trilho das
  capturas; e no rodapé o caminho do sinal: controle deslizante de entrada
  com medidor, pedal, amplificador, caixa e controle deslizante de saída.
- Modelos 3D feitos pelo Nexus Gear Studio (`tools/gear_studio`) no Blender,
  renderizados como imagens. Cada botão giratório = 101 imagens (posição 0,0 a
  10,0 de 0,1 em 0,1); chaves e luzes, poucas imagens.
- Renders em 2 tamanhos (normal e alta densidade).
- Telas: cadeia, equipamento aberto, player de estudo, importação.
- Elementos exigidos pela TONE3000 no mesmo acabamento.

### 3.10 Organização de pastas (proposta)
```
Nexus-Modeler/
  CMakeLists.txt
  libs/            JUCE, NeuralAmpModelerCore, demucs.cpp (baixados pelo CMake)
  src/
    audio/         motor de troca, pedais, amplificador, caixa, equalização
    gear/          descrições, listas curadas, reconhecimento de nomes
    tone3000/      login, importação, download, cache
    separation/    separação de faixas em segundo plano
    player/        player de estudo (tempo, tom, trecho, mudo)
    ui/            telas e botões animados
  gear/            descrições e listas curadas dos simulacros
  art/blender/     scripts de modelagem e render
  art/renders/     imagens geradas
  site/            página no GitHub Pages
  tests/
  docs/
```

## 4. Ordem de construção
1. **Prova do motor:** plugin mínimo que carrega um pacote de ganho local e
   troca de captura sem clique. Medir processador.
2. **Conexão TONE3000:** login, Importar NAM (lista curada + catálogo +
   arquivo), full rig trancando a caixa.
3. **Cadeia completa:** pedais, amplificador, caixa, equalização, salvar ajustes.
4. **Player de estudo** com htdemucs_6s.
5. **Primeiro simulacro 3D (Brit 800)**, iterado até o nível ToneX/AmpliTube.
6. **Instaladores** para Windows, Mac e Linux e testes nos três sistemas.
7. **Site no GitHub Pages**, no estilo da página do Praxis, com referências da
   SpaceX e da Apple.
8. Mais simulacros e separação RoFormer.

## 5. Decisões pendentes
1. **Código aberto ou fechado.** O repositório já está público. O plano
   gratuito da TONE3000 fala em software "gratuito e aberto"; fechado e
   gratuito não está claro e pede confirmação com support@tone3000.com.
   Recomendação: aberto.
2. **Pedir à TONE3000 a busca por marca** no plano gratuito (e-mail para
   support@tone3000.com). Sem isso, o filtro Marshall é feito pela lista curada.

## Fontes
- Motor do NAM: https://github.com/sdatkinson/NeuralAmpModelerCore
- Plugin do NAM: https://github.com/sdatkinson/NeuralAmpModelerPlugin
- Plugin da TONE3000: https://github.com/tone-3000/tone3000-plugin
- API da TONE3000: https://www.tone3000.com/api
- NAM A2: https://www.tone3000.com/blog/introducing-neural-amp-modeler-nam-architecture-2-a2
- Licença JUCE 8: https://forum.juce.com/t/amendments-to-the-juce-end-user-licence-agreement-for-juce-8/61265
- demucs.cpp: https://github.com/sevagh/demucs.cpp
- Comparação de guitarra (MVSep): https://www.mvsep.com/algorithms/17
