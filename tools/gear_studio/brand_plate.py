# Nexus Gear Studio: a marca do Nexus em relevo (triângulo impossível + NEXUS
# + MODELER), igual à da barra de cima do programa, para a frente dos
# cabeçotes. O triângulo vem de assets/marca/nexus_marca.svg (o mesmo da tela,
# do ícone e do site); o NEXUS repete o desenho letra por letra de
# plugin/source/ui/Brand.h (E sem haste, X em azul).
import math
import os
import re

import bmesh
import bpy

import studio as st

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
SVG = os.path.join(ROOT, "assets", "marca", "nexus_marca.svg")


def _quad(p0, c, p1, steps=10):
    return [((1 - t) ** 2 * p0[0] + 2 * (1 - t) * t * c[0] + t * t * p1[0],
             (1 - t) ** 2 * p0[1] + 2 * (1 - t) * t * c[1] + t * t * p1[1]) for t in (i / steps for i in range(1, steps + 1))]


class _Path:
    """Contorno 2D (y para baixo, como na tela) com reta e curva."""

    def __init__(self, x, y):
        self.pts = [(x, y)]

    def line(self, x, y):
        self.pts.append((x, y))
        return self

    def quad(self, cx, cy, x, y):
        self.pts += _quad(self.pts[-1], (cx, cy), (x, y))
        return self


def wordmark(h):
    """Polígonos do NEXUS (altura h) como em Brand.h: devolve (letras, x, largura)."""
    t, w, gap, r = h * 0.20, h * 1.22, h * 0.30, h * 0.34
    letters, xmark = [], []
    x, y = 0.0, 0.0
    rect = lambda a, b, cw, ch: [(a, b), (a + cw, b), (a + cw, b + ch), (a, b + ch)]
    # N
    letters += [rect(x, y, t, h), rect(x + w - t, y, t, h),
                [(x, y), (x + t * 1.35, y), (x + w, y + h), (x + w - t * 1.35, y + h)]]
    x += w + gap
    # E sem haste
    letters += [rect(x, y, w, t), rect(x, y + (h - t) * 0.5, w * 0.92, t), rect(x, y + h - t, w, t)]
    x += w + gap
    # X
    k = t * 0.75
    xmark += [[(x, y), (x + k * 1.6, y), (x + w, y + h), (x + w - k * 1.6, y + h)],
              [(x + w, y), (x + w - k * 1.6, y), (x, y + h), (x + k * 1.6, y + h)]]
    x += w + gap
    # U
    ri = max(r - t, 0.0005)
    u = _Path(x, y).line(x, y + h - r).quad(x, y + h, x + r, y + h).line(x + w - r, y + h).quad(x + w, y + h, x + w, y + h - r)
    u.line(x + w, y).line(x + w - t, y).line(x + w - t, y + h - t - ri).quad(x + w - t, y + h - t, x + w - t - ri, y + h - t)
    u.line(x + t + ri, y + h - t).quad(x + t, y + h - t, x + t, y + h - t - ri).line(x + t, y)
    letters.append(u.pts)
    x += w + gap
    # S
    m = (h - t) * 0.5
    ro = r * 0.8
    ri = max(ro - t, 0.0005)
    X = lambda v: x + v
    s = _Path(X(w), 0).line(X(w), t).line(X(t + ri), t).quad(X(t), t, X(t), t + ri).line(X(t), m - ri).quad(X(t), m, X(t + ri), m)
    s.line(X(w - ro), m).quad(X(w), m, X(w), m + ro).line(X(w), h - ro).quad(X(w), h, X(w - ro), h).line(X(0), h).line(X(0), h - t)
    s.line(X(w - t - ri), h - t).quad(X(w - t), h - t, X(w - t), h - t - ri).line(X(w - t), m + t + ri).quad(X(w - t), m + t, X(w - t - ri), m + t)
    s.line(X(ro), m + t).quad(X(0), m + t, X(0), m + t - ro).line(X(0), ro).quad(X(0), 0, X(ro), 0)
    letters.append(s.pts)
    x += w
    return letters, xmark, x


def penrose():
    """As três faces preenchidas do triângulo (unidades do SVG, y para baixo)."""
    svg = open(SVG, encoding="utf-8").read()
    faces = []
    for m in re.finditer(r'<path d="([^"]+)" fill="url\(#(nx[xyz])\)"', svg):
        nums = [float(v) for v in re.findall(r"-?\d+\.?\d*", m.group(1))]
        pts = []
        for i in range(0, len(nums) - 1, 2):
            p = (nums[i], nums[i + 1])
            if not pts or abs(p[0] - pts[-1][0]) > 1e-6 or abs(p[1] - pts[-1][1]) > 1e-6:
                pts.append(p)
        if len(pts) > 2 and abs(pts[0][0] - pts[-1][0]) < 1e-6 and abs(pts[0][1] - pts[-1][1]) < 1e-6:
            pts.pop()
        faces.append((m.group(2), pts))
    return faces


def _solid(name, pts2d, scale, origin, depth, material):
    """Peça plana em relevo na frente (plano XZ, olhando para -Y)."""
    ox, oy, oz = origin
    mesh = bpy.data.meshes.new(name)
    bm = bmesh.new()
    front = [bm.verts.new((ox + px * scale, oy, oz - py * scale)) for px, py in pts2d]
    back = [bm.verts.new((ox + px * scale, oy + depth, oz - py * scale)) for px, py in pts2d]
    try:
        bm.faces.new(front[::-1])
        bm.faces.new(back)
        n = len(front)
        for i in range(n):
            j = (i + 1) % n
            bm.faces.new((front[i], front[j], back[j], back[i]))
    except ValueError:
        pass
    bmesh.ops.recalc_face_normals(bm, faces=bm.faces)
    bm.to_mesh(mesh)
    bm.free()
    obj = bpy.data.objects.new(name, mesh)
    bpy.context.collection.objects.link(obj)
    obj.data.materials.append(material)
    bev = obj.modifiers.new("chanfro", "BEVEL")
    bev.width = depth * 0.35
    bev.segments = 2
    bev.limit_method = "ANGLE"
    return obj


def is_light(rgb):
    """Frente clara (prateada, creme, tela clara): a marca precisa de letra escura."""
    r, g, b = rgb
    return 0.2126 * r + 0.7152 * g + 0.0722 * b > 0.42


def build(center, height, front_y, fonts, with_modeler=True, depth=0.003, satin=False, dark=False):
    """Monta a marca centrada em center (x, z), com o NEXUS da altura dada.
    dark: letras pretas brilhantes, para frente clara. Devolve a largura total."""
    if dark:
        letters = st.mat_plastic("marca_letras", (0.012, 0.012, 0.014), rough=0.18, coat=1.0)
    elif satin:
        # Prateado acetinado (claro mesmo sem nada para refletir, como na caixa).
        letters = st.mat_plastic("marca_letras", (0.78, 0.8, 0.83), rough=0.3, coat=0.6)
    else:
        letters = st.mat_metal("marca_letras", (0.93, 0.95, 0.97), rough=0.14)
    mats = {
        "letters": letters,
        "x": st.mat_metal("marca_x", (0.10, 0.52, 0.95), rough=0.22),
        "nxx": st.mat_metal("marca_azul_claro", (0.18, 0.62, 0.98), rough=0.25),
        "nxy": st.mat_metal("marca_azul_fundo", (0.03, 0.26, 0.78), rough=0.25),
        "nxz": st.mat_metal("marca_aco", (0.42, 0.52, 0.62), rough=0.28),
        "modeler": (st.mat_plastic("marca_modeler", (0.01, 0.12, 0.42), rough=0.25, coat=0.8) if dark
                    else st.mat_glow("marca_modeler", (0.16, 0.71, 1.0), 2.2)),
    }
    letters, xmark, word_w = wordmark(1.0)
    tri = penrose()
    tri_h_units = 88.6  # altura do desenho do SVG (viewBox)
    tri_h = height * 1.55
    tri_scale = tri_h / tri_h_units
    tri_w = 104.0 * tri_scale
    gap = height * 0.55
    total_w = tri_w + gap + word_w * height
    cx, cz = center
    x0 = cx - total_w / 2
    top = cz + tri_h / 2
    # Triângulo à esquerda, centrado na altura.
    for key, pts in tri:
        _solid(f"marca_{key}", [(px + 2.0, py + 2.0) for px, py in pts], tri_scale, (x0, front_y, top), depth, mats[key])
    # NEXUS: em cima, alinhado ao topo do triângulo um pouco abaixo; MODELER embaixo.
    wx = x0 + tri_w + gap
    word_top = cz + height * (0.62 if with_modeler else 0.5)
    for i, pts in enumerate(letters):
        _solid(f"marca_letra_{i}", pts, height, (wx, front_y, word_top), depth, mats["letters"])
    for i, pts in enumerate(xmark):
        _solid(f"marca_x_{i}", pts, height, (wx, front_y, word_top), depth, mats["x"])
    if with_modeler:
        mz = word_top - height * 1.62
        st.text("marca_modeler", "M O D E L E R", fonts["ui"], height * 0.36, (wx + word_w * height / 2, front_y - 0.0004, mz),
                mats["modeler"], extrude=0.0003, spacing=1.0)
        line_w = height * 1.25
        half = height * 1.55  # metade da largura do MODELER espaçado
        for side in (-1, 1):
            lx = wx + word_w * height / 2 + side * (half + height * 0.35 + line_w / 2)
            st.box(f"marca_traco_{side}", (line_w, 0.001, height * 0.05), (lx, front_y - 0.0004, mz + height * 0.12), material=mats["modeler"])
    return total_w
