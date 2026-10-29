# Fichas do lote 5: Laney, ENGL, Diezel Herbert, Bogner, Friedman Dirty Shirley, Dumble.
from cabecotes_fiel import *  # noqa: F401,F403

TOLEX_PRETO = {"cor": [0.006, 0.006, 0.006], "grao_mm": 2.6}
BRANCO = rgb(0.85, 0.85, 0.85)
PRETA = rgb(0.012, 0.012, 0.012)
LISO_PRETO = estilo("liso", 0.019, (0.012, 0.012, 0.012), traco=(0.9, 0.9, 0.9), escala="nenhuma")
EXTRA = estilo("liso", 0.011, (0.015, 0.015, 0.015), traco=(0.9, 0.9, 0.9), escala="nenhuma")
FICHAS = {}

# ------------------------------------------------------------------ Black Country 100 (Laney Supergroup)
FICHAS["blackcountry100"] = dict(
    inspiracao=("Laney Supergroup: cabeçote baixo e largo de 765 × 225 × 220 mm em tolex cinza-prata, tela preta salpicada de "
                "prata com a marca no alto à esquerda e o painel de alumínio prata no terço de baixo, no meio, com letras "
                "pretas: alavancas POWER e STANDBY e a lâmpada vermelha, seis botões cromados de saia preta numerada "
                "(PRESENCE, BASS, MIDDLE, TREBLE, MASTER, GAIN) e as quatro entradas BASS e TREBLE, HI e LO."),
    mm=(765, 225, 220), couro={"cor": [0.5, 0.51, 0.52], "grao_mm": 2.4},
    fachada=tela((0.02, 0.02, 0.022), (0.35, 0.35, 0.37), 1.6),
    logo={"lado_rel": -0.3, "altura_m": 0.014, "altura_rel": 0.78},
    painel=dict(posicao="sobre", altura_m=0.075, largura_m=0.5, margem_m=0.008, cor=rgb(0.66, 0.67, 0.68), texto_cor=PRETA),
    estilos={"padrao": estilo("saia_numerada", 0.019, (0.82, 0.82, 0.84), (0.86, 0.86, 0.88), saia=(0.015, 0.015, 0.015),
                              numeros=(0.85, 0.85, 0.85), traco=(0.02, 0.02, 0.02), escala="nenhuma", metal=True,
                              tampa_metal=True)},
    cantos="preto",
    fileiras=[ROW(0.45, 0.0, 1.0, T("POWER", x=0.05), T("STANDBY", x=0.11), JW((0.9, 0.05, 0.02), x=0.165, raio_m=0.006),
                  K("presenca", "PRESENCE", x=0.25), K("grave", "BASS", x=0.33), K("medio", "MIDDLE", x=0.41),
                  K("agudo", "TREBLE", x=0.49), K("volume", "MASTER", x=0.57), K("ganho", "GAIN", x=0.65),
                  J(x=0.75, z=0.7), J(x=0.75, z=0.25), J(x=0.83, z=0.7), J(x=0.83, z=0.25),
                  TX("BASS", x=0.75, z=0.95), TX("TREBLE", x=0.83, z=0.95), M("BLACK COUNTRY", 0.008, x=0.93, z=0.45),
                  letra_m=0.0055, rotulo_pos="baixo")],
)

# ------------------------------------------------------------------ Iron 120 (Laney Ironheart)
FICHAS["iron120"] = dict(
    inspiracao=("Laney Ironheart: cabeçote de 680 × 275 × 290 mm em tolex preto, grade de metal expandido preta em losangos "
                "com a marca no meio e painel preto embaixo com letras brancas: entrada, chaves PRE-BOOST e CLEAN/RHYTHM, "
                "a chave de canal com LEDs, duas fileiras de botões pretos (PRE-BOOST vermelho, LEAD GAIN, BASS, MIDDLE, "
                "TREBLE e VOLUME em cima, que é a que toca; CLEAN VOLUME, RHYTHM GAIN, BASS, MIDDLE, TREBLE e VOLUME "
                "embaixo), DYNAMICS, TONE, REVERB e WATTS maiores e as duas joias vermelhas com as alavancas."),
    mm=(680, 275, 290), couro=TOLEX_PRETO,
    fachada={"tipo": "losango", "cor": [0.02, 0.02, 0.022], "furo_mm": 7.0, "passo_mm": 8.5},
    logo={"altura_m": 0.024, "altura_rel": 0.52},
    painel=dict(posicao="baixo", altura_m=0.095, cor=rgb(0.012, 0.012, 0.013), material="pintado", texto_cor=BRANCO),
    estilos={"padrao": dict(LISO_PRETO, diametro_m=0.018), "verm": estilo("liso", 0.018, (0.6, 0.03, 0.02), traco=(0.9, 0.9, 0.9), escala="nenhuma"),
             "grande": dict(LISO_PRETO, diametro_m=0.024, escala="tracos")},
    cantos="preto", parafusos_painel=False,
    fileiras=[
        ROW(0.5, 0.0, 1.0, J("INPUT", x=0.03), MT("BOOST", x=0.07, z=0.72), MT("CLEAN/RHY", x=0.07, z=0.3),
            LC("", x=0.1, z=0.72), LC("", x=0.1, z=0.5), LC("", x=0.1, z=0.3), letra_m=0.0042, rotulo_pos="baixo"),
        ROW(0.73, 0.13, 0.55, F("PRE-BOOST", estilo="verm"), K("ganho", "LEAD GAIN"), K("grave", "BASS"), K("medio", "MIDDLE"),
            K("agudo", "TREBLE"), K("volume", "VOLUME"), letra_m=0.0042, rotulo_pos="cima"),
        ROW(0.25, 0.13, 0.55, F("CLEAN VOL"), F("RHYTHM GAIN"), F("BASS"), F("MIDDLE"), F("TREBLE"), F("VOLUME"),
            letra_m=0.0042, rotulo_pos="baixo"),
        ROW(0.45, 0.58, 0.82, F("DYNAMICS", estilo="grande"), K("presenca", "TONE", estilo="grande"), F("REVERB", estilo="grande"),
            F("WATTS", estilo="grande"), letra_m=0.0042, rotulo_pos="baixo"),
        ROW(0.5, 0.0, 1.0, JW((0.9, 0.05, 0.02), x=0.87, z=0.7), T("STANDBY", x=0.87, z=0.25), JW((0.9, 0.05, 0.02), x=0.93, z=0.7),
            T("POWER", x=0.93, z=0.25), letra_m=0.0042, rotulo_pos="baixo"),
    ],
    notas="O Ironheart não tem presença no painel: a presença do Nexus ficou no TONE.",
)

# ------------------------------------------------------------------ Fire 100 (ENGL Fireball)
ENGL_TELA = {"tipo": "tela", "cor_a": [0.012, 0.012, 0.013], "cor_b": [0.05, 0.05, 0.055], "trama_mm": 4.0}
FICHAS["fire100"] = dict(
    inspiracao=("ENGL Fireball 100: cabeçote de 710 × 270 × 270 mm em tolex preto com cantoneiras cromadas, grade de aço preta "
                "em trama de cesto com a marca no meio e painel preto metálico embaixo com letras prateadas: entrada, nove "
                "botões pretos pequenos (CLEAN GAIN, LEAD GAIN, BASS, MIDDLE, TREBLE, LEAD VOLUME, MASTER A, MASTER B, "
                "PRESENCE) com as teclas BRIGHT, BOTTOM, MID BOOST e CLEAN/LEAD e seus LEDs, os LEDs das válvulas, o nome "
                "em letra cursiva e as gangorras STAND BY (preta) e POWER (vermelha acesa)."),
    mm=(710, 270, 270), couro=TOLEX_PRETO, fachada=ENGL_TELA, logo={"altura_m": 0.026, "altura_rel": 0.52},
    painel=dict(posicao="baixo", altura_m=0.085, cor=rgb(0.03, 0.03, 0.032), material="pintado", texto_cor=BRANCO),
    estilos={"padrao": LISO_PRETO}, cantos="cromo",
    fileiras=[ROW(0.45, 0.0, 1.0, J("INPUT", x=0.035), F("CLEAN GAIN", x=0.095), K("ganho", "LEAD GAIN", x=0.15),
                  P("BRIGHT", x=0.195, tamanho_m=0.007), P("BOTTOM", x=0.23, tamanho_m=0.007),
                  K("grave", "BASS", x=0.28), K("medio", "MIDDLE", x=0.335), K("agudo", "TREBLE", x=0.39),
                  K("volume", "LEAD VOLUME", x=0.445), P("MID BOOST", x=0.49, tamanho_m=0.007),
                  P("CLEAN/LEAD", x=0.53, tamanho_m=0.007), LC("", x=0.53, z=0.88), LC("", x=0.555, z=0.88),
                  F("MASTER A", x=0.585), F("MASTER B", x=0.64), K("presenca", "PRESENCE", x=0.695),
                  L("V1", (0.9, 0.05, 0.02), aceso=True, x=0.74, z=0.5), L("V2", (0.9, 0.05, 0.02), aceso=True, x=0.76, z=0.5),
                  L("V3", (0.9, 0.05, 0.02), aceso=True, x=0.78, z=0.5), L("V4", (0.9, 0.05, 0.02), aceso=True, x=0.8, z=0.5),
                  M("Fire 100", 0.014, x=0.86, z=0.5, fonte="script", cor=(0.8, 0.8, 0.82)),
                  RB("STAND BY", x=0.92), R("POWER", x=0.965), letra_m=0.0048, rotulo_pos="baixo")],
)

# ------------------------------------------------------------------ Feral 120 (ENGL Savage)
SAVAGE = estilo("marshall", 0.017, (0.015, 0.015, 0.015), (0.03, 0.03, 0.03), traco=(0.9, 0.9, 0.9), escala="tracos")
FICHAS["feral120"] = dict(
    inspiracao=("ENGL Savage 120 Mark II: cabeçote de 710 × 270 × 290 mm em tolex preto com cantoneiras cromadas, frente de "
                "cinco barras horizontais pretas com a luz azul das válvulas atrás e a marca no meio, painel antracite "
                "embaixo com letras brancas: vinte botões pretos de saia em duas fileiras (CLEAN GAIN, CRUNCH I GAIN, BASS, "
                "MID LEAN, MID BOLD, TREBLE, os VOLUMEs e PRESENCE A e B em cima, que é a que toca; CRUNCH II e LEAD GAIN, "
                "BASS, MIDDLE, TREBLE, TONE BALANCE, VOLUMEs e MASTER A e B embaixo), teclinhas com LED, a entrada e as "
                "gangorras STAND BY (preta) e POWER (azul acesa)."),
    mm=(710, 270, 290), couro=TOLEX_PRETO,
    fachada={"tipo": "barras", "cor": [0.015, 0.015, 0.016], "barras": 5, "luz": [0.1, 0.3, 1.0]},
    logo={"altura_m": 0.024, "altura_rel": 0.5},
    painel=dict(posicao="baixo", altura_m=0.1, cor=rgb(0.035, 0.036, 0.04), material="pintado", texto_cor=BRANCO),
    estilos={"padrao": SAVAGE}, cantos="cromo",
    fileiras=[
        ROW(0.72, 0.06, 0.84, F("CLEAN GAIN"), K("ganho", "CRUNCH GAIN"), K("grave", "BASS"), K("medio", "MID LEAN"),
            F("MID BOLD"), K("agudo", "TREBLE"), F("CLEAN VOL."), K("volume", "CRUNCH VOL."), K("presenca", "PRES. A"),
            F("PRES. B"), letra_m=0.0038, rotulo_pos="cima"),
        ROW(0.26, 0.06, 0.84, F("CRUNCH II"), F("LEAD GAIN"), F("BASS"), F("MIDDLE"), F("TREBLE"), F("TONE BAL."),
            F("CRUNCH II VOL."), F("LEAD VOL."), F("MASTER A"), F("MASTER B"), letra_m=0.0038, rotulo_pos="baixo"),
        ROW(0.5, 0.0, 1.0, J("INPUT", x=0.028, z=0.25), P(None, x=0.028, z=0.72, tamanho_m=0.006),
            LC("CLEAN", x=0.86, z=0.85), LC("CRUNCH", x=0.885, z=0.85), LC("LEAD", x=0.91, z=0.85),
            RB("STAND BY", x=0.88, z=0.4), R("POWER", (0.1, 0.3, 1.0), x=0.94, z=0.4), letra_m=0.0036, rotulo_pos="baixo"),
    ],
)

# ------------------------------------------------------------------ Steel 20 (ENGL Ironball, caixinha)
FICHAS["steel20"] = dict(
    inspiracao=("ENGL Ironball: caixinha de metal preta texturizada de 340 × 140 × 220 mm com alça cromada, grade de metal "
                "preta em losango nos dois terços de cima com as válvulas acesas de vermelho e a marca, e faixa de alumínio "
                "escovado prata embaixo com letras pretas: entrada, oito botões pretos (CLEAN GAIN, LEAD GAIN, BASS, MIDDLE, "
                "TREBLE, LEAD PRESENCE, LEAD VOLUME, MASTER), oito minichaves com LED em duas fileiras e as alavancas "
                "STAND BY e POWER."),
    formato="caixinha", mm=(340, 140, 220),
    couro={"cor": [0.02, 0.02, 0.022], "metal": True, "aspereza": 0.6},
    fachada={"tipo": "losango", "cor": [0.02, 0.02, 0.022], "furo_mm": 5.0, "passo_mm": 6.5},
    logo={"altura_m": 0.012, "altura_rel": 0.5},
    alca="barra",
    painel=dict(posicao="baixo", altura_m=0.044, cor=rgb(0.62, 0.63, 0.64), texto_cor=PRETA),
    estilos={"padrao": dict(LISO_PRETO, diametro_m=0.0125)}, parafusos_painel=False,
    fileiras=[ROW(0.45, 0.0, 1.0, J("INPUT", x=0.04), F("CLEAN GAIN", x=0.1), K("ganho", "LEAD GAIN", x=0.155),
                  K("grave", "BASS", x=0.21), K("medio", "MIDDLE", x=0.265), K("agudo", "TREBLE", x=0.32),
                  K("presenca", "PRESENCE", x=0.375), K("volume", "LEAD VOL", x=0.43), F("MASTER", x=0.485),
                  letra_m=0.0032, rotulo_pos="baixo"),
              ROW(0.72, 0.53, 0.83, MT("STORE"), MT("M.V.B."), MT("REVERB"), MT("DELAY"), letra_m=0.003, rotulo_pos="baixo"),
              ROW(0.25, 0.53, 0.83, MT("BOOST"), MT("CLEAN/LEAD"), MT("FX LOOP"), MT("GATE"), letra_m=0.003, rotulo_pos="baixo"),
              ROW(0.45, 0.0, 1.0, T("STAND BY", x=0.88), T("POWER", x=0.94), letra_m=0.0032, rotulo_pos="baixo")],
)

# ------------------------------------------------------------------ Herb 180 (Diezel Herbert)
DIEZEL = estilo("facetado", 0.019, (0.012, 0.012, 0.012), (0.75, 0.75, 0.77), traco=(0.9, 0.9, 0.9), escala="nenhuma", tampa_metal=True)
REBAIXO = (0.45, 0.46, 0.47)
FICHAS["herb180"] = dict(
    inspiracao=("Diezel Herbert: cabeçote de 740 × 270 × 280 mm em tolex preto com cantos cromados, grade de metal preta "
                "recortada em garras com a marca no meio e a faixa de alumínio escovado prata embaixo com cada grupo num "
                "rebaixo e letras pretas: entrada, CHANNEL 1 (VOLUME, TREBLE, MIDDLE, BASS), CHANNEL 2 e 3 (GAIN, VOLUME, "
                "TREBLE, MIDDLE, BASS; o 2 é o que toca), MIDCUT (INTENSITY, LEVEL) e MASTER (VOLUME 2, PRESENCE, DEEP, "
                "VOLUME 1), LEDs, teclinhas e as gangorras pretas STANDBY e POWER, em vinte botões pretos facetados."),
    mm=(740, 270, 280), couro=TOLEX_PRETO,
    fachada={"tipo": "garras", "cor": [0.015, 0.015, 0.016], "passo_mm": 16.0},
    logo={"altura_m": 0.024, "altura_rel": 0.5},
    painel=dict(posicao="baixo", altura_m=0.088, cor=rgb(0.64, 0.65, 0.66), texto_cor=PRETA),
    estilos={"padrao": DIEZEL}, cantos="cromo",
    fileiras=[ROW(0.4, 0.0, 1.0, J("INPUT", x=0.03),
                  F("VOLUME", x=0.07), F("TREBLE", x=0.105), F("MIDDLE", x=0.14), F("BASS", x=0.175),
                  K("ganho", "GAIN", x=0.225), F("VOLUME", x=0.26), K("agudo", "TREBLE", x=0.295), K("medio", "MIDDLE", x=0.33),
                  K("grave", "BASS", x=0.365),
                  F("GAIN", x=0.415), F("VOLUME", x=0.45), F("TREBLE", x=0.485), F("MIDDLE", x=0.52), F("BASS", x=0.555),
                  F("INTENSITY", x=0.605), F("LEVEL", x=0.64),
                  F("VOLUME 2", x=0.69), K("presenca", "PRESENCE", x=0.725), F("DEEP", x=0.76), K("volume", "VOLUME 1", x=0.795),
                  LC("", x=0.12, z=0.9), LC("", x=0.29, z=0.9), LC("", x=0.485, z=0.9),
                  P("LOOP", x=0.84, z=0.6, tamanho_m=0.0065), P("MUTE", x=0.84, z=0.25, tamanho_m=0.0065),
                  RB("STANDBY", x=0.9), RB("POWER", x=0.95), letra_m=0.0045, rotulo_pos="baixo"),
              ROW(0.9, 0.0, 1.0, TX("CH 1", x=0.07), TX("CH 2", x=0.225), TX("CH 3", x=0.415), TX("MIDCUT", x=0.622),
                  TX("MASTER", x=0.742), letra_m=0.0045)],
    riscos=[dict(RISCO(a, 0.42, b, 0.42, 0.06, cor=REBAIXO)) for a, b in ((0.055, 0.19), (0.21, 0.38), (0.4, 0.57), (0.59, 0.655), (0.675, 0.81))],
)

# ------------------------------------------------------------------ Bliss 100 (Bogner Ecstasy)
FICHAS["bliss100"] = dict(
    inspiracao=("Bogner Ecstasy: cabeçote de cerca de 711 × 279 × 260 mm em tolex preto, tela bege sal-e-pimenta com a marca no "
                "meio e painel cromado embaixo com letras pretas: canal 1 (VOLUME, TREBLE, MIDDLE, BASS), canais 2 e 3 "
                "(GAIN, VOLUME, TREBLE, MIDDLE, BASS; o 2 é o que toca), PRESENCE, EXCURSION e MASTER 1 e 2, as minichaves "
                "de cada canal e os LEDs, em vinte botões pretos."),
    mm=(711, 279, 260), couro=TOLEX_PRETO,
    fachada=tela((0.62, 0.56, 0.44), (0.12, 0.1, 0.08), 1.6),
    logo={"altura_m": 0.024, "altura_rel": 0.52},
    painel=dict(posicao="baixo", altura_m=0.095, cor=rgb(0.86, 0.86, 0.88), material="cromo", texto_cor=PRETA),
    estilos={"padrao": estilo("pointer", 0.017, (0.015, 0.015, 0.015), traco=(0.9, 0.9, 0.9), escala="tracos")},
    cantos="preto",
    fileiras=[ROW(0.4, 0.04, 0.96, F("VOLUME"), F("TREBLE"), F("MIDDLE"), F("BASS"), GAP(0.5),
                  K("ganho", "GAIN"), F("VOLUME"), K("agudo", "TREBLE"), K("medio", "MIDDLE"), K("grave", "BASS"), GAP(0.5),
                  F("GAIN"), F("VOLUME"), F("TREBLE"), F("MIDDLE"), F("BASS"), GAP(0.5),
                  K("presenca", "PRES. 1"), F("PRES. 2"), F("EXC. 1"), F("EXC. 2"), K("volume", "MASTER 1"), F("MASTER 2"),
                  letra_m=0.0038, rotulo_pos="baixo"),
              ROW(0.86, 0.04, 0.96, LC(""), MT("PLEXI"), MT("PRE-EQ"), GAP(1.5), LC(""), MT("BOOST"), MT("STRUCT."), GAP(2),
                  LC(""), MT("BOOST"), MT("STRUCT."), GAP(2), MT("VARIAC"), GAP(4), letra_m=0.0034, rotulo_pos="direita")],
    notas="Confiança baixa nas medidas (largura estimada) e na posição exata das minichaves do Ecstasy.",
)

# ------------------------------------------------------------------ Goddess 80 (Bogner Shiva)
FICHAS["goddess80"] = dict(
    inspiracao=("Bogner Shiva: cabeçote de 622 × 292 × 267 mm em tolex preto com estampa em relevo, grade de metal preta com "
                "aberturas inclinadas em fileiras e a marca no meio, e painel dourado-champanhe fosco embaixo com letras "
                "pretas: alavanca ST.BY/PLAY e luz piloto verde à esquerda, onze botões pretos bico-de-galinha (PRESENCE, "
                "M.VOL, B.VOL, e do canal 2 VOLUME, TREBLE, MIDDLE, BASS e GAIN, que é o que toca; do canal 1 TREBLE, BASS "
                "e VOLUME), minichaves SHIFT e BRIGHT, LEDs e a entrada à direita."),
    mm=(622, 292, 267), couro={"cor": [0.008, 0.008, 0.009], "grao_mm": 4.0},
    fachada={"tipo": "inclinadas", "cor": [0.02, 0.02, 0.022], "passo_mm": 12.0},
    logo={"altura_m": 0.024, "altura_rel": 0.52},
    painel=dict(posicao="baixo", altura_m=0.085, cor=rgb(0.6, 0.5, 0.32), material="pintado", texto_cor=PRETA),
    estilos={"padrao": estilo("chicken", 0.02, (0.015, 0.015, 0.015), traco=(0.9, 0.9, 0.9), escala="tracos")},
    cantos="preto",
    fileiras=[ROW(0.42, 0.0, 1.0, T("ST.BY / PLAY", x=0.04), L(None, (0.1, 0.8, 0.1), aceso=True, x=0.04, z=0.88),
                  K("presenca", "PRESENCE", x=0.1), K("volume", "M.VOL", x=0.16), F("B.VOL", x=0.22),
                  F("VOLUME", x=0.29), K("agudo", "TREBLE", x=0.35), K("medio", "MIDDLE", x=0.41), K("grave", "BASS", x=0.47),
                  K("ganho", "GAIN", x=0.53), MT("SHIFT", x=0.575, z=0.72), MT("BRIGHT", x=0.575, z=0.25),
                  LC("", x=0.6, z=0.9), F("TREBLE", x=0.65), F("BASS", x=0.71), F("VOLUME", x=0.77), MT("BRIGHT", x=0.815),
                  LC("", x=0.84, z=0.9), M("GODDESS 80", 0.01, x=0.89, z=0.55), J("INPUT", x=0.955),
                  letra_m=0.0048, rotulo_pos="baixo")],
)

# ------------------------------------------------------------------ Dirty 40 (Friedman Dirty Shirley)
FICHAS["dirty40"] = dict(
    inspiracao=("Friedman Dirty Shirley: cabeçote de 610 × 254 × 216 mm em tolex preto com friso branco fino, tela de trama "
                "preta e prata em toda a frente com a marca à esquerda, e a placa prata pequena no canto de baixo à direita "
                "com letras pretas: alavancas OFF/ON e STANDBY e a luz INDICATOR laranja, seis botões pretos de traço "
                "branco (PRESENCE, BASS, MIDDLE, TREBLE, MASTER, GAIN), as entradas H e L e uma minichave."),
    mm=(610, 254, 216), couro=TOLEX_PRETO,
    fachada=tela((0.03, 0.03, 0.032), (0.4, 0.4, 0.42), 2.2, friso=(0.85, 0.85, 0.82)),
    logo={"lado_rel": -0.28, "altura_m": 0.016, "altura_rel": 0.7},
    painel=dict(posicao="sobre", altura_m=0.062, largura_m=0.4, x_m=0.075, cor=rgb(0.66, 0.67, 0.68), texto_cor=PRETA),
    estilos={"padrao": dict(LISO_PRETO, diametro_m=0.02, escala="tracos")}, cantos="preto",
    fileiras=[ROW(0.45, 0.0, 1.0, T("OFF/ON", x=0.06), T("STANDBY", x=0.13), JW((0.95, 0.45, 0.05), x=0.19, raio_m=0.0055),
                  K("presenca", "PRESENCE", x=0.27), K("grave", "BASS", x=0.36), K("medio", "MIDDLE", x=0.45),
                  K("agudo", "TREBLE", x=0.54), K("volume", "MASTER", x=0.63), K("ganho", "GAIN", x=0.72),
                  J("H", x=0.8), J("L", x=0.87), MT(None, x=0.94), letra_m=0.0048, rotulo_pos="baixo")],
)

# ------------------------------------------------------------------ Special 100 (Dumble ODS)
FICHAS["special100"] = dict(
    inspiracao=("Dumble Overdrive Special: cabeçote de cerca de 650 × 280 × 250 mm em tolex preto, tela preta na parte de "
                "cima com a marca no meio e painel preto embaixo com friso prata e letras brancas: entradas FET e NOR, "
                "VOLUME, as minichaves BRIGHT, DEEP e ROCK/JAZZ, TREBLE, MIDDLE, BASS, o OVERDRIVE (LEVEL e RATIO em "
                "botões cromados), REVERB, TREMOLO, MASTER e PRESENCE em botões pretos estriados, a luz piloto laranja e as "
                "alavancas OPERATE/STANDBY e ON/OFF."),
    mm=(650, 280, 250), couro=TOLEX_PRETO,
    fachada=tela((0.008, 0.008, 0.008), (0.03, 0.03, 0.03), 1.8),
    logo={"altura_m": 0.024, "altura_rel": 0.52},
    painel=dict(posicao="baixo", altura_m=0.085, cor=rgb(0.01, 0.01, 0.011), material="pintado", texto_cor=BRANCO,
                friso=(0.75, 0.75, 0.77)),
    estilos={"padrao": estilo("cromado", 0.019, (0.015, 0.015, 0.015), (0.03, 0.03, 0.03), traco=(0.9, 0.9, 0.9), escala="tracos"),
             "cromo": estilo("cromado", 0.019, (0.8, 0.8, 0.82), (0.88, 0.88, 0.9), traco=(0.02, 0.02, 0.02), escala="tracos", metal=True)},
    cantos="preto", parafusos_painel=False,
    fileiras=[ROW(0.42, 0.0, 1.0, J("FET", x=0.035), J("NOR", x=0.075), F("VOLUME", x=0.125),
                  MT("BRIGHT", x=0.165, z=0.75), MT("DEEP", x=0.165, z=0.42), MT("R/J", x=0.165, z=0.1),
                  K("agudo", "TREBLE", x=0.21), K("medio", "MIDDLE", x=0.26), K("grave", "BASS", x=0.31),
                  F("LEVEL", estilo="cromo", x=0.37), K("ganho", "RATIO", estilo="cromo", x=0.42),
                  F("SEND", x=0.48), F("RETURN", x=0.53), F("RATE", x=0.59), F("MOD.", x=0.64),
                  K("volume", "MASTER", x=0.7), K("presenca", "PRESENCE", x=0.75),
                  LC("", x=0.395, z=0.92), LC("", x=0.42, z=0.92), JW((0.95, 0.45, 0.05), x=0.81, raio_m=0.006),
                  T("OPERATE", x=0.87), T("ON", x=0.93), letra_m=0.005, rotulo_pos="baixo"),
              ROW(0.93, 0.0, 1.0, TX("PREAMP", x=0.24), TX("OVERDRIVE", x=0.395), TX("REVERB", x=0.505), TX("TREMOLO", x=0.615),
                  letra_m=0.0045)],
    notas="Dumble não publica medidas: tamanho estimado pela foto. Reverb e tremolo não existem em todas as unidades.",
)
