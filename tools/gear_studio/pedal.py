# Nexus Gear Studio: família "pedal". Caixa de alumínio fundido pintada,
# botões, LED, pisante e entradas. O pedal fica deitado no chão: largura em X,
# comprimento em Y (a ponta do pé em -Y, perto da câmera) e altura em Z.
#
# Formatos ("formato" na receita):
#   classico  caixa inteira com tampa grande de pisar na metade da frente
#             (o overdrive verde)
#   compacto  base baixa com pedal basculante ocupando a frente, botões no
#             fundo (a distorção laranja)
#   padrao    caixa retangular com pisante de metal (o overdrive dourado)
#   redondo   caixa redonda (Fuzz Face)
#   wah       base com pedal basculante comprido (Cry Baby)
#   (os campos novos da modelagem fiel estão explicados antes de build())
#
# "logo" na receita: {"svg": arquivo em logos/, "tam_m": altura, "pos": [x, y]}
# desenha o símbolo do pedal no tampo, com as cores do próprio desenho.
# "estampa": {"svg": ...} é a arte do tampo inteiro (desenho em milímetros,
# do tamanho do tampo, com os cantos marcados em #FF00FF).
import math
import os

import bpy

import knobs
import studio as st


def mat_pedal_paint(name, color, metallic, rough=0.28, print_rings=None, texture=None):
    """Pintura de pedal: base colorida, pó metálico opcional e verniz
    brilhante por cima, com a leve "casca de laranja" da pintura a pó.
    print_rings: estampa de ondas (anéis finos em volta de um ponto, como a
    gota caindo na água) só no tampo: {"centro": [x, y] em metros, "passo_mm",
    "linha", "cor"}."""
    mat = bpy.data.materials.new(name)
    mat.use_nodes = True
    nodes, links = mat.node_tree.nodes, mat.node_tree.links
    bsdf = nodes["Principled BSDF"]
    bsdf.inputs["Base Color"].default_value = (*color, 1)
    if print_rings:
        m = st._math
        tc0 = nodes.new("ShaderNodeTexCoord")
        sep = nodes.new("ShaderNodeSeparateXYZ")
        links.new(tc0.outputs["Object"], sep.inputs[0])
        cx, cy = print_rings["centro"]
        dx = m(nodes, links, "SUBTRACT", sep.outputs["X"], cx)
        dy = m(nodes, links, "SUBTRACT", sep.outputs["Y"], cy)
        dist = m(nodes, links, "SQRT", m(nodes, links, "ADD", m(nodes, links, "MULTIPLY", dx, dx), m(nodes, links, "MULTIPLY", dy, dy)))
        pitch = print_rings.get("passo_mm", 2.6) / 1000.0
        # Passo que abre aos poucos para fora, como onda que se espalha.
        warped = m(nodes, links, "POWER", m(nodes, links, "DIVIDE", dist, pitch), 0.82)
        frac = m(nodes, links, "FRACT", warped)
        band = m(nodes, links, "ABSOLUTE", m(nodes, links, "SUBTRACT", frac, 0.5))
        line = m(nodes, links, "LESS_THAN", band, print_rings.get("linha", 0.12))
        # Só no tampo (normal para cima).
        geo = nodes.new("ShaderNodeNewGeometry")
        sn = nodes.new("ShaderNodeSeparateXYZ")
        links.new(geo.outputs["Normal"], sn.inputs[0])
        top = m(nodes, links, "GREATER_THAN", sn.outputs["Z"], 0.95)
        mask = m(nodes, links, "MULTIPLY", line, top)
        mix = nodes.new("ShaderNodeMix")
        mix.data_type = "RGBA"
        mix.inputs[6].default_value = (*color, 1)
        mix.inputs[7].default_value = (*print_rings["cor"], 1)
        links.new(mask, mix.inputs[0])
        links.new(mix.outputs[2], bsdf.inputs["Base Color"])
    bsdf.inputs["Metallic"].default_value = metallic
    bsdf.inputs["Roughness"].default_value = rough
    bsdf.inputs["Coat Weight"].default_value = 1.0
    bsdf.inputs["Coat Roughness"].default_value = 0.06
    tc = nodes.new("ShaderNodeTexCoord")
    noise = nodes.new("ShaderNodeTexNoise")
    noise.inputs["Scale"].default_value = 900.0
    noise.inputs["Detail"].default_value = 2.0
    links.new(tc.outputs["Object"], noise.inputs["Vector"])
    bump = nodes.new("ShaderNodeBump")
    bump.inputs["Strength"].default_value = 0.08
    bump.inputs["Distance"].default_value = 0.0002
    links.new(noise.outputs["Fac"], bump.inputs["Height"])
    links.new(bump.outputs["Normal"], bsdf.inputs["Coat Normal"])
    if texture in ("martelado", "enrugado"):
        # Pintura martelada (Fuzz Face, Tone Bender) ou enrugada (MXR antigo):
        # relevo na própria tinta, não só no verniz.
        tex = nodes.new("ShaderNodeTexNoise")
        tex.inputs["Scale"].default_value = 260.0 if texture == "martelado" else 1500.0
        tex.inputs["Detail"].default_value = 3.0 if texture == "martelado" else 6.0
        links.new(tc.outputs["Object"], tex.inputs["Vector"])
        tb = nodes.new("ShaderNodeBump")
        tb.inputs["Strength"].default_value = 0.45 if texture == "martelado" else 0.35
        tb.inputs["Distance"].default_value = 0.0005 if texture == "martelado" else 0.0002
        links.new(tex.outputs["Fac"], tb.inputs["Height"])
        links.new(tb.outputs["Normal"], bsdf.inputs["Normal"])
        links.new(tb.outputs["Normal"], bsdf.inputs["Coat Normal"])
        bsdf.inputs["Coat Weight"].default_value = 0.5
    if texture == "purpurina":
        # Pintura com brilho (sparkle): flocos maiores e mais espalhados.
        metallic = max(metallic, 0.5)
        bsdf.inputs["Metallic"].default_value = metallic
    if metallic > 0.05 and texture not in ("martelado", "enrugado"):
        # Pó metálico: pontinhos que brilham em ângulos diferentes.
        flake = nodes.new("ShaderNodeTexVoronoi")
        flake.inputs["Scale"].default_value = 1400.0 if texture == "purpurina" else 4000.0
        links.new(tc.outputs["Object"], flake.inputs["Vector"])
        fb = nodes.new("ShaderNodeBump")
        fb.inputs["Strength"].default_value = 0.6 if texture == "purpurina" else 0.25
        fb.inputs["Distance"].default_value = 0.0001
        links.new(flake.outputs["Distance"], fb.inputs["Height"])
        links.new(fb.outputs["Normal"], bsdf.inputs["Normal"])
    return mat


def mat_rubber(name, ridges=False):
    mat = bpy.data.materials.new(name)
    mat.use_nodes = True
    nodes, links = mat.node_tree.nodes, mat.node_tree.links
    bsdf = nodes["Principled BSDF"]
    bsdf.inputs["Base Color"].default_value = (0.012, 0.012, 0.012, 1)
    bsdf.inputs["Roughness"].default_value = 0.75
    tc = nodes.new("ShaderNodeTexCoord")
    bump = nodes.new("ShaderNodeBump")
    bump.inputs["Distance"].default_value = 0.0006
    if ridges:
        wave = nodes.new("ShaderNodeTexWave")
        wave.bands_direction = "Y"
        wave.wave_profile = "SIN"
        wave.inputs["Scale"].default_value = 120.0
        links.new(tc.outputs["Object"], wave.inputs["Vector"])
        bump.inputs["Strength"].default_value = 0.9
        links.new(wave.outputs["Fac"], bump.inputs["Height"])
    else:
        noise = nodes.new("ShaderNodeTexNoise")
        noise.inputs["Scale"].default_value = 1500.0
        links.new(tc.outputs["Object"], noise.inputs["Vector"])
        bump.inputs["Strength"].default_value = 0.3
        links.new(noise.outputs["Fac"], bump.inputs["Height"])
    links.new(bump.outputs["Normal"], bsdf.inputs["Normal"])
    return mat


def _flat_text(name, body, font, size, location, mat, align="CENTER", spacing=1.0, offset=0.0):
    return st.text(name, body, font, size, location, mat, extrude=0.00003, align=align, spacing=spacing,
                   offset=offset, rotation=(0, 0, 0))


LOGOS = os.path.join(os.path.dirname(os.path.abspath(__file__)), "logos")


def _logo(spec, height, location):
    """Logo ou estampa impressa no tampo: o desenho vetorial (cada forma com a
    cor de preenchimento do arquivo; formas de cor nova ficam um fio acima das
    anteriores), com a altura dada e centrado em location. Formas na cor
    #FF00FF são só marcas de enquadramento: definem o tamanho do desenho (a
    estampa que cobre o tampo inteiro marca os cantos) e não aparecem.
    Devolve (objetos, largura)."""
    import addon_utils
    addon_utils.enable("io_curve_svg", default_set=False)
    before = set(bpy.data.objects)
    bpy.ops.import_curve.svg(filepath=os.path.join(LOGOS, spec["svg"]))
    curves = sorted((o for o in bpy.data.objects if o not in before), key=lambda o: o.name)
    for o in curves:
        o.data.dimensions = "2D"
        o.data.fill_mode = "BOTH"
        o.data.resolution_u = 24
    dg = bpy.context.evaluated_depsgraph_get()
    dg.update()
    parts, marks = [], []
    for o in curves:
        me = bpy.data.meshes.new_from_object(o.evaluated_get(dg))
        me.transform(o.matrix_world)
        colors = [tuple(s.material.diffuse_color[:3]) for s in o.material_slots if s.material]
        bpy.data.objects.remove(o)
        if colors and colors[0][0] > 0.99 and colors[0][1] < 0.01 and colors[0][2] > 0.99:
            marks.append(me)
        else:
            parts.append((me, colors))
    frame = [me for me in marks] or [me for me, _ in parts]
    xs = [v.co.x for me in frame for v in me.vertices]
    ys = [v.co.y for me in frame for v in me.vertices]
    k = height / (max(ys) - min(ys))
    cx, cy = (max(xs) + min(xs)) / 2, (max(ys) + min(ys)) / 2
    x, y, z = location
    inks, objs, layer, last = {}, [], 0, None
    for me, colors in parts:
        if colors != last:
            layer, last = layer + 1, colors
        for v in me.vertices:
            v.co = ((v.co.x - cx) * k, (v.co.y - cy) * k, layer * 0.000004)
        me.materials.clear()
        for c in colors:
            if c not in inks:
                inks[c] = st.mat_paint("logo_tinta_%d" % len(inks), c, rough=0.4)
            me.materials.append(inks[c])
        obj = bpy.data.objects.new("logo_%03d" % len(objs), me)
        bpy.context.collection.objects.link(obj)
        obj.location = (x, y, z)
        objs.append(obj)
    for me in marks:
        bpy.data.meshes.remove(me)
    return objs, (max(xs) - min(xs)) * k


def _fit(obj, width):
    """Encolhe o texto se ele passar da largura disponível."""
    bpy.context.view_layer.update()
    if obj.dimensions.x > width:
        f = width / obj.dimensions.x
        obj.scale = (f, f, f)


def _frame_on_top(name, width, length, radius, thickness, location, mat):
    """Moldura impressa (retângulo de cantos arredondados) deitada no topo."""
    obj = st.rounded_rect_curve(name, width, length, radius, thickness, location, mat)
    obj.rotation_euler = (math.radians(-90), 0, 0)
    obj.scale = (1, 0.08, 1)  # achata o cordão (eixo normal ao desenho): vira linha impressa
    return obj


def _led(name, location, mats):
    x, y, z = location
    st.cylinder(name + "_aro", 0.0042, 0.0016, (x, y, z + 0.0008), material=mats["chrome"], axis="Z", bevel=0.0004)
    bpy.ops.mesh.primitive_uv_sphere_add(radius=0.0031, location=(x, y, z + 0.0016), segments=32, ring_count=16)
    dome = bpy.context.active_object
    dome.name = name
    dome.scale = (1, 1, 0.8)
    dome.data.materials.append(mats["led"])
    st.smooth(dome, 80)


def _metal_stomp(name, location, mats):
    """Pisante de metal: porca sextavada, rosca e cabeça com sulcos."""
    x, y, z = location
    st.cylinder(name + "_porca", 0.0105, 0.0030, (x, y, z + 0.0015), vertices=6, material=mats["chrome"], axis="Z", bevel=0.0006)
    st.cylinder(name + "_rosca", 0.0072, 0.0060, (x, y, z + 0.0055), vertices=48, material=mats["chrome_dark"], axis="Z")
    st.cylinder(name + "_cabeca", 0.0088, 0.0065, (x, y, z + 0.0115), vertices=64, material=mats["chrome"], axis="Z", bevel=0.0018)
    for i, r in enumerate((0.0062, 0.0042, 0.0022)):
        bpy.ops.mesh.primitive_torus_add(major_radius=r, minor_radius=0.00045, location=(x, y, z + 0.0149))
        ring = bpy.context.active_object
        ring.name = f"{name}_sulco{i}"
        ring.data.materials.append(mats["chrome_dark"])


def _pedal_slider(idx, ctl, x, y_mid, travel, top, mats, text_color):
    """Deslizante do equalizador no tampo: rasgo, escala de traços e o cursor
    (pendurado num ponto que a ferramenta corre para frente e para trás)."""
    st.box(f"rasgo_{idx}", (0.0024, travel + 0.0062, 0.0008), (x, y_mid, top + 0.0001), bevel=0.0003, material=mats["hole"])
    for k in range(7):
        ty = y_mid - travel / 2 + travel * k / 6
        st.box(f"eq_traco_{idx}_{k}", (0.0026 if k in (0, 3, 6) else 0.0015, 0.0004, 0.00006),
               (x - 0.0042, ty, top + 0.00005), material=mats["ink"])
    pivot = bpy.data.objects.new(f"giro_{ctl['id']}", None)
    bpy.context.collection.objects.link(pivot)
    pivot.location = (x, y_mid, top)
    bpy.context.view_layer.update()
    cap = st.box(f"cursor_{idx}", (0.0062, 0.0050, 0.0058), (x, y_mid, top + 0.0031), bevel=0.0010, material=mats["slider"])
    line = st.box(f"cursor_traco_{idx}", (0.0062 * 0.84, 0.0006, 0.0002), (x, y_mid, top + 0.0061), material=mats["pointer"])
    for part in (cap, line):
        part.parent = pivot
        part.matrix_parent_inverse = pivot.matrix_world.inverted()
    st.label(ctl["rotulo"], (x, y_mid - travel / 2 - 0.0062, top + 0.0001), 0.0030, up="Y", color=text_color, spacing=1.05)
    return {"id": ctl["id"], "pivot": pivot, "tipo": "deslizante", "eixo": "y", "z0": y_mid, "curso": travel,
            "center": (x, y_mid, top + 0.0031), "w": 0.0082, "h": travel + 0.0064, "depth": 0.0064}


def _jacks(W, y, z, mats):
    for side in (-1, 1):
        x = side * (W / 2 + 0.0012)
        st.cylinder("entrada_porca", 0.0065, 0.0024, (x, y, z), vertices=6, material=mats["chrome"], axis="X", bevel=0.0004)
        st.cylinder("entrada_furo", 0.0032, 0.0004, (x + side * 0.0013, y, z), material=mats["hole"], axis="X")


# ------------------------------------------------------------------ peças novas (modelagem fiel)
# Tudo opcional na receita; receita sem estes campos sai igual a antes.
#
#   "formato": "redondo"  caixa redonda (Fuzz Face): largura = diâmetro
#              "wah"      base fundida com pedal basculante comprido por cima,
#                         sem botões no tampo ("talao_m": faixa lisa atrás do
#                         basculante, onde ficam botões que sobraram)
#   "cunha": {"frente_m": altura}   tampo inclinado, mais alto atrás (RAT, Rangemaster)
#   "disposicao": "boss"  botões em fila no alto, o do meio mais baixo (3 botões),
#                         o quarto menor (seletor) e o LED CHECK no alto, no centro
#   "basculante_ang" (compacto, padrão -2) e "pisante_ang" (clássico, padrão -1,5):
#                         inclinação em graus; 0 deixa reto, de frente para a câmera
#   "pintura_lados": {...}  laterais de outra cor (o tampo leva "pintura")
#   "pintura": {"textura": "martelado" | "enrugado" | "purpurina"}
#   "led_cor": cor do vidro do LED apagado (azul, amarelo...)
#   "pisante": {"estilo": "metal" | "redondo" | "pequeno" | "nenhum", "pos": [x, y],
#               "raio_m": área de clique quando não há pisante}
#              no clássico: "almofada" | "placa_moldura" (TS9) | "placa_pequena" (TS808)
#   "etiqueta_estilo": "placa_prata" no clássico (placa de metal do nome)
#   "modelo_oculto": true  não escreve o nome sozinho (vai em "textos")
#   "modelo_cor": cor só do nome (no compacto)
#   "jacks_topo": true      entradas na face de trás, e não nas laterais
#   "botoes": cada um pode ter "fixo": true (desenhado, fora dos controles),
#             "tipo": "deslizante" (com "curso_m"), "estilo": {...} (troca o
#             botao_estilo só dele), "concentrico": true (botão duplo),
#             "rotulo_pos": "cima" | "baixo" | "nenhum", "rotulo_cor", "rotulo_m"
#   "botao_estilo": "forma": "galinha" (chicken head), "material": "transparente"
#   "pecas": peças fixas desenhadas no tampo:
#       {"tipo": "minichave", "pos", "rotulo"?, "lado": "cima"|"baixo"}  alavanca
#       {"tipo": "apertar", "pos", "diam_m"?, "cor"?, "rotulo"?}          botão de apertar
#       {"tipo": "chave_deslizante", "pos", "rotulo"?, "vertical"?}       chave de 3 posições
#       {"tipo": "trimpot", "pos"}                                         ajuste pequeno
#       {"tipo": "led", "pos", "cor"?}                                     LED a mais
#       {"tipo": "leds", "pos", "n", "passo_m", "cor"?}                    fileira de LEDs
#       {"tipo": "placa", "pos", "tam_m": [w, l], "cor", "forma": "retangulo"|"triangulo",
#        "metal": true?, "frisos": true?, "raio_m"?}                       placa no tampo
#       {"tipo": "pisante", "pos", "estilo"}                               pisante a mais
#       {"tipo": "rotulo", "pos", "rotulo"}                                 só o letreiro (SAÍDA, ENTRADA)
#   "paineis": áreas pintadas de outra cor no tampo, desenhadas como chapa fina:
#       {"pontos": [[x, y], ...]} ou {"pos": [x, y], "tam_m": [w, l], "raio_m"},
#       "cor", "metal": true (escovado)?
#   "textos": [{"texto", "pos", "tam_m", "cor", "fonte": "label"|"script"|"mesa"|"ui",
#               "espaco", "ang"}]  letreiros com a cor de cada um
def _mat_cache(mats, key, factory):
    if key not in mats:
        mats[key] = factory()
    return mats[key]


def _color_key(c):
    return tuple(round(v, 4) for v in c)


def _ink(mats, color):
    return _mat_cache(mats, ("tinta",) + _color_key(color), lambda: st.mat_paint("tinta_%d" % len(mats), color, rough=0.42))


def _glass(name, color):
    """Plástico transparente (botão de acrílico)."""
    mat = st.mat_plastic(name, color, rough=0.05, coat=1.0)
    bsdf = mat.node_tree.nodes["Principled BSDF"]
    bsdf.inputs["Transmission Weight"].default_value = 0.85
    bsdf.inputs["IOR"].default_value = 1.49
    return mat


def _flat_poly(name, pts, z, mat):
    """Chapa plana (polígono) deitada no tampo, na altura z."""
    me = bpy.data.meshes.new(name)
    me.from_pydata([(x, y, z) for x, y in pts], [], [list(range(len(pts)))])
    me.update()
    obj = bpy.data.objects.new(name, me)
    bpy.context.collection.objects.link(obj)
    obj.data.materials.append(mat)
    return obj


def _boss_layout(rec):
    """Disposição Boss: fila de botões no alto, o do meio mais baixo quando são
    três, o quarto (seletor) menor; LED CHECK no alto, no centro."""
    ks = rec["botoes"]
    n = len(ks)
    rows = {1: [(0.0, 0.66)], 2: [(-0.5, 0.66), (0.5, 0.66)], 3: [(-0.6, 0.68), (0.0, 0.56), (0.6, 0.68)],
            4: [(-0.69, 0.66), (-0.23, 0.66), (0.23, 0.66), (0.69, 0.66)]}.get(n)
    if rows:
        for k, p in zip(ks, rows):
            k.setdefault("pos", list(p))
        if n == 4:
            ks[3].setdefault("escala", 0.78)
            for k in ks[:3]:
                k.setdefault("escala", 0.92)
    rec.setdefault("led", [0.0, 0.9])


def _toggle(name, loc, mats, up=True):
    """Minichave de alavanca: porca sextavada, bucha e alavanca cromada inclinada."""
    x, y, z = loc
    st.cylinder(name + "_porca", 0.0042, 0.0016, (x, y, z + 0.0008), vertices=6, material=mats["chrome"], axis="Z", bevel=0.0003)
    st.cylinder(name + "_bucha", 0.0026, 0.0038, (x, y, z + 0.0034), vertices=32, material=mats["chrome_dark"], axis="Z")
    lever = st.cylinder(name + "_alavanca", 0.00095, 0.0105, (0, 0, 0), vertices=24, material=mats["chrome"], axis="Z")
    s = 1 if up else -1
    ang = math.radians(22)
    lever.rotation_euler = (-s * ang, 0, 0)
    lever.location = (x, y + s * 0.00525 * math.sin(ang), z + 0.0052 + 0.00525 * math.cos(ang))
    bpy.ops.mesh.primitive_uv_sphere_add(radius=0.0016, location=(x, y + s * 0.0105 * math.sin(ang), z + 0.0052 + 0.0105 * math.cos(ang)),
                                         segments=24, ring_count=12)
    tip = bpy.context.active_object
    tip.name = name + "_ponta"
    tip.data.materials.append(mats["chrome"])
    st.smooth(tip, 80)


def _push(name, loc, mats, diam=0.008, color=None):
    """Botão de apertar: aro cromado e capa colorida."""
    x, y, z = loc
    st.cylinder(name + "_aro", diam / 2 + 0.0012, 0.0014, (x, y, z + 0.0007), material=mats["chrome"], axis="Z", bevel=0.0003)
    cap = st.mat_plastic(name + "_capa", color or (0.02, 0.02, 0.02), rough=0.35, coat=0.3)
    st.cylinder(name, diam / 2, 0.0034, (x, y, z + 0.0024), material=cap, axis="Z", bevel=0.0007)


def _slide_switch(name, loc, mats, vertical=True):
    """Chave deslizante de 3 posições: placa preta com rasgo e o cursor."""
    x, y, z = loc
    w, l = (0.0055, 0.0125) if vertical else (0.0125, 0.0055)
    st.box(name + "_placa", (w, l, 0.0009), (x, y, z + 0.00045), bevel=0.0003, material=mats["plastic"])
    st.box(name + "_rasgo", (w * 0.42, l * 0.72, 0.0003) if vertical else (w * 0.72, l * 0.42, 0.0003), (x, y, z + 0.0009),
           material=mats["hole"])
    st.box(name + "_cursor", (0.0022, 0.0030, 0.0030), (x, y, z + 0.0020), bevel=0.0004, material=mats["chrome_dark"])


def _trimpot(name, loc, mats):
    x, y, z = loc
    st.cylinder(name, 0.0026, 0.0018, (x, y, z + 0.0009), material=st.mat_plastic(name + "_m", (0.85, 0.85, 0.82), rough=0.4),
                axis="Z", bevel=0.0003)
    st.box(name + "_fenda", (0.0034, 0.0006, 0.0004), (x, y, z + 0.0018), material=mats["hole"])


def _led_color(name, loc, mats, color=None, r=0.0031):
    x, y, z = loc
    st.cylinder(name + "_aro", r * 1.35, 0.0016, (x, y, z + 0.0008), material=mats["chrome"], axis="Z", bevel=0.0004)
    bpy.ops.mesh.primitive_uv_sphere_add(radius=r, location=(x, y, z + 0.0016), segments=32, ring_count=16)
    dome = bpy.context.active_object
    dome.name = name
    dome.scale = (1, 1, 0.8)
    mat = mats["led"] if color is None else _mat_cache(
        mats, ("led",) + _color_key(color), lambda: st.mat_plastic("led_%d" % len(mats), color, rough=0.06, coat=1.0))
    dome.data.materials.append(mat)
    st.smooth(dome, 80)


def _round_stomp(name, loc, mats):
    """Pisante redondo grande de metal (OD808): disco cromado com sulcos."""
    x, y, z = loc
    st.cylinder(name + "_porca", 0.0105, 0.0030, (x, y, z + 0.0015), vertices=6, material=mats["chrome"], axis="Z", bevel=0.0006)
    st.cylinder(name + "_cabeca", 0.0125, 0.0055, (x, y, z + 0.0058), vertices=96, material=mats["chrome"], axis="Z", bevel=0.0016)
    for i, r in enumerate((0.0095, 0.0068, 0.0040)):
        bpy.ops.mesh.primitive_torus_add(major_radius=r, minor_radius=0.00045, location=(x, y, z + 0.0086))
        ring = bpy.context.active_object
        ring.name = f"{name}_sulco{i}"
        ring.data.materials.append(mats["chrome_dark"])


def _small_stomp(name, loc, mats):
    """Pisante pequeno (Fuzz Face, Klon): mesma peça, menor."""
    x, y, z = loc
    st.cylinder(name + "_porca", 0.0080, 0.0026, (x, y, z + 0.0013), vertices=6, material=mats["chrome"], axis="Z", bevel=0.0005)
    st.cylinder(name + "_rosca", 0.0055, 0.0050, (x, y, z + 0.0046), vertices=48, material=mats["chrome_dark"], axis="Z")
    st.cylinder(name + "_cabeca", 0.0068, 0.0055, (x, y, z + 0.0095), vertices=64, material=mats["chrome"], axis="Z", bevel=0.0015)


def _stomp(name, style, loc, mats):
    """Desenha o pisante e devolve (centro, raio) da área de pisar."""
    x, y, z = loc
    if style == "nenhum":
        return (x, y, z + 0.002), 0.008
    if style == "redondo":
        _round_stomp(name, loc, mats)
        return (x, y, z + 0.0086), 0.0125
    if style == "pequeno":
        _small_stomp(name, loc, mats)
        return (x, y, z + 0.0122), 0.0068
    _metal_stomp(name, loc, mats)
    return (x, y, z + 0.012), 0.012


def _ridged_plate(name, size, loc, mats, ridges=True, tilt=0.0):
    """Placa de metal de pisar com frisos (Tube Screamer)."""
    w, l, t = size
    x, y, z = loc
    plate = st.box(name, (w, l, t), (x, y, z), bevel=min(0.0018, t * 0.45), segments=4, material=mats["chrome"])
    plate.rotation_euler = (math.radians(tilt), 0, 0)
    if ridges:
        n = max(4, int(l / 0.0035))
        for i in range(n):
            ry = y - l / 2 + l * (i + 0.5) / n
            st.box(f"{name}_friso{i}", (w * 0.86, 0.0011, 0.0007), (x, ry, z + t / 2 + 0.0002), bevel=0.0003,
                   material=mats["chrome_dark"])
    return plate


def _chicken_knob(name, r, h, mats):
    """Botão "cabeça de galinha": pé redondo baixo e asa comprida com o traço."""
    pivot = bpy.data.objects.new(f"giro_{name}", None)
    bpy.context.collection.objects.link(pivot)
    bpy.context.view_layer.update()
    parts = [st.cylinder(f"pe_{name}", r * 0.62, h * 0.45, (0, -h * 0.225, 0), material=mats["body"], bevel=0.0006)]
    wing = st.box(f"asa_{name}", (r * 0.62, h * 0.62, r * 2.05), (0, -h * 0.45 - h * 0.31, r * 0.18), bevel=r * 0.22, segments=4,
                  material=mats["body"])
    parts.append(wing)
    parts.append(st.box(f"traco_{name}", (r * 0.08, 0.0005, r * 0.85), (0, -h * 0.45 - h * 0.62 - 0.0002, r * 0.62),
                        material=mats["pointer"]))
    for p in parts:
        p.parent = pivot
        p.matrix_parent_inverse = pivot.matrix_world.inverted()
    return pivot


def _knob_mats(mats, style):
    key = ("botao", _color_key(style["corpo"]), _color_key(style.get("tampa", (0.8, 0.8, 0.8))),
           _color_key(style.get("traco", (0.95, 0.95, 0.92))), style.get("material", ""))
    if key not in mats:
        n = len(mats)
        body = (_glass(f"botao_{n}", style["corpo"]) if style.get("material") == "transparente"
                else st.mat_plastic(f"botao_{n}", style["corpo"], rough=0.3, coat=0.4))
        mats[key] = {"body": body,
                     "cap": st.mat_brushed(f"tampa_{n}", style.get("tampa", (0.8, 0.8, 0.8)), rough=0.18, radial_axis="Y"),
                     "pointer": st.mat_paint(f"traco_{n}", style.get("traco", (0.95, 0.95, 0.92)), rough=0.4)}
    return mats[key]


def _place_knob(c, rec, W, L, top, mats):
    """Botão giratório (funcional ou fixo) e o rótulo. Devolve o controle."""
    style = dict(rec["botao_estilo"], **c.get("estilo", {}))
    sc = c.get("escala", 1.0)
    r = style["diametro_m"] / 2 * sc
    h = style["altura_m"] * sc ** 0.5
    x, y = c["pos"][0] * W / 2, c["pos"][1] * L / 2
    km = _knob_mats(mats, style)
    if style.get("forma") == "galinha":
        pivot = _chicken_knob(c["id"], r, h, km)
        outer = r * 1.05
    else:
        pivot = knobs.knob(c["id"], r, h, km, skirt=style.get("saia", False), cap=style.get("com_tampa", False))
        outer = r * (1.28 if style.get("saia") else 1.0)
    knobs.lay_on_top(pivot, (x, y, top))
    hh = h + (0.0025 if style.get("saia", False) else 0.0)
    if c.get("concentrico"):
        # Botão duplo (MT-2): um menor por cima, no mesmo eixo.
        inner = knobs.knob(c["id"] + "_dentro", r * 0.6, h * 0.55, km, cap=False, side_notch=False, flutes=18)
        knobs.lay_on_top(inner, (x, y, top + hh))
        hh += h * 0.55
    pos = c.get("rotulo_pos", "baixo" if rec.get("rotulo_embaixo") else "cima")
    if c.get("rotulo") and pos != "nenhum":
        ly = y - outer - 0.0055 if pos == "baixo" else y + outer + 0.0055
        st.label(c["rotulo"], (x, ly, top + 0.0001), c.get("rotulo_m", rec.get("rotulo_m", 0.0052)), up="Y",
                 color=c.get("rotulo_cor", rec["texto_cor"]), spacing=1.15)
    return {"id": c["id"], "pivot": pivot, "center": (x, y, top + hh / 2), "outer_r": outer, "depth": hh, "axis": (0.0, 0.0, 1.0)}


def _pieces(rec, W, L, top, mats):
    """Peças fixas do tampo (chaves, botões de apertar, LEDs, placas, pisantes a mais)."""
    for i, p in enumerate(rec.get("pecas", [])):
        x, y = p["pos"][0] * W / 2, p["pos"][1] * L / 2
        kind = p["tipo"]
        nm = f"peca{i}_{kind}"
        lab_dy = 0.0
        if kind == "minichave":
            _toggle(nm, (x, y, top), mats, up=p.get("lado", "cima") == "cima")
            lab_dy = 0.0085
        elif kind == "apertar":
            d = p.get("diam_m", 0.008)
            _push(nm, (x, y, top), mats, d, p.get("cor"))
            lab_dy = d / 2 + 0.0055
        elif kind == "chave_deslizante":
            _slide_switch(nm, (x, y, top), mats, p.get("vertical", True))
            lab_dy = 0.0105
        elif kind == "trimpot":
            _trimpot(nm, (x, y, top), mats)
            lab_dy = 0.0055
        elif kind == "led":
            _led_color(nm, (x, y, top), mats, p.get("cor"), p.get("raio_m", 0.0031))
            lab_dy = 0.0065
        elif kind == "leds":
            n, step = p["n"], p.get("passo_m", 0.0062)
            cols = p.get("cores") or [p.get("cor")] * n
            for k in range(n):
                _led_color(f"{nm}_{k}", (x + (k - (n - 1) / 2) * step, y, top), mats, cols[k], p.get("raio_m", 0.0021))
        elif kind == "placa":
            w, l = p["tam_m"]
            mat = (st.mat_brushed(nm + "_m", p.get("cor", (0.8, 0.8, 0.8)), rough=0.22) if p.get("metal")
                   else _mat_cache(mats, ("placa",) + _color_key(p.get("cor", (0.01, 0.01, 0.01))),
                                   lambda: st.mat_plastic("placa_%d" % len(mats), p.get("cor", (0.01, 0.01, 0.01)), rough=0.4, coat=0.2)))
            t = p.get("espessura_m", 0.0012)
            if p.get("forma") == "triangulo":
                # Ponta para trás, base na frente.
                tri = [(x - w / 2, y - l / 2), (x + w / 2, y - l / 2), (x, y + l / 2)]
                obj = _flat_poly(nm, tri, 0.0, mat)
                mod = obj.modifiers.new("espessura", "SOLIDIFY")
                mod.thickness = t
                mod.offset = 1.0
                st.apply_modifiers(obj)
                obj.location.z = top
                bev = obj.modifiers.new("chanfro", "BEVEL")
                bev.width = 0.0005
                bev.segments = 3
                st.apply_modifiers(obj)
            else:
                st.box(nm, (w, l, t), (x, y, top + t / 2), bevel=min(p.get("raio_m", 0.0008), t * 0.45), material=mat)
            if p.get("frisos"):
                n = max(3, int(l / 0.0032))
                for k in range(n):
                    ry = y - l / 2 + l * (k + 0.5) / n
                    fw = w * 0.82 * ((1 - (ry - (y - l / 2)) / l) if p.get("forma") == "triangulo" else 1.0)
                    if fw > 0.003:
                        st.box(f"{nm}_friso{k}", (fw, 0.0010, 0.0006), (x, ry, top + t + 0.0002), bevel=0.0002,
                               material=mats["rubber"] if not p.get("metal") else mats["chrome_dark"])
        elif kind == "pisante":
            _stomp(nm, p.get("estilo", "metal"), (x, y, top), mats)
        elif kind == "rotulo":
            pass  # só o letreiro (a tela escreve, traduzido)
        if p.get("rotulo"):
            ly = y + lab_dy if p.get("rotulo_pos", "cima") == "cima" else y - lab_dy
            st.label(p["rotulo"], (x, ly, top + 0.0001), p.get("rotulo_m", rec.get("rotulo_m", 0.0052) * 0.8), up="Y",
                     color=p.get("rotulo_cor", rec["texto_cor"]), spacing=1.1)


def _panels(rec, W, L, top, mats):
    for i, p in enumerate(rec.get("paineis", [])):
        color = p["cor"]
        mat = (st.mat_brushed(f"painel{i}_m", color, rough=p.get("aspereza", 0.24)) if p.get("metal")
               else st.mat_plastic(f"painel{i}_m", color, rough=p.get("aspereza", 0.32), coat=p.get("verniz", 0.6)))
        z = top + 0.00003 + i * 0.000004
        if "pontos" in p:
            # Dentro da parte lisa do tampo (sem passar do chanfro da borda).
            mx = W / 2 - rec.get("chanfro_m", 0.0045) * 0.8
            my = L / 2 - rec.get("chanfro_m", 0.0045) * 0.8
            pts = [(max(-mx, min(mx, u * W / 2)), max(-my, min(my, v * L / 2))) for u, v in p["pontos"]]
            _flat_poly(f"painel{i}", pts, z, mat)
        else:
            w, l = p["tam_m"]
            x, y = p["pos"][0] * W / 2, p["pos"][1] * L / 2
            st.box(f"painel{i}", (w, l, 0.0001), (x, y, z - 0.00005), bevel=p.get("raio_m", 0.0), segments=6, material=mat)


def _texts(rec, W, L, top, fonts, mats):
    for i, t in enumerate(rec.get("textos", [])):
        x, y = t["pos"][0] * W / 2, t["pos"][1] * L / 2
        obj = _flat_text(f"texto{i}", t["texto"], fonts[t.get("fonte", "label")], t.get("tam_m", 0.006),
                         (x, y, top + 0.0003), _ink(mats, t.get("cor", rec["texto_cor"])), align=t.get("alinhar", "CENTER"),
                         spacing=t.get("espaco", 1.0), offset=t.get("grossura", 0.0))
        if t.get("ang"):
            obj.rotation_euler = (0, 0, math.radians(t["ang"]))
        if t.get("largura_max"):
            _fit(obj, t["largura_max"] * W / 2)


def _bend(objs, split, ty, th, zc, hh, base_z):
    """Arte achatada no tampo: o que fica atrás de split vai na base (altura
    base_z); o resto acompanha o pedal basculante inclinado (ângulo th, centro
    ty/zc, meia espessura hh)."""
    for o in objs:
        vs = o.data.vertices
        if not len(vs):
            continue
        if sum(v.co.y for v in vs) / len(vs) + o.location.y > split:
            o.location.z = base_z + 0.00005
            continue
        oy = o.location.y
        for v in vs:
            dy = v.co.y + oy - ty
            dz = hh + 0.00005 + v.co.z
            v.co = (v.co.x, ty + dy * math.cos(th) - dz * math.sin(th) - oy, zc + dy * math.sin(th) + dz * math.cos(th))


def _wedge(objs_top, body_objs, H, Hf, L, label_from, controls, points):
    """Caixa em cunha: tampo mais baixo na frente (Hf) que atrás (H). O corpo é
    esticado; as peças do tampo descem com a rampa e inclinam junto."""
    slope = (H - Hf) / L
    th = math.atan(slope)

    def dz(y):
        return (y - L / 2) * slope

    for o in body_objs:
        ox, oy, oz = o.location
        for v in o.data.vertices:
            # Coordenadas do mundo: o fundo fica no chão, o tampo desce na frente.
            hy = H + dz(v.co.y + oy)
            v.co.z = (v.co.z + oz) * hy / H - oz
    for o in objs_top:
        if o.parent is not None:
            continue
        y = o.location.y
        o.location.z += dz(y)
        if o.type == "MESH" and o.name.startswith("logo_"):
            # Estampa: segue a rampa vértice por vértice.
            for v in o.data.vertices:
                v.co.z += dz(v.co.y + y) - dz(y)
            continue
        o.rotation_euler.x += th
    for lab in st.LABELS[label_from:]:
        x, y, z = lab["local"]
        lab["local"] = (x, y, z + dz(y))
    for c in controls:
        x, y, z = c["center"]
        c["center"] = (x, y, z + dz(y))
        if "axis" in c:
            c["axis"] = (0.0, -math.sin(th), math.cos(th))
    out = []
    for p in points:
        if p is None:
            out.append(None)
            continue
        (x, y, z), r = p
        out.append(((x, y, z + dz(y)), r))
    return out


def build(rec, fonts):
    W = rec["medidas_m"]["largura"]
    L = rec["medidas_m"]["comprimento"]
    H = rec["medidas_m"]["altura"]
    shape = rec["formato"]
    paint = rec["pintura"]
    if rec.get("disposicao") == "boss":
        _boss_layout(rec)
    mats = {
        "paint": mat_pedal_paint("pintura", paint["cor"], paint.get("metalico", 0.0), paint.get("aspereza", 0.28),
                                 rec.get("estampa_ondas"), paint.get("textura")),
        "ink": st.mat_paint("tinta", rec["texto_cor"], rough=0.45),
        "ink2": st.mat_paint("tinta2", rec.get("texto_cor2", rec["texto_cor"]), rough=0.45),
        "chrome": st.mat_metal("cromo", (0.92, 0.92, 0.93), rough=0.08),
        "chrome_dark": st.mat_metal("cromo_escuro", (0.55, 0.55, 0.56), rough=0.2),
        "hole": st.mat_paint("furo", (0.0, 0.0, 0.0), rough=0.9),
        # LED apagado (vidro vermelho escuro). Aceso, a tela desenha o brilho
        # por cima, e assim liga e desliga sem precisar de dois renders.
        "led": st.mat_plastic("led_vidro", tuple(rec.get("led_cor", (0.30, 0.012, 0.008))), rough=0.06, coat=1.0),
        "rubber": mat_rubber("borracha"),
        "rubber_ridges": mat_rubber("borracha_frisos", ridges=True),
        "plastic": st.mat_plastic("plastico", (0.015, 0.015, 0.015), rough=0.42),
        "knob": st.mat_plastic("botao", rec["botao_estilo"]["corpo"], rough=0.3, coat=0.4),
        "cap": st.mat_brushed("tampa", rec["botao_estilo"].get("tampa", (0.8, 0.8, 0.8)), rough=0.18, radial_axis="Y"),
        "pointer": st.mat_paint("traco", rec["botao_estilo"].get("traco", (0.95, 0.95, 0.92)), rough=0.4),
    }
    if rec.get("pintura_lados"):
        pl = rec["pintura_lados"]
        mats["sides"] = (st.mat_metal("lados", pl["cor"], rough=pl.get("aspereza", 0.12)) if pl.get("cromo")
                         else mat_pedal_paint("lados", pl["cor"], pl.get("metalico", 0.0), pl.get("aspereza", 0.28), None,
                                              pl.get("textura")))
    body_mat = mats.get("sides", mats["paint"])
    label_from = len(st.LABELS)
    first_objs = set(bpy.data.objects)

    top = H  # altura da face onde ficam os botões
    body_objs = []
    if shape == "compacto":
        base_h = H * rec.get("base_rel", 0.55)
        st.box("base", (W, L, base_h), (0, 0, base_h / 2), bevel=0.004, segments=6, material=mats["paint"])
        # Pedal basculante: cobre a frente, mais alto, com borracha frisada.
        tl = L * rec.get("basculante_rel", 0.64)
        ty = -L / 2 + tl / 2
        treadle = st.box("basculante", (W - 0.001, tl, H - base_h + 0.006), (0, ty, base_h + (H - base_h) / 2 - 0.003),
                         bevel=0.006, segments=6, material=mats["paint"])
        treadle.rotation_euler = (math.radians(rec.get("basculante_ang", -2.0)), 0, 0)
        # Borracha: a frente fica sempre no mesmo lugar; menor, sobra espaço para a arte.
        rub = rec.get("borracha_rel", 0.62)
        st.box("borracha", (W * rec.get("borracha_larg", 0.80), tl * rub, 0.0025), (0, ty - tl * 0.35 + tl * rub / 2, H + 0.0005),
               bevel=0.001, material=mats["rubber_ridges"])
        # Parafuso de trocar a bateria, na ponta.
        st.cylinder("parafuso_bateria", 0.0055, 0.006, (0, -L / 2 - 0.001, base_h * 0.55), vertices=48,
                    material=mats["chrome"], axis="Y", bevel=0.0012)
        top = base_h
        if not rec.get("modelo_oculto"):
            # Nome: tamanho e deslocamento ajustáveis (nomes longos não podem encostar no LED).
            # O engrossamento da letra quebra o contorno do B e do R em letra pequena.
            size = rec.get("modelo_m", 0.0085)
            name_x = rec.get("modelo_x", 0.0) * W / 2
            if "logo" in rec:
                # Logo na ponta direita da faixa; o nome ocupa o espaço entre o LED e o logo.
                lg = rec["logo"]
                objs, lw = _logo(lg, lg.get("tam_m", 0.011), (0, ty + tl * 0.385, H + 0.0004))
                right = W / 2 - 0.004
                for o in objs:
                    o.location.x = right - lw / 2
                left = (rec["led"][0] * W / 2 + 0.006) if "led" in rec and rec.get("led_no_basculante") else -W / 2 + 0.004
                name_x = (left + right - lw - 0.002) / 2
            name = _flat_text("modelo", rec["modelo"], fonts["label"], size,
                              (name_x, ty + tl * 0.38, H + 0.0004),
                              _ink(mats, rec["modelo_cor"]) if rec.get("modelo_cor") else mats["ink"], spacing=1.1,
                              offset=0.0002 if size >= 0.0085 else 0.0)
            if "logo" in rec:
                _fit(name, right - lw - 0.002 - left)
            else:
                left = (rec["led"][0] * W / 2 + 0.0065) if rec.get("led_no_basculante") else -W / 2 + 0.005
                right = W / 2 - 0.005
                _fit(name, 2 * min(name_x - left, right - name_x))
            if rec.get("tipo_texto"):
                _flat_text("tipo", rec["tipo_texto"], fonts["label"], 0.0062, (0, ty - tl * 0.40, H + 0.0004), mats["ink"],
                           spacing=1.4)
    elif shape == "wah":
        # Base fundida baixa e o pedal basculante comprido por cima (ponta do pé
        # levantada), com borracha frisada; atrás, uma faixa lisa ("talao_m").
        base_h = rec.get("base_m", H * 0.5)
        ledge = rec.get("talao_m", 0.0)
        body_objs.append(st.box("base", (W, L, base_h), (0, 0, base_h / 2), bevel=0.006, segments=6, material=body_mat))
        tl = L - ledge - 0.004
        ty = -L / 2 + tl / 2 + 0.002
        th = math.radians(-rec.get("inclinacao", 5.0))
        tt = 0.013
        zc = base_h + 0.009 + tl / 2 * math.sin(-th)
        treadle = st.box("basculante", (W * 0.96, tl, tt), (0, ty, zc), bevel=0.005, segments=6, material=mats["paint"])
        treadle.rotation_euler = (th, 0, 0)
        rub = rec.get("borracha_rel", 0.62)
        ry = ty + tl / 2 - tl * rub / 2 - tl * 0.06
        rubber = st.box("borracha", (W * 0.80, tl * rub, 0.0025), (0, ry, 0), bevel=0.001, material=mats["rubber_ridges"])
        # Borracha deitada sobre o basculante inclinado.
        dy = ry - ty
        rubber.location = (0, ty + dy * math.cos(th) - (tt / 2 + 0.0012) * math.sin(th), zc + dy * math.sin(th) + (tt / 2 + 0.0012) * math.cos(th))
        rubber.rotation_euler = (th, 0, 0)
        # Engrenagem do eixo, de lado (a cremalheira aparece na lateral).
        for side in (-1, 1):
            st.cylinder(f"eixo_{side}", 0.006, 0.004, (side * (W / 2 + 0.001), L / 2 - ledge - 0.03, base_h + 0.006), material=mats["chrome_dark"],
                        axis="X", bevel=0.001)
        top = base_h
        wah = (ty, th, zc, tt / 2, tl, ledge)
    elif shape == "redondo":
        R = W / 2
        body_objs.append(st.cylinder("caixa", R, H, (0, 0, H / 2), vertices=160, material=body_mat, axis="Z",
                                     bevel=rec.get("chanfro_m", 0.008)))
        if rec.get("pintura_lados"):
            st.cylinder("tampo", R - rec.get("chanfro_m", 0.008) * 0.8, 0.0012, (0, 0, H - 0.0006 + 0.00002), vertices=160,
                        material=mats["paint"], axis="Z")
    else:
        if rec.get("pintura_lados"):
            ch = rec.get("chanfro_m", 0.0045)
            body_objs.append(st.box("caixa", (W, L, H - 0.0004), (0, 0, (H - 0.0004) / 2), bevel=ch, segments=6, material=body_mat))
            inset = rec.get("tampo_margem_m", ch * 0.9)
            body_objs.append(st.box("tampo", (W - 2 * inset, L - 2 * inset, 0.0016), (0, 0, H - 0.0008), bevel=0.0006,
                                    segments=3, material=mats["paint"]))
        else:
            body_objs.append(st.box("caixa", (W, L, H), (0, 0, H / 2), bevel=rec.get("chanfro_m", 0.0045), segments=6,
                                    material=mats["paint"]))
        if rec.get("aro_cromo"):
            # Aro cromado em volta do tampo (filete de metal na borda).
            t = 0.0016
            for nm, size, loc in (("aro_esq", (t, L - 0.002, t), (-W / 2 + t / 2 + 0.0004, 0, H + t / 2 - 0.0003)),
                                  ("aro_dir", (t, L - 0.002, t), (W / 2 - t / 2 - 0.0004, 0, H + t / 2 - 0.0003)),
                                  ("aro_tras", (W - 0.002, t, t), (0, L / 2 - t / 2 - 0.0004, H + t / 2 - 0.0003)),
                                  ("aro_frente", (W - 0.002, t, t), (0, -L / 2 + t / 2 + 0.0004, H + t / 2 - 0.0003))):
                st.box(nm, size, loc, bevel=t * 0.45, segments=3, material=mats["chrome"])

    if shape == "redondo":
        st.cylinder("fundo_borracha", W / 2 - 0.002, 0.003, (0, 0, 0.0015), vertices=96, material=mats["rubber"], axis="Z", bevel=0.001)
    else:
        st.box("fundo_borracha", (W - 0.004, L - 0.004, 0.003), (0, 0, 0.0015), bevel=0.001, material=mats["rubber"])

    _panels(rec, W, L, top if shape not in ("compacto", "wah") else top, mats)

    if "estampa" in rec and shape not in ("compacto", "wah"):
        # Arte impressa no tampo inteiro (o desenho marca os cantos do tampo).
        _logo(rec["estampa"], L, (0, 0, H + 0.00005))
    elif "estampa" in rec and shape == "compacto":
        # No compacto a arte vem separada na linha do basculante: a parte de trás
        # vai no tampo da base, a da frente segue o basculante (inclinado).
        objs, _ = _logo(rec["estampa"], L, (0, 0, 0))
        _bend(objs, -L / 2 + tl, ty, math.radians(rec.get("basculante_ang", -2.0)), base_h + (H - base_h) / 2 - 0.003, (H - base_h + 0.006) / 2, base_h)
    elif "estampa" in rec:
        objs, _ = _logo(rec["estampa"], L, (0, 0, 0))
        wy, wth, wzc, whh, wtl, wledge = wah
        _bend(objs, L / 2 - wledge, wy, wth, wzc, whh, top)

    # Botões: posição relativa (x de -1 a 1, y de -1 a 1 sobre a face).
    controls = []
    style = rec["botao_estilo"]
    for c in rec["botoes"]:
        if c.get("tipo") == "deslizante":
            mats.setdefault("slider", st.mat_plastic("cursor", style.get("cursor", style["corpo"]), rough=0.3, coat=0.3))
            ctl = _pedal_slider(len(controls) + 100, c, c["pos"][0] * W / 2, c["pos"][1] * L / 2, c.get("curso_m", 0.020), top,
                                mats, c.get("rotulo_cor", rec["texto_cor"]))
        else:
            ctl = _place_knob(c, rec, W, L, top, mats)
        if not c.get("fixo"):
            controls.append(ctl)

    # Equalizador gráfico: deslizantes em fila no tampo (o de nível um pouco afastado).
    sliders = rec.get("deslizantes", [])
    if sliders:
        mats["slider"] = st.mat_plastic("cursor", style.get("cursor", style["corpo"]), rough=0.3, coat=0.3)
        lay = rec.get("deslizantes_layout", {})
        travel = lay.get("curso", 0.020)
        y_mid = lay.get("y", 0.62) * L / 2
        margin = lay.get("margem", 0.0075)
        gap = 0.003 if sliders[-1]["id"] == "nivel" else 0.0
        step = (W - 2 * margin - gap) / (len(sliders) - 1)
        for j, ctl in enumerate(sliders):
            x = -W / 2 + margin + j * step + (gap if j == len(sliders) - 1 else 0.0)
            controls.append(_pedal_slider(j, ctl, x, y_mid, travel, top, mats, lay.get("rotulo_cor", rec["texto_cor"])))

    display = None
    if "visor" in rec:
        # Visor digital: moldura preta e vidro escuro (a tela acende o cristal
        # por cima e escreve o valor).
        v = rec["visor"]
        vx, vy = v["pos"][0] * W / 2, v["pos"][1] * L / 2
        vw, vl = v["largura_rel"] * W, v["altura_rel"] * L
        st.box("visor_moldura", (vw + 0.005, vl + 0.005, 0.0012), (vx, vy, top + 0.0005), bevel=0.0006, material=mats["plastic"])
        glass = st.mat_plastic("visor_vidro", (0.004, 0.012, 0.035), rough=0.04, coat=1.0)
        st.box("visor_vidro", (vw, vl, 0.0006), (vx, vy, top + 0.0011), bevel=0.0002, material=glass)
        display = (vx, vy, top + 0.0014, vw, vl)

    if "led" in rec:
        lx, lyr = rec["led"]
        # No pedal compacto o LED pode ficar em cima do basculante.
        _led("led", (lx * W / 2, lyr * L / 2, H if rec.get("led_no_basculante") else top), mats)

    _pieces(rec, W, L, top, mats)
    _texts(rec, W, L, top, fonts, mats)

    foot = None
    if shape == "classico":
        pst = rec.get("pisante", {}).get("estilo", "almofada")
        pang = rec.get("pisante_ang", -1.5)
        pl = L * 0.40
        py = -L / 2 + pl / 2 + 0.004
        if pst == "placa_moldura":
            # TS9: moldura preta e placa de metal com frisos dentro.
            st.box("pisante_moldura", (W * 0.86, pl, 0.0075), (0, py, H + 0.0035), bevel=0.0022, segments=4, material=mats["plastic"])
            _ridged_plate("pisante", (W * 0.66, pl * 0.72, 0.0035), (0, py - pl * 0.02, H + 0.0085), mats, tilt=pang)
            foot = ((0.0, py, H + 0.0105), W * 0.33)
        elif pst == "placa_pequena":
            # TS808: placa de metal pequena direto na caixa.
            pw, pll = W * 0.62, pl * 0.62
            _ridged_plate("pisante", (pw, pll, 0.0045), (0, py, H + 0.0035), mats, tilt=pang)
            foot = ((0.0, py, H + 0.006), pw / 2)
        else:
            # Tampa grande de pisar, preta, na metade da frente, com borracha.
            pad = st.box("pisante", (W * 0.86, pl, 0.010), (0, py, H + 0.006), bevel=0.0028, segments=4, material=mats["plastic"])
            pad.rotation_euler = (math.radians(pang), 0, 0)
            st.box("pisante_borracha", (W * 0.60, pl * 0.66, 0.0018), (0, py - pl * 0.02, H + 0.0115), bevel=0.0008,
                   material=mats["rubber"])
        # Etiqueta com moldura entre os botões e o pisante.
        fy = rec.get("etiqueta_y", 0.0) * L / 2
        zt = H + 0.0003
        if rec.get("etiqueta_estilo") == "placa_prata":
            st.box("etiqueta_placa", (W * 0.70, L * 0.13, 0.0008), (0, fy, H + 0.0002), bevel=0.0003,
                   material=st.mat_brushed("etiqueta_metal", (0.78, 0.78, 0.80), rough=0.2))
            zt = H + 0.00065
        elif rec.get("etiqueta_estilo") != "nenhuma":
            _frame_on_top("moldura", W * 0.70, L * 0.13, 0.002, 0.0011, (0, fy, H + 0.0002), mats["ink2"])
        if not rec.get("modelo_oculto"):
            name_x, room = 0.0, None
            if "logo" in rec:
                # Logo dentro da moldura, à esquerda; nome e tipo centrados no resto.
                lg = rec["logo"]
                objs, lw = _logo(lg, lg.get("tam_m", L * 0.13 * 0.78), (0, fy, zt))
                left = -W * 0.35 + 0.003
                for o in objs:
                    o.location.x = left + lw / 2
                name_x = (left + lw + 0.002 + W * 0.35 - 0.003) / 2
                room = W * 0.35 - 0.003 - (left + lw + 0.002)
            name = _flat_text("modelo", rec["modelo"], fonts["label"], 0.0082, (name_x, fy + 0.0018, zt), mats["ink2"],
                              spacing=1.12, offset=0.00018)
            kind = _flat_text("tipo", rec["tipo_texto"], fonts["label"], 0.0042, (name_x, fy - 0.0052, zt), mats["ink2"],
                              spacing=1.5)
            if room:
                _fit(name, room)
                _fit(kind, room)
    elif shape in ("padrao", "redondo"):
        ps = rec.get("pisante", {})
        sx, sy = (ps["pos"][0] * W / 2, ps["pos"][1] * L / 2) if "pos" in ps else (0.0, rec.get("pisante_y", -0.55) * L / 2)
        foot = _stomp("pisante", ps.get("estilo", "metal"), (sx, sy, H), mats)
        if ps.get("raio_m"):
            foot = (foot[0], ps["raio_m"])
        if "logo" in rec:
            lg = rec["logo"]
            px, py = lg.get("pos", [0.0, 0.27])
            _logo(lg, lg.get("tam_m", 0.014), (px * W / 2, py * L / 2, H + 0.0001))
        if not rec.get("modelo_oculto"):
            # Nome: no meio, ou de lado nos pedais largos ("logo_x", de -1 a 1).
            lx = rec.get("logo_x", 0.0) * W / 2
            if rec.get("logo_script"):
                name = _flat_text("logo", rec["modelo"], fonts["script"], rec.get("logo_m", 0.024),
                                  (lx, rec.get("logo_y", 0.0) * L / 2, H + 0.0001), mats["ink2"])
            else:
                name = _flat_text("modelo", rec["modelo"], fonts["label"], rec.get("logo_m", 0.01),
                                  (lx, rec.get("logo_y", 0.0) * L / 2, H + 0.0001), mats["ink2"])
            _fit(name, W - 2 * rec.get("chanfro_m", 0.0045) - 0.006 - 2 * abs(lx))
            if rec.get("tipo_texto"):
                _flat_text("tipo", rec["tipo_texto"], fonts["label"], 0.0045, (lx, rec.get("logo_y", 0.0) * L / 2 - 0.011, H + 0.0001),
                           mats["ink2"], spacing=1.6)
    elif shape == "wah" and not rec.get("modelo_oculto"):
        # Nome na ponta do basculante, à frente da borracha (segue a inclinação).
        wy, wth, wzc, whh, wtl, wledge = wah
        ny = -L / 2 + wtl * 0.10 + 0.002
        dy = ny - wy
        name = _flat_text("modelo", rec["modelo"], fonts["label"], rec.get("logo_m", 0.011),
                          (0, wy + dy * math.cos(wth) - (whh + 0.0002) * math.sin(wth), wzc + dy * math.sin(wth) + (whh + 0.0002) * math.cos(wth)),
                          mats["ink2"], spacing=1.1)
        name.rotation_euler = (wth, 0, 0)
        _fit(name, W * 0.8)

    # Entradas: nas laterais (na caixa redonda, onde a borda passa) ou atrás.
    jy = L * 0.22
    jz = top * 0.55 if shape in ("compacto", "wah") else H * 0.55
    if "cunha" in rec:
        hf = rec["cunha"]["frente_m"]
        jz = (hf + (jy / L + 0.5) * (H - hf)) * 0.5
    if rec.get("jacks_topo"):
        for side in (-1, 1):
            st.cylinder("entrada_porca", 0.0065, 0.0024, (side * W * 0.22, L / 2 + 0.0012, jz), vertices=6, material=mats["chrome"],
                        axis="Y", bevel=0.0004)
            st.cylinder("entrada_furo", 0.0032, 0.0004, (side * W * 0.22, L / 2 + 0.0025, jz), material=mats["hole"], axis="Y")
    elif shape == "redondo":
        R = W / 2
        jx = math.sqrt(max(R * R - jy * jy, 0.0)) - 0.002
        for side in (-1, 1):
            st.cylinder("entrada_porca", 0.0065, 0.0024, (side * jx, jy, H * 0.55), vertices=6, material=mats["chrome"], axis="X", bevel=0.0004)
            st.cylinder("entrada_furo", 0.0032, 0.0004, (side * (jx + 0.0013), jy, H * 0.55), material=mats["hole"], axis="X")
    else:
        _jacks(W, jy, jz, mats)

    # Onde pisar: centro da tampa, do basculante ou do pisante de metal.
    if shape == "classico":
        foot = foot or ((0.0, -L / 2 + L * 0.20 + 0.004, H + 0.011), W * 0.40)
    elif shape == "compacto":
        tl = L * rec.get("basculante_rel", 0.64)
        foot = ((0.0, -L / 2 + tl / 2, H), W * 0.42)
    elif shape == "wah":
        wy, wth, wzc, whh, wtl, wledge = wah
        dy = -wtl * 0.30
        foot = ((0.0, wy + dy, wzc + dy * math.sin(wth) + whh), W * 0.36)
    led = None
    if "led" in rec:
        lx, lyr = rec["led"]
        led = ((lx * W / 2, lyr * L / 2, (H if rec.get("led_no_basculante") else top) + 0.0018), 0.0031)

    if "cunha" in rec:
        new = [o for o in bpy.data.objects if o not in first_objs]
        skip = set(body_objs) | {o for o in new if o.name.startswith(("entrada", "fundo_borracha"))}
        foot, led = _wedge([o for o in new if o not in skip], body_objs, H, rec["cunha"]["frente_m"], L, label_from, controls,
                           [foot, led])
    return {"controls": controls, "size": (W, L, H), "front": -L / 2, "target": (0.0, 0.0, H * 0.6), "footswitch": foot,
            "led": led, "display": display}
