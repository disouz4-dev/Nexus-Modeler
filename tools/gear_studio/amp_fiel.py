# Nexus Gear Studio: cabeçotes fiéis ao original (receitas com "fileiras").
#
# Monta cabeçote, combo, rack ou caixinha (lunchbox) com o painel do
# original: fileiras e blocos de botões em vários estilos, chaves, entradas,
# luz piloto, LEDs, VU, EQ gráfico, riscos impressos e a frente (tela de
# pano, grade de metal, chapa xadrez, vidro com válvulas ou tolex com friso).
#
# Coordenadas: metros, a frente olha para -Y, o chão é z = 0 e o centro x = 0.
# O painel é montado num quadro próprio (x para a direita, z para cima, -y
# saindo do painel) pendurado num ponto "raiz"; no painel de cima dos combos
# (posição "topo") a raiz deita e tudo vai junto.
#
# Campos novos da receita (os antigos continuam valendo onde fazem sentido):
#   formato      "cabecote" | "combo" | "rack" | "caixinha"
#   rack_u       altura em U do rack (1, 2, 3...)
#   painel       posicao: "baixo" | "cima" | "inteiro" | "topo"; material:
#                "escovado" | "pintado" | "cromo" | "acrilico" (com "luz");
#                altura_m (faixa do painel), margem_m, recuo_m
#   fachada      tipo: "tela" | "grade" | "losango" | "garras" | "xadrez" |
#                "tolex" | "vidro" (com "valvulas": n)
#   estilos      {"nome": {forma, diametro_m, corpo, tampa, saia, traco,
#                numeros, escala, anel, metal}}; "botao_estilo" é o padrão
#   fileiras     [{z, x0, x1, rotulo_pos, letra_m, estilo, itens: [...]}]
#                item: {tipo, id, rotulo, fixo, estilo, peso, x, z, dz, ...}
#                tipos: botao, deslizante, entrada, basculante, alavanca,
#                minichave, push, joia, led, led_canal, vu, texto, modelo,
#                marca, logo, espaco
#   riscos       [{x0, z0, x1, z1, esp_m, cor}] faixas e linhas impressas
#   alca         "couro" | "cromo" | "barra" | "nenhuma"; cantos "cromo" |
#                "preto" | "nenhum"
# Botão com "fixo": true é desenhado na frente mas não entra no mapa (não
# gira na tela). Os de "id" conhecido (ganho, grave...) são os que tocam.
import math

import bpy
from mathutils import Matrix, Vector

import brand_plate
import knobs
import studio as st

U = 0.04445  # uma unidade de rack


# ---------------------------------------------------------------- materiais próprios

def mat_claws(name, color, pitch_mm=14.0, rough=0.4):
    """Grade de metal recortada em "garras" (frente dos Diezel): rasgos
    curvos inclinados, desencontrados, com o fundo escuro aparecendo."""
    mat, nodes, links, bsdf = st._principled(name)
    coords = st._object_coords(nodes)
    bsdf.inputs["Metallic"].default_value = 0.5
    bsdf.inputs["Roughness"].default_value = rough
    m = st._math
    s = 1000.0 / pitch_mm
    sep = nodes.new("ShaderNodeSeparateXYZ")
    links.new(coords, sep.inputs[0])
    z = m(nodes, links, "MULTIPLY", sep.outputs["Z"], s * 0.55)
    row = m(nodes, links, "FLOOR", z)
    shift = m(nodes, links, "MULTIPLY", m(nodes, links, "MODULO", row, 2.0), 0.5)
    x = m(nodes, links, "ADD", m(nodes, links, "MULTIPLY", sep.outputs["X"], s), shift)
    fx = m(nodes, links, "SUBTRACT", m(nodes, links, "FRACT", x), 0.5)
    fz = m(nodes, links, "SUBTRACT", m(nodes, links, "FRACT", z), 0.5)
    # Rasgo curvo: a coluna do rasgo anda com o quadrado da altura (garra).
    bend = m(nodes, links, "SUBTRACT", fx, m(nodes, links, "ADD", m(nodes, links, "MULTIPLY", fz, 0.45),
                                                  m(nodes, links, "MULTIPLY", m(nodes, links, "MULTIPLY", fz, fz), 0.9)))
    d = m(nodes, links, "MAXIMUM", m(nodes, links, "DIVIDE", m(nodes, links, "ABSOLUTE", bend), 0.13),
          m(nodes, links, "DIVIDE", m(nodes, links, "ABSOLUTE", fz), 0.40))
    solid = m(nodes, links, "MINIMUM", m(nodes, links, "MAXIMUM", m(nodes, links, "DIVIDE", m(nodes, links, "SUBTRACT", d, 1.0), 0.25), 0.0), 1.0)
    mix = nodes.new("ShaderNodeMix")
    mix.data_type = "RGBA"
    mix.inputs[6].default_value = (0.002, 0.002, 0.002, 1)
    mix.inputs[7].default_value = (*color, 1)
    links.new(solid, mix.inputs[0])
    links.new(mix.outputs[2], bsdf.inputs["Base Color"])
    bump = nodes.new("ShaderNodeBump")
    bump.inputs["Strength"].default_value = 0.8
    bump.inputs["Distance"].default_value = 0.0008
    links.new(solid, bump.inputs["Height"])
    links.new(bump.outputs["Normal"], bsdf.inputs["Normal"])
    return mat


def mat_diamond_cloth(name, base, line, pitch_mm=38.0, weave_mm=1.6):
    """Tela de pano com losangos (a da Vox): fundo vinho em trama fina e
    os fios claros desenhando losangos grandes."""
    mat = st.mat_grille_cloth(name, base, [c * 0.8 for c in base], weave_mm)
    nodes, links = mat.node_tree.nodes, mat.node_tree.links
    bsdf = nodes["Principled BSDF"]
    coords = st._object_coords(nodes)
    m = st._math
    s = 1000.0 / pitch_mm
    sep = nodes.new("ShaderNodeSeparateXYZ")
    links.new(coords, sep.inputs[0])
    x = m(nodes, links, "MULTIPLY", sep.outputs["X"], s)
    z = m(nodes, links, "MULTIPLY", sep.outputs["Z"], s * 0.62)
    u = m(nodes, links, "ABSOLUTE", m(nodes, links, "SUBTRACT", m(nodes, links, "FRACT", m(nodes, links, "ADD", x, z)), 0.5))
    v = m(nodes, links, "ABSOLUTE", m(nodes, links, "SUBTRACT", m(nodes, links, "FRACT", m(nodes, links, "SUBTRACT", x, z)), 0.5))
    edge = m(nodes, links, "MAXIMUM", u, v)
    lineness = m(nodes, links, "MINIMUM", m(nodes, links, "MAXIMUM", m(nodes, links, "DIVIDE", m(nodes, links, "SUBTRACT", edge, 0.475), 0.012), 0.0), 1.0)
    old = bsdf.inputs["Base Color"].links[0].from_socket
    mix = nodes.new("ShaderNodeMix")
    mix.data_type = "RGBA"
    links.new(lineness, mix.inputs[0])
    links.new(old, mix.inputs[6])
    mix.inputs[7].default_value = (*line, 1)
    links.new(mix.outputs[2], bsdf.inputs["Base Color"])
    return mat


def mat_glass(name, tint=(0.02, 0.02, 0.025), rough=0.04):
    mat, _, _, bsdf = st._principled(name)
    bsdf.inputs["Base Color"].default_value = (*tint, 1)
    bsdf.inputs["Roughness"].default_value = rough
    bsdf.inputs["Transmission Weight"].default_value = 0.85
    bsdf.inputs["IOR"].default_value = 1.5
    return mat


def mat_acrylic(name, color):
    """Acrílico preto de piano (painel do TriAmp)."""
    mat, _, _, bsdf = st._principled(name)
    bsdf.inputs["Base Color"].default_value = (*color, 1)
    bsdf.inputs["Roughness"].default_value = 0.2
    bsdf.inputs["Coat Weight"].default_value = 0.6
    bsdf.inputs["Coat Roughness"].default_value = 0.16
    return mat


# ---------------------------------------------------------------- quadro do painel

class Frame:
    """Quadro do painel: converte (x, y, z) do painel para o mundo e pendura
    na raiz tudo o que for criado dentro dele."""

    def __init__(self, origin, lying):
        self.origin = Vector(origin)
        self.lying = lying
        self.rot = Matrix.Rotation(math.radians(-90), 3, "X") if lying else Matrix.Identity(3)
        self.root = bpy.data.objects.new("painel_raiz", None)
        bpy.context.collection.objects.link(self.root)
        self.root.location = self.origin
        self.root.rotation_euler = (math.radians(-90) if lying else 0.0, 0.0, 0.0)
        self._before = None

    def world(self, p):
        return tuple(self.origin + self.rot @ Vector(p))

    def axis(self):
        return tuple(self.rot @ Vector((0.0, 1.0, 0.0)))

    def label(self, text, p, size, color, spacing=1.0):
        st.label(text, self.world(p), size, up="Y" if self.lying else "Z", color=color, spacing=spacing)

    def __enter__(self):
        self._before = set(bpy.data.objects.keys())
        return self

    def __exit__(self, *exc):
        bpy.context.view_layer.update()
        for name in set(bpy.data.objects.keys()) - self._before:
            obj = bpy.data.objects[name]
            if obj is self.root or obj.parent is not None:
                continue
            obj.parent = self.root
        bpy.context.view_layer.update()
        return False


# ---------------------------------------------------------------- botões

DEFAULT_STYLE = {"forma": "marshall", "diametro_m": 0.028, "corpo": (0.018, 0.018, 0.018),
                 "tampa": (0.56, 0.39, 0.17), "traco": (0.01, 0.01, 0.01), "escala": "numeros"}


def _style(rec, name):
    """Estilo com nome vem de "estilos"; sem nome, é o "botao_estilo" antigo
    (ou estilos.padrao). A tampa, se não vier, é da cor do corpo."""
    base = dict(DEFAULT_STYLE)
    named = rec.get("estilos", {}).get(name or "padrao")
    if named is None:
        old = rec.get("botao_estilo", {})
        base.update(old)
        if old.get("corpo_metal"):
            base.setdefault("metal", True)
        return base
    base.update(named)
    if "tampa" not in named and base.get("forma") != "marshall":
        base["tampa"] = base["corpo"]
    return base


def _mats_for(style, cache, key):
    if key in cache:
        return cache[key]
    body = (st.mat_metal(f"botao_{key}", style["corpo"], rough=0.12) if style.get("metal")
            else st.mat_plastic(f"botao_{key}", style["corpo"], rough=0.28, coat=0.3))
    if style.get("tampa_plastico") or style.get("forma") in ("liso", "chicken", "bruxa", "saia_numerada", "facetado", "pointer"):
        cap = st.mat_plastic(f"tampa_{key}", style.get("tampa", style["corpo"]), rough=0.18, coat=0.5)
        if style.get("tampa_metal"):
            cap = st.mat_metal(f"tampa_{key}", style["tampa"], rough=0.14)
    else:
        cap = st.mat_brushed(f"tampa_{key}", style.get("tampa", (0.6, 0.6, 0.6)), rough=0.14, radial_axis="Y")
    out = {
        "body": body,
        "cap": cap,
        "pointer": st.mat_paint(f"traco_{key}", style.get("traco", (0.01, 0.01, 0.01)), rough=0.4),
        "skirt": st.mat_plastic(f"saia_{key}", style.get("saia", style["corpo"]), rough=0.3, coat=0.3),
        "nums": st.mat_paint(f"numeros_{key}", style.get("numeros", (0.9, 0.9, 0.9)), rough=0.5),
        "ring": st.mat_glow(f"anel_{key}", style["anel"], 3.0) if style.get("anel") else None,
    }
    cache[key] = out
    return out


def _cone(name, r1, r2, depth, y_center, material, vertices=64):
    bpy.ops.mesh.primitive_cone_add(vertices=vertices, radius1=r1, radius2=r2, depth=depth,
                                    location=(0, y_center, 0), rotation=(math.radians(90), 0, 0))
    obj = bpy.context.active_object
    obj.name = name
    bpy.ops.object.transform_apply(location=False, rotation=True, scale=False)
    obj.data.materials.append(material)
    st.smooth(obj, 40)
    return obj


def make_knob(name, style, mats, fonts):
    """Botão na origem olhando para -Y, com o traço em 12 horas. Devolve
    (ponto de giro, raio externo, profundidade)."""
    r = style["diametro_m"] / 2
    form = style.get("forma", "marshall")
    pivot = bpy.data.objects.new(f"giro_{name}", None)
    bpy.context.collection.objects.link(pivot)
    parts = []
    outer = r
    depth = 0.016
    if form == "marshall":
        parts.append(st.cylinder(f"saia_{name}", r * 1.22, 0.0032, (0, -0.0016, 0), material=mats["body"], bevel=0.0009))
        body = knobs.knurled_body(f"corpo_{name}", r, 0.0145, 30, mats["body"])
        body.location = (0, -0.003, 0)
        parts.append(body)
        top = -0.0175
        parts.append(st.cylinder(f"tampa_{name}", r * 0.80, 0.0016, (0, top - 0.0004, 0), material=mats["cap"], bevel=0.0004))
        parts.append(st.box(f"traco_{name}", (r * 0.075, 0.0006, r * 0.62), (0, top - 0.0013, r * 0.42), material=mats["pointer"]))
        parts.append(st.box(f"traco_lat_{name}", (r * 0.08, 0.012, r * 0.06), (0, -0.0095, r * 0.985), material=mats["pointer"]))
        outer, depth = r * 1.22, 0.018
    elif form == "cromado":
        h = max(0.012, r * 1.05)
        body = knobs.knurled_body(f"corpo_{name}", r, h, 36, mats["body"])
        parts.append(body)
        parts.append(st.cylinder(f"tampa_{name}", r * 0.86, 0.0012, (0, -h - 0.0003, 0), material=mats["cap"], bevel=0.0004))
        parts.append(st.box(f"traco_{name}", (r * 0.10, 0.0006, r * 0.6), (0, -h - 0.0010, r * 0.45), material=mats["pointer"]))
        outer, depth = r, h + 0.002
    elif form in ("liso", "pointer"):
        h = max(0.011, r * 0.95)
        parts.append(_cone(f"corpo_{name}", r, r * 0.9, h, -h / 2, mats["body"]))
        bev = st.cylinder(f"tampa_{name}", r * 0.86, 0.0012, (0, -h - 0.0003, 0), material=mats["cap"], bevel=0.0004)
        parts.append(bev)
        parts.append(st.box(f"traco_{name}", (r * 0.11, 0.0006, r * 0.66), (0, -h - 0.0010, r * 0.42), material=mats["pointer"]))
        parts.append(st.box(f"traco_lat_{name}", (r * 0.10, h * 0.8, r * 0.06), (0, -h * 0.5, r * 0.95), material=mats["pointer"]))
        outer, depth = r, h + 0.002
    elif form == "facetado":
        h = max(0.012, r * 1.0)
        body = st.cylinder(f"corpo_{name}", r, h, (0, -h / 2, 0), vertices=10, material=mats["body"], bevel=r * 0.12)
        parts.append(body)
        parts.append(st.cylinder(f"centro_{name}", r * 0.42, 0.002, (0, -h - 0.0006, 0), material=mats["cap"], bevel=0.0004))
        parts.append(st.box(f"traco_{name}", (r * 0.10, 0.0006, r * 0.30), (0, -h - 0.0003, r * 0.70), material=mats["pointer"]))
        outer, depth = r, h + 0.002
    elif form == "saia_numerada":
        # Saia larga com os números de 1 a 10 (Fender), corpo e tampa.
        sk = r * 1.38
        parts.append(st.cylinder(f"saia_{name}", sk, 0.0022, (0, -0.0011, 0), material=mats["skirt"], bevel=0.0006))
        for i in range(10):
            ang = math.radians(-150 + 300 * i / 9)
            rn = (r + sk) / 2 + 0.0003
            num = st.text(f"num_{name}_{i}", str(i + 1), fonts["label"], (sk - r) * 0.62,
                          (rn * math.sin(ang), -0.0023, rn * math.cos(ang)), mats["nums"], extrude=0.00005)
            num.rotation_euler = (math.radians(90), ang, 0)
            parts.append(num)
        h = max(0.010, r * 0.9)
        parts.append(_cone(f"corpo_{name}", r, r * 0.92, h, -0.0022 - h / 2, mats["body"]))
        parts.append(st.cylinder(f"tampa_{name}", r * 0.8, 0.0010, (0, -0.0022 - h - 0.0003, 0), material=mats["cap"], bevel=0.0003))
        parts.append(st.box(f"traco_{name}", (r * 0.10, 0.0005, r * 0.55), (0, -0.0022 - h - 0.0009, r * 0.45), material=mats["pointer"]))
        outer, depth = sk, h + 0.004
    elif form == "chicken":
        # Bico de galinha: base redonda e a ponta comprida apontando.
        h = 0.012
        parts.append(_cone(f"base_{name}", r * 0.8, r * 0.7, h * 0.55, -h * 0.275, mats["body"]))
        blade = st.box(f"bico_{name}", (r * 0.62, h, r * 1.9), (0, -h / 2, r * 0.45), bevel=r * 0.25, material=mats["body"])
        bm = blade.data
        for v in bm.vertices:
            # Afina para a ponta (z alto) e engrossa na base.
            t = (v.co.z + r * 0.5) / (r * 1.9)
            v.co.x *= 1.0 - 0.55 * max(0.0, min(1.0, t))
        parts.append(blade)
        parts.append(st.box(f"traco_{name}", (r * 0.09, 0.0005, r * 0.95), (0, -h - 0.0003, r * 0.80), material=mats["pointer"]))
        outer, depth = r * 1.4, h + 0.002
    elif form == "bruxa":
        # Chapéu de bruxa: saia larga, cone e cabeça pequena.
        parts.append(st.cylinder(f"saia_{name}", r * 1.35, 0.0028, (0, -0.0014, 0), material=mats["body"], bevel=0.0008))
        parts.append(_cone(f"cone_{name}", r * 0.95, r * 0.5, 0.010, -0.0028 - 0.005, mats["body"]))
        parts.append(st.cylinder(f"cabeca_{name}", r * 0.52, 0.0045, (0, -0.0128 - 0.00225, 0), material=mats["body"], bevel=0.0012))
        parts.append(st.box(f"traco_{name}", (r * 0.10, 0.0006, r * 0.42), (0, -0.0029, r * 1.12), material=mats["pointer"]))
        parts.append(st.box(f"traco_top_{name}", (r * 0.08, 0.0005, r * 0.36), (0, -0.0175, r * 0.25), material=mats["pointer"]))
        outer, depth = r * 1.35, 0.018
    else:
        raise ValueError(f"forma de botão desconhecida: {form}")
    if mats["ring"] is not None:
        parts.append(st.cylinder(f"anel_{name}", outer * 1.12, 0.0012, (0, -0.0004, 0), material=mats["ring"]))
        outer *= 1.12
    for p in parts:
        p.parent = pivot
    return pivot, outer, depth


# ---------------------------------------------------------------- peças do painel

class Panel:
    def __init__(self, rec, frame, pw, ph, fonts, text_color, mats):
        self.rec, self.f, self.pw, self.ph, self.fonts = rec, frame, pw, ph, fonts
        self.text_color = text_color
        self.mats = mats
        self.knob_mats = {}
        self.controls = []
        self.channel_leds = []
        self.channel_i = 0
        self.ink_y = -0.00012

    def label(self, text, x, z, size, color=None, spacing=1.0):
        if text:
            self.f.label(text, (x, self.ink_y, z), size, color or self.text_color, spacing)

    def ink(self, name, size, loc):
        return st.box(name, size, loc, material=self.mats["ink"])

    # -- cada tipo de peça devolve a meia altura que ocupa acima e abaixo do centro
    def knob(self, item, x, z, rowcfg):
        sname = item.get("estilo", rowcfg.get("estilo"))
        style = _style(self.rec, sname)
        if item.get("diametro_m"):
            style["diametro_m"] = item["diametro_m"]
        key = f"{sname or 'padrao'}_{int(style['diametro_m'] * 10000)}"
        mats = _mats_for(style, self.knob_mats, key)
        name = item.get("id") or f"fixo_{len(bpy.data.objects)}"
        pivot, outer, depth = make_knob(name, style, mats, self.fonts)
        pivot.location = (x, 0.0, z)
        scale_kind = item.get("escala", style.get("escala", "numeros"))
        rr = outer + 0.0016
        top = outer
        if scale_kind in ("numeros", "tracos"):
            self.scale(f"escala_{name}", x, z, rr, scale_kind == "numeros", style["diametro_m"])
            top = rr + (0.0034 + (0.0072 if scale_kind == "numeros" else 0.0))
        fixed = item.get("fixo") or not item.get("id")
        if fixed:
            pivot.rotation_euler = (0, math.radians(item.get("angulo", 0.0)), 0)
            pivot.name = f"fixo_{name}"
        else:
            self.controls.append({"id": item["id"], "pivot": pivot, "center": self.f.world((x, -depth / 2, z)),
                                  "outer_r": outer, "depth": depth, "axis": self.f.axis()})
        return top

    def scale(self, name, x, z, radius, numbers, diam):
        num_size = max(0.0030, min(0.0062, diam * 0.2))
        for i in range(11):
            ang = math.radians(-150 + 30 * i)
            long_tick = i % 2 == 0
            length = 0.0030 if long_tick else 0.0019
            rr = radius + length / 2
            tick = self.ink(f"{name}_t{i}", (0.0005, 0.0001, length), (x + rr * math.sin(ang), self.ink_y, z + rr * math.cos(ang)))
            tick.rotation_euler = (0, ang, 0)
            if numbers and long_tick:
                rn = radius + 0.0034 + num_size * 0.75
                st.text(f"{name}_n{i}", str(i), self.fonts["label"], num_size,
                        (x + rn * math.sin(ang), self.ink_y, z + rn * math.cos(ang)), self.mats["ink"])

    def slider(self, item, x, z, rowcfg):
        travel = item.get("curso_m", rowcfg.get("curso_m", 0.034))
        idx = item.get("id") or f"eqfixo_{len(bpy.data.objects)}"
        st.box(f"rasgo_{idx}", (0.0034, 0.004, travel + 0.008), (x, 0.0008, z), bevel=0.0012, material=self.mats["hole"])
        for k in range(7):
            tz = z - travel / 2 + travel * k / 6
            self.ink(f"eq_traco_{idx}_{k}", (0.0042 if k in (0, 3, 6) else 0.0026, 0.0001, 0.0005), (x - 0.0068, self.ink_y, tz))
        pivot = bpy.data.objects.new(f"giro_{idx}", None)
        bpy.context.collection.objects.link(pivot)
        cap = st.box(f"cursor_{idx}", (0.0105, 0.010, 0.0062), (0, -0.005, 0), bevel=0.0012, material=self.mats["slider"])
        line = st.box(f"cursor_traco_{idx}", (0.0105 * 0.86, 0.0004, 0.0007), (0, -0.0102, 0), material=self.mats["slider_line"])
        cap.parent = pivot
        line.parent = pivot
        pivot.location = (x, 0.0, z)
        if not item.get("fixo") and item.get("id"):
            self.controls.append({"id": item["id"], "pivot": pivot, "tipo": "deslizante", "z0": z, "curso": travel,
                                  "center": self.f.world((x, -0.005, z)), "w": 0.0135, "h": travel + 0.012, "depth": 0.012})
        else:
            pivot.location = (x, 0.0, z + travel * (item.get("valor", 0.5) - 0.5))
        return travel / 2 + 0.006

    def jack(self, item, x, z, rowcfg):
        m = self.mats
        st.cylinder("porca", 0.0082, 0.0035, (x, -0.0017, z), vertices=6, material=m["chrome"], bevel=0.0006)
        st.cylinder("bucha", 0.0058, 0.006, (x, -0.004, z), material=m["chrome"], bevel=0.0005)
        st.cylinder("furo", 0.0034, 0.0005, (x, -0.0071, z), material=m["hole"])
        return 0.0085

    def rocker(self, item, x, z, rowcfg):
        m = self.mats
        w, h = item.get("largura_m", 0.019), item.get("altura_m", 0.026)
        st.box("chave_caixa", (w + 0.005, 0.006, h + 0.006), (x, -0.003, z), bevel=0.0015, material=m["rocker_house"])
        color = item.get("cor", (0.85, 0.015, 0.008))
        mat = (st.mat_glow("chave_luz", color, 1.6) if item.get("aceso", True)
               else st.mat_plastic("chave_preta", color, rough=0.3, coat=0.4))
        rk = st.box("chave", (w, 0.008, h), (x, -0.0062, z), bevel=0.002, material=mat)
        rk.rotation_euler = (math.radians(-9), 0, 0)
        return h / 2 + 0.003

    def toggle(self, item, x, z, rowcfg, small=False):
        m = self.mats
        k = 0.62 if small else 1.0
        st.cylinder("alavanca_porca", 0.0062 * k, 0.0022, (x, -0.0011, z), vertices=6, material=m["chrome"], bevel=0.0004)
        st.cylinder("alavanca_bucha", 0.0036 * k, 0.004, (x, -0.003, z), material=m["chrome"], bevel=0.0003)
        up = 1 if item.get("posicao", "cima") == "cima" else -1
        length = 0.016 * k
        bpy.ops.mesh.primitive_cone_add(vertices=32, radius1=0.0016 * k, radius2=0.0026 * k, depth=length,
                                        location=(0, 0, 0))
        bat = bpy.context.active_object
        bat.name = "alavanca"
        bat.data.materials.append(m["chrome"])
        st.smooth(bat, 40)
        tilt = math.radians(55) * up
        bat.rotation_euler = (math.radians(90) - tilt, 0, 0)
        bat.location = (x, -0.005 - length / 2 * math.cos(tilt), z + up * length / 2 * math.sin(tilt))
        return 0.0075 * k + 0.004

    def push(self, item, x, z, rowcfg):
        s = item.get("tamanho_m", 0.009)
        st.box("push_aro", (s + 0.003, 0.002, s + 0.003), (x, -0.001, z), bevel=0.0008, material=self.mats["rocker_house"])
        col = item.get("cor", (0.02, 0.02, 0.02))
        mat = st.mat_glow("push_luz", col, 2.0) if item.get("aceso") else st.mat_plastic("push", col, rough=0.3, coat=0.4)
        st.box("push", (s, 0.004, s), (x, -0.003, z), bevel=0.0012, material=mat)
        return s / 2 + 0.003

    def handle_bar(self, item, x, z, rowcfg):
        """Puxador cromado em pé na frente (Orange Rockerverb)."""
        h = item.get("altura_m", self.ph * 0.6)
        for dz in (-h / 2, h / 2):
            st.cylinder("puxador_pe", 0.0045, 0.02, (x, -0.01, z + dz), material=self.mats["chrome"])
        st.cylinder("puxador", 0.0055, h + 0.011, (x, -0.02, z), axis="Z", material=self.mats["chrome"])
        return h / 2

    def jewel(self, item, x, z, rowcfg):
        r = item.get("raio_m", 0.008)
        st.cylinder("joia_aro", r * 1.25, 0.004, (x, -0.002, z), material=self.mats["chrome"], bevel=0.0008)
        bpy.ops.mesh.primitive_ico_sphere_add(subdivisions=1, radius=r, location=(x, -0.004, z))
        dome = bpy.context.active_object
        dome.name = "joia"
        dome.scale = (1, 0.75, 1)
        dome.data.materials.append(st.mat_glow("joia_luz", item.get("cor", (0.9, 0.06, 0.02)), 3.0))
        return r * 1.25

    def led(self, item, x, z, rowcfg, channel=False):
        r = item.get("raio_m", 0.0030)
        st.cylinder("led_aro", r * 1.4, 0.0016, (x, -0.0008, z), material=self.mats["chrome"], bevel=0.0004)
        bpy.ops.mesh.primitive_uv_sphere_add(radius=r, location=(x, -0.0016, z), segments=32, ring_count=16)
        dome = bpy.context.active_object
        dome.name = "led"
        dome.scale = (1, 0.7, 1)
        col = item.get("cor", (0.30, 0.012, 0.008))
        if channel:
            dome.data.materials.append(self.mats["led_canal"])
            # A tela acende o LED do canal ativo: anota onde ele fica.
            self.channel_leds.append((self.f.world((x, -0.0035, z)), r))
        elif item.get("aceso"):
            dome.data.materials.append(st.mat_glow("led_luz", col, 3.0))
        else:
            dome.data.materials.append(st.mat_plastic("led", col, rough=0.06, coat=1.0))
        st.smooth(dome, 80)
        return r * 1.4

    def vu(self, item, x, z, rowcfg):
        w, h = item.get("largura_m", 0.06), item.get("altura_m", 0.04)
        st.box("vu_moldura", (w + 0.006, 0.004, h + 0.006), (x, -0.002, z), bevel=0.0015, material=self.mats["black_metal"])
        st.box("vu_fundo", (w, 0.001, h), (x, -0.0035, z), material=st.mat_glow("vu_fundo", item.get("cor", (0.95, 0.78, 0.45)), 1.2))
        cz = z - h * 0.45
        for i in range(9):
            ang = math.radians(-45 + 90 * i / 8)
            rr = h * 0.78
            t = self.ink(f"vu_traco_{i}", (0.0006, 0.0002, 0.004 if i % 2 == 0 else 0.0025),
                         (x + rr * math.sin(ang), -0.0042, cz + rr * math.cos(ang)))
            t.rotation_euler = (0, ang, 0)
        needle = st.box("vu_agulha", (0.0006, 0.0003, h * 0.8), (x, -0.0045, cz + h * 0.4), material=self.mats["ink"])
        needle.rotation_euler = (0, math.radians(-20), 0)
        st.box("vu_vidro", (w, 0.001, h), (x, -0.0052, z), material=mat_glass("vu_vidro", (0.9, 0.9, 0.9)))
        return h / 2 + 0.003

    def model(self, item, x, z, rowcfg):
        size = item.get("tamanho_m", 0.024)
        txt = item.get("texto", self.rec["painel"].get("modelo", ""))
        font = self.fonts[item.get("fonte", "label")]
        mat = st.mat_paint("modelo_tinta", item["cor"], rough=0.5) if item.get("cor") else self.mats["ink"]
        obj = st.text("modelo", txt, font, size, (x, self.ink_y, z), mat, align=item.get("alinhar", "CENTER"))
        obj.data.shear = item.get("inclinacao", 0.0)
        return size * 0.6

    def brand_text(self, item, x, z, rowcfg):
        st.text("marca", self.rec["painel"].get("marca", "NEXUS MODELER"), self.fonts["label"], item.get("tamanho_m", 0.0062),
                (x, self.ink_y, z), self.mats["ink"], spacing=1.15)
        return 0.004

    def logo(self, item, x, z, rowcfg):
        h = item.get("altura_m", 0.012)
        brand_plate.build((x, z), h, -0.0015, self.fonts, item.get("modeler", False),
                          dark=item.get("escuro", brand_plate.is_light(self.rec["painel"]["cor"])))
        return h

    # -- fileiras
    def rows(self):
        for ri, row in enumerate(self.rec.get("fileiras", [])):
            items = row["itens"]
            x0 = row.get("x0", 0.04) * self.pw - self.pw / 2
            x1 = row.get("x1", 0.96) * self.pw - self.pw / 2
            weights = [it.get("peso", 1.0) for it in items]
            total = sum(weights) or 1.0
            acc = 0.0
            letter = row.get("letra_m", 0.0078)
            for it, wgt in zip(items, weights):
                frac = (acc + wgt / 2) / total
                acc += wgt
                kind = it.get("tipo", "botao")
                if kind == "espaco":
                    continue
                x = (it["x"] * self.pw - self.pw / 2) if "x" in it else x0 + (x1 - x0) * frac
                z = (it["z"] if "z" in it else row.get("z", 0.5)) * self.ph + it.get("dz", 0.0)
                fn = {"botao": self.knob, "deslizante": self.slider, "entrada": self.jack, "basculante": self.rocker,
                      "alavanca": self.toggle, "minichave": lambda i, a, b, c: self.toggle(i, a, b, c, small=True),
                      "push": self.push, "joia": self.jewel, "puxador": self.handle_bar, "led": self.led,
                      "led_canal": lambda i, a, b, c: self.led(i, a, b, c, channel=True),
                      "vu": self.vu, "modelo": self.model, "marca": self.brand_text, "logo": self.logo,
                      "texto": lambda i, a, b, c: 0.0}[kind]
                half = fn(it, x, z, row)
                text = it.get("rotulo")
                if kind == "led_canal" and text is None:
                    chans = self.rec.get("canais", [])
                    text = chans[self.channel_i]["nome"].upper() if self.channel_i < len(chans) else None
                    self.channel_i += 1
                elif kind == "led_canal":
                    self.channel_i += 1
                if text:
                    size = it.get("letra_m", letter)
                    pos = it.get("rotulo_pos", row.get("rotulo_pos", "cima"))
                    if kind == "texto":
                        lz = z
                    elif pos == "baixo":
                        lz = z - half - size * 0.9
                    elif pos == "direita":
                        self.label(text, x + half + size * 0.4 + len(text) * size * 0.28, z, size)
                        continue
                    else:
                        lz = z + half + size * 0.9
                    self.label(text, x, lz, size)

    def stripes(self):
        for i, s in enumerate(self.rec.get("riscos", [])):
            ax, az = s["x0"] * self.pw - self.pw / 2, s["z0"] * self.ph
            bx, bz = s["x1"] * self.pw - self.pw / 2, s["z1"] * self.ph
            length = math.hypot(bx - ax, bz - az)
            mat = (st.mat_glow(f"risco_{i}", s["cor"], s.get("luz", 3.0)) if s.get("luz")
                   else st.mat_paint(f"risco_{i}", s["cor"], rough=0.5) if s.get("cor") else self.mats["ink"])
            b = st.box(f"risco_{i}", (length, 0.0001, s.get("esp_m", 0.0006)), ((ax + bx) / 2, -0.00008, (az + bz) / 2), material=mat)
            b.rotation_euler = (0, -math.atan2(bz - az, bx - ax), 0)


# ---------------------------------------------------------------- caixa

def _fascia_mat(fa, name="fachada"):
    kind = fa.get("tipo")
    if kind == "tela":
        return st.mat_grille_cloth(name, fa["cor_a"], fa["cor_b"], fa["trama_mm"])
    if kind == "xadrez":
        return st.mat_diamond_plate(name, fa["cor"], fa.get("passo_mm", 11.0))
    if kind in ("grade", "losango"):
        return st.mat_perforated(name, fa["cor"], fa.get("furo_mm", 3.2), fa.get("passo_mm", 4.8),
                                 shape="losango" if kind == "losango" else fa.get("furo", "redondo"))
    if kind == "losangos":
        return mat_diamond_cloth(name, fa["cor"], fa.get("linha", (0.55, 0.5, 0.32)), fa.get("passo_mm", 38.0))
    if kind == "garras":
        return mat_claws(name, fa["cor"], fa.get("passo_mm", 14.0))
    if kind == "vidro":
        return mat_glass(name, fa.get("cor", (0.3, 0.3, 0.34)), rough=0.18)
    return st.mat_leather(name, fa["cor"], fa.get("grao_mm", 1.2), rough=0.42, strength=0.35)


def build(rec, fonts):
    fmt = rec.get("formato", "cabecote")
    W = rec["medidas_m"]["largura"]
    H = rec["medidas_m"]["altura"]
    D = rec["medidas_m"]["profundidade"]
    pn = rec["painel"]
    fa = rec.get("fachada", {})
    style = _style(rec, None)
    text_color = pn["texto_cor"]
    material = pn.get("material", "pintado" if pn.get("pintado") else "escovado")
    shell_metal = rec.get("couro", {}).get("metal") or fmt in ("rack",)
    mats = {
        "tolex": (st.mat_plastic("caixa", rec["couro"]["cor"], rough=rec["couro"].get("aspereza", 0.45), coat=0.2) if shell_metal
                  else st.mat_leather("couro", rec["couro"]["cor"], rec["couro"]["grao_mm"])),
        "piping": st.mat_plastic("friso", fa.get("friso") or (0.8, 0.78, 0.72), rough=0.32, coat=0.4),
        "panel": {"pintado": lambda: st.mat_plastic("painel", pn["cor"], rough=0.48, coat=0.15),
                  "cromo": lambda: st.mat_metal("painel", pn["cor"], rough=0.24),
                  "acrilico": lambda: mat_acrylic("painel", pn["cor"]),
                  "escovado": lambda: st.mat_brushed("painel", pn["cor"], rough=0.40, streak_axis="X")}[material](),
        "ink": st.mat_paint("tinta", text_color, rough=0.6),
        "slider": st.mat_plastic("cursor", style.get("cursor", (0.02, 0.02, 0.02)), rough=0.3, coat=0.3),
        "slider_line": st.mat_paint("cursor_traco", style.get("traco", (0.9, 0.9, 0.9)), rough=0.4),
        "chrome": st.mat_metal("cromo", (0.92, 0.92, 0.93), rough=0.10),
        "black_metal": st.mat_metal("metal_preto", (0.035, 0.035, 0.037), rough=0.32),
        "hole": st.mat_paint("furo", (0.0, 0.0, 0.0), rough=0.9),
        "rocker_house": st.mat_plastic("chave_caixa", (0.012, 0.012, 0.012), rough=0.3),
        "handle": st.mat_leather("alca", (0.012, 0.011, 0.010), 1.8, rough=0.38, strength=0.4),
        "led_canal": st.mat_plastic("led_canal", (0.30, 0.012, 0.008), rough=0.06, coat=1.0),
    }
    front = -D / 2

    # ---- caixa e moldura da frente
    if fmt == "rack":
        units = rec.get("rack_u", max(1, round(H / U)))
        H = units * U
        body_w = W - 0.04
        st.box("caixa", (body_w, D - 0.004, H - 0.002), (0, 0.002, H / 2), bevel=0.002, material=mats["tolex"])
        plate_t = 0.0035
        st.box("frente_rack", (W, plate_t, H - 0.001), (0, front - plate_t / 2 + 0.002, H / 2), bevel=0.0008, material=mats["panel"])
        # Orelhas com os rasgos de parafuso.
        ear = (W - body_w) / 2
        for sx in (-1, 1):
            ex = sx * (W / 2 - ear / 2 - 0.002)
            n = 1 if units == 1 else 2
            for k in range(n):
                hz = H / 2 if n == 1 else (0.0159 + k * (H - 0.0318))
                st.box("rasgo_rack", (0.0095, 0.0004, 0.0065), (ex, front - 0.0017, hz), bevel=0.00018, material=mats["hole"])
                st.cylinder("parafuso_rack", 0.0045, 0.0018, (ex, front - 0.0026, hz), material=mats["chrome"], bevel=0.0006)
        inner_w, t = body_w - 0.02, 0.0
        panel_x0, panel_w = -inner_w / 2, inner_w
        if rec.get("alca") == "rack":
            for sx in (-1, 1):
                hx = sx * (W / 2 - ear - 0.004)
                for hz in (H * 0.2, H * 0.8):
                    st.cylinder("alca_rack_pe", 0.004, 0.012, (hx, front - 0.006, hz), material=mats["chrome"])
                st.cylinder("alca_rack", 0.005, H * 0.62, (hx, front - 0.014, H / 2), axis="Z", material=mats["chrome"])
            panel_w = inner_w - 0.05
        panel_front = front - 0.0015
        panel_z0, panel_h = 0.004, H - 0.008
        fascia = None
    else:
        t = rec.get("aba_m", 0.019 if fmt in ("cabecote", "combo") else 0.008)
        bevel = 0.013 if fmt != "caixinha" else rec.get("chanfro_m", 0.006)
        shell = st.box("caixa", (W, D, H), (0, 0, H / 2), bevel=bevel, segments=6, material=mats["tolex"])
        inset = rec.get("recuo_frente_m", 0.03 if fmt != "caixinha" else 0.012)
        cutter = st.box("corte_frente", (W - 2 * t, inset * 2, H - 2 * t), (0, front, H / 2), bevel=0.004)
        st.cut(shell, cutter)
        st.smooth(shell, 35)
        inner_w = W - 2 * t
        inner_h = H - 2 * t
        # Frente da tela e do painel: um pouco para dentro da aba.
        fascia_y = front + (0.007 if fmt != "caixinha" else 0.004)
        panel_front = front + (0.016 if fmt != "caixinha" else 0.008)
        pos = pn.get("posicao", "baixo")
        ph = pn.get("altura_m", inner_h if pos == "inteiro" else 0.088)
        if pos == "inteiro":
            ph = inner_h
        gap = pn.get("vao_m", 0.003)
        if pos == "baixo":
            panel_z0, fz0, fz1 = t, t + ph + gap, H - t
        elif pos == "cima":
            panel_z0, fz0, fz1 = H - t - ph, t, H - t - ph - gap
        else:
            panel_z0, fz0, fz1 = t, t, H - t
        panel_x0, panel_w, panel_h = -inner_w / 2, inner_w, ph
        st.box("fundo_frente", (inner_w, 0.004, inner_h), (0, front + inset, H / 2),
               material=mats["tolex"] if fa.get("tipo") in (None, "tolex") else st.mat_paint("fundo", (0.004, 0.004, 0.004)))
        fascia = None
        if pos != "inteiro" and fa:
            fascia = (fz0, fz1)
            fh = fz1 - fz0
            fm = _fascia_mat(fa)
            fy = fascia_y
            band = fa.get("faixa")
            if band:
                # Faixa de vinil em cima da tela (Vox): o logo vai nela.
                bh = fh * band.get("altura_rel", 0.25)
                st.box("faixa_logo", (inner_w, 0.006, bh), (0, fy + 0.003, fz1 - bh / 2), bevel=0.002,
                       material=st.mat_leather("faixa", band.get("cor", rec["couro"]["cor"]), 1.4, rough=0.4, strength=0.4))
                if band.get("friso"):
                    st.cylinder("faixa_friso", 0.0018, inner_w - 0.004, (0, fy - 0.0004, fz1 - bh), axis="X",
                                material=st.mat_metal("faixa_friso", band["friso"], rough=0.2))
                fz1 -= bh
                fh = fz1 - fz0
            st.box("fachada", (inner_w, 0.006, fh), (0, fy + 0.003, fz0 + fh / 2), bevel=0.002, material=fm)
            if fa.get("tipo") == "vidro":
                # Válvulas acesas atrás do vidro escuro.
                n = fa.get("valvulas", 6)
                glow = st.mat_glow("filamento", (1.0, 0.35, 0.06), 6.0)
                glass = mat_glass("valvula_vidro", (0.8, 0.8, 0.8))
                for k in range(n):
                    vx = -inner_w * 0.36 + inner_w * 0.72 * (k + 0.5) / n
                    st.cylinder(f"valvula_{k}", 0.011, fh * 0.6, (vx, fy + 0.03, fz0 + fh * 0.4), axis="Z", material=glass)
                    st.cylinder(f"filamento_{k}", 0.003, fh * 0.25, (vx, fy + 0.03, fz0 + fh * 0.4), axis="Z", material=glow)
                if fa.get("neon"):
                    st.box("neon", (inner_w * 0.9, 0.002, 0.003), (0, fy + 0.02, fz0 + fh * 0.12),
                           material=st.mat_glow("neon", fa["neon"], 4.0))
            if fa.get("friso") is not None:
                st.rounded_rect_curve("friso", inner_w - 0.006, fh - 0.006, 0.006, 0.0027,
                                      (0, fy - 0.0006, fz0 + fh / 2), mats["piping"])
            if rec.get("selo_grade"):
                # Número ou nome na grade (5150): no canto de baixo à direita
                # ou, com "x_rel"/"z_rel", centrado onde a receita disser.
                sg = rec["selo_grade"]
                sh = sg.get("altura_m", 0.034)
                if "x_rel" in sg:
                    loc, al = (inner_w * sg["x_rel"], fy - 0.0035, fz0 + fh * sg.get("z_rel", 0.5)), "CENTER"
                else:
                    loc, al = (inner_w / 2 - sg.get("margem_m", 0.05), fy - 0.0035, fz0 + sh * 0.75), "RIGHT"
                st.text("selo_grade", sg["texto"], fonts[sg.get("fonte", "label")], sh, loc,
                        st.mat_brushed("selo", sg.get("cor", (0.86, 0.87, 0.9)), rough=0.2), extrude=0.0015, bevel=0.0004,
                        align=al)
            if fa.get("janela"):
                # Janela de ventilação com fendas (Rectifier) e a plaqueta embaixo.
                jw = inner_w * fa["janela"].get("largura_rel", 0.32)
                jh = fh * fa["janela"].get("altura_rel", 0.42)
                jz = fz0 + fh * fa["janela"].get("z_rel", 0.5)
                st.box("janela", (jw, 0.002, jh), (0, fy - 0.0012, jz), bevel=0.002, material=st.mat_paint("janela", (0.01, 0.01, 0.01), 0.7))
                n = int(jw / 0.012)
                for k in range(n):
                    st.box(f"janela_fenda_{k}", (0.0045, 0.003, jh * 0.86), (-jw / 2 + (k + 0.5) * jw / n, fy - 0.0022, jz),
                           bevel=0.001, material=mats["chrome"])
            lg = rec.get("logo", {})
            if band:
                fz0, fh = fz1, fz1 + bh - fz1
                fh = bh
            if lg:
                lz = fz0 + fh * lg.get("altura_rel", 0.5)
                lx = inner_w * lg.get("lado_rel", 0.0)
                face = fa.get("cor") or [(a + b) / 2 for a, b in zip(fa.get("cor_a", (0, 0, 0)), fa.get("cor_b", (0, 0, 0)))]
                brand_plate.build((lx, lz), lg.get("altura_m", 0.03), fy - 0.0045, fonts, lg.get("modeler", True),
                                  dark=lg.get("escuro", brand_plate.is_light(face)))

    # ---- painel
    lying = rec["painel"].get("posicao") == "topo"
    if lying:
        # Painel deitado no tampo, perto do fundo, voltado para cima.
        pd = pn.get("altura_m", 0.09)
        pw_ = pn.get("largura_m", W - 0.08)
        y_front = D / 2 - pn.get("fundo_m", 0.02) - pd
        origin = (0, y_front, H + 0.0032)
        frame = Frame(origin, True)
        panel_w, panel_h = pw_, pd
        panel_x0 = -pw_ / 2
    else:
        origin = (panel_x0 + panel_w / 2, panel_front, panel_z0)
        frame = Frame(origin, False)
    with frame:
        if fmt != "rack":
            st.box("painel", (panel_w - 0.004, 0.003, panel_h), (0, 0.0015, panel_h / 2), bevel=0.0008, material=mats["panel"])
        if pn.get("friso"):
            st.rounded_rect_curve("painel_friso", panel_w - 0.006, panel_h - 0.004, 0.003, 0.0016,
                                  (0, -0.0008, panel_h / 2), st.mat_metal("painel_friso", pn["friso"], rough=0.18))
        if material == "acrilico" and pn.get("luz"):
            st.box("painel_luz", (panel_w - 0.012, 0.001, 0.0025), (0, -0.0003, panel_h - 0.006),
                   material=st.mat_glow("painel_luz", pn["luz"], 4.0))
        p = Panel(rec, frame, panel_w, panel_h, fonts, text_color, mats)
        p.stripes()
        p.rows()
        # Canais sem LED na receita: põe os LEDs à esquerda, como antes.
        chans = rec.get("canais", [])
        if len(chans) > 1 and p.channel_i == 0:
            for i, ch in enumerate(chans):
                lx = -panel_w / 2 + rec.get("leds_canais_x", 0.075) + i * rec.get("leds_canais_passo", 0.024)
                lz = rec.get("leds_canais_z", 0.024)
                p.led({}, lx, lz, {}, channel=True)
                p.label(ch["nome"].upper(), lx, lz + 0.0105, 0.0068)
        if rec.get("parafusos_painel", fmt != "rack"):
            for sx in (-panel_w / 2 + 0.007, panel_w / 2 - 0.007):
                for sz in (0.008, panel_h - 0.008):
                    bpy.ops.mesh.primitive_uv_sphere_add(radius=0.0028, location=(sx, -0.0002, sz), segments=24, ring_count=12)
                    s = bpy.context.active_object
                    s.scale = (1, 0.35, 1)
                    s.data.materials.append(mats["chrome"])

    # ---- cantoneiras, alça e pés
    corners = rec.get("cantos", "preto" if fmt in ("cabecote",) else "nenhum")
    if corners != "nenhum" and fmt in ("cabecote", "combo"):
        cmat = mats["chrome"] if corners == "cromo" else mats["black_metal"]
        for cx in (-W / 2, W / 2):
            for cz in (0.0, H):
                for cy in (front, -front):
                    st.box("cantoneira", (0.042, 0.042, 0.042),
                           (cx - math.copysign(0.0155, cx), cy - math.copysign(0.0155, cy), cz - math.copysign(0.0155, cz - H / 2)),
                           bevel=0.012, segments=5, material=cmat)
                    if cy == front:
                        st.cylinder("parafuso_canto", 0.0028, 0.0016,
                                    (cx - math.copysign(0.011, cx), front - 0.0068, cz - math.copysign(0.011, cz - H / 2)),
                                    material=mats["chrome"], bevel=0.0005)
    handle = rec.get("alca", "couro" if fmt in ("cabecote", "combo") else "nenhuma")
    hy = rec.get("alca_y", 0.0)
    if lying and handle != "nenhuma":
        hy = min(hy, y_front - 0.07)
    span = min(0.25, W * 0.42)
    if handle == "couro":
        st.strap_arc("alca", span, 0.022, H + 0.007, 0.0058, 2.7, mats["handle"])
        for hx in (-span * 0.56, span * 0.56):
            st.box("ponta_alca", (0.045, 0.04, 0.008), (hx, hy, H + 0.004), bevel=0.003, material=mats["black_metal"])
        for o in bpy.data.objects:
            if o.name.startswith("alca") and hy:
                o.location.y = hy
    elif handle in ("cromo", "barra"):
        bar_span = min(0.22, W * 0.5)
        rise = 0.03 if handle == "cromo" else 0.022
        for hx in (-bar_span / 2, bar_span / 2):
            st.cylinder("alca_pe", 0.005, rise, (hx, hy, H + rise / 2), axis="Z", material=mats["chrome"])
        st.cylinder("alca_barra", 0.0065 if handle == "cromo" else 0.005, bar_span + 0.01, (0, hy, H + rise), axis="X",
                    material=mats["chrome"] if handle == "barra" else mats["chrome"])
    target = (0, front, H * 0.52)
    if lying:
        # Vista de cima e da frente: mira no meio do volume para caber o tampo.
        target = (0, 0.0, H * 0.5)
    return {"controls": p.controls, "size": (W, D, H), "front": front, "channel_leds": p.channel_leds,
            "target": target}
