# Fichas do lote 3: Marshall (45, 50, Super Bass, 900, 2000, JVM, DSL), Uberschall, Super-Sonic, Bassman tweed.
from cabecotes_fiel import *  # noqa: F401,F403

TOLEX_PRETO = {"cor": [0.006, 0.006, 0.006], "grao_mm": 2.6}
MARSHALL_TELA = tela((0.006, 0.006, 0.006), (0.03, 0.03, 0.03), 1.4, friso=(0.86, 0.85, 0.82))
OURO = rgb(0.5, 0.34, 0.14)
PLEXI = rgb(0.55, 0.38, 0.14)
TINTA = rgb(0.015, 0.012, 0.01)
BRANCO = rgb(0.85, 0.85, 0.85)
MARSHALL = estilo("marshall", 0.026, (0.018, 0.018, 0.018), (0.56, 0.39, 0.17))
MARSHALL_P = estilo("marshall", 0.021, (0.018, 0.018, 0.018), (0.56, 0.39, 0.17), escala="tracos")
FRISO_OURO = {"tipo": "tolex", "cor": [0.006, 0.006, 0.006], "grao_mm": 2.6, "friso": [0.72, 0.52, 0.2]}
FICHAS = {}


def _plexi(nome, sub, mm, largura, rotulos, notas, extra_esq=None):
    a, b = rotulos
    itens = []
    if extra_esq:
        itens.append(TX(extra_esq, x=0.05, z=0.8, letra_m=0.0065))
    itens += [T("MAINS", x=0.06, rotulo_pos="baixo"), T("STANDBY", x=0.115, rotulo_pos="baixo"),
              P("INDICATOR", x=0.17, cor=(0.9, 0.06, 0.02), aceso=True, tamanho_m=0.011, rotulo_pos="baixo"),
              M(nome, 0.017, x=0.29, z=0.58), TX(sub, x=0.29, z=0.28, letra_m=0.0065),
              K("presenca", "PRESENCE", x=0.41), K("grave", "BASS", x=0.48), K("medio", "MIDDLE", x=0.55),
              K("agudo", "TREBLE", x=0.62), K("ganho", a, x=0.69), K("volume", b, x=0.76),
              J(x=0.83, z=0.68), J(x=0.83, z=0.26), J(x=0.9, z=0.68), J(x=0.9, z=0.26),
              TX("I", x=0.83, z=0.95, letra_m=0.006), TX("II", x=0.9, z=0.95, letra_m=0.006)]
    return dict(mm=mm, couro=TOLEX_PRETO, fachada=FRISO_OURO,
                painel=dict(posicao="baixo", altura_m=0.08, largura_m=largura, cor=PLEXI, material="acrilico", texto_cor=TINTA),
                estilos={"padrao": dict(MARSHALL, diametro_m=0.024)}, cantos="preto",
                fileiras=[ROW(0.42, 0.0, 1.0, *itens, letra_m=0.0068, rotulo_pos="baixo")], notas=notas)


FICHAS["brit45"] = dict(
    _plexi("BRIT 45", "MK II", (665, 265, 205), 0.56, ("HIGH TREBLE", "NORMAL"),
           "O JTM45 não tem master: o ganho do Nexus é o LOUDNESS 1 (HIGH TREBLE) e o volume o LOUDNESS 2 (NORMAL)."),
    inspiracao=("Marshall JTM45: cabeçote de 665 × 265 × 205 mm todo em tolex preto, sem tela, com friso dourado e a marca "
                "no meio, e o painel pequeno de plexi dourado embutido embaixo com letras pretas: chaves de alavanca "
                "MAINS e STANDBY, luz INDICATOR vermelha quadrada, o nome BRIT 45, seis botões pretos de tampa dourada "
                "(PRESENCE, BASS, MIDDLE, TREBLE, HIGH TREBLE e NORMAL) e as quatro entradas à direita."),
)
FICHAS["britplexi50"] = dict(
    _plexi("BRIT PLEXI 50", "LEAD 50", (660, 260, 205), 0.56, ("HIGH TREBLE", "NORMAL"),
           "O 1987 não tem master: o ganho do Nexus é o LOUDNESS 1 (HIGH TREBLE) e o volume o LOUDNESS 2 (NORMAL)."),
    inspiracao=("Marshall 1987 Lead 50 W: cabeçote de 660 × 260 × 205 mm todo em tolex preto, sem tela, com friso dourado e "
                "a marca no meio, e o painel pequeno de plexi dourado embutido embaixo: chaves MAINS e STANDBY, luz "
                "INDICATOR vermelha, o nome, seis botões pretos de tampa dourada (PRESENCE, BASS, MIDDLE, TREBLE, "
                "HIGH TREBLE e NORMAL) e as quatro entradas à direita."),
)
FICHAS["britbass"] = dict(
    _plexi("BRIT BASS", "SUPER BASS 100", (740, 305, 210), 0.62, ("VOLUME 1", "VOLUME 2"),
           "O Super Bass não tem master: o ganho do Nexus é o VOLUME 1 e o volume o VOLUME 2."),
    inspiracao=("Marshall Super Bass: cabeçote de 740 × 305 × 210 mm todo em tolex preto, sem tela, com friso dourado e a "
                "marca no meio, e o painel de plexi dourado embutido embaixo: chaves MAINS e STANDBY, luz INDICATOR "
                "vermelha quadrada, o nome, seis botões pretos de tampa dourada (PRESENCE, BASS, MIDDLE, TREBLE, VOLUME 1 "
                "e VOLUME 2) e as quatro entradas (I e II, alta e baixa)."),
)

# ------------------------------------------------------------------ Brit 900
FICHAS["brit900"] = dict(
    inspiracao=("Marshall JCM900: cabeçote de 740 × 310 × 210 mm em tolex preto com cantoneiras pretas, tela de pano preta "
                "com friso branco e a marca no meio, painel dourado escovado na largura toda: chave de força vermelha "
                "acesa e STANDBY preta à esquerda, o jaque do pedal, dez botões pretos de tampa dourada (VOLUME e REVERB "
                "do canal B, VOLUME e REVERB do canal A, PRESENCE, BASS, MIDDLE, TREBLE, LEAD GAIN e GAIN) e a entrada "
                "à direita."),
    mm=(740, 310, 210), couro=TOLEX_PRETO, fachada=MARSHALL_TELA,
    painel=dict(posicao="baixo", altura_m=0.088, cor=OURO, texto_cor=TINTA),
    estilos={"padrao": dict(MARSHALL, diametro_m=0.024)}, cantos="preto",
    fileiras=[ROW(0.42, 0.0, 1.0, R("POWER", x=0.04), RB("STANDBY", x=0.085), J("FOOTSWITCH", x=0.135),
                  M("BRIT 900", 0.016, x=0.215, z=0.5),
                  K("volume", "VOLUME B", x=0.29), F("REVERB B", x=0.345), F("VOLUME A", x=0.4), F("REVERB A", x=0.455),
                  K("presenca", "PRESENCE", x=0.51), K("grave", "BASS", x=0.565), K("medio", "MIDDLE", x=0.62),
                  K("agudo", "TREBLE", x=0.675), K("ganho", "LEAD GAIN", x=0.73), F("GAIN", x=0.785),
                  LC("A", x=0.83, z=0.72), LC("B", x=0.83, z=0.3),
                  J("INPUT", x=0.88), MARCA(x=0.95, z=0.47, tamanho_m=0.0055), letra_m=0.0065, rotulo_pos="baixo")],
    notas="Os LEDs de canal A e B entre o GAIN e a entrada são para a tela; no JCM900 a troca é pelo pedal.",
)

# ------------------------------------------------------------------ Brit 410 (JVM410)
def _jvm():
    cima = [F("MASTER 2"), K("volume", "MASTER 1"), F("RVB OD1"), F("RVB CLEAN"), F("VOLUME"), K("grave", "BASS"),
            K("medio", "MIDDLE"), K("agudo", "TREBLE"), K("ganho", "GAIN"), F("VOLUME"), F("BASS"), F("MIDDLE"),
            F("TREBLE"), F("GAIN")]
    baixo = [F("RESONANCE"), K("presenca", "PRESENCE"), F("RVB OD2"), F("RVB CRUNCH"), F("VOLUME"), F("BASS"),
             F("MIDDLE"), F("TREBLE"), F("GAIN"), F("VOLUME"), F("BASS"), F("MIDDLE"), F("TREBLE"), F("GAIN")]
    return cima, baixo


_c, _b = _jvm()
_tecla = dict(tamanho_m=0.0085)
FICHAS["brit410"] = dict(
    inspiracao=("Marshall JVM410: cabeçote de 740 × 310 × 215 mm em tolex preto, tela de pano preta com friso branco e a "
                "marca no meio, painel dourado escovado com letras pretas: gangorras POWER (vermelha) e STANDBY à "
                "esquerda, o nome BRIT 410, 28 botões de tampa dourada em duas fileiras de 14 (MASTER 2, MASTER 1, "
                "RESONANCE, PRESENCE, os REVERBs e VOLUME, BASS, MIDDLE, TREBLE e GAIN dos canais OD1, CLEAN, OD2 e "
                "CRUNCH; o OD1 é o que toca), oito teclas quadradas pretas iluminadas e a entrada na ponta direita."),
    mm=(740, 310, 215), couro=TOLEX_PRETO, fachada=MARSHALL_TELA,
    painel=dict(posicao="baixo", altura_m=0.1, cor=OURO, texto_cor=TINTA),
    estilos={"padrao": dict(MARSHALL, diametro_m=0.018, escala="tracos")}, cantos="preto",
    fileiras=[
        ROW(0.5, 0.0, 1.0, R("POWER", x=0.03, z=0.62), RB("STANDBY", x=0.03, z=0.22), M("BRIT 410", 0.012, x=0.095, z=0.5),
            letra_m=0.0045, rotulo_pos="baixo"),
        ROW(0.73, 0.16, 0.86, *_c, letra_m=0.0042, rotulo_pos="cima"),
        ROW(0.25, 0.16, 0.86, *_b, letra_m=0.0042, rotulo_pos="baixo"),
        ROW(0.5, 0.0, 1.0, P("FX LOOP", x=0.885, z=0.8, **_tecla), P("MASTER", x=0.915, z=0.8, **_tecla),
            P("CLEAN", x=0.885, z=0.45, cor=(0.1, 0.8, 0.1), aceso=True, **_tecla), P("CRUNCH", x=0.915, z=0.45, **_tecla),
            P("OD1", x=0.885, z=0.12, **_tecla), P("OD2", x=0.915, z=0.12, **_tecla),
            LC("", x=0.9, z=0.62), LC("", x=0.93, z=0.62), LC("", x=0.9, z=0.3),
            J("INPUT", x=0.965, z=0.5), letra_m=0.0038, rotulo_pos="baixo"),
    ],
    notas="As teclas do JVM acendem na cor do modo; aqui três LEDs entre elas mostram o canal ativo na tela (Limpo, Crunch, Overdrive).",
)

# ------------------------------------------------------------------ Brit 2000 (JCM2000 DSL)
CLARO = estilo("marshall", 0.023, (0.018, 0.018, 0.018), (0.72, 0.66, 0.5))
FICHAS["brit2000"] = dict(
    inspiracao=("Marshall JCM2000 DSL: cabeçote de 748 × 297 × 214 mm em tolex preto, tela de pano preta com friso branco "
                "e a marca no meio, painel dourado escovado: gangorras POWER (vermelha) e STANDBY, o nome BRIT 2000, "
                "teclinha DEEP, PRESENCE, BASS, MIDDLE, TREBLE, TONE SHIFT, os dois REVERBs, ULTRA GAIN (VOLUME, LEAD "
                "1/2, GAIN), a tecla CHANNEL com os LEDs, CLASSIC GAIN (VOLUME, CLEAN/CRUNCH, GAIN) e a entrada, em dez "
                "botões de tampa clara."),
    mm=(748, 297, 214), couro=TOLEX_PRETO, fachada=MARSHALL_TELA,
    painel=dict(posicao="baixo", altura_m=0.088, cor=OURO, texto_cor=TINTA),
    estilos={"padrao": CLARO}, cantos="preto",
    fileiras=[ROW(0.42, 0.0, 1.0, R("POWER", x=0.035), RB("STANDBY", x=0.078), M("BRIT 2000", 0.014, x=0.15, z=0.55),
                  TX("DUAL SUPER LEAD", x=0.15, z=0.28, letra_m=0.0055),
                  P("DEEP", x=0.215), K("presenca", "PRESENCE", x=0.255), K("grave", "BASS", x=0.305),
                  K("medio", "MIDDLE", x=0.355), K("agudo", "TREBLE", x=0.405), P("TONE SHIFT", x=0.445),
                  F("CLASSIC", x=0.49), F("ULTRA", x=0.54),
                  K("volume", "VOLUME", x=0.6), P("LEAD 1/2", x=0.64), K("ganho", "GAIN", x=0.68),
                  P("CHANNEL", x=0.725), LC("", x=0.725, z=0.88), LC("", x=0.745, z=0.88), LC("", x=0.705, z=0.88),
                  F("VOLUME", x=0.775), P("CLEAN/CRUNCH", x=0.815), F("GAIN", x=0.855),
                  J("INPUT", x=0.91), letra_m=0.0058, rotulo_pos="baixo"),
              ROW(0.93, 0.0, 1.0, TX("EQUALISATION", x=0.33), TX("REVERB", x=0.515), TX("ULTRA GAIN", x=0.64),
                  TX("CLASSIC GAIN", x=0.815), letra_m=0.0052)],
)

# ------------------------------------------------------------------ Brit DS 20 (DSL20)
FICHAS["britds20"] = dict(
    inspiracao=("Marshall DSL20: cabeçote pequeno de 500 × 250 × 240 mm em tolex preto, tela de pano preta com friso branco "
                "e a marca no meio, painel dourado escovado: gangorras POWER (vermelha) e OUTPUT, dez botões de tampa "
                "dourada (REVERB, RESONANCE, PRESENCE, BASS, MIDDLE, TREBLE, ULTRA VOLUME e GAIN, CLASSIC VOLUME e GAIN), "
                "teclas TONE SHIFT e CHANNEL SELECT com os LEDs e a entrada na ponta direita."),
    mm=(500, 250, 240), couro=TOLEX_PRETO, fachada=MARSHALL_TELA,
    logo={"altura_m": 0.022},
    painel=dict(posicao="baixo", altura_m=0.078, cor=OURO, texto_cor=TINTA),
    estilos={"padrao": estilo("marshall", 0.019, (0.018, 0.018, 0.018), (0.56, 0.39, 0.17), escala="tracos")},
    cantos="preto",
    fileiras=[ROW(0.45, 0.0, 1.0, R("POWER", x=0.045), RB("OUTPUT", x=0.105),
                  F("REVERB", x=0.17), F("RESONANCE", x=0.23), K("presenca", "PRESENCE", x=0.29), K("grave", "BASS", x=0.35),
                  K("medio", "MIDDLE", x=0.41), K("agudo", "TREBLE", x=0.47), P("TONE SHIFT", x=0.515),
                  K("volume", "VOLUME", x=0.565), K("ganho", "GAIN", x=0.625), P("CHANNEL", x=0.675),
                  LC("", x=0.66, z=0.88), LC("", x=0.69, z=0.88), LC("", x=0.675, z=0.06),
                  F("VOLUME", x=0.725), F("GAIN", x=0.785), J("INPUT", x=0.86), M("DSL 20", 0.01, x=0.94, z=0.45),
                  letra_m=0.0048, rotulo_pos="baixo")],
    notas="O DSL20 tem dois LEDs na tecla CHANNEL; o terceiro, embaixo, é para a tela mostrar os três canais.",
)

# ------------------------------------------------------------------ Uber 100 (Bogner Uberschall)
BICO = estilo("chicken", 0.021, (0.015, 0.015, 0.015), traco=(0.9, 0.9, 0.9), escala="tracos")
PRATA = rgb(0.7, 0.71, 0.72)
FICHAS["uber100"] = dict(
    inspiracao=("Bogner Uberschall: cabeçote de 692 × 241 × 260 mm em tolex preto com estampa em relevo, frente de malha "
                "escura fina com a marca, e painel preto embaixo com duas barras prateadas de rótulo, uma sobre cada "
                "canal: alavanca POWER/STANDBY com LED azul e a plaqueta UBER 100 à esquerda, M. VOLUME, canal 1 "
                "(VOLUME, GAIN, BASS, MIDDLE, TREBLE, PRESENCE, o que toca) e canal 2 (GAIN, BASS, MIDDLE, TREBLE, VOLUME, "
                "PRESENCE) em botões pretos bico-de-galinha, a chave de canal e a entrada à direita."),
    mm=(692, 241, 260),
    couro={"cor": [0.008, 0.008, 0.009], "grao_mm": 4.0},
    fachada=tela((0.012, 0.012, 0.013), (0.03, 0.03, 0.032), 1.2),
    logo={"altura_m": 0.024},
    painel=dict(posicao="baixo", altura_m=0.085, cor=rgb(0.01, 0.01, 0.011), material="pintado", texto_cor=TINTA),
    estilos={"padrao": BICO}, cantos="preto", parafusos_painel=False,
    fileiras=[ROW(0.4, 0.0, 1.0, T(None, x=0.035), L(None, (0.1, 0.3, 1.0), aceso=True, x=0.035, z=0.85),
                  M("UBER 100", 0.011, x=0.1, z=0.42, cor=(0.8, 0.8, 0.8)),
                  K("volume", "M. VOLUME", x=0.17), F("VOLUME", x=0.225), K("ganho", "GAIN", x=0.28),
                  K("grave", "BASS", x=0.335), K("medio", "MIDDLE", x=0.39), K("agudo", "TREBLE", x=0.445),
                  K("presenca", "PRESENCE", x=0.5), F("GAIN", x=0.575), F("BASS", x=0.63), F("MIDDLE", x=0.685),
                  F("TREBLE", x=0.74), F("VOLUME", x=0.795), F("PRESENCE", x=0.85), MT(None, x=0.9),
                  J(None, x=0.955), letra_m=0.0058, rotulo_pos="cima")],
    riscos=[RISCO(0.2, 0.83, 0.53, 0.83, 0.014, cor=PRATA), RISCO(0.55, 0.83, 0.87, 0.83, 0.014, cor=PRATA)],
)

# ------------------------------------------------------------------ Sonic 60 (Super-Sonic)
RADIO = estilo("saia_numerada", 0.019, (0.82, 0.78, 0.66), (0.82, 0.78, 0.66), saia=(0.82, 0.78, 0.66),
               numeros=(0.02, 0.02, 0.02), traco=(0.02, 0.02, 0.02), escala="nenhuma")
FICHAS["sonic60"] = dict(
    inspiracao=("Fender Super-Sonic (loiro): cabeçote de 643 × 219 × 244 mm em tolex loiro, tela marrom oxblood com a "
                "marca à esquerda, painel marrom escuro na faixa de cima dividido em três blocos por frisos prateados: "
                "entrada, canal VINTAGE (VOLUME, minichave de voz, TREBLE, BASS), minichave VINTAGE/BURN com LED, canal "
                "BURN (GAIN 1, GAIN 2, TREBLE, BASS, MIDDLE, VOLUME), REVERB, o nome em script e a joia vermelha, em "
                "botões creme tipo rádio numerados."),
    mm=(643, 219, 244),
    couro={"cor": [0.62, 0.52, 0.36], "grao_mm": 2.0},
    fachada=tela((0.16, 0.04, 0.03), (0.08, 0.02, 0.015), 2.0, friso=(0.82, 0.78, 0.66)),
    logo={"lado_rel": -0.3, "altura_m": 0.016, "altura_rel": 0.78},
    painel=dict(posicao="cima", altura_m=0.068, cor=rgb(0.06, 0.025, 0.018), material="pintado", texto_cor=BRANCO,
                friso=(0.75, 0.75, 0.76)),
    estilos={"padrao": RADIO}, cantos="nenhum",
    fileiras=[ROW(0.55, 0.0, 1.0, J("INPUT", x=0.03), F("VOLUME", x=0.085), MT(None, x=0.12), F("TREBLE", x=0.155),
                  F("BASS", x=0.215), MT("VINTAGE / BURN", x=0.27), LC("", x=0.27, z=0.88),
                  K("ganho", "GAIN 1", x=0.33), F("GAIN 2", x=0.39), K("agudo", "TREBLE", x=0.45), K("grave", "BASS", x=0.51),
                  K("medio", "MIDDLE", x=0.57), K("volume", "VOLUME", x=0.63), K("presenca", "REVERB", x=0.7),
                  M("Sonic 60", 0.016, x=0.82, z=0.55, fonte="script", cor=(0.85, 0.85, 0.85)),
                  JW((0.95, 0.05, 0.03), x=0.955, raio_m=0.007), letra_m=0.0055, rotulo_pos="baixo")],
    riscos=[RISCO(0.3, 0.05, 0.3, 0.95, 0.0012, cor=(0.7, 0.7, 0.72)), RISCO(0.665, 0.05, 0.665, 0.95, 0.0012, cor=(0.7, 0.7, 0.72))],
    notas="O Super-Sonic não tem presença: a presença do Nexus ficou no REVERB (o volume é o VOLUME do canal Burn).",
)

# ------------------------------------------------------------------ Tweed 59 (Bassman 5F6-A, combo)
FICHAS["tweed59"] = dict(
    inspiracao=("Fender Bassman 5F6-A: combo 4×10 de 597 × 568 × 270 mm em tweed laqueado cor de mel, tela marrom com "
                "listras douradas e a marca no alto, e o painel cromado no tampo, virado para trás, com letras pretas: "
                "chaves LIGA e STANDBY, joia vermelha, seis botões pretos de ponteiro (PRESENCE, MIDDLE, BASS, TREBLE, "
                "VOLUME BRIGHT e VOLUME NORMAL) e as quatro entradas na ponta."),
    formato="combo",
    mm=(597, 568, 270),
    couro={"cor": [0.62, 0.4, 0.13], "tweed": True, "grao_mm": 3.2},
    fachada={"tipo": "listras", "cor": [0.18, 0.09, 0.04], "linha": [0.62, 0.46, 0.18], "passo_mm": 8.0},
    logo={"lado_rel": -0.25, "altura_rel": 0.9, "altura_m": 0.016},
    painel=dict(posicao="cima", altura_m=0.07, largura_m=0.5, cor=rgb(0.86, 0.86, 0.88), material="cromo", texto_cor=TINTA),
    estilos={"padrao": estilo("chicken", 0.022, (0.012, 0.012, 0.012), traco=(0.9, 0.9, 0.9), escala="nenhuma")},
    cantos="nenhum", aba_m=0.024,
    fileiras=[ROW(0.45, 0.0, 1.0, T("STANDBY", x=0.06), T("ON", x=0.11), JW((0.9, 0.05, 0.02), x=0.16, raio_m=0.007),
                  K("presenca", "PRESENCE", x=0.24), K("medio", "MIDDLE", x=0.33), K("grave", "BASS", x=0.42),
                  K("agudo", "TREBLE", x=0.51), K("ganho", "BRIGHT", x=0.6), K("volume", "NORMAL", x=0.69),
                  J(x=0.78, z=0.7), J(x=0.78, z=0.25), J(x=0.87, z=0.7), J(x=0.87, z=0.25),
                  TX("BRIGHT", x=0.78, z=0.96), TX("NORMAL", x=0.87, z=0.96), letra_m=0.0055, rotulo_pos="baixo")],
    notas="Bassman sem master: o ganho é o VOLUME BRIGHT e o volume o VOLUME NORMAL.",
)
