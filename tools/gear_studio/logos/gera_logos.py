# Gera os desenhos vetoriais dos pedais (logos e estampas do tampo inteiro).
# As estampas são desenhadas em milímetros, do tamanho do tampo, com os cantos
# marcados em #FF00FF (o pedal.py usa as marcas para enquadrar e não as pinta).
# Cada forma é um polígono preenchido; a ordem no arquivo é a ordem das camadas.
import math
import os

AQUI = os.path.dirname(os.path.abspath(__file__))
MARCA = "#FF00FF"


def poly(pts):
    return "M " + " L ".join("%.3f %.3f" % p for p in pts) + " Z"


def circle(cx, cy, r, n=160):
    return [(cx + r * math.cos(2 * math.pi * i / n), cy + r * math.sin(2 * math.pi * i / n)) for i in range(n)]


def ellipse(cx, cy, rx, ry, rot, a0=0.0, a1=360.0, n=200):
    t = math.radians(rot)
    out = []
    for i in range(n + 1):
        a = math.radians(a0 + (a1 - a0) * i / n)
        x, y = rx * math.cos(a), ry * math.sin(a)
        out.append((cx + x * math.cos(t) - y * math.sin(t), cy + x * math.sin(t) + y * math.cos(t)))
    return out


def ring(cx, cy, rx, ry, w, rot, a0=0.0, a1=360.0):
    """Faixa elíptica (anel) de largura w, do ângulo a0 ao a1."""
    k = ry / rx
    return poly(ellipse(cx, cy, rx, ry, rot, a0, a1) + ellipse(cx, cy, rx - w, ry - w * k, rot, a1, a0))


def star(cx, cy, r, thin=0.22):
    pts = []
    for i in range(8):
        a = math.radians(i * 45 - 90)
        rr = r if i % 2 == 0 else r * thin
        pts.append((cx + rr * math.cos(a), cy + rr * math.sin(a)))
    return poly(pts)


def chord_band(cx, cy, r, y0, y1, n=24):
    """Faixa horizontal recortada no círculo (as listras do planeta)."""
    y0, y1 = max(y0, cy - r + 0.01), min(y1, cy + r - 0.01)
    ys = [y0 + (y1 - y0) * i / n for i in range(n + 1)]
    left = [(cx - math.sqrt(r * r - (y - cy) ** 2), y) for y in ys]
    right = [(2 * cx - x, y) for x, y in reversed(left)]
    return poly(left + right)


def crescent(cx, cy, r, shift, n=360):
    """Sombra em meia-lua: a parte do disco que fica fora do disco deslocado
    para a esquerda (luz vindo da esquerda)."""
    inside = lambda x, y, ox: (x - ox) ** 2 + (y - cy) ** 2 < r * r
    ang = lambda p: math.atan2(p[1] - cy, p[0] - cx)
    outer = [p for p in circle(cx, cy, r, n) if not inside(p[0], p[1], cx - shift)]
    inner = [p for p in circle(cx - shift, cy, r * 1.02, n) if inside(p[0], p[1], cx)]
    outer.sort(key=ang)
    inner.sort(key=ang, reverse=True)
    return poly(outer + inner)


def wedges(cx, cy, r, n=8, gap=5.0):
    """Gomos de uma fatia de fruta cítrica."""
    out = []
    for i in range(n):
        a0, a1 = i * 360 / n + gap, (i + 1) * 360 / n - gap
        hub = [(cx + r * 0.16 * math.cos(math.radians(a)), cy + r * 0.16 * math.sin(math.radians(a))) for a in (a0 + gap, a1 - gap)]
        arc = [(cx + r * math.cos(math.radians(a0 + (a1 - a0) * k / 16)), cy + r * math.sin(math.radians(a0 + (a1 - a0) * k / 16)))
               for k in range(17)]
        out.append(poly([hub[0]] + arc + [hub[1]]))
    return out


def marks(w, h):
    s = 0.4
    return [(poly([(0, 0), (s, 0), (s, s), (0, s)]), MARCA), (poly([(w - s, h - s), (w, h - s), (w, h), (w - s, h)]), MARCA)]


def save(name, w, h, shapes):
    body = "\n".join('  <path d="%s" fill="%s"/>' % (d, c) for d, c in shapes)
    with open(os.path.join(AQUI, name), "w") as f:
        f.write('<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 %g %g" width="%gmm" height="%gmm">\n%s\n</svg>\n'
                % (w, h, w, h, body))


# ---------------------------------------------------------------- Neptune Chorus
# Tampo 94 x 120 mm (y para baixo). Botões em y 27, nome em y 82, pisante em y 100.
def neptune():
    W, H = 94.0, 120.0
    sh = marks(W, H)
    STAR = "#DCE7FF"
    for x, y, r in ((9, 9, 2.6), (84, 13, 1.8), (30, 13, 1.1), (62, 8, 1.3), (10, 50, 1.6), (85, 47, 2.2),
                    (16, 70, 1.2), (79, 72, 1.4), (36, 47, 0.9), (60, 74, 0.8)):
        sh.append((star(x, y, r), STAR))
    for x, y in ((22, 6), (72, 18), (6, 30), (89, 30), (50, 45), (25, 76), (70, 47), (88, 62), (5, 62)):
        sh.append((poly(circle(x, y, 0.45, 24)), STAR))
    # Netuno: disco azul com faixas, mancha escura e anel fino inclinado.
    cx, cy, r = 47.0, 60.0, 12.5
    sh.append((ring(cx, cy, 25, 5.2, 0.7, -14, 180, 360), "#A9C2F2"))
    sh.append((poly(circle(cx, cy, r)), "#4F82E6"))
    for y0, y1, c in ((48, 51, "#6E9CF0"), (53.5, 55.5, "#3A68CF"), (63, 66, "#6E9CF0"), (68.5, 70.5, "#3A68CF")):
        sh.append((chord_band(cx, cy, r, y0, y1), c))
    sh.append((poly(ellipse(51.5, 58.6, 3.2, 1.6, -8)), "#22459E"))
    sh.append((crescent(cx, cy, r, 4.0), "#2C56B8"))
    sh.append((ring(cx, cy, 25, 5.2, 0.7, -14, 0, 180), "#A9C2F2"))
    # Ondas do mar no pé do pedal, em três camadas, com espuma nas cristas.
    x0, x1, base = 3.6, W - 3.6, H - 3.6
    for top, amp, lam, ph, col, foam in ((101.0, 2.6, 30.0, 0.0, "#1C3F9C", "#7FA6EE"),
                                         (106.0, 2.4, 24.0, 1.7, "#2B57BF", "#B9CFF7"),
                                         (111.0, 2.0, 19.0, 3.1, "#3B6CD6", "#EEF4FF")):
        xs = [x0 + (x1 - x0) * i / 240 for i in range(241)]
        crest = [(x, top - amp * (0.5 + 0.5 * math.sin(2 * math.pi * x / lam + ph)) ** 3 * 1.6) for x in xs]
        sh.append((poly(crest + [(x1, base), (x0, base)]), col))
        under = [(x, y + 0.9 + 0.6 * (0.5 + 0.5 * math.sin(2 * math.pi * x / lam + ph))) for x, y in reversed(crest)]
        sh.append((poly(crest + under), foam))
    save("netuno_estampa.svg", W, H, sh)


# ---------------------------------------------------------------- Saturn Rotary
# Gravura em tinta escura sobre o dourado: moldura dupla, órbitas, Saturno e estrelas.
def saturn():
    W, H = 94.0, 120.0
    INK = "#2B1E0E"
    sh = marks(W, H)
    def frame(m, w):
        out = [(m, m), (W - m, m), (W - m, H - m), (m, H - m)]
        inn = [(m + w, m + w), (m + w, H - m - w), (W - m - w, H - m - w), (W - m - w, m + w)]
        return poly(out + [out[0]] + [inn[0]] + inn[1:] + [inn[0]])
    sh.append((frame(4.5, 0.55), INK))
    sh.append((frame(5.6, 0.25), INK))
    cx, cy = 47.0, 61.0
    for rx, ry, rot in ((40.5, 13.5, -10), (35.5, 9.0, 8)):
        sh.append((ring(cx, cy, rx, ry, 0.3, rot), INK))
    for x, y, r in ((12, 13, 2.4), (82, 12, 1.6), (13, 48, 1.4), (82, 49, 2.0), (16, 79, 1.1), (78, 80, 1.5),
                    (30, 46, 0.9), (66, 75, 0.9)):
        sh.append((star(x, y, r, 0.18), INK))
    for x, y in ((24, 9), (70, 18), (9, 30), (86, 31), (60, 44), (28, 77), (88, 64), (7, 64)):
        sh.append((poly(circle(x, y, 0.4, 24)), INK))
    rot = -16
    for rx, w in ((29.5, 2.4), (25.6, 0.9), (23.6, 0.5)):
        sh.append((ring(cx, cy, rx, rx * 0.27, w, rot, 180, 360), INK))
    r = 13.0
    sh.append((poly(circle(cx, cy, r + 0.55)), INK))
    sh.append((poly(circle(cx, cy, r)), "#E6C37A"))
    for y0, y1 in ((51.0, 52.0), (54.0, 56.2), (58.0, 58.6), (62.5, 64.5), (66.5, 67.2), (69.5, 71.5)):
        sh.append((chord_band(cx, cy, r, y0, y1), "#7A5420"))
    sh.append((crescent(cx, cy, r, 3.6), "#A47A33"))
    for rx, w in ((29.5, 2.4), (25.6, 0.9), (23.6, 0.5)):
        sh.append((ring(cx, cy, rx, rx * 0.27, w, rot, 0, 180), INK))
    save("saturno_estampa.svg", W, H, sh)


# ---------------------------------------------------------------- Greengage Drive
# Tampo 74 x 124 mm. Fatias de limão em verde sobre verde atrás dos botões.
def lemon():
    W, H = 74.0, 124.0
    sh = marks(W, H)
    TONE = "#4FAE6E"
    for cx, cy, r in ((13.5, 37.5, 5.2), (60.5, 37.5, 5.2), (37.0, 22.5, 4.0), (11.0, 10.5, 3.2), (63.0, 10.5, 3.2)):
        sh.append((ring(cx, cy, r, r, r * 0.11, 0), TONE))
        sh += [(w, TONE) for w in wedges(cx, cy, r * 0.8)]
    save("limao_estampa.svg", W, H, sh)


neptune()
saturn()
lemon()
print("ok")
