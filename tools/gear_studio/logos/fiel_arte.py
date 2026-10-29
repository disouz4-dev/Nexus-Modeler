# Arte dos pedais na modelagem fiel: lê a receita nova (botões, peças fixas,
# pisantes, painéis, textos), acha o maior espaço livre do tampo e desenha ali
# o tema (fruta, pedra preciosa ou astro, da cor do pedal original), com um
# padrão tom sobre tom no resto.
#
# Uso: python fiel_arte.py id [id ...]   (precisa do shapely)
# Grava logos/estampas/<id>.svg e põe "estampa" na receita. O tema de cada
# pedal fica em TEMAS, no fim do arquivo.
import json
import math
import os
import random
import sys

import shapely
from shapely import affinity
from shapely.geometry import MultiPolygon, Point, Polygon, box
from shapely.ops import unary_union

import arte as A

AQUI = os.path.dirname(os.path.abspath(__file__))
REC = os.path.join(AQUI, "..", "recipes")
OUT = os.path.join(AQUI, "estampas")
CREME = "#F6F0E2"


# ------------------------------------------------------------------ pedras, astros e frutas novas
def _shades(base):
    return A.mix(base, "#FFFFFF", 0.45), A.mix(base, "#000000", 0.35), A.mix(base, "#FFFFFF", 0.2), A.mix(base, "#000000", 0.15)


def brilhante(base, n=8, r=44):
    """Pedra lapidada redonda vista de cima: mesa, estrelas e facetas."""
    light, dark, mid, mid2 = _shades(base)
    out = [(A.circle(0, 0, r, 96), dark)]
    girdle = [(r * math.cos(2 * math.pi * k / (2 * n) - math.pi / 2), r * math.sin(2 * math.pi * k / (2 * n) - math.pi / 2))
              for k in range(2 * n)]
    t = 0.55
    table = [(r * t * math.cos(2 * math.pi * k / n - math.pi / 2 + math.pi / n), r * t * math.sin(2 * math.pi * k / n - math.pi / 2 + math.pi / n))
             for k in range(n)]
    cols = [light, mid, base, mid2, dark, mid2, base, mid]
    for k in range(2 * n):
        a, b = girdle[k], girdle[(k + 1) % (2 * n)]
        tv = table[(k // 2) % n] if k % 2 else table[(k // 2 - 1) % n]
        out.append((A.poly([a, b, tv]), cols[k % len(cols)]))
    for k in range(n):
        a, b = table[k], table[(k + 1) % n]
        g = girdle[(2 * k + 2) % (2 * n)]
        out.append((A.poly([a, b, g]), cols[(k + 3) % len(cols)]))
    out.append((A.poly(table), A.mix(base, "#FFFFFF", 0.3)))
    out.append((A.poly([(x * 0.55, y * 0.55 - 6) for x, y in table]).intersection(A.poly(table)), A.mix(base, "#FFFFFF", 0.5)))
    out.append((A.star(-r * 0.42, -r * 0.5, 9, 4, 0.18), "#FFFFFF"))
    return out


def esmeralda(base, w=66, h=88, steps=3):
    """Lapidação esmeralda (retângulo de cantos cortados em degraus)."""
    light, dark, mid, mid2 = _shades(base)

    def octo(ww, hh, c):
        return A.poly([(-ww / 2 + c, -hh / 2), (ww / 2 - c, -hh / 2), (ww / 2, -hh / 2 + c), (ww / 2, hh / 2 - c),
                       (ww / 2 - c, hh / 2), (-ww / 2 + c, hh / 2), (-ww / 2, hh / 2 - c), (-ww / 2, -hh / 2 + c)])
    out = []
    shades = [dark, mid2, base, mid]
    for i in range(steps + 1):
        k = 1 - i * 0.16
        g = octo(w * k, h * k, 12 * k)
        # cada degrau: metade clara (cima/esquerda) e metade escura
        out.append((g, shades[i % 4]))
        out.append((g.intersection(A.poly([(-200, -200), (200, -200), (-200, 200)])), A.mix(shades[i % 4], "#FFFFFF", 0.18)))
    out.append((octo(w * 0.42, h * 0.42, 5), A.mix(base, "#FFFFFF", 0.35)))
    out.append((A.star(-w * 0.28, -h * 0.32, 8, 4, 0.18), "#FFFFFF"))
    return out


def cabochao(base, kind="liso", c2=None, c3=None, rx=42, ry=34, seed=3):
    """Pedra polida em domo (oval). kind: liso, faixas (ágata), veios (turquesa,
    howlita), brilho (labradorita, opala, pedra-da-lua), perola, espiral (charoíta)."""
    light, dark, mid, mid2 = _shades(base)
    body = A.ellipse(0, 0, rx, ry)
    out = [(body, base)]
    rnd = random.Random(seed)
    if kind == "faixas":
        for i in range(1, 7):
            k = 1 - i * 0.13
            g = A.blob(0, 0, 1, lambda a, k=k, i=i: k * (1 + 0.06 * math.sin(3 * a + i)), 160)
            g = affinity.scale(g, rx, ry, origin=(0, 0))
            out.append((A.ring(g, 2.2).intersection(body), c2 if i % 2 else (c3 or dark)))
    elif kind == "veios":
        for _ in range(9):
            x0, y0 = rnd.uniform(-rx, rx), rnd.uniform(-ry, ry)
            pts = [(x0, y0)]
            for _ in range(6):
                x0 += rnd.uniform(-14, 14)
                y0 += rnd.uniform(-10, 10)
                pts.append((x0, y0))
            out.append((A.stroke(pts, rnd.uniform(0.8, 2.0)).intersection(body), c2 or dark))
    elif kind == "brilho":
        for c in (c2, c3):
            if not c:
                continue
            for _ in range(4):
                g = A.ellipse(rnd.uniform(-rx * 0.5, rx * 0.5), rnd.uniform(-ry * 0.5, ry * 0.5), rnd.uniform(8, 18), rnd.uniform(5, 12),
                              rnd.uniform(0, 180))
                out.append((g.intersection(body), c))
    elif kind == "espiral":
        for i in range(10):
            a0 = rnd.uniform(0, 6.28)
            pts = [((6 + t * 3.2) * math.cos(a0 + t * 0.5) * rx / 40, (6 + t * 3.2) * math.sin(a0 + t * 0.5) * ry / 40) for t in range(12)]
            out.append((A.stroke(pts, 2.2).intersection(body), c2 if i % 2 else (c3 or light)))
    elif kind == "perola":
        body = A.circle(0, 0, 38, 96)
        out = [(body, base), (A.shade(body, -10, -9), A.mix(base, c2 or "#B8AFA0", 0.6)),
               (A.hl(body, -12, -12, 0.42), A.mix(base, "#FFFFFF", 0.4)), (A.circle(-13, -14, 5), "#FFFFFF")]
        return out
    out.append((A.shade(body, -rx * 0.18, -ry * 0.2), A.mix(base, "#000000", 0.3)))
    out.append((A.hl(body, -rx * 0.3, -ry * 0.32, 0.36), A.mix(base, "#FFFFFF", 0.5)))
    out.append((A.ellipse(-rx * 0.38, -ry * 0.42, 5, 3, -30), "#FFFFFF"))
    return out


def prisma(cx, cy, w, h, ang, base, tip=0.32):
    """Cristal de seis faces (de lado): corpo, ponta e as três faces com luz diferente."""
    light, dark, mid, mid2 = _shades(base)
    pts = [(-w / 2, 0), (w / 2, 0), (w / 2, -h), (0, -h - w * tip * 2), (-w / 2, -h)]
    out = [(A.poly(pts), base), (A.poly([(-w / 2, 0), (-w / 6, 0), (-w / 6, -h - w * tip * 1.3), (-w / 2, -h)]), light),
           (A.poly([(w / 6, 0), (w / 2, 0), (w / 2, -h), (w / 6, -h - w * tip * 1.3)]), dark),
           (A.poly([(-w / 6, -h - w * tip * 1.3), (w / 6, -h - w * tip * 1.3), (0, -h - w * tip * 2)]), mid)]
    return [(affinity.translate(affinity.rotate(g, ang, origin=(0, 0)), cx, cy), c) for g, c in out]


def cristais(base, n=5, long=1.0, rock="#6A6058"):
    """Drusa: cristais saindo de uma pedra."""
    out = [(A.ellipse(0, 30, 40, 12), rock), (A.ellipse(-6, 27, 30, 7), A.mix(rock, "#FFFFFF", 0.15))]
    spec = [(0, 28, 15, 56, 0), (-17, 30, 12, 44, -24), (17, 30, 12, 46, 22), (-30, 33, 9, 28, -45), (30, 33, 9, 30, 44),
            (-8, 30, 8, 30, -10), (9, 30, 8, 26, 12)][:n]
    for x, y, w, h, a in sorted(spec, key=lambda s: -abs(s[4])):
        out += prisma(x, y, w, h * long, a, base)
    return out


def laminas(base, n=6):
    """Selenita: lâminas compridas, translúcidas, com estrias."""
    light, dark, mid, mid2 = _shades(base)
    out = []
    for i in range(n):
        a = -30 + i * 12
        w = 9 - abs(i - n / 2) * 0.6
        g = affinity.rotate(A.poly([(-w / 2, 40), (w / 2, 40), (w * 0.3, -44), (0, -48), (-w * 0.3, -44)]), a, origin=(0, 40))
        out.append((g, [base, mid, light, mid2][i % 4]))
        out.append((affinity.rotate(A.stroke([(0, 36), (0, -40)], 0.7), a, origin=(0, 40)), dark))
    return out


def cubo(cx, cy, s, base):
    light, dark, mid, mid2 = _shades(base)
    c, sn = math.cos(math.radians(30)), 0.5
    top = A.poly([(cx, cy - s), (cx + s * c, cy - s * sn), (cx, cy), (cx - s * c, cy - s * sn)])
    left = A.poly([(cx - s * c, cy - s * sn), (cx, cy), (cx, cy + s), (cx - s * c, cy + s * sn)])
    right = A.poly([(cx, cy), (cx + s * c, cy - s * sn), (cx + s * c, cy + s * sn), (cx, cy + s)])
    out = [(top, light), (left, base), (right, dark)]
    # estrias da pirita
    for k in (0.3, 0.55, 0.8):
        out.append((A.stroke([(cx - s * c * k, cy - s * sn * k + s * k * 0 + s * (1 - k) * 0), (cx - s * c * k, cy - s * sn * k + s)], 0.5)
                    .intersection(left), mid2))
    return out


def cubos(base):
    out = []
    for x, y, s in ((-14, 10, 22), (16, 14, 18), (2, -14, 20), (-30, 22, 11), (30, -6, 10)):
        out += cubo(x, y, s, base)
    return out


def octaedro(base):
    light, dark, mid, mid2 = _shades(base)
    t, b, l, r, f = (0, -46), (0, 46), (-34, 0), (34, 0), (8, 6)
    return [(A.poly([t, l, f]), light), (A.poly([t, f, r]), mid), (A.poly([l, b, f]), base), (A.poly([f, b, r]), dark),
            (A.star(-14, -18, 6, 4, 0.18), "#FFFFFF")]


def botrioide(base):
    """Hematita: bolhas metálicas juntas, com brilho forte."""
    out = []
    for x, y, r in ((-20, 6, 18), (6, -8, 20), (22, 14, 15), (-4, 22, 14), (-30, -14, 11), (28, -18, 10)):
        b = A.circle(x, y, r)
        out += [(b, base), (A.shade(b, -r * 0.3, -r * 0.3), A.mix(base, "#000000", 0.4)),
                (A.hl(b, -r * 0.3, -r * 0.35, 0.3), A.mix(base, "#FFFFFF", 0.55))]
    return out


def colunas(base):
    """Basalto: colunas de seis lados vistas de cima (calçada dos gigantes)."""
    light, dark, mid, mid2 = _shades(base)
    out = []
    r = 11
    rnd = random.Random(5)
    for q in range(-3, 4):
        for k in range(-3, 4):
            x = r * 1.75 * (q + k / 2)
            y = r * 1.5 * k
            if x * x + y * y > 44 * 44:
                continue
            hexa = A.poly([(x + r * math.cos(math.pi / 3 * i + math.pi / 6), y + r * math.sin(math.pi / 3 * i + math.pi / 6)) for i in range(6)])
            out.append((hexa.buffer(-0.9), rnd.choice([base, mid, mid2, light, dark])))
    return out


def ambar(base):
    light, dark, mid, mid2 = _shades(base)
    body = A.blob(0, 2, 40, lambda a: 1 + 0.12 * math.sin(2 * a + 0.5) + 0.06 * math.cos(3 * a))
    out = [(body, base), (A.shade(body, -8, -7), dark), (A.hl(body, -12, -12, 0.4), light)]
    for x, y, r in ((10, 8, 2.4), (-6, 14, 1.6), (16, -6, 1.3), (-14, -2, 1.1)):
        out.append((A.circle(x, y, r), A.mix(base, "#FFFFFF", 0.55)))
    # folhinha presa na resina
    out.append((A.leaf(-2, 6, 18, 8, -30, 0.1), A.mix(base, "#3A2A10", 0.55)))
    return out


def meteorito(base="#4A4642", fogo=("#F2A23A", "#E2562A")):
    """Meteorito em queda: pedra escura com crateras e o rastro de fogo."""
    out = []
    for w, c, l in ((30, fogo[1], 70), (20, fogo[0], 64), (10, "#FFE6A8", 52)):
        g = A.poly([(10, -w / 2), (10 + l, -w * 0.12), (10 + l, w * 0.12), (10, w / 2)])
        out.append((affinity.rotate(g, -35, origin=(0, 0)), c))
    rock = A.blob(-8, 8, 26, lambda a: 1 + 0.12 * math.sin(3 * a) + 0.08 * math.cos(5 * a + 1))
    out.append((rock, base))
    for x, y, r in ((-14, 2, 5), (-2, 16, 4), (4, 0, 3.4), (-18, 18, 3), (-6, -8, 2.6)):
        c = A.circle(x, y, r)
        out += [(c, A.mix(base, "#000000", 0.35)), (A.shade(c, 1.2, 1.2).intersection(c), A.mix(base, "#FFFFFF", 0.2))]
    out.append((A.shade(rock, 6, 5), A.mix(base, "#FFFFFF", 0.25)))
    return out


def greengage():
    out = A.roundfruit("#A8C64A", "#6E8E22", "#E6F2A6", r=36)
    out.append((A.stroke(A.curve((0, -30), (-1, 0), (2, 38)), 1.4), "#7E9A30"))
    out += A.stem_leaf(2, -30, "#4E8E2E", "#5A3A1E", 1, 0.9)
    return out


def jamun():
    out = [(A.stroke(A.curve((-40, -30), (0, -38), (38, -26)), 3), "#5A4030")]
    for x, y in ((-24, -6), (-2, 0), (20, -4), (-12, 22), (10, 24)):
        b = A.ellipse(x, y, 11, 15, 10)
        out += [(b, "#3A1A4A"), (A.shade(b, -3, -3), "#1E0A2A"), (A.hl(b, -3, -5, 0.3), "#9A7AB0")]
    out += [(A.leaf(-30, -30, 30, 12, -150, 0.1), "#3E7E2E"), (A.leaf(26, -28, 28, 11, -20, 0.1), "#3E7E2E")]
    return out


def sapote():
    """Sapota-preta: fruta redonda verde-escura quase preta, aberta mostrando a polpa escura."""
    body = A.circle(0, 4, 38)
    out = [(body, "#2E3A22"), (A.shade(body, -8, -7), "#18200F"), (A.hl(body, -12, -12, 0.25), "#5E6E46")]
    half = A.circle(0, 4, 26)
    out += [(half, "#3A2A1E"), (A.circle(0, 4, 21), "#24160E"), (A.circle(-6, 0, 3), "#4A3A2A"), (A.circle(6, 8, 3), "#4A3A2A")]
    out.append((A.star(0, -34, 9, 4, 0.4), "#4E5A30"))
    return out


def mercurio():
    out = A.planet(42, "#9A9590", [], "#4E4A46", None, light="#D8D4CE")
    for x, y, r in ((-16, -10, 7), (12, 14, 6), (8, -20, 4), (-6, 22, 4.5), (22, -4, 3.5), (-24, 12, 3)):
        c = A.circle(x, y, r)
        out.insert(-2, (c, "#7E7A76"))
        out.insert(-2, (A.shade(c, 1.5, 1.5).intersection(c), "#B6B2AC"))
    return out


def netuno():
    return A.planet(42, "#3AB6CC", [(-0.7, -0.45, "#56C8DA"), (-0.1, 0.12, "#2A9AB4"), (0.4, 0.6, "#48BED2")], "#1E6E86", None,
                    spot=(-0.25, 0.25, 0.22, 0.1, "#1E7896"), light="#B6EEF6")


def jupiter():
    return A.planet(42, "#D6A070", [(-0.75, -0.55, "#A86A3E"), (-0.38, -0.2, "#EEC898"), (-0.05, 0.12, "#B87A48"),
                                     (0.28, 0.42, "#9A5E32"), (0.6, 0.72, "#CC9464")], "#6E4626", None,
                    spot=(0.3, 0.33, 0.2, 0.11, "#B8442A"), light="#FFE0BC")


def marte():
    return A.planet(40, "#E06A3A", [(-0.62, -0.5, "#C04E26"), (-0.15, -0.02, "#F08A56"), (0.3, 0.42, "#B2441E")],
                    "#7A2A12", None, spot=(-0.2, 0.55, 0.18, 0.08, "#8E3216"), light="#FFB28A")


def nebulosa_prata():
    return A.nebula(["#5E6670", "#8C949E", "#B8C0CA", "#E8ECF0"], 6)


def cometa_branco():
    return A.comet("#FFFFFF", "#8C9AAE")


def pulsar_prata():
    return A.pulsar("#FFFFFF", "#D8DEE6", ["#B8C0CA", "#8C949E", "#6A727C", "#4C545E"])


def galaxia_azul():
    return A.galaxy("#F4F2FF", "#9AA8F4", "#4C4EC8", "#2A2A86")


# ------------------------------------------------------------------ geometria da receita
def _sx(W):
    return lambda u: W / 2 + u * W / 2


def geometry(rec):
    """Em milímetros, y para baixo, origem no canto de trás à esquerda do tampo."""
    W, H = rec["medidas_m"]["largura"] * 1000, rec["medidas_m"]["comprimento"] * 1000
    shape = rec["formato"]
    sx, sy = _sx(W), (lambda v: H / 2 - v * H / 2)
    g = {"W": W, "H": H, "shape": shape, "obst": [], "clear": [], "split": None}
    stl = rec["botao_estilo"]
    lab_h = rec.get("rotulo_m", 0.0052) * 1000
    under = rec.get("rotulo_embaixo")

    def label_box(x, y, text, h, below):
        lw = len(text) * h * 0.66 + 2
        ly = y + 0 if False else y
        return box(x - lw / 2, ly - h / 2 - 1, x + lw / 2, ly + h / 2 + 1)

    for b in rec["botoes"]:
        x, y = sx(b["pos"][0]), sy(b["pos"][1])
        st = dict(stl, **b.get("estilo", {}))
        if b.get("tipo") == "deslizante":
            tr = b.get("curso_m", 0.02) * 1000
            g["obst"].append(box(x - 5, y - tr / 2 - 4, x + 5, y + tr / 2 + 4))
            lab = label_box(x, y + tr / 2 + 6.2, b["rotulo"], 3.0, True)
            g["obst"].append(lab)
            g["clear"].append(lab)
            continue
        r = st["diametro_m"] / 2 * b.get("escala", 1.0) * 1000 * (1.28 if st.get("saia") else 1.05 if st.get("forma") == "galinha" else 1.0)
        g["obst"].append(A.circle(x, y, r + 1.5))
        pos = b.get("rotulo_pos", "baixo" if under else "cima")
        if b.get("rotulo") and pos != "nenhum":
            h = b.get("rotulo_m", rec.get("rotulo_m", 0.0052)) * 1000
            ly = y + r + 5.5 if pos == "baixo" else y - r - 5.5
            lab = label_box(x, ly, b["rotulo"], h, pos == "baixo")
            g["obst"].append(lab)
            g["clear"].append(lab)
    sl = rec.get("deslizantes", [])
    if sl:
        lay = rec.get("deslizantes_layout", {})
        travel, ym = lay.get("curso", 0.020) * 1000, sy(lay.get("y", 0.62))
        mg = lay.get("margem", 0.0075) * 1000
        gap = 3.0 if sl[-1]["id"] == "nivel" else 0.0
        step = (W - 2 * mg - gap) / (len(sl) - 1)
        for j, c in enumerate(sl):
            x = mg + j * step + (gap if j == len(sl) - 1 else 0.0)
            ob = box(x - 4.5, ym - travel / 2 - 3.5, x + 4.5, ym + travel / 2 + 9.5)
            g["obst"].append(ob)
            g["clear"].append(ob)
    if "led" in rec:
        lx, ly = sx(rec["led"][0]), sy(rec["led"][1])
        g["obst"].append(A.circle(lx, ly, 5.5))
        g["clear"].append(A.circle(lx, ly, 5.5))
    if "visor" in rec:
        v = rec["visor"]
        vx, vy = sx(v["pos"][0]), sy(v["pos"][1])
        vw, vl = v["largura_rel"] * W, v["altura_rel"] * H
        vb = box(vx - vw / 2 - 3, vy - vl / 2 - 3, vx + vw / 2 + 3, vy + vl / 2 + 3)
        g["obst"].append(vb)
        g["clear"].append(vb)
    for p in rec.get("pecas", []):
        x, y = sx(p["pos"][0]), sy(p["pos"][1])
        k = p["tipo"]
        if k == "placa":
            w, l = p["tam_m"][0] * 1000, p["tam_m"][1] * 1000
            if p.get("forma") == "triangulo":
                sh = A.poly([(x - w / 2, y + l / 2), (x + w / 2, y + l / 2), (x, y - l / 2)])
            else:
                sh = box(x - w / 2, y - l / 2, x + w / 2, y + l / 2)
            g["obst"].append(sh.buffer(1.2))
            g["clear"].append(sh.buffer(0.8))
        elif k == "leds":
            n, step = p["n"], p.get("passo_m", 0.0062) * 1000
            sh = box(x - (n - 1) / 2 * step - 4, y - 4, x + (n - 1) / 2 * step + 4, y + 4)
            g["obst"].append(sh)
            g["clear"].append(sh)
        elif k == "rotulo":
            pass
        else:
            r = {"minichave": 7.5, "apertar": p.get("diam_m", 0.008) * 500 + 3, "chave_deslizante": 8.5, "trimpot": 4,
                 "led": 5.5, "pisante": 12.5}.get(k, 6)
            sh = A.circle(x, y, r)
            g["obst"].append(sh)
            g["clear"].append(sh)
        if p.get("rotulo"):
            dy = {"minichave": 8.5, "apertar": p.get("diam_m", 0.008) * 500 + 5.5, "chave_deslizante": 10.5, "trimpot": 5.5,
                  "led": 6.5}.get(k, 0)
            ly = y - dy if p.get("rotulo_pos", "cima") == "cima" else y + dy
            h = p.get("rotulo_m", rec.get("rotulo_m", 0.0052) * 0.8) * 1000
            lab = label_box(x, ly, p["rotulo"], h, False)
            g["obst"].append(lab)
            g["clear"].append(lab)
    for t in rec.get("textos", []):
        x, y = sx(t["pos"][0]), sy(t["pos"][1])
        h = t.get("tam_m", 0.006) * 1000
        k = 0.42 if t.get("fonte") == "script" else 0.55
        tw = len(t["texto"]) * h * k * t.get("espaco", 1.0) + 3
        if t.get("largura_max"):
            tw = min(tw, t["largura_max"] * W / 2 + 3)
        ax = {"CENTER": 0.5, "LEFT": 0.0, "RIGHT": 1.0}[t.get("alinhar", "CENTER")]
        tb = box(x - tw * ax, y - h * 0.7, x + tw * (1 - ax), y + h * 0.7)
        if t.get("ang"):
            tb = affinity.rotate(tb, -t["ang"], origin=(x, y))
        g["obst"].append(tb)
        g["clear"].append(tb)
    for p in rec.get("paineis", []):
        if p.get("fundo"):
            continue  # painel de fundo (moldura): a arte vai por cima
        if "pontos" in p:
            sh = A.poly([(sx(u), sy(v)) for u, v in p["pontos"]])
        else:
            w, l = p["tam_m"][0] * 1000, p["tam_m"][1] * 1000
            x, y = sx(p["pos"][0]), sy(p["pos"][1])
            sh = box(x - w / 2, y - l / 2, x + w / 2, y + l / 2)
        g["obst"].append(sh.buffer(1.0))
        g["clear"].append(sh.buffer(0.6))
    cham = rec.get("chanfro_m", 0.0045) * 1000
    if shape in ("padrao", "redondo"):
        m = cham + 1.0
        if shape == "redondo":
            g["areas"] = [A.circle(W / 2, H / 2, W / 2 - m, 96)]
        else:
            g["areas"] = [A.poly([(m, m), (W - m, m), (W - m, H - m), (m, H - m)]).buffer(-1.5).buffer(1.5)]
        if not rec.get("modelo_oculto"):
            ny, nx = sy(rec.get("logo_y", 0.0)), sx(rec.get("logo_x", 0.0))
            size = rec.get("logo_m", 0.024 if rec.get("logo_script") else 0.01) * 1000
            nw = min(len(rec["modelo"]) * size * (0.42 if rec.get("logo_script") else 0.5) + 4, W - 2 * m)
            name = box(nx - nw / 2, ny - size * 0.62, nx + nw / 2, ny + size * 0.62)
            g["obst"].append(name)
            g["clear"].append(name)
            if rec.get("tipo_texto"):
                ty = ny + 11
                tw = len(rec["tipo_texto"]) * 4.5 * 0.62 * 1.6 + 3
                tb = box(nx - tw / 2, ty - 3.2, nx + tw / 2, ty + 3.2)
                g["obst"].append(tb)
                g["clear"].append(tb)
        ps = rec.get("pisante", {})
        px, py = (sx(ps["pos"][0]), sy(ps["pos"][1])) if "pos" in ps else (W / 2, sy(rec.get("pisante_y", -0.55)))
        r = {"metal": 11.5, "redondo": 14.5, "pequeno": 9.5, "nenhum": 0.0}.get(ps.get("estilo", "metal"), 11.5)
        if r:
            g["obst"].append(A.circle(px, py, r))
            g["clear"].append(A.circle(px, py, r - 1))
    elif shape == "classico":
        m = cham + 1.0
        fy = sy(rec.get("etiqueta_y", 0.0))
        fw, fh = W * 0.70, H * 0.13
        pl = H * 0.40
        est = rec.get("pisante", {}).get("estilo", "almofada")
        pad_top = H - pl - 4 if est != "placa_pequena" else H - pl / 2 - pl * 0.31 - 6
        top = A.poly([(m, m), (W - m, m), (W - m, fy - fh / 2 - 2), (m, fy - fh / 2 - 2)])
        mid = box(m, fy + fh / 2 + 2, W - m, pad_top - 2)
        g["areas"] = [top] + ([mid] if mid.area > 20 else [])
        if est == "placa_pequena":
            g["areas"].append(box(m, pad_top - 2, W - m, H - m))
            g["obst"].append(box(W / 2 - W * 0.31 - 2, H - pl / 2 - 4 - pl * 0.31 - 2, W / 2 + W * 0.31 + 2, H - pl / 2 - 4 + pl * 0.31 + 2))
        g["obst"].append(box(W / 2 - fw / 2, fy - fh / 2, W / 2 + fw / 2, fy + fh / 2))
        g["clear"].append(box(W / 2 - fw / 2, fy - fh / 2, W / 2 + fw / 2, fy + fh / 2))
    elif shape == "compacto":
        tl = H * rec.get("basculante_rel", 0.64)
        tb_y = H - tl
        g["split"] = tb_y
        ty_c = H - tl / 2
        rr = rec.get("borracha_rel", 0.62)
        rub_h = rr * tl
        rub_c = H - 0.15 * tl - rub_h / 2
        base = box(4.5, 4.5, W - 4.5, tb_y - 2.5)
        strip = box(6.5, tb_y + 6.5, W - 6.5, rub_c - rub_h / 2 - 1.5)
        g["areas"] = [base] + ([strip] if strip.bounds[3] - strip.bounds[1] > 2 else [])
        g["obst"].append(box(0, tb_y - 3, W, tb_y + 7))
        if not rec.get("modelo_oculto"):
            size = rec.get("modelo_m", 0.0085) * 1000
            nx = W / 2 + rec.get("modelo_x", 0.0) * W / 2
            ny = ty_c - tl * 0.38
            nw = min(len(rec["modelo"]) * size * 0.5 + 3, W - 10)
            name = box(nx - nw / 2, ny - size * 0.65, nx + nw / 2, ny + size * 0.65)
            g["obst"].append(name)
            g["clear"].append(name)
            if rec.get("tipo_texto"):
                tyy = ty_c + tl * 0.40
                tw = len(rec["tipo_texto"]) * 6.2 * 0.6 * 1.4
                tb = box(W / 2 - tw / 2, tyy - 4, W / 2 + tw / 2, tyy + 4)
                g["obst"].append(tb)
                g["clear"].append(tb)
    elif shape == "wah":
        ledge = rec.get("talao_m", 0.0) * 1000
        tl = H - ledge - 4
        g["split"] = None
        g["wah_split"] = ledge + 2
        rr = rec.get("borracha_rel", 0.62)
        # borracha: do fundo do basculante para a frente
        r_top = ledge + 2 + tl * 0.06
        rub = box(W * 0.10 - 1, r_top - 1, W * 0.90 + 1, r_top + tl * rr + 1)
        g["areas"] = [box(W * 0.04 + 3, ledge + 6, W * 0.96 - 3, H - 5)]
        g["obst"].append(rub)
        g["clear"].append(rub)
        if not rec.get("modelo_oculto"):
            ny = H - (tl * 0.10 + 2)
            size = rec.get("logo_m", 0.011) * 1000
            nw = min(len(rec["modelo"]) * size * 0.5 + 4, W * 0.8)
            name = box(W / 2 - nw / 2, ny - size * 0.65, W / 2 + nw / 2, ny + size * 0.65)
            g["obst"].append(name)
            g["clear"].append(name)
    g["safe"] = unary_union(g["areas"])
    return g


def biggest_circle(region, prefer_x=None):
    """Centro e raio do maior círculo que cabe na região livre. Entre os lugares
    quase tão bons quanto o maior (90%), fica o mais perto do meio (prefer_x)."""
    top = _biggest_circle(region)
    if top is None or prefer_x is None:
        return top
    rmin = top[2] * 0.9
    bnd = region.boundary
    minx, miny, maxx, maxy = region.bounds
    best, step = top, 0.8
    y = miny
    while y <= maxy:
        x = minx
        while x <= maxx:
            pt = Point(x, y)
            if region.contains(pt):
                r = bnd.distance(pt)
                if r >= rmin and abs(x - prefer_x) < abs(best[0] - prefer_x) - 0.3:
                    best = (x, y, r)
            x += step
        y += step
    return best


def _biggest_circle(region):
    best = None
    polys = [region] if isinstance(region, Polygon) else list(getattr(region, "geoms", []))
    for p in polys:
        if p.is_empty or p.area < 4:
            continue
        line = shapely.maximum_inscribed_circle(p, 0.2)
        (x0, y0), (x1, y1) = line.coords[0], line.coords[1]
        r = math.hypot(x1 - x0, y1 - y0)
        if best is None or r > best[2]:
            best = (x0, y0, r)
    return best


def compose(pid, rec, spec):
    g = geometry(rec)
    W, H = g["W"], g["H"]
    paint = spec.get("paint") or A.lin_to_hex(rec["pintura"]["cor"])
    ink = A.lin_to_hex(rec["texto_cor"])
    tn = spec.get("tone") or A.tone(paint, spec.get("tone_amt", 0.12))
    clear = unary_union(g["clear"]).buffer(1.2)
    area = g["safe"].difference(clear)
    if "zona" in spec:
        x0, y0, x1, y1 = spec["zona"]
        area_emb = g["safe"].intersection(box(x0 * W, y0 * H, x1 * W, y1 * H))
    else:
        area_emb = g["safe"]
    free = area_emb.difference(unary_union(g["obst"]).buffer(1.0))
    shapes = []
    c = biggest_circle(free, W / 2)
    ecx, ecy, er = c if c else (W / 2, H / 2, 10)
    if "at" in spec:
        ecx, ecy, er = spec["at"]
    if "centro" in spec:
        # padrão em volta de um ponto marcado (o alvo do Wylde em volta do LED)
        ecx, ecy = spec["centro"]
    pat = spec.get("pattern", ("dots", 3.2, 0.9))
    kind = pat[0]
    if kind == "scatter":
        motif = pat[1]() if callable(pat[1]) else A.FRUTAS[pat[1]]()
        shapes += A.pattern_scatter(area, motif, pat[2], tn, pat[3], seed=len(pid))
    elif kind == "rays":
        shapes += A.pattern_rays(area, tn, ecx, ecy, pat[1] if len(pat) > 1 else 28)
    elif kind == "stripes":
        shapes += A.pattern_stripes(area, tn, pat[1], pat[2], pat[3] if len(pat) > 3 else 35)
    elif kind == "dots":
        shapes += A.pattern_dots(area, tn, pat[1] if len(pat) > 1 else 3.2, pat[2] if len(pat) > 2 else 1.1, ecx, ecy)
    elif kind == "waves":
        shapes += A.pattern_waves(area, tn, pat[1], pat[2], pat[3], pat[4])
    elif kind == "rings":
        rings = [A.ring(A.circle(ecx, ecy, r, 96), pat[2]) for r in [8 + i * pat[1] for i in range(40)]]
        shapes += [(unary_union(rings).intersection(area), tn)]
    elif kind == "stars":
        shapes += A.stars_field(area, pat[1] if len(pat) > 1 else tn, pat[2] if len(pat) > 2 else 40, len(pid) + 3)
    elif kind == "facets":
        # facetas de pedra: triângulos tom sobre tom
        rnd = random.Random(len(pid))
        tris = []
        minx, miny, maxx, maxy = area.bounds
        st = pat[1] if len(pat) > 1 else 9
        y = miny
        while y < maxy:
            x = minx
            while x < maxx:
                if rnd.random() < 0.45:
                    tris.append(A.poly([(x, y), (x + st, y + rnd.uniform(0, st)), (x + rnd.uniform(0, st), y + st)]))
                x += st
            y += st
        shapes += [(unary_union(tris).intersection(area), tn)]
    elif kind == "none":
        pass
    if spec.get("stars"):
        col, n = spec["stars"]
        shapes += A.stars_field(area, col, n, len(pid) * 7)
    emb = spec.get("emblem")
    if emb:
        art = emb() if callable(emb) else A.FRUTAS[emb]()
        if spec.get("halo") is not False:
            hc = spec.get("halo") or (CREME if A.lum(paint) < 0.6 else A.mix(paint, "#000000", 0.55))
            art = A.sticker(art, hc, spec.get("halo_w", 2.4))
        if spec.get("ang"):
            art = A.rot(art, spec["ang"])
        sil = A.silhouette(art)
        mnx, mny, mxx, mxy = sil.bounds
        cx0, cy0 = (mnx + mxx) / 2, (mny + mxy) / 2
        rad = max(math.hypot(x - cx0, y - cy0) for x, y in
                  (pt for gg in (sil.geoms if hasattr(sil, "geoms") else [sil]) for pt in gg.exterior.coords))
        k = er * spec.get("fill", 0.96) / rad
        shapes += [(affinity.translate(affinity.scale(gg, k, k, origin=(cx0, cy0)), ecx - cx0 + spec.get("dx", 0),
                                       ecy - cy0 + spec.get("dy", 0)), cc) for gg, cc in art]
    shapes = [(sh.intersection(g["safe"]), cc) for sh, cc in shapes]
    return g, shapes


def run(ids):
    os.makedirs(OUT, exist_ok=True)
    for pid in ids:
        path = os.path.join(REC, pid + ".json")
        rec = json.load(open(path, encoding="utf-8"))
        spec = TEMAS[pid]
        g, shapes = compose(pid, rec, spec)
        name = "estampas/%s.svg" % pid
        A.save_svg(os.path.join(AQUI, name), g["W"], g["H"], shapes, g.get("split") or g.get("wah_split"))
        rec["estampa"] = {"svg": name}
        rec.pop("logo", None)
        json.dump(rec, open(path, "w", encoding="utf-8"), ensure_ascii=False, indent=2)
        print(pid, "ok")


# ------------------------------------------------------------------ temas
F = lambda name, **kw: dict(emblem=name, **kw)
TEMAS = {
    # Lote A: Boss compactos
    "orange_drive": F("orange_slice", pattern=("rays", 30)),
    "banana_overdrive": F("banana", pattern=("stripes", 2.2, 3.2, -30)),
    "neptune_chorus": F(netuno, pattern=("stars", None, 26), halo=False),
    "coconut_delay": F("coconut", pattern=("waves", 1.2, 14, 4.5, 0.9)),
    "grape_reverb": F(meteorito, pattern=("stars", None, 30), halo=False),
    "moon_tremolo": F(lambda: cabochao("#3FB6A8", "veios", "#6A4A2E"), pattern=("facets", 9)),
    "jabuticaba_buzzsaw": F("jabuticaba", pattern=("stripes", 1.6, 3.0, 30)),
    "avocado_metal": F(lambda: cabochao("#1A1A1C", "faixas", "#E8E4DC", "#2A2A2C"), pattern=("facets", 9)),
    "blueberry_blues": F("blueberry", pattern=("dots", 3.0, 0.9)),
    "apricot_overdrive": F(lambda: brilhante("#F2B21E"), pattern=("facets", 9)),
    "clementine_turbo": F("clementine", pattern=("stripes", 2.0, 3.0, 35)),
    "jupiter_octave": F(jupiter, pattern=("stars", None, 24), halo=False),
    "acerola_eq": F(lambda: laminas("#EEE8D8"), pattern=("facets", 9), halo="#5E584C", halo_w=1.6),
}
TEMAS.update({
    # Lote B: MXR e caixas estreitas
    "pear_gate": F(lambda: cubos("#B4B0A2"), pattern=("facets", 8)),
    "mars_phaser": F(marte, pattern=("dots", 3.0, 0.8), halo=False),
    "quince_plus": F("quince", pattern=("rays", 26)),
    "apple_comp": F("apple", pattern=("rays", 26)),
    "starfruit_rock": F(lambda: botrioide("#5E626A"), pattern=("dots", 3.0, 0.8)),
    "cranberry_wylde": dict(pattern=("rings", 5.0, 2.2), tone="#141414", centro=(31.0, 45.5)),
    "guava_precision": F(lambda: brilhante("#33363D"), pattern=("facets", 8), halo="#3A8EE8", halo_w=1.4),
    "honeydew_drive": F("honeydew", pattern=("stripes", 2.0, 3.2, 30)),
    "peach_glory": F(lambda: ambar("#E2A02E"), pattern=("dots", 3.2, 0.9)),
    "raspberry_drive": F("raspberry", pattern=("scatter", "raspberry", 7, 12)),
    "gooseberry_g3": F(lambda: cabochao("#7A3AC0", "espiral", "#C8A0F0", "#4A1A80"), pattern=("facets", 8)),
    "nectarine_od": F(jamun, pattern=("dots", 3.0, 0.8), halo="#E8D9A8", halo_w=1.6),
    "mangosteen_fuzz": F(lambda: cabochao("#EEF0F4", "brilho", "#9AB8F0", "#C8D8F8"), pattern=("dots", 3.0, 0.9),
                         tone="#9A84D2", halo="#6A3EB0", halo_w=1.8),
    "grapefruit_drive": F(lambda: brilhante("#8EAC3A"), pattern=("rings", 3.0, 0.5), tone="#7A9230"),
    "pitanga_250": F(lambda: brilhante("#F0A81E"), pattern=("facets", 8)),
})
for _k, _v in TEMAS.items():
    if _v.get("pattern", ("",))[0] == "stars" and _v["pattern"][1] is None:
        _v["pattern"] = ("stars", "#FFFFFF", _v["pattern"][2])

if __name__ == "__main__":
    run(sys.argv[1:])
