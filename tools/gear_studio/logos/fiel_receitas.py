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


LOTES = {"a": lote_a, "b": lote_b}
if __name__ == "__main__":
    for a in sys.argv[1:]:
        if a == "acerola_eq":
            acerola()
        else:
            LOTES[a]()
