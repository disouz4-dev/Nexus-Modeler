# Pedais fiéis aos originais: progresso

Ramo `fiel-pedais` (saiu de `modelagem-fiel`), pasta `Nexus Modeler-fiel-pedais`.
Ponto de retomada: quem continuar lê este arquivo e `git log --oneline` do ramo,
vê a última etapa marcada e segue da próxima. Cada etapa termina com um commit.

Arquivos desta frente: `tools/gear_studio/pedal.py`, `tools/gear_studio/knobs.py`
(se preciso), `tools/gear_studio/logos/` (estampas e o gerador `fiel_arte.py`),
`tools/gear_studio/recipes/<pedal>.json` e este arquivo. Nomes de arquivo e
códigos das receitas não mudam (a troca é outra etapa).

## Como gerar a arte (estampa SVG)

`logos/fiel_arte.py <id> ...` lê a receita, acha o espaço livre (botões, peças,
pisantes, textos, painéis) e desenha o tema (fruta, pedra ou astro) e o padrão
de fundo. Precisa do shapely: `python3 -m venv venv && venv/bin/pip install shapely`
e rodar com esse python.

## Etapas

- [x] 1. Recursos novos no pedal.py (com prévias de teste). Campos novos da receita
      explicados no comentário antes de `build()`: formato redondo e wah, cunha,
      disposição boss, laterais de outra cor, textura da pintura, cor do LED,
      estilos de pisante (metal, redondo, pequeno, nenhum; no clássico placa com
      moldura e placa pequena), placa prateada da etiqueta, botões fixos,
      concêntricos, cabeça de galinha, transparentes, deslizantes na lista de
      botões, peças fixas (minichave, apertar, chave deslizante, trimpot, LED,
      fileira de LEDs, placa, pisante a mais), painéis e textos com cor própria.
      Quadro: com proporção > 1 a câmera mede pela altura (altura vista =
      largura x largura_rel); pedal comprido precisa de largura_rel maior.
- [ ] 2. Receitas em lotes (cor, medidas, botões, chaves, nome, estampa, inspiração)
  - [ ] Lote A: Boss compactos (orange_drive, banana_overdrive, neptune_chorus,
        coconut_delay, grape_reverb, moon_tremolo, jabuticaba_buzzsaw,
        avocado_metal, blueberry_blues, apricot_overdrive, clementine_turbo,
        jupiter_octave, acerola_eq)
  - [ ] Lote B: MXR e caixas estreitas (pear_gate, mars_phaser, quince_plus,
        apple_comp, starfruit_rock, cranberry_wylde, guava_precision, honeydew_drive,
        peach_glory, raspberry_drive, gooseberry_g3, nectarine_od, mangosteen_fuzz,
        grapefruit_drive, pitanga_250)
  - [ ] Lote C: clássicos e formatos especiais (lemon_drive, olive_drive,
        pistachio_drive, gold_centaur, cherry_drop, acai_rat, snowpear_treble,
        cherry_fuzz, fig_bender, kiwi_drive, comet_wah, pomegranate_mud,
        lychee_fuzz, nebula_flanger, elderberry_war, tamarind_amp)
  - [ ] Lote D: caixas mini, deitadas e grandes (cashew_boost, kumquat_klone,
        papaya_grind, persimmon_33, blackcurrant_bass, mulberry_omega,
        pineapple_king, strawberry_5150, cupuacu_hammer, graviola_bassdi,
        pulsar_echo, jackfruit_monkey, lychee_bassfuzz, juniper_sky, lime_echo,
        galaxy_mod)
- [ ] 3. Conferir na fila que todos ficaram "pronta" e sem erro

Não mexer: saturn_rotary (decisão do dono), eclipse_vibe e blueberry_chain (veredito "ok").

## Botões que sobraram (o original tem menos botões que a receita)

Ficam funcionando, pequenos e no canto mais discreto. O dono decide depois.

(preenchido a cada lote)

## Mudanças em build.py

Nenhuma até agora.

## Registro

- 29/10: ramo e pasta criados.
