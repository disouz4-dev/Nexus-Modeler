# Fichas do lote 8: cabeçotes de baixo (Ampeg, Orange, GK, Hartke, Eden, Darkglass, Sunn, Bassman, Aguilar, Markbass).
from cabecotes_fiel import *  # noqa: F401,F403

TOLEX_PRETO = {"cor": [0.006, 0.006, 0.006], "grao_mm": 2.6}
BRANCO = rgb(0.85, 0.85, 0.85)
PRETA = rgb(0.012, 0.012, 0.012)
EXTRA = estilo("liso", 0.011, (0.015, 0.015, 0.015), traco=(0.9, 0.9, 0.9), escala="nenhuma")
LISO = estilo("liso", 0.017, (0.015, 0.015, 0.015), traco=(0.9, 0.9, 0.9), escala="tracos")
AMPEG_SAIA = estilo("saia_numerada", 0.019, (0.012, 0.012, 0.012), (0.02, 0.02, 0.02), saia=(0.75, 0.76, 0.78),
                    numeros=(0.02, 0.02, 0.02), traco=(0.9, 0.9, 0.9), escala="nenhuma")
AZUL_AMPEG = rgb(0.04, 0.12, 0.45)
FICHAS = {}

FICHAS["vault300"] = dict(
    inspiracao=("Ampeg SVT dos anos 70: cabeçote de 610 × 290 × 330 mm em tolex preto com cantos de metal, painel de alumínio "
                "prateado na faixa de cima com letras AZUIS e a tela preta embaixo com friso claro e a marca: entradas normal e "
                "-15 dB, GAIN, chaves ULTRA HI e ULTRA LO empilhadas, BASS, MIDRANGE, FREQUENCY de cinco posições, TREBLE e "
                "MASTER em botões pretos de saia prateada, os LEDs e as gangorras STANDBY e POWER."),
    mm=(610, 290, 330), couro=TOLEX_PRETO,
    fachada=tela((0.01, 0.01, 0.012), (0.04, 0.04, 0.05), 2.4, friso=(0.75, 0.75, 0.76)),
    logo={"altura_m": 0.022, "altura_rel": 0.5},
    painel=dict(posicao="cima", altura_m=0.08, cor=rgb(0.7, 0.71, 0.72), texto_cor=AZUL_AMPEG),
    estilos={"padrao": AMPEG_SAIA, "extra": EXTRA}, cantos="cromo",
    fileiras=[ROW(0.45, 0.0, 1.0, J("NORMAL", x=0.04), J("-15 dB", x=0.09), K("ganho", "GAIN", x=0.16),
                  T("ULTRA HI", x=0.215, z=0.72, rotulo_pos="cima"), T("ULTRA LO", x=0.215, z=0.22),
                  K("grave", "BASS", x=0.28), K("medio", "MIDRANGE", x=0.35), F("FREQUENCY", x=0.42), K("agudo", "TREBLE", x=0.49),
                  K("volume", "MASTER", x=0.56), K("presenca", "PRESENÇA", estilo="extra", x=0.62, z=0.7),
                  M("VAULT 300", 0.013, x=0.72, z=0.55, cor=AZUL_AMPEG), L(None, (0.9, 0.05, 0.02), aceso=True, x=0.79, z=0.55),
                  RB("STANDBY", x=0.86), RB("POWER", x=0.93), letra_m=0.005, rotulo_pos="baixo")],
    notas="Feito como o SVT antigo (decisão do PROGRESSO: painel prata e letras azuis), com os botões do SVT atual. O SVT não tem presença: ficou num botão pequeno depois do MASTER.",
)

FICHAS["vaultv4"] = dict(
    inspiracao=("Ampeg V-4B: cabeçote de 610 × 280 × 280 mm em tolex preto com cantos de metal, painel de alumínio escovado "
                "inclinado na faixa de cima com letras pretas e a tela prata-azulada de trama larga embaixo com a marca: "
                "entradas 0 dB e -15 dB, as três chaves basculantes brancas (ULTRA LO, ULTRA HI e o seletor de médios), GAIN, "
                "BASS, MIDRANGE, TREBLE e MASTER em botões pretos de saia prateada, a luz piloto e as alavancas STANDBY e POWER."),
    mm=(610, 280, 280), couro=TOLEX_PRETO,
    fachada=tela((0.45, 0.5, 0.58), (0.12, 0.14, 0.18), 3.0),
    logo={"altura_m": 0.022, "altura_rel": 0.5},
    painel=dict(posicao="cima", altura_m=0.085, cor=rgb(0.66, 0.67, 0.68), texto_cor=PRETA),
    estilos={"padrao": AMPEG_SAIA, "extra": EXTRA}, cantos="cromo",
    fileiras=[ROW(0.4, 0.0, 1.0, J("0 dB", x=0.04), J("-15 dB", x=0.09), K("ganho", "GAIN", x=0.18), K("grave", "BASS", x=0.27),
                  K("medio", "MIDRANGE", x=0.36), K("agudo", "TREBLE", x=0.45), K("volume", "MASTER", x=0.54),
                  K("presenca", "PRESENÇA", estilo="extra", x=0.61, z=0.65), M("VAULT V4", 0.013, x=0.72, z=0.45),
                  T("STANDBY", x=0.85), T("POWER", x=0.92), letra_m=0.005, rotulo_pos="baixo"),
              ROW(0.86, 0.0, 1.0, R("ULTRA LO", (0.85, 0.85, 0.82), aceso=False, x=0.25, largura_m=0.012, altura_m=0.008, rotulo_pos="direita"),
                  R("ULTRA HI", (0.85, 0.85, 0.82), aceso=False, x=0.36, largura_m=0.012, altura_m=0.008, rotulo_pos="direita"),
                  R("MID", (0.85, 0.85, 0.82), aceso=False, x=0.47, largura_m=0.012, altura_m=0.008, rotulo_pos="direita"),
                  JW((0.9, 0.05, 0.02), x=0.95, raio_m=0.006), letra_m=0.0045)],
    notas="O V-4B não tem presença: ficou num botão pequeno depois do MASTER.",
)

ORANGE_BOTAO = estilo("cromado", 0.026, (0.015, 0.015, 0.015), (0.03, 0.03, 0.03), traco=(0.9, 0.9, 0.9), escala="tracos")
FICHAS["citrusbass200"] = dict(
    inspiracao=("Orange AD200 Bass MK3: cabeçote de 550 × 270 × 280 mm em tolex laranja com cantos pretos; a frente inteira é a "
                "placa branca com os puxadores cromados, a marca e o nome em cima e a faixa laranja com os nomes: alavancas "
                "POWER e STANDBY, a joia âmbar, MASTER e GAIN grandes, TREBLE, MIDDLE e BASS pequenos em botões pretos de saia "
                "serrilhada, e as entradas PASSIVE e ACTIVE empilhadas."),
    mm=(550, 270, 280), couro={"cor": [0.8, 0.22, 0.01], "grao_mm": 1.8}, fachada=None,
    painel=dict(posicao="inteiro", cor=rgb(0.84, 0.84, 0.82), material="pintado", texto_cor=PRETA),
    estilos={"padrao": ORANGE_BOTAO, "peq": dict(ORANGE_BOTAO, diametro_m=0.018), "extra": EXTRA}, cantos="preto",
    fileiras=[ROW(0.68, 0.0, 1.0, PUX(x=0.025, altura_m=0.12), PUX(x=0.975, altura_m=0.12), LOGO(0.017, x=0.4, z=0.72),
                  M("CITRUS BASS 200", 0.011, x=0.8, z=0.72)),
              ROW(0.4, 0.0, 1.0, TX("MASTER", x=0.3), TX("TREBLE", x=0.42), TX("MIDDLE", x=0.52), TX("BASS", x=0.62),
                  TX("GAIN", x=0.74), letra_m=0.0062),
              ROW(0.18, 0.0, 1.0, T(None, x=0.08), T(None, x=0.13), JW((0.95, 0.5, 0.05), x=0.19, raio_m=0.006),
                  K("volume", "", x=0.3), K("agudo", "", estilo="peq", x=0.42), K("medio", "", estilo="peq", x=0.52),
                  K("grave", "", estilo="peq", x=0.62), K("ganho", "", x=0.74), K("presenca", "PRESENÇA", estilo="extra", x=0.83, z=0.3),
                  J("PASSIVE", x=0.91, z=0.3, rotulo_pos="cima"), J("ACTIVE", x=0.91, z=0.09))],
    riscos=[RISCO(0.05, 0.4, 0.95, 0.4, 0.024, cor=(0.85, 0.27, 0.01))],
    notas="O AD200 não tem presença: ficou num botão pequeno entre o GAIN e as entradas.",
)

FICHAS["granite800"] = dict(
    inspiracao=("Gallien-Krueger 800RB: chassi de metal preto de 432 × 152 × 229 mm com a tampa de fendas, faixa de comandos "
                "prateada em cima com letras pretas e faixa preta embaixo com o nome em branco: entrada, VOLUME, as teclas de "
                "voz, TREBLE, HI MID, LO MID, BASS, BOOST, o CROSSOVER e os MASTERs de 300 W e 100 W em botões pretos, e a "
                "chave de liga embaixo à direita."),
    formato="caixinha", mm=(432, 152, 229), couro={"cor": [0.02, 0.02, 0.022], "metal": True, "aspereza": 0.5},
    fachada=None, alca="nenhuma",
    painel=dict(posicao="inteiro", cor=rgb(0.62, 0.63, 0.64), texto_cor=PRETA),
    estilos={"padrao": LISO}, parafusos_painel=False,
    fileiras=[ROW(0.62, 0.0, 1.0, J("INPUT", x=0.04), K("ganho", "VOLUME", x=0.11), P(None, x=0.16, z=0.75, tamanho_m=0.006),
                  P(None, x=0.16, z=0.55, tamanho_m=0.006), P(None, x=0.16, z=0.35, tamanho_m=0.006),
                  K("agudo", "TREBLE", x=0.22), K("medio", "HI MID", x=0.29), F("LO MID", x=0.36), K("grave", "BASS", x=0.43),
                  K("presenca", "BOOST", x=0.52), F("CROSSOVER", x=0.62), F("300W", x=0.72), K("volume", "100W", x=0.82),
                  letra_m=0.0042, rotulo_pos="cima"),
              ROW(0.12, 0.0, 1.0, LOGO(0.008, x=0.2, z=0.13), M("GRANITE 800", 0.009, x=0.5, z=0.13, cor=(0.85, 0.85, 0.85)),
                  RB(None, x=0.93, z=0.13, largura_m=0.012, altura_m=0.016))],
    riscos=[dict(RISCO(0.0, 0.13, 1.0, 0.13, 0.04, cor=(0.012, 0.012, 0.012)))],
    notas="A presença do Nexus ficou no BOOST do 800RB; o volume no MASTER de 100 W.",
)

FICHAS["hart1000"] = dict(
    inspiracao=("Hartke LH1000: rack 2U (483 × 83 × 369 mm) de aço cinza-chumbo com faixas mais claras nas seções, alças pretas "
                "e orelhas: entradas PASSIVE e ACTIVE, VOLUME, a tecla BRITE com LED amarelo, o EQUALIZER (BASS, MID, TREBLE), "
                "LIMITER com LED azul, BALANCE, a saída XLR, a janela de ventilação com tela preta e a gangorra POWER vermelha "
                "acesa, em botões cromados de saia numerada."),
    formato="rack", rack_u=2, mm=(483, 89, 369), couro={"cor": [0.03, 0.03, 0.032], "metal": True}, fachada=None,
    alca="rack",
    painel=dict(posicao="inteiro", cor=rgb(0.1, 0.1, 0.105), material="pintado", texto_cor=BRANCO),
    estilos={"padrao": estilo("saia_numerada", 0.016, (0.8, 0.8, 0.82), (0.86, 0.86, 0.88), saia=(0.75, 0.76, 0.78),
                              numeros=(0.02, 0.02, 0.02), traco=(0.02, 0.02, 0.02), escala="nenhuma", metal=True, tampa_metal=True),
             "extra": EXTRA},
    fileiras=[ROW(0.45, 0.0, 1.0, J("PASSIVE", x=0.06), J("ACTIVE", x=0.11), K("ganho", "VOLUME", x=0.18),
                  P("BRITE", x=0.23, tamanho_m=0.0065), L(None, (0.95, 0.75, 0.05), x=0.23, z=0.82),
                  K("grave", "BASS", x=0.29), K("medio", "MID", x=0.35), K("agudo", "TREBLE", x=0.41),
                  P("LIMITER", x=0.46, tamanho_m=0.0065), L(None, (0.1, 0.3, 1.0), aceso=True, x=0.46, z=0.82),
                  K("volume", "BALANCE", x=0.52), K("presenca", "PRESENÇA", estilo="extra", x=0.57, z=0.7),
                  J("DIRECT OUT", x=0.63), LOGO(0.006, x=0.74, z=0.8), R("POWER", x=0.92), letra_m=0.0042, rotulo_pos="baixo")],
    riscos=[dict(RISCO(0.03, 0.45, 0.14, 0.45, 0.06, cor=(0.2, 0.2, 0.21))), dict(RISCO(0.26, 0.45, 0.44, 0.45, 0.06, cor=(0.2, 0.2, 0.21))),
            dict(RISCO(0.68, 0.36, 0.86, 0.36, 0.035, cor=(0.005, 0.005, 0.005)))],
    notas="O LH1000 não tem master nem presença: o volume do Nexus ficou no BALANCE e a presença num botão pequeno.",
)

def _cap(c):
    return estilo("marshall", 0.016, (0.012, 0.012, 0.012), c, escala="tracos", tampa_plastico=True)


FICHAS["garden800"] = dict(
    inspiracao=("Eden World Tour WT-800: rack 2U (483 × 89 mm) dourado anodizado com friso preto e vermelho em volta do EQ e a "
                "marca à esquerda: entrada, LED de nível, tecla INPUT PAD, GAIN (azul), ENHANCE (cinza), BASS, o EQ "
                "semiparamétrico de três bandas com as frequências em cima, TREBLE (vermelhos), MASTER (azul), BALANCE e "
                "CROSSOVER, teclas do compressor e do bi-amp, saída de fone, LEDs e a gangorra MAINS vermelha acesa, em "
                "botões de tampa colorida e saia preta serrilhada."),
    formato="rack", rack_u=2, mm=(483, 89, 330), couro={"cor": [0.02, 0.02, 0.022], "metal": True}, fachada=None,
    painel=dict(posicao="inteiro", cor=rgb(0.5, 0.36, 0.12), texto_cor=PRETA),
    estilos={"padrao": _cap((0.75, 0.05, 0.03)), "azul": _cap((0.08, 0.2, 0.7)), "cinza": _cap((0.45, 0.45, 0.47))},
    fileiras=[ROW(0.72, 0.36, 0.6, F("LOW FREQ"), F("MID FREQ"), F("HIGH FREQ"), letra_m=0.0034, rotulo_pos="cima"),
              ROW(0.28, 0.0, 1.0, LOGO(0.006, x=0.07, z=0.75), J("INPUT", x=0.05), P("PAD", x=0.1, tamanho_m=0.006),
                  K("ganho", "GAIN", estilo="azul", x=0.16), K("presenca", "ENHANCE", estilo="cinza", x=0.22),
                  K("grave", "BASS", x=0.28), F("LOW", x=0.36), K("medio", "MID", x=0.48), F("HIGH", x=0.6),
                  K("agudo", "TREBLE", x=0.66), K("volume", "MASTER", estilo="azul", x=0.73), F("BALANCE", estilo="cinza", x=0.79),
                  F("XOVER", x=0.85), M("GARDEN 800", 0.007, x=0.85, z=0.85), R("MAINS", x=0.93, z=0.5),
                  letra_m=0.0034, rotulo_pos="baixo")],
    riscos=[RISCO(0.25, 0.05, 0.69, 0.05, 0.0012, cor=(0.6, 0.05, 0.03)), RISCO(0.25, 0.97, 0.69, 0.97, 0.0012, cor=(0.6, 0.05, 0.03))],
    notas="Profundidade do WT-800 não encontrada (usei 330 mm).",
)

FICHAS["glass500"] = dict(
    inspiracao=("Darkglass Microtubes 500: caixinha de metal preta fosca de 230 × 71 × 180 mm com letras brancas finas: entrada e "
                "as três teclas com LED à esquerda, a seção MICROTUBES com COMP, DRIVE e LEVEL em cima e TONE e BLEND embaixo, "
                "o EQ de seis deslizantes com cursor azul, MUTE e o MASTER à direita, em botões pretos com marca branca."),
    formato="caixinha", mm=(230, 71, 180), couro={"cor": [0.02, 0.02, 0.022], "metal": True, "aspereza": 0.7},
    fachada=None, alca="nenhuma",
    painel=dict(posicao="inteiro", cor=rgb(0.015, 0.015, 0.016), material="pintado", texto_cor=BRANCO),
    estilos={"padrao": dict(LISO, diametro_m=0.013), "extra": dict(EXTRA, diametro_m=0.008)}, parafusos_painel=False,
    fileiras=[ROW(0.7, 0.0, 1.0, J("INPUT", x=0.05), F("COMP", x=0.15), K("ganho", "DRIVE", x=0.24), F("LEVEL", x=0.33),
                  K("volume", "MASTER", x=0.92), letra_m=0.003, rotulo_pos="cima"),
              ROW(0.3, 0.0, 1.0, P(None, x=0.04, tamanho_m=0.005), P(None, x=0.08, tamanho_m=0.005), P(None, x=0.12, tamanho_m=0.005),
                  K("agudo", "TONE", x=0.24), K("presenca", "BLEND", x=0.33), K("grave", "GRAVE", estilo="extra", x=0.405, z=0.55),
                  K("medio", "MÉDIO", estilo="extra", x=0.405, z=0.2), P("MUTE", x=0.92, tamanho_m=0.005), letra_m=0.003, rotulo_pos="baixo"),
              ROW(0.5, 0.47, 0.84, SF("LO"), SF("250"), SF("500"), SF("1.5K"), SF("3K"), SF("HI"), curso_m=0.03, letra_m=0.0028,
                  rotulo_pos="baixo")],
    notas=("O EQ de seis deslizantes ficou desenhado fixo; grave e médio do Nexus são dois botões pequenos ao lado do BLEND, "
           "o agudo é o TONE e a presença o BLEND."),
)

FICHAS["solart"] = dict(
    inspiracao=("Sunn Model T: cabeçote de 760 × 260 × 280 mm em tolex preto com cantoneiras cromadas, grade de metal preta de "
                "aletas verticais na frente com a marca no canto de cima à esquerda, e a faixa preta embaixo com a tira de "
                "alumínio escovado: as cinco tomadas à esquerda, sete botões pretos pequenos de tampa prateada e saia numerada "
                "(BRITE VOLUME, NORMAL VOLUME, BASS, MID RANGE, TREBLE, PRESENCE, MASTER VOLUME), o nome e as gangorras "
                "STANDBY e POWER."),
    mm=(760, 260, 280), couro=TOLEX_PRETO,
    fachada={"tipo": "aletas", "cor": [0.02, 0.02, 0.022], "passo_mm": 9.0},
    logo={"lado_rel": -0.32, "altura_m": 0.016, "altura_rel": 0.82},
    painel=dict(posicao="baixo", altura_m=0.075, cor=rgb(0.012, 0.012, 0.013), material="pintado", texto_cor=PRETA),
    estilos={"padrao": estilo("saia_numerada", 0.017, (0.012, 0.012, 0.012), (0.75, 0.76, 0.78), saia=(0.012, 0.012, 0.012),
                              numeros=(0.85, 0.85, 0.85), traco=(0.9, 0.9, 0.9), escala="nenhuma", tampa_metal=True)},
    cantos="cromo",
    fileiras=[ROW(0.5, 0.0, 1.0, J(x=0.04, z=0.72), J(x=0.04, z=0.28), J(x=0.08, z=0.5), J(x=0.12, z=0.72), J(x=0.12, z=0.28),
                  K("ganho", "BRITE", x=0.22), F("NORMAL", x=0.29), K("grave", "BASS", x=0.36), K("medio", "MID", x=0.43),
                  K("agudo", "TREBLE", x=0.5), K("presenca", "PRESENCE", x=0.57), K("volume", "MASTER", x=0.64),
                  M("model T", 0.014, x=0.75, z=0.5), RB("STANDBY", x=0.87), RB("POWER", x=0.93), letra_m=0.0045, rotulo_pos="baixo")],
    riscos=[dict(RISCO(0.02, 0.5, 0.8, 0.5, 0.062, cor=(0.62, 0.63, 0.64)))],
)

FICHAS["bassline100"] = dict(
    inspiracao=("Fender Bassman silverface: cabeçote de 610 × 241 × 241 mm em tolex preto, painel de alumínio prateado na faixa "
                "de cima com letras pretas, o nome em azul-turquesa e friso fino, e a tela prateada e turquesa embaixo com a marca: "
                "canal BASS INSTRUMENT (entradas, DEEP, VOLUME, TREBLE, BASS; o que toca) e canal NORMAL (entradas, BRIGHT, "
                "VOLUME, TREBLE, BASS) em botões pretos de tampa prateada e saia numerada, a joia piloto e as chaves."),
    mm=(610, 241, 241), couro=TOLEX_PRETO,
    fachada=tela((0.42, 0.5, 0.52), (0.06, 0.07, 0.08), 2.0, friso=(0.75, 0.75, 0.76)),
    logo={"lado_rel": -0.3, "altura_m": 0.016, "altura_rel": 0.7},
    painel=dict(posicao="cima", altura_m=0.07, cor=rgb(0.68, 0.69, 0.7), texto_cor=PRETA, friso=(0.3, 0.3, 0.32)),
    estilos={"padrao": estilo("saia_numerada", 0.019, (0.012, 0.012, 0.012), (0.75, 0.76, 0.78), saia=(0.012, 0.012, 0.012),
                              numeros=(0.85, 0.85, 0.85), traco=(0.9, 0.9, 0.9), escala="nenhuma", tampa_metal=True), "extra": EXTRA},
    cantos="nenhum",
    fileiras=[ROW(0.55, 0.0, 1.0, J("1", x=0.035), J("2", x=0.075), MT("DEEP", x=0.11), K("ganho", "VOLUME", x=0.16),
                  K("agudo", "TREBLE", x=0.22), K("grave", "BASS", x=0.28), TX("BASS INSTRUMENT", x=0.17, z=0.15, letra_m=0.0052),
                  J("1", x=0.36), J("2", x=0.4), MT("BRIGHT", x=0.435), K("volume", "VOLUME", x=0.485), F("TREBLE", x=0.545),
                  F("BASS", x=0.605), TX("NORMAL", x=0.49, z=0.15, letra_m=0.0052),
                  K("medio", "MÉDIO", estilo="extra", x=0.665, z=0.7), K("presenca", "PRESENÇA", estilo="extra", x=0.665, z=0.3),
                  M("BASSLINE", 0.014, x=0.79, z=0.55, cor=(0.1, 0.5, 0.55)), JW((0.9, 0.05, 0.02), x=0.89, raio_m=0.0065),
                  T(None, x=0.95), letra_m=0.0048, rotulo_pos="baixo")],
    notas="O Bassman não tem médio, presença nem master: o volume do Nexus é o VOLUME do canal Normal; médio e presença em botões pequenos.",
)

FICHAS["eagle751"] = dict(
    inspiracao=("Aguilar DB 751: rack 3U (483 × 133 × 356 mm) de painel prata fosco com moldura preta fina, orelhas claras e a "
                "marca grande no centro: entrada, teclinhas -6dB, BRIGHT e DEEP, GAIN, TREBLE, MID, BASS e MASTER, o LED CLIP, "
                "SEND e RETURN com os botões, POST/PRE EQ, a saída XLR, LIFT/GND, FOOT SWITCH, OPERATE/MUTE e a gangorra POWER "
                "com os LEDs azuis, em botões pretos pequenos com plaquinha do nome."),
    formato="rack", rack_u=3, mm=(483, 133, 356), couro={"cor": [0.02, 0.02, 0.022], "metal": True}, fachada=None,
    painel=dict(posicao="inteiro", cor=rgb(0.6, 0.61, 0.62), texto_cor=PRETA),
    estilos={"padrao": LISO, "extra": EXTRA},
    fileiras=[ROW(0.7, 0.0, 1.0, LOGO(0.016, x=0.5, z=0.78), R("POWER", (0.012, 0.012, 0.012), aceso=False, x=0.93, z=0.78),
                  L("OPERATE", (0.1, 0.3, 1.0), aceso=True, x=0.86, z=0.85), L("STATUS", (0.1, 0.3, 1.0), aceso=True, x=0.86, z=0.68),
                  letra_m=0.0042, rotulo_pos="direita"),
              ROW(0.35, 0.0, 1.0, J("INPUT", x=0.05), P("-6dB", x=0.095, tamanho_m=0.006), K("ganho", "GAIN", x=0.14),
                  P("BRIGHT", x=0.185, tamanho_m=0.006), K("agudo", "TREBLE", x=0.23), K("medio", "MID", x=0.29),
                  K("grave", "BASS", x=0.35), P("DEEP", x=0.395, tamanho_m=0.006), K("volume", "MASTER", x=0.44),
                  K("presenca", "PRESENÇA", estilo="extra", x=0.49, z=0.55), L("CLIP", x=0.49, z=0.15),
                  J("SEND", x=0.55), J("RETURN", x=0.6), F("SEND", x=0.65), F("RETURN", x=0.7), P("POST/PRE", x=0.745, tamanho_m=0.006),
                  J("XLR", x=0.8), MT("LIFT", x=0.84), J("FT SW", x=0.88), P("MUTE", x=0.93, tamanho_m=0.007),
                  letra_m=0.004, rotulo_pos="cima")],
    riscos=[RISCO(0.01, 0.03, 0.99, 0.03, 0.0015), RISCO(0.01, 0.97, 0.99, 0.97, 0.0015)],
    notas="O DB 751 não tem presença: ficou num botão pequeno depois do MASTER.",
)

AMARELO = rgb(0.9, 0.72, 0.1)
FICHAS["littletube"] = dict(
    inspiracao=("Markbass Little Mark Tube 800: caixinha de metal de 276 × 83 × 250 mm com painel de bronze escovado, frisos e "
                "letras amarelo-dourados: em cima a entrada XLR, GAIN 2, PHANTOM, MIX, a marca, MASTER e a gangorra preta de "
                "liga; embaixo INPUT, GAIN 1, LED azul, LINE OUT e o EQ de seis botões amarelos (LOW, MID LOW, MID HIGH, HIGH, "
                "VLE, VPF) com o LED ON."),
    formato="caixinha", mm=(276, 83, 250), couro={"cor": [0.03, 0.03, 0.032], "metal": True}, fachada=None, alca="nenhuma",
    painel=dict(posicao="inteiro", cor=rgb(0.32, 0.2, 0.08), texto_cor=AMARELO),
    estilos={"padrao": dict(LISO, diametro_m=0.012), "amarelo": estilo("liso", 0.012, (0.85, 0.65, 0.08), traco=(0.02, 0.02, 0.02), escala="tracos")},
    parafusos_painel=False,
    fileiras=[ROW(0.72, 0.0, 1.0, J("BAL IN", x=0.05), F("GAIN 2", x=0.13), MT("PHANTOM", x=0.19), F("MIX", x=0.26),
                  LOGO(0.0065, x=0.5, z=0.74), K("volume", "MASTER", x=0.78), RB(None, x=0.92, largura_m=0.012, altura_m=0.016),
                  letra_m=0.0028, rotulo_pos="cima"),
              ROW(0.27, 0.0, 1.0, J("INPUT", x=0.05), K("ganho", "GAIN 1", x=0.13), L(None, (0.1, 0.3, 1.0), x=0.18),
                  F("LINE OUT", x=0.24), K("grave", "LOW", estilo="amarelo", x=0.34), K("medio", "MID LOW", estilo="amarelo", x=0.43),
                  K("presenca", "MID HIGH", estilo="amarelo", x=0.52), K("agudo", "HIGH", estilo="amarelo", x=0.61),
                  F("VLE", estilo="amarelo", x=0.7), F("VPF", estilo="amarelo", x=0.79), L(None, (0.1, 0.3, 1.0), aceso=True, x=0.88),
                  letra_m=0.0028, rotulo_pos="baixo")],
    riscos=[RISCO(0.02, 0.5, 0.98, 0.5, 0.0012, cor=AMARELO)],
    notas="Painel bronze (decisão do PROGRESSO). A presença do Nexus ficou no MID HIGH.",
)

FICHAS["vault4pro"] = dict(
    inspiracao=("Ampeg SVT-4 Pro: rack 3U (483 × 142 × 394 mm) de chapa preta com alças de rack, a janela do EQ gráfico de nove "
                "deslizantes e LEVEL no alto ao centro e o painel de controle rebaixado embaixo com letras claras: entrada e "
                "TUNER OUT, GAIN, COMPRESSION, BASS, MIDRANGE, FREQUENCY, TREBLE, LINE OUT, MASTER, CROSSOVER FREQUENCY e "
                "BALANCE em botões pretos, as teclas MUTE, -15 dB, ULTRA HI, ULTRA LO, BRIGHT, GRAPHIC EQ, BYPASS e LIMIT, os "
                "LEDs e a gangorra ON/OFF à direita."),
    formato="rack", rack_u=3, mm=(483, 142, 394), couro={"cor": [0.02, 0.02, 0.022], "metal": True}, fachada=None, alca="rack",
    painel=dict(posicao="inteiro", cor=rgb(0.012, 0.012, 0.013), material="pintado", texto_cor=BRANCO),
    estilos={"padrao": dict(LISO, diametro_m=0.015), "extra": EXTRA},
    fileiras=[ROW(0.72, 0.28, 0.72, SF("33"), SF("80"), SF("150"), SF("300"), SF("600"), SF("900"), SF("2K"), SF("5K"), SF("8K"),
                  SF("LEVEL"), curso_m=0.03, letra_m=0.003, rotulo_pos="baixo"),
              ROW(0.72, 0.0, 1.0, LOGO(0.008, x=0.14, z=0.78), M("VAULT 4 PRO", 0.008, x=0.82, z=0.78),
                  R("ON/OFF", (0.012, 0.012, 0.012), aceso=False, x=0.93, z=0.75), letra_m=0.0034, rotulo_pos="baixo"),
              ROW(0.22, 0.0, 1.0, J("INPUT", x=0.04), P("MUTE", x=0.08, tamanho_m=0.005), P("-15dB", x=0.11, tamanho_m=0.005),
                  K("ganho", "GAIN", x=0.16), F("COMP", x=0.215), P("ULTRA HI", x=0.255, tamanho_m=0.005),
                  P("ULTRA LO", x=0.29, tamanho_m=0.005), K("grave", "BASS", x=0.335), K("medio", "MID", x=0.39), F("FREQ", x=0.445),
                  K("agudo", "TREBLE", x=0.5), P("BRIGHT", x=0.54, tamanho_m=0.005), P("EQ", x=0.575, tamanho_m=0.005),
                  F("LINE", x=0.62), K("volume", "MASTER", x=0.675), K("presenca", "PRESENÇA", estilo="extra", x=0.72, z=0.6),
                  F("XOVER", x=0.77), F("BALANCE", x=0.825), P("LIMIT", x=0.87, tamanho_m=0.005), letra_m=0.0032, rotulo_pos="baixo")],
    riscos=[dict(RISCO(0.26, 0.72, 0.74, 0.72, 0.065, cor=(0.05, 0.05, 0.055))), dict(RISCO(0.02, 0.22, 0.98, 0.22, 0.06, cor=(0.03, 0.03, 0.033)))],
    notas="O EQ gráfico de nove faixas ficou desenhado fixo. O SVT-4 Pro não tem presença: ficou num botão pequeno depois do MASTER.",
)
