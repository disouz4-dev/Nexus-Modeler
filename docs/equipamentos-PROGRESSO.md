# Equipamentos baixáveis: progresso

Plano: `docs/equipamentos-baixaveis.md` (commit `4d83ec9`, ramo
`claude/vibrant-heisenberg-8038e5`), etapas 1, 2, 4 e 5 da seção 6. Ramo de
trabalho: `equipamentos-novos`, pasta `Nexus Modeler-equipamentos`, compilação
em `build-teste/`.

Decisões do dono que valem sobre o plano:

- gratuitos no próprio `disouz4-dev/Nexus-Modeler`, ramo `main`, lidos por
  `raw.githubusercontent.com`; capturas apontam para a TONE3000;
- um baixador só (o `ArteNova` de 3901d61, estendido), um índice só
  (`assets/gear/indice.json`) e uma pasta só (`Equipamentos/<id>/`);
- sem assinatura digital por ora: o índice traz o sha256 de cada arquivo e o
  programa confere antes de instalar;
- ficha de cada peça em `assets/gear/<id>_manifesto.json` (padrão `<id>_*`
  para o `tools/renomear_pedais.py`);
- a automação que empacota cada peça roda no lab2 (Linux, python3 + Pillow).

## Etapas

- [x] **1. Catálogo de fábrica unificado.** `plugin/source/gear/GearCatalog`
  lê as fichas embutidas (186: 63 pedais, 94 cabeçotes, 29 caixas) e as da
  pasta `Equipamentos/`. Editor, navegador e motor dos pedais leem do
  catálogo; as tabelas `kPedalPrefixes`, `kPedalInfo`, `kPedalKinds`,
  `kAmpPrefixes`, `kAmpInfo`, `kCabOfAmp`, `kCabPrefixes` e `kCabInfo` saíram.
  Fichas geradas por `tools/gear_studio/manifestos.py` a partir das tabelas de
  3901d61 (`--conferir` compara). `tools/site/catalogo.py` lê as fichas.
  Teste: `nexus_gear_test assets/gear` (catálogo = fichas) e fotos da tela
  (`nexus_snapshot`) iguais às de antes, fora o texto de uso do processador.
- [x] **2. Id em texto no estado.** `p<n>_visual_id`, `cabecote_id[_b]`,
  `gabinete_id[_b]` no estado; parâmetros de escolha congelados; peça baixada
  guarda a escolha à parte e deixa o parâmetro no de fábrica do mesmo motor;
  id não instalado fica marcado (pedal passa direto) e não se perde ao salvar.
  Teste: `nexus_gear_test` monta o estado da versão antiga (sem ids) e confere.
- [ ] **4. Pacote no disco e instalação.**
- [ ] **5. Busca e download no mesmo repositório.**

## Para retomar

1. `ninja -C build-teste nexus_gear_test nexus_arte_test nexus_snapshot`
2. `build-teste/plugin/nexus_gear_test_artefacts/Release/nexus_gear_test assets/gear`
3. Seguir da primeira etapa sem `[x]`.
