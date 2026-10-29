# Receitas dos pedais na modelagem fiel: cada pedal com a cor, o tamanho e os
# controles do original (dados em docs/modelagem/verificado_pedais_*.json).
# Reescreve o conteúdo das receitas (nunca o nome do arquivo nem a "familia"),
# guardando o "id" dos botões que já funcionam no som.
#
# Uso: python3 fiel_receitas.py id [id ...]   (depois: fiel_arte.py nos mesmos)
import json
import os
import sys

AQUI = os.path.dirname(os.path.abspath(__file__))
REC = os.path.join(AQUI, "..", "recipes")

# Campos de layout antigos que a receita nova sempre redefine.
LIMPAR = ("logo", "logo_x", "logo_y", "logo_m", "logo_script", "tipo_texto", "pisante_y", "estampa_ondas", "aro_cromo",
          "visor", "led", "led_no_basculante", "modelo_x", "modelo_m", "basculante_rel", "base_rel", "borracha_rel",
          "etiqueta_y", "transformacao", "aparencia", "deslizantes", "deslizantes_layout", "texto_cor2", "chanfro_m",
          "rotulo_m", "rotulo_embaixo", "quadro", "pisante", "pecas", "paineis", "textos", "cunha", "disposicao",
          "pintura_lados", "led_cor", "modelo_oculto", "modelo_cor", "jacks_topo", "base_m", "talao_m", "inclinacao", "etiqueta_estilo",
          "borracha_larg", "tampo_margem_m")


def lin(h):
    h = h.lstrip("#")
    return [round((int(h[i:i + 2], 16) / 255) ** 2.2, 4) for i in (0, 2, 4)]


PRETO = "#141414"
BRANCO = "#F2F0EA"
PRATA = "#C8CACC"


def quadro(W, L):
    """Enquadramento: pedal em pé (proporção 1,25, a câmera mede pela altura)
    ou deitado (proporção até 1, mede pela largura)."""
    if L >= W:
        return {"largura_rel": round(max(2.0, 1.25 * L / W), 2), "proporcao": 1.25, "elevacao": 90.0}
    return {"largura_rel": 1.3, "proporcao": round(min(1.0, max(0.85, 1.2 * L / W)), 2), "elevacao": 90.0}


def estilo(diam=0.0135, alt=0.012, corpo=PRETO, traco="#F2F2EC", saia=False, tampa=None, **kw):
    e = {"diametro_m": diam, "altura_m": alt, "saia": saia, "corpo": lin(corpo), "traco": lin(traco)}
    if tampa:
        e.update({"com_tampa": True, "tampa": lin(tampa)})
    for k, v in kw.items():
        e[k] = lin(v) if isinstance(v, str) and v.startswith("#") else v
    return e


def k(id, rotulo, pos=None, **kw):
    d = {"id": id, "rotulo": rotulo}
    if pos is not None:
        d["pos"] = list(pos)
    for kk, v in kw.items():
        d[kk] = lin(v) if isinstance(v, str) and v.startswith("#") else v
    return d


def fixo(id, rotulo, pos=None, **kw):
    return k("fixo_" + id, rotulo, pos, fixo=True, **kw)


def novo(pid, nome, modelo, inspiracao, original, W, L, H, cor, letras, botoes, botao_estilo, formato="padrao", metalico=0.0,
         aspereza=0.28, textura=None, **kw):
    path = os.path.join(REC, pid + ".json")
    rec = json.load(open(path, encoding="utf-8"))
    ids_antes = sorted(b["id"] for b in rec.get("botoes", []) + rec.get("deslizantes", []) if not b.get("fixo"))
    for c in LIMPAR:
        rec.pop(c, None)
    rec.update({"nome": nome, "modelo": modelo, "inspiracao": inspiracao, "formato": formato,
                "medidas_m": {"largura": W / 1000, "comprimento": L / 1000, "altura": H / 1000},
                "medidas_fonte": original,
                "pintura": {"cor": lin(cor), "metalico": metalico, "aspereza": aspereza},
                "texto_cor": lin(letras), "botao_estilo": botao_estilo, "botoes": botoes,
                "quadro": quadro(W, L)})
    if textura:
        rec["pintura"]["textura"] = textura
    for kk, v in kw.items():
        rec[kk] = v
    if rec.get("disposicao") == "boss":
        boss_layout(rec)
    ids_depois = sorted(b["id"] for b in rec["botoes"] + rec.get("deslizantes", []) if not b.get("fixo"))
    if ids_antes != ids_depois:
        raise SystemExit(f"{pid}: botões que funcionam mudaram {ids_antes} -> {ids_depois}")
    json.dump(rec, open(path, "w", encoding="utf-8"), ensure_ascii=False, indent=2)
    print(pid, "ok")


def boss_layout(rec):
    """Mesma conta do pedal.py (_boss_layout), escrita na receita para a arte saber onde ficam os botões."""
    ks = rec["botoes"]
    n = len(ks)
    rows = {1: [(0.0, 0.66)], 2: [(-0.5, 0.66), (0.5, 0.66)], 3: [(-0.6, 0.68), (0.0, 0.56), (0.6, 0.68)],
            4: [(-0.69, 0.66), (-0.23, 0.66), (0.23, 0.66), (0.69, 0.66)]}.get(n)
    if rows:
        for kk, p in zip(ks, rows):
            kk.setdefault("pos", list(p))
        if n == 4:
            ks[3].setdefault("escala", 0.78)
            for kk in ks[:3]:
                kk.setdefault("escala", 0.92)


# ------------------------------------------------------------------ Boss compacto
BOSS = (73, 129, 59)


def boss(pid, nome, modelo, original, cor, letras, botoes, desc_botoes, tema, botao=None, painel=None, painel_metal=False,
         rotulo_cor=None, metalico=0.0, extra="", **kw):
    W, L, H = BOSS
    textos = [{"texto": "CHECK", "pos": [0.0, 0.965], "tam_m": 0.0022, "cor": lin(rotulo_cor or letras), "espaco": 1.2}]
    paineis = []
    if painel:
        paineis.append({"pos": [0.0, 0.665], "tam_m": [0.0655, 0.0405], "cor": lin(painel), "metal": painel_metal, "raio_m": 0.0015})
    paineis += kw.pop("paineis_extra", [])
    if rotulo_cor:
        for b in botoes:
            b.setdefault("rotulo_cor", lin(rotulo_cor))
    insp = (f"Boss compacto {desc_botoes[0]} ({W} x {L} x {H} mm), {desc_botoes[1]}; botões {desc_botoes[2]}, "
            f"LED CHECK no alto, no centro, e pedal basculante com borracha frisada{extra}. Tema: {tema}. "
            f"Original: {original}.")
    novo(pid, nome, modelo, insp, f"{original}: {W} x {L} x {H} mm (Boss, ficha oficial)", W, L, H, cor, letras, botoes,
         botao or estilo(saia=True), formato="compacto", metalico=metalico, aspereza=0.24,
         base_rel=0.68, basculante_rel=0.6, borracha_rel=0.36, modelo_m=0.0084, modelo_x=0.0, tipo_texto="",
         rotulo_embaixo=True, rotulo_m=0.0042, disposicao="boss", led=[0.0, 0.9], textos=textos, paineis=paineis,
         pecas=[{"tipo": "rotulo", "pos": [-0.6, 0.27], "rotulo": "SAÍDA", "rotulo_m": 0.0034},
                {"tipo": "rotulo", "pos": [0.6, 0.27], "rotulo": "ENTRADA", "rotulo_m": 0.0034}], **kw)


def lote_a():
    boss("orange_drive", "Orange Drive", "ORANGE DRIVE", "Boss DS-1 Distortion", "#F06A18", PRETO,
         [k("tom", "TOM"), k("nivel", "NÍVEL"), k("ganho", "DISTORÇÃO")],
         ("laranja", "letras pretas", "pretos com saia: TOM, NÍVEL (no meio, mais baixo) e DISTORÇÃO"),
         "fatia de laranja no basculante, raios laranja-escuro tom sobre tom")
    boss("banana_overdrive", "Banana Overdrive", "BANANA OVERDRIVE", "Boss SD-1 Super OverDrive", "#F0D832", PRETO,
         [k("nivel", "NÍVEL"), k("tom", "TOM"), k("ganho", "DRIVE")],
         ("amarelo", "letras pretas", "pretos com saia e tampa prateada: NÍVEL, TOM (no meio, mais baixo) e DRIVE"),
         "banana no basculante, listras amarelo-escuro tom sobre tom",
         botao=estilo(saia=True, tampa="#C8C8C8"), transformacao="Standard")
    boss("neptune_chorus", "Neptune Chorus", "NEPTUNE CHORUS", "Boss CE-2 Chorus", "#2AAFC4", PRETO,
         [k("a", "VELOCIDADE", (-0.5, 0.66)), k("c", "MISTURA", (0.0, 0.55), escala=0.55, rotulo_m=0.0030),
          k("b", "PROFUNDIDADE", (0.5, 0.66))],
         ("azul-turquesa claro", "letras pretas", "pretos com saia: VELOCIDADE e PROFUNDIDADE (o MISTURA a mais fica pequeno, "
          "no meio, porque o CE-2 só tem dois)"),
         "o planeta Netuno azul-turquesa com faixas, num céu de estrelas brancas")
    boss("coconut_delay", "Coconut Delay", "COCONUT DELAY", "Boss DD-3 Digital Delay", "#DCD8CC", "#1F5FAF",
         [k("c", "MISTURA"), k("b", "REPETIÇÕES"), k("a", "TEMPO"), fixo("modo", "MODO")],
         ("branco-prata", "letras azuis", "azuis com saia sobre a faixa preta do alto: MISTURA (E.LEVEL), REPETIÇÕES "
          "(F.BACK), TEMPO (D.TIME) e o seletor MODO menor, à direita (fixo)"),
         "meio coco com ondas azul-claro tom sobre tom", botao=estilo(saia=True, corpo="#2A9ADC"), painel=PRETO,
         rotulo_cor=BRANCO, extra=", faixa preta atrás dos botões")
    boss("grape_reverb", "Meteorite Reverb", "METEORITE REVERB", "Boss RV-6 Reverb", "#3A3B3F", BRANCO,
         [k("c", "MISTURA"), k("b", "TOM"), k("a", "TEMPO"), fixo("modo", "MODO")],
         ("cinza-grafite metálico", "letras brancas", "pretos com saia sobre o painel prateado escovado: MISTURA (E.LEVEL), "
          "TOM, TEMPO e o seletor MODO menor, à direita (fixo)"),
         "meteorito em queda com rastro de fogo, num céu de estrelas", painel="#BDBFC2", painel_metal=True,
         rotulo_cor="#1A1A1A", metalico=0.45, extra=", painel prateado escovado atrás dos botões")
    boss("moon_tremolo", "Turquoise Tremolo", "TURQUOISE TREMOLO", "Boss TR-2 Tremolo", "#1E8A86", PRETO,
         [k("a", "VELOCIDADE"), k("c", "FORMA"), k("b", "PROFUNDIDADE")],
         ("verde-azulado (turquesa)", "letras pretas", "pretos com saia e tampa dourada: VELOCIDADE, FORMA (no meio, mais "
          "baixo) e PROFUNDIDADE"),
         "turquesa polida com veios marrons, facetas tom sobre tom", botao=estilo(saia=True, tampa="#C8A050"))
    boss("jabuticaba_buzzsaw", "Jabuticaba Buzzsaw", "JABUTICABA BUZZSAW", "Boss HM-2 Heavy Metal", "#18181A", "#F07A28",
         [k("nivel", "NÍVEL"), k("tom", "ALTO"), fixo("baixo", "BAIXO"), k("ganho", "DISTORÇÃO")],
         ("preto", "letras laranja", "laranja com saia: NÍVEL, ALTO e BAIXO (o COLOR MIX; o BAIXO é fixo) e DISTORÇÃO"),
         "galho de jabuticabas pretas, listras tom sobre tom", botao=estilo(saia=True, corpo="#F07A28", traco="#141414"))
    boss("avocado_metal", "Onyx Metal", "ONYX METAL", "Boss MT-2 Metal Zone", "#2A2A2D", BRANCO,
         [k("nivel", "NÍVEL"), k("tom", "AGUDO", concentrico=True), fixo("medio", "MÉDIO", concentrico=True),
          k("ganho", "DISTORÇÃO")],
         ("cinza-grafite quase preto", "letras brancas e o nome em laranja-avermelhado", "pretos com saia: NÍVEL, AGUDO e "
          "MÉDIO duplos (concêntricos, o MÉDIO é fixo) e DISTORÇÃO"),
         "ônix preto polido com faixa branca, facetas tom sobre tom", painel="#111112", modelo_cor=lin("#E8502A"),
         extra=", faixa preta atrás dos botões")
    boss("blueberry_blues", "Blueberry Blues", "BLUEBERRY BLUES", "Boss BD-2 Blues Driver", "#1D5FA6", "#F2A33A",
         [k("nivel", "NÍVEL"), k("tom", "TOM"), k("ganho", "GANHO")],
         ("azul", "letras laranja-amareladas", "pretos com saia e tampa prateada: NÍVEL, TOM (no meio, mais baixo) e GANHO"),
         "mirtilos azuis, pontos tom sobre tom", botao=estilo(saia=True, tampa="#C8C8C8"))
    boss("apricot_overdrive", "Citrine Overdrive", "CITRINE OVERDRIVE", "Boss OD-3 OverDrive", "#F4C21E", PRETO,
         [k("nivel", "NÍVEL"), k("tom", "TOM"), k("ganho", "DRIVE")],
         ("amarelo-gema", "letras pretas", "pretos com saia e tampa prateada: NÍVEL, TOM (no meio, mais baixo) e DRIVE"),
         "citrino lapidado amarelo-dourado, facetas tom sobre tom", botao=estilo(saia=True, tampa="#C8C8C8"),
         transformacao="Standard")
    boss("clementine_turbo", "Clementine Turbo", "CLEMENTINE TURBO", "Boss DS-2 Turbo Distortion", "#F06A1C", PRETO,
         [k("nivel", "NÍVEL", (-0.72, 0.66), escala=0.88), k("tom", "TOM", (-0.30, 0.66), escala=0.88),
          k("ganho", "DISTORÇÃO", (0.12, 0.66), escala=0.88), fixo("turbo", "TURBO", (0.66, 0.66), escala=0.72)],
         ("laranja", "letras pretas", "laranja com saia sobre a faixa preta: NÍVEL, TOM, DISTORÇÃO e, separado à direita, "
          "o seletor TURBO menor (fixo)"),
         "clementina com folha, listras laranja-escuro tom sobre tom",
         botao=estilo(saia=True, corpo="#F07A28", traco="#141414"), painel="#111112", rotulo_cor=BRANCO,
         extra=", faixa preta atrás dos botões e um filete separando o TURBO",
         paineis_extra=[{"pos": [0.42, 0.665], "tam_m": [0.0008, 0.034], "cor": lin(BRANCO)}])
    boss("jupiter_octave", "Jupiter Octave", "JUPITER OCTAVE", "Boss OC-2 Octave", "#6A4632", BRANCO,
         [k("c", "OITAVA 2"), k("a", "DIRETO"), k("b", "OITAVA 1")],
         ("marrom metálico", "letras brancas", "pretos com saia e tampa prateada: OITAVA 2, DIRETO (no meio, mais baixo) e "
          "OITAVA 1"),
         "o planeta Júpiter com as faixas e a mancha vermelha, num céu de estrelas", botao=estilo(saia=True, tampa="#C8C8C8"),
         metalico=0.5)


def acerola():
    # GE-7: os 8 deslizantes continuam iguais (só cor e faixa preta).
    pid = "acerola_eq"
    rec = json.load(open(os.path.join(REC, pid + ".json"), encoding="utf-8"))
    sl, lay = rec["deslizantes"], rec["deslizantes_layout"]
    lay["rotulo_cor"] = lin(BRANCO)
    W, L, H = BOSS
    insp = (f"Boss compacto branco-acinzentado ({W} x {L} x {H} mm), letras pretas no corpo e brancas na faixa preta do alto, "
            "onde ficam os 8 deslizantes de pontas brancas (100 Hz a 6,4 kHz e NÍVEL); LED CHECK no alto, à direita, e pedal "
            "basculante com borracha frisada. Tema: cristais de selenita brancos em lâminas, facetas tom sobre tom. "
            "Original: Boss GE-7 Equalizer.")
    novo(pid, "Selenite EQ", "SELENITE EQ", insp, "Boss GE-7 Equalizer: 73 x 129 x 59 mm (Boss, ficha oficial)", W, L, H,
         "#D8D4C8", PRETO, [], estilo(saia=True, corpo=PRETO, cursor="#F2F2F2"), formato="compacto", aspereza=0.24,
         base_rel=0.68, basculante_rel=0.6, borracha_rel=0.36, modelo_m=0.0084, modelo_x=0.0, tipo_texto="",
         rotulo_embaixo=True, rotulo_m=0.0042, led=[0.78, 0.93], deslizantes=sl, deslizantes_layout=lay,
         paineis=[{"pos": [0.0, 0.64], "tam_m": [0.0655, 0.044], "cor": lin("#111112"), "raio_m": 0.0015}])


# ------------------------------------------------------------------ caixas retangulares (MXR, boutique)
def caixa(pid, nome, modelo, insp, fonte, dims, cor, letras, botoes, botao, logo_y=-0.86, logo_m=0.0085, pisante_y=-0.62,
          led=None, chanfro=0.004, **kw):
    W, L, H = dims
    extra = {"chanfro_m": chanfro, "logo_y": logo_y, "logo_m": logo_m, "pisante_y": pisante_y, "rotulo_embaixo": True,
             "rotulo_m": kw.pop("rotulo_m", 0.0040), "texto_cor2": lin(kw.pop("nome_cor", letras))}
    if led:
        extra["led"] = list(led)
    extra.update(kw)
    novo(pid, nome, modelo, insp, fonte, W, L, H, cor, letras, botoes, botao, **extra)


def moldura(W, L, cor_borda, cor_meio, margem=0.0035, fio=0.0012, raio=0.002):
    """Contorno pintado em volta do painel: chapa da cor da borda e outra, menor, da cor do meio."""
    w, l = W / 1000 - 2 * margem, L / 1000 - 2 * margem
    return [{"pos": [0.0, 0.0], "tam_m": [w, l], "cor": lin(cor_borda), "raio_m": raio, "fundo": True},
            {"pos": [0.0, 0.0], "tam_m": [w - 2 * fio, l - 2 * fio], "cor": lin(cor_meio), "raio_m": raio * 0.6, "fundo": True}]


MXR = (62, 111, 32)
MXR_FONTE = "caixa MXR padrão, aprox. 62 x 111 x 32 mm sem botões (Dunlop não publica; medida de lojas)"


def lote_b():
    mxr_botao = estilo(diam=0.0165, alt=0.0125)
    pequeno = dict(escala=0.5, rotulo_m=0.0026)
    caixa("pear_gate", "Marcasite Gate", "MARCASITE GATE",
          "Caixa MXR prateada, cinza-claro metálico (62 x 111 x 32 mm), letras pretas e contorno preto em volta do painel; "
          "um botão preto grande SENSIBILIDADE (TRIGGER LEVEL), botão de apertar da faixa alta à esquerda no alto, chave de "
          "3 posições de corte de ruído à direita, LED amarelo do gate e LED vermelho acima do pisante; ATAQUE e RELEASE, que o "
          "original não tem, ficam pequenos nos cantos de baixo. Tema: cristais de marcassita (cubos prateados) e facetas "
          "tom sobre tom. Original: MXR Smart Gate.", MXR_FONTE, MXR, "#BFC2C4", PRETO,
          [k("a", "SENSIBILIDADE", (0.0, 0.6), escala=1.15), k("b", "ATAQUE", (-0.72, -0.5), **pequeno),
           k("c", "RELEASE", (0.72, -0.5), **pequeno)], mxr_botao, led=[0.0, -0.36], metalico=0.6, logo_y=-0.86,
          logo_m=0.0075, pisante_y=-0.6,
          paineis=moldura(62, 111, PRETO, "#BFC2C4"),
          pecas=[{"tipo": "apertar", "pos": [-0.62, 0.88], "diam_m": 0.005, "rotulo": "ALTO", "rotulo_pos": "baixo"},
                 {"tipo": "minichave", "pos": [0.68, 0.62]}, {"tipo": "led", "pos": [0.62, 0.9], "cor": lin("#6A5200")}])
    caixa("mars_phaser", "Mars Phaser", "MARS PHASER",
          "Caixa MXR laranja vivo com pintura enrugada (62 x 111 x 32 mm), letras pretas; um botão preto grande VELOCIDADE "
          "(SPEED) no alto, LED vermelho acima do pisante; PROFUNDIDADE e INTENSIDADE, que o original não tem, ficam pequenas nos "
          "cantos de baixo. Tema: o planeta Marte com dunas, pontos tom sobre tom. Original: MXR Phase 90.", MXR_FONTE, MXR,
          "#F07418", PRETO,
          [k("a", "VELOCIDADE", (0.0, 0.6), escala=1.15), k("b", "PROFUNDIDADE", (-0.72, -0.5), **pequeno),
           k("c", "INTENSIDADE", (0.72, -0.5), **pequeno)], mxr_botao, led=[0.0, -0.36], textura="enrugado", logo_y=-0.86,
          logo_m=0.0075, pisante_y=-0.6)
    caixa("quince_plus", "Quince Plus", "QUINCE PLUS",
          "Caixa MXR amarela com pintura enrugada (62 x 111 x 32 mm), letras pretas; botões pretos SAÍDA e DISTORÇÃO no alto, "
          "LED vermelho acima do pisante. Tema: marmelo amarelo, raios tom sobre tom. Original: MXR Distortion+.", MXR_FONTE,
          MXR, "#F2D02A", PRETO, [k("nivel", "SAÍDA", (-0.45, 0.66)), k("ganho", "DISTORÇÃO", (0.45, 0.66))], mxr_botao,
          led=[0.0, -0.36], textura="enrugado", transformacao="Standard")
    caixa("apple_comp", "Apple Comp", "APPLE COMP",
          "Caixa MXR vermelha com pintura enrugada (62 x 111 x 32 mm), letras pretas; botões pretos SAÍDA e SENSIBILIDADE no "
          "alto, LED vermelho acima do pisante; o ATAQUE, que o original não tem, fica pequeno no canto de baixo. Tema: maçã "
          "vermelha, raios tom sobre tom. Original: MXR Dyna Comp.", MXR_FONTE, MXR, "#D2202A", PRETO,
          [k("a", "SAÍDA", (-0.45, 0.66)), k("b", "SENSIBILIDADE", (0.45, 0.66)), k("c", "ATAQUE", (0.72, -0.5), **pequeno)],
          mxr_botao, led=[0.0, -0.36], textura="enrugado")
    caixa("starfruit_rock", "Hematite Rock", "HEMATITE ROCK",
          "Caixa MXR preta (62 x 111 x 32 mm) com painel azul-celeste no alto; no painel, os deslizantes VOLUME e GANHO, à "
          "esquerda o botão MODO com os 4 LEDs (vermelho, amarelo, verde e azul) e à direita o botão CHORUS com LED; LED "
          "vermelho acima do pisante; letras brancas e o nome em azul-celeste. Tema: hematita (bolhas de metal cinza "
          "brilhante). Original: MXR Rockman X100.", MXR_FONTE, MXR, "#151517", BRANCO,
          [k("nivel", "VOLUME", (-0.12, 0.62), tipo="deslizante", curso_m=0.018, rotulo_cor=lin(PRETO), rotulo_pos="cima"),
           k("ganho", "GANHO", (0.12, 0.62), tipo="deslizante", curso_m=0.018, rotulo_cor=lin(PRETO), rotulo_pos="cima")],
          estilo(cursor="#E8E8E8"), led=[0.0, -0.36], nome_cor="#2AB4E8",
          paineis=[{"pos": [0.0, 0.6], "tam_m": [0.054, 0.04], "cor": lin("#2AAEE6"), "raio_m": 0.0015}],
          pecas=[{"tipo": "led", "pos": [-0.66, 0.84], "cor": lin("#6A0A06"), "raio_m": 0.0016},
                 {"tipo": "led", "pos": [-0.66, 0.76], "cor": lin("#6A5200"), "raio_m": 0.0016},
                 {"tipo": "led", "pos": [-0.66, 0.68], "cor": lin("#0A4A12"), "raio_m": 0.0016},
                 {"tipo": "led", "pos": [-0.66, 0.60], "cor": lin("#0A1A6A"), "raio_m": 0.0016},
                 {"tipo": "apertar", "pos": [-0.66, 0.46], "diam_m": 0.0045, "rotulo": "MODO", "rotulo_pos": "baixo",
                  "rotulo_cor": lin(PRETO)},
                 {"tipo": "led", "pos": [0.66, 0.62], "raio_m": 0.0018},
                 {"tipo": "apertar", "pos": [0.66, 0.46], "diam_m": 0.0045, "rotulo": "CHORUS", "rotulo_pos": "baixo",
                  "rotulo_cor": lin(PRETO)}])
    caixa("cranberry_wylde", "Agate Wylde", "AGATE WYLDE",
          "Caixa MXR prateada, cinza-azulada (62 x 111 x 32 mm), com o alvo de anéis pretos pintado no tampo e o LED vermelho "
          "no centro dele; botões transparentes SAÍDA (esq.), TOM (no alto, no meio) e GANHO (dir.); letras creme. Tema: os "
          "anéis do alvo desenhados como as faixas de uma ágata cinza. Original: MXR Wylde Overdrive.", MXR_FONTE, MXR,
          "#AEB4BA", "#F2ECD8",
          [k("nivel", "SAÍDA", (-0.6, 0.5)), k("tom", "TOM", (0.0, 0.78)), k("ganho", "GANHO", (0.6, 0.5))],
          estilo(diam=0.014, alt=0.012, corpo="#D8DCE0", traco="#202020", material="transparente"), led=[0.0, 0.18],
          metalico=0.5, nome_cor=PRETO)
    caixa("guava_precision", "Spinel Precision", "SPINEL PRECISION",
          "Caixa estreita preta com brilho (purpurina cinza-chumbo), 60 x 112 x 32 mm, letras azuis; botões pretos VOLUME e "
          "DRIVE grandes no alto, AGUDO (BRIGHT, fixo) e ATAQUE menores embaixo deles, o ajuste pequeno do GATE no meio com o LED "
          "azul do gate e o LED azul de ligado acima do pisante. Tema: espinélio negro lapidado, facetas tom sobre tom. "
          "Original: Horizon Devices Precision Drive.", "aprox. 60 x 112 mm, caixa 1590B (o fabricante não publica)",
          (60, 112, 32), "#24272C", "#3A8EE8",
          [k("nivel", "VOLUME", (-0.5, 0.74)), k("ganho", "DRIVE", (0.5, 0.74)), fixo("brilho", "AGUDO", (-0.62, 0.42), escala=0.62),
           k("tom", "ATAQUE", (0.62, 0.42), escala=0.62)],
          estilo(diam=0.015, alt=0.012), led=[0.0, -0.36], led_cor=lin("#0A1E6A"), textura="purpurina", metalico=0.4,
          pecas=[{"tipo": "trimpot", "pos": [0.0, 0.5], "rotulo": "GATE"}, {"tipo": "led", "pos": [0.0, 0.3], "cor": lin("#0A1E6A")}])
    caixa("honeydew_drive", "Honeydew Drive", "HONEYDEW DRIVE",
          "Caixa estreita verde-água claro (o lote Surf Green), 60 x 112 x 32 mm, letras azul-marinho; botões pretos VOLUME e "
          "GANHO no alto, GRAVE (fixo) e AGUDO embaixo, minichave de clipagem no meio, LED acima do pisante. Tema: melão "
          "honeydew verde-claro, listras tom sobre tom. Original: Paul Cochrane Timmy.",
          "aprox. 60 x 112 mm, caixa 1590B (sem página oficial)", (60, 112, 32), "#8CCDB0", "#1A2A5A",
          [k("nivel", "VOLUME", (-0.52, 0.74)), k("ganho", "GANHO", (0.52, 0.74)), fixo("grave", "GRAVE", (-0.52, 0.36), escala=0.8),
           k("tom", "AGUDO", (0.52, 0.36), escala=0.8)],
          estilo(diam=0.015, alt=0.012), led=[0.0, -0.36], pecas=[{"tipo": "minichave", "pos": [0.0, 0.56]}])
    caixa("peach_glory", "Amber Glory", "AMBER GLORY",
          "Caixa dourada envelhecida, latão escovado (66 x 122 x 41 mm), letras pretas; botões brancos VOLUME e DRIVE no alto e "
          "TOM embaixo à direita, minichave GANHO à esquerda, LED azul ao lado do pisante. Tema: âmbar dourado com uma folha "
          "presa, pontos tom sobre tom. Original: JHS Morning Glory.", "66 x 122 x 41 mm (JHS, ficha oficial)", (66, 122, 41),
          "#B49A54", PRETO,
          [k("nivel", "VOLUME", (-0.5, 0.74)), k("ganho", "DRIVE", (0.5, 0.74)), k("tom", "TOM", (0.5, 0.4))],
          estilo(diam=0.016, alt=0.013, corpo="#F2F0EA", traco="#141414"), led=[0.62, -0.74], led_cor=lin("#0A1E6A"),
          metalico=0.55, aspereza=0.4, pisante_y=-0.74, logo_y=-0.42,
          pecas=[{"tipo": "minichave", "pos": [-0.5, 0.42], "rotulo": "GANHO", "rotulo_pos": "baixo"}])
    caixa("raspberry_drive", "Raspberry Drive", "RASPBERRY DRIVE",
          "Caixa estreita vermelha (56 x 122 x 41 mm), letras pretas; botões pretos VOLUME e DRIVE no alto com a minichave de 3 "
          "posições entre eles, TOM (EQ), AR (fixo) e REFORÇO (fixo) menores na segunda fileira, dois pisantes (drive e "
          "reforço) com um LED vermelho cada. Tema: framboesa no lugar do @ do meio, framboesas tom sobre tom. Original: JHS "
          "Andy Timmons (AT).", "56 x 122 x 41 mm (JHS, ficha oficial)", (56, 122, 41), "#D21E26", PRETO,
          [k("nivel", "VOLUME", (-0.56, 0.76), escala=0.9), k("ganho", "DRIVE", (0.56, 0.76), escala=0.9),
           k("tom", "TOM", (-0.62, 0.42), escala=0.72), fixo("ar", "AR", (0.0, 0.42), escala=0.72),
           fixo("reforco", "REFORÇO", (0.62, 0.42), escala=0.72)],
          estilo(diam=0.015, alt=0.012), led=[-0.5, -0.55], logo_y=-0.98, logo_m=0.0001, pisante={"pos": [-0.5, -0.76]},
          modelo_oculto=True,
          pecas=[{"tipo": "minichave", "pos": [0.0, 0.8]}, {"tipo": "pisante", "pos": [0.5, -0.76]},
                 {"tipo": "led", "pos": [0.5, -0.55]}])
    caixa("gooseberry_g3", "Charoite G3", "CHAROITE G3",
          "Caixa roxa (70 x 119 x 32 mm), letras brancas, entradas no alto; GRAVE e MÉDIO (fixos) e AGUDO na fileira de cima, "
          "VOLUME e GANHO embaixo com a minichave AGRESSÃO entre eles, LED acima do pisante. Tema: charoíta roxa com redemoinhos "
          "lilás, facetas tom sobre tom. Original: Revv G3.", "70 x 119 x 32 mm (Revv, ficha oficial)", (70, 119, 32),
          "#6A2CB0", BRANCO,
          [fixo("grave", "GRAVE", (-0.58, 0.76), escala=0.8), fixo("medio", "MÉDIO", (0.0, 0.76), escala=0.8),
           k("tom", "AGUDO", (0.58, 0.76), escala=0.8), k("nivel", "VOLUME", (-0.58, 0.4), escala=0.8),
           k("ganho", "GANHO", (0.58, 0.4), escala=0.8)],
          estilo(diam=0.014, alt=0.012, tampa="#C8C8CC"), led=[0.0, -0.36], jacks_topo=True, metalico=0.3,
          pecas=[{"tipo": "minichave", "pos": [0.0, 0.42], "rotulo": "AGRESSÃO", "rotulo_pos": "baixo"}])
    caixa("nectarine_od", "Jamun OD", "JAMUN OD",
          "Caixa estreita preta (64 x 112 x 38 mm) com filete creme na borda e a faixa creme do nome, letras creme; botões "
          "pretos em duas fileiras: GRAVE (fixo), AGUDO e PRESENÇA (fixo) em cima, VOLUME, GANHO e TIGHT (fixo) embaixo; LED "
          "acima do pisante. Tema: jamelão (frutinhas roxo-escuras) e pontos tom sobre tom. Original: Friedman BE-OD.",
          "aprox. 64 x 112 mm (o fabricante não publica)", (64, 112, 38), "#141414", "#E8D9A8",
          [fixo("grave", "GRAVE", (-0.6, 0.8), escala=0.85), k("tom", "AGUDO", (0.0, 0.8), escala=0.85),
           fixo("presenca", "PRESENÇA", (0.6, 0.8), escala=0.85), k("nivel", "VOLUME", (-0.6, 0.44), escala=0.85),
           k("ganho", "GANHO", (0.0, 0.44), escala=0.85), fixo("tight", "TIGHT", (0.6, 0.44), escala=0.85)],
          estilo(diam=0.015, alt=0.013), led=[0.0, -0.38], nome_cor=PRETO, logo_y=0.02, logo_m=0.0095,
          paineis=moldura(64, 112, "#E8D9A8", "#141414", fio=0.0010) +
          [{"pos": [0.0, 0.02], "tam_m": [0.05, 0.013], "cor": lin("#E8D9A8"), "raio_m": 0.001}])
    caixa("mangosteen_fuzz", "Moonstone Fuzz", "MOONSTONE FUZZ",
          "Caixa de alumínio prateado escovado (64 x 121 x 38 mm), letras e desenho roxos, entradas no alto; botões pretos "
          "VOLUME (esq.), SUSTAIN (no alto, no meio) e TOM (dir.), LED acima do pisante. Tema: pedra-da-lua branca com brilho "
          "azul, desenhada em roxo. Original: EarthQuaker Devices Hizumitas.", "64 x 121 x 57 mm com botões (EarthQuaker, ficha "
          "oficial)", (64, 121, 38), "#C8CBD0", "#6A3EB0",
          [k("nivel", "VOLUME", (-0.6, 0.52)), k("ganho", "SUSTAIN", (0.0, 0.76)), k("tom", "TOM", (0.6, 0.52))],
          estilo(diam=0.015, alt=0.013), led=[0.0, -0.4], metalico=0.85, aspereza=0.35, jacks_topo=True, logo_y=-0.9)
    caixa("grapefruit_drive", "Peridot Drive", "PERIDOT DRIVE",
          "Caixa verde-oliva (64 x 121 x 38 mm) com painel amarelo-mostarda no alto, entradas no alto; no painel, botões pretos "
          "NÍVEL, TOM (no meio, mais baixo) e GANHO e a minichave MODO 1|2|3 no meio, letras verdes; LED acima do pisante. "
          "Tema: peridoto lapidado verde-oliva, círculos tom sobre tom. Original: EarthQuaker Devices Plumes.",
          "64 x 121 x 57 mm com botões (EarthQuaker, ficha oficial)", (64, 121, 38), "#5C7828", "#D8B42E",
          [k("nivel", "NÍVEL", (-0.62, 0.62), rotulo_cor=lin("#3E5418")), k("tom", "TOM", (0.0, 0.42), rotulo_cor=lin("#3E5418")),
           k("ganho", "GANHO", (0.62, 0.62), rotulo_cor=lin("#3E5418"))],
          estilo(diam=0.015, alt=0.013), led=[-0.5, -0.66], jacks_topo=True, logo_y=-0.9,
          paineis=[{"pos": [0.0, 0.6], "tam_m": [0.058, 0.046], "cor": lin("#D8B42E"), "raio_m": 0.002}],
          pecas=[{"tipo": "minichave", "pos": [0.0, 0.76], "rotulo": "MODO", "rotulo_cor": lin("#3E5418")}])
    caixa("pitanga_250", "Topaz 250", "TOPAZ 250",
          "Caixa amarela (67 x 119 x 40 mm), letras pretas; botões pretos GANHO e NÍVEL no alto, LED azul entre eles, listras "
          "pretas verticais embaixo, atrás do pisante. Tema: topázio amarelo lapidado, facetas tom sobre tom. Original: DOD "
          "Overdrive Preamp 250.", "67 x 119 x 57 mm com botões (DOD, ficha oficial)", (67, 119, 40), "#F2D01C", PRETO,
          [k("ganho", "GANHO", (-0.5, 0.7), rotulo_pos="cima"), k("nivel", "NÍVEL", (0.5, 0.7), rotulo_pos="cima")],
          estilo(diam=0.018, alt=0.014, tampa="#C8C8C8"), led=[0.0, 0.7], led_cor=lin("#0A1E6A"), transformacao="Standard",
          logo_y=0.05, logo_m=0.011, pisante_y=-0.66,
          paineis=[{"pos": [x, -0.66], "tam_m": [0.0022, 0.03], "cor": lin(PRETO)} for x in
                   (-0.86, -0.74, -0.62, -0.5, -0.38, 0.38, 0.5, 0.62, 0.74, 0.86)])


def lote_c():
    ts = estilo(diam=0.019, alt=0.014, tampa=None)
    trio = lambda r=True: [k("ganho", "DRIVE", (-0.5, 0.66)), k("tom", "TOM", (0.0, 0.44), escala=0.72), k("nivel", "NÍVEL", (0.5, 0.66))]
    novo("lemon_drive", "Greengage Drive", "GREENGAGE DRIVE",
         "Caixa Tube Screamer verde-claro amarelado (74 x 124 x 53 mm), rótulos cinza-claro; botões pretos DRIVE, TOM "
         "(pequeno, no meio, mais baixo) e NÍVEL, LED vermelho no alto, no meio; placa prateada do nome com letras pretas e, "
         "na frente, moldura preta com a placa de metal frisada de pisar. Tema: ameixa-rainha-cláudia verde-amarelada, "
         "fatias tom sobre tom. Original: Ibanez TS9 Tube Screamer.", "Ibanez TS9: aprox. 74 x 124 x 53 mm (sem ficha oficial)",
         74, 124, 53, "#7EC04A", "#E2E6DC", trio(), ts, formato="classico", metalico=0.35, aspereza=0.3,
         chanfro_m=0.0065, texto_cor2=lin("#141414"), tipo_texto="OVERDRIVE", etiqueta_y=0.06, etiqueta_estilo="placa_prata",
         pisante={"estilo": "placa_moldura"}, led=[0.0, 0.8], rotulo_embaixo=True)
    novo("olive_drive", "Olive Drive", "OLIVE DRIVE",
         "Caixa Tube Screamer verde-médio azulado (74 x 124 x 53 mm), letras brancas e filete branco em volta do painel; "
         "botões pretos DRIVE (OVERDRIVE), TOM (pequeno, no meio, mais baixo) e NÍVEL, LED vermelho no alto entre o TOM e o "
         "NÍVEL; nome em branco, sem moldura, e a placa de metal pequena de pisar direto na caixa. Tema: ramo de azeitonas, "
         "azeitonas tom sobre tom. Original: Ibanez TS808 Tube Screamer.", "Ibanez TS808: aprox. 74 x 124 x 53 mm",
         74, 124, 53, "#1E9A5C", BRANCO, trio(), ts, formato="classico", metalico=0.2, aspereza=0.3, chanfro_m=0.0065,
         texto_cor2=lin(BRANCO), tipo_texto="OVERDRIVE PRO", etiqueta_y=0.08, etiqueta_estilo="nenhuma",
         pisante={"estilo": "placa_pequena"}, led=[0.3, 0.8], rotulo_embaixo=True,
         paineis=[{"pos": [0.0, 0.0], "tam_m": [0.0645, 0.1145], "cor": lin(BRANCO), "raio_m": 0.002, "fundo": True},
                  {"pos": [0.0, 0.0], "tam_m": [0.0629, 0.1129], "cor": lin("#1E9A5C"), "raio_m": 0.0015, "fundo": True}])
    caixa("pistachio_drive", "Pistachio Drive", "PISTACHIO",
          "Caixa retangular verde (70 x 113 x 38 mm, 56 com os botões), letras brancas; botões pretos DRIVE e NÍVEL (BALANCE) "
          "no alto e TOM pequeno no meio, mais baixo; LED vermelho no alto, no meio; pisante redondo grande de metal e o nome "
          "grande embaixo. Tema: pistaches abertos, pontos tom sobre tom. Original: Maxon OD808.",
          "70 x 113 x 56 mm com botões (Maxon, ficha oficial)", (70, 113, 38), "#1E9A3E", BRANCO,
          [k("ganho", "DRIVE", (-0.48, 0.72)), k("tom", "TOM", (0.0, 0.5), escala=0.6), k("nivel", "NÍVEL", (0.48, 0.72))],
          estilo(diam=0.019, alt=0.014), led=[0.0, 0.88], pisante={"estilo": "redondo", "pos": [0.0, -0.36]},
          logo_y=-0.8, logo_m=0.013, tipo_texto="", rotulo_m=0.0042)
    caixa("gold_centaur", "Gold Centaur", "GOLD CENTAUR",
          "Caixa deitada dourada fosca, cor de bronze (120 x 95 x 40 mm), letras pretas; botões grandes cor de sangue de boi "
          "GANHO, AGUDO e SAÍDA em fila, LED vermelho à esquerda do GANHO, pisante pequeno no canto de baixo à direita, GOLD "
          "embaixo à esquerda e CENTAUR à direita. Tema: coroa de louros dourada. Original: Klon Centaur.",
          "aprox. 120 x 95 x 40 mm (medida de loja; a Klon não publica)", (120, 95, 40), "#C49A48", PRETO,
          [k("ganho", "GANHO", (-0.55, 0.5)), k("tom", "AGUDO", (0.0, 0.5)), k("nivel", "SAÍDA", (0.55, 0.5))],
          estilo(diam=0.021, alt=0.016, corpo="#4A1612", traco="#E8D8C0"), led=[-0.84, 0.5], metalico=0.55, aspereza=0.5,
          pisante={"estilo": "pequeno", "pos": [0.5, -0.36]}, modelo_oculto=True, rotulo_m=0.0045,
          textos=[{"texto": "GOLD", "pos": [-0.6, -0.78], "tam_m": 0.0062, "cor": lin(PRETO), "espaco": 1.6},
                  {"texto": "CENTAUR", "pos": [0.6, -0.78], "tam_m": 0.0062, "cor": lin(PRETO), "espaco": 1.4}])
    caixa("cherry_drop", "Cherry Drop", "CHERRY DROP",
          "Caixa vermelha (73 x 120 x 44 mm) com faixa diagonal prateada, letras brancas e pretas; um botão preto DROP no alto à "
          "esquerda, chave de alavanca à direita, fileira de 9 LEDs da afinação no meio e LED vermelho ao lado do pisante, sem "
          "tela; a MISTURA, que o original não tem, fica pequena no canto. Tema: cerejas, pontos tom sobre tom. Original: "
          "DigiTech Drop.", "73 x 120 x 44 mm (DigiTech, ficha oficial)", (73, 120, 44), "#D42428", BRANCO,
          [k("a", "DROP", (-0.5, 0.74)), k("b", "MISTURA", (-0.78, -0.86), escala=0.5, rotulo_m=0.0026)],
          estilo(diam=0.019, alt=0.014), led=[0.5, -0.62], pisante={"pos": [0.12, -0.62]}, logo_y=-0.25, logo_m=0.012,
          nome_cor="#F2F2F2",
          paineis=[{"pontos": [[-1, 0.1], [-1, 0.36], [1, -0.06], [1, -0.32]], "cor": lin("#C8CACC"), "metal": True}],
          pecas=[{"tipo": "minichave", "pos": [0.55, 0.74], "rotulo": "MOMENTÂNEO", "rotulo_pos": "baixo"},
                 {"tipo": "leds", "pos": [0.0, 0.38], "n": 9, "passo_m": 0.0068, "raio_m": 0.0019}])
    novo("acai_rat", "Açaí Rat", "AÇAÍ RAT",
         "Caixa preta em cunha, mais alta atrás (79 x 103 mm, 55 mm atrás e 30 na frente), letras brancas; botões pretos "
         "DISTORÇÃO, FILTRO e VOLUME em fila no alto, com os nomes em caixinhas, o nome grande no meio, LED vermelho e pisante. "
         "Tema: cacho de açaí roxo-escuro, listras tom sobre tom. Original: ProCo RAT.",
         "aprox. 79 x 103 x 73 mm com botões (medida de loja)", 79, 103, 55, "#161616", BRANCO,
         [k("ganho", "DISTORÇÃO", (-0.6, 0.6)), k("tom", "FILTRO", (0.0, 0.6)), k("nivel", "VOLUME", (0.6, 0.6))],
         estilo(diam=0.017, alt=0.014), cunha={"frente_m": 0.030}, chanfro_m=0.004, rotulo_m=0.0034,
         rotulo_embaixo=False, led=[0.0, -0.36], pisante_y=-0.64, logo_y=0.05, logo_m=0.017, texto_cor2=lin(BRANCO),
         paineis=[{"pos": [x, 0.88], "tam_m": [0.023, 0.0075], "cor": lin(BRANCO), "raio_m": 0.0004} for x in (-0.6, 0.0, 0.6)] +
                 [{"pos": [x, 0.88], "tam_m": [0.0216, 0.0061], "cor": lin("#161616"), "raio_m": 0.0003} for x in (-0.6, 0.0, 0.6)] +
                 [{"pos": [0.0, 0.05], "tam_m": [0.06, 0.022], "cor": lin(BRANCO), "raio_m": 0.0006},
                  {"pos": [0.0, 0.05], "tam_m": [0.0584, 0.0204], "cor": lin("#161616"), "raio_m": 0.0005}])
    caixa("snowpear_treble", "Snow Pear Treble", "SNOW PEAR TREBLE",
          "Caixa deitada de chapa cinza, em cunha (110 x 65 mm, 60 mm atrás e 35 na frente), feita para ficar em cima do "
          "amplificador; um botão preto REFORÇO à direita e a chave de alavanca liga/desliga à esquerda (sem pisante, sem LED); o "
          "VOLUME, que o original não tem, fica pequeno no canto. Tema: pera-da-neve cinza-esbranquiçada, pontos tom sobre tom. "
          "Original: Dallas Rangemaster Treble Booster.", "sem ficha do fabricante; medida estimada pelas fotos",
          (110, 65, 60), "#B4B6B8", PRETO,
          [k("ganho", "REFORÇO", (0.55, 0.3), escala=1.1), k("nivel", "VOLUME", (0.86, -0.62), escala=0.5, rotulo_m=0.0026)],
          estilo(diam=0.019, alt=0.015), cunha={"frente_m": 0.035}, metalico=0.25, aspereza=0.4,
          pisante={"estilo": "nenhum", "pos": [-0.6, 0.3], "raio_m": 0.007}, logo_y=-0.55, logo_m=0.008,
          pecas=[{"tipo": "minichave", "pos": [-0.6, 0.3], "rotulo": "LIGA"}])
    caixa("cherry_fuzz", "Cherry Fuzz", "Cherry Fuzz",
          "Caixa redonda vermelha com pintura martelada (150 mm de diâmetro, 55 de altura), letras brancas; botões pretos "
          "VOLUME e FUZZ na borda de trás, o nome em letra cursiva no meio e, na frente, a placa preta triangular com frisos e o "
          "pisante pequeno no meio dela; sem LED. Tema: par de cerejas vermelhas. Original: Dunlop Fuzz Face.",
          "aprox. 150 mm de diâmetro x 55 mm (estimativa; a Dunlop não publica)", (150, 150, 55), "#C81E28", BRANCO,
          [k("nivel", "VOLUME", (-0.42, 0.56)), k("ganho", "FUZZ", (0.42, 0.56))], estilo(diam=0.019, alt=0.014),
          formato="redondo", textura="martelado", chanfro=0.012, logo_script=True, logo_y=0.2, logo_m=0.026,
          pisante={"estilo": "pequeno", "pos": [0.0, -0.5]},
          pecas=[{"tipo": "placa", "forma": "triangulo", "pos": [0.0, -0.46], "tam_m": [0.09, 0.06], "cor": lin("#0E0E0E"),
                  "frisos": True}])
    caixa("fig_bender", "Mercury Bender", "MERCURY BENDER",
          "Caixa fundida cinza martelada em cunha, mais alta atrás (95 x 150 mm, 60 mm atrás e 30 na frente); placa preta no "
          "alto com os botões pretos ATAQUE e NÍVEL e o nome em letras brancas, pisante pequeno na frente; sem LED; o TOM, que o "
          "original não tem, fica pequeno ao lado do pisante. Tema: o planeta Mercúrio cinza com crateras. Original: Sola Sound "
          "Tone Bender (MK I).", "sem ficha do fabricante; medida estimada pelas fotos", (95, 150, 60), "#8C8E90", BRANCO,
          [k("ganho", "ATAQUE", (-0.38, 0.62)), k("nivel", "NÍVEL", (0.38, 0.62)),
           k("tom", "TOM", (0.62, -0.72), escala=0.5, rotulo_m=0.0026, rotulo_cor=lin(PRETO))],
          estilo(diam=0.017, alt=0.014), cunha={"frente_m": 0.030}, textura="martelado", metalico=0.4,
          pisante={"estilo": "pequeno", "pos": [0.0, -0.72]}, logo_y=0.38, logo_m=0.0075,
          paineis=[{"pos": [0.0, 0.58], "tam_m": [0.07, 0.05], "cor": lin("#121212"), "raio_m": 0.002}])
    caixa("kiwi_drive", "Opal Drive", "OPAL DRIVE",
          "Caixa creme, branco-marfim (64 x 112 x 38 mm), letras pretas; botões creme \"cabeça de galinha\" VOLUME, DRIVE e TOM "
          "em fila, minichave HP/LP no meio, LED transparente e pisante; o nome grande no meio. Tema: opala branca com reflexos "
          "coloridos. Original: Fulltone OCD.", "aprox. 64 x 112 mm (fulltone.com fora do ar)", (64, 112, 38), "#EEE6D0", PRETO,
          [k("nivel", "VOLUME", (-0.62, 0.62)), k("ganho", "DRIVE", (0.0, 0.62)), k("tom", "TOM", (0.62, 0.62))],
          estilo(diam=0.016, alt=0.014, corpo="#D2C4A4", traco="#141414", forma="galinha"), led=[0.0, -0.36],
          led_cor=lin("#8A8A88"), logo_y=-0.06, logo_m=0.012,
          pecas=[{"tipo": "minichave", "pos": [0.0, 0.32], "rotulo": "AGUDO"}])
    novo("comet_wah", "Comet Wah", "COMET WAH",
         "Pedal de wah preto texturizado (102 x 254 x 64 mm): base fundida e pedal basculante comprido com borracha frisada, o "
         "nome na ponta; chave liga/desliga embaixo da ponta, sem LED e sem botões no tampo; os três ajustes (PEDAL, FAIXA, "
         "NÍVEL), que o original não tem, ficam pequenos na faixa lisa atrás do basculante. Tema: cometa branco-azulado na "
         "ponta do pedal. Original: Dunlop Cry Baby.", "aprox. 102 x 254 x 64 mm (zZounds; a Dunlop não publica)",
         102, 254, 64, "#141414", BRANCO,
         [k("a", "PEDAL", (-0.5, 0.915), escala=0.5, rotulo_pos="nenhum"), k("b", "FAIXA", (0.0, 0.915), escala=0.5,
          rotulo_pos="nenhum"), k("c", "NÍVEL", (0.5, 0.915), escala=0.5, rotulo_pos="nenhum")],
         estilo(), formato="wah", textura="enrugado", base_m=0.032, talao_m=0.022, borracha_rel=0.66, inclinacao=5.0,
         logo_m=0.012, texto_cor2=lin(BRANCO))
    caixa("pomegranate_mud", "Heliodor Mud", "HELIODOR MUD",
          "Caixa amarelo-gema (100 x 120 x 40 mm), letras pretas; botões pretos GANHO, TOM (no meio, mais baixo) e NÍVEL, botão "
          "de apertar REFORÇO pequeno entre o GANHO e o NÍVEL, LED pequeno no alto e pisante embaixo à direita. Tema: cristal de "
          "heliodoro amarelo (berilo), pontos tom sobre tom. Original: T-Rex Mudhoney.", "100 x 120 x 55 mm com botões (T-Rex, ficha "
          "oficial)", (100, 120, 40), "#F2B41E", PRETO,
          [k("ganho", "GANHO", (-0.58, 0.62)), k("tom", "TOM", (0.0, 0.42)), k("nivel", "NÍVEL", (0.58, 0.62))],
          estilo(diam=0.019, alt=0.015), led=[0.0, 0.9], pisante={"pos": [0.5, -0.56]}, logo_y=-0.84, logo_m=0.009,
          transformacao="Standard",
          pecas=[{"tipo": "apertar", "pos": [0.0, 0.7], "diam_m": 0.006, "rotulo": "REFORÇO", "rotulo_pos": "cima"}])
    novo("lychee_fuzz", "Pearl Fuzz", "PEARL FUZZ",
         "Caixa grande (140 x 175 x 50 mm) de laterais pretas, tampo de aço escovado prateado na parte de cima e preto na de "
         "baixo; botões pretos VOLUME, TOM e SUSTAIN numa moldura preta, LED vermelho entre o VOLUME e o TOM, o nome grande em "
         "vermelho e o pisante na parte preta; letras pretas. Tema: pérola branca. Original: Electro-Harmonix Big Muff Pi.",
         "140 x 175 x 75 mm com botões (EHX: 5,5 x 6,88 x 2,95 pol.)", 140, 175, 50, "#C2C4C6", PRETO,
         [k("nivel", "VOLUME", (-0.55, 0.66)), k("tom", "TOM", (0.0, 0.66)), k("ganho", "SUSTAIN", (0.55, 0.66))],
         estilo(diam=0.022, alt=0.016), metalico=0.85, aspereza=0.32, pintura_lados={"cor": lin("#141414")},
         chanfro_m=0.004, rotulo_embaixo=False, rotulo_m=0.0055, led=[-0.28, 0.66], pisante_y=-0.62, logo_y=0.12,
         logo_m=0.022, texto_cor2=lin("#C81E22"), tipo_texto="",
         paineis=[{"pos": [0.0, -0.58], "tam_m": [0.132, 0.068], "cor": lin("#121212"), "raio_m": 0.001},
                  {"pos": [0.0, 0.64], "tam_m": [0.13, 0.05], "cor": lin("#121212"), "raio_m": 0.002},
                  {"pos": [0.0, 0.64], "tam_m": [0.127, 0.047], "cor": lin("#C2C4C6"), "metal": True, "raio_m": 0.0018}])
    novo("nebula_flanger", "Nebula Flanger", "NEBULA FLANGER",
         "Caixa grande de alumínio prateado escovado (110 x 140 x 57 mm), no feitio clássico, letras pretas e o nome grande em "
         "letra cursiva; botões pretos COR, FAIXA e VELOCIDADE em fila no alto, minichave FILTER MATRIX no meio, LED vermelho e "
         "pisante. Tema: nebulosa prateada com estrelas novas. Original: Electro-Harmonix Electric Mistress (clássico).",
         "caixa clássica grande, aprox. 110 x 140 x 57 mm (estimativa pela caixa EHX antiga)", 110, 140, 57, "#C6C8CA", PRETO,
         [k("c", "COR", (-0.58, 0.7)), k("b", "FAIXA", (0.0, 0.7)), k("a", "VELOCIDADE", (0.58, 0.7))],
         estilo(diam=0.019, alt=0.015), metalico=0.9, aspereza=0.3, chanfro_m=0.004, rotulo_m=0.0048, rotulo_embaixo=True,
         led=[0.42, -0.5], pisante_y=-0.62, logo_script=True, logo_y=0.02, logo_m=0.024, texto_cor2=lin(PRETO), tipo_texto="",
         pecas=[{"tipo": "minichave", "pos": [0.0, 0.32], "rotulo": "FILTRO"}])
    caixa("elderberry_war", "Howlite War", "HOWLITE WAR",
          "Caixa de alumínio branco-prateado (120 x 149 x 45 mm) com grafismos pretos de spray, letras pretas; botões pretos "
          "grandes FUZZ (esq.) e TOM (dir.) com o VOLUME pequeno entre eles, LED vermelho e pisante. Tema: howlita branca com "
          "veios cinza, raios pretos tom sobre tom. Original: Death By Audio Fuzz War.",
          "149 x 120 x 64 mm com peças (DBA, ficha oficial)", (120, 149, 45), "#ECECE8", PRETO,
          [k("ganho", "FUZZ", (-0.45, 0.5), escala=1.25), k("nivel", "VOLUME", (0.0, 0.76), escala=0.7),
           k("tom", "TOM", (0.45, 0.5), escala=1.25)], estilo(diam=0.022, alt=0.016), led=[0.3, -0.62],
          pisante_y=-0.62, logo_y=-0.08, logo_m=0.016, metalico=0.3)
    caixa("tamarind_amp", "Basalt Amp", "BASALT AMP",
          "Caixa preta com pintura salpicada (95 x 121 x 51 mm), letras amarelas; botões pretos com traço amarelo NÍVEL, ALTO, "
          "BAIXO (fixo) e DRIVE em fila no alto, três chaves deslizantes de 3 posições (MIC, MOD, AMP) no meio, LED vermelho à "
          "direita e pisante. Tema: colunas de basalto cinza-escuro vistas de cima. Original: Tech 21 SansAmp GT2.",
          "95 x 121 x 51 mm (Tech 21, ficha oficial)", (95, 121, 51), "#1C1C1C", "#F2C81E",
          [k("nivel", "NÍVEL", (-0.69, 0.74)), k("tom", "ALTO", (-0.23, 0.74)), fixo("baixo", "BAIXO", (0.23, 0.74)),
           k("ganho", "DRIVE", (0.69, 0.74))], estilo(diam=0.016, alt=0.013, traco="#F2C81E"), led=[0.72, 0.3],
          textura="martelado", rotulo_embaixo=False, logo_y=-0.84, logo_m=0.011, nome_cor="#F2C81E",
          pecas=[{"tipo": "chave_deslizante", "pos": [x, 0.3], "rotulo": r, "rotulo_pos": "cima"} for x, r in
                 ((-0.55, "MIC"), (-0.15, "MOD"), (0.25, "AMP"))])


def lote_d():
    preto = estilo(diam=0.015, alt=0.013)
    caixa("cashew_boost", "Magnetite Boost", "MAGNETITE",
          "Caixa mini (38 x 89 x 38 mm) de tampo preto e laterais cromadas, letras prateadas; um botão transparente grande "
          "REFORÇO no alto, LED azul e pisante; o VOLUME, que o original não tem, fica pequeno à direita do LED. Tema: cristal "
          "de magnetita (octaedro cinza-chumbo). Original: Xotic EP Booster.", "38 x 89 x 38 mm (Xotic, ficha oficial)",
          (38, 89, 38), "#141414", "#C8CACC",
          [k("ganho", "REFORÇO", (0.0, 0.58), escala=1.1), k("nivel", "VOLUME", (0.62, 0.2), escala=0.4, rotulo_pos="nenhum")],
          estilo(diam=0.018, alt=0.014, corpo="#D8DCE0", traco="#202020", material="transparente"), led=[0.0, 0.18],
          led_cor=lin("#0A1E6A"), pintura_lados={"cor": lin("#E0E2E4"), "cromo": True}, tampo_margem_m=0.0028,
          pisante_y=-0.6, logo_y=-0.08, logo_m=0.0058, rotulo_m=0.0034)
    caixa("kumquat_klone", "Pyrite Klone", "PYRITE KLONE",
          "Caixa mini dourada (38 x 89 x 38 mm), letras pretas; botões pretos pequenos VOLUME e GANHO no alto e AGUDO embaixo, "
          "no meio, LED azul e pisante. Tema: cubos de pirita dourada. Original: Wampler Tumnus.",
          "38 x 89 x 38 mm (Wampler, ficha oficial)", (38, 89, 38), "#C8A050", PRETO,
          [k("nivel", "VOLUME", (-0.48, 0.8)), k("ganho", "GANHO", (0.48, 0.8)), k("tom", "AGUDO", (0.0, 0.52))],
          estilo(diam=0.011, alt=0.011), led=[0.0, 0.2], led_cor=lin("#0A1E6A"), metalico=0.6, aspereza=0.35,
          pisante_y=-0.62, logo_y=-0.1, logo_m=0.0052, rotulo_m=0.003, jacks_topo=False)
    fortin = dict(aspereza=0.75, metalico=0.0, pisante_y=-0.66, rotulo_m=0.0036)
    caixa("papaya_grind", "Tourmaline Grind", "TOURMALINE GRIND",
          "Caixa estreita preta fosca (60 x 112 x 27 mm) com grafismos em preto brilhante, um botão preto NÍVEL de ponteiro "
          "branco no alto, LED no meio e pisante; GANHO e AGUDO, que o original não tem, ficam pequenos nos cantos de baixo. "
          "Tema: cristais de turmalina negra em preto brilhante. Original: Fortin Grind.", "112 x 60 x 27 mm (Fortin, ficha "
          "oficial)", (60, 112, 27), "#121212", "#2C2C2E",
          [k("nivel", "NÍVEL", (0.0, 0.66), escala=1.2), k("ganho", "GANHO", (-0.7, -0.5), escala=0.5, rotulo_m=0.0026),
           k("tom", "AGUDO", (0.7, -0.5), escala=0.5, rotulo_m=0.0026)], preto, led=[0.0, 0.22],
          logo_y=-0.9, logo_m=0.006, nome_cor="#3A3A3C", **fortin)
    caixa("persimmon_33", "Sapote 33", "SAPOTE",
          "Caixa estreita preta fosca (60 x 112 x 27 mm) com o 33 grande em preto brilhante, um botão preto NÍVEL no alto, LED "
          "no meio e pisante; o GANHO, que o original não tem, fica pequeno no canto de baixo. Tema: sapota-preta aberta, em "
          "tons quase pretos. Original: Fortin 33.", "112 x 60 x 27 mm (Fortin, ficha oficial)", (60, 112, 27), "#121212",
          "#2C2C2E", [k("nivel", "NÍVEL", (0.0, 0.7), escala=1.1), k("ganho", "GANHO", (0.7, -0.5), escala=0.5, rotulo_m=0.0026)],
          preto, led=[0.0, 0.42], logo_y=-0.9, logo_m=0.006, nome_cor="#3A3A3C",
          textos=[{"texto": "33", "pos": [0.0, 0.0], "tam_m": 0.034, "cor": lin("#050505"), "grossura": 0.0004}], **fortin)
    caixa("blackcurrant_bass", "Blackcurrant Bass", "BLACKCURRANT BASS",
          "Caixa deitada (125 x 96 x 40 mm) de tampo preto e laterais prateadas, letras brancas; oito botões pretos em duas "
          "fileiras: VOLUME (MASTER, fixo), MISTURA (fixo), NÍVEL e DRIVE em cima, GRAVE, MÉDIO GRAVE e MÉDIO AGUDO (fixos) e "
          "AGUDO embaixo; minichaves ATTACK, GRUNT e as das frequências entre eles, LED e dois pisantes (DISTORÇÃO e BYPASS). "
          "Tema: cacho de groselha-preta. Original: Darkglass Microtubes B7K Ultra.", "125 x 96 x 57 mm (Darkglass, ficha "
          "oficial)", (125, 96, 40), "#141414", BRANCO,
          [fixo("master", "VOLUME", (-0.69, 0.66)), fixo("blend", "MISTURA", (-0.23, 0.66)), k("nivel", "NÍVEL", (0.23, 0.66)),
           k("ganho", "DRIVE", (0.69, 0.66)), fixo("grave", "GRAVE", (-0.69, 0.08)), fixo("lomid", "MÉDIO GRAVE", (-0.23, 0.08)),
           fixo("himid", "MÉDIO AGUDO", (0.23, 0.08)), k("tom", "AGUDO", (0.69, 0.08))],
          estilo(diam=0.017, alt=0.014), led=[0.0, -0.42], pintura_lados={"cor": lin("#D8DADC"), "cromo": True},
          pisante={"pos": [0.66, -0.66]}, logo_y=-0.86, logo_m=0.0065, rotulo_m=0.0034, chanfro=0.005,
          pecas=[{"tipo": "minichave", "pos": [-0.46, 0.42], "rotulo": "ATAQUE", "rotulo_m": 0.0026},
                 {"tipo": "minichave", "pos": [0.46, 0.42], "rotulo": "GRUNT", "rotulo_m": 0.0026},
                 {"tipo": "minichave", "pos": [-0.46, -0.12]}, {"tipo": "minichave", "pos": [0.46, -0.12]},
                 {"tipo": "pisante", "pos": [-0.66, -0.66], "rotulo": "DISTORÇÃO", "rotulo_m": 0.003}])
    caixa("mulberry_omega", "Labradorite Omega", "LABRADORITE OMEGA",
          "Caixa azul-ardósia (94 x 120 x 46 mm), letras brancas e o triângulo com o círculo em linhas brancas; MISTURA (fixo), "
          "NÍVEL e DRIVE na fileira de cima com as minichaves GROWL e BITE, MOD, GRAVE, MÉDIO (fixos) e AGUDO embaixo, LED "
          "vermelho e pisante dentro do desenho. Tema: labradorita escura com reflexos azul e verde. Original: Darkglass Alpha "
          "Omega.", "94 x 120 x 46 mm (Darkglass, ficha oficial)", (94, 120, 46), "#4A6A8E", BRANCO,
          [fixo("blend", "MISTURA", (-0.6, 0.78)), k("nivel", "NÍVEL", (-0.12, 0.78)), k("ganho", "DRIVE", (0.36, 0.78)),
           fixo("mod", "MOD", (-0.72, 0.4)), fixo("grave", "GRAVE", (-0.24, 0.4)), fixo("medio", "MÉDIO", (0.24, 0.4)),
           k("tom", "AGUDO", (0.72, 0.4))], estilo(diam=0.015, alt=0.013), led=[0.0, 0.05], pisante_y=-0.58,
          logo_y=-0.9, logo_m=0.0068, rotulo_m=0.0034, metalico=0.2,
          pecas=[{"tipo": "minichave", "pos": [0.76, 0.86], "rotulo": "GROWL", "rotulo_pos": "baixo", "rotulo_m": 0.0026},
                 {"tipo": "minichave", "pos": [0.76, 0.64]}])
    caixa("pineapple_king", "Amethyst King", "AMETHYST KING",
          "Caixa deitada roxo-vinho (121 x 95 x 38 mm), letras douradas; dois lados de overdrive: VOLUME e DRIVE em cima e TOM "
          "embaixo em cada lado (o lado direito é fixo), LED amarelo à esquerda e vermelho à direita, dois pisantes. Tema: "
          "drusa de ametista roxa no lugar da rosa dos ventos, pontos dourados tom sobre tom. Original: Analogman King of Tone.",
          "121 x 95 x 38 mm (Analogman, ficha oficial)", (121, 95, 38), "#5A2448", "#D8B04A",
          [k("nivel", "VOLUME", (-0.78, 0.62)), k("ganho", "DRIVE", (-0.38, 0.62)), fixo("vol2", "VOLUME", (0.38, 0.62)),
           fixo("drive2", "DRIVE", (0.78, 0.62)), k("tom", "TOM", (-0.62, 0.08), escala=0.85),
           fixo("tom2", "TOM", (0.62, 0.08), escala=0.85)],
          estilo(diam=0.016, alt=0.014), led=[-0.4, -0.42], led_cor=lin("#6A5200"), pisante={"pos": [-0.66, -0.66]},
          logo_y=-0.86, logo_m=0.0068, rotulo_m=0.0036, metalico=0.25,
          pecas=[{"tipo": "pisante", "pos": [0.66, -0.66]}, {"tipo": "led", "pos": [0.4, -0.42]}])
    caixa("strawberry_5150", "Jet 5150", "JET 5150",
          "Caixa deitada cinza-escuro escovado com listras pretas (122 x 91 x 40 mm), letras brancas; botões pretos SAÍDA, "
          "GRAVE e MÉDIO (fixos), AGUDO e GANHO em fila, o GATE pequeno à esquerda (fixo) e o botão de apertar REFORÇO com LED "
          "à direita; LED e pisante. Tema: azeviche preto lapidado, listras pretas. Original: EVH 5150 Overdrive.",
          "122 x 91 x 52 mm (Thomann; a Dunlop não publica)", (122, 91, 40), "#3A3C40", BRANCO,
          [k("nivel", "SAÍDA", (-0.6, 0.62)), fixo("grave", "GRAVE", (-0.3, 0.62)), fixo("medio", "MÉDIO", (0.0, 0.62)),
           k("tom", "AGUDO", (0.3, 0.62)), k("ganho", "GANHO", (0.6, 0.62)), fixo("gate", "GATE", (-0.86, 0.3), escala=0.6)],
          estilo(diam=0.0145, alt=0.013), led=[0.0, -0.3], metalico=0.6, aspereza=0.35, pisante_y=-0.6, logo_y=-0.88,
          logo_m=0.0065, rotulo_m=0.0032,
          pecas=[{"tipo": "apertar", "pos": [0.86, 0.3], "diam_m": 0.006, "rotulo": "REFORÇO", "rotulo_m": 0.0026},
                 {"tipo": "led", "pos": [0.86, 0.6], "raio_m": 0.0022}])
    caixa("cupuacu_hammer", "Galena Hammer", "GALENA HAMMER",
          "Caixa deitada prateada (140 x 102 x 38 mm), letras pretas com etiquetas vermelhas de DRIVE; GANHO, AGUDO, GRAVE "
          "(fixo) e VOLUME na fileira de cima, FREQUÊNCIA e MÉDIO (fixos) embaixo, DRIVE e MISTURA pequenos (fixos), dois "
          "pisantes (ENGAGE e DRIVE) com LED azul e amarelo. Tema: cubos de galena cinza-chumbo. Original: Aguilar Tone Hammer.",
          "140 x 102 x 38 mm (Aguilar, ficha oficial)", (140, 102, 38), "#C8CACC", PRETO,
          [k("ganho", "GANHO", (-0.72, 0.6)), k("tom", "AGUDO", (-0.24, 0.6)), fixo("grave", "GRAVE", (0.24, 0.6)),
           k("nivel", "VOLUME", (0.72, 0.6)), fixo("freq", "FREQUÊNCIA", (-0.24, 0.02)), fixo("medio", "MÉDIO", (0.24, 0.02)),
           fixo("drive", "DRIVE", (-0.48, 0.86), escala=0.5, rotulo_pos="cima", rotulo_m=0.0026),
           fixo("mix", "MISTURA", (0.48, 0.86), escala=0.5, rotulo_pos="cima", rotulo_m=0.0026)],
          estilo(diam=0.017, alt=0.014), led=[-0.62, -0.42], led_cor=lin("#0A1E6A"), metalico=0.8, aspereza=0.3,
          pisante={"pos": [-0.62, -0.68]}, logo_y=-0.68, logo_m=0.0075, rotulo_m=0.0034,
          pecas=[{"tipo": "pisante", "pos": [0.62, -0.68]}, {"tipo": "led", "pos": [0.62, -0.42], "cor": lin("#6A5200")},
                 {"tipo": "placa", "pos": [-0.48, 0.74], "tam_m": [0.012, 0.004], "cor": lin("#C81E22")},
                 {"tipo": "placa", "pos": [0.62, -0.88], "tam_m": [0.014, 0.0045], "cor": lin("#C81E22")}])
    caixa("graviola_bassdi", "Melanite Bass DI", "MELANITE BASS DI",
          "Caixa MXR deitada preta (122 x 92 x 40 mm), letras brancas; VOLUME (limpo), DISTORÇÃO, MISTURA, SENSIBILIDADE "
          "(TRIGGER) e GANHO na fileira de cima, GRAVE, MÉDIO e AGUDO embaixo (os que o som não usa são fixos), botões de "
          "apertar COR, GATE e PHANTOM, LEDs vermelho, vermelho e amarelo e dois pisantes (EFEITO e DISTORÇÃO). Tema: "
          "melanita, granada negra lapidada. Original: MXR M80 Bass D.I.+.",
          "aprox. 122 x 92 mm (estimativa; a Dunlop não publica)", (122, 92, 40), "#141414", BRANCO,
          [k("nivel", "VOLUME", (-0.6, 0.68)), fixo("dist", "DISTORÇÃO", (-0.3, 0.68)), fixo("blend", "MISTURA", (0.0, 0.68)),
           fixo("trigger", "SENSIBILIDADE", (0.3, 0.68)), k("ganho", "GANHO", (0.6, 0.68)), fixo("grave", "GRAVE", (-0.3, 0.2)),
           fixo("medio", "MÉDIO", (0.0, 0.2)), k("tom", "AGUDO", (0.3, 0.2))],
          estilo(diam=0.0145, alt=0.013), led=[-0.62, -0.42], pisante={"pos": [-0.62, -0.68]}, logo_y=-0.62,
          logo_m=0.0068, rotulo_m=0.003,
          pecas=[{"tipo": "apertar", "pos": [-0.86, 0.4], "diam_m": 0.005, "rotulo": "COR", "rotulo_m": 0.0026},
                 {"tipo": "apertar", "pos": [0.68, 0.32], "diam_m": 0.005, "rotulo": "GATE", "rotulo_m": 0.0026},
                 {"tipo": "led", "pos": [0.86, 0.32], "cor": lin("#6A5200"), "raio_m": 0.0022},
                 {"tipo": "apertar", "pos": [-0.86, -0.05], "diam_m": 0.005, "rotulo": "PHANTOM", "rotulo_m": 0.0026},
                 {"tipo": "pisante", "pos": [0.62, -0.68]}, {"tipo": "led", "pos": [0.62, -0.42]}])
    caixa("pulsar_echo", "Pulsar Echo", "PULSAR ECHO",
          "Caixa deitada (146 x 121 x 50 mm) dividida na diagonal: prateada em cima à esquerda, preta embaixo à direita; "
          "botões pretos MISTURA, NÍVEL (fixo), REPETIÇÕES, CHORUS (fixo), PROFUNDIDADE (fixo) e TEMPO em fila no alto, LED de "
          "sobrecarga e LED de ligado, pisante; letras pretas no prateado e brancas no preto. Tema: pulsar prateado com os "
          "anéis de onda. Original: Electro-Harmonix Deluxe Memory Man.", "146 x 121 x 64 mm (EHX: 5,75 x 4,75 x 2,5 pol.)",
          (146, 121, 50), "#141414", "#141414",
          [k("c", "MISTURA", (-0.75, 0.68)), fixo("nivel", "NÍVEL", (-0.45, 0.68)), k("b", "REPETIÇÕES", (-0.15, 0.68)),
           fixo("chorus", "CHORUS", (0.15, 0.68)), fixo("prof", "PROFUNDIDADE", (0.45, 0.68)), k("a", "TEMPO", (0.75, 0.68))],
          estilo(diam=0.016, alt=0.014), led=[0.3, -0.36], pisante={"pos": [0.3, -0.62]}, logo_x=0.5, logo_y=0.0, logo_m=0.012,
          rotulo_m=0.0032, rotulo_embaixo=True, nome_cor=BRANCO, metalico=0.2,
          paineis=[{"pontos": [[-1, 1], [1, 1], [1, 0.42], [-1, -0.62]], "cor": lin("#C6C8CA"), "metal": True}],
          pecas=[{"tipo": "led", "pos": [-0.62, 0.36], "rotulo": "SOBRECARGA", "rotulo_m": 0.0026, "rotulo_cor": lin(PRETO)}])
    novo("jackfruit_monkey", "Jackfruit Monkey", "JACKFRUIT MONKEY",
         "Caixa DigiTech verde (79 x 125 x 54 mm) com o pedal basculante largo de borracha; botões pretos NÍVEL, GRAVE (fixo), "
         "AGUDO e GANHO em fila no alto, LED vermelho, letras brancas e o nome em amarelo. Tema: jaca verde aberta, listras "
         "tom sobre tom. Original: DigiTech Bad Monkey.", "79 x 125 x 54 mm (Bax Music; a DigiTech não publica)", 79, 125, 54,
         "#2E8A3A", BRANCO,
         [k("nivel", "NÍVEL", (-0.69, 0.7), escala=0.85), fixo("grave", "GRAVE", (-0.23, 0.7), escala=0.85),
          k("tom", "AGUDO", (0.23, 0.7), escala=0.85), k("ganho", "GANHO", (0.69, 0.7), escala=0.85)],
         estilo(diam=0.0135, alt=0.012), formato="compacto", aspereza=0.26, base_rel=0.7, basculante_rel=0.6,
         borracha_rel=0.62, borracha_larg=0.9, modelo_m=0.0084, modelo_x=0.0, tipo_texto="", rotulo_embaixo=True,
         rotulo_m=0.0036, led=[0.0, 0.9], modelo_cor=lin("#F2C21E"))
    novo("lychee_bassfuzz", "Lime Bass Fuzz", "LIME BASS FUZZ",
         "Caixa de alumínio fundido (102 x 121 x 45 mm) com o tampo verde-limão vivo e as bordas prateadas, letras pretas e o "
         "nome em branco; botões pretos VOLUME, TOM e SUSTAIN em fila, minichave BASS BOOST / NORM / DRY embaixo deles, LED "
         "vermelho e pisante. Tema: fatia de limão verde, raios tom sobre tom. Original: Electro-Harmonix Bass Big Muff Pi.",
         "121 x 102 x 57 mm (EHX: 4,75 x 4 x 2,25 pol.)", 102, 121, 45, "#86CC1E", PRETO,
         [k("nivel", "VOLUME", (-0.6, 0.68)), k("tom", "TOM", (0.0, 0.68)), k("ganho", "SUSTAIN", (0.6, 0.68))],
         estilo(diam=0.019, alt=0.015), pintura_lados={"cor": lin("#C8CACC"), "cromo": True}, tampo_margem_m=0.005,
         chanfro_m=0.005, rotulo_m=0.0042, rotulo_embaixo=True, led=[0.62, -0.38], pisante_y=-0.62, logo_y=-0.12,
         logo_m=0.013, texto_cor2=lin(BRANCO), transformacao="Standard",
         pecas=[{"tipo": "minichave", "pos": [-0.5, 0.3], "rotulo": "GRAVE", "rotulo_pos": "baixo"}])
    multi = dict(formato="padrao", chanfro_m=0.006, rotulo_m=0.0038, rotulo_embaixo=True, metalico=0.5, aspereza=0.26)
    novo("juniper_sky", "Aquamarine Sky", "AQUAMARINE SKY",
         "Caixa grande de alumínio anodizado azul-celeste (171 x 130 x 50 mm), letras brancas; visor verde no alto à "
         "esquerda, seletor TIPO grande embaixo dele, VALOR (fixo), DECAIMENTO, PRÉ-ATRASO e MISTURA na fileira de cima, TOM, "
         "PARAM 1, PARAM 2 (fixo) e MODULAÇÃO embaixo, três pisantes de metal A, B e C com um LED cada. Tema: água-marinha "
         "lapidada azul-clara. Original: Strymon BigSky.", "171,5 x 129,5 x 68,6 mm com botões (Strymon: 6,75 x 5,1 x 2,7 pol.)",
         171, 130, 50, "#1EAEDC", BRANCO,
         [k("modo", "TIPO", (-0.72, 0.18), escala=1.25), fixo("valor", "VALOR", (-0.3, 0.7)), k("a", "DECAIMENTO", (0.04, 0.7)),
          k("b", "PRÉ-ATRASO", (0.38, 0.7)), k("c", "MISTURA", (0.72, 0.7)), k("tom", "TOM", (-0.3, 0.2)),
          k("nivel", "PARAM 1", (0.04, 0.2)), fixo("param2", "PARAM 2", (0.38, 0.2)), k("d", "MODULAÇÃO", (0.72, 0.2))],
         estilo(diam=0.015, alt=0.013), visor={"pos": [-0.72, 0.72], "largura_rel": 0.2, "altura_rel": 0.14},
         led=[-0.62, -0.42], pisante={"pos": [-0.62, -0.66]}, logo_x=0.0, logo_y=-0.25, logo_m=0.011, tipo_texto="",
         pecas=[{"tipo": "pisante", "pos": [0.0, -0.66], "rotulo": "B", "rotulo_pos": "cima"}, {"tipo": "led", "pos": [0.0, -0.42]},
                {"tipo": "pisante", "pos": [0.62, -0.66]}, {"tipo": "led", "pos": [0.62, -0.42]}], **multi)
    for pid, nome, modelo, cor, orig, insp_cor, tema, rot in (
            ("lime_echo", "Emerald Echo", "EMERALD ECHO", "#2E9A7E", "Line 6 DL4 Delay Modeler", "verde-mar metálico",
             "esmeralda lapidada verde", ("TEMPO", "REPETIÇÕES")),
            ("galaxy_mod", "Galaxy Mod", "GALAXY MOD", "#3A3AA8", "Line 6 MM4 Modulation Modeler", "azul-violeta metálico",
             "galáxia espiral azul-violeta", ("VELOCIDADE", "PROFUNDIDADE"))):
        tweak = {"lime_echo": "MODULAÇÃO", "galaxy_mod": "AJUSTE"}[pid]
        novo(pid, nome, modelo,
             f"Caixa grande {insp_cor} (250 x 152 x 64 mm) com o painel prateado escovado no alto, letras pretas no painel e "
             f"brancas no corpo; seletor de modelo grande à esquerda e {rot[0]}, {rot[1]}, AJUSTE (TWEAK), TWEEZ e MISTURA em "
             f"fila no painel, quatro pisantes de metal (A, B, C e TAP) com um LED vermelho em cima de cada; sem visor. Tema: "
             f"{tema}. Original: {orig}.", "aprox. 250 x 152 x 64 mm (medida de lojas; a Line 6 não publica)", 250, 152, 64,
             cor, BRANCO,
             [k("modo", "MODO", (-0.74, 0.5), escala=1.5, rotulo_cor=lin(PRETO)), k("a", rot[0], (-0.42, 0.5), rotulo_cor=lin(PRETO)),
              k("b", rot[1], (-0.18, 0.5), rotulo_cor=lin(PRETO)), k("d", tweak, (0.06, 0.5), rotulo_cor=lin(PRETO)),
              k("tom", "TWEEZ", (0.30, 0.5), rotulo_cor=lin(PRETO)), k("c", "MISTURA", (0.54, 0.5), rotulo_cor=lin(PRETO))],
             estilo(diam=0.019, alt=0.015), led=[-0.72, -0.36], pisante={"pos": [-0.72, -0.62]}, logo_x=0.78, logo_y=0.5,
             logo_m=0.0085, tipo_texto="", texto_cor2=lin(PRETO),
             paineis=[{"pos": [0.0, 0.5], "tam_m": [0.236, 0.056], "cor": lin("#C4C6C8"), "metal": True, "raio_m": 0.002}],
             pecas=[{"tipo": "pisante", "pos": [x, -0.62]} for x in (-0.24, 0.24, 0.72)] +
                   [{"tipo": "led", "pos": [x, -0.36]} for x in (-0.24, 0.24, 0.72)], **dict(multi, metalico=0.6))


LOTES = {"a": lote_a, "b": lote_b, "c": lote_c, "d": lote_d}
if __name__ == "__main__":
    for a in sys.argv[1:]:
        if a == "acerola_eq":
            acerola()
        else:
            LOTES[a]()
