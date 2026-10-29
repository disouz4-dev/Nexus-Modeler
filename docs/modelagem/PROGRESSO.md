# Modelagem fiel aos originais: progresso

Ramo `modelagem-fiel`, pasta `Nexus Modeler-modelagem` (saiu de `pecas-novas` em 0563c19).
Este arquivo é o ponto de retomada: se a luz cair ou a sessão parar, quem continuar
lê este arquivo, vê a última etapa marcada e segue da próxima. Cada etapa termina
com um commit.

## Regra (pedido do Diego, 29/10/2026)

Cada pedal, cabeçote e cabinet tem a cor, o tamanho e as características do
original em que se inspira, e a descrição da galeria diz isso. Pedais com tema
(fruta, pedra preciosa ou planeta) da mesma cor do original. Fontes: site e
manual do fabricante, mais foto do original.

## Pesquisa

- `docs/modelagem/TRIAGEM.md`: o que muda em cada peça, com a fonte.
- `docs/modelagem/verificado_*.json`: dados conferidos (cores, medidas, botões, chaves).
- Fotos e PDFs dos originais: `Nexus Modeler/blender/referencias/` (local, fora do git).

## Decisões aprovadas (29/10, 03h50)

- Nomes novos de pedal (código interno também muda):
  Lemon Drive → Greengage Drive · Grape Reverb → Meteorite Reverb ·
  Pear Gate → Marcasite Gate · Moon Tremolo → Turquoise Tremolo ·
  Lychee Fuzz → Pearl Fuzz · Avocado Metal → Onyx Metal ·
  Pomegranate Mud → Heliodor Mud · Juniper Sky → Aquamarine Sky ·
  Lime Echo → Emerald Echo · Apricot Overdrive → Citrine Overdrive ·
  Fig Bender → Mercury Bender · Kiwi Drive → Opal Drive ·
  Starfruit Rock → Hematite Rock · Tamarind Amp → Basalt Amp ·
  Mulberry Omega → Labradorite Omega · Guava Precision → Spinel Precision ·
  Peach Glory → Amber Glory · Pineapple King → Amethyst King ·
  Cashew Boost → Magnetite Boost · Papaya Grind → Tourmaline Grind ·
  Persimmon 33 → Sapote 33 · Strawberry 5150 → Jet 5150 ·
  Gooseberry G3 → Charoite G3 · Nectarine OD → Jamun OD ·
  Cranberry Wylde → Agate Wylde · Kumquat Klone → Pyrite Klone ·
  Elderberry War → Howlite War · Mangosteen Fuzz → Moonstone Fuzz ·
  Grapefruit Drive → Peridot Drive · Pitanga 250 → Topaz 250 ·
  Lychee Bass Fuzz → Lime Bass Fuzz · Cupuaçu Hammer → Galena Hammer ·
  Graviola Bass DI → Melanite Bass DI · Acerola EQ → Selenite EQ
  (Grape Reverb e Lychee Bass Fuzz trocaram de proposta para não repetir
  Labradorite e Greengage.)
- Originais que são combo viram combo, no tamanho real.
- Mark Four: cabeçote curto (476 mm). Vault 300: SVT antigo (painel prata,
  letras azuis). Nebula Flanger: Electric Mistress clássico (prata, grande).
  Dark Star 20: Blackstar HT-20. Little Tube: painel bronze. Saturn Rotary fica
  como está.

## Etapas

- [x] 0. Pesquisa, triagem e conferência no fabricante
- [x] 1. Fila 3D com retomada (lab2) e pasta de render própria do ramo
      (`tools/gear_studio/lab2_sync.sh` manda o ramo para `~/nexus-gear/repo-fiel`;
      pedidos com `--repo ~/nexus-gear/repo-fiel`)
- [ ] 2. Gerador: recursos novos dos pedais (Boss com LED no alto e botão do
      meio mais baixo, redondo, wah, cunha, pisantes múltiplos, minichaves,
      fileiras de LEDs, mini, cunha RAT)
- [ ] 3. Receitas dos pedais (61) + render
- [ ] 4. Gerador: cabinets (inclinada, grades de metal, frisos, cantoneiras, painel no topo)
- [ ] 5. Receitas das cabinets (27) + render
- [ ] 6. Gerador: cabeçotes (combo, rack, caixinha, fileiras de botões,
      minichaves, frente de tolex sem tela, grades)
- [ ] 7. Receitas dos cabeçotes (94) + render
- [ ] 8. Troca dos nomes e códigos no programa
- [ ] 9. Arte nova no programa, descrições e galeria

A cada lote pronto: trazer a arte para `assets/gear` deste ramo, compilar o
Nexus de teste (Standalone) e avisar o Diego. O Nexus de sempre só muda quando
ele aprovar juntar o ramo.

## Como retomar

1. `git -C "Nexus Modeler-modelagem" log --oneline -5` e este arquivo.
2. Fila no lab2: `ssh lab2 '~/nexus-gear/fila3d.sh ver'`. Se o lab2 reiniciou,
   rodar `ssh lab2 '~/nexus-gear/fila3d.sh retomar'`: as peças que estavam
   "rodando" voltam para a fila e o trabalhador segue. (O início automático
   pelo cron precisa ser ligado pelo Diego.)
3. Seguir da primeira etapa sem [x].

## Registro

- 29/10 03h50: ramo criado, pesquisa guardada.
- 29/10 04h00: fila com retomada e repo-fiel no lab2.
