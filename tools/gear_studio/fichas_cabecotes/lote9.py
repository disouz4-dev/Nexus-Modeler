# Fichas do lote 9: os últimos cabeçotes de baixo (SWR, Mesa 800D, Acoustic, Orange, Trace, Ashdown, Traynor, EBS, Peavey, Behringer, Aguilar).
from cabecotes_fiel import *  # noqa: F401,F403

TOLEX_PRETO = {"cor": [0.006, 0.006, 0.006], "grao_mm": 2.6}
BRANCO = rgb(0.85, 0.85, 0.85)
PRETA = rgb(0.012, 0.012, 0.012)
EXTRA = estilo("liso", 0.011, (0.015, 0.015, 0.015), traco=(0.9, 0.9, 0.9), escala="nenhuma")
LISO = estilo("liso", 0.016, (0.015, 0.015, 0.015), traco=(0.9, 0.9, 0.9), escala="tracos")
FICHAS = {}

BORDO = rgb(0.28, 0.03, 0.035)
FICHAS["workhorse350"] = dict(
    inspiracao=("SWR Bass 350: rack 2U (483 × 89 × 257 mm) de chapa preta com seções bordô, letras brancas e alças tubulares: "
                "entradas PASSIVE/ACTIVE e ACTIVE, oito botões pretos serrilhados (GAIN, AURAL ENHANCER, BASS, MID-RANGE LEVEL "
                "e FREQUENCY, TREBLE, EFFECTS BLEND, MASTER VOLUME), os LEDs CLIP e LIMITER, a placa com o selo e as gangorras "
                "INTERNAL FAN e POWER."),
    formato="rack", rack_u=2, mm=(483, 89, 257), couro={"cor": [0.02, 0.02, 0.022], "metal": True}, fachada=None, alca="rack",
    painel=dict(posicao="inteiro", cor=rgb(0.012, 0.012, 0.013), material="pintado", texto_cor=BRANCO),
    estilos={"padrao": estilo("cromado", 0.017, (0.015, 0.015, 0.015), (0.03, 0.03, 0.03), traco=(0.9, 0.9, 0.9), escala="tracos")},
    fileiras=[ROW(0.45, 0.0, 1.0, J("PASSIVE", x=0.06), J("ACTIVE", x=0.11), K("ganho", "GAIN", x=0.18),
                  K("presenca", "ENHANCER", x=0.24), K("grave", "BASS", x=0.3), K("medio", "MID LEVEL", x=0.36), F("MID FREQ", x=0.42),
                  K("agudo", "TREBLE", x=0.48), F("BLEND", x=0.54), K("volume", "MASTER", x=0.6),
                  L("CLIP", (0.9, 0.05, 0.02), x=0.65, z=0.75), L("LIMIT", (0.95, 0.75, 0.05), x=0.65, z=0.3),
                  LOGO(0.006, x=0.75, z=0.5), RB("FAN", x=0.86), R("POWER", x=0.92), letra_m=0.0036, rotulo_pos="baixo")],
    riscos=[dict(RISCO(0.03, 0.5, 0.14, 0.5, 0.065, cor=BORDO)), dict(RISCO(0.69, 0.5, 0.81, 0.5, 0.065, cor=BORDO))],
    notas="Feito como o Bass 350 de 1994–99 (seções bordô).",
)

FICHAS["underground800"] = dict(
    inspiracao=("Mesa/Boogie Subway D-800 (Bass 800D): caixinha de metal preta de 270 × 80 × 286 mm com as aberturas de "
                "ventilação à esquerda da frente e a alça de metal na lateral direita: entrada, as chavinhas MUTE, PAD e DEEP "
                "com LEDs, sete botões pretos de risco branco (INPUT, VOICING, BASS, LO MID, HI MID, TREBLE, MASTER) e os LEDs "
                "de estado."),
    formato="caixinha", mm=(270, 80, 286), couro={"cor": [0.02, 0.02, 0.022], "metal": True, "aspereza": 0.6}, fachada=None,
    alca="nenhuma",
    painel=dict(posicao="inteiro", cor=rgb(0.012, 0.012, 0.013), material="pintado", texto_cor=BRANCO),
    estilos={"padrao": dict(LISO, diametro_m=0.013), "extra": dict(EXTRA, diametro_m=0.009)}, parafusos_painel=False,
    fileiras=[ROW(0.48, 0.0, 1.0, J("INPUT", x=0.15), MT("MUTE", x=0.21, z=0.75), MT("PAD", x=0.21, z=0.25), MT("DEEP", x=0.25, z=0.5),
                  K("ganho", "INPUT", x=0.31), F("VOICING", x=0.39), K("grave", "BASS", x=0.47), K("medio", "LO MID", x=0.55),
                  K("presenca", "HI MID", x=0.63), K("agudo", "TREBLE", x=0.71), K("volume", "MASTER", x=0.79),
                  L(None, (0.1, 0.8, 0.1), aceso=True, x=0.85, z=0.7), PUX(x=0.94, altura_m=0.05), letra_m=0.003, rotulo_pos="baixo")],
    riscos=[dict(RISCO(0.03 + k * 0.018, 0.2, 0.03 + k * 0.018, 0.8, 0.004, cor=(0.0, 0.0, 0.0))) for k in range(5)],
    notas="A presença do Nexus ficou no HI MID.",
)

FICHAS["acoustica360"] = dict(
    inspiracao=("Acoustic 360: cabeçote de 660 × 230 × 280 mm em tolex preto com cantoneiras cromadas e a frente numa moldura de "
                "alumínio escovado: faixa preta com nove botões cromados de letras brancas (VOLUME, TREBLE, BASS, VARIAMP de "
                "balanço e de efeito, FUZZ ATTACK e GAIN, TUNING grosso e fino), as entradas HIGH e LOW com a chave BRIGHT, "
                "a chavinha do diapasão, as chaves e a luz piloto âmbar, e embaixo outra faixa preta com a marca."),
    mm=(660, 230, 280), couro=TOLEX_PRETO, fachada={"tipo": "tolex", "cor": [0.012, 0.012, 0.013], "grao_mm": 1.2},
    logo={"altura_m": 0.02, "altura_rel": 0.4},
    painel=dict(posicao="cima", altura_m=0.1, cor=rgb(0.012, 0.012, 0.013), material="pintado", texto_cor=BRANCO,
                friso=(0.7, 0.71, 0.72)),
    estilos={"padrao": estilo("cromado", 0.019, (0.8, 0.8, 0.82), (0.88, 0.88, 0.9), traco=(0.02, 0.02, 0.02), escala="tracos", metal=True),
             "extra": EXTRA},
    cantos="cromo",
    fileiras=[ROW(0.45, 0.0, 1.0, J("HIGH", x=0.04), J("LOW", x=0.09), MT("BRIGHT", x=0.13),
                  K("volume", "VOLUME", x=0.19), K("agudo", "TREBLE", x=0.25), K("grave", "BASS", x=0.31), K("medio", "VARIAMP", x=0.37),
                  F("EFFECT", x=0.43), F("ATTACK", x=0.49), K("ganho", "FUZZ", x=0.55), F("TUNING", x=0.61), F("FINE", x=0.67),
                  K("presenca", "PRESENÇA", estilo="extra", x=0.72, z=0.7), MT(None, x=0.76), T(None, x=0.83), T(None, x=0.88),
                  JW((0.95, 0.5, 0.05), x=0.94, raio_m=0.006), letra_m=0.0048, rotulo_pos="baixo")],
    notas="O 360 não tem médio nem presença: o médio ficou no VARIAMP (balanço) e a presença num botão pequeno; o ganho é o FUZZ GAIN.",
)

ORANGE_BOTAO = estilo("cromado", 0.024, (0.015, 0.015, 0.015), (0.03, 0.03, 0.03), traco=(0.9, 0.9, 0.9), escala="tracos")
FICHAS["tinybass"] = dict(
    inspiracao=("Orange Terror Bass: caixinha de aço branco de 307 × 190 × 160 mm com alça cromada; frente branca com a marca "
                "grande e o nome em cima e a faixa de blocos preto e laranja com os nomes: chave OFF/WARMUP/ON, joia laranja, "
                "VOLUME e GAIN grandes, TREBLE, MIDDLE e BASS menores em botões pretos, as chavinhas CLEAN e PAD e a entrada."),
    formato="caixinha", mm=(307, 190, 160), couro={"cor": [0.82, 0.82, 0.8], "metal": True, "aspereza": 0.35}, fachada=None,
    alca="barra",
    painel=dict(posicao="inteiro", cor=rgb(0.86, 0.86, 0.84), material="pintado", texto_cor=PRETA),
    estilos={"padrao": ORANGE_BOTAO, "peq": dict(ORANGE_BOTAO, diametro_m=0.017), "extra": EXTRA},
    fileiras=[ROW(0.78, 0.0, 1.0, LOGO(0.0125, x=0.36, z=0.75), M("TINY BASS", 0.011, x=0.74, z=0.75),
                  K("presenca", "PRESENÇA", estilo="extra", x=0.9, z=0.75), letra_m=0.0034, rotulo_pos="baixo"),
              ROW(0.5, 0.0, 1.0, TX("POWER", x=0.08), TX("VOLUME", x=0.22), TX("TREBLE", x=0.36), TX("MIDDLE", x=0.47),
                  TX("BASS", x=0.58), TX("GAIN", x=0.72), TX("INPUT", x=0.92), letra_m=0.0052),
              ROW(0.22, 0.0, 1.0, T(None, x=0.06), JW((0.95, 0.4, 0.05), x=0.13, raio_m=0.006), K("volume", "", x=0.22),
                  K("agudo", "", estilo="peq", x=0.36), K("medio", "", estilo="peq", x=0.47), K("grave", "", estilo="peq", x=0.58),
                  K("ganho", "", x=0.72), MT("CLEAN", x=0.8), MT("PAD", x=0.86), J(None, x=0.93), letra_m=0.004, rotulo_pos="baixo")],
    riscos=[RISCO(0.0, 0.5, 1.0, 0.5, 0.03, cor=(0.01, 0.01, 0.01)), RISCO(0.0, 0.43, 1.0, 0.43, 0.012, cor=(0.85, 0.27, 0.01))],
    notas="O Terror Bass não tem presença: ficou num botão pequeno à direita do nome.",
)

VERDE = rgb(0.2, 0.9, 0.3)
FICHAS["tracer7"] = dict(
    inspiracao=("Trace Elliot GP7: cabeçote de rack 3U num case forrado de carpete preto (491 × 157 × 335 mm) com cantoneiras e "
                "alças de metal, painel preto com serigrafia verde: entrada, INPUT GAIN de tampa vermelha, HI/LO e PRE SHAPE, o "
                "quadrado verde com a marca, o EQ gráfico de sete deslizantes acesos em verde, COMPRESSOR de tampa preta, OUTPUT "
                "de tampa verde, as teclas de EQ, COMPRESSOR e MUTE e as tomadas à direita."),
    formato="rack", rack_u=3, mm=(491, 157, 335), couro={"cor": [0.03, 0.03, 0.032], "metal": True, "aspereza": 0.9},
    fachada=None, alca="rack",
    painel=dict(posicao="inteiro", cor=rgb(0.012, 0.012, 0.013), material="pintado", texto_cor=VERDE),
    estilos={"padrao": estilo("marshall", 0.018, (0.015, 0.015, 0.015), (0.7, 0.05, 0.03), escala="tracos", tampa_plastico=True),
             "preto": estilo("marshall", 0.018, (0.015, 0.015, 0.015), (0.03, 0.03, 0.03), escala="tracos", tampa_plastico=True),
             "verde": estilo("marshall", 0.018, (0.015, 0.015, 0.015), (0.1, 0.6, 0.15), escala="tracos", tampa_plastico=True),
             "extra": EXTRA},
    fileiras=[ROW(0.45, 0.0, 1.0, J("INPUT", x=0.05), K("ganho", "GAIN", x=0.11), P("HI/LO", x=0.16, z=0.7, tamanho_m=0.006),
                  P("SHAPE", x=0.16, z=0.3, tamanho_m=0.006), LOGO(0.008, x=0.23, z=0.5, escuro=False),
                  K("presenca", "COMP", estilo="preto", x=0.72), K("volume", "OUTPUT", estilo="verde", x=0.78),
                  K("grave", "GRAVE", estilo="extra", x=0.31, z=0.85), K("medio", "MÉDIO", estilo="extra", x=0.35, z=0.85),
                  K("agudo", "AGUDO", estilo="extra", x=0.39, z=0.85),
                  P("EQ", x=0.83, z=0.7, tamanho_m=0.006), P("COMP", x=0.83, z=0.3, tamanho_m=0.006), P("MUTE", x=0.87, tamanho_m=0.006),
                  J("SEND", x=0.92, z=0.7), J("RET", x=0.92, z=0.3), letra_m=0.0036, rotulo_pos="baixo"),
              ROW(0.45, 0.3, 0.66, SF("40"), SF("80"), SF("160"), SF("320"), SF("640"), SF("1K2"), SF("5K"), curso_m=0.05,
                  letra_m=0.0034, rotulo_pos="baixo")],
    riscos=[dict(RISCO(0.2, 0.5, 0.26, 0.5, 0.05, cor=(0.1, 0.55, 0.15)))],
    notas="O EQ de sete deslizantes ficou desenhado fixo; grave, médio e agudo do Nexus ficaram em três botões pequenos acima dele. A presença é o COMPRESSOR.",
)

CINZA_AZUL = rgb(0.32, 0.38, 0.44)
FICHAS["ash500"] = dict(
    inspiracao=("Ashdown ABM EVO III: cabeçote baixo e compacto de 474 × 134 × 315 mm em tolex preto, sem grade: a frente "
                "inteira é o painel cinza-azulado com letras claras e o VU de fundo âmbar aceso à esquerda; entrada com chave "
                "passivo/ativo, INPUT, BASS, dois deslizantes, MIDDLE, dois deslizantes, TREBLE e OUTPUT em cima, VALVE DRIVE, "
                "COMPRESSION e SUB HARMONICS embaixo, as teclas de liga dos efeitos e a saída D.I."),
    mm=(474, 134, 315), couro=TOLEX_PRETO, fachada=None, aba_m=0.012, recuo_frente_m=0.02,
    painel=dict(posicao="inteiro", cor=CINZA_AZUL, material="pintado", texto_cor=BRANCO),
    estilos={"padrao": LISO, "extra": EXTRA}, cantos="preto", alca="couro",
    fileiras=[ROW(0.68, 0.0, 1.0, VU(None, x=0.1, largura_m=0.06, altura_m=0.042), J("INPUT", x=0.21), K("ganho", "INPUT", x=0.27),
                  K("grave", "BASS", x=0.35), SF("180", x=0.4), SF("340", x=0.44), K("medio", "MIDDLE", x=0.5),
                  SF("1K", x=0.55), SF("3K", x=0.59), K("agudo", "TREBLE", x=0.65), K("volume", "OUTPUT", x=0.85),
                  letra_m=0.0036, rotulo_pos="baixo", curso_m=0.026),
              ROW(0.22, 0.0, 1.0, LOGO(0.008, x=0.1, z=0.2, escuro=False), F("DRIVE", x=0.35), K("presenca", "COMP", x=0.5),
                  F("SUB", x=0.65), P(None, x=0.4, tamanho_m=0.006), P(None, x=0.55, tamanho_m=0.006), P(None, x=0.7, tamanho_m=0.006),
                  J("D.I.", x=0.8), P("MUTE", x=0.86, tamanho_m=0.006), letra_m=0.0036, rotulo_pos="baixo")],
    notas="Os quatro deslizantes do EQ ficaram desenhados fixos. A presença do Nexus é o COMPRESSION.",
)

FICHAS["maple400"] = dict(
    inspiracao=("Traynor YBA-200: cabeçote de 570 × 240 × 290 mm em tolex preto com cantos cromados, painel preto na faixa de "
                "cima com letras brancas e a grade de metal preta perfurada embaixo com a marca: entradas 0 dB e -6 dB, MUTE "
                "com LED, BRIGHT, GAIN, TREBLE, MID, RANGE, BASS, SCOOP, RESONANCE e MASTER em botões pretos, as conexões, a "
                "lâmpada âmbar e as alavancas STANDBY e POWER."),
    mm=(570, 240, 290), couro=TOLEX_PRETO,
    fachada={"tipo": "grade", "cor": [0.02, 0.02, 0.022], "furo_mm": 3.6, "passo_mm": 5.0},
    logo={"altura_m": 0.02, "altura_rel": 0.5},
    painel=dict(posicao="cima", altura_m=0.075, cor=rgb(0.012, 0.012, 0.013), material="pintado", texto_cor=BRANCO),
    estilos={"padrao": LISO}, cantos="cromo",
    fileiras=[ROW(0.45, 0.0, 1.0, J("0 dB", x=0.035), J("-6 dB", x=0.075), P("MUTE", x=0.11, tamanho_m=0.006), MT("BRIGHT", x=0.145),
                  K("ganho", "GAIN", x=0.2), K("agudo", "TREBLE", x=0.26), K("medio", "MID", x=0.32), F("RANGE", x=0.38),
                  K("grave", "BASS", x=0.44), F("SCOOP", x=0.5), K("presenca", "RESONANCE", x=0.56), K("volume", "MASTER", x=0.62),
                  J("PRE OUT", x=0.68), J("AMP IN", x=0.72), J("LINE", x=0.76), JW((0.95, 0.5, 0.05), x=0.82, raio_m=0.006),
                  T("STANDBY", x=0.88), T("POWER", x=0.94), letra_m=0.0042, rotulo_pos="baixo")],
    notas="A presença do Nexus ficou no RESONANCE.",
)

FICHAS["nord650"] = dict(
    inspiracao=("EBS Classic 500: cabeçote de 490 × 215 × 300 mm em tolex preto com cantos cromados e alça de couro, faixa preta "
                "lisa no alto com a marca, tela de pano cinza-escura no meio e a faixa preta de painel embaixo com letras brancas: "
                "entrada, GAIN com a tecla CHARACTER e o LED PEAK, BASS, LOW MID e a frequência, HIGH MID e a frequência, TREBLE, "
                "DRIVE e VOLUME em botões pretos, as alavancas STANDBY e POWER e a lâmpada vermelha grande."),
    mm=(490, 215, 300), couro=TOLEX_PRETO,
    fachada={"tipo": "tela", "cor_a": [0.05, 0.05, 0.055], "cor_b": [0.1, 0.1, 0.11], "trama_mm": 1.8,
             "faixa": {"altura_rel": 0.35, "cor": [0.008, 0.008, 0.009], "lisa": True}},
    logo={"altura_m": 0.016, "altura_rel": 0.5},
    painel=dict(posicao="baixo", altura_m=0.07, cor=rgb(0.012, 0.012, 0.013), material="pintado", texto_cor=BRANCO),
    estilos={"padrao": LISO, "peq": dict(LISO, diametro_m=0.012)}, cantos="cromo",
    fileiras=[ROW(0.45, 0.0, 1.0, J("INPUT", x=0.04), K("ganho", "GAIN", x=0.1), P("CHAR.", x=0.145, tamanho_m=0.006),
                  K("grave", "BASS", x=0.2), K("medio", "LOW MID", x=0.26), F("FREQ", estilo="peq", x=0.31),
                  K("presenca", "HIGH MID", x=0.36), F("FREQ", estilo="peq", x=0.41), K("agudo", "TREBLE", x=0.46),
                  F("DRIVE", x=0.53), K("volume", "VOLUME", x=0.6), M("NORD 650", 0.01, x=0.7, z=0.5),
                  T("STANDBY", x=0.79), T("POWER", x=0.85), JW((0.9, 0.05, 0.02), x=0.93, raio_m=0.008),
                  letra_m=0.0042, rotulo_pos="baixo")],
    notas="A presença do Nexus ficou no HIGH MID.",
)

FICHAS["wind300"] = dict(
    inspiracao=("Peavey Mark III Bass: cabeçote baixo de cerca de 600 × 200 × 280 mm em tolex preto com cantos cromados e a frente "
                "toda de painel preto com letras brancas: entradas dos dois canais e a AUTOMIX, canal 1 (VOLUME, BASS, TREBLE "
                "com BRIGHT), canal 2 (VOLUME, BASS, MID, TREBLE; o que toca), COMPRESSOR, MASTER, as chaves e a lâmpada vermelha, "
                "em botões de tampa cromada."),
    mm=(600, 200, 280), couro=TOLEX_PRETO, fachada=None,
    painel=dict(posicao="inteiro", cor=rgb(0.012, 0.012, 0.013), material="pintado", texto_cor=BRANCO),
    estilos={"padrao": estilo("marshall", 0.019, (0.015, 0.015, 0.015), (0.78, 0.78, 0.8), escala="tracos")},
    cantos="cromo",
    fileiras=[ROW(0.62, 0.0, 1.0, LOGO(0.016, x=0.5, z=0.88)),
              ROW(0.42, 0.0, 1.0, J("1", x=0.04, z=0.6), J("2", x=0.04, z=0.25), F("VOLUME", x=0.1), F("BASS", x=0.16),
                  F("TREBLE", x=0.22), MT("BRIGHT", x=0.265), J("1", x=0.31, z=0.6), J("2", x=0.31, z=0.25),
                  K("ganho", "VOLUME", x=0.37), K("grave", "BASS", x=0.43), K("medio", "MID", x=0.49), K("agudo", "TREBLE", x=0.55),
                  J("AUTOMIX", x=0.61), K("presenca", "COMPRESSOR", x=0.68), K("volume", "MASTER", x=0.75),
                  MT("PUNCH", x=0.8), JW((0.9, 0.05, 0.02), x=0.86, raio_m=0.0065), T("POWER", x=0.93), letra_m=0.0048, rotulo_pos="baixo")],
    notas="Confiança baixa: o Mark III de baixo não tem medidas nem painel confirmados do fabricante; feito pela foto de época. A presença do Nexus ficou no COMPRESSOR.",
)

FICHAS["bassboxv"] = dict(
    inspiracao=("Behringer Bass V-Amp Pro: rack 2U (483 × 89 × 135 mm) de painel prata com desenhos e letras pretos: tecla POWER "
                "quadrada à esquerda, GAIN, VOLUME, BASS, MID e TREBLE, o seletor AMPS no alto, o visor de LED vermelho de dois "
                "dígitos no centro com as teclas A a E, TUNER, BANK e TAP, REVERB e EFFECTS, MASTER, a chave LINE IN, a entrada "
                "e o fone à direita."),
    formato="rack", rack_u=2, mm=(483, 89, 135), couro={"cor": [0.03, 0.03, 0.032], "metal": True}, fachada=None,
    painel=dict(posicao="inteiro", cor=rgb(0.66, 0.67, 0.68), texto_cor=PRETA),
    estilos={"padrao": LISO, "extra": EXTRA},
    fileiras=[ROW(0.5, 0.0, 1.0, P("POWER", x=0.04, tamanho_m=0.01), K("ganho", "GAIN", x=0.1), K("volume", "VOLUME", x=0.16),
                  K("grave", "BASS", x=0.22), K("medio", "MID", x=0.28), K("agudo", "TREBLE", x=0.34), F("AMPS", x=0.41, z=0.72),
                  P("A", x=0.4, z=0.2, tamanho_m=0.005), P("B", x=0.43, z=0.2, tamanho_m=0.005), P("C", x=0.46, z=0.2, tamanho_m=0.005),
                  P("D", x=0.49, z=0.2, tamanho_m=0.005), P("E", x=0.52, z=0.2, tamanho_m=0.005),
                  P("TUNER", x=0.58, z=0.2, tamanho_m=0.005), P("TAP", x=0.62, z=0.2, tamanho_m=0.005),
                  K("presenca", "REVERB", x=0.67), F("EFFECTS", x=0.73), F("MASTER", x=0.79), MT("LINE IN", x=0.84),
                  J("INPUT", x=0.89), J("PHONES", x=0.94), letra_m=0.0036, rotulo_pos="baixo")],
    riscos=[dict(RISCO(0.45, 0.65, 0.55, 0.65, 0.026, cor=(0.01, 0.01, 0.01))), dict(RISCO(0.47, 0.65, 0.53, 0.65, 0.016, cor=(0.9, 0.03, 0.01)), luz=1.5)],
    notas="A presença do Nexus ficou no REVERB/ATTACK; o MASTER do original ficou fixo e o volume do Nexus é o VOLUME.",
)

FICHAS["eaglehammer500"] = dict(
    inspiracao=("Aguilar Tone Hammer 500: caixinha de metal de 273 × 76 × 216 mm de base preta e painel prata com moldura "
                "arredondada e letras pretas: entrada com a tecla -10 dB, botões pretos em duas fileiras desencontradas (GAIN, "
                "MID LEVEL e BASS em cima; DRIVE, MID FREQ e TREBLE embaixo), o VOLUME separado à direita, a saída D.I. com "
                "as teclas POST/PRE e LIFT, MUTE e os LEDs STATUS e OPERATE."),
    formato="caixinha", mm=(273, 76, 216), couro={"cor": [0.02, 0.02, 0.022], "metal": True}, fachada=None, alca="nenhuma",
    painel=dict(posicao="inteiro", cor=rgb(0.66, 0.67, 0.68), texto_cor=PRETA),
    estilos={"padrao": dict(LISO, diametro_m=0.013), "extra": dict(EXTRA, diametro_m=0.008)}, parafusos_painel=False,
    fileiras=[ROW(0.7, 0.0, 1.0, J("INPUT", x=0.06), K("ganho", "GAIN", x=0.2), K("medio", "MID", x=0.36), K("grave", "BASS", x=0.52),
                  K("volume", "VOLUME", x=0.72), L("STATUS", (0.1, 0.3, 1.0), aceso=True, x=0.88, z=0.75), letra_m=0.0028, rotulo_pos="cima"),
              ROW(0.28, 0.0, 1.0, P("-10dB", x=0.06, tamanho_m=0.005), F("DRIVE", x=0.28), F("FREQ", x=0.44), K("agudo", "TREBLE", x=0.6),
                  K("presenca", "PRESENÇA", estilo="extra", x=0.68), P("MUTE", x=0.8, tamanho_m=0.005), J("D.I.", x=0.9),
                  letra_m=0.0028, rotulo_pos="baixo")],
    notas="O Tone Hammer não tem presença: ficou num botão pequeno ao lado do TREBLE. O DRIVE do original ficou fixo (o ganho do Nexus é o GAIN).",
)
