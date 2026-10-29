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


LOTES = {"a": lote_a}
if __name__ == "__main__":
    for a in sys.argv[1:]:
        if a == "acerola_eq":
            acerola()
        else:
            LOTES[a]()
