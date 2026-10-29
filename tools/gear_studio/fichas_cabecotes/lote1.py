# Fichas do lote 1: os cabeçotes mais conhecidos e os formatos de teste.
from cabecotes_fiel import *  # noqa: F401,F403

TOLEX_PRETO = {"cor": [0.006, 0.006, 0.006], "grao_mm": 2.6}
MARSHALL_TELA = tela((0.006, 0.006, 0.006), (0.03, 0.03, 0.03), 1.4, friso=(0.86, 0.85, 0.82))
OURO = rgb(0.5, 0.34, 0.14)
TINTA_PRETA = rgb(0.015, 0.012, 0.01)
BRANCO = rgb(0.85, 0.85, 0.85)
MARSHALL_BOTAO = estilo("marshall", 0.026, (0.018, 0.018, 0.018), (0.56, 0.39, 0.17))

FICHAS = {}

# ------------------------------------------------------------------ Brit 800
FICHAS["brit800"] = dict(
    inspiracao=("Marshall JCM800 2203: cabeçote de 740 × 315 × 210 mm em tolex preto com cantoneiras pretas, "
                "tela de pano preta de trama fina com friso branco e a marca no meio; painel dourado escovado "
                "embaixo com as chaves POWER (vermelha, acesa) e STANDBY à esquerda, o nome BRIT 800 inclinado e "
                "seis botões pretos de tampa dourada na ordem do original: PRESENÇA, GRAVE, MÉDIO, AGUDO, MASTER e "
                "PRE-AMP (o ganho na ponta direita), com as entradas HIGH e LOW uma em cima da outra."),
    mm=(740, 315, 210),
    couro=TOLEX_PRETO,
    fachada=MARSHALL_TELA,
    painel=dict(posicao="baixo", altura_m=0.088, cor=OURO, texto_cor=TINTA_PRETA),
    estilos={"padrao": MARSHALL_BOTAO},
    cantos="preto",
    fileiras=[
        ROW(0.42, 0.0, 1.0,
            R("POWER", x=0.045, rotulo_pos="baixo"), RB("STANDBY", x=0.095, rotulo_pos="baixo"),
            M("BRIT 800", 0.026, x=0.215, z=0.56, inclinacao=0.0), TX("LEAD SERIES", x=0.215, z=0.27, letra_m=0.0085),
            K("presenca", x=0.44), K("grave", x=0.515), K("medio", x=0.59), K("agudo", x=0.665),
            K("volume", "MASTER", x=0.74), K("ganho", "PRE-AMP", x=0.815),
            J(x=0.875, z=0.66), J(x=0.875, z=0.28), TX("HIGH", x=0.91, z=0.66, letra_m=0.006),
            TX("LOW", x=0.91, z=0.28, letra_m=0.006), MARCA(x=0.955, z=0.47, tamanho_m=0.0055)),
    ],
    riscos=[RISCO(0.44, 0.1, 0.665, 0.1), RISCO(0.74, 0.1, 0.815, 0.1)],
)

# ------------------------------------------------------------------ Mark Five
MESA_BOTAO = estilo("liso", 0.019, (0.012, 0.012, 0.012), (0.02, 0.02, 0.02), traco=(0.85, 0.85, 0.85), escala="nenhuma")
MESA_PEQ = estilo("liso", 0.015, (0.012, 0.012, 0.012), (0.02, 0.02, 0.02), traco=(0.85, 0.85, 0.85), escala="nenhuma")


def _mesa_canal(x0, funcional, n):
    """Canal do Mark V: duas fileiras de três botões e as minichaves."""
    sp = 0.042
    if funcional:
        top = [K("ganho", "GAIN"), K("volume", "MASTER"), K("presenca", "PRESENCE")]
        bot = [K("agudo", "TREBLE"), K("medio", "MID"), K("grave", "BASS")]
    else:
        top = [F("GAIN"), F("MASTER"), F("PRESENCE")]
        bot = [F("TREBLE"), F("MID"), F("BASS")]
    out_top, out_bot = [], []
    for i, (a, b) in enumerate(zip(top, bot)):
        a.update(x=x0 + 0.04 + i * sp, z=0.66)
        b.update(x=x0 + 0.04 + i * sp, z=0.25, rotulo_pos="baixo")
        out_top.append(a)
        out_bot.append(b)
    chaves = [MT(x=x0, z=0.74), MT(x=x0, z=0.47), MT(x=x0, z=0.2)]
    return out_top + out_bot + chaves + [LC("", x=x0 + 0.04 + sp, z=0.93)]


FICHAS["mark5"] = dict(
    inspiracao=("Mesa/Boogie Mark V: cabeçote de 578 × 254 × 298 mm em tolex preto, painel preto recuado na faixa "
                "de CIMA com letras brancas e a tela preta de trama embaixo (marca à esquerda, como a plaqueta Boogie); "
                "entrada à esquerda, três canais com GAIN, MASTER, PRESENCE em cima e TREBLE, MID, BASS embaixo e "
                "três minichaves cada (o canal 1 é o que toca), EQ gráfico de cinco deslizantes, coluna de REVERB, "
                "OUTPUT e SOLO e as alavancas STANDBY e POWER à direita."),
    mm=(578, 254, 298),
    couro=TOLEX_PRETO,
    fachada=tela((0.008, 0.008, 0.008), (0.03, 0.03, 0.03), 2.4),
    logo={"lado_rel": -0.3, "altura_m": 0.014, "altura_rel": 0.62},
    parafusos_painel=False,
    painel=dict(posicao="cima", altura_m=0.088, cor=rgb(0.01, 0.01, 0.011), material="pintado", texto_cor=BRANCO),
    estilos={"padrao": MESA_BOTAO, "peq": MESA_PEQ},
    cantos="nenhum",
    fileiras=[
        ROW(0.5, 0.0, 1.0, J("INPUT", x=0.025, z=0.45, rotulo_pos="baixo"), letra_m=0.0052),
        ROW(0.5, 0.0, 1.0, *_mesa_canal(0.055, True, 1), letra_m=0.0048),
        ROW(0.5, 0.0, 1.0, *_mesa_canal(0.245, False, 2), letra_m=0.0048),
        ROW(0.5, 0.0, 1.0, *_mesa_canal(0.435, False, 3), letra_m=0.0048),
        ROW(0.5, 0.64, 0.80, S("eq80", "80"), S("eq240", "240"), S("eq750", "750"), S("eq2200", "2200"),
            S("eq6600", "6600"), rotulo_pos="baixo", letra_m=0.0045, curso_m=0.040),
        ROW(0.5, 0.0, 1.0, F("REVERB", estilo="peq", x=0.835, z=0.8), F("", estilo="peq", x=0.835, z=0.5),
            F("", estilo="peq", x=0.835, z=0.2), F("OUTPUT", estilo="peq", x=0.875, z=0.68),
            F("SOLO", estilo="peq", x=0.875, z=0.28, rotulo_pos="baixo"), letra_m=0.0045),
        ROW(0.5, 0.0, 1.0, T("STANDBY", x=0.915, z=0.55, rotulo_pos="baixo"), T("POWER", x=0.955, z=0.55, rotulo_pos="baixo"),
            M("MARK FIVE", 0.0095, x=0.935, z=0.15, fonte="mesa"), letra_m=0.0045),
    ],
    riscos=[RISCO(0.235, 0.08, 0.235, 0.92), RISCO(0.425, 0.08, 0.425, 0.92), RISCO(0.62, 0.08, 0.62, 0.92),
            RISCO(0.815, 0.08, 0.815, 0.92)],
)

# ------------------------------------------------------------------ Silver Twin (combo)
FENDER_BOTAO = estilo("saia_numerada", 0.02, (0.012, 0.012, 0.012), (0.02, 0.02, 0.02),
                      saia=(0.72, 0.73, 0.75), numeros=(0.02, 0.02, 0.02), traco=(0.9, 0.9, 0.9), escala="nenhuma")
FICHAS["silvertwin"] = dict(
    inspiracao=("Fender Twin Reverb (blackface): combo 2×12 de 673 × 505 × 264 mm em tolex preto, tela prateada "
                "com a marca no alto à esquerda e painel preto com friso prateado na faixa de cima: canal NORMAL "
                "(entradas 1 e 2, BRIGHT, VOLUME, TREBLE, MIDDLE, BASS) e canal VIBRATO (entradas, BRIGHT, VOLUME, "
                "TREBLE, MIDDLE, BASS, REVERB, SPEED, INTENSITY), botões pretos de saia prateada numerada, o nome "
                "em script e a joia vermelha na ponta direita."),
    formato="combo",
    mm=(673, 505, 264),
    couro={"cor": [0.007, 0.007, 0.007], "grao_mm": 2.2},
    fachada=tela((0.5, 0.51, 0.52), (0.36, 0.37, 0.39), 1.8, friso=(0.75, 0.75, 0.75)),
    logo={"lado_rel": -0.32, "altura_rel": 0.86, "altura_m": 0.017},
    painel=dict(posicao="cima", altura_m=0.07, cor=rgb(0.008, 0.008, 0.009), material="pintado", texto_cor=BRANCO,
                friso=(0.78, 0.79, 0.8)),
    estilos={"padrao": FENDER_BOTAO},
    cantos="cromo",
    fileiras=[
        ROW(0.55, 0.0, 1.0,
            J("1", x=0.035), J("2", x=0.068), MT("BRIGHT", x=0.1), TX("NORMAL", x=0.05, z=0.18, letra_m=0.0062),
            K("ganho", "VOLUME", x=0.145), K("agudo", "TREBLE", x=0.195), K("medio", "MIDDLE", x=0.245),
            K("grave", "BASS", x=0.295),
            J("1", x=0.355), J("2", x=0.388), MT("BRIGHT", x=0.42), TX("VIBRATO", x=0.372, z=0.18, letra_m=0.0062),
            K("volume", "VOLUME", x=0.465), F("TREBLE", x=0.515), F("MIDDLE", x=0.565), F("BASS", x=0.615),
            F("REVERB", x=0.665), F("SPEED", x=0.715), K("presenca", "INTENSITY", x=0.765),
            M("Silver Twin", 0.016, x=0.87, z=0.6, fonte="script"), TX("REVERB-AMP", x=0.87, z=0.24, letra_m=0.005),
            JW((0.95, 0.05, 0.03), x=0.96, z=0.55, raio_m=0.0075),
            rotulo_pos="baixo", letra_m=0.0055),
    ],
    riscos=[RISCO(0.33, 0.06, 0.33, 0.94, 0.0012, cor=(0.7, 0.7, 0.72))],
    notas="O Twin não tem presença nem master: a presença ficou no INTENSITY (vibrato) e o volume de saída no VOLUME do canal Vibrato; o ganho é o VOLUME do canal Normal.",
)

# ------------------------------------------------------------------ Top Boost 15 (combo, painel no topo)
VOX_BICO = estilo("chicken", 0.020, (0.015, 0.015, 0.015), traco=(0.85, 0.75, 0.45), escala="tracos")
OURO_VOX = rgb(0.62, 0.45, 0.16)
FICHAS["topboost15"] = dict(
    inspiracao=("Vox AC15 C1: combo 1×12 de 602 × 456 × 265 mm em vinil preto, tela vinho com losangos de fios "
                "claros e friso branco, faixa preta com a marca dourada e filete dourado acima da tela; painel preto "
                "com filetes e letras dourados no TAMPO, com LED, chaves POWER e STANDBY, botões bico-de-galinha "
                "pretos agrupados (MASTER: VOLUME e TONE CUT; TREMOLO: SPEED e DEPTH; REVERB; TOP BOOST: TREBLE e "
                "BASS; VOLUMEs dos canais) e as entradas NORMAL e TOP BOOST à direita."),
    formato="combo",
    mm=(602, 456, 265),
    couro={"cor": [0.008, 0.008, 0.008], "grao_mm": 1.6},
    fachada={"tipo": "losangos", "cor": [0.17, 0.035, 0.03], "linha": [0.55, 0.52, 0.36], "passo_mm": 46.0,
             "friso": [0.85, 0.84, 0.8], "faixa": {"altura_rel": 0.27, "friso": [0.75, 0.55, 0.2]}},
    logo={"lado_rel": -0.3, "altura_rel": 0.5, "altura_m": 0.018},
    painel=dict(posicao="cima", altura_m=0.075, largura_m=0.52, cor=rgb(0.01, 0.01, 0.01), material="pintado",
                texto_cor=OURO_VOX),
    estilos={"padrao": VOX_BICO},
    cantos="preto",
    fileiras=[
        ROW(0.45, 0.0, 1.0,
            L(None, (0.9, 0.05, 0.02), aceso=True, x=0.03), T("POWER", x=0.07), T("STANDBY", x=0.115),
            K("volume", "VOLUME", x=0.175), K("presenca", "TONE CUT", x=0.24),
            F("SPEED", x=0.32), F("DEPTH", x=0.385), F("LEVEL", x=0.46),
            K("agudo", "TREBLE", x=0.54), K("grave", "BASS", x=0.605), K("ganho", "VOLUME", x=0.67),
            K("medio", "VOLUME", x=0.75),
            J("NORMAL", x=0.85), J("TOP BOOST", x=0.93), LC("", x=0.85, z=0.9), LC("", x=0.93, z=0.9),
            rotulo_pos="baixo", letra_m=0.0052),
        ROW(0.9, 0.0, 1.0, TX("MASTER", x=0.21), TX("TREMOLO", x=0.352), TX("REVERB", x=0.46), TX("TOP BOOST", x=0.605),
            TX("NORMAL", x=0.75), letra_m=0.0052),
    ],
    riscos=[RISCO(0.145, 0.05, 0.145, 0.95, cor=OURO_VOX), RISCO(0.282, 0.05, 0.282, 0.95, cor=OURO_VOX),
            RISCO(0.425, 0.05, 0.425, 0.95, cor=OURO_VOX), RISCO(0.5, 0.05, 0.5, 0.95, cor=OURO_VOX),
            RISCO(0.71, 0.05, 0.71, 0.95, cor=OURO_VOX), RISCO(0.79, 0.05, 0.79, 0.95, cor=OURO_VOX)],
    notas=("O AC15 não tem botão de médio: o médio do Nexus ficou no VOLUME do canal Normal; a presença é o TONE CUT "
           "(no original corta agudo no power). Os dois LEDs de canal sobre as entradas não existem no original: "
           "são para a tela mostrar o canal ativo."),
)

# ------------------------------------------------------------------ Brit Rack 1 (rack 1U)
TECLA = dict(tamanho_m=0.0075)
FICHAS["britrack1"] = dict(
    inspiracao=("Marshall JMP-1: pré-amplificador de rack 1U (483 × 44 × 260 mm) com frente dourada escovada e "
                "orelhas com rasgos; entradas INPUT e PHONES e o OUTPUT LEVEL à esquerda, janela preta no meio com "
                "o nome, os visores PATCH e DATA, as teclas PATCH e STORE e o botão DATA, duas fileiras de teclas "
                "redondas pretas (VOLUME, GAIN, BASS, MIDDLE, TREBLE, PRESENCE, EFFECT / OD 1, OD 2, BASS SHIFT, "
                "CLEAN 1, CLEAN 2, STEP, CHANNEL) e a tecla POWER à direita. Os seis botões que tocam ficam na "
                "fileira de cima, no lugar das teclas."),
    formato="rack", rack_u=1,
    mm=(483, 44, 260),
    couro={"cor": [0.03, 0.03, 0.03], "metal": True},
    fachada=None,
    painel=dict(posicao="inteiro", cor=OURO, texto_cor=TINTA_PRETA),
    estilos={"padrao": estilo("liso", 0.011, (0.015, 0.015, 0.015), traco=(0.9, 0.9, 0.9), escala="nenhuma"),
             "data": estilo("liso", 0.016, (0.015, 0.015, 0.015), traco=(0.9, 0.9, 0.9), escala="nenhuma")},
    fileiras=[
        ROW(0.45, 0.0, 1.0, J("INPUT", x=0.04), J("PHONES", x=0.09), F("OUTPUT LEVEL", estilo="data", x=0.15),
            LOGO(0.0055, x=0.29, z=0.62), TX("VALVE MIDI PRE-AMP", x=0.29, z=0.28, letra_m=0.0034),
            TX("PATCH", x=0.39, z=0.82, letra_m=0.0034), TX("DATA", x=0.45, z=0.82, letra_m=0.0034),
            P(None, x=0.505, z=0.62, **TECLA), P(None, x=0.505, z=0.3, **TECLA), F("DATA", estilo="data", x=0.55, z=0.45),
            letra_m=0.0036, rotulo_pos="cima"),
        ROW(0.68, 0.6, 0.93, K("volume", "VOLUME"), K("ganho", "GAIN"), K("grave", "BASS"), K("medio", "MIDDLE"),
            K("agudo", "TREBLE"), K("presenca", "PRESENCE"), P(None, **TECLA), letra_m=0.0032),
        ROW(0.25, 0.0, 1.0, LC("CLEAN 1", x=0.765), LC("OD 1", x=0.6236), rotulo_pos="baixo", letra_m=0.0032),
        ROW(0.25, 0.6, 0.93, GAP(), P("OD 2", **TECLA), P("BASS SHIFT", **TECLA), GAP(), P("CLEAN 2", **TECLA),
            P("STEP", **TECLA), P("CHANNEL", **TECLA), rotulo_pos="baixo", letra_m=0.0032),
        ROW(0.45, 0.0, 1.0, P("POWER", x=0.965, rotulo_pos="cima", tamanho_m=0.009), letra_m=0.0034),
    ],
    riscos=[RISCO(0.22, 0.5, 0.58, 0.5, 0.034, cor=(0.01, 0.01, 0.01)),
            dict(RISCO(0.37, 0.48, 0.41, 0.48, 0.016, cor=(0.9, 0.03, 0.01)), luz=1.5),
            dict(RISCO(0.43, 0.48, 0.47, 0.48, 0.016, cor=(0.9, 0.03, 0.01)), luz=1.5)],
    notas=("Os botões que tocam ficam no lugar das teclas de cima do JMP-1 (que no original escolhem o parâmetro do "
           "botão DATA). Os LEDs de canal são as teclas CLEAN 1 (Limpo) e OD 1 (Overdrive)."),
)

# ------------------------------------------------------------------ Tiny 15 (caixinha)
ORANGE = rgb(0.85, 0.27, 0.01)
FICHAS["tiny15"] = dict(
    inspiracao=("Orange Tiny Terror: caixinha de aço branco de 303 × 190 × 153 mm com alça cromada de tubo; placa "
                "de cima branca com a marca grande e o nome TINY 15, faixa preta e laranja com os nomes, e embaixo "
                "as alavancas POWER e OUTPUT, a luz piloto laranja, VOLUME e GAIN grandes e TONE pequeno em botões "
                "pretos, e a entrada INPUT à direita."),
    formato="caixinha",
    mm=(303, 190, 153),
    couro={"cor": [0.82, 0.82, 0.8], "metal": True, "aspereza": 0.35},
    fachada=None,
    alca="barra",
    painel=dict(posicao="inteiro", cor=rgb(0.86, 0.86, 0.84), material="pintado", texto_cor=rgb(0.01, 0.01, 0.01)),
    estilos={"padrao": estilo("cromado", 0.026, (0.015, 0.015, 0.015), (0.03, 0.03, 0.03), traco=(0.9, 0.9, 0.9), escala="tracos"),
             "peq": estilo("cromado", 0.018, (0.015, 0.015, 0.015), (0.03, 0.03, 0.03), traco=(0.9, 0.9, 0.9), escala="tracos"),
             "extra": estilo("liso", 0.011, (0.015, 0.015, 0.015), traco=(0.9, 0.9, 0.9), escala="nenhuma")},
    fileiras=[
        ROW(0.78, 0.0, 1.0, LOGO(0.0125, x=0.36, z=0.75), M("TINY 15", 0.012, x=0.72, z=0.75),
            K("grave", "GRAVE", estilo="extra", x=0.88, z=0.86), K("medio", "MÉDIO", estilo="extra", x=0.93, z=0.86),
            K("presenca", "PRESENÇA", estilo="extra", x=0.88, z=0.66), letra_m=0.0034, rotulo_pos="baixo"),
        ROW(0.5, 0.0, 1.0, TX("POWER", x=0.1), TX("OUTPUT", x=0.2), TX("VOLUME", x=0.46), TX("TONE", x=0.6),
            TX("GAIN", x=0.74), TX("INPUT", x=0.9), letra_m=0.0058),
        ROW(0.22, 0.0, 1.0, T("OFF / ON", x=0.1), T("15W / 7W", x=0.2), JW((0.95, 0.4, 0.05), x=0.3, raio_m=0.0065),
            K("volume", "", x=0.46), K("agudo", "", estilo="peq", x=0.6), K("ganho", "", x=0.74), J(None, x=0.9),
            letra_m=0.0045, rotulo_pos="baixo"),
    ],
    riscos=[RISCO(0.0, 0.5, 1.0, 0.5, 0.03, cor=(0.01, 0.01, 0.01)), RISCO(0.0, 0.43, 1.0, 0.43, 0.012, cor=ORANGE),
            RISCO(0.0, 0.405, 1.0, 0.405, 0.002, cor=(0.01, 0.01, 0.01))],
    notas="O Tiny Terror só tem VOLUME, TONE e GAIN: o TONE é o agudo; grave, médio e presença do Nexus ficaram em três botões pequenos no canto de cima à direita (no lugar do brasão).",
)

# ------------------------------------------------------------------ Vier 100 (Diezel VH4)
DIEZEL = estilo("facetado", 0.02, (0.012, 0.012, 0.012), (0.75, 0.75, 0.77), traco=(0.9, 0.9, 0.9), escala="nenhuma",
                tampa_metal=True)


def _diezel():
    nomes = ["GAIN", "VOLUME", "TREBLE", "MIDDLE", "BASS"]
    ids = ["ganho", None, "agudo", "medio", "grave"]
    itens, riscos = [], []
    for p in range(5):
        z = 0.84 - p * 0.17
        riscos.append(dict(RISCO(0.06, z - 0.03, 0.80, z + 0.05, 0.011, cor=(0.01, 0.01, 0.01))))
    for c in range(4):
        for p in range(5):
            x = 0.12 + c * 0.155 + (4 - p) * 0.026
            z = 0.84 - p * 0.17 + (x - 0.06) / 0.74 * 0.08 - 0.03
            if c == 0 and ids[p]:
                it = K(ids[p], "")
            else:
                it = F("")
            it.update(x=x, z=z)
            itens.append(it)
            itens.append(TX(nomes[p].capitalize(), x=x + 0.055, z=z + 0.006, letra_m=0.0062))
        led = LC("") if c < 3 else L(None)
        led.update(x=0.12 + c * 0.155 + 0.13, z=0.96)
        itens.append(led)
        itens.append(TX(f"CH {c + 1}", x=0.12 + c * 0.155 + 0.105, z=0.96, letra_m=0.0062))
        if c < 2:
            itens.append(MT("Bright", x=0.12 + c * 0.155 + 0.03, z=0.96, rotulo_pos="direita", letra_m=0.0055))
    return itens, riscos


_dz, _dr = _diezel()
FICHAS["vier100"] = dict(
    inspiracao=("Diezel VH4: cabeçote de 740 × 300 × 280 mm em tolex preto com cantoneiras cromadas, grade de metal "
                "preta recortada em garras na metade de cima com a marca no meio e painel de alumínio escovado prata "
                "na metade de baixo: quatro canais lado a lado com GAIN, VOLUME, TREBLE, MIDDLE e BASS em diagonal "
                "sobre faixas pretas com os nomes em branco (o canal 1 é o que toca), LEDs vermelhos CH 1 a CH 4, "
                "minichaves Bright nos canais 1 e 2, entradas à esquerda e, à direita, VOLUME (master), PARALLEL LOOP, "
                "PRESENCE e DEEP, e as gangorras pretas STANDBY e POWER, todos em botões pretos facetados."),
    mm=(740, 300, 280),
    couro=TOLEX_PRETO,
    fachada={"tipo": "garras", "cor": [0.015, 0.015, 0.016], "passo_mm": 16.0},
    logo={"lado_rel": 0.0, "altura_m": 0.026, "altura_rel": 0.5},
    painel=dict(posicao="baixo", altura_m=0.14, cor=rgb(0.62, 0.63, 0.64), texto_cor=BRANCO),
    estilos={"padrao": DIEZEL},
    cantos="cromo",
    fileiras=[
        ROW(0.5, 0.0, 1.0, *_dz),
        ROW(0.5, 0.0, 1.0, J(x=0.035, z=0.62), J(x=0.035, z=0.3),
            K("volume", "VOLUME", x=0.795, z=0.78), F("PARALLEL LOOP", x=0.835, z=0.56), K("presenca", "PRESENCE", x=0.875, z=0.34),
            F("DEEP", x=0.915, z=0.12, rotulo_pos="direita"), RB("STANDBY", x=0.93, z=0.8), RB("POWER", x=0.97, z=0.8),
            M("VIER 100", 0.011, x=0.95, z=0.5, cor=(0.01, 0.01, 0.01)), letra_m=0.0062, rotulo_pos="cima"),
    ],
    riscos=_dr,
)

# ------------------------------------------------------------------ Tri 100 (TriAmp)
TRI = estilo("cromado", 0.016, (0.85, 0.86, 0.88), (0.9, 0.9, 0.92), traco=(0.05, 0.05, 0.06), escala="nenhuma",
             metal=True, anel=(0.05, 0.25, 1.0))
AZUL = rgb(0.25, 0.45, 1.0)


def _tri():
    itens = []
    for a, base in enumerate((0.27, 0.51, 0.745)):
        amp = 3 - a
        for row, z in (("A", 0.66), ("B", 0.26)):
            for i, nome in enumerate(["MASTER", "TREBLE", "MID", "BASS", "GAIN"]):
                x = base + i * 0.04
                if amp == 1 and row == "A":
                    kid = {"MASTER": "volume", "TREBLE": "agudo", "MID": "medio", "BASS": "grave", "GAIN": "ganho"}[nome]
                    it = K(kid, nome if row == "A" else "")
                else:
                    it = F(nome if row == "A" else "")
                it.update(x=x, z=z)
                itens.append(it)
            itens.append(P(f"{amp}{row}" if row == "B" else "CHANNEL", x=base + 0.2, z=z, cor=AZUL, aceso=(row == "A"),
                           tamanho_m=0.008))
    return itens


FICHAS["tri100"] = dict(
    inspiracao=("Hughes & Kettner TriAmp Mark 3: cabeçote de 740 × 303 × 254 mm em tolex preto com cantoneiras pretas "
                "e alça cromada, frente aberta de vidro escuro mostrando as válvulas acesas e uma linha de luz azul, a "
                "marca no meio, e painel de acrílico preto brilhante embaixo com 33 botões cromados de anel azul: "
                "MASTER (VOLUME, PRESENCE, RESONANCE) e os três amplificadores AMP 3, AMP 2 e AMP 1 em duas fileiras "
                "(A e B) de MASTER, TREBLE, MID, BASS e GAIN (a fileira A do AMP 1 é a que toca), teclas azuis de "
                "canal, chaves POWER e STANDBY, STOMP BOOST e a entrada à direita."),
    mm=(740, 303, 254),
    couro=TOLEX_PRETO,
    fachada={"tipo": "vidro", "cor": [0.3, 0.3, 0.34], "valvulas": 8, "neon": [0.1, 0.3, 1.0]},
    logo={"lado_rel": 0.0, "altura_m": 0.026, "altura_rel": 0.6},
    painel=dict(posicao="baixo", altura_m=0.075, cor=rgb(0.004, 0.004, 0.005), material="acrilico", luz=AZUL,
                texto_cor=rgb(0.75, 0.82, 1.0)),
    estilos={"padrao": TRI},
    cantos="preto", alca="cromo",
    parafusos_painel=False,
    fileiras=[
        ROW(0.5, 0.0, 1.0, *_tri(), letra_m=0.0045),
        ROW(0.5, 0.0, 1.0, L("ON", AZUL, aceso=True, x=0.03, z=0.7), T("POWER", x=0.03, z=0.25, rotulo_pos="baixo"),
            P("MIDI LEARN", x=0.075, z=0.66, cor=AZUL, aceso=True), T("STANDBY", x=0.075, z=0.25, rotulo_pos="baixo"),
            F("VOLUME", x=0.125, z=0.66), K("presenca", "PRESENCE", x=0.165, z=0.66), F("RESONANCE", x=0.205, z=0.66),
            P("NOISE GATE", x=0.125, z=0.26, cor=AZUL, aceso=True, rotulo_pos="baixo"),
            P("FX LOOP", x=0.16, z=0.26, cor=AZUL, aceso=True, rotulo_pos="baixo"),
            P("1/2", x=0.195, z=0.26, cor=AZUL, aceso=True, rotulo_pos="baixo"),
            P("3/4", x=0.22, z=0.26, cor=AZUL, aceso=True, rotulo_pos="baixo"),
            P("5/6", x=0.245, z=0.26, cor=AZUL, aceso=True, rotulo_pos="baixo"),
            L("STOMP BOOST", (0.9, 0.05, 0.02), aceso=True, x=0.975, z=0.66), J("INPUT", x=0.975, z=0.28, rotulo_pos="baixo"),
            letra_m=0.0045),
    ],
    notas="O TriAmp não tem presença no canal: a presença do Nexus é o PRESENCE do MASTER (que existe no original).",
)
