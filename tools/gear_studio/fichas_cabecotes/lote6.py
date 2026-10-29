# Fichas do lote 6: Two-Rock, Bad Cat, Hiwatt, Blackstar, Revv, Fortin, Tone King, Randall, PRS, Victory, Supro, Fryette.
from cabecotes_fiel import *  # noqa: F401,F403

TOLEX_PRETO = {"cor": [0.006, 0.006, 0.006], "grao_mm": 2.6}
CRAQUELADO = {"cor": [0.012, 0.012, 0.013], "grao_mm": 1.2}
BRANCO = rgb(0.85, 0.85, 0.85)
PRETA = rgb(0.012, 0.012, 0.012)
CREME = rgb(0.8, 0.72, 0.52)
PONTEIRO = estilo("pointer", 0.019, (0.015, 0.015, 0.015), traco=(0.9, 0.9, 0.9), escala="tracos")
EXTRA = estilo("liso", 0.011, (0.015, 0.015, 0.015), traco=(0.9, 0.9, 0.9), escala="nenhuma")
SO_TOLEX = {"tipo": "tolex", "cor": [0.006, 0.006, 0.006], "grao_mm": 2.6}
FICHAS = {}

FICHAS["doublestone50"] = dict(
    inspiracao=("Two-Rock Classic Reverb: cabeçote de 524 × 305 × 270 mm em tolex preto com friso branco, painel de alumínio "
                "escovado prata na faixa de CIMA com letras pretas e o nome em script, e a tela preta embaixo com a marca: "
                "entrada, nove botões cromados serrilhados (FET GAIN, TREBLE, MIDDLE, BASS, GAIN, MASTER, REVERB SEND e "
                "RETURN, CONTOUR) com as minichaves FET, BRIGHT, MID, DEEP, EQ, GAIN STRUCTURE e BYPASS em cima, a alavanca "
                "ON/STANDBY e a luz piloto azul."),
    mm=(524, 305, 270), couro=TOLEX_PRETO,
    fachada=tela((0.008, 0.008, 0.008), (0.03, 0.03, 0.03), 1.8, friso=(0.86, 0.85, 0.82)),
    logo={"altura_m": 0.02, "altura_rel": 0.5},
    painel=dict(posicao="cima", altura_m=0.088, cor=rgb(0.66, 0.67, 0.68), texto_cor=PRETA),
    estilos={"padrao": estilo("cromado", 0.019, (0.8, 0.8, 0.82), (0.88, 0.88, 0.9), traco=(0.02, 0.02, 0.02), escala="tracos", metal=True)},
    cantos="preto",
    fileiras=[ROW(0.36, 0.0, 1.0, J("INPUT", x=0.04), F("FET GAIN", x=0.11), K("agudo", "TREBLE", x=0.18),
                  K("medio", "MIDDLE", x=0.25), K("grave", "BASS", x=0.32), K("ganho", "GAIN", x=0.39),
                  K("volume", "MASTER", x=0.46), F("SEND", x=0.53), F("RETURN", x=0.6), K("presenca", "CONTOUR", x=0.67),
                  M("Double Stone", 0.014, x=0.79, z=0.4, fonte="script"), T("ON/STBY", x=0.89),
                  JW((0.15, 0.35, 1.0), x=0.95, raio_m=0.0065), letra_m=0.005, rotulo_pos="baixo"),
              ROW(0.85, 0.08, 0.72, MT("FET"), MT("BRIGHT"), MT("MID"), MT("DEEP"), MT("EQ"), MT("STRUCT."), MT("BYPASS"),
                  letra_m=0.0042, rotulo_pos="direita")],
    notas="A presença do Nexus ficou no CONTOUR (o Two-Rock não tem PRESENCE na frente).",
)

FICHAS["blackcat30"] = dict(
    inspiracao=("Bad Cat Hot Cat: cabeçote de 527 × 267 × 260 mm em tolex preto de couro de crocodilo com friso prata, sem "
                "tela (a frente é do mesmo tolex com a marca no meio), e a faixa de painel creme no alto da frente com letras "
                "pretas: entrada, dez botões pretos estriados (GAIN 1, GAIN 2, BASS, MID, TREBLE, VOLUME 1, VOLUME 2, MASTER, "
                "PRESENCE, REVERB) e as minichaves CH1/CH2 e LO/HI com os LEDs."),
    mm=(527, 267, 260), couro={"cor": [0.008, 0.008, 0.009], "grao_mm": 4.5},
    fachada={"tipo": "tolex", "cor": [0.008, 0.008, 0.009], "grao_mm": 4.5, "friso": [0.72, 0.73, 0.75]},
    logo={"altura_m": 0.02, "altura_rel": 0.5},
    painel=dict(posicao="cima", altura_m=0.075, cor=rgb(0.82, 0.78, 0.66), material="pintado", texto_cor=PRETA),
    estilos={"padrao": estilo("cromado", 0.018, (0.015, 0.015, 0.015), (0.03, 0.03, 0.03), traco=(0.9, 0.9, 0.9), escala="tracos")},
    cantos="preto", parafusos_painel=False,
    fileiras=[ROW(0.45, 0.0, 1.0, J("INPUT", x=0.04), F("GAIN 1", x=0.1), K("ganho", "GAIN 2", x=0.16), K("grave", "BASS", x=0.22),
                  K("medio", "MID", x=0.28), K("agudo", "TREBLE", x=0.34), F("VOLUME 1", x=0.4), F("VOLUME 2", x=0.46),
                  MT("CH1/CH2", x=0.52, z=0.62), LC("", x=0.52, z=0.9), LC("", x=0.545, z=0.9), MT("LO/HI", x=0.57, z=0.62),
                  K("volume", "MASTER", x=0.63), K("presenca", "PRESENCE", x=0.69), F("REVERB", x=0.75),
                  M("BLACK CAT 30", 0.011, x=0.87, z=0.45), letra_m=0.0048, rotulo_pos="baixo")],
)

FICHAS["hi103"] = dict(
    inspiracao=("Hiwatt DR103: cabeçote de 631 × 280 × 280 mm em tolex preto, sem tela: a frente é do mesmo tolex com friso "
                "branco em volta e outro no meio, a marca em cima; painel preto rebaixado embaixo com letras brancas: quatro "
                "entradas NORMAL e BRIGHT à esquerda, sete botões pretos de ponteiro branco (NORMAL VOL, BRIGHT VOL, BASS, "
                "TREBLE, MIDDLE, PRESENCE, MASTER VOL), a luz piloto vermelha e as alavancas STANDBY e MAINS."),
    mm=(631, 280, 280), couro=TOLEX_PRETO,
    fachada={"tipo": "tolex", "cor": [0.006, 0.006, 0.006], "grao_mm": 2.6, "friso": [0.86, 0.85, 0.82], "friso_meio": 0.42},
    logo={"altura_m": 0.02, "altura_rel": 0.72},
    painel=dict(posicao="baixo", altura_m=0.08, cor=rgb(0.012, 0.012, 0.013), material="pintado", texto_cor=BRANCO),
    estilos={"padrao": PONTEIRO}, cantos="preto",
    fileiras=[ROW(0.45, 0.0, 1.0, J(x=0.04, z=0.7), J(x=0.04, z=0.25), J(x=0.09, z=0.7), J(x=0.09, z=0.25),
                  TX("NORMAL", x=0.04, z=0.95), TX("BRIGHT", x=0.09, z=0.95),
                  K("ganho", "NORMAL VOL", x=0.17), F("BRIGHT VOL", x=0.26), K("grave", "BASS", x=0.35),
                  K("agudo", "TREBLE", x=0.44), K("medio", "MIDDLE", x=0.53), K("presenca", "PRESENCE", x=0.62),
                  K("volume", "MASTER VOL", x=0.71), JW((0.9, 0.05, 0.02), x=0.79, raio_m=0.0065),
                  T("STANDBY", x=0.86), T("MAINS", x=0.92), letra_m=0.0052, rotulo_pos="baixo")],
)

FICHAS["darkstar20"] = dict(
    inspiracao=("Blackstar HT-20RH MkIII: cabeçote pequeno de 468 × 245 × 222 mm em tolex preto craquelado, sem grade (a frente "
                "é do mesmo tolex com a marca no meio) e painel preto embaixo com letras brancas: alavanca POWER com LED e "
                "chave 20W/2W à esquerda, dez botões pretos (MASTER VOLUME, REVERB, ISF, TREBLE, MIDDLE, BASS, OD VOLUME, "
                "OD GAIN, CLEAN TONE, CLEAN VOLUME), as teclas VOICE e OD SELECT com LEDs e a entrada à direita."),
    mm=(468, 245, 222), couro=CRAQUELADO,
    fachada={"tipo": "tolex", "cor": [0.012, 0.012, 0.013], "grao_mm": 1.2},
    logo={"altura_m": 0.02, "altura_rel": 0.5},
    painel=dict(posicao="baixo", altura_m=0.075, cor=rgb(0.012, 0.012, 0.013), material="pintado", texto_cor=BRANCO),
    estilos={"padrao": estilo("liso", 0.016, (0.015, 0.015, 0.015), traco=(0.9, 0.9, 0.9), escala="tracos")},
    cantos="preto", parafusos_painel=False,
    fileiras=[ROW(0.45, 0.0, 1.0, T("POWER", x=0.04), L(None, (0.9, 0.05, 0.02), aceso=True, x=0.04, z=0.9), MT("20W/2W", x=0.09),
                  K("volume", "MASTER", x=0.15), F("REVERB", x=0.21), K("presenca", "ISF", x=0.27), K("agudo", "TREBLE", x=0.33),
                  K("medio", "MIDDLE", x=0.39), K("grave", "BASS", x=0.45), F("OD VOLUME", x=0.51), K("ganho", "OD GAIN", x=0.57),
                  P("VOICE", x=0.62, tamanho_m=0.0065), P("OD", x=0.655, tamanho_m=0.0065),
                  LC("", x=0.62, z=0.9), LC("", x=0.655, z=0.9), LC("", x=0.69, z=0.9),
                  F("CLEAN TONE", x=0.72), F("CLEAN VOL", x=0.78), P("VOICE", x=0.83, tamanho_m=0.0065),
                  J("INPUT", x=0.92), letra_m=0.0045, rotulo_pos="baixo")],
    notas="Feito como o HT-20RH MkIII (decisão do PROGRESSO). A presença do Nexus ficou no ISF.",
)

FICHAS["gen120"] = dict(
    inspiracao=("Revv Generator 120: cabeçote de 673 × 267 × 254 mm em tolex grafite craquelado com cantoneiras pretas, sem "
                "grade (a frente é do mesmo tolex com a placa iluminada no meio) e painel preto embaixo com letras brancas: "
                "entrada, os quatro canais em fileiras alternadas (CH1 TREBLE, MID, BASS, LEVEL em cima; CH2 GAIN, TREBLE, "
                "MID, BASS, LEVEL embaixo; CH3 em cima, que é o que toca; CH4 embaixo), MASTER 1 e 2, PRESENCE e DEPTH, "
                "LEVEL e VIRTUAL CAB, teclinhas com LEDs e as alavancas STANDBY e POWER."),
    mm=(673, 267, 254), couro={"cor": [0.03, 0.03, 0.032], "grao_mm": 1.2},
    fachada={"tipo": "tolex", "cor": [0.03, 0.03, 0.032], "grao_mm": 1.2},
    logo={"altura_m": 0.022, "altura_rel": 0.5},
    painel=dict(posicao="baixo", altura_m=0.095, cor=rgb(0.01, 0.01, 0.011), material="pintado", texto_cor=BRANCO),
    estilos={"padrao": estilo("liso", 0.016, (0.015, 0.015, 0.015), traco=(0.9, 0.9, 0.9), escala="tracos")},
    cantos="preto", parafusos_painel=False,
    fileiras=[
        ROW(0.5, 0.0, 1.0, J("INPUT", x=0.03), letra_m=0.004, rotulo_pos="baixo"),
        ROW(0.72, 0.06, 0.8, F("TREBLE"), F("MID"), F("BASS"), F("LEVEL"), GAP(0.6), K("ganho", "GAIN"), K("agudo", "TREBLE"),
            K("medio", "MID"), K("grave", "BASS"), F("LEVEL"), GAP(0.6), K("volume", "MASTER 1"), F("MASTER 2"),
            letra_m=0.0038, rotulo_pos="cima"),
        ROW(0.25, 0.06, 0.8, F("GAIN"), F("TREBLE"), F("MID"), F("BASS"), F("LEVEL"), GAP(0.6), F("GAIN"), F("TREBLE"),
            F("MID"), F("BASS"), F("LEVEL"), GAP(0.6), K("presenca", "PRESENCE"), F("DEPTH"), letra_m=0.0038, rotulo_pos="baixo"),
        ROW(0.5, 0.0, 1.0, F("LEVEL", x=0.83, z=0.72), F("CAB", x=0.83, z=0.25, rotulo_pos="baixo"),
            LC("", x=0.87, z=0.8), LC("", x=0.87, z=0.5), LC("", x=0.87, z=0.2),
            T("STANDBY", x=0.91), T("POWER", x=0.955), letra_m=0.0038, rotulo_pos="baixo"),
    ],
)

FICHAS["nova100"] = dict(
    inspiracao=("Fortin Natas: cabeçote de cerca de 700 × 270 × 260 mm em tolex preto, grade de metal preta de venezianas com "
                "as válvulas e a marca no meio, e painel vermelho anodizado embaixo com letras brancas: entrada, canal 1 "
                "(GIRTH, GRIND, GAIN, minichave KILL, VOLUME, BASS, MIDDLE, TREBLE; o que toca), canal 2 (GAIN, BRIGHT, "
                "VOLUME, BASS, MIDDLE, TREBLE), DEPTH, PRESENCE e MASTER em botões pretos de ponteiro e as gangorras "
                "vermelhas STANDBY e POWER."),
    mm=(700, 270, 260), couro=TOLEX_PRETO,
    fachada={"tipo": "barras", "cor": [0.015, 0.015, 0.016], "barras": 6, "luz": [1.0, 0.35, 0.06]},
    logo={"altura_m": 0.024, "altura_rel": 0.5},
    painel=dict(posicao="baixo", altura_m=0.085, cor=rgb(0.45, 0.02, 0.02), material="escovado", texto_cor=BRANCO),
    estilos={"padrao": dict(PONTEIRO, diametro_m=0.018)}, cantos="preto",
    fileiras=[ROW(0.45, 0.0, 1.0, J("INPUT", x=0.03), F("GIRTH", x=0.075), F("GRIND", x=0.118), K("ganho", "GAIN", x=0.161),
                  MT("KILL", x=0.195), F("VOLUME", x=0.229), K("grave", "BASS", x=0.272), K("medio", "MIDDLE", x=0.315),
                  K("agudo", "TREBLE", x=0.358),
                  F("GAIN", x=0.42), MT("BRIGHT", x=0.454), F("VOLUME", x=0.488), F("BASS", x=0.531), F("MIDDLE", x=0.574),
                  F("TREBLE", x=0.617), F("DEPTH", x=0.68), K("presenca", "PRESENCE", x=0.723), K("volume", "MASTER", x=0.766),
                  MT("CH", x=0.81), M("NOVA 100", 0.01, x=0.86, z=0.5), R("STANDBY", (0.85, 0.02, 0.02), aceso=False, x=0.915),
                  R("POWER", x=0.96), letra_m=0.0045, rotulo_pos="baixo")],
    notas="Medidas estimadas pela foto (a Fortin não publica).",
)

FICHAS["emperor20"] = dict(
    inspiracao=("Tone King Imperial: cabeçote de 572 × 267 × 229 mm em tolex preto, painel preto no alto da frente com filetes "
                "e letras creme, e tela prata e preta numa moldura creme embaixo com a marca: entrada INSTRUMENT, canal LEAD "
                "(VOLUME, TONE, MID-BITE), a chave LEAD/RHYTHM, canal RHYTHM (VOLUME, TREBLE, BASS), REVERB, TREMOLO "
                "(SPEED, DEPTH) e a joia piloto, em botões creme de saia com ponteiro."),
    mm=(572, 267, 229), couro=TOLEX_PRETO,
    fachada=tela((0.45, 0.46, 0.48), (0.05, 0.05, 0.055), 1.8, friso=(0.82, 0.76, 0.6)),
    logo={"lado_rel": 0.22, "altura_m": 0.014, "altura_rel": 0.24},
    painel=dict(posicao="cima", altura_m=0.07, cor=rgb(0.01, 0.01, 0.011), material="pintado", texto_cor=CREME, friso=(0.75, 0.66, 0.42)),
    estilos={"padrao": estilo("saia_numerada", 0.018, (0.82, 0.78, 0.62), (0.82, 0.78, 0.62), saia=(0.82, 0.78, 0.62),
                              numeros=(0.02, 0.02, 0.02), traco=(0.02, 0.02, 0.02), escala="nenhuma")},
    cantos="nenhum",
    fileiras=[ROW(0.5, 0.0, 1.0, J("INSTRUMENT", x=0.04), K("ganho", "VOLUME", x=0.11), K("agudo", "TONE", x=0.18),
                  K("medio", "MID-BITE", x=0.25), MT("LEAD/RHY", x=0.31), LC("", x=0.31, z=0.88), LC("", x=0.33, z=0.88),
                  K("volume", "VOLUME", x=0.37), K("presenca", "TREBLE", x=0.44), K("grave", "BASS", x=0.51),
                  F("REVERB", x=0.6), F("SPEED", x=0.69), F("DEPTH", x=0.76), M("EMPEROR 20", 0.011, x=0.86, z=0.5, cor=CREME),
                  JW((0.95, 0.45, 0.05), x=0.95, raio_m=0.0065), letra_m=0.0048, rotulo_pos="baixo")],
    notas=("O Imperial não tem presença: a presença do Nexus é o TREBLE do canal Rhythm; o grave é o BASS do Rhythm; o ganho, "
           "o agudo e o médio são VOLUME, TONE e MID-BITE do Lead."),
)

FICHAS["rg100"] = dict(
    inspiracao=("Randall RG100ES: cabeçote de cerca de 737 × 279 × 254 mm em tolex preto com cantoneiras pretas, sem grade (a "
                "frente é do mesmo tolex com friso branco e a marca grande) e a faixa estreita de painel preto embaixo com "
                "letras brancas: chave liga e lâmpada vermelha à esquerda, nove botões pretos de tampa clara (REVERB, "
                "PRESENCE, BASS, MIDDLE, TREBLE, MASTER e GAIN do lead, MASTER e GAIN do limpo) e as entradas HI e LO."),
    mm=(737, 279, 254), couro=TOLEX_PRETO,
    fachada={"tipo": "tolex", "cor": [0.006, 0.006, 0.006], "grao_mm": 2.6, "friso": [0.86, 0.85, 0.82]},
    logo={"altura_m": 0.03, "altura_rel": 0.5},
    painel=dict(posicao="baixo", altura_m=0.065, cor=rgb(0.012, 0.012, 0.013), material="pintado", texto_cor=BRANCO),
    estilos={"padrao": estilo("marshall", 0.019, (0.015, 0.015, 0.015), (0.75, 0.75, 0.77), escala="tracos")},
    cantos="preto",
    fileiras=[ROW(0.45, 0.0, 1.0, RB("POWER", x=0.04), JW((0.9, 0.05, 0.02), x=0.085, raio_m=0.006),
                  F("REVERB", x=0.15), K("presenca", "PRESENCE", x=0.215), K("grave", "BASS", x=0.28), K("medio", "MIDDLE", x=0.345),
                  K("agudo", "TREBLE", x=0.41), K("volume", "MASTER", x=0.475), K("ganho", "GAIN", x=0.54),
                  F("MASTER", x=0.605), F("GAIN", x=0.67), LC("", x=0.72, z=0.75), LC("", x=0.72, z=0.3),
                  M("RG 100", 0.012, x=0.79, z=0.5), J("HI", x=0.88), J("LO", x=0.94), letra_m=0.0045, rotulo_pos="baixo")],
    notas="Medidas de loja (Randall não publica).",
)

FICHAS["arch100"] = dict(
    inspiracao=("PRS Archon: cabeçote de cerca de 546 × 267 × 254 mm em tolex preto, grade de metal preta perfurada em cima "
                "com o nome ARCH 100 prateado, faixa de madeira escura no meio com a marca e painel preto embaixo com letras "
                "e filetes creme: chave ST-BY/ON/OFF com LED verde, CLEAN MASTER, LEAD MASTER, DEPTH, PRESENCE, o canal "
                "CLEAN (BASS, MIDDLE, TREBLE, VOLUME) e o canal LEAD (BASS, MIDDLE, TREBLE, VOLUME; o que toca) em botões "
                "pretos de ponteiro, minichaves BRIGHT e LEAD/CLEAN e a entrada à direita."),
    mm=(546, 267, 254), couro=TOLEX_PRETO,
    fachada={"tipo": "grade", "cor": [0.02, 0.02, 0.022], "furo_mm": 3.6, "passo_mm": 5.2,
             "faixa": {"altura_rel": 0.3, "posicao": "baixo", "cor": [0.045, 0.02, 0.012], "lisa": True}},
    selo_grade={"texto": "ARCH 100", "altura_m": 0.022, "x_rel": 0.0, "z_rel": 0.62, "cor": [0.75, 0.76, 0.78]},
    logo={"altura_m": 0.013, "altura_rel": 0.5},
    painel=dict(posicao="baixo", altura_m=0.075, cor=rgb(0.012, 0.012, 0.013), material="pintado", texto_cor=CREME),
    estilos={"padrao": dict(PONTEIRO, diametro_m=0.017)}, cantos="preto", parafusos_painel=False,
    fileiras=[ROW(0.45, 0.0, 1.0, T("ST-BY/ON", x=0.04), L(None, (0.1, 0.8, 0.1), aceso=True, x=0.04, z=0.9),
                  F("CLEAN MASTER", x=0.1), K("volume", "LEAD MASTER", x=0.16), F("DEPTH", x=0.22), K("presenca", "PRESENCE", x=0.28),
                  F("BASS", x=0.35), F("MIDDLE", x=0.405), F("TREBLE", x=0.46), F("VOLUME", x=0.515), MT("BRIGHT", x=0.555),
                  K("grave", "BASS", x=0.6), K("medio", "MIDDLE", x=0.655), K("agudo", "TREBLE", x=0.71),
                  K("ganho", "VOLUME", x=0.765), MT("BRIGHT", x=0.805), MT("LEAD/CLEAN", x=0.85),
                  LC("", x=0.85, z=0.9), LC("", x=0.875, z=0.9), J("INPUT", x=0.94), letra_m=0.0042, rotulo_pos="baixo")],
    riscos=[RISCO(0.33, 0.06, 0.33, 0.94, cor=CREME), RISCO(0.575, 0.06, 0.575, 0.94, cor=CREME)],
    notas="Medidas de loja (o site da PRS bloqueou). O ganho do Nexus é o VOLUME do canal Lead (no Archon é o ganho do canal).",
)

FICHAS["kraken50"] = dict(
    inspiracao=("Victory Kraken (cabeçote compacto): 350 × 220 × 220 mm em tolex grafite com cantoneiras pretas, tela preta com "
                "a marca e o painel de metal azul-ardósia embaixo com letras pretas: entrada, minichave CLEAN/GAIN I, oito "
                "botões pretos de ponteiro (CLEAN/GAIN I, GAIN II, BASS, MIDDLE, TREBLE, MASTER I, MASTER II, PRESENCE), a "
                "lâmpada piloto e a chave HIGH/PREHEAT/LOW à direita."),
    mm=(350, 220, 220), couro={"cor": [0.03, 0.03, 0.032], "grao_mm": 2.0},
    fachada=tela((0.008, 0.008, 0.008), (0.03, 0.03, 0.03), 1.8),
    logo={"altura_m": 0.014, "altura_rel": 0.5},
    painel=dict(posicao="baixo", altura_m=0.07, cor=rgb(0.2, 0.25, 0.3), material="pintado", texto_cor=PRETA),
    estilos={"padrao": dict(PONTEIRO, diametro_m=0.016)}, cantos="preto",
    fileiras=[ROW(0.42, 0.0, 1.0, J("INPUT", x=0.05), MT("CLEAN/GAIN", x=0.11), LC("", x=0.11, z=0.9), LC("", x=0.14, z=0.9),
                  F("GAIN I", x=0.18), K("ganho", "GAIN II", x=0.26), K("grave", "BASS", x=0.34), K("medio", "MIDDLE", x=0.42),
                  K("agudo", "TREBLE", x=0.5), F("MASTER I", x=0.58), K("volume", "MASTER II", x=0.66),
                  K("presenca", "PRESENCE", x=0.74), JW((0.9, 0.05, 0.02), x=0.82, raio_m=0.0055), T("HIGH/LOW", x=0.9),
                  letra_m=0.0042, rotulo_pos="baixo")],
)

FICHAS["magick25"] = dict(
    inspiracao=("Supro Black Magick: cabeçote de 520 × 254 × 185 mm em tolex Black Rhino mesclado com friso branco, tela prata "
                "e preta na metade de cima com a marca, faixa do mesmo tolex embaixo dela e o painel de latão escovado com "
                "filetes e letras pretas: entradas IN 1+2 e IN 2, VOL 1, VOL 2, TONE, SPEED, o jaque do tremolo, DEPTH em "
                "botões cromados, a alavanca STANDBY, a lâmpada vermelha e a alavanca POWER."),
    mm=(520, 254, 185), couro={"cor": [0.05, 0.05, 0.052], "grao_mm": 1.4},
    fachada={"tipo": "tela", "cor_a": [0.5, 0.51, 0.52], "cor_b": [0.04, 0.04, 0.045], "trama_mm": 1.6,
             "friso": [0.86, 0.85, 0.82], "faixa": {"altura_rel": 0.3, "posicao": "baixo", "logo": False}},
    logo={"altura_m": 0.018, "altura_rel": 0.55},
    painel=dict(posicao="baixo", altura_m=0.07, cor=rgb(0.55, 0.42, 0.18), texto_cor=PRETA),
    estilos={"padrao": estilo("cromado", 0.02, (0.8, 0.8, 0.82), (0.88, 0.88, 0.9), traco=(0.02, 0.02, 0.02), escala="tracos", metal=True),
             "extra": EXTRA},
    cantos="nenhum",
    fileiras=[ROW(0.45, 0.0, 1.0, J("IN 1+2", x=0.05), J("IN 2", x=0.11), K("ganho", "VOL 1", x=0.19), K("volume", "VOL 2", x=0.27),
                  K("agudo", "TONE", x=0.35), F("SPEED", x=0.43), J("TREM", x=0.5), F("DEPTH", x=0.57),
                  K("grave", "GRAVE", estilo="extra", x=0.64, z=0.65), K("medio", "MÉDIO", estilo="extra", x=0.69, z=0.65),
                  K("presenca", "PRESENÇA", estilo="extra", x=0.74, z=0.65),
                  T("STANDBY", x=0.82), JW((0.9, 0.05, 0.02), x=0.88, raio_m=0.006), T("POWER", x=0.94),
                  letra_m=0.0048, rotulo_pos="baixo")],
    riscos=[RISCO(0.03, 0.92, 0.97, 0.92, 0.0008), RISCO(0.03, 0.08, 0.97, 0.08, 0.0008)],
    notas="O Black Magick só tem VOL 1, VOL 2 e TONE: grave, médio e presença do Nexus ficaram em botões pequenos entre o DEPTH e as chaves. A tela em flecha do Supro ficou retangular.",
)

FICHAS["deliver120"] = dict(
    inspiracao=("Fryette Deliverance: cabeçote de 743 × 267 × 241 mm em tolex preto, sem grade (a frente é do mesmo tolex com a "
                "placa prata da marca no meio) e painel de metal preto escovado embaixo com letras brancas: entrada, nove "
                "botões pretos de ponteiro (GAIN I, GAIN II, MASTER, SOLO, TREBLE, MIDDLE, BASS, PRESENCE, DEPTH), as "
                "minichaves BRIGHT/SOLO e MORE/SOLO com LEDs vermelhos e as gangorras OPERATE (preta) e MAINS (verde acesa)."),
    mm=(743, 267, 241), couro=TOLEX_PRETO, fachada=SO_TOLEX,
    logo={"altura_m": 0.024, "altura_rel": 0.5},
    painel=dict(posicao="baixo", altura_m=0.08, cor=rgb(0.03, 0.03, 0.032), texto_cor=BRANCO),
    estilos={"padrao": PONTEIRO}, cantos="preto",
    fileiras=[ROW(0.45, 0.0, 1.0, J("INPUT", x=0.04), F("GAIN I", x=0.11), K("ganho", "GAIN II", x=0.18), MT("BRIGHT", x=0.225),
                  K("volume", "MASTER", x=0.27), F("SOLO", x=0.34), MT("MORE", x=0.385), L(None, (0.9, 0.05, 0.02), x=0.385, z=0.9),
                  K("agudo", "TREBLE", x=0.43), K("medio", "MIDDLE", x=0.5), K("grave", "BASS", x=0.57),
                  K("presenca", "PRESENCE", x=0.64), F("DEPTH", x=0.71), M("DELIVER 120", 0.012, x=0.8, z=0.5),
                  RB("OPERATE", x=0.88), R("MAINS", (0.1, 0.85, 0.15), x=0.94), letra_m=0.005, rotulo_pos="baixo")],
)
