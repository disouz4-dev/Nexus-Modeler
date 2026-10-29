# Gera a arte (estampa do tampo e logo da etiqueta) de cada pedal a partir da
# receita: lê onde ficam botões, rótulos, LED, nome e pisante, acha o espaço
# livre para o desenho principal e cobre o resto com um padrão tom sobre tom.
#
# Uso: python pedais_arte.py [prefixo ...]   (sem nada: todos os da tabela)
# Grava logos/estampas/<prefixo>.svg (e <prefixo>_logo.svg nos clássicos) e
# põe "estampa"/"logo" na receita. Precisa do shapely.
import json
import math
import os
import sys

from shapely import affinity
from shapely.geometry import Point, box
from shapely.ops import unary_union

import arte as A

AQUI = os.path.dirname(os.path.abspath(__file__))
REC = os.path.join(AQUI, "..", "recipes")
OUT = os.path.join(AQUI, "estampas")

CREME = "#F6F0E2"


# ------------------------------------------------------------------ geometria da receita
def geometry(rec):
    """Tudo em milímetros, com y para baixo e origem no canto de trás à esquerda do tampo."""
    W, H = rec["medidas_m"]["largura"] * 1000, rec["medidas_m"]["comprimento"] * 1000
    shape = rec["formato"]
    g = {"W": W, "H": H, "shape": shape, "obst": [], "clear": [], "labels": []}
    sx = lambda u: W / 2 + u * W / 2
    sy = lambda v: H / 2 - v * H / 2
    st = rec["botao_estilo"]
    g["knobs"] = []
    for b in rec["botoes"]:
        r = st["diametro_m"] / 2 * b.get("escala", 1.0) * 1000 * (1.28 if st.get("saia") else 1.0)
        x, y = sx(b["pos"][0]), sy(b["pos"][1])
        g["knobs"].append((x, y, r))
        lh = rec.get("rotulo_m", 0.0052) * 1000
        ly = y + r + 5.5 if rec.get("rotulo_embaixo") else y - r - 5.5
        lw = len(b["rotulo"]) * lh * 0.66 + 2
        lab = box(x - lw / 2, ly - lh / 2 - 1, x + lw / 2, ly + lh / 2 + 1)
        g["labels"].append(lab)
        g["obst"].append(unary_union([A.circle(x, y, r + 1), lab]))
    sl = rec.get("deslizantes", [])
    if sl:
        lay = rec.get("deslizantes_layout", {})
        travel, ym = lay.get("curso", 0.020) * 1000, sy(lay.get("y", 0.62))
        mg = lay.get("margem", 0.0075) * 1000
        gap = 3.0 if sl[-1]["id"] == "nivel" else 0.0
        step = (W - 2 * mg - gap) / (len(sl) - 1)
        for j, c in enumerate(sl):
            x = mg + j * step + (gap if j == len(sl) - 1 else 0.0)
            g["knobs"].append((x, ym, 0))
            g["sliders"] = g.get("sliders", []) + [(x, ym, travel)]
            ly = ym + travel / 2 + 6.2
            lab = box(x - len(c["rotulo"]) * 1.2 - 1, ly - 2.5, x + len(c["rotulo"]) * 1.2 + 1, ly + 2.5)
            g["labels"].append(lab)
            g["obst"].append(unary_union([box(x - 4.5, ym - travel / 2 - 3.5, x + 4.5, ym + travel / 2 + 3.5), lab]))
    if "led" in rec:
        lx, ly = sx(rec["led"][0]), sy(rec["led"][1])
        g["led"] = (lx, ly)
        g["obst"].append(A.circle(lx, ly, 5.5))
        g["clear"].append(A.circle(lx, ly, 5.5))
    if "visor" in rec:
        v = rec["visor"]
        vx, vy = sx(v["pos"][0]), sy(v["pos"][1])
        vw, vl = v["largura_rel"] * W, v["altura_rel"] * H
        vb = box(vx - vw / 2 - 3, vy - vl / 2 - 3, vx + vw / 2 + 3, vy + vl / 2 + 3)
        g["obst"].append(vb)
        g["clear"].append(vb)
    cham = rec.get("chanfro_m", 0.0045) * 1000
    if shape == "padrao":
        m = cham + 1.0
        g["areas"] = [A.poly([(m, m), (W - m, m), (W - m, H - m), (m, H - m)]).buffer(-1.5).buffer(1.5)]
        ny = sy(rec.get("logo_y", 0.0))
        size = rec.get("logo_m", 0.024) * 1000 if rec.get("logo_script") else 10.0
        nw = len(rec["modelo"]) * size * (0.42 if rec.get("logo_script") else 0.5) + 4
        name = box(W / 2 - nw / 2, ny - size * 0.62, W / 2 + nw / 2, ny + size * 0.62)
        g["name"] = (W / 2, ny, size, rec["modelo"], rec.get("logo_script", False))
        g["obst"].append(name)
        g["clear"].append(name)
        if rec.get("tipo_texto"):
            ty = ny + 11
            tw = len(rec["tipo_texto"]) * 4.5 * 0.62 * 1.6 + 3
            tb = box(W / 2 - tw / 2, ty - 3.2, W / 2 + tw / 2, ty + 3.2)
            g["tipo"] = (W / 2, ty, 4.5, rec["tipo_texto"])
            g["obst"].append(tb)
            g["clear"].append(tb)
        py = sy(rec.get("pisante_y", -0.55))
        g["stomp"] = (W / 2, py, 7.5)
        g["obst"].append(A.circle(W / 2, py, 10.5))
    elif shape == "classico":
        m = cham + 1.0
        fy = sy(rec.get("etiqueta_y", 0.0))
        fw, fh = W * 0.70, H * 0.13
        g["frame"] = (W / 2, fy, fw, fh)
        pl = H * 0.40
        g["pad"] = (W / 2, H - pl / 2 - 4, W * 0.86, pl)
        top = A.poly([(m, m), (W - m, m), (W - m, fy - fh / 2 - 2), (m, fy - fh / 2 - 2)])
        mid = box(m, fy + fh / 2 + 2, W - m, H - pl - 6)
        g["areas"] = [top] + ([mid] if mid.area > 20 else [])
        g["obst"].append(box(W / 2 - fw / 2, fy - fh / 2, W / 2 + fw / 2, fy + fh / 2))
    else:  # compacto
        tl = H * rec.get("basculante_rel", 0.64)
        tb_y = H - tl  # onde começa o pedal basculante
        g["treadle_y"] = tb_y
        ty_c = H - tl / 2
        rr = rec.get("borracha_rel", 0.62)
        rub_h = rr * tl
        rub_c = H - 0.15 * tl - rub_h / 2
        g["rubber"] = (W / 2, rub_c, W * 0.80, rub_h)
        base = box(4.5, 4.5, W - 4.5, tb_y - 2.5)
        strip = box(6.5, tb_y + 6.5, W - 6.5, rub_c - rub_h / 2 - 1.5)
        g["areas"] = [base] + ([strip] if strip.bounds[3] - strip.bounds[1] > 2 else [])
        size = rec.get("modelo_m", 0.0085) * 1000
        nx = W / 2 + rec.get("modelo_x", 0.0) * W / 2
        ny = ty_c - tl * 0.38
        nw = len(rec["modelo"]) * size * 0.5 + 3
        name = box(nx - nw / 2, ny - size * 0.65, nx + nw / 2, ny + size * 0.65)
        g["name"] = (nx, ny, size, rec["modelo"], False)
        g["clear"].append(name)
        # o desenho principal fica no pedal basculante, entre o nome e a borracha
        g["obst"] = [box(0, 0, W, ny + size * 0.65 + 1.0), box(0, rub_c - rub_h / 2 - 1.0, W, H)]
        if rec.get("tipo_texto"):
            g["tipo"] = (W / 2, ty_c + tl * 0.40, 6.2, rec["tipo_texto"])
    g["safe"] = unary_union(g["areas"])
    return g


def free_zone(g, minh=12.0):
    """Maior faixa horizontal livre de obstáculos dentro da área (para o desenho principal)."""
    safe = g["safe"]
    minx, miny, maxx, maxy = safe.bounds
    obst = unary_union(g["obst"])
    best = None
    y = miny
    step = 0.5
    start = None
    while y <= maxy + step:
        band = box(minx + 4, y, maxx - 4, y + step)
        free = band.within(safe) and not band.intersects(obst)
        if free and start is None:
            start = y
        if (not free or y > maxy) and start is not None:
            h = y - start
            if h >= minh and (best is None or h > best[1] - best[0]):
                best = (start, y)
            start = None
        y += step
    return best


# ------------------------------------------------------------------ montagem
def compose(prefix, rec, spec):
    g = geometry(rec)
    W, H = g["W"], g["H"]
    paint = spec.get("paint") or A.lin_to_hex(rec["pintura"]["cor"])
    ink = A.lin_to_hex(rec["texto_cor"])
    tn = spec.get("tone") or A.tone(paint, spec.get("tone_amt", 0.14))
    clear = unary_union(g["clear"] + g["labels"]).buffer(1.2)
    area = g["safe"].difference(clear)
    shapes = []
    # 1. padrão de fundo
    pat = spec.get("pattern", ("dots",))
    kind = pat[0]
    zone = free_zone(g)
    ecx = W / 2
    ecy = (zone[0] + zone[1]) / 2 if zone else H / 2
    if kind == "scatter":
        motif = A.FRUTAS[pat[1]]() if isinstance(pat[1], str) else pat[1]
        shapes += A.pattern_scatter(area, motif, pat[2], tn, pat[3], seed=len(prefix))
    elif kind == "rays":
        shapes += A.pattern_rays(area, tn, ecx, ecy, pat[1] if len(pat) > 1 else 28)
    elif kind == "stripes":
        shapes += A.pattern_stripes(area, tn, pat[1], pat[2], pat[3] if len(pat) > 3 else 35)
    elif kind == "dots":
        shapes += A.pattern_dots(area, tn, pat[1] if len(pat) > 1 else 3.2, pat[2] if len(pat) > 2 else 1.1, ecx, ecy)
    elif kind == "waves":
        shapes += A.pattern_waves(area, tn, pat[1], pat[2], pat[3], pat[4])
    elif kind == "rings":
        rings = [A.ring(A.circle(ecx, ecy, r, 96), pat[2]) for r in [8 + i * pat[1] for i in range(30)]]
        shapes += [(unary_union(rings).intersection(area), tn)]
    elif kind == "stars":
        shapes += A.stars_field(area, pat[1] if len(pat) > 1 else tn, pat[2] if len(pat) > 2 else 40, len(prefix) + 3)
    if spec.get("stars"):
        col, n = spec["stars"]
        shapes += A.stars_field(area, col, n, len(prefix) * 7, avoid=None)
    if spec.get("frame"):
        fr = A.frame((0, 0, W, H), rec.get("chanfro_m", 0.0045) * 1000 + 1.6, 0.55, spec.get("frame_col", ink))
        shapes += [(gg.difference(clear), c) for gg, c in fr]
    # 2. desenho principal
    emb = spec.get("emblem")
    if emb and g["shape"] != "classico":
        if "at" in spec:  # lugar fixo (pedal sem faixa livre de lado a lado)
            ecx, ecy, size = spec["at"]
        else:
            if zone is None:
                raise ValueError("%s: sem espaço livre para o desenho" % prefix)
            zh = zone[1] - zone[0]
            width = (g["safe"].bounds[2] - g["safe"].bounds[0]) - 8
            size = min(zh * spec.get("fill", 0.95), width * 0.62) * spec.get("scale", 1.0)
        art = emb() if callable(emb) else A.FRUTAS[emb]()
        if spec.get("halo") is not False:
            art = A.sticker(art, spec.get("halo") or halo_for(paint, ink), spec.get("halo_w", 2.6))
        if spec.get("ang"):
            art = A.rot(art, spec["ang"])
        # escala pelo contorno real: cabe na altura da faixa e em 3/4 da largura
        mnx, mny, mxx, mxy = A.silhouette(art).bounds
        if "at" in spec:
            k = size / max(mxx - mnx, mxy - mny)
        else:
            k = min(zh * spec.get("fill", 0.92) / (mxy - mny), width * 0.78 / (mxx - mnx)) * min(spec.get("scale", 1.0), 1.0)
        shapes += [(affinity.translate(affinity.scale(gg, k, k, origin=((mnx + mxx) / 2, (mny + mxy) / 2)),
                                       ecx + spec.get("dx", 0) - (mnx + mxx) / 2, ecy + spec.get("dy", 0) - (mny + mxy) / 2), c)
                   for gg, c in art]
    # 3. faixa de baixo (ondas, dunas...)
    if spec.get("ground"):
        shapes += spec["ground"](g, paint, tn, ink)
    # tudo dentro da área segura
    shapes = [(sh.intersection(g["safe"]), c) for sh, c in shapes]
    return g, shapes, paint, ink


def halo_for(paint, ink):
    return CREME if A.lum(paint) < 0.72 else A.mix(ink, paint, 0.0)


def classic_logo(spec):
    art = spec["emblem"]() if callable(spec["emblem"]) else A.FRUTAS[spec["emblem"]]()
    return A.sticker(art, spec.get("halo", CREME), 3.0)


# ------------------------------------------------------------------ faixas de baixo
def waves_ground(cols):
    def f(g, paint, tn, ink):
        W, H = g["W"], g["H"]
        out = []
        top0 = g["stomp"][1] - 4
        for i, (col, foam) in enumerate(cols):
            top = top0 + i * 5
            lam, amp, ph = 26 - i * 4, 2.4 - i * 0.3, i * 1.7
            xs = [i2 * 0.5 for i2 in range(int(W / 0.5) + 2)]
            crest = [(x, top - amp * (0.5 + 0.5 * math.sin(2 * math.pi * x / lam + ph)) ** 3 * 1.6) for x in xs]
            out.append((A.poly(crest + [(W, H), (0, H)]), col))
            under = [(x, y + 0.9 + 0.6 * (0.5 + 0.5 * math.sin(2 * math.pi * x / lam + ph))) for x, y in reversed(crest)]
            out.append((A.poly(crest + under), foam))
        return out
    return f


def dunes_ground(cols):
    def f(g, paint, tn, ink):
        W, H = g["W"], g["H"]
        out = []
        top0 = g["stomp"][1] - 2
        for i, col in enumerate(cols):
            top = top0 + i * 5.5
            xs = [k * 0.6 for k in range(int(W / 0.6) + 2)]
            crest = [(x, top - 3.2 * math.sin(math.pi * ((x / W) * (1.2 + i * 0.4) + i * 0.6)) ** 2) for x in xs]
            out.append((A.poly(crest + [(W, H), (0, H)]), col))
        return out
    return f


def phases_ground(lit, dark):
    def f(g, paint, tn, ink):
        W, H = g["W"], g["H"]
        y = g["stomp"][1]
        out = []
        n = 7
        for i in range(n):
            x = 10 + i * (W - 20) / (n - 1)
            if abs(x - W / 2) < 11:
                continue
            t = i / (n - 1) * 2
            t = t if t <= 1 else 2 - t
            out += A.place(A.phase(40, t if i != n // 2 else 1.0, lit, dark), x, y, 8.5)
        return out
    return f


# ------------------------------------------------------------------ os pedais
P = {}
# Astros (modulação): caixa boutique, desenho grande e céu estrelado.
P["mars_phaser"] = dict(
    ink="#F6E2CC",
    paint="#9E3618", pattern=("dots", 3.0, 0.7), stars=("#F6D9C2", 16),
    emblem=lambda: A.planet(40, "#E06A3A", [(-0.62, -0.5, "#C04E26"), (-0.15, -0.02, "#F08A56"), (0.3, 0.42, "#B2441E")],
                            "#7A2A12", None, spot=(-0.2, 0.55, 0.18, 0.08, "#8E3216"), light="#FFB28A")
    + [(A.circle(-58, -30, 6), "#C9A48E"), (A.circle(-60, -32, 3.5), "#E8CDB8"), (A.circle(52, 34, 4.2), "#BFA08A")],
    halo=False, ground=dunes_ground(["#7E2A12", "#64200C"]))
P["nebula_flanger"] = dict(
    ink="#FFD2EE",
    paint="#2A0E3E", pattern=("stars", "#E8B8F0", 22),
    emblem=lambda: A.nebula(["#5A1E8A", "#A0287E", "#E2569E", "#FFC0DE"], 6), halo=False, scale=1.7, stars=("#FFFFFF", 5))
P["moon_tremolo"] = dict(
    ink="#E6E4DC",
    paint="#1E2230", pattern=("stars", "#B8C0D8", 20),
    emblem=lambda: A.moon(42, "#E9E6DC", "#CCC8BA", "#8A877E"), halo=False,
    ground=phases_ground("#E9E6DC", "#3A4052"))
P["eclipse_vibe"] = dict(
    ink="#D8C2FF",
    paint="#141218", pattern=("rings", 3.2, 0.35), tone="#2A2236",
    emblem=lambda: A.eclipse("#9A6AE0", "#E8D8FF", "#050507"), halo=False, scale=1.15, stars=("#C8B2F0", 8))
P["comet_wah"] = dict(
    ink="#BFE4FF",
    paint="#101520", pattern=("stars", "#9FC8EE", 22),
    emblem=lambda: A.comet("#EAF4FF", "#4C9CE6"), halo=False, ang=-24, scale=1.45, stars=("#FFFFFF", 5))
P["pulsar_echo"] = dict(
    ink="#D8D2FF",
    paint="#1C1846", pattern=("rings", 3.4, 0.4), tone="#2A2662",
    emblem=lambda: A.pulsar("#FFFFFF", "#A8D8FF", ["#8C7CF0", "#6A5AD0", "#4C3EA8", "#36307E"]), halo=False, scale=1.25,
    stars=("#D8D2FF", 9))
P["jupiter_octave"] = dict(
    ink="#FBE8D2",
    paint="#9A4E24", pattern=("dots", 3.0, 0.7), stars=("#FBE3C8", 14),
    emblem=lambda: A.planet(42, "#E2B07A", [(-0.75, -0.55, "#B87A4A"), (-0.38, -0.2, "#F4D6AE"), (-0.05, 0.12, "#C28A58"),
                                            (0.28, 0.42, "#A8683A"), (0.6, 0.72, "#D8A070")], "#8A5A32", None,
                            spot=(0.3, 0.33, 0.2, 0.11, "#C2442A"), light="#FFE6C2")
    + [(A.circle(-60, 24, 4.5), "#E8D6BE"), (A.circle(56, -28, 3.6), "#D6BCA0"), (A.circle(64, 10, 2.8), "#F2E2CC")],
    halo=False)

# Frutas: o desenho da fruta recortado como adesivo, padrão tom sobre tom.
F = {
    # efeitos e capturas antigos
    "orange_drive": ("orange_slice", ("rays", 30)),
    "banana_overdrive": ("banana", ("stripes", 2.2, 3.2, -30)),
    "gold_centaur": (lambda: A.gold_laurel("#2A1E0E", "#B8892E"), ("dots", 3.0, 0.8)),
    "coconut_delay": ("coconut", ("waves", 1.2, 14, 4.5, 0.9)),
    "grape_reverb": ("grapes", ("scatter", "grapes", 7, 13)),
    "pear_gate": ("pear", ("dots", 3.2, 1.0)),
    "cherry_drop": ("cherries", ("dots", 3.0, 0.9)),
    "acai_rat": ("acai", ("stripes", 1.6, 3.0, 30)),
    "jabuticaba_buzzsaw": ("jabuticaba", ("stripes", 1.6, 3.0, 30)),
    "lychee_fuzz": ("lychee", ("dots", 3.2, 1.0)),
    "avocado_metal": ("avocado", ("rays", 30)),
    "pomegranate_mud": ("pomegranate", ("rays", 30)),
    "snowpear_treble": ("snow_pear", ("dots", 3.2, 1.0)),
    # drives da seleção
    "blueberry_blues": ("blueberry", ("dots", 3.0, 0.9)),
    "apricot_overdrive": ("apricot", ("rays", 28)),
    "clementine_turbo": ("clementine", ("stripes", 2.0, 3.0, 35)),
    "cherry_fuzz": ("cherries", ("scatter", "cherries", 7, 13)),
    "fig_bender": ("fig", ("dots", 3.2, 1.0)),
    "kiwi_drive": ("kiwi", ("scatter", A.slice_icon(), 8, 14)),
    "quince_plus": ("quince", ("rays", 26)),
    "starfruit_rock": ("starfruit", ("scatter", "starfruit", 7, 12)),
    "tamarind_amp": ("tamarind", ("stripes", 1.8, 3.4, -35)),
    "blackcurrant_bass": ("blackcurrant", ("dots", 3.2, 1.0)),
    "mulberry_omega": ("mulberry", ("stripes", 1.8, 3.2, -30)),
    "guava_precision": ("guava", ("rays", 30)),
    "peach_glory": ("peach", ("dots", 3.2, 1.0)),
    "raspberry_drive": ("raspberry", ("scatter", "raspberry", 7, 12)),
    "honeydew_drive": ("honeydew", ("stripes", 2.0, 3.2, 30)),
    "pineapple_king": ("pineapple", ("rays", 30)),
    "cashew_boost": ("cashew", ("dots", 3.2, 1.0)),
    "papaya_grind": ("papaya", ("stripes", 2.2, 3.2, -30)),
    "persimmon_33": ("persimmon", ("rays", 26)),
    "strawberry_5150": ("strawberry", ("scatter", "strawberry", 7, 12)),
    "gooseberry_g3": ("gooseberry", ("dots", 3.2, 1.0)),
    "nectarine_od": ("nectarine", ("stripes", 2.0, 3.0, 35)),
    "cranberry_wylde": ("cranberry", ("rays", 28)),
    "kumquat_klone": ("kumquat", ("dots", 3.2, 1.0)),
    "elderberry_war": ("elderberry", ("dots", 3.2, 1.0)),
    "mangosteen_fuzz": ("mangosteen", ("rays", 30)),
    "grapefruit_drive": ("grapefruit", ("scatter", A.slice_icon(), 8, 14)),
    "jackfruit_monkey": ("jackfruit", ("stripes", 2.0, 3.2, 30)),
    "pitanga_250": ("pitanga", ("dots", 3.2, 1.0)),
    "lychee_bassfuzz": ("lychee", ("rays", 28)),
    "cupuacu_hammer": ("cupuacu", ("rays", 28)),
    "graviola_bassdi": ("graviola", ("dots", 3.2, 1.0)),
    "acerola_eq": ("acerola", ("dots", 3.0, 0.9)),
    "apple_comp": ("apple", ("rays", 26)),
    # clássicos: o desenho vai na etiqueta, fatias tom sobre tom no tampo
    "lemon_drive": ("lime_slice", ("scatter", A.slice_icon(), 9, 15)),
    "olive_drive": ("olive", ("scatter", "olive", 9, 15)),
    "pistachio_drive": ("pistachio", ("dots", 3.2, 1.0)),
}
for k, (emb, pat) in F.items():
    P.setdefault(k, dict(emblem=emb, pattern=pat))
# Pedais antigos repintados na cor da própria fruta.
P["avocado_metal"].update(paint="#3A5420", ink=CREME)
P["acai_rat"].update(paint="#2C0C2A", ink="#F2E6F0")
P["pear_gate"].update(paint="#D8E07E", ink="#1E2410")
P["jabuticaba_buzzsaw"].update(paint="#1C0A18")
P["lychee_fuzz"].update(paint="#C8384E", ink=CREME)
P["cherry_drop"].update(at=(37.0, 48.0, 15.0))

ASTROS = ("mars_phaser", "nebula_flanger", "moon_tremolo", "eclipse_vibe", "comet_wah", "pulsar_echo", "jupiter_octave")
KEEP_LAYOUT = ("cherry_drop", "saturn_rotary", "neptune_chorus")


def lin(hexcolor):
    return [round(v ** 2.2, 4) for v in A.rgb(hexcolor)]


def prepare(p, rec):
    """Arruma a receita para a arte: cor, caixa boutique dos astros e a ordem
    LED no topo, botões, desenho, nome e pisante no pé."""
    spec = P[p]
    if p in ASTROS and rec["formato"] == "compacto":
        base = json.load(open(os.path.join(REC, "neptune_chorus.json")))
        for k in ("formato", "medidas_m", "chanfro_m", "rotulo_m", "botao_estilo", "quadro", "rotulo_embaixo"):
            rec[k] = base[k]
        for k in ("basculante_rel", "base_rel", "led_no_basculante", "modelo_m", "modelo_x"):
            rec.pop(k, None)
        rec["botoes"] = [dict(b, pos=nb["pos"], escala=nb.get("escala", 1.0)) for b, nb in zip(rec["botoes"], base["botoes"])]
    if spec.get("paint"):
        rec["pintura"]["cor"] = lin(spec["paint"])
        rec["pintura"]["metalico"] = min(rec["pintura"].get("metalico", 0.2), 0.35)
    if spec.get("ink"):
        rec["texto_cor"] = rec["texto_cor2"] = lin(spec["ink"])
    rec.pop("estampa_ondas", None)
    if rec["formato"] == "compacto":
        rec["borracha_rel"] = 0.36
        # nome no tamanho cheio; o pedal.py encolhe o que não couber ao lado do LED
        rec["modelo_m"] = 0.0085
        rec["modelo_x"] = 0.12
    if rec["formato"] == "padrao" and p not in KEEP_LAYOUT:
        # LED no topo, se couber entre os botões
        W, H = rec["medidas_m"]["largura"] * 1000, rec["medidas_m"]["comprimento"] * 1000
        st = rec["botao_estilo"]
        for v in (0.84, 0.88, 0.80):
            lx, ly = W / 2, H / 2 - v * H / 2
            if ly < 7.5:
                continue
            if all(math.hypot(lx - (W / 2 + b["pos"][0] * W / 2), ly - (H / 2 - b["pos"][1] * H / 2))
                   > st["diametro_m"] / 2 * b.get("escala", 1.0) * 1000 * (1.28 if st.get("saia") else 1.0) + 5.5
                   for b in rec["botoes"]):
                rec["led"] = [0.0, v]
                break
        # Como no Neptune: nome mais baixo, pisante no pé e sem a linha do tipo.
        rec["logo_y"] = -0.367
        rec["pisante_y"] = -0.667
        rec["tipo_texto"] = ""
    return rec


def run(prefixes):
    os.makedirs(OUT, exist_ok=True)
    for p in prefixes:
        path = os.path.join(REC, p + ".json")
        rec = prepare(p, json.load(open(path)))
        spec = P[p]
        g, shapes, paint, ink = compose(p, rec, spec)
        name = "estampas/%s.svg" % p
        A.save_svg(os.path.join(AQUI, name), g["W"], g["H"], shapes, g.get("treadle_y"))
        rec["estampa"] = {"svg": name}
        if g["shape"] == "classico":
            lname = "estampas/%s_logo.svg" % p
            A.save_svg_plain(os.path.join(AQUI, lname), classic_logo(spec))
            rec["logo"] = {"svg": lname}
        else:
            rec.pop("logo", None)
        json.dump(rec, open(path, "w"), ensure_ascii=False, indent=2)
        print(p, "ok")


if __name__ == "__main__":
    run(sys.argv[1:] or [p for p in P if p not in ("saturn_rotary", "neptune_chorus")])
