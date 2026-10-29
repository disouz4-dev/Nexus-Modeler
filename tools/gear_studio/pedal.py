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


def mat_pedal_paint(name, color, metallic, rough=0.28, print_rings=None):
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
    if metallic > 0.05:
        # Pó metálico: pontinhos que brilham em ângulos diferentes.
        flake = nodes.new("ShaderNodeTexVoronoi")
        flake.inputs["Scale"].default_value = 4000.0
        links.new(tc.outputs["Object"], flake.inputs["Vector"])
        fb = nodes.new("ShaderNodeBump")
        fb.inputs["Strength"].default_value = 0.25
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


def build(rec, fonts):
    W = rec["medidas_m"]["largura"]
    L = rec["medidas_m"]["comprimento"]
    H = rec["medidas_m"]["altura"]
    shape = rec["formato"]
    paint = rec["pintura"]
    mats = {
        "paint": mat_pedal_paint("pintura", paint["cor"], paint.get("metalico", 0.0), paint.get("aspereza", 0.28),
                                 rec.get("estampa_ondas")),
        "ink": st.mat_paint("tinta", rec["texto_cor"], rough=0.45),
        "ink2": st.mat_paint("tinta2", rec.get("texto_cor2", rec["texto_cor"]), rough=0.45),
        "chrome": st.mat_metal("cromo", (0.92, 0.92, 0.93), rough=0.08),
        "chrome_dark": st.mat_metal("cromo_escuro", (0.55, 0.55, 0.56), rough=0.2),
        "hole": st.mat_paint("furo", (0.0, 0.0, 0.0), rough=0.9),
        # LED apagado (vidro vermelho escuro). Aceso, a tela desenha o brilho
        # por cima, e assim liga e desliga sem precisar de dois renders.
        "led": st.mat_plastic("led_vidro", (0.30, 0.012, 0.008), rough=0.06, coat=1.0),
        "rubber": mat_rubber("borracha"),
        "rubber_ridges": mat_rubber("borracha_frisos", ridges=True),
        "plastic": st.mat_plastic("plastico", (0.015, 0.015, 0.015), rough=0.42),
        "knob": st.mat_plastic("botao", rec["botao_estilo"]["corpo"], rough=0.3, coat=0.4),
        "cap": st.mat_brushed("tampa", rec["botao_estilo"].get("tampa", (0.8, 0.8, 0.8)), rough=0.18, radial_axis="Y"),
        "pointer": st.mat_paint("traco", rec["botao_estilo"].get("traco", (0.95, 0.95, 0.92)), rough=0.4),
    }

    top = H  # altura da face onde ficam os botões
    if shape == "compacto":
        base_h = H * rec.get("base_rel", 0.55)
        st.box("base", (W, L, base_h), (0, 0, base_h / 2), bevel=0.004, segments=6, material=mats["paint"])
        # Pedal basculante: cobre a frente, mais alto, com borracha frisada.
        tl = L * rec.get("basculante_rel", 0.64)
        ty = -L / 2 + tl / 2
        treadle = st.box("basculante", (W - 0.001, tl, H - base_h + 0.006), (0, ty, base_h + (H - base_h) / 2 - 0.003),
                         bevel=0.006, segments=6, material=mats["paint"])
        treadle.rotation_euler = (math.radians(-2.0), 0, 0)
        # Borracha: a frente fica sempre no mesmo lugar; menor, sobra espaço para a arte.
        rub = rec.get("borracha_rel", 0.62)
        st.box("borracha", (W * 0.80, tl * rub, 0.0025), (0, ty - tl * 0.35 + tl * rub / 2, H + 0.0005), bevel=0.001,
               material=mats["rubber_ridges"])
        # Parafuso de trocar a bateria, na ponta.
        st.cylinder("parafuso_bateria", 0.0055, 0.006, (0, -L / 2 - 0.001, base_h * 0.55), vertices=48,
                    material=mats["chrome"], axis="Y", bevel=0.0012)
        top = base_h
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
            left = (rec["led"][0] * W / 2 + 0.006) if "led" in rec else -W / 2 + 0.004
            name_x = (left + right - lw - 0.002) / 2
        name = _flat_text("modelo", rec["modelo"], fonts["label"], size,
                          (name_x, ty + tl * 0.38, H + 0.0004), mats["ink"], spacing=1.1,
                          offset=0.0002 if size >= 0.0085 else 0.0)
        if "logo" in rec:
            _fit(name, right - lw - 0.002 - left)
        else:
            left = (rec["led"][0] * W / 2 + 0.0065) if rec.get("led_no_basculante") else -W / 2 + 0.005
            right = W / 2 - 0.005
            _fit(name, 2 * min(name_x - left, right - name_x))
        _flat_text("tipo", rec["tipo_texto"], fonts["label"], 0.0062, (0, ty - tl * 0.40, H + 0.0004), mats["ink"], spacing=1.4)
    else:
        st.box("caixa", (W, L, H), (0, 0, H / 2), bevel=rec.get("chanfro_m", 0.0045), segments=6, material=mats["paint"])
        if rec.get("aro_cromo"):
            # Aro cromado em volta do tampo (filete de metal na borda).
            t = 0.0016
            for nm, size, loc in (("aro_esq", (t, L - 0.002, t), (-W / 2 + t / 2 + 0.0004, 0, H + t / 2 - 0.0003)),
                                  ("aro_dir", (t, L - 0.002, t), (W / 2 - t / 2 - 0.0004, 0, H + t / 2 - 0.0003)),
                                  ("aro_tras", (W - 0.002, t, t), (0, L / 2 - t / 2 - 0.0004, H + t / 2 - 0.0003)),
                                  ("aro_frente", (W - 0.002, t, t), (0, -L / 2 + t / 2 + 0.0004, H + t / 2 - 0.0003))):
                st.box(nm, size, loc, bevel=t * 0.45, segments=3, material=mats["chrome"])

    st.box("fundo_borracha", (W - 0.004, L - 0.004, 0.003), (0, 0, 0.0015), bevel=0.001, material=mats["rubber"])

    if "estampa" in rec and shape != "compacto":
        # Arte impressa no tampo inteiro (o desenho marca os cantos do tampo).
        _logo(rec["estampa"], L, (0, 0, H + 0.00005))
    elif "estampa" in rec:
        # No compacto a arte vem separada na linha do basculante: a parte de trás
        # vai no tampo da base, a da frente segue o basculante (inclinado).
        objs, _ = _logo(rec["estampa"], L, (0, 0, 0))
        split = -L / 2 + tl
        th = math.radians(-2.0)
        zc = base_h + (H - base_h) / 2 - 0.003
        hh = (H - base_h + 0.006) / 2
        for o in objs:
            vs = o.data.vertices
            if not len(vs):
                continue
            if sum(v.co.y for v in vs) / len(vs) + o.location.y > split:
                o.location.z = base_h + 0.00005
                continue
            oy = o.location.y
            for v in vs:
                dy = v.co.y + oy - ty
                dz = hh + 0.00005 + v.co.z
                v.co = (v.co.x, ty + dy * math.cos(th) - dz * math.sin(th) - oy, zc + dy * math.sin(th) + dz * math.cos(th))

    # Botões: posição relativa (x de -1 a 1, y de -1 a 1 sobre a face).
    controls = []
    style = rec["botao_estilo"]
    for c in rec["botoes"]:
        r = style["diametro_m"] / 2 * c.get("escala", 1.0)
        x = c["pos"][0] * W / 2
        y = c["pos"][1] * L / 2
        pivot = knobs.knob(c["id"], r, style["altura_m"] * c.get("escala", 1.0) ** 0.5,
                           {"body": mats["knob"], "cap": mats["cap"], "pointer": mats["pointer"]},
                           skirt=style.get("saia", False), cap=style.get("com_tampa", False))
        knobs.lay_on_top(pivot, (x, y, top))
        h = style["altura_m"] * c.get("escala", 1.0) ** 0.5 + (0.0025 if style.get("saia", False) else 0.0)
        controls.append({"id": c["id"], "pivot": pivot, "center": (x, y, top + h / 2), "outer_r": r * (1.28 if style.get("saia") else 1.0),
                         "depth": h, "axis": (0.0, 0.0, 1.0)})
        ly = y + r * (1.28 if style.get("saia") else 1.0) + 0.0055
        if rec.get("rotulo_embaixo"):
            ly = y - r * (1.28 if style.get("saia") else 1.0) - 0.0055
        st.label(c["rotulo"], (x, ly, top + 0.0001), rec.get("rotulo_m", 0.0052), up="Y", color=rec["texto_cor"], spacing=1.15)

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
            controls.append(_pedal_slider(j, ctl, x, y_mid, travel, top, mats, rec["texto_cor"]))

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

    if shape == "classico":
        # Tampa grande de pisar, preta, na metade da frente, com borracha.
        pl = L * 0.40
        py = -L / 2 + pl / 2 + 0.004
        pad = st.box("pisante", (W * 0.86, pl, 0.010), (0, py, H + 0.006), bevel=0.0028, segments=4, material=mats["plastic"])
        pad.rotation_euler = (math.radians(-1.5), 0, 0)
        st.box("pisante_borracha", (W * 0.60, pl * 0.66, 0.0018), (0, py - pl * 0.02, H + 0.0115), bevel=0.0008,
               material=mats["rubber"])
        # Etiqueta com moldura entre os botões e o pisante.
        fy = rec.get("etiqueta_y", 0.0) * L / 2
        _frame_on_top("moldura", W * 0.70, L * 0.13, 0.002, 0.0011, (0, fy, H + 0.0002), mats["ink2"])
        name_x, room = 0.0, None
        if "logo" in rec:
            # Logo dentro da moldura, à esquerda; nome e tipo centrados no resto.
            lg = rec["logo"]
            objs, lw = _logo(lg, lg.get("tam_m", L * 0.13 * 0.78), (0, fy, H + 0.0003))
            left = -W * 0.35 + 0.003
            for o in objs:
                o.location.x = left + lw / 2
            name_x = (left + lw + 0.002 + W * 0.35 - 0.003) / 2
            room = W * 0.35 - 0.003 - (left + lw + 0.002)
        name = _flat_text("modelo", rec["modelo"], fonts["label"], 0.0082, (name_x, fy + 0.0018, H + 0.0003), mats["ink2"],
                          spacing=1.12, offset=0.00018)
        kind = _flat_text("tipo", rec["tipo_texto"], fonts["label"], 0.0042, (name_x, fy - 0.0052, H + 0.0003), mats["ink2"],
                          spacing=1.5)
        if room:
            _fit(name, room)
            _fit(kind, room)
    elif shape == "padrao":
        sy = rec.get("pisante_y", -0.55) * L / 2
        _metal_stomp("pisante", (0, sy, H), mats)
        if "logo" in rec:
            lg = rec["logo"]
            px, py = lg.get("pos", [0.0, 0.27])
            _logo(lg, lg.get("tam_m", 0.014), (px * W / 2, py * L / 2, H + 0.0001))
        if rec.get("logo_script"):
            name = _flat_text("logo", rec["modelo"], fonts["script"], rec.get("logo_m", 0.024),
                              (0, rec.get("logo_y", 0.0) * L / 2, H + 0.0001), mats["ink2"])
        else:
            name = _flat_text("modelo", rec["modelo"], fonts["label"], 0.01, (0, rec.get("logo_y", 0.0) * L / 2, H + 0.0001), mats["ink2"])
        _fit(name, W - 2 * rec.get("chanfro_m", 0.0045) - 0.006)
        if rec.get("tipo_texto"):
            _flat_text("tipo", rec["tipo_texto"], fonts["label"], 0.0045, (0, rec.get("logo_y", 0.0) * L / 2 - 0.011, H + 0.0001),
                       mats["ink2"], spacing=1.6)

    _jacks(W, L * 0.22, top * 0.55 if shape == "compacto" else H * 0.55, mats)

    # Onde pisar: centro da tampa, do basculante ou do pisante de metal.
    if shape == "classico":
        foot = ((0.0, -L / 2 + L * 0.20 + 0.004, H + 0.011), W * 0.40)
    elif shape == "compacto":
        tl = L * rec.get("basculante_rel", 0.64)
        foot = ((0.0, -L / 2 + tl / 2, H), W * 0.42)
    else:
        foot = ((0.0, rec.get("pisante_y", -0.55) * L / 2, H + 0.012), 0.012)
    led = None
    if "led" in rec:
        lx, lyr = rec["led"]
        led = ((lx * W / 2, lyr * L / 2, (H if rec.get("led_no_basculante") else top) + 0.0018), 0.0031)
    return {"controls": controls, "size": (W, L, H), "front": -L / 2, "target": (0.0, 0.0, H * 0.6), "footswitch": foot,
            "led": led, "display": display}
