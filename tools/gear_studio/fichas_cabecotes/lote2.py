# Fichas do lote 2: Rectifier, Soldano, 5150, Plexi, Jubilee, Rockerverb, BE-100, Powerball.
from cabecotes_fiel import *  # noqa: F401,F403

TOLEX_PRETO = {"cor": [0.006, 0.006, 0.006], "grao_mm": 2.6}
BRANCO = rgb(0.85, 0.85, 0.85)
PRETA = rgb(0.012, 0.012, 0.012)
CROMADO = estilo("cromado", 0.021, (0.8, 0.8, 0.82), (0.88, 0.88, 0.9), traco=(0.02, 0.02, 0.02), escala="numeros", metal=True)
FICHAS = {}


def _recto(nome_modelo, triplo):
    canal_um = [K("volume", "MASTER"), K("presenca", "PRESENCE"), K("grave", "BASS"), K("medio", "MID"),
                K("agudo", "TREBLE"), K("ganho", "GAIN")]
    canal_dois = [F("MASTER"), F("PRESENCE"), F("BASS"), F("MID"), F("TREBLE"), F("GAIN")]
    return dict(
        mm=(648, 254, 248),
        couro=TOLEX_PRETO,
        fachada={"tipo": "xadrez", "cor": [0.78, 0.78, 0.8], "passo_mm": 11.0,
                 "janela": {"largura_rel": 0.3, "altura_rel": 0.5, "z_rel": 0.55}},
        logo={"lado_rel": 0.0, "altura_m": 0.012, "altura_rel": 0.14},
        painel=dict(posicao="baixo", altura_m=0.092, cor=rgb(0.01, 0.01, 0.011), material="pintado", texto_cor=BRANCO),
        estilos={"padrao": dict(CROMADO, diametro_m=0.019, escala="tracos")},
        cantos="preto",
        parafusos_painel=False,
        fileiras=[
            ROW(0.5, 0.0, 1.0, T("BOLD", x=0.035, z=0.66), T("SPONGY", x=0.035, z=0.26, posicao="baixo"),
                T("ON", x=0.075, z=0.66), T("STANDBY", x=0.075, z=0.26, posicao="baixo"),
                JW((0.9, 0.05, 0.02), x=0.115, z=0.5, raio_m=0.0065),
                M(nome_modelo, 0.017, x=0.2, z=0.55, fonte="mesa", cor=(0.8, 0.72, 0.52)),
                TX("SOLO HEAD" if not triplo else "SOLO HEAD", x=0.2, z=0.25, letra_m=0.0048),
                F("LOOP ACTIVE MASTER", x=0.3, z=0.5), letra_m=0.0048, rotulo_pos="baixo"),
            ROW(0.72, 0.37, 0.83, *canal_um, letra_m=0.0048, rotulo_pos="cima"),
            ROW(0.24, 0.37, 0.83, *canal_dois, letra_m=0.0048, rotulo_pos="baixo"),
            ROW(0.5, 0.0, 1.0, L("CLEAN", (0.1, 0.8, 0.1), x=0.865, z=0.75), L("LOOP", (0.9, 0.4, 0.02), x=0.865, z=0.45),
                L("", (0.9, 0.05, 0.02), x=0.865, z=0.15),
                J("FT. SW.", x=0.91, z=0.5), J("INPUT", x=0.96, z=0.5), letra_m=0.0045, rotulo_pos="baixo"),
        ],
    )


FICHAS["recto_dual"] = dict(
    _recto("Dual Rectifier", False),
    inspiracao=("Mesa/Boogie Dual Rectifier: cabeçote de 648 × 254 × 248 mm em tolex preto com cantoneiras pretas, "
                "chapa xadrez de alumínio claro na frente com a janela de fendas no meio e a marca embaixo dela, e "
                "painel preto com letras brancas: alavancas BOLD/SPONGY e ON/STANDBY, joia vermelha, o nome em script "
                "creme, LOOP ACTIVE MASTER e duas fileiras de seis botões cromados recartilhados (MASTER, PRESENCE, "
                "BASS, MID, TREBLE, GAIN; a de cima é a que toca), LEDs e as entradas FT. SW. e INPUT à direita."),
)
FICHAS["recto_triple"] = dict(
    _recto("Triple Rectifier", True),
    inspiracao=("Mesa/Boogie Triple Rectifier: igual ao Dual por fora (648 × 254 × 248 mm, tolex preto, chapa xadrez "
                "clara com janela de fendas, painel preto) com o nome Triple Rectifier em script creme; alavancas "
                "BOLD/SPONGY e STANDBY, joia vermelha, LOOP ACTIVE MASTER e duas fileiras de seis botões cromados "
                "(MASTER, PRESENCE, BASS, MID, TREBLE, GAIN; a de cima toca), LEDs de canal e as entradas FT. SW. e INPUT."),
)

# ------------------------------------------------------------------ SL 100 (Soldano SLO-100)
SOLDANO = estilo("saia_numerada", 0.019, (0.78, 0.78, 0.8), (0.86, 0.86, 0.88), saia=(0.8, 0.8, 0.82),
                 numeros=(0.02, 0.02, 0.02), traco=(0.02, 0.02, 0.02), escala="nenhuma", metal=True, tampa_metal=True)
FICHAS["sl100"] = dict(
    inspiracao=("Soldano SLO-100: cabeçote de 635 × 260 × 222 mm em tolex preto, grade de metal expandido preta em "
                "losangos na metade de cima com a marca no meio e painel cromado espelhado embaixo com letras pretas: "
                "entrada e as minichaves OVERDRIVE/NORMAL, BRIGHT e CRUNCH à esquerda, oito botões de alumínio com "
                "saia numerada (PREAMP NORMAL e OVERDRIVE, BASS, MIDDLE, TREBLE, MASTER NORMAL e OVERDRIVE, PRESENCE) e, "
                "à direita, as minichaves STANDBY e POWER e a joia azul."),
    mm=(635, 260, 222),
    couro=TOLEX_PRETO,
    fachada={"tipo": "losango", "cor": [0.02, 0.02, 0.022], "furo_mm": 6.5, "passo_mm": 8.0},
    logo={"lado_rel": 0.0, "altura_m": 0.026, "altura_rel": 0.5},
    painel=dict(posicao="baixo", altura_m=0.082, cor=rgb(0.9, 0.9, 0.92), material="cromo", texto_cor=PRETA),
    estilos={"padrao": SOLDANO},
    cantos="preto",
    fileiras=[
        ROW(0.45, 0.0, 1.0, J("INPUT", x=0.04), MT("OD / NORMAL", x=0.09), MT("BRIGHT", x=0.13), MT("CRUNCH", x=0.17),
            LC("", x=0.09, z=0.9), LC("", x=0.17, z=0.9),
            F("NORMAL", x=0.24), K("ganho", "OVERDRIVE", x=0.31), K("grave", "BASS", x=0.38), K("medio", "MIDDLE", x=0.45),
            K("agudo", "TREBLE", x=0.52), F("NORMAL", x=0.59), K("volume", "OVERDRIVE", x=0.66),
            K("presenca", "PRESENCE", x=0.73),
            M("SL 100", 0.014, x=0.82, z=0.62), MT("STANDBY", x=0.88), MT("POWER", x=0.92),
            JW((0.15, 0.35, 1.0), x=0.965, raio_m=0.0065),
            letra_m=0.0052, rotulo_pos="baixo"),
        ROW(0.92, 0.0, 1.0, TX("PREAMP", x=0.275), TX("MASTER", x=0.625), letra_m=0.0048),
    ],
    notas="O SLO-100 não tem LED de canal: os dois LEDs sobre as minichaves são para a tela mostrar Normal e Overdrive.",
)

# ------------------------------------------------------------------ Block 5150 (Peavey 5150)
PONTEIRO = estilo("pointer", 0.021, (0.015, 0.015, 0.015), traco=(0.9, 0.9, 0.9), escala="numeros")
FICHAS["block5150"] = dict(
    inspiracao=("Peavey 5150: cabeçote de 673 × 292 × 241 mm em tolex preto, grade de metal perfurado preta com o "
                "5150 grande no meio e a marca pequena embaixo à direita, e painel prateado embaixo com letras pretas: "
                "entradas HIGH e NORMAL GAIN e a tecla de canal à esquerda com os LEDs verde (RHYTHM) e vermelho (LEAD), "
                "nove botões pretos de ponteiro (RHYTHM PRE GAIN, LEAD PRE GAIN, LOW, MID, HIGH, RHYTHM POST GAIN, "
                "LEAD POST GAIN, RESONANCE, PRESENCE) com as teclas BRIGHT e CRUNCH, e as gangorras STANDBY e POWER."),
    mm=(673, 292, 241),
    couro=TOLEX_PRETO,
    fachada={"tipo": "grade", "cor": [0.025, 0.025, 0.027], "furo_mm": 3.0, "passo_mm": 4.6},
    selo_grade={"texto": "5150", "altura_m": 0.07, "x_rel": 0.0, "z_rel": 0.6, "cor": [0.7, 0.7, 0.72]},
    logo={"lado_rel": 0.34, "altura_m": 0.011, "altura_rel": 0.14},
    painel=dict(posicao="baixo", altura_m=0.085, cor=rgb(0.6, 0.61, 0.62), texto_cor=PRETA),
    estilos={"padrao": PONTEIRO},
    cantos="preto",
    fileiras=[
        ROW(0.42, 0.0, 1.0, J("HIGH", x=0.035), J("NORMAL", x=0.08), P("CHANNEL", x=0.125),
            LC("RHYTHM", x=0.16, z=0.72, rotulo_pos="cima"), LC("LEAD", x=0.16, z=0.25),
            F("RHYTHM PRE", x=0.215), P("BRIGHT", x=0.258), K("ganho", "LEAD PRE", x=0.3), P("CRUNCH", x=0.342),
            K("grave", "LOW", x=0.39), K("medio", "MID", x=0.45), K("agudo", "HIGH", x=0.51),
            F("RHYTHM POST", x=0.575), K("volume", "LEAD POST", x=0.64), F("RESONANCE", x=0.705),
            K("presenca", "PRESENCE", x=0.77),
            L("STANDBY", (0.9, 0.05, 0.02), x=0.835, z=0.75), L("POWER", (0.9, 0.05, 0.02), aceso=True, x=0.835, z=0.25),
            RB("STANDBY", x=0.89), R("POWER", x=0.945),
            letra_m=0.0048, rotulo_pos="baixo"),
    ],
)

# ------------------------------------------------------------------ Block 5150 III (EVH)
EVH = estilo("chicken", 0.02, (0.88, 0.88, 0.86), traco=(0.02, 0.02, 0.02), escala="tracos")


def _evh():
    itens = [J("INSERT", x=0.035)]
    nomes = [("GAIN", "ganho"), ("LOW", "grave"), ("MID", "medio"), ("HIGH", "agudo"), ("VOLUME", "volume")]
    for c, base in enumerate((0.08, 0.35, 0.62)):
        for i, (nome, kid) in enumerate(nomes):
            it = K(kid, nome) if c == 0 else F(nome)
            it.update(x=base + i * 0.044)
            itens.append(it)
        itens.append(P("SELECT", x=base + 5 * 0.044 + 0.01, z=0.3, tamanho_m=0.009))
        itens.append(LC("", x=base + 5 * 0.044 + 0.01, z=0.72))
        itens.append(TX(["ONE", "TWO", "THREE"][c], x=base + 2 * 0.044, z=0.94))
    for i in range(3):
        it = K("presenca", "PRESENCE") if i == 0 else F("PRESENCE")
        it.update(x=0.88 + i * 0.036)
        itens.append(it)
    itens.append(JW((0.9, 0.05, 0.02), x=0.975, raio_m=0.008))
    return itens


FICHAS["block5150iii"] = dict(
    inspiracao=("EVH 5150 III: cabeçote de 755 × 260 × 293 mm em vinil preto, grade de chapa perfurada preta com a "
                "marca oval no meio e o selo 5150 III embaixo à direita, e faixa de painel branca embaixo com letras "
                "pretas: entrada INSERT, os três canais ONE, TWO e THREE com GAIN, LOW, MID, HIGH e VOLUME (o ONE é o "
                "que toca), tecla SELECT e LED de cada canal, três PRESENCE e a joia vermelha grande, em 18 botões "
                "brancos de ponteiro."),
    mm=(755, 260, 293),
    couro=TOLEX_PRETO,
    fachada={"tipo": "grade", "cor": [0.02, 0.02, 0.022], "furo_mm": 3.2, "passo_mm": 4.8},
    selo_grade={"texto": "5150 III", "altura_m": 0.026, "margem_m": 0.04},
    logo={"lado_rel": 0.0, "altura_m": 0.024, "altura_rel": 0.55},
    painel=dict(posicao="baixo", altura_m=0.07, cor=rgb(0.82, 0.82, 0.8), material="pintado", texto_cor=PRETA),
    estilos={"padrao": EVH},
    cantos="preto",
    parafusos_painel=False,
    fileiras=[ROW(0.48, 0.0, 1.0, *_evh(), letra_m=0.005, rotulo_pos="baixo")],
    notas="A grade do original tem as listras EVH em barras por cima; aqui ficou só a grade perfurada preta.",
)

# ------------------------------------------------------------------ Brit Plexi (Super Lead 1959)
MARSHALL = estilo("marshall", 0.026, (0.018, 0.018, 0.018), (0.56, 0.39, 0.17))
FICHAS["britplexi"] = dict(
    inspiracao=("Marshall Super Lead 1959 (plexi): cabeçote de 740 × 305 × 210 mm em vinil preto, frente toda em vinil "
                "preto com friso branco e a marca no meio, painel de plexi dourado embaixo com letras pretas: chaves "
                "de alavanca MAINS e STANDBY e a luz piloto vermelha quadrada à esquerda, o nome BRIT PLEXI, seis botões "
                "pretos de tampa dourada (PRESENCE, BASS, MIDDLE, TREBLE, VOLUME I e VOLUME II) e as quatro entradas "
                "(I e II, alta e baixa) à direita."),
    mm=(740, 305, 210),
    couro=TOLEX_PRETO,
    fachada={"tipo": "tolex", "cor": [0.006, 0.006, 0.006], "grao_mm": 2.6, "friso": [0.86, 0.85, 0.82]},
    painel=dict(posicao="baixo", altura_m=0.085, cor=rgb(0.55, 0.38, 0.14), material="acrilico", texto_cor=rgb(0.015, 0.012, 0.01)),
    estilos={"padrao": MARSHALL},
    cantos="preto",
    fileiras=[
        ROW(0.42, 0.0, 1.0, T("MAINS", x=0.04), T("STANDBY", x=0.09),
            P("INDICATOR", x=0.14, cor=(0.9, 0.06, 0.02), aceso=True, tamanho_m=0.011),
            M("BRIT PLEXI", 0.02, x=0.25, z=0.55), TX("SUPER LEAD 100", x=0.25, z=0.25, letra_m=0.0075),
            K("presenca", "PRESENCE", x=0.4), K("grave", "BASS", x=0.47), K("medio", "MIDDLE", x=0.54),
            K("agudo", "TREBLE", x=0.61), K("ganho", "VOLUME I", x=0.68), K("volume", "VOLUME II", x=0.75),
            J(x=0.83, z=0.68), J(x=0.83, z=0.26), J(x=0.89, z=0.68), J(x=0.89, z=0.26),
            TX("I", x=0.83, z=0.95, letra_m=0.006), TX("II", x=0.89, z=0.95, letra_m=0.006),
            MARCA(x=0.95, z=0.47, tamanho_m=0.0055),
            letra_m=0.0085, rotulo_pos="baixo"),
    ],
    notas="O 1959 não tem master: o ganho do Nexus é o VOLUME I e o volume é o VOLUME II.",
)

# ------------------------------------------------------------------ Brit Jubilee (2555)
FICHAS["britjubilee"] = dict(
    inspiracao=("Marshall Silver Jubilee 2555: cabeçote de 740 × 315 × 222 mm em tolex prata de textura de croco com "
                "cantoneiras pretas, tela de pano preta de trama fina com friso fino prateado e a marca no meio, painel "
                "prateado escovado embaixo com letras pretas: gangorras POWER (vermelha), STANDBY e OUTPUT à esquerda, "
                "o nome BRIT JUBILEE, sete botões pretos de tampa prateada (PRESENCE, BASS, MIDDLE, TREBLE, OUTPUT "
                "MASTER, LEAD MASTER, INPUT GAIN) com o LED de canal entre os masters e uma entrada só."),
    mm=(740, 315, 222),
    couro={"cor": [0.42, 0.43, 0.45], "grao_mm": 3.4},
    fachada=tela((0.006, 0.006, 0.006), (0.03, 0.03, 0.03), 1.4, friso=(0.72, 0.73, 0.75)),
    painel=dict(posicao="baixo", altura_m=0.088, cor=rgb(0.62, 0.63, 0.64), texto_cor=PRETA),
    estilos={"padrao": estilo("marshall", 0.026, (0.018, 0.018, 0.018), (0.72, 0.73, 0.75))},
    cantos="preto",
    fileiras=[
        ROW(0.42, 0.0, 1.0, R("POWER", x=0.04), RB("STANDBY", x=0.085), RB("OUTPUT", x=0.13),
            M("BRIT JUBILEE", 0.018, x=0.245, z=0.56), TX("25/50 MODEL", x=0.245, z=0.26, letra_m=0.0075),
            K("presenca", "PRESENCE", x=0.38), K("grave", "BASS", x=0.45), K("medio", "MIDDLE", x=0.52),
            K("agudo", "TREBLE", x=0.59), K("volume", "OUTPUT MASTER", x=0.66), L(None, (0.9, 0.05, 0.02), x=0.697, z=0.75),
            F("LEAD MASTER", x=0.735), K("ganho", "INPUT GAIN", x=0.81), J("INPUT", x=0.88), MARCA(x=0.95, z=0.47, tamanho_m=0.0055),
            letra_m=0.0075, rotulo_pos="baixo"),
    ],
)

# ------------------------------------------------------------------ Rocker 50 (Rockerverb)
LARANJA = rgb(0.85, 0.27, 0.01)
ORANGE_BOTAO = estilo("cromado", 0.024, (0.015, 0.015, 0.015), (0.03, 0.03, 0.03), traco=(0.9, 0.9, 0.9), escala="tracos")
FICHAS["rocker50"] = dict(
    inspiracao=("Orange Rockerverb 50: cabeçote de 550 × 270 × 280 mm em tolex laranja com cantoneiras pretas; a frente "
                "inteira é uma placa branca recuada com dois puxadores cromados em pé nas pontas, a marca grande em cima, "
                "a faixa laranja com os nomes e embaixo o bloco preto das chaves (OFF/ON, FULL/HALF, CLEAN/DIRTY e a "
                "joia laranja), dez botões pretos de dentes em três grupos (ATTENUATOR e REVERB; DIRTY VOLUME, TREBLE, "
                "MIDDLE, BASS e GAIN; CLEAN TREBLE, BASS e VOLUME) e a entrada num bloco preto à direita."),
    mm=(550, 270, 280),
    couro={"cor": [0.8, 0.22, 0.01], "grao_mm": 1.8},
    fachada=None,
    painel=dict(posicao="inteiro", cor=rgb(0.84, 0.84, 0.82), material="pintado", texto_cor=PRETA),
    estilos={"padrao": ORANGE_BOTAO, "peq": dict(ORANGE_BOTAO, diametro_m=0.018),
             "extra": estilo("liso", 0.011, (0.015, 0.015, 0.015), traco=(0.9, 0.9, 0.9), escala="nenhuma")},
    cantos="preto",
    fileiras=[
        ROW(0.68, 0.0, 1.0, PUX(x=0.025, altura_m=0.12), PUX(x=0.975, altura_m=0.12), LOGO(0.017, x=0.4, z=0.72),
            M("ROCKER 50", 0.013, x=0.8, z=0.72)),
        ROW(0.4, 0.0, 1.0, TX("ATTEN.", x=0.2), TX("REVERB", x=0.27), TX("VOLUME", x=0.36), TX("TREBLE", x=0.43),
            TX("MIDDLE", x=0.5), TX("BASS", x=0.57), TX("GAIN", x=0.64), TX("TREBLE", x=0.73), TX("BASS", x=0.8),
            TX("VOLUME", x=0.87), letra_m=0.0062),
        ROW(0.18, 0.0, 1.0, T("OFF/ON", x=0.065, z=0.24), T("HALF", x=0.065, z=0.1, posicao="baixo"),
            T("CLEAN/DIRTY", x=0.105, z=0.24), JW((0.95, 0.4, 0.05), x=0.105, z=0.09, raio_m=0.006),
            K("presenca", "PRESENÇA", estilo="extra", x=0.145, z=0.17),
            F("", x=0.2), F("", x=0.27, estilo="peq"),
            K("volume", "", x=0.36), K("agudo", "", estilo="peq", x=0.43), K("medio", "", estilo="peq", x=0.5),
            K("grave", "", estilo="peq", x=0.57), K("ganho", "", x=0.64),
            F("", x=0.73, estilo="peq"), F("", x=0.8, estilo="peq"), F("", x=0.87), J(None, x=0.94),
            letra_m=0.0045, rotulo_pos="baixo"),
    ],
    riscos=[RISCO(0.05, 0.4, 0.95, 0.4, 0.024, cor=LARANJA),
            RISCO(0.045, 0.17, 0.165, 0.17, 0.05, cor=(0.012, 0.012, 0.012)),
            RISCO(0.915, 0.17, 0.965, 0.17, 0.05, cor=(0.012, 0.012, 0.012))],
    notas="O Rockerverb não tem presença: ficou num botão pequeno no bloco preto das chaves, à esquerda.",
)

# ------------------------------------------------------------------ BE 100 (Friedman)
DOURADO = rgb(0.75, 0.55, 0.22)
FICHAS["be100"] = dict(
    inspiracao=("Friedman BE-100: cabeçote de 743 × 311 × 222 mm todo em tolex preto, sem tela, com friso dourado e a "
                "marca no canto de cima à esquerda; painel preto embaixo com letras douradas: joia âmbar à esquerda, "
                "quinze botões pretos de tampa dourada (SYSTEM VOL, THUMP, PRESENCE, MASTER 2, MASTER 1, TREBLE, MIDDLE, "
                "BASS, GAIN 2, GAIN 1 e, do canal limpo, VOLUME, TREBLE, MIDDLE, BASS, GAIN), as minichaves FREQ, "
                "RESPONSE, VOICE, HBE-BE-CLN e BRIGHT e a entrada INPUT na ponta direita."),
    mm=(743, 311, 222),
    couro=TOLEX_PRETO,
    fachada={"tipo": "tolex", "cor": [0.006, 0.006, 0.006], "grao_mm": 2.6, "friso": [0.75, 0.55, 0.22]},
    logo={"lado_rel": -0.31, "altura_m": 0.014, "altura_rel": 0.85},
    painel=dict(posicao="baixo", altura_m=0.085, cor=rgb(0.012, 0.012, 0.012), material="pintado", texto_cor=DOURADO),
    estilos={"padrao": estilo("marshall", 0.021, (0.018, 0.018, 0.018), (0.56, 0.39, 0.17), escala="tracos")},
    cantos="preto",
    fileiras=[
        ROW(0.42, 0.06, 0.94, F("SYSTEM VOL"), F("THUMP"), K("presenca", "PRESENCE"), F("MASTER 2"), K("volume", "MASTER 1"),
            K("agudo", "TREBLE"), K("medio", "MIDDLE"), K("grave", "BASS"), F("GAIN 2"), K("ganho", "GAIN 1"),
            F("VOLUME"), F("TREBLE"), F("MIDDLE"), F("BASS"), F("GAIN"), letra_m=0.0058, rotulo_pos="baixo"),
        ROW(0.88, 0.0, 1.0, MT("FREQ", x=0.12, rotulo_pos="direita"), MT("RESPONSE", x=0.2, rotulo_pos="direita"),
            MT("VOICE", x=0.5, rotulo_pos="direita"), MT("HBE-BE-CLN", x=0.6, rotulo_pos="direita"),
            MT("BRIGHT", x=0.72, rotulo_pos="direita"), letra_m=0.005),
        ROW(0.42, 0.0, 1.0, JW((0.95, 0.5, 0.05), x=0.028, raio_m=0.0065), J("INPUT", x=0.972, rotulo_pos="baixo"), letra_m=0.0058),
    ],
)

# ------------------------------------------------------------------ Power 100 (ENGL Powerball)
ENGL = estilo("marshall", 0.019, (0.015, 0.015, 0.015), (0.03, 0.03, 0.03), traco=(0.9, 0.9, 0.9), escala="nenhuma")
FICHAS["power100"] = dict(
    inspiracao=("ENGL Powerball II: cabeçote de 710 × 270 × 270 mm em tolex preto com cantoneiras cromadas, grade de "
                "metal preta vazada com a marca no meio e painel grafite fosco embaixo com letras brancas: entrada à "
                "esquerda, vinte botões pretos de saia em duas fileiras (CLEAN GAIN, CRUNCH GAIN, BASS, MIDDLE, TREBLE, "
                "VOLUMEs, PRESENCE e DEPTH PUNCH em cima, que é a que toca; LEAD GAIN 3 e 4, BASS, MIDDLE, MID BOOST, "
                "TREBLE, LEAD VOL 3 e 4, MASTER A e B embaixo), teclinhas com LEDs, o nome em script e as gangorras "
                "STAND BY (preta) e POWER (vermelha acesa) à direita."),
    mm=(710, 270, 270),
    couro=TOLEX_PRETO,
    fachada={"tipo": "grade", "cor": [0.02, 0.02, 0.022], "furo_mm": 4.5, "passo_mm": 6.0, "furo": "losango"},
    logo={"lado_rel": 0.0, "altura_m": 0.026, "altura_rel": 0.52},
    painel=dict(posicao="baixo", altura_m=0.1, cor=rgb(0.04, 0.04, 0.042), material="pintado", texto_cor=BRANCO),
    estilos={"padrao": ENGL},
    cantos="cromo",
    fileiras=[
        ROW(0.5, 0.0, 1.0, J("INPUT", x=0.035, rotulo_pos="baixo"), letra_m=0.0048),
        ROW(0.7, 0.08, 0.8, K("ganho", "CLEAN GAIN"), F("CRUNCH GAIN"), K("grave", "BASS"), K("medio", "MIDDLE"),
            K("agudo", "TREBLE"), F("TREBLE"), K("volume", "CLEAN VOL"), F("CRUNCH VOL"), K("presenca", "PRESENCE"),
            F("DEPTH PUNCH"), letra_m=0.0042, rotulo_pos="cima"),
        ROW(0.25, 0.08, 0.8, F("LEAD GAIN 3"), F("LEAD GAIN 4"), F("BASS"), F("MIDDLE"), F("MID BOOST"), F("TREBLE"),
            F("LEAD VOL 3"), F("LEAD VOL 4"), F("MASTER A"), F("MASTER B"), letra_m=0.0042, rotulo_pos="baixo"),
        ROW(0.5, 0.0, 1.0, P("CH 1/2", x=0.82, z=0.72, tamanho_m=0.007), P("CH 3/4", x=0.82, z=0.28, tamanho_m=0.007),
            L(None, (0.1, 0.8, 0.1), aceso=True, x=0.845, z=0.72), L(None, (0.9, 0.05, 0.02), x=0.845, z=0.28),
            M("Power 100", 0.013, x=0.895, z=0.5, fonte="script"),
            RB("STAND BY", x=0.945, z=0.55), R("POWER", x=0.975, z=0.55), letra_m=0.0042, rotulo_pos="baixo"),
    ],
)
