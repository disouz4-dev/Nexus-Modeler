# Nexus Gear Studio: biblioteca da arte impressa nos pedais.
#
# Tudo é desenhado como polígonos (shapely) e salvo em SVG para o pedal.py
# imprimir no tampo. As frutas e os astros são desenhados numa caixa de 100
# unidades centrada em (0, 0), com y para baixo, e depois postos no tampo com
# place(). Cada desenho é uma lista de (forma, cor) na ordem das camadas.
#
# Precisa do shapely (pip install shapely).
import math
import random

from shapely import affinity
from shapely.geometry import LineString, MultiPolygon, Point, Polygon, box
from shapely.ops import unary_union

MARCA = "#FF00FF"


# ------------------------------------------------------------------ cores
def rgb(h):
    h = h.lstrip("#")
    return tuple(int(h[i:i + 2], 16) / 255 for i in (0, 2, 4))


def hexc(c):
    return "#%02X%02X%02X" % tuple(max(0, min(255, round(v * 255))) for v in c)


def mix(a, b, t):
    a, b = rgb(a), rgb(b)
    return hexc(tuple(x + (y - x) * t for x, y in zip(a, b)))


def lum(c):
    r, g, b = rgb(c)
    return 0.2126 * r + 0.7152 * g + 0.0722 * b


def tone(paint, amount=0.16):
    """Tom sobre tom: um pouco mais claro na pintura escura, mais escuro na clara."""
    return mix(paint, "#FFFFFF", amount * 1.2) if lum(paint) < 0.42 else mix(paint, "#000000", amount)


def lin_to_hex(c):
    return hexc(tuple(v ** (1 / 2.2) for v in c))


# ------------------------------------------------------------------ formas
def circle(cx, cy, r, res=48):
    return Point(cx, cy).buffer(r, resolution=res)


def ellipse(cx, cy, rx, ry, rot=0.0):
    g = affinity.rotate(affinity.scale(circle(0, 0, 1), rx, ry), rot, origin=(0, 0))
    return affinity.translate(g, cx, cy)


def poly(pts):
    return Polygon(pts).buffer(0)


def blob(cx, cy, r, f, n=180):
    """Forma fechada pelo raio em função do ângulo: r * f(a)."""
    return poly([(cx + r * f(a) * math.cos(a), cy + r * f(a) * math.sin(a))
                 for a in (2 * math.pi * i / n for i in range(n))])


def leaf(x, y, length, width, angle, curve=0.0):
    """Folha: duas curvas que se encontram na ponta, a base em (x, y)."""
    pts = []
    n = 40
    for i in range(n + 1):
        t = i / n
        w = width / 2 * math.sin(math.pi * t) ** 0.85
        pts.append((t * length, -w + curve * math.sin(math.pi * t) * width))
    for i in range(n, -1, -1):
        t = i / n
        w = width / 2 * math.sin(math.pi * t) ** 0.85
        pts.append((t * length, w + curve * math.sin(math.pi * t) * width))
    g = poly(pts)
    return affinity.translate(affinity.rotate(g, angle, origin=(0, 0)), x, y)


def stroke(pts, w, cap=1):
    return LineString(pts).buffer(w / 2, cap_style=cap, join_style=1, resolution=12)


def curve(p0, p1, p2, n=30):
    """Curva de Bézier quadrática (pontos para stroke)."""
    out = []
    for i in range(n + 1):
        t = i / n
        out.append(((1 - t) ** 2 * p0[0] + 2 * (1 - t) * t * p1[0] + t * t * p2[0],
                    (1 - t) ** 2 * p0[1] + 2 * (1 - t) * t * p1[1] + t * t * p2[1]))
    return out


def star(cx, cy, r, points=4, thin=0.22, rot=-90):
    pts = []
    for i in range(points * 2):
        a = math.radians(rot + i * 180 / points)
        rr = r if i % 2 == 0 else r * thin
        pts.append((cx + rr * math.cos(a), cy + rr * math.sin(a)))
    return poly(pts)


def ring(g, w):
    return g.difference(g.buffer(-w))


def shade(g, dx, dy):
    """Sombra em meia-lua: a parte da forma que sai de baixo dela deslocada."""
    return g.difference(affinity.translate(g, dx, dy))


def hl(g, dx, dy, s=0.35):
    """Brilho: forma reduzida e puxada para o lado da luz."""
    c = g.centroid
    return affinity.translate(affinity.scale(g, s, s * 0.8, origin=c), dx, dy).intersection(g)


def rot(shapes, ang, origin=(0, 0)):
    return [(affinity.rotate(g, ang, origin=origin), c) for g, c in shapes]


def place(shapes, cx, cy, size, ang=0.0, flip=False):
    """Põe um desenho de 100 unidades no tampo: centro (cx, cy) e altura size (mm)."""
    k = size / 100.0
    out = []
    for g, c in shapes:
        g = affinity.scale(g, -k if flip else k, k, origin=(0, 0))
        if ang:
            g = affinity.rotate(g, ang, origin=(0, 0))
        out.append((affinity.translate(g, cx, cy), c))
    return out


def silhouette(shapes):
    return unary_union([g for g, _ in shapes])


def sticker(shapes, color, w):
    """Contorno de adesivo recortado em volta do desenho (destaca na pintura)."""
    return [(silhouette(shapes).buffer(w, resolution=16), color)] + shapes


# ------------------------------------------------------------------ frutas
# Cada fruta: desenho de 100 unidades centrado em (0, 0), luz vindo de cima à esquerda.
LIGHT = (-0.09, -0.11)


def roundfruit(base, dark, light, r=40, sq=1.0, cy=4):
    body = affinity.scale(circle(0, cy, r), 1.0, sq)
    return [(body, base), (shade(body, -r * 0.22, -r * 0.18), dark), (hl(body, -r * 0.33, -r * 0.33, 0.3), light)]


def stem_leaf(x=0, y=-36, leafc="#3E8E2E", stemc="#5A3A1E", side=1, size=1.0, ang=-30):
    out = [(stroke(curve((x, y + 6), (x + 1 * side, y - 4), (x + 5 * side, y - 12 * size)), 4.2 * size), stemc)]
    lf = leaf(x + 3 * side, y - 7 * size, 30 * size, 15 * size, ang if side > 0 else 180 - ang, 0.15)
    out += [(lf, leafc), (stroke([(x + 3 * side, y - 7 * size),
                                   (x + 3 * side + 24 * size * math.cos(math.radians(ang if side > 0 else 180 - ang)),
                                    y - 7 * size + 24 * size * math.sin(math.radians(ang if side > 0 else 180 - ang)))], 1.1),
                            mix(leafc, "#000000", 0.25))]
    return out


def citrus_slice(rind, pith, flesh, n=10, r=46):
    out = [(circle(0, 0, r), rind), (circle(0, 0, r * 0.88), pith)]
    for i in range(n):
        a0, a1 = 2 * math.pi * i / n, 2 * math.pi * (i + 1) / n
        g = 0.05
        pts = [(r * 0.09 * math.cos((a0 + a1) / 2), r * 0.09 * math.sin((a0 + a1) / 2))]
        for k in range(17):
            a = a0 + g + (a1 - a0 - 2 * g) * k / 16
            pts.append((r * 0.81 * math.cos(a), r * 0.81 * math.sin(a)))
        out.append((poly(pts), flesh))
        # brilho do gomo
        out.append((poly([(r * 0.3 * math.cos(a0 + 0.2), r * 0.3 * math.sin(a0 + 0.2)),
                          (r * 0.7 * math.cos(a0 + 0.17), r * 0.7 * math.sin(a0 + 0.17)),
                          (r * 0.7 * math.cos(a0 + 0.24), r * 0.7 * math.sin(a0 + 0.24))]), mix(flesh, "#FFFFFF", 0.45)))
    out.append((circle(0, 0, r * 0.06), pith))
    return out


def citrus_whole(base, dark, light, leafc="#3E8E2E", r=40, sq=1.0):
    out = roundfruit(base, dark, light, r, sq)
    rnd = random.Random(7)
    for _ in range(26):
        a, d = rnd.uniform(0, 2 * math.pi), rnd.uniform(0.15, 0.85) * r
        out.append((circle(d * math.cos(a), 4 + d * math.sin(a) * sq, 0.9), mix(base, dark, 0.5)))
    out += stem_leaf(0, 4 - r * sq + 2, leafc, "#4E3A20", 1, 0.9)
    return out


def lemon(base="#F4D23C", dark="#C9A419", light="#FFF3A6"):
    body = poly([(44 * math.cos(a) * (1 - 0.08 * math.cos(a) ** 2) + (6 * math.cos(a) ** 9),
                  30 * math.sin(a)) for a in (2 * math.pi * i / 120 for i in range(120))])
    body = affinity.rotate(body, -25, origin=(0, 0))
    out = [(body, base), (shade(body, -7, -6), dark), (hl(body, -12, -10, 0.32), light)]
    out += [(leaf(-28, -16, 30, 14, -120, 0.1), "#3E8E2E")]
    return out


def lime_slice():
    return citrus_slice("#4C8F1F", "#EEF3C6", "#A9CF45", 8)


def orange_slice():
    return citrus_slice("#E8730E", "#FBE7C4", "#F59A23", 10)


def grapefruit_slice():
    return citrus_slice("#F07A4A", "#FCE4D6", "#EE5A5A", 11)


def kiwi_slice():
    out = [(ellipse(0, 0, 46, 40), "#6B4A24"), (ellipse(0, 0, 42, 36), "#7DB52A"),
           (ellipse(0, 0, 32, 27), "#9BCB3C"), (ellipse(0, 0, 13, 10), "#F2F1D5")]
    for i in range(22):
        a = 2 * math.pi * i / 22
        out.append((affinity.rotate(ellipse(18 * math.cos(a), 15 * math.sin(a), 2.6, 1.3), math.degrees(a), origin=(18 * math.cos(a), 15 * math.sin(a))), "#2A1D10"))
    for i in range(16):
        a = 2 * math.pi * (i + 0.5) / 16
        out.append((stroke([(13 * math.cos(a), 10 * math.sin(a)), (40 * math.cos(a), 34 * math.sin(a))], 0.8), "#B6DA62"))
    return out


def starfruit_slice():
    st5 = star(0, 0, 46, 5, 0.58)
    out = [(st5.buffer(2), "#C99A12"), (st5, "#F4D23C"), (star(0, 0, 36, 5, 0.58), "#FBE46E")]
    for i in range(5):
        a = math.radians(-90 + i * 72)
        out.append((stroke([(0, 0), (30 * math.cos(a), 30 * math.sin(a))], 1.2), "#E7C02E"))
        out.append((ellipse(9 * math.cos(a), 9 * math.sin(a), 2.6, 1.5, math.degrees(a)), "#8A6A12"))
    return out


def cherries(base="#C3122B", dark="#7E0A1A", light="#FF7A86"):
    out = []
    out += [(stroke(curve((-14, 16), (-6, -20), (8, -40)), 3.2), "#4E6B1E"),
            (stroke(curve((16, 22), (14, -12), (8, -40)), 3.2), "#4E6B1E")]
    out += [(leaf(8, -40, 30, 14, -15, 0.15), "#3E8E2E")]
    for x, y, r in ((-18, 22, 21), (18, 28, 21)):
        b = circle(x, y, r)
        out += [(b, base), (shade(b, -6, -5), dark), (hl(b, -6, -7, 0.3), light)]
    return out


def apple(base="#D2222D", dark="#8E0F1A", light="#FF8A8A"):
    body = unary_union([circle(-13, 8, 30), circle(13, 8, 30), circle(0, 22, 26)]).buffer(2).buffer(-2)
    body = body.difference(ellipse(0, -24, 7, 6))
    out = [(body, base), (shade(body, -9, -7), dark), (hl(body, -10, -10, 0.28), light)]
    out += stem_leaf(0, -18, "#3E8E2E", "#5A3A1E", 1, 0.9)
    return out


def pear(base="#C6D44A", dark="#8E9C2A", light="#F2F7B0", spots=True):
    body = unary_union([circle(0, 18, 30), circle(0, -12, 18), poly([(-18, -12), (18, -12), (29, 14), (-29, 14)])])
    body = body.buffer(3).buffer(-3)
    out = [(body, base), (shade(body, -8, -6), dark), (hl(body, -8, -10, 0.3), light)]
    if spots:
        rnd = random.Random(3)
        for _ in range(18):
            x, y = rnd.uniform(-22, 22), rnd.uniform(-20, 40)
            if body.contains(Point(x, y)):
                out.append((circle(x, y, 0.9), mix(base, dark, 0.7)))
    out += stem_leaf(0, -28, "#3E8E2E", "#5A3A1E", 1, 0.8)
    return out


def banana():
    outer = curve((-46, -10), (0, 52), (46, -26), 60)
    inner = curve((40, -30), (2, 26), (-42, -18), 60)
    body = poly(outer + inner)
    out = [(body, "#F5D43A"), (poly(curve((-44, -12), (0, 40), (44, -28), 60) + inner), "#E0B51E"),
           (stroke(curve((-36, -14), (0, 28), (36, -26), 40), 1.4), "#C99A12"),
           (poly([(40, -30), (49, -34), (50, -27), (44, -24)]), "#5A4A1E"),
           (circle(-45, -11, 3), "#3A2A10")]
    return out


def grape_bunch(base="#7A3EA0", dark="#4C1E6A", light="#C7A2E2", leafc="#4E9A3A"):
    out = [(stroke(curve((0, -36), (2, -44), (8, -48)), 3), "#5A3A1E"),
           (leaf(4, -40, 34, 22, -10, 0.1), leafc)]
    rows = [(-34, 3, 13), (-17, 4, 12.5), (0, 3, 12.5), (17, 2, 12), (32, 1, 11)]
    for y, n, r in rows:
        for i in range(n):
            x = (i - (n - 1) / 2) * r * 1.65
            b = circle(x, y, r)
            out += [(b, base), (shade(b, -3.5, -3), dark), (hl(b, -3.5, -4, 0.3), light)]
    return out


def berry_cluster(base, dark, light, n=5, r=17, crown=True, leafc=None):
    pos = [(-18, 10), (18, 12), (0, -10), (-2, 26), (24, -12), (-24, -10)][:n]
    out = []
    if leafc:
        out += [(leaf(-6, -22, 36, 18, -140, 0.1), leafc), (leaf(6, -22, 36, 18, -40, 0.1), leafc)]
    for x, y in pos:
        b = circle(x, y, r)
        out += [(b, base), (shade(b, -4.5, -4), dark), (hl(b, -5, -5, 0.28), light)]
        if crown:
            out.append((star(x + r * 0.15, y + r * 0.05, r * 0.32, 5, 0.4), dark))
    return out


def hexfill(shape, step):
    """Pontos em colmeia dentro da forma."""
    minx, miny, maxx, maxy = shape.bounds
    pts, row, y = [], 0, miny + step / 2
    while y < maxy:
        x = minx + (step / 2 if row % 2 else 0)
        while x < maxx:
            if shape.contains(Point(x, y)):
                pts.append((x, y))
            x += step
        y += step * 0.866
        row += 1
    return pts


def drupe_berry(base, dark, light, leafc="#3E8E2E", shape="cone"):
    if shape == "cone":
        sil = unary_union([circle(0, -6, 28), poly([(-27, -2), (27, -2), (4, 42), (-4, 42)])]).buffer(4).buffer(-4)
        sil = affinity.scale(sil, 0.95, 1.0)
    else:
        sil = ellipse(0, 6, 20, 38)
    out = [(sil, dark)]
    mid = mix(base, dark, 0.45)
    for x, y in sorted(hexfill(sil.buffer(-3.0), 7.6), key=lambda p: (p[1], p[0])):
        b = circle(x, y, 4.9)
        out += [(b, base), (ring(b, 1.0).intersection(box(x - 6, y - 1, x + 6, y + 6)), mid), (circle(x - 1.5, y - 1.7, 1.0, 8), light)]
    top = sil.bounds[1]
    out += [(leaf(-2, top + 2, 24, 11, -160, 0.1), leafc), (leaf(2, top + 2, 24, 11, -20, 0.1), leafc),
            (leaf(0, top + 2, 20, 9, -90, 0.0), mix(leafc, "#000000", 0.15)), (stroke([(0, top + 2), (3, top - 12)], 3), "#4E3A20")]
    return out


def strawberry():
    pts = []
    for i in range(61):
        t = i / 60
        a = math.pi + t * math.pi
        pts.append((40 * math.cos(a), -16 + 16 * math.sin(a)))
    body = unary_union([poly(pts + [(0, 46)]), circle(-20, -10, 20), circle(20, -10, 20)]).buffer(5).buffer(-5)
    body = body.intersection(box(-60, -30, 60, 60))
    out = [(body, "#E2232E"), (shade(body, -8, -6), "#A3121D"), (hl(body, -9, -6, 0.3), "#FF7F86")]
    for x, y in hexfill(body.buffer(-3.5), 8.0):
        out.append((ellipse(x, y, 1.3, 2.0, 15), "#F6D35A"))
    for a in (-170, -135, -100, -80, -45, -10):
        out.append((leaf(0, -27, 24, 10, a, 0.0), "#3E9A2E"))
    out.append((stroke([(0, -28), (3, -42)], 3.6), "#4E7A1E"))
    return out


def pineapple():
    body = ellipse(0, 18, 28, 34)
    out = [(body, "#E7A92A"), (shade(body, -8, -5), "#B27416")]
    for i in range(-6, 7):
        out.append((stroke([(-40 + i * 9, -20), (40 + i * 9, 60)], 1.4), "#9A5F12"))
        out.append((stroke([(40 + i * 9, -20), (-40 + i * 9, 60)], 1.4), "#9A5F12"))
    out = [out[0], out[1]] + [(g.intersection(body), c) for g, c in out[2:]]
    for i in range(-3, 4):
        for j in range(-4, 5):
            x, y = i * 9 + (4.5 if j % 2 else 0), 18 + j * 8
            if body.buffer(-3).contains(Point(x, y)):
                out.append((star(x, y, 1.6, 4, 0.4), "#7A4A0E"))
    for a, l in ((-90, 40), (-120, 34), (-60, 34), (-145, 26), (-35, 26)):
        out.append((leaf(0, -14, l, 9, a, 0.05), "#3E8E2E"))
    out.append((leaf(0, -14, 30, 7, -100, 0.0), "#5DB04A"))
    return out


def avocado_half():
    body = unary_union([circle(0, 16, 30), circle(0, -14, 19)]).convex_hull.buffer(2).buffer(-2)
    out = [(body, "#2E4A16"), (body.buffer(-3.5), "#B9D46A"), (body.buffer(-9), "#E6EBA0"),
           (circle(0, 16, 14), "#7A4A22"), (shade(circle(0, 16, 14), -4, -4), "#5A3216"), (hl(circle(0, 16, 14), -4, -4, 0.3), "#B2805A")]
    return out


def papaya_half():
    body = ellipse(0, 0, 28, 46)
    out = [(body, "#5E8E2A"), (body.buffer(-3), "#F58A2E"), (body.buffer(-8), "#F7A04A"),
           (ellipse(0, 4, 11, 26), "#E66A1E")]
    rnd = random.Random(9)
    for _ in range(26):
        x, y = rnd.uniform(-9, 9), rnd.uniform(-18, 26)
        if ellipse(0, 4, 9, 23).contains(Point(x, y)):
            out.append((circle(x, y, 2.3), "#1E1410"))
    return out


def fig_half():
    body = unary_union([circle(0, 14, 32), circle(0, -18, 12), poly([(-12, -18), (12, -18), (30, 6), (-30, 6)])]).buffer(3).buffer(-3)
    out = [(body, "#4E1E4A"), (body.buffer(-3.5), "#E9D6B0"), (body.buffer(-9), "#C2384A")]
    rnd = random.Random(11)
    inner = body.buffer(-10)
    for _ in range(60):
        x, y = rnd.uniform(-26, 26), rnd.uniform(-20, 42)
        if inner.contains(Point(x, y)):
            out.append((ellipse(x, y, 0.9, 1.6, rnd.uniform(0, 180)), "#F4E4BE"))
    out.append((stroke([(0, -30), (1, -40)], 3.6), "#4E3A20"))
    return out


def peach(base="#F7A06A", dark="#E2564A", light="#FFE0BE"):
    body = unary_union([circle(-4, 6, 38), circle(6, 4, 36)])
    out = [(body, base), (shade(body, 14, 10).intersection(body), mix(base, dark, 0.7)),
           (shade(body, -10, -8), mix(base, dark, 0.35)), (hl(body, -12, -12, 0.3), light),
           (stroke(curve((2, -30), (-6, 6), (4, 40)), 1.6), mix(base, dark, 0.5))]
    out += stem_leaf(2, -30, "#4E9A3A", "#5A3A1E", 1, 0.9)
    return out


def pomegranate():
    body = circle(0, 8, 38)
    crown = poly([(-10, -26), (-14, -42), (-6, -34), (0, -44), (6, -34), (14, -42), (10, -26)])
    out = [(crown, "#8E1A26"), (body, "#B71C2E"), (shade(body, -9, -7), "#7A0E1C"), (hl(body, -10, -10, 0.28), "#FF8A8A")]
    # aberta: fatia com sementes
    cut = poly([(0, 8)] + [(38 * math.cos(a), 8 + 38 * math.sin(a)) for a in [math.radians(-20 + i * 3) for i in range(31)]])
    out.append((cut, "#F3D9C4"))
    rnd = random.Random(4)
    for _ in range(40):
        a, d = math.radians(rnd.uniform(-16, 66)), rnd.uniform(6, 33)
        x, y = d * math.cos(a), 8 + d * math.sin(a)
        out.append((ellipse(x, y, 2.6, 3.2, rnd.uniform(0, 180)), "#D4162E"))
        out.append((circle(x - 0.8, y - 1, 0.8), "#FF9AA6"))
    return out


def watermelon_slice():
    out = [(poly([(-46, -10)] + [(46 * math.cos(a), -10 + 46 * math.sin(a)) for a in [i * math.pi / 40 for i in range(41)]]), "#2E7A2A")]
    out.append((poly([(-41, -10)] + [(41 * math.cos(a), -10 + 41 * math.sin(a)) for a in [i * math.pi / 40 for i in range(41)]]), "#E8F2C8"))
    out.append((poly([(-37, -10)] + [(37 * math.cos(a), -10 + 37 * math.sin(a)) for a in [i * math.pi / 40 for i in range(41)]]), "#EE3A4A"))
    for x, y in ((-18, 4), (0, 10), (18, 4), (-9, 18), (9, 18)):
        out.append((ellipse(x, y, 2, 3.2), "#1E1410"))
    return out


def coconut_half():
    out = [(circle(0, 0, 44), "#5A3A1E"), (shade(circle(0, 0, 44), -8, -8), "#3E2610"),
           (circle(0, 0, 37), "#F8F4EA"), (circle(0, 2, 29), "#EAE4D4")]
    rnd = random.Random(2)
    for _ in range(40):
        a = rnd.uniform(0, 2 * math.pi)
        out.append((stroke([(41 * math.cos(a), 41 * math.sin(a)), (46 * math.cos(a + 0.03), 46 * math.sin(a + 0.03))], 1.0), "#7A5232"))
    return out


def olive_branch():
    out = [(stroke(curve((-44, 30), (0, 6), (40, -34)), 2.6), "#5A4A22")]
    for t, side in ((0.15, 1), (0.3, -1), (0.48, 1), (0.62, -1), (0.8, 1)):
        x, y = -44 + 84 * t, 30 - 64 * t + 10 * math.sin(math.pi * t)
        out.append((leaf(x, y, 24, 7, -40 + side * 55, 0.05), "#6E8A3A"))
    for x, y in ((-18, 26), (6, 2), (22, -4)):
        b = ellipse(x, y, 7, 9.5, -30)
        out += [(b, "#3E5A1A"), (hl(b, -2, -2, 0.35), "#8EAA4A")]
    return out


def pistachio():
    out = []
    for x, y, a in ((-14, 6, -20), (16, 2, 25)):
        shell = ellipse(x, y, 17, 25, a)
        nut = ellipse(x + 2 * math.cos(math.radians(a)), y, 11, 19, a)
        out += [(shell, "#E9D2A4"), (shade(shell, -4, -4), "#C4A270"), (nut, "#8CB43A"),
                (hl(nut, -2, -3, 0.35), "#C9E07A"), (stroke(curve((x - 10, y - 18), (x - 6, y), (x - 9, y + 18)), 1.2), "#7A4A5A")]
    return out


def blueberry():
    return berry_cluster("#3A5BB0", "#22337A", "#9AB4F0", 4, 19, True, "#4E9A3A")


def round_berry_branch(base, dark, light, n=7, r=10, leafc="#4E9A3A"):
    out = [(stroke(curve((-40, -30), (0, -28), (30, -40)), 2.4), "#4E3A20"),
           (leaf(20, -36, 30, 14, -20, 0.15), leafc)]
    rnd = random.Random(n)
    pts = [(-30, -12), (-14, -6), (2, -12), (-22, 10), (-4, 12), (12, 4), (-12, 28), (8, 26)][:n]
    for x, y in pts:
        out.append((stroke([(x, y - r), (x * 0.6 - 4, -30)], 1.0), "#4E3A20"))
    for x, y in pts:
        b = circle(x, y, r)
        out += [(b, base), (shade(b, -3, -2.5), dark), (hl(b, -3, -3, 0.32), light)]
    return out


def lychee():
    out = []
    for x, y, r in ((-14, 10, 25), (16, 4, 23)):
        b = circle(x, y, r)
        out += [(b, "#D2324A"), (shade(b, -6, -5), "#8E1A2A")]
        for i in range(30):
            a, d = i * 2.4, (i % 6 + 0.5) / 6 * r * 0.95
            out.append((star(x + d * math.cos(a), y + d * math.sin(a), 2.4, 4, 0.5, a * 30), "#A2203A"))
        out.append((hl(b, -6, -6, 0.3), "#FF8A9A"))
    # uma descascada
    out += [(ellipse(-2, 28, 15, 13), "#F7F4EE"), (hl(ellipse(-2, 28, 15, 13), -3, -3, 0.4), "#FFFFFF")]
    out += [(stroke(curve((16, -18), (18, -30), (26, -38)), 2.4), "#4E6B1E"), (leaf(24, -36, 22, 11, -20), "#3E8E2E")]
    return out


def mangosteen():
    body = circle(0, 8, 36)
    out = [(body, "#4E1A3E"), (shade(body, -9, -7), "#2E0A24"), (hl(body, -10, -10, 0.25), "#9A5A8A")]
    for i in range(5):
        a = math.radians(-90 + (i - 2) * 36)
        out.append((leaf(0, -24, 22, 13, math.degrees(a) + (180 if i < 2 else 0) * 0, 0.0).intersection(circle(0, -24, 24)), "#5E7A2A"))
    out.append((circle(0, -24, 7), "#4E6A22"))
    out.append((stroke([(0, -26), (0, -38)], 4), "#3E4A1A"))
    return out


def jackfruit():
    body = ellipse(0, 4, 30, 42, 10)
    out = [(body, "#A6B02E"), (shade(body, -7, -6), "#6E7A1A")]
    for i in range(-6, 7):
        for j in range(-8, 9):
            x, y = i * 6 + (3 if j % 2 else 0), 4 + j * 6
            if body.buffer(-2).contains(Point(x, y)):
                out.append((star(x, y, 2.2, 3, 0.4, -90 + j * 13), "#7E8A1E"))
    out.append((hl(body, -8, -12, 0.25), "#D8DE7A"))
    out.append((stroke([(-6, -38), (-10, -48)], 4), "#4E3A20"))
    return out


def tamarind():
    pts = curve((-44, 14), (0, -22), (44, 8), 60)
    lobes = [ellipse(pts[k][0], pts[k][1], 10.5, 8.5, math.degrees(math.atan2(pts[k + 1][1] - pts[k][1], pts[k + 1][0] - pts[k][0])))
             for k in (3, 14, 25, 36, 47)]
    pod = unary_union(lobes + [stroke(pts[3:48], 11)]).buffer(1).buffer(-1)
    out = [(pod, "#B08654"), (shade(pod, -2.5, -3), "#7A5430"), (hl(pod, -2, -3, 0.5), "#D8B482")]
    # pedaço aberto: polpa e sementes
    cut = ellipse(pts[52][0], pts[52][1], 10, 8, -20)
    out += [(cut, "#7A4A20"), (cut.buffer(-1.2), "#5A2E12"), (ellipse(pts[52][0] - 2, pts[52][1], 3.4, 2.4, -20), "#2E1A0A"),
            (ellipse(pts[52][0] + 4, pts[52][1] - 2, 2.6, 2.0, -20), "#2E1A0A")]
    out.append((stroke(curve((-44, 14), (-50, 14), (-52, 6)), 2.4), "#5A3A1E"))
    return out


def slice_icon():
    """Fatia cítrica vazada (para o padrão de fundo)."""
    r = 46
    parts = [ring(circle(0, 0, r), 6)]
    for i in range(8):
        a0, a1 = 2 * math.pi * i / 8 + 0.09, 2 * math.pi * (i + 1) / 8 - 0.09
        pts = [(5 * math.cos((a0 + a1) / 2), 5 * math.sin((a0 + a1) / 2))]
        pts += [(36 * math.cos(a0 + (a1 - a0) * k / 12), 36 * math.sin(a0 + (a1 - a0) * k / 12)) for k in range(13)]
        parts.append(poly(pts))
    return [(unary_union(parts), "#000000")]


def cashew():
    fruit = unary_union([ellipse(0, -8, 24, 30), circle(0, 8, 23)]).buffer(2).buffer(-2)
    out = [(fruit, "#F2532A"), (shade(fruit, -7, -6), "#C2301E"), (fruit.intersection(box(-40, -50, 40, -24)), "#F8B42A"),
           (hl(fruit, -8, -9, 0.3), "#FFC27A")]
    nut = unary_union([circle(-6, 40, 9), circle(7, 38, 8), ellipse(0, 36, 13, 7)]).difference(circle(0, 47, 4.5)).buffer(1.2)
    out += [(nut, "#8E9A5A"), (shade(nut, -2, -2), "#5E6A32"), (hl(nut, -2, -2, 0.4), "#C2CE8E")]
    out.append((stroke([(0, -36), (2, -46)], 3.2), "#4E3A20"))
    return out


def persimmon():
    body = affinity.scale(circle(0, 6, 38), 1.0, 0.85)
    out = [(body, "#F2731E"), (shade(body, -9, -7), "#C24A10"), (hl(body, -10, -9, 0.28), "#FFC07A")]
    for i in range(4):
        out.append((leaf(0, -22, 20, 12, -90 + 90 * i + 45, 0.0), "#4E7A2A"))
    out.append((circle(0, -22, 5), "#3E5A1E"))
    out.append((stroke([(0, -24), (0, -34)], 3.2), "#4E3A20"))
    return out


def guava_half():
    body = ellipse(0, 0, 40, 36)
    out = [(body, "#8EBA3A"), (body.buffer(-3.5), "#F7A0A0"), (body.buffer(-12), "#F27888")]
    rnd = random.Random(6)
    for _ in range(26):
        a, d = rnd.uniform(0, 2 * math.pi), rnd.uniform(4, 16)
        out.append((circle(d * math.cos(a), d * math.sin(a) * 0.9, 1.4), "#F8E2A6"))
    return out


def honeydew():
    body = ellipse(0, 4, 42, 36)
    out = [(body, "#D6E6A2"), (shade(body, -9, -7), "#A6BA6A")]
    for i in range(-5, 6):
        out.append((stroke(curve((i * 8, -32), (i * 11, 4), (i * 8, 40)), 0.9).intersection(body), "#B8CC80"))
    out.append((hl(body, -12, -11, 0.28), "#F2FAD2"))
    # fatia na frente
    sl = poly([(-6, 40), (40, 8), (44, 14), (0, 46)]).buffer(2)
    out += [(sl, "#7A9A3A"), (sl.buffer(-1.6), "#B6E07A")]
    return out


def acerola(base="#D61E1E", dark="#8E0E10", light="#FF8A7A"):
    out = []
    for x, y, r in ((-16, 12, 22), (18, 6, 20)):
        b = affinity.scale(circle(x, y, r), 1.0, 0.88)
        out += [(b, base), (shade(b, -5, -4), dark), (hl(b, -6, -6, 0.3), light)]
        for k in (-0.45, 0, 0.45):
            out.append((stroke(curve((x + k * r, y - r * 0.85), (x + k * r * 1.5, y), (x + k * r, y + r * 0.85)), 0.9), mix(base, dark, 0.6)))
    out += [(stroke(curve((-14, -8), (-2, -26), (10, -34)), 2.2), "#4E6B1E"),
            (stroke(curve((18, -12), (14, -26), (10, -34)), 2.2), "#4E6B1E"), (leaf(10, -34, 26, 12, -30), "#3E8E2E")]
    return out


def graviola():
    body = unary_union([circle(0, 12, 30), circle(-4, -14, 20)]).convex_hull.buffer(4).buffer(-4)
    body = affinity.rotate(body, 18, origin=(0, 0))
    out = [(body, "#3E7A2A"), (shade(body, -7, -6), "#245216")]
    for i in range(-6, 7):
        for j in range(-8, 9):
            x, y = i * 7 + (3.5 if j % 2 else 0), j * 6.5
            if body.buffer(-2).contains(Point(x, y)):
                out.append((poly([(x - 1.6, y + 1.6), (x + 2.6, y - 2.2), (x + 0.6, y + 1.8)]), "#9AC25A"))
    out.append((hl(body, -8, -10, 0.25), "#7AB04A"))
    return out


def cupuacu():
    body = ellipse(0, 6, 30, 40, -12)
    out = [(body, "#7A4A24"), (shade(body, -7, -6), "#4E2A10"), (hl(body, -8, -10, 0.25), "#B27A4A")]
    cut = ellipse(18, 22, 18, 22, -12)
    out += [(cut, "#5A3216"), (cut.buffer(-3), "#F6EED2")]
    for x, y in ((14, 16), (22, 26), (16, 30)):
        out.append((ellipse(x, y, 3.6, 2.6, 30), "#8A5A30"))
    return out


def pitanga(base="#D9241E", dark="#9A1210", light="#FF9A7A"):
    """Pitanga: frutinha em gomos (como uma abóbora pequena), vista de lado."""
    out = []
    for x, y, r in ((-15, 10, 23), (18, 14, 19)):
        body = ellipse(x, y, r, r * 0.8)
        out.append((body, dark))
        for k, (dx, w) in enumerate(((-0.66, 0.36), (0.66, 0.36), (-0.33, 0.36), (0.33, 0.36), (0.0, 0.36))):
            lobe = ellipse(x + dx * r, y, w * r, r * 0.76).intersection(body).buffer(-0.7)
            out.append((lobe, mix(base, dark, 0.5) if k < 2 else (mix(base, dark, 0.2) if k < 4 else base)))
        out.append((hl(body, -r * 0.25, -r * 0.3, 0.3), light))
        out.append((star(x, y - r * 0.78, r * 0.22, 5, 0.45), "#4E6B1E"))
    out += [(leaf(-8, -14, 32, 13, -150, 0.1), "#3E8E2E"), (leaf(-6, -14, 28, 11, -40, 0.1), "#4E9A3A")]
    return out


def quince():
    body = unary_union([circle(0, 14, 32), circle(-2, -16, 20)]).buffer(4).buffer(-4)
    out = [(body, "#EAC83A"), (shade(body, -8, -6), "#B8941E"), (hl(body, -9, -10, 0.28), "#FFF0A2")]
    out += stem_leaf(-2, -34, "#4E8A3A", "#5A3A1E", 1, 0.9)
    return out


def apricot():
    return peach("#F59A2E", "#D9622A", "#FFD99A")


def nectarine():
    return peach("#E9452E", "#9E1A1A", "#FFB07A")


def kumquat():
    out = []
    for x, y, r, a in ((-14, 8, 20, -20), (16, 14, 18, 25)):
        b = ellipse(x, y, r * 0.78, r, a)
        out += [(b, "#F59A12"), (shade(b, -5, -4), "#C2620A"), (hl(b, -5, -6, 0.3), "#FFD27A")]
    out += [(stroke(curve((-12, -12), (0, -26), (8, -34)), 2.2), "#4E6B1E"), (leaf(8, -34, 26, 12, -25), "#3E8E2E")]
    return out


def clementine():
    return citrus_whole("#F27A12", "#C2500A", "#FFC27A", "#3E8E2E", 38, 0.9)


def orange_whole():
    return citrus_whole("#F28C12", "#C25E0A", "#FFD08A", "#3E8E2E", 40, 1.0)


def snow_pear():
    return pear("#EEF0DC", "#C2C6A6", "#FFFFFF", True)


def gooseberry():
    out = []
    for x, y, r in ((-14, 8, 22), (16, 14, 19)):
        b = ellipse(x, y, r * 0.9, r)
        out += [(b, "#9CC24A"), (shade(b, -5, -4), "#6E8E2A")]
        for k in (-0.5, -0.17, 0.17, 0.5):
            out.append((stroke(curve((x + k * r * 0.6, y - r * 0.9), (x + k * r * 1.5, y), (x + k * r * 0.6, y + r * 0.9)), 0.8), "#D6EA9A"))
        out.append((hl(b, -5, -6, 0.3), "#E6F6B2"))
    out += [(stroke(curve((-14, -14), (0, -30), (10, -36)), 2), "#5A4A22"), (leaf(10, -36, 26, 13, -20), "#3E8E2E")]
    return out


def banana_bunch():
    out = []
    for a in (-14, 0, 14):
        out += rot(banana(), a, origin=(46, -30))
    return out


def acai():
    return round_berry_branch("#3A1236", "#1E0618", "#8A5A86", 8, 9)


def jabuticaba():
    out = [(stroke(curve((-46, 20), (0, -6), (46, -18)), 9), "#6A5236"), (stroke(curve((-46, 20), (0, -6), (46, -18)), 3), "#8A7050")]
    for x, y, r in ((-30, 8, 10), (-12, -2, 11), (8, -10, 10.5), (26, -16, 9.5), (-20, 22, 10), (2, 12, 11), (20, 2, 10)):
        b = circle(x, y, r)
        out += [(b, "#1E0E1A"), (hl(b, -3, -3.5, 0.32), "#6A4A66")]
    return out


def blackcurrant():
    return round_berry_branch("#2A0E2E", "#14041A", "#7A5A86", 7, 10)


def elderberry():
    dome = unary_union([ellipse(0, 6, 40, 24), box(-40, 6, 40, 14)]).intersection(ellipse(0, 6, 42, 28))
    out = []
    for x in (-30, -16, 0, 16, 30):
        out.append((stroke(curve((0, 44), (x * 0.4, 30), (x, 12)), 1.3), "#8A2A4A"))
    out.append((stroke([(0, 44), (0, 50)], 2.4), "#6A2A3A"))
    for x, y in hexfill(dome, 7.0):
        b = circle(x, y, 4.1)
        out += [(b, "#2A0E2E"), (ring(b, 0.9).intersection(box(x - 5, y - 1, x + 5, y + 5)), "#14041A"), (circle(x - 1.2, y - 1.3, 0.9, 8), "#9A6AAA")]
    out += [(leaf(-6, 40, 30, 11, 160, 0.1), "#3E7A2A"), (leaf(6, 40, 30, 11, 20, 0.1), "#4E8A3A")]
    return out


def mulberry():
    return drupe_berry("#3E0E2E", "#22041A", "#9A4A7A", "#4E9A3A", "long")


def raspberry():
    return drupe_berry("#E2245A", "#A2103A", "#FF8AAA", "#3E8E2E", "cone")


def cranberry():
    out = []
    for x, y, r in ((-18, 12, 15), (6, 18, 14), (22, -2, 13), (-4, -8, 14), (-24, -14, 11)):
        b = ellipse(x, y, r, r * 0.86, 20)
        out += [(b, "#B8102E"), (shade(b, -4, -3), "#7A0418"), (hl(b, -4, -4, 0.3), "#FF7A8A")]
    out += [(leaf(10, -24, 26, 10, -40), "#4E7A2A"), (leaf(-6, -26, 22, 9, -130), "#3E6A22")]
    return out


def strawberry_simple():
    return strawberry()


def gold_laurel(ink, gold):
    """Coroa de louros dourada com arco e flecha (o centauro arqueiro)."""
    out = []
    for side in (-1, 1):
        stem = curve((0, 44), (side * 52, 14), (side * 26, -40), 50)
        out.append((stroke(stem, 1.8), ink))
        for i in range(3, 48, 5):
            x, y = stem[i]
            nx, ny = stem[i + 1][0] - x, stem[i + 1][1] - y
            ang = math.degrees(math.atan2(ny, nx))
            out.append((leaf(x, y, 13, 5.5, ang - side * 35, 0.0), gold))
            out.append((leaf(x, y, 13, 5.5, ang + side * 35, 0.0), mix(gold, ink, 0.25)))
    # símbolo de Sagitário (o centauro arqueiro): flecha com a barra
    out += [(stroke([(-16, 24), (18, -10)], 4.2), ink),
            (poly([(24, -16), (6, -14), (22, 0)]), ink),
            (stroke([(-12, -2), (6, 16)], 4.2), ink)]
    return out


def juniper():
    """Raminho de zimbro: agulhas finas e as frutinhas azuladas com pó claro."""
    out = [(stroke(curve((-44, 30), (-4, 8), (40, -30)), 2.4), "#5A4A2A")]
    for i in range(14):
        t = i / 13
        x, y = -44 + 84 * t, 30 - 60 * t + 8 * math.sin(math.pi * t)
        for side in (-1, 1):
            out.append((leaf(x, y, 14, 2.6, -40 + side * 60 - t * 10, 0.0), "#3E6A4A" if side < 0 else "#4E7E5A"))
    for x, y, r in ((-20, 18, 9), (-4, 24, 8.5), (6, 6, 9.5), (20, 14, 8), (16, -6, 8.5), (-12, 2, 8)):
        b = circle(x, y, r)
        out += [(b, "#3E4E86"), (shade(b, -2.4, -2), "#28325E"), (hl(b, -2.8, -3, 0.45), "#A8B6DE"),
                (star(x + r * 0.2, y + r * 0.1, r * 0.22, 3, 0.4, 90), "#28325E")]
    return out


def blueberry_chain_art():
    """Três mirtilos ligados por um cabinho, como uma corrente."""
    out = [(stroke(curve((-44, 4), (-22, -22), (0, 4)), 2.2), "#4E6B1E"),
           (stroke(curve((0, 4), (22, -22), (44, 4)), 2.2), "#4E6B1E"),
           (leaf(-22, -14, 22, 10, -150, 0.1), "#3E8E2E"), (leaf(22, -14, 22, 10, -30, 0.1), "#4E9A3A")]
    for x in (-34, 0, 34):
        b = circle(x, 18, 15)
        out += [(b, "#3A5BB0"), (shade(b, -4, -3.5), "#22337A"), (hl(b, -4.5, -5, 0.3), "#9AB4F0"),
                (star(x + 2, 19, 4.6, 5, 0.4), "#22337A")]
    return out


def galaxy(core="#FFF2FA", inner="#F2A6D8", outer="#8A5AE0", dust="#4A2A8A"):
    """Galáxia em espiral: dois braços de estrelas e o miolo brilhante."""
    out = [(ellipse(0, 0, 46, 26, -20), dust)]
    rnd = random.Random(21)
    for arm in (0, math.pi):
        for i in range(70):
            t = i / 69
            a = arm + t * 3.6
            r = 6 + 40 * t
            x, y = r * math.cos(a), r * math.sin(a) * 0.55
            x, y = x * math.cos(-0.35) - y * math.sin(-0.35), x * math.sin(-0.35) + y * math.cos(-0.35)
            size = (3.4 - 2.4 * t) * rnd.uniform(0.6, 1.1)
            out.append((circle(x + rnd.uniform(-1.5, 1.5), y + rnd.uniform(-1.5, 1.5), size, 10), outer if t > 0.55 else inner))
    out += [(ellipse(0, 0, 11, 7, -20), inner), (ellipse(0, 0, 6, 4, -20), core)]
    for x, y, r in ((-30, -20, 2.6), (34, 18, 2.0), (26, -24, 1.6)):
        out.append((star(x, y, r, 4, 0.2), "#FFFFFF"))
    return out


FRUTAS = {
    "lemon": lemon, "lime_slice": lime_slice, "orange_slice": orange_slice, "orange": orange_whole,
    "grapefruit": grapefruit_slice, "kiwi": kiwi_slice, "starfruit": starfruit_slice, "cherries": cherries,
    "apple": apple, "pear": pear, "snow_pear": snow_pear, "banana": banana, "grapes": grape_bunch,
    "blueberry": blueberry, "strawberry": strawberry, "pineapple": pineapple, "avocado": avocado_half,
    "papaya": papaya_half, "fig": fig_half, "peach": peach, "apricot": apricot, "nectarine": nectarine,
    "pomegranate": pomegranate, "watermelon": watermelon_slice, "coconut": coconut_half, "olive": olive_branch,
    "pistachio": pistachio, "lychee": lychee, "mangosteen": mangosteen, "jackfruit": jackfruit,
    "tamarind": tamarind, "cashew": cashew, "persimmon": persimmon, "guava": guava_half, "honeydew": honeydew,
    "acerola": acerola, "graviola": graviola, "cupuacu": cupuacu, "pitanga": pitanga, "quince": quince,
    "kumquat": kumquat, "clementine": clementine, "gooseberry": gooseberry, "acai": acai,
    "jabuticaba": jabuticaba, "blackcurrant": blackcurrant, "elderberry": elderberry, "mulberry": mulberry,
    "raspberry": raspberry, "cranberry": cranberry,
}


# ------------------------------------------------------------------ astros
def planet(r, base, bands, dark, ring_col=None, ring_tilt=-14, spot=None, light=None):
    """Planeta de 100 unidades: disco com faixas, sombra e anel opcional."""
    body = circle(0, 0, r, 64)
    out = []
    if ring_col:
        rg = ellipse(0, 0, r * 1.9, r * 0.42, ring_tilt).difference(ellipse(0, 0, r * 1.45, r * 0.27, ring_tilt))
        back = rg.intersection(affinity.rotate(box(-200, -200, 200, 0), ring_tilt, origin=(0, 0)))
        out.append((back, ring_col))
    out.append((body, base))
    for y0, y1, c in bands:
        out.append((box(-r, y0 * r, r, y1 * r).intersection(body), c))
    if spot:
        out.append((ellipse(spot[0] * r, spot[1] * r, spot[2] * r, spot[3] * r, -6), spot[4]))
    out.append((shade(body, -r * 0.3, -r * 0.08), dark))
    if light:
        out.append((hl(body, -r * 0.35, -r * 0.35, 0.25), light))
    if ring_col:
        front = rg.difference(affinity.rotate(box(-200, -200, 200, 0), ring_tilt, origin=(0, 0)))
        out.append((front, ring_col))
    return out


def moon(r, base, crater, dark):
    body = circle(0, 0, r, 64)
    out = [(body, base)]
    rnd = random.Random(12)
    for x, y, rr in ((-0.3, -0.25, 0.22), (0.25, 0.1, 0.16), (-0.1, 0.38, 0.13), (0.38, -0.32, 0.1), (-0.48, 0.15, 0.09), (0.05, -0.05, 0.07)):
        c = circle(x * r, y * r, rr * r)
        out += [(c, crater), (shade(c, rr * r * 0.25, rr * r * 0.25).intersection(c), mix(crater, dark, 0.4))]
    out.append((shade(body, -r * 0.32, -r * 0.1), dark))
    return out


def phase(r, t, lit, dark):
    """Fase da Lua: t de 0 (nova) a 1 (cheia) e de volta, como disco iluminado."""
    body = circle(0, 0, r, 48)
    if t >= 0.999:
        return [(body, lit)]
    k = math.cos(math.pi * t)  # 1 nova, -1 cheia
    term = ellipse(0, 0, abs(k) * r + 0.01, r)
    right = box(0, -r - 1, r + 1, r + 1).intersection(body)
    litg = right.difference(term) if k > 0 else right.union(term.intersection(body))
    return [(body, dark), (litg, lit)]


def comet(color, tail, core="#FFFFFF"):
    out = []
    for w, c, l in ((22, mix(tail, "#000000", 0.35), 96), (15, tail, 92), (8, mix(tail, "#FFFFFF", 0.4), 86)):
        out.append((poly([(30, -w / 2 * 0.6), (30 - l, -w * 0.15), (30 - l, w * 0.15), (30, w / 2 * 0.6)]).union(circle(30, 0, w / 2 * 0.62)), c))
    out += [(circle(30, 0, 9), color), (circle(30, 0, 5), core)]
    return out


def pulsar(core, beam, rings):
    out = []
    for i, c in enumerate(rings):
        r = 16 + i * 9
        out.append((ring(circle(0, 0, r, 64), 1.4), c))
    out.append((poly([(-4, 0), (0, -48), (4, 0), (0, 48)]), beam))
    out.append((affinity.rotate(poly([(-2.4, 0), (0, -30), (2.4, 0), (0, 30)]), 90, origin=(0, 0)), mix(beam, "#FFFFFF", 0.3)))
    out += [(circle(0, 0, 9), mix(core, beam, 0.3)), (circle(0, 0, 5.5), core)]
    return out


def eclipse(corona, inner, disk):
    out = []
    rnd = random.Random(3)
    rays = []
    for i in range(36):
        a = math.radians(i * 10 + rnd.uniform(-3, 3))
        l = rnd.uniform(38, 50) if i % 2 else rnd.uniform(30, 40)
        rays.append(poly([(26 * math.cos(a - 0.08), 26 * math.sin(a - 0.08)), (l * math.cos(a), l * math.sin(a)),
                          (26 * math.cos(a + 0.08), 26 * math.sin(a + 0.08))]))
    out.append((unary_union(rays), corona))
    out.append((circle(0, 0, 30, 64), inner))
    out.append((circle(0, 0, 27, 64), disk))
    out.append((ring(circle(0, 0, 27.6, 64), 0.8), mix(inner, "#FFFFFF", 0.4)))
    out.append((circle(17, -18, 2.2), "#FFFFFF"))
    return out


def nebula(colors, seed=4):
    rnd = random.Random(seed)
    out = []
    for i, c in enumerate(colors):
        parts = []
        for _ in range(7):
            a = rnd.uniform(0, 2 * math.pi)
            d = rnd.uniform(0, 26 - i * 4)
            parts.append(ellipse(d * math.cos(a) * 1.3, d * math.sin(a) * 0.7, rnd.uniform(10, 22) - i * 2, rnd.uniform(6, 12) - i,
                                 rnd.uniform(-30, 30)))
        out.append((unary_union(parts).buffer(3).buffer(-1), c))
    # estrelas novas no meio da nuvem
    for x, y, r in ((-14, -4, 4.5), (10, 6, 3.2), (24, -8, 2.4), (-26, 8, 2.0)):
        out.append((star(x, y, r, 4, 0.2), "#FFFFFF"))
    return out


def stars_field(area, color, n, seed, rmin=0.5, rmax=1.8, avoid=None):
    rnd = random.Random(seed)
    out = []
    minx, miny, maxx, maxy = area.bounds
    tries = 0
    placed = []
    while len(out) < n and tries < n * 40:
        tries += 1
        x, y = rnd.uniform(minx, maxx), rnd.uniform(miny, maxy)
        if not area.contains(Point(x, y)):
            continue
        if avoid is not None and avoid.buffer(1.2).contains(Point(x, y)):
            continue
        if any((x - a) ** 2 + (y - b) ** 2 < 36 for a, b in placed):
            continue
        placed.append((x, y))
        r = rnd.uniform(rmin, rmax)
        out.append((star(x, y, r, 4, 0.22) if r > 1.3 else circle(x, y, r * 0.5, 8), color))
    return out


# ------------------------------------------------------------------ padrões de fundo
def pattern_scatter(area, motif, size, color, step, seed=1, jitter=0.25, rotate=True):
    """Motivo repetido em grade de linhas desencontradas (tom sobre tom)."""
    rnd = random.Random(seed)
    minx, miny, maxx, maxy = area.bounds
    out = []
    y = miny + step / 2
    row = 0
    while y < maxy + step:
        x = minx + (step / 2 if row % 2 else 0)
        while x < maxx + step:
            px, py = x + rnd.uniform(-jitter, jitter) * step, y + rnd.uniform(-jitter, jitter) * step
            g = place([(silhouette(motif), color)], px, py, size, rnd.uniform(0, 360) if rotate else 0)[0][0]
            g = g.intersection(area)
            if not g.is_empty:
                out.append((g, color))
            x += step
        y += step * 0.87
        row += 1
    return out


def pattern_stripes(area, color, width, gap, angle=35):
    minx, miny, maxx, maxy = area.bounds
    d = math.hypot(maxx - minx, maxy - miny)
    cx, cy = (minx + maxx) / 2, (miny + maxy) / 2
    bands = []
    x = -d
    while x < d:
        bands.append(box(cx + x, cy - d, cx + x + width, cy + d))
        x += width + gap
    g = affinity.rotate(unary_union(bands), angle, origin=(cx, cy)).intersection(area)
    return [(g, color)]


def pattern_rays(area, color, cx, cy, n=24):
    d = 200
    rays = []
    for i in range(n):
        a0 = 2 * math.pi * i / n
        a1 = a0 + math.pi / n
        rays.append(poly([(cx, cy), (cx + d * math.cos(a0), cy + d * math.sin(a0)), (cx + d * math.cos(a1), cy + d * math.sin(a1))]))
    return [(unary_union(rays).intersection(area), color)]


def pattern_dots(area, color, step, rmax, cx, cy):
    """Meio-tom: pontos que crescem para longe do centro."""
    minx, miny, maxx, maxy = area.bounds
    out = []
    y = miny
    row = 0
    dmax = math.hypot(maxx - minx, maxy - miny) / 2
    while y < maxy + step:
        x = minx + (step / 2 if row % 2 else 0)
        while x < maxx + step:
            r = rmax * min(1.0, math.hypot(x - cx, y - cy) / dmax) ** 1.2
            if r > 0.15:
                g = circle(x, y, r, 8).intersection(area)
                if not g.is_empty:
                    out.append((g, color))
            x += step
        y += step * 0.87
        row += 1
    return [(unary_union([g for g, _ in out]), color)] if out else []


def pattern_waves(area, color, amp, lam, step, width):
    minx, miny, maxx, maxy = area.bounds
    lines = []
    y = miny
    while y < maxy + step:
        pts = [(x, y + amp * math.sin(2 * math.pi * x / lam)) for x in [minx - 2 + i * 0.8 for i in range(int((maxx - minx + 4) / 0.8) + 2)]]
        lines.append(stroke(pts, width))
        y += step
    return [(unary_union(lines).intersection(area), color)]


def frame(area_box, inset, w, color, double=True, corner=True):
    minx, miny, maxx, maxy = area_box
    out = []
    r = box(minx + inset, miny + inset, maxx - inset, maxy - inset)
    out.append((ring(r, w), color))
    if double:
        r2 = r.buffer(-w - 0.9, join_style=2)
        out.append((ring(r2, w * 0.45), color))
    if corner:
        for x, y in ((minx + inset, miny + inset), (maxx - inset, miny + inset), (minx + inset, maxy - inset), (maxx - inset, maxy - inset)):
            out.append((affinity.rotate(box(x - 2, y - 2, x + 2, y + 2), 45, origin=(x, y)), color))
    return out


# ------------------------------------------------------------------ SVG
def path_of(g):
    def ring_d(coords):
        pts = list(coords)
        return "M " + " L ".join("%.3f %.3f" % (x, y) for x, y in pts[:-1]) + " Z"
    polys = [g] if isinstance(g, Polygon) else [p for p in getattr(g, "geoms", []) if isinstance(p, Polygon)]
    d = []
    for p in polys:
        if p.is_empty or p.area < 0.01:
            continue
        d.append(ring_d(p.exterior.coords))
        d += [ring_d(i.coords) for i in p.interiors]
    return " ".join(d)


def save_svg_plain(path, shapes):
    """Desenho solto (logo), enquadrado pelo próprio contorno."""
    body = []
    for g, c in shapes:
        d = path_of(g.buffer(0)) if g is not None and not g.is_empty else ""
        if d:
            body.append('  <path d="%s" fill="%s"/>' % (d, c))
    minx, miny, maxx, maxy = unary_union([g for g, _ in shapes]).bounds
    with open(path, "w") as f:
        f.write('<svg xmlns="http://www.w3.org/2000/svg" viewBox="%g %g %g %g">\n%s\n</svg>\n'
                % (minx, miny, maxx - minx, maxy - miny, "\n".join(body)))


def save_svg(path, w, h, shapes, split_y=None):
    """Estampa do tampo. split_y: no pedal compacto, separa o que fica na base
    (acima da linha) do que fica no pedal basculante (abaixo), em formas
    diferentes, para o pedal.py pôr cada parte na sua altura."""
    if split_y is not None:
        top, bot = box(-1, -1, w + 1, split_y), box(-1, split_y, w + 1, h + 1)
        shapes = [(g.intersection(part), c) for g, c in shapes for part in (top, bot)]
    body = ['  <path d="M 0 0 L 0.4 0 L 0.4 0.4 L 0 0.4 Z" fill="%s"/>' % MARCA,
            '  <path d="M %g %g L %g %g L %g %g L %g %g Z" fill="%s"/>' % (w - 0.4, h - 0.4, w, h - 0.4, w, h, w - 0.4, h, MARCA)]
    for g, c in shapes:
        if g is None or g.is_empty:
            continue
        g = g.buffer(0)
        # GeometryCollection: só os polígonos
        if g.geom_type == "GeometryCollection":
            g = unary_union([p for p in g.geoms if p.geom_type in ("Polygon", "MultiPolygon")])
        d = path_of(g)
        if d:
            body.append('  <path d="%s" fill="%s"/>' % (d, c))
    with open(path, "w") as f:
        f.write('<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 %g %g" width="%gmm" height="%gmm">\n%s\n</svg>\n'
                % (w, h, w, h, "\n".join(body)))
