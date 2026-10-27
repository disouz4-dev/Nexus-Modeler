#!/usr/bin/env python3
"""Marca do Nexus Modeler: o triângulo impossível (de Penrose).

Proporção e estética da referência aprovada: vigas largas e furo pequeno no
meio, cada viga com duas faces separadas por filetes claros, trilhas de
circuito gravadas nas faces e contorno claro fino. Cores do projeto: faces de
fora em azul (do ciano ao azul fundo), faces de dentro em aço.

Gera, a partir da mesma geometria:
  assets/marca/nexus_marca.svg        a marca completa (tela, ícone, site)
  assets/marca/camadas/*.svg          uma camada por material, sem degradê,
                                      para o Gear Studio montar a placa 3D
                                      (faces de fora, faces de dentro, trilhas)
  docs/site/img/nexus-icone.svg       cópia para o site

Uso: python3 tools/marca/logo.py
"""
import math
import os
import shutil

RAIZ = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))

# Geometria: o triângulo de Penrose perfeito, com os três cantos iguais.
#
# Uma viga de seção quadrada ao longo de x, em projeção isométrica. A viga
# seguinte (ao longo de y) passa na frente dela no canto de chegada; então a
# parte visível da primeira viga é a projeção dela menos a da seguinte. As
# outras duas peças são essa mesma peça girada 120 e 240 graus em volta do
# centro: os três encontros ficam idênticos, como no desenho clássico.
# (Precisa da biblioteca shapely, só para gerar; o SVG final é puro.)
from shapely.geometry import LineString, Polygon
from shapely import affinity

L = 1.0
W = 0.22          # largura da viga em relação ao lado (proporção da referência)
COS30 = math.sqrt(3) / 2

AZUL_CLARO = "#6fd6ff"
AZUL_MEIO = "#1f86e8"
AZUL_FUNDO = "#0b4fb8"
ACO_CLARO = "#8fb2d4"   # aço azulado
ACO_ESCURO = "#2a4462"
FILETE = "#eaf7ff"
GRAVADO = "#071a2c"

# Cor de cada face pela direção dela no espaço (a luz fica coerente nas três peças).
CORES = {"x": (AZUL_CLARO, AZUL_MEIO), "y": (AZUL_MEIO, AZUL_FUNDO), "z": (ACO_CLARO, ACO_ESCURO)}
# Girar 120 graus troca os eixos em ciclo: a face y vira z, a z vira x.
CICLO = {"x": "y", "y": "z", "z": "x"}


def proj(p):
    x, y, z = p
    return ((x - y) * COS30, (x + y) * 0.5 - z)


def caixa_2d(lo, hi):
    pts = [proj((x, y, z)) for x in (lo[0], hi[0]) for y in (lo[1], hi[1]) for z in (lo[2], hi[2])]
    return Polygon(pts).convex_hull


VIGA = ((0, 0, 0), (L, W, W))
SEGUINTE = caixa_2d((L - W, 0, 0), (L, L, W))
# Centro de giro: os cubos de canto das três vigas formam um triângulo equilátero.
_q = [proj((W / 2, W / 2, W / 2)), proj((L - W / 2, W / 2, W / 2)), proj((L - W / 2, L - W / 2, W / 2))]
CENTRO = (sum(p[0] for p in _q) / 3, sum(p[1] for p in _q) / 3)
_a0 = math.atan2(_q[0][1] - CENTRO[1], _q[0][0] - CENTRO[0])
_a1 = math.atan2(_q[1][1] - CENTRO[1], _q[1][0] - CENTRO[0])
GIRO = math.degrees(_a1 - _a0)


def face_viga(eixo):
    (x0, y0, z0), (x1, y1, z1) = VIGA
    if eixo == "y":
        cantos = [(x0, y1, z0), (x1, y1, z0), (x1, y1, z1), (x0, y1, z1)]
    else:
        cantos = [(x0, y0, z1), (x1, y0, z1), (x1, y1, z1), (x0, y1, z1)]
    return Polygon([proj(p) for p in cantos])


def ponto_face(eixo, u, v):
    """u ao longo da viga (0 a 1), v na largura da face (0 a 1)."""
    if eixo == "y":
        return proj((u * L, W, v * W))
    return proj((u * L, v * W, W))


# Trilhas de circuito: (v inicial, u inicial, u da curva, v final, u final).
TRILHAS = {
    "y": [(0.22, 0.10, 0.30, 0.50, 0.46), (0.78, 0.40, 0.56, 0.48, 0.70), (0.30, 0.62, 0.74, 0.78, 0.92)],
    "z": [(0.72, 0.14, 0.26, 0.34, 0.52), (0.26, 0.58, 0.66, 0.60, 0.80), (0.82, 0.70, 0.78, 0.66, 0.90)],
}

# A peça base: as duas faces visíveis da viga, sem o que a viga seguinte cobre.
PECA = []
for eixo in ("y", "z"):
    area = face_viga(eixo).difference(SEGUINTE)
    linhas = []
    for v0, u0, u1, v1, u2 in TRILHAS[eixo]:
        du = abs(v1 - v0) * W / L
        linha = LineString([ponto_face(eixo, u0, v0), ponto_face(eixo, u1, v0),
                            ponto_face(eixo, u1 + du, v1), ponto_face(eixo, u2, v1)])
        # Trilha cortada pela viga da frente: some inteira (pedaço solto parece defeito).
        if area.buffer(-0.004).contains(linha):
            linhas.append(linha)
    # Direção do degradê: ao longo da viga.
    grad = (proj((0, W / 2, W / 2)), proj((L, W / 2, W / 2)))
    PECA.append((eixo, area, linhas, grad))

# Giro final do desenho: a ponta para cima e a base reta embaixo.
ROT = -90.0


def _rot(x, y):
    a = math.radians(ROT)
    dx, dy = x - CENTRO[0], y - CENTRO[1]
    return (CENTRO[0] + dx * math.cos(a) - dy * math.sin(a), CENTRO[1] + dx * math.sin(a) + dy * math.cos(a))


# Enquadramento: as três peças giradas, e a escala para 100 de largura.
_todas = []
for k in range(3):
    for _, area, _, _ in PECA:
        g = affinity.rotate(area, GIRO * k, origin=CENTRO)
        _todas.append(Polygon([_rot(x, y) for x, y in g.exterior.coords]))
_xs = [x for g in _todas for x, _ in g.exterior.coords]
_ys = [y for g in _todas for _, y in g.exterior.coords]
# Gira o conjunto para a ponta ficar para cima.
_topo = min(range(len(_ys)), key=lambda i: _ys[i])
_ESC = 100.0 / (max(_xs) - min(_xs))
_X0, _Y0 = min(_xs), min(_ys)
ALTURA = (max(_ys) - min(_ys)) * _ESC


# Sentido do giro das vigas: o do desenho clássico (e da referência), sem espelho.
ESPELHO = False


def tela(x, y):
    x, y = _rot(x, y)
    px, py = (x - _X0) * _ESC, (y - _Y0) * _ESC
    return (100.0 - px if ESPELHO else px, py)


def pts_svg(coords, fechar=True):
    ps = [tela(x, y) for x, y in coords]
    s = "M" + " L".join(f"{x:.3f} {y:.3f}" for x, y in ps)
    return s + ("Z" if fechar else "")


def gira_ponto(p, k):
    g = affinity.rotate(LineString([p, (p[0] + 1e-9, p[1])]), GIRO * k, origin=CENTRO)
    return g.coords[0]


def caminho_area(area):
    partes = area.geoms if hasattr(area, "geoms") else [area]
    return "".join(pts_svg(list(p.exterior.coords)[:-1]) for p in partes if not p.is_empty)


def svg_marca(ids="nx"):
    """Três fitas contínuas, como no desenho clássico: a face de fora de uma viga
    dobra o canto e vira, sem emenda, a face de dentro da viga seguinte."""
    from shapely.ops import unary_union

    fitas = {"x": [], "y": [], "z": []}
    trilhas = []
    for k in range(3):
        for eixo, area, linhas, _ in PECA:
            eixo_k = eixo
            for _ in range(k):
                eixo_k = CICLO[eixo_k]
            fitas[eixo_k].append(affinity.rotate(area, GIRO * k, origin=CENTRO))
            for linha in linhas:
                trilhas.append(affinity.rotate(linha, GIRO * k, origin=CENTRO))

    defs, corpo = [], []
    for eixo, partes in fitas.items():
        fita = unary_union([p.buffer(1e-7) for p in partes]).buffer(-1e-7)
        # Degradê ao longo da fita: das duas pontas mais distantes.
        pts = [tela(x, y) for x, y in fita.exterior.coords]
        p0, p1 = max(((a, b) for a in pts for b in pts), key=lambda ab: (ab[0][0] - ab[1][0]) ** 2 + (ab[0][1] - ab[1][1]) ** 2)
        c1, c2 = CORES[eixo]
        gid = f"{ids}{eixo}"
        defs.append(f'<linearGradient id="{gid}" gradientUnits="userSpaceOnUse" x1="{p0[0]:.3f}" y1="{p0[1]:.3f}" '
                    f'x2="{p1[0]:.3f}" y2="{p1[1]:.3f}"><stop offset="0" stop-color="{c1}"/><stop offset="1" stop-color="{c2}"/></linearGradient>')
        corpo.append(f'<path d="{caminho_area(fita)}" fill="url(#{gid})"/>')
    for linha in trilhas:
        coords = list(linha.coords)
        t = pts_svg(coords, fechar=False)
        corpo.append(f'<path d="{t}" fill="none" stroke="#ffffff" stroke-opacity=".30" stroke-width=".7" '
                     f'stroke-linejoin="round" transform="translate(.3 .3)"/>')
        corpo.append(f'<path d="{t}" fill="none" stroke="{GRAVADO}" stroke-opacity=".7" stroke-width=".65" stroke-linejoin="round"/>')
        for x, y in (tela(*coords[0]), tela(*coords[-1])):
            corpo.append(f'<circle cx="{x:.3f}" cy="{y:.3f}" r=".9" fill="none" stroke="{GRAVADO}" stroke-opacity=".7" stroke-width=".5"/>')
    # Filetes só nas bordas das fitas (nunca no meio de uma fita).
    for eixo, partes in fitas.items():
        fita = unary_union([p.buffer(1e-7) for p in partes]).buffer(-1e-7)
        corpo.append(f'<path d="{caminho_area(fita)}" fill="none" stroke="{FILETE}" stroke-opacity=".8" stroke-width=".45" stroke-linejoin="round"/>')
    return (f'<svg xmlns="http://www.w3.org/2000/svg" viewBox="-2 -2 104 {ALTURA + 4:.1f}">\n<defs>\n'
            + "\n".join(defs) + "\n</defs>\n" + "\n".join(corpo) + "\n</svg>\n")


def main():
    destino = os.path.join(RAIZ, "assets", "marca")
    os.makedirs(destino, exist_ok=True)
    marca = svg_marca()
    with open(os.path.join(destino, "nexus_marca.svg"), "w", encoding="utf-8") as f:
        f.write(marca)
    shutil.copyfile(os.path.join(destino, "nexus_marca.svg"), os.path.join(RAIZ, "docs", "site", "img", "nexus-icone.svg"))
    print("marca gravada em", destino)


if __name__ == "__main__":
    main()
