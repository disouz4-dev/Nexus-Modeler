# Fichas do lote 4: Mesa (IIC+, III, IV, JP-2C, Badlander, Lonestar), Orange OR50 e AD30, Vox AC30 e Matchless.
from cabecotes_fiel import *  # noqa: F401,F403

TOLEX_PRETO = {"cor": [0.006, 0.006, 0.006], "grao_mm": 2.6}
BRONCO = {"cor": [0.008, 0.008, 0.008], "grao_mm": 3.6}
BRANCO = rgb(0.85, 0.85, 0.85)
PRETA = rgb(0.012, 0.012, 0.012)
MESA_TELA = tela((0.008, 0.008, 0.008), (0.03, 0.03, 0.03), 2.4)
MESA_LOGO = {"lado_rel": -0.28, "altura_m": 0.012, "altura_rel": 0.62}
MESA_SAIA = estilo("saia_numerada", 0.016, (0.012, 0.012, 0.012), (0.02, 0.02, 0.02), saia=(0.015, 0.015, 0.015),
                   numeros=(0.85, 0.85, 0.85), traco=(0.85, 0.85, 0.85), escala="nenhuma")
EXTRA = estilo("liso", 0.011, (0.015, 0.015, 0.015), traco=(0.9, 0.9, 0.9), escala="nenhuma")
JANELA_EQ = (0.62, 0.63, 0.65)
FICHAS = {}


def _eq(x0, x1, z=0.5, fixo=False):
    nomes = ["80", "240", "750", "2200", "6600"]
    if fixo:
        return ROW(z, x0, x1, *[SF(n) for n in nomes], rotulo_pos="baixo", letra_m=0.0042, curso_m=0.034)
    return ROW(z, x0, x1, *[S("eq" + n, n) for n in nomes], rotulo_pos="baixo", letra_m=0.0042, curso_m=0.034)


def _mesa_curto(nome, inspiracao, botoes, chaves, canais_x, notas, painel_cor=(0.01, 0.01, 0.011), friso=None, mm=(476, 235, 262)):
    return dict(
        inspiracao=inspiracao, mm=mm, couro=BRONCO, fachada=MESA_TELA, logo=MESA_LOGO,
        painel=dict(posicao="cima", altura_m=0.09, cor=rgb(*painel_cor), material="pintado", texto_cor=BRANCO,
                    **({"friso": friso} if friso else {})),
        estilos={"padrao": MESA_SAIA, "extra": EXTRA}, cantos="cromo", parafusos_painel=False,
        fileiras=[ROW(0.5, 0.0, 1.0, J("INPUT", x=0.04, z=0.7), J("FOOT SW.", x=0.04, z=0.25, rotulo_pos="baixo"),
                      *botoes, letra_m=0.0045, rotulo_pos="baixo"),
                  _eq(0.555, 0.725),
                  ROW(0.55, 0.0, 1.0, *chaves, *canais_x, letra_m=0.0042, rotulo_pos="baixo")],
        riscos=[dict(RISCO(0.545, 0.5, 0.735, 0.5, 0.075, cor=JANELA_EQ))],
        notas=notas,
    )


FICHAS["mark2plus"] = _mesa_curto(
    "Mark Two Plus",
    ("Mesa/Boogie Mark IIC+: cabeçote curto de 476 × 235 × 262 mm em tolex preto Bronco com cantoneiras cromadas, painel "
     "preto na faixa de cima com letras brancas e a tela preta embaixo com a marca à esquerda: entradas INPUT e FOOT SW. "
     "empilhadas, sete botões pretos de saia numerada (VOLUME 1, TREBLE, BASS, MIDDLE, MASTER 1, LEAD DRIVE, LEAD "
     "MASTER), o EQ gráfico de cinco deslizantes numa janela prateada, as alavancas EQ, STANDBY e POWER e a luz piloto azul."),
    [F("VOLUME 1", x=0.09), K("agudo", "TREBLE", x=0.145), K("grave", "BASS", x=0.2), K("medio", "MIDDLE", x=0.255),
     F("MASTER 1", x=0.31), K("ganho", "LEAD DRIVE", x=0.365), K("volume", "LEAD MASTER", x=0.42),
     K("presenca", "PRESENÇA", estilo="extra", x=0.48, z=0.3)],
    [T("EQ", x=0.77), T("STANDBY", x=0.83), T("POWER", x=0.89), JW((0.15, 0.35, 1.0), x=0.95, raio_m=0.006)],
    [LC("", x=0.48, z=0.78), LC("LEAD", x=0.51, z=0.78)],
    "O PRESENCE do IIC+ fica atrás: aqui é um botão pequeno ao lado do LEAD MASTER. Os LEDs de canal não existem no original.",
)
FICHAS["mark3"] = _mesa_curto(
    "Mark Three",
    ("Mesa/Boogie Mark III: cabeçote curto de 476 × 235 × 264 mm em tolex preto, painel preto em cima com letras e moldura "
     "brancas e a tela preta embaixo: entradas INPUT e FOOT SW., oito botões pretos de saia numerada (VOLUME, TREBLE, BASS, "
     "MIDDLE, MASTER, LEAD DRIVE, LEAD MASTER, REVERB), o EQ gráfico de cinco deslizantes numa janela cinza, as alavancas "
     "EQ, FULL/HALF, STANDBY e POWER e a luz piloto azul."),
    [F("VOLUME", x=0.085), K("agudo", "TREBLE", x=0.135), K("grave", "BASS", x=0.185), K("medio", "MIDDLE", x=0.235),
     F("MASTER", x=0.285), K("ganho", "LEAD DRIVE", x=0.335), K("volume", "LEAD MASTER", x=0.385), F("REVERB", x=0.435),
     K("presenca", "PRESENÇA", estilo="extra", x=0.495, z=0.3)],
    [T("EQ", x=0.765), T("FULL", x=0.815), T("STANDBY", x=0.865), T("POWER", x=0.915), JW((0.15, 0.35, 1.0), x=0.96, raio_m=0.006)],
    [LC("", x=0.475, z=0.78), LC("", x=0.5, z=0.78), LC("", x=0.525, z=0.78)],
    "O PRESENCE do Mark III fica atrás: aqui é um botão pequeno depois do REVERB. Os três LEDs de canal são para a tela.",
    friso=(0.8, 0.8, 0.8),
)

# Mark IV: capas coloridas por canal.
def _cap(c):
    return estilo("liso", 0.016, (0.012, 0.012, 0.012), c, traco=(0.02, 0.02, 0.02) if c[0] > 0.5 else (0.9, 0.9, 0.9), escala="nenhuma")


FICHAS["mark4"] = dict(
    inspiracao=("Mesa/Boogie Mark IV: cabeçote curto de 476 × 235 × 264 mm em tolex preto, painel preto em cima com letras "
                "brancas e a tela preta embaixo: 17 botões pretos em duas fileiras com capas por canal (branca no Rhythm 1, "
                "azul no Rhythm 2, vermelha no Lead; os do Lead são os que tocam), entrada na fileira de baixo, EQ gráfico "
                "de cinco deslizantes em janela, OUTPUT LEVEL vermelho, LEDs R1, R2 e LD, chaves e a luz piloto azul."),
    mm=(476, 235, 264), couro=BRONCO, fachada=MESA_TELA, logo=MESA_LOGO,
    painel=dict(posicao="cima", altura_m=0.09, cor=rgb(0.01, 0.01, 0.011), material="pintado", texto_cor=BRANCO),
    estilos={"padrao": _cap((0.85, 0.85, 0.85)), "azul": _cap((0.1, 0.25, 0.8)), "verm": _cap((0.75, 0.05, 0.03))},
    cantos="cromo", parafusos_painel=False,
    fileiras=[
        ROW(0.74, 0.03, 0.54, F("VOL/GAIN"), F("GAIN", estilo="azul"), F("TREBLE"), F("BASS"), F("MID"),
            F("TREBLE", estilo="azul"), F("PRES."), F("PRES.", estilo="azul"), K("presenca", "PRES.", estilo="verm"),
            letra_m=0.0038, rotulo_pos="cima"),
        ROW(0.27, 0.03, 0.54, J("INPUT"), K("ganho", "GAIN", estilo="verm"), K("agudo", "TREBLE", estilo="verm"),
            K("grave", "BASS", estilo="verm"), K("medio", "MID", estilo="verm"), F("DRIVE", estilo="verm"),
            F("MASTER"), F("MASTER", estilo="azul"), K("volume", "MASTER", estilo="verm"), letra_m=0.0038, rotulo_pos="baixo"),
        _eq(0.585, 0.745),
        ROW(0.55, 0.0, 1.0, F("OUTPUT", estilo="verm", x=0.79), LC("R1", x=0.835, z=0.85), LC("R2", x=0.86, z=0.85),
            LC("LD", x=0.885, z=0.85), T("STANDBY", x=0.86, z=0.4), T("FULL", x=0.91, z=0.4),
            JW((0.15, 0.35, 1.0), x=0.96, raio_m=0.006), letra_m=0.0038, rotulo_pos="baixo"),
    ],
    riscos=[dict(RISCO(0.575, 0.5, 0.755, 0.5, 0.075, cor=JANELA_EQ))],
)

# ------------------------------------------------------------------ Mark JP (JP-2C)
BOOGIE = estilo("cromado", 0.015, (0.012, 0.012, 0.012), (0.02, 0.02, 0.02), traco=(0.85, 0.8, 0.62), escala="nenhuma")


def _jp_canal(x0, n):
    sp = 0.036
    if n == 1:
        top = [K("ganho", "GAIN"), K("volume", "MASTER"), K("presenca", "PRESENCE")]
        bot = [K("agudo", "TREBLE"), K("medio", "MID"), K("grave", "BASS")]
    else:
        top = [F("GAIN"), F("MASTER"), F("PRES")]
        bot = [F("TREBLE"), F("MID"), F("BASS")]
    out = []
    for i, (a, b) in enumerate(zip(top, bot)):
        a.update(x=x0 + i * sp, z=0.66)
        b.update(x=x0 + i * sp, z=0.25, rotulo_pos="baixo")
        out += [a, b]
    out += [LC("", x=x0 + 3 * sp - 0.008, z=0.88), MT(None, x=x0 + 3 * sp - 0.008, z=0.5)]
    return out


FICHAS["markjp"] = dict(
    inspiracao=("Mesa/Boogie JP-2C: cabeçote curto de 476 × 235 × 276 mm em tolex preto Bronco com cantoneiras pretas, painel "
                "preto em cima com letras brancas e a tela preta embaixo: entrada, três blocos de canal lado a lado com GAIN, "
                "MASTER e PRESENCE em cima e TREBLE, MID e BASS embaixo, um LED e uma minichave cada (o canal 1 é o que toca), "
                "dois EQs gráficos de cinco deslizantes lado a lado, minichaves e as alavancas POWER e STANDBY, em botões "
                "pretos recartilhados com risco creme."),
    mm=(476, 235, 276), couro=BRONCO, fachada=MESA_TELA, logo=MESA_LOGO,
    painel=dict(posicao="cima", altura_m=0.09, cor=rgb(0.01, 0.01, 0.011), material="pintado", texto_cor=BRANCO),
    estilos={"padrao": BOOGIE}, cantos="preto", parafusos_painel=False,
    fileiras=[ROW(0.5, 0.0, 1.0, J("INPUT", x=0.03), *_jp_canal(0.07, 1), *_jp_canal(0.215, 2), *_jp_canal(0.36, 3),
                  letra_m=0.0038, rotulo_pos="cima"),
              _eq(0.515, 0.665), _eq(0.685, 0.835, fixo=True),
              ROW(0.5, 0.0, 1.0, MT("SHRED", x=0.86, z=0.75), MT("CH", x=0.86, z=0.3), T("POWER", x=0.91), T("STANDBY", x=0.955),
                  letra_m=0.0038, rotulo_pos="baixo")],
    riscos=[RISCO(0.198, 0.06, 0.198, 0.94), RISCO(0.343, 0.06, 0.343, 0.94), RISCO(0.495, 0.06, 0.495, 0.94),
            RISCO(0.845, 0.06, 0.845, 0.94)],
    notas="O segundo EQ gráfico fica desenhado fixo (o Nexus tem um EQ só).",
)

# ------------------------------------------------------------------ Badland 100 (Badlander)
BADLAND = estilo("marshall", 0.019, (0.012, 0.012, 0.012), (0.8, 0.8, 0.82), traco=(0.02, 0.02, 0.02), escala="tracos")
FICHAS["badland100"] = dict(
    inspiracao=("Mesa/Boogie Badlander: cabeçote de 578 × 244 × 264 mm em tolex preto Taurus com cantoneiras pretas, chapa "
                "xadrez preta na parte de cima com a grelha cromada de fendas no meio e painel preto embaixo com letras "
                "brancas, na ordem invertida do Rectifier: alavancas BOLD/VARIAC e ON/STANDBY, luz piloto vermelha, o nome, "
                "chave de potência, duas fileiras de seis botões pretos de tampa cromada (MASTER, PRESENCE, BASS, MID, "
                "TREBLE, GAIN; a de cima toca), minichaves de modo com LED e as entradas INPUT e FTSW à direita."),
    mm=(578, 244, 264), couro=BRONCO,
    fachada={"tipo": "xadrez", "cor": [0.05, 0.05, 0.055], "passo_mm": 11.0,
             "janela": {"largura_rel": 0.3, "altura_rel": 0.5, "z_rel": 0.55}},
    logo={"lado_rel": 0.0, "altura_m": 0.011, "altura_rel": 0.15},
    painel=dict(posicao="baixo", altura_m=0.09, cor=rgb(0.01, 0.01, 0.011), material="pintado", texto_cor=BRANCO),
    estilos={"padrao": BADLAND}, cantos="preto", parafusos_painel=False,
    fileiras=[
        ROW(0.5, 0.0, 1.0, T("BOLD", x=0.035), T("ON", x=0.075), JW((0.9, 0.05, 0.02), x=0.115, raio_m=0.006),
            M("Badland 100", 0.014, x=0.2, z=0.55, fonte="mesa", cor=(0.8, 0.8, 0.8)), MT("100/50/20", x=0.2, z=0.22),
            letra_m=0.0042, rotulo_pos="baixo"),
        ROW(0.72, 0.29, 0.79, K("volume", "MASTER"), K("presenca", "PRESENCE"), K("grave", "BASS"), K("medio", "MID"),
            K("agudo", "TREBLE"), K("ganho", "GAIN"), letra_m=0.0042, rotulo_pos="cima"),
        ROW(0.25, 0.29, 0.79, F("MASTER"), F("PRESENCE"), F("BASS"), F("MID"), F("TREBLE"), F("GAIN"), letra_m=0.0042,
            rotulo_pos="baixo"),
        ROW(0.5, 0.0, 1.0, MT("CH1", x=0.82, z=0.72), LC("", x=0.845, z=0.72), MT("CH2", x=0.82, z=0.3), LC("", x=0.845, z=0.3),
            LC("", x=0.87, z=0.5), J("INPUT", x=0.91), J("FTSW", x=0.96), letra_m=0.0042, rotulo_pos="baixo"),
    ],
    notas="O Badlander tem LED só nos canais 1 e 2; o terceiro LED é para a tela (Limpo, Crunch, Crush).",
)

# ------------------------------------------------------------------ Lone Star 100
FICHAS["lonestar100"] = dict(
    inspiracao=("Mesa/Boogie Lone Star: cabeçote de 584 × 305 × 292 mm em tolex preto Bronco com cantoneiras pretas, painel "
                "CREME em cima com letras pretas, faixa de tolex com a marca e a tela preta embaixo: entradas INPUT e FT SW, "
                "LED e minichaves de modo, canal 1 em cima (GAIN, TREBLE, MID, BASS, PRESENCE, MASTER; o que toca) e canal 2 "
                "embaixo (DRIVE, GAIN, TREBLE, MID, BASS, PRESENCE, MASTER) em botões pretos, OUTPUT e SOLO, luz piloto e as "
                "alavancas POWER e STANDBY."),
    mm=(584, 305, 292), couro=BRONCO,
    fachada={"tipo": "tela", "cor_a": [0.008, 0.008, 0.008], "cor_b": [0.03, 0.03, 0.03], "trama_mm": 2.4,
             "faixa": {"altura_rel": 0.3}},
    logo={"lado_rel": 0.0, "altura_m": 0.016, "altura_rel": 0.5},
    painel=dict(posicao="cima", altura_m=0.095, cor=rgb(0.78, 0.74, 0.62), material="pintado", texto_cor=PRETA),
    estilos={"padrao": estilo("liso", 0.017, (0.012, 0.012, 0.012), traco=(0.9, 0.9, 0.9), escala="tracos")},
    cantos="preto", parafusos_painel=False,
    fileiras=[
        ROW(0.5, 0.0, 1.0, J("INPUT", x=0.035, z=0.7), J("FT SW", x=0.035, z=0.25, rotulo_pos="baixo"),
            LC("", x=0.075, z=0.72), LC("", x=0.075, z=0.3), MT(None, x=0.1, z=0.72), MT(None, x=0.1, z=0.3),
            letra_m=0.0042, rotulo_pos="baixo"),
        ROW(0.72, 0.17, 0.62, K("ganho", "GAIN"), K("agudo", "TREBLE"), K("medio", "MID"), K("grave", "BASS"),
            K("presenca", "PRESENCE"), K("volume", "MASTER"), MT(None), letra_m=0.0042, rotulo_pos="cima"),
        ROW(0.26, 0.12, 0.62, F("DRIVE"), F("GAIN"), F("TREBLE"), F("MID"), F("BASS"), F("PRESENCE"), F("MASTER"), MT(None),
            letra_m=0.0042, rotulo_pos="baixo"),
        ROW(0.5, 0.0, 1.0, F("OUTPUT", x=0.69, z=0.7), F("SOLO", x=0.69, z=0.26, rotulo_pos="baixo"),
            M("Lone Star", 0.014, x=0.79, z=0.5, fonte="mesa"), JW((0.15, 0.35, 1.0), x=0.865, raio_m=0.006),
            T("POWER", x=0.91, rotulo_pos="baixo"), T("STANDBY", x=0.955, rotulo_pos="baixo"), letra_m=0.0042),
    ],
)

# ------------------------------------------------------------------ Orange OR50 e AD30
LARANJA = rgb(0.85, 0.27, 0.01)
COROA = estilo("cromado", 0.026, (0.015, 0.015, 0.015), (0.03, 0.03, 0.03), traco=(0.9, 0.9, 0.9), escala="tracos")
COROA_P = dict(COROA, diametro_m=0.018)
TOLEX_LARANJA = {"cor": [0.8, 0.22, 0.01], "grao_mm": 1.8}


def _orange_frente(modelo):
    return [PUX(x=0.025, altura_m=0.12), PUX(x=0.975, altura_m=0.12), LOGO(0.017, x=0.4, z=0.72),
            M(modelo, 0.013, x=0.8, z=0.72)]


FICHAS["citrus50"] = dict(
    inspiracao=("Orange OR50: cabeçote de 550 × 270 × 240 mm em tolex laranja com cantoneiras pretas; a frente inteira é "
                "uma placa branca embutida com os puxadores cromados nas pontas, a marca em cima e a faixa laranja com os "
                "pictogramas: bloco preto com POWER, STANDBY e a lâmpada âmbar, MASTER e HF DRIVE grandes, TREBLE, MIDDLE "
                "e BASS pequenos, GAIN grande, em botões pretos de saia em coroa, e o bloco preto com a entrada e o jaque "
                "do pedal."),
    mm=(550, 270, 240), couro=TOLEX_LARANJA, fachada=None,
    painel=dict(posicao="inteiro", cor=rgb(0.84, 0.84, 0.82), material="pintado", texto_cor=PRETA),
    estilos={"padrao": COROA, "peq": COROA_P}, cantos="preto",
    fileiras=[ROW(0.68, 0.0, 1.0, *_orange_frente("CITRUS 50")),
              ROW(0.4, 0.0, 1.0, TX("ON", x=0.08), TX("PLAY", x=0.13), TX("MASTER", x=0.25), TX("HF DRIVE", x=0.37),
                  TX("TREBLE", x=0.48), TX("MIDDLE", x=0.56), TX("BASS", x=0.64), TX("GAIN", x=0.76), TX("INPUT", x=0.9),
                  letra_m=0.0062),
              ROW(0.18, 0.0, 1.0, T(None, x=0.08), T(None, x=0.13), JW((0.95, 0.5, 0.05), x=0.18, raio_m=0.006),
                  K("volume", "", x=0.25), K("presenca", "", x=0.37), K("agudo", "", estilo="peq", x=0.48),
                  K("medio", "", estilo="peq", x=0.56), K("grave", "", estilo="peq", x=0.64), K("ganho", "", x=0.76),
                  J(None, x=0.885), J(None, x=0.935))],
    riscos=[RISCO(0.05, 0.4, 0.95, 0.4, 0.024, cor=LARANJA), RISCO(0.05, 0.17, 0.205, 0.17, 0.05, cor=(0.012, 0.012, 0.012)),
            RISCO(0.86, 0.17, 0.96, 0.17, 0.05, cor=(0.012, 0.012, 0.012))],
    notas="A presença do Nexus ficou no HF DRIVE do OR50 (que realça os agudos do ganho).",
)
FICHAS["citrus30"] = dict(
    inspiracao=("Orange AD30 Twin Channel: cabeçote de 550 × 270 × 240 mm em tolex laranja com cantoneiras pretas; a frente "
                "inteira é uma placa branca embutida com os puxadores cromados, a marca e o nome em cima e a faixa laranja: "
                "chaves POWER, WARMDOWN/PLAY e de canal com a lâmpada âmbar, dez botões pretos em duas fileiras desencontradas "
                "(MASTER, TREBLE, MIDDLE, BASS e GAIN do canal 2 em cima, que é o que toca, e do canal 1 embaixo) e a entrada."),
    mm=(550, 270, 240), couro=TOLEX_LARANJA, fachada=None,
    painel=dict(posicao="inteiro", cor=rgb(0.84, 0.84, 0.82), material="pintado", texto_cor=PRETA),
    estilos={"padrao": dict(COROA, diametro_m=0.022), "peq": dict(COROA, diametro_m=0.016), "extra": EXTRA},
    cantos="preto",
    fileiras=[ROW(0.72, 0.0, 1.0, *_orange_frente("CITRUS 30")),
              ROW(0.46, 0.0, 1.0, TX("MASTER", x=0.3), TX("TREBLE", x=0.42), TX("MIDDLE", x=0.54), TX("BASS", x=0.66),
                  TX("GAIN", x=0.78), letra_m=0.0058),
              ROW(0.3, 0.0, 1.0, T(None, x=0.07, z=0.3), T(None, x=0.115, z=0.3), JW((0.95, 0.5, 0.05), x=0.16, z=0.3, raio_m=0.006),
                  T(None, x=0.205, z=0.3), LC("", x=0.205, z=0.12), LC("", x=0.205, z=0.21),
                  K("volume", "", x=0.3), K("agudo", "", estilo="peq", x=0.42), K("medio", "", estilo="peq", x=0.54),
                  K("grave", "", estilo="peq", x=0.66), K("ganho", "", x=0.78),
                  F("", x=0.36, z=0.12), F("", estilo="peq", x=0.48, z=0.12), F("", estilo="peq", x=0.6, z=0.12),
                  F("", estilo="peq", x=0.72, z=0.12), F("", x=0.84, z=0.12),
                  K("presenca", "PRESENÇA", estilo="extra", x=0.86, z=0.32), J(None, x=0.93, z=0.2), letra_m=0.004)],
    riscos=[RISCO(0.05, 0.46, 0.95, 0.46, 0.022, cor=LARANJA)],
    notas="O AD30 não tem presença: ficou num botão pequeno à direita, perto da entrada. Os LEDs de canal não existem no original.",
)

# ------------------------------------------------------------------ Top Boost 30 (AC30CH, painel no tampo)
VOX_BICO = estilo("chicken", 0.02, (0.015, 0.015, 0.015), traco=(0.85, 0.85, 0.85), escala="tracos")
FICHAS["topboost30"] = dict(
    inspiracao=("Vox AC30CH: cabeçote de 705 × 284 × 266 mm em vinil preto com cantoneiras pretas, tela vinho com losangos "
                "e friso creme cobrindo a frente, com a marca à esquerda, e o painel vinho no TAMPO, virado para trás, com "
                "letras e molduras prateadas: quatro entradas (NORMAL e TOP BOOST), dez botões pretos bico-de-galinha (NORMAL "
                "VOLUME, TOP BOOST VOLUME, TREBLE, BASS, REVERB TONE e LEVEL, TREMOLO SPEED e DEPTH, TONE CUT e MASTER VOLUME) "
                "e as alavancas STANDBY e POWER com os LEDs à direita."),
    mm=(705, 284, 266), couro={"cor": [0.008, 0.008, 0.008], "grao_mm": 1.6},
    fachada={"tipo": "losangos", "cor": [0.17, 0.035, 0.03], "linha": [0.55, 0.52, 0.36], "passo_mm": 46.0,
             "friso": [0.85, 0.84, 0.8]},
    logo={"lado_rel": -0.3, "altura_rel": 0.78, "altura_m": 0.018},
    painel=dict(posicao="topo", altura_m=0.075, largura_m=0.6, cor=rgb(0.22, 0.03, 0.035), material="pintado",
                texto_cor=rgb(0.8, 0.8, 0.82)),
    estilos={"padrao": VOX_BICO}, cantos="preto",
    fileiras=[ROW(0.45, 0.0, 1.0, J(x=0.035, z=0.7), J(x=0.035, z=0.25), J(x=0.08, z=0.7), J(x=0.08, z=0.25),
                  K("medio", "VOLUME", x=0.14), K("ganho", "VOLUME", x=0.2), K("agudo", "TREBLE", x=0.26),
                  K("grave", "BASS", x=0.32), F("TONE", x=0.39), F("LEVEL", x=0.45), F("SPEED", x=0.52), F("DEPTH", x=0.58),
                  K("presenca", "TONE CUT", x=0.66), K("volume", "VOLUME", x=0.72),
                  L(None, (0.1, 0.8, 0.1), aceso=True, x=0.8, z=0.85), T("STANDBY", x=0.8),
                  L(None, (0.9, 0.05, 0.02), aceso=True, x=0.87, z=0.85), T("POWER", x=0.87),
                  letra_m=0.0052, rotulo_pos="baixo"),
              ROW(0.92, 0.0, 1.0, TX("NORMAL", x=0.14), TX("TOP BOOST", x=0.26), TX("REVERB", x=0.42), TX("TREMOLO", x=0.55),
                  TX("MASTER", x=0.69), letra_m=0.0052)],
    riscos=[RISCO(x, 0.06, x, 0.94, cor=(0.75, 0.75, 0.77)) for x in (0.11, 0.17, 0.355, 0.485, 0.62, 0.755)],
    notas="O AC30 não tem médio: o médio do Nexus ficou no NORMAL VOLUME; a presença é o TONE CUT.",
)

# ------------------------------------------------------------------ Match 30 (DC-30, combo)
CREME_BICO = estilo("chicken", 0.022, (0.8, 0.76, 0.62), traco=(0.02, 0.02, 0.02), escala="tracos")
FICHAS["match30"] = dict(
    inspiracao=("Matchless DC-30: combo 2×12 de 686 × 533 × 267 mm em tolex preto de couro de elefante, painel preto recuado "
                "no alto da frente com letras brancas iluminado por trás, faixa preta com a marca acima da tela e a tela "
                "preta de fio prateado com friso prata: entradas 1 e 2 de cada canal, sete botões creme bico-de-galinha "
                "(VOLUME, BASS e TREBLE do canal 1; VOLUME, TONE de seis posições, CUT e MASTER), a joia vermelha e as "
                "alavancas STANDBY e ON."),
    formato="combo", mm=(686, 533, 267), couro={"cor": [0.008, 0.008, 0.008], "grao_mm": 4.0},
    fachada={"tipo": "tela", "cor_a": [0.02, 0.02, 0.022], "cor_b": [0.42, 0.42, 0.44], "trama_mm": 2.0,
             "friso": [0.72, 0.72, 0.74], "faixa": {"altura_rel": 0.14}},
    logo={"lado_rel": 0.0, "altura_m": 0.016, "altura_rel": 0.5},
    painel=dict(posicao="cima", altura_m=0.075, cor=rgb(0.01, 0.01, 0.011), material="pintado", texto_cor=BRANCO),
    estilos={"padrao": CREME_BICO}, cantos="nenhum",
    fileiras=[ROW(0.48, 0.0, 1.0, J("1", x=0.04), J("2", x=0.08), K("ganho", "VOLUME", x=0.14), K("grave", "BASS", x=0.2),
                  K("agudo", "TREBLE", x=0.26), J("1", x=0.33), J("2", x=0.37), F("VOLUME", x=0.43),
                  K("medio", "TONE", x=0.49), K("presenca", "CUT", x=0.55), K("volume", "MASTER", x=0.61),
                  M("MATCH 30", 0.012, x=0.72, z=0.5, cor=(0.85, 0.85, 0.85)), JW((0.9, 0.05, 0.02), x=0.8, raio_m=0.007),
                  T("STANDBY", x=0.87), T("ON", x=0.93), letra_m=0.0055, rotulo_pos="baixo")],
    notas="O TONE do DC-30 é chave de seis posições; aqui é o médio do Nexus, girando contínuo. A presença é o CUT.",
)
