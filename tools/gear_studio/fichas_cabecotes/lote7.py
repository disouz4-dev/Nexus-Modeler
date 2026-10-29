# Fichas do lote 7: combos (Fender blackface, tweed e Hot Rod, Roland JC-120, Polytone, Ampeg BA-210) e os flip-tops Ampeg.
from cabecotes_fiel import *  # noqa: F401,F403

BRANCO = rgb(0.85, 0.85, 0.85)
PRETA = rgb(0.012, 0.012, 0.012)
TINTA = rgb(0.015, 0.012, 0.01)
FENDER = estilo("saia_numerada", 0.02, (0.012, 0.012, 0.012), (0.75, 0.76, 0.78), saia=(0.012, 0.012, 0.012),
                numeros=(0.85, 0.85, 0.85), traco=(0.9, 0.9, 0.9), escala="nenhuma", tampa_metal=True)
EXTRA = estilo("liso", 0.011, (0.015, 0.015, 0.015), traco=(0.9, 0.9, 0.9), escala="nenhuma")
SILVER_TELA = tela((0.5, 0.51, 0.52), (0.36, 0.37, 0.39), 1.8, friso=(0.75, 0.75, 0.75))
TOLEX_COMBO = {"cor": [0.007, 0.007, 0.007], "grao_mm": 2.2}
FICHAS = {}


def _blackface(mm, logo_rel, normal, vibrato, nome, notas, inspiracao, joia_x=0.96):
    return dict(
        inspiracao=inspiracao, formato="combo", mm=mm, couro=TOLEX_COMBO, fachada=SILVER_TELA,
        logo={"lado_rel": logo_rel, "altura_rel": 0.86, "altura_m": 0.016},
        painel=dict(posicao="cima", altura_m=0.068, cor=rgb(0.008, 0.008, 0.009), material="pintado", texto_cor=BRANCO,
                    friso=(0.78, 0.79, 0.8)),
        estilos={"padrao": FENDER, "extra": EXTRA}, cantos="cromo",
        fileiras=[ROW(0.55, 0.0, 1.0, *normal, *vibrato, M(nome, 0.014, x=joia_x - 0.095, z=0.6, fonte="script", cor=(0.85, 0.85, 0.85)),
                      JW((0.95, 0.05, 0.03), x=joia_x, raio_m=0.0075), rotulo_pos="baixo", letra_m=0.0052)],
        notas=notas,
    )


FICHAS["silverdeluxe"] = _blackface(
    (622, 445, 241), -0.3,
    [J("1", x=0.035), J("2", x=0.075), TX("NORMAL", x=0.055, z=0.18, letra_m=0.0058), F("VOLUME", x=0.13), F("TREBLE", x=0.19), F("BASS", x=0.25)],
    [J("1", x=0.32), J("2", x=0.36), TX("VIBRATO", x=0.34, z=0.18, letra_m=0.0058), K("ganho", "VOLUME", x=0.42),
     K("agudo", "TREBLE", x=0.48), K("grave", "BASS", x=0.54), F("REVERB", x=0.6), F("SPEED", x=0.66),
     K("presenca", "INTENSITY", x=0.72), K("medio", "MÉDIO", estilo="extra", x=0.775, z=0.7),
     K("volume", "VOLUME", estilo="extra", x=0.775, z=0.3)],
    "Silver Deluxe",
    "O Deluxe Reverb não tem médio, presença nem master: a presença ficou no INTENSITY e médio e volume em dois botões pequenos depois dele.",
    ("Fender Deluxe Reverb (blackface): combo 1×12 de 622 × 445 × 241 mm em tolex preto, tela prateada com a marca no alto à "
     "esquerda e painel preto com friso prateado na faixa de cima: canal NORMAL (entradas 1 e 2, VOLUME, TREBLE, BASS) e "
     "canal VIBRATO (entradas, VOLUME, TREBLE, BASS, REVERB, SPEED, INTENSITY; o que toca), botões pretos de saia numerada, "
     "o nome em script e a joia vermelha na ponta direita."),
)
FICHAS["silversuper"] = _blackface(
    (638, 632, 267), -0.3,
    [J("1", x=0.03), J("2", x=0.065), MT("BRIGHT", x=0.095), TX("NORMAL", x=0.05, z=0.18, letra_m=0.0058),
     K("volume", "VOLUME", x=0.14), F("TREBLE", x=0.19), F("BASS", x=0.24)],
    [J("1", x=0.29), J("2", x=0.325), MT("BRIGHT", x=0.355), TX("VIBRATO", x=0.31, z=0.18, letra_m=0.0058),
     K("ganho", "VOLUME", x=0.4), K("agudo", "TREBLE", x=0.45), K("medio", "MIDDLE", x=0.5), K("grave", "BASS", x=0.55),
     F("REVERB", x=0.6), F("SPEED", x=0.65), K("presenca", "INTENSITY", x=0.7)],
    "Silver Super",
    "O Super Reverb não tem presença nem master: a presença ficou no INTENSITY e o volume de saída no VOLUME do canal Normal.",
    ("Fender Super Reverb (blackface): combo 4×10 de 638 × 632 × 267 mm em tolex preto, tela prateada com a marca no alto à "
     "esquerda e painel preto com friso prateado em cima: canal NORMAL (entradas, BRIGHT, VOLUME, TREBLE, BASS) e canal "
     "VIBRATO (entradas, BRIGHT, VOLUME, TREBLE, MIDDLE, BASS, REVERB, SPEED, INTENSITY; o que toca), botões pretos de saia "
     "numerada, o nome em script e a joia vermelha."),
)
FICHAS["silverprince"] = _blackface(
    (505, 406, 241), -0.28,
    [J("1", x=0.05), J("2", x=0.1), TX("INSTRUMENTS", x=0.075, z=0.18, letra_m=0.0058)],
    [K("ganho", "VOLUME", x=0.18), K("agudo", "TREBLE", x=0.25), K("grave", "BASS", x=0.32), F("REVERB", x=0.39),
     F("SPEED", x=0.46), K("presenca", "INTENSITY", x=0.53), K("medio", "MÉDIO", estilo="extra", x=0.6, z=0.7),
     K("volume", "VOLUME", estilo="extra", x=0.6, z=0.3)],
    "Silver Prince",
    "O Princeton Reverb não tem médio, presença nem master: a presença ficou no INTENSITY e médio e volume em dois botões pequenos.",
    ("Fender Princeton Reverb (blackface): combo 1×10 de 505 × 406 × 241 mm em tolex preto, tela prateada com a marca no alto "
     "à esquerda e painel preto com friso prateado em cima: entradas 1 e 2 (INSTRUMENTS), VOLUME, TREBLE, BASS, REVERB, SPEED "
     "e INTENSITY em botões pretos de saia numerada, o nome em script e a joia vermelha."),
    joia_x=0.94,
)

# ------------------------------------------------------------------ Tweed 57 (5E3) e Champion 5 (Champ), painel cromado no tampo
TWEED = {"cor": [0.62, 0.4, 0.13], "tweed": True, "grao_mm": 3.2}
TWEED_TELA = {"tipo": "listras", "cor": [0.18, 0.09, 0.04], "linha": [0.62, 0.46, 0.18], "passo_mm": 8.0}
PONTEIRO = estilo("chicken", 0.022, (0.012, 0.012, 0.012), traco=(0.9, 0.9, 0.9), escala="nenhuma")
FICHAS["tweed57"] = dict(
    inspiracao=("Fender Tweed Deluxe 5E3: combo 1×12 de 508 × 425 × 241 mm em tweed laqueado, tela marrom com listras douradas "
                "e a marca no alto, e o painel cromado no tampo, virado para trás, com letras pretas: chaves STANDBY e ON, "
                "porta-fusível, joia vermelha, três botões pretos de ponteiro (TONE, INST VOL, MIC VOL) e as quatro entradas "
                "INST e MIC."),
    formato="combo", mm=(508, 425, 241), couro=TWEED, fachada=TWEED_TELA,
    logo={"lado_rel": -0.25, "altura_rel": 0.88, "altura_m": 0.014},
    painel=dict(posicao="cima", altura_m=0.065, largura_m=0.42, cor=rgb(0.86, 0.86, 0.88), material="cromo", texto_cor=TINTA),
    estilos={"padrao": PONTEIRO, "extra": EXTRA}, cantos="nenhum", aba_m=0.024,
    fileiras=[ROW(0.45, 0.0, 1.0, T("STANDBY", x=0.05), J("FUSE", x=0.1), T("ON", x=0.15), JW((0.9, 0.05, 0.02), x=0.2, raio_m=0.007),
                  K("agudo", "TONE", x=0.28), K("ganho", "INST VOL", x=0.37), K("volume", "MIC VOL", x=0.46),
                  K("grave", "GRAVE", estilo="extra", x=0.53, z=0.72), K("medio", "MÉDIO", estilo="extra", x=0.58, z=0.72),
                  K("presenca", "PRESENÇA", estilo="extra", x=0.63, z=0.72),
                  J(x=0.73, z=0.7), J(x=0.73, z=0.25), J(x=0.83, z=0.7), J(x=0.83, z=0.25),
                  TX("INST", x=0.73, z=0.96), TX("MIC", x=0.83, z=0.96), letra_m=0.0052, rotulo_pos="baixo")],
    notas="O 5E3 só tem TONE e dois VOLUMEs: grave, médio e presença do Nexus ficaram em três botões pequenos no painel.",
)
FICHAS["champion5"] = dict(
    inspiracao=("Fender Champ (tweed): combo pequeno de 343 × 318 × 193 mm em tweed laqueado cor de mostarda, tela marrom com "
                "fios dourados e a marca no alto, e o painel cromado espelhado no tampo com letras pretas: alavanca OFF/ON, "
                "joia vermelha, o botão VOLUME preto de ponteiro com a escala de 1 a 12 impressa e as entradas INSTRUMENTS 1 e 2."),
    formato="combo", mm=(343, 318, 193), couro=TWEED, fachada=TWEED_TELA,
    logo={"lado_rel": 0.0, "altura_rel": 0.86, "altura_m": 0.011},
    painel=dict(posicao="cima", altura_m=0.06, largura_m=0.29, cor=rgb(0.86, 0.86, 0.88), material="cromo", texto_cor=TINTA),
    estilos={"padrao": dict(PONTEIRO, escala="numeros"), "extra": dict(EXTRA, diametro_m=0.009)}, cantos="nenhum", aba_m=0.02,
    alca="couro",
    fileiras=[ROW(0.45, 0.0, 1.0, T("OFF/ON", x=0.08), JW((0.9, 0.05, 0.02), x=0.18, raio_m=0.0065), K("ganho", "VOLUME", x=0.34),
                  K("agudo", "AGUDO", estilo="extra", x=0.5, z=0.75), K("medio", "MÉDIO", estilo="extra", x=0.58, z=0.75),
                  K("grave", "GRAVE", estilo="extra", x=0.66, z=0.75), K("presenca", "PRESENÇA", estilo="extra", x=0.54, z=0.3),
                  K("volume", "VOLUME", estilo="extra", x=0.62, z=0.3),
                  J("1", x=0.8), J("2", x=0.9), TX("INSTRUMENTS", x=0.85, z=0.92), letra_m=0.0042, rotulo_pos="baixo")],
    notas="O Champ só tem VOLUME: agudo, médio, grave, presença e o volume de saída do Nexus ficaram em cinco botões pequenos.",
)

# ------------------------------------------------------------------ Hot Rod 40 (Hot Rod Deluxe, painel no tampo)
MARFIM = estilo("chicken", 0.02, (0.82, 0.78, 0.66), traco=(0.02, 0.02, 0.02), escala="tracos")
FICHAS["hotrod40"] = dict(
    inspiracao=("Fender Hot Rod Deluxe: combo 1×12 de 597 × 476 × 265 mm em vinil preto, tela prateada envelhecida com a marca "
                "no alto à esquerda e o painel PRETO no tampo, virado para trás, com letras brancas: entradas 1 e 2, teclas "
                "BRIGHT, MORE DRIVE e CHANNEL SELECT, oito botões marfim bico-de-galinha (VOLUME, DRIVE, TREBLE, BASS, MIDDLE, "
                "MASTER, REVERB, PRESENCE), os jaques PREAMP OUT, POWER AMP IN e FOOT SWITCH, a joia vermelha e as alavancas "
                "STANDBY e POWER."),
    formato="combo", mm=(597, 476, 265), couro={"cor": [0.008, 0.008, 0.008], "grao_mm": 2.2},
    fachada=tela((0.5, 0.48, 0.42), (0.3, 0.29, 0.26), 1.8, friso=(0.75, 0.73, 0.68)),
    logo={"lado_rel": -0.3, "altura_rel": 0.86, "altura_m": 0.016},
    painel=dict(posicao="cima", altura_m=0.07, largura_m=0.54, cor=rgb(0.01, 0.01, 0.011), material="pintado", texto_cor=BRANCO),
    estilos={"padrao": MARFIM}, cantos="nenhum",
    fileiras=[ROW(0.45, 0.0, 1.0, J("1", x=0.03), J("2", x=0.065), P("BRIGHT", x=0.1, tamanho_m=0.007), F("VOLUME", x=0.145),
                  P("MORE DRIVE", x=0.19, tamanho_m=0.007), K("ganho", "DRIVE", x=0.235), P("CHANNEL", x=0.28, tamanho_m=0.007),
                  LC("", x=0.28, z=0.88), LC("", x=0.3, z=0.88),
                  K("agudo", "TREBLE", x=0.33), K("grave", "BASS", x=0.385), K("medio", "MIDDLE", x=0.44),
                  K("volume", "MASTER", x=0.495), F("REVERB", x=0.55), K("presenca", "PRESENCE", x=0.605),
                  J("PRE OUT", x=0.66), J("PWR IN", x=0.7), J("FT SW", x=0.74), JW((0.95, 0.05, 0.03), x=0.8, raio_m=0.007),
                  T("STANDBY", x=0.86), T("POWER", x=0.92), letra_m=0.0045, rotulo_pos="baixo")],
)

# ------------------------------------------------------------------ Jazz 120 (Roland JC-120, combo)
JC = estilo("marshall", 0.02, (0.015, 0.015, 0.015), (0.75, 0.76, 0.78), traco=(0.02, 0.02, 0.02), escala="tracos")
OLIVA = rgb(0.42, 0.36, 0.16)
FICHAS["jazz120"] = dict(
    inspiracao=("Roland JC-120: combo 2×12 de 760 × 622 × 280 mm em tolex preto com tachas, tela preta de malha prateada com "
                "frisos de alumínio e a marca no alto, e painel cinza-grafite na faixa de cima com letras brancas: entradas e "
                "chave BRI de cada canal, canal 1 (VOLUME, TREBLE, MIDDLE, BASS) e canal 2 (VOLUME, TREBLE, MIDDLE, BASS, "
                "DISTORTION, REVERB; o que toca), a seção dourada SPEED, DEPTH e o seletor de VIB/CHORUS, o nome, a luz vermelha "
                "e a alavanca POWER, em botões pretos de tampa prateada."),
    formato="combo", mm=(760, 622, 280), couro=TOLEX_COMBO,
    fachada=tela((0.02, 0.02, 0.022), (0.32, 0.32, 0.34), 3.2, friso=(0.7, 0.71, 0.72)),
    logo={"lado_rel": 0.0, "altura_rel": 0.86, "altura_m": 0.02},
    painel=dict(posicao="cima", altura_m=0.08, cor=rgb(0.05, 0.05, 0.055), material="pintado", texto_cor=BRANCO),
    estilos={"padrao": JC}, cantos="nenhum",
    fileiras=[ROW(0.5, 0.0, 1.0, J("HIGH", x=0.025, z=0.7), J("LOW", x=0.025, z=0.25), MT("BRI", x=0.055),
                  F("VOLUME", x=0.09), K("presenca", "TREBLE", x=0.135), F("MIDDLE", x=0.18), F("BASS", x=0.225),
                  J("HIGH", x=0.27, z=0.7), J("LOW", x=0.27, z=0.25), MT("BRI", x=0.3),
                  K("volume", "VOLUME", x=0.335), K("agudo", "TREBLE", x=0.38), K("medio", "MIDDLE", x=0.425),
                  K("grave", "BASS", x=0.47), K("ganho", "DISTORTION", x=0.515), F("REVERB", x=0.56),
                  F("SPEED", x=0.615), F("DEPTH", x=0.66), F("VIB/CHO", x=0.705),
                  M("JAZZ 120", 0.014, x=0.8, z=0.55, cor=(0.85, 0.85, 0.85)), JW((0.9, 0.05, 0.02), x=0.885, raio_m=0.006),
                  T("POWER", x=0.94), letra_m=0.0048, rotulo_pos="cima")],
    riscos=[dict(RISCO(0.595, 0.5, 0.725, 0.5, 0.07, cor=OLIVA)), dict(RISCO(0.75, 0.5, 0.85, 0.5, 0.05, cor=(0.01, 0.01, 0.01)))],
    notas="O JC-120 não tem presença: a presença do Nexus é o TREBLE do canal 1.",
)

# ------------------------------------------------------------------ Bebop 15 (Polytone Mini Brute, painel no tampo)
FICHAS["bebop15"] = dict(
    inspiracao=("Polytone Mini Brute: combo 1×15 de cerca de 445 × 483 × 279 mm em tolex preto com cantos plásticos, grade de "
                "metal expandido preta na frente com a marca e o painel bordô escuro recuado no tampo com letras brancas: "
                "alavanca POWER com LED, chave BOOST/CUT, entradas HI e LO GAIN, MASTER VOLUME, GAIN, CONTOUR, o seletor "
                "PROGRAM, BASS, MID, TREBLE, VOLUME e REVERB em botões pretos pequenos de ponteiro branco."),
    formato="combo", mm=(445, 483, 279), couro=TOLEX_COMBO,
    fachada={"tipo": "losango", "cor": [0.02, 0.02, 0.022], "furo_mm": 7.0, "passo_mm": 8.5},
    logo={"lado_rel": 0.0, "altura_rel": 0.85, "altura_m": 0.016},
    painel=dict(posicao="cima", altura_m=0.065, largura_m=0.38, cor=rgb(0.08, 0.015, 0.02), material="pintado", texto_cor=BRANCO),
    estilos={"padrao": estilo("pointer", 0.016, (0.015, 0.015, 0.015), traco=(0.9, 0.9, 0.9), escala="tracos")},
    cantos="preto",
    fileiras=[ROW(0.45, 0.0, 1.0, T("POWER", x=0.04), L(None, (0.9, 0.05, 0.02), aceso=True, x=0.04, z=0.9), MT("BOOST", x=0.09),
                  J("HI", x=0.14), J("LO", x=0.19), K("volume", "MASTER", x=0.25), K("ganho", "GAIN", x=0.32),
                  K("presenca", "CONTOUR", x=0.39), F("PROGRAM", x=0.46), K("grave", "BASS", x=0.53), K("medio", "MID", x=0.6),
                  K("agudo", "TREBLE", x=0.67), F("VOLUME", x=0.74), F("REVERB", x=0.81), J("FX", x=0.88), J("OUT", x=0.94),
                  letra_m=0.0042, rotulo_pos="baixo")],
    notas="Medidas de anúncio (sem fonte do fabricante). A presença do Nexus é o CONTOUR.",
)

# ------------------------------------------------------------------ Vault 210 (Ampeg BA-210, combo)
FICHAS["vault210"] = dict(
    inspiracao=("Ampeg BA-210: combo 2×10 de 508 × 630 × 381 mm em tolex preto com cantos pretos, painel preto na faixa de cima "
                "com letras brancas e a tela preta embaixo com a marca no canto de cima à esquerda: entrada e tecla -15 dB, "
                "SCRAMBLER (tecla, DRIVE e BLEND), VOLUME, teclas ULTRA HI e ULTRA LO, BASS, MID, TREBLE, AUX LEVEL, as "
                "conexões e a chave POWER à direita."),
    formato="combo", mm=(508, 630, 381), couro=TOLEX_COMBO,
    fachada=tela((0.012, 0.012, 0.013), (0.04, 0.04, 0.045), 2.0),
    logo={"lado_rel": -0.28, "altura_rel": 0.9, "altura_m": 0.013},
    painel=dict(posicao="cima", altura_m=0.07, cor=rgb(0.01, 0.01, 0.011), material="pintado", texto_cor=BRANCO),
    estilos={"padrao": estilo("liso", 0.016, (0.015, 0.015, 0.015), traco=(0.9, 0.9, 0.9), escala="tracos"), "extra": EXTRA},
    cantos="preto",
    fileiras=[ROW(0.45, 0.0, 1.0, J("INPUT", x=0.04), P("-15dB", x=0.085, tamanho_m=0.0065), P("SCRAMBLER", x=0.13, tamanho_m=0.0065),
                  K("ganho", "DRIVE", x=0.18), F("BLEND", x=0.235), K("volume", "VOLUME", x=0.3), P("ULTRA HI", x=0.345, tamanho_m=0.0065),
                  P("ULTRA LO", x=0.385, tamanho_m=0.0065), K("grave", "BASS", x=0.43), K("medio", "MID", x=0.485),
                  K("agudo", "TREBLE", x=0.54), F("AUX", x=0.595), K("presenca", "PRESENÇA", estilo="extra", x=0.64, z=0.7),
                  J("AUX IN", x=0.7), J("PHONES", x=0.75), J("FT SW", x=0.8), T("POWER", x=0.93), letra_m=0.0042, rotulo_pos="baixo")],
    notas="O BA-210 não tem presença: ficou num botão pequeno depois do AUX LEVEL.",
)

# ------------------------------------------------------------------ Flip-tops Ampeg (B-15 e B-18)
BLUE_CHECK = {"cor": [0.05, 0.07, 0.16], "xadrez": [0.01, 0.012, 0.02], "passo_mm": 2.6}
AMPEG_SAIA = estilo("saia_numerada", 0.015, (0.012, 0.012, 0.012), (0.78, 0.78, 0.8), saia=(0.75, 0.76, 0.78),
                    numeros=(0.02, 0.02, 0.02), traco=(0.02, 0.02, 0.02), escala="nenhuma", tampa_metal=True)
AZUL = rgb(0.05, 0.12, 0.45)


def _flip(mm, caixa_h, entradas, extras_x, inspiracao, notas, cor_painel, texto):
    canal = lambda n, f: ([K("ganho", "VOLUME"), K("agudo", "TREBLE"), K("grave", "BASS")] if f else
                          [K("volume", "VOLUME"), K("presenca", "TREBLE"), F("BASS")])
    return dict(
        inspiracao=inspiracao, formato="fliptop", mm=mm, caixa_altura_m=caixa_h, couro=BLUE_CHECK,
        fachada=tela((0.45, 0.46, 0.48), (0.12, 0.12, 0.13), 2.0),
        logo={"lado_rel": 0.0, "altura_rel": 0.86, "altura_m": 0.016},
        painel=dict(posicao="inteiro", cor=cor_painel, texto_cor=texto), alca="nenhuma", cantos="nenhum",
        estilos={"padrao": AMPEG_SAIA, "extra": EXTRA}, parafusos_painel=False,
        fileiras=[ROW(0.48, 0.0, 1.0, *entradas, letra_m=0.005, rotulo_pos="baixo"),
                  ROW(0.48, 0.27, 0.48, *canal("ONE", True), letra_m=0.005, rotulo_pos="baixo"),
                  ROW(0.48, 0.5, 0.71, *canal("TWO", False), letra_m=0.005, rotulo_pos="baixo"),
                  ROW(0.48, 0.0, 1.0, K("medio", "MÉDIO", estilo="extra", x=extras_x, z=0.72), M(None, 0.011, x=0.8, z=0.6),
                      T("POLARITY", x=0.86, z=0.4), JW((0.9, 0.05, 0.02), x=0.91, raio_m=0.0065), T("POWER", x=0.96, z=0.4),
                      letra_m=0.005, rotulo_pos="baixo"),
                  ROW(0.92, 0.0, 1.0, TX("ONE", x=0.375), TX("TWO", x=0.605), letra_m=0.005)],
        riscos=[RISCO(0.25, 0.08, 0.25, 0.92, cor=texto), RISCO(0.495, 0.08, 0.495, 0.92, cor=texto),
                RISCO(0.735, 0.08, 0.735, 0.92, cor=texto)],
        notas=notas,
    )


FICHAS["fliptop15"] = _flip(
    (560, 820, 380), 0.67,
    [J(x=0.06, z=0.7), J(x=0.06, z=0.25), J(x=0.13, z=0.7), J(x=0.13, z=0.25), TX("ONE", x=0.06, z=0.95), TX("TWO", x=0.13, z=0.95)],
    0.765,
    ("Ampeg B-15 Portaflex: flip-top em tolex xadrez azul (Blue Check), a caixa 1×15 com tela prata salpicada e a marca no alto "
     "e o chassi em cima com a gaiola das válvulas entre os dois blocos dos transformadores; na frente do chassi o painel de "
     "alumínio escovado com letras e quadros azuis: quatro entradas em quadro, canal ONE (VOLUME, TREBLE, BASS; o que toca) e "
     "canal TWO (VOLUME, TREBLE, BASS) em botões pretos de saia cromada numerada, a chave POLARITY, a luz piloto vermelha e a "
     "chave POWER."),
    ("Medidas: o chassi (405 × 250 × 300 mm nas fontes) em cima de uma caixa 1×15 estimada; total 560 × 860 × 380. O volume do "
     "Nexus é o VOLUME do canal TWO e a presença o TREBLE dele; o médio ficou num botão pequeno."),
    rgb(0.66, 0.67, 0.68), AZUL,
)
FICHAS["fliptop18"] = _flip(
    (600, 900, 400), 0.75,
    [J(x=0.05), J(x=0.11), J(x=0.17), TX("INPUTS", x=0.11, z=0.9)],
    0.765,
    ("Ampeg B-18 Portaflex: flip-top em tolex xadrez azul-marinho (Blue Check), a caixa 1×18 com tela cinza-prateada e a marca "
     "no alto e o chassi em cima com as válvulas entre os blocos dos transformadores; na frente do chassi o painel cromado "
     "estreito com letras pretas: três entradas, VOLUME, TREBLE e BASS dos canais 1 (o que toca) e 2 em botões pretos de saia, "
     "e as alavancas STANDBY, GROUND e ON/OFF à direita."),
    ("Medidas do B-18 não encontradas: estimadas a partir do B-15 com a caixa 1×18 maior (600 × 940 × 400). O volume do Nexus é "
     "o VOLUME do canal 2 e a presença o TREBLE dele; o médio ficou num botão pequeno."),
    rgb(0.86, 0.86, 0.88), PRETA,
)
