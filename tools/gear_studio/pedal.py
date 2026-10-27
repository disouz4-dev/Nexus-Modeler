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
import math

import bpy

import knobs
import studio as st


def mat_pedal_paint(name, color, metallic, rough=0.28):
    """Pintura de pedal: base colorida, pó metálico opcional e verniz
    brilhante por cima, com a leve "casca de laranja" da pintura a pó."""
    mat = bpy.data.materials.new(name)
    mat.use_nodes = True
    nodes, links = mat.node_tree.nodes, mat.node_tree.links
    bsdf = nodes["Principled BSDF"]
    bsdf.inputs["Base Color"].default_value = (*color, 1)
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


def _frame_on_top(name, width, length, radius, thickness, location, mat):
    """Moldura impressa (retângulo de cantos arredondados) deitada no topo."""
    obj = st.rounded_rect_curve(name, width, length, radius, thickness, location, mat)
    obj.rotation_euler = (math.radians(-90), 0, 0)
    obj.scale = (1, 0.08, 1)  # achata o cordão (eixo normal ao desenho): vira linha impressa
    return obj


def _led(name, location, mats):
    x, y, z = location
    st.cylinder(name + "_aro", 0.0042, 0.0016, (x, y, z + 0.0008), material=mats["chrome"], axis="Z", bevel=0.0004)
    bpy.ops.mesh.primitive_uv_sphere_add(radius=0.0028, location=(x, y, z + 0.0016), segments=32, ring_count=16)
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
        "paint": mat_pedal_paint("pintura", paint["cor"], paint.get("metalico", 0.0), paint.get("aspereza", 0.28)),
        "ink": st.mat_paint("tinta", rec["texto_cor"], rough=0.45),
        "ink2": st.mat_paint("tinta2", rec.get("texto_cor2", rec["texto_cor"]), rough=0.45),
        "chrome": st.mat_metal("cromo", (0.92, 0.92, 0.93), rough=0.08),
        "chrome_dark": st.mat_metal("cromo_escuro", (0.55, 0.55, 0.56), rough=0.2),
        "hole": st.mat_paint("furo", (0.0, 0.0, 0.0), rough=0.9),
        "led": st.mat_glow("led", (1.0, 0.05, 0.02), 6.0, base_rough=0.05),
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
        st.box("borracha", (W * 0.80, tl * 0.62, 0.0025), (0, ty - tl * 0.04, H + 0.0005), bevel=0.001,
               material=mats["rubber_ridges"])
        # Parafuso de trocar a bateria, na ponta.
        st.cylinder("parafuso_bateria", 0.0055, 0.006, (0, -L / 2 - 0.001, base_h * 0.55), vertices=48,
                    material=mats["chrome"], axis="Y", bevel=0.0012)
        top = base_h
        _flat_text("modelo", rec["modelo"], fonts["label"], 0.0085, (0, ty + tl * 0.38, H + 0.0004), mats["ink"], spacing=1.1, offset=0.0002)
        _flat_text("tipo", rec["tipo_texto"], fonts["label"], 0.0062, (0, ty - tl * 0.40, H + 0.0004), mats["ink"], spacing=1.4)
    else:
        st.box("caixa", (W, L, H), (0, 0, H / 2), bevel=rec.get("chanfro_m", 0.0045), segments=6, material=mats["paint"])

    st.box("fundo_borracha", (W - 0.004, L - 0.004, 0.003), (0, 0, 0.0015), bevel=0.001, material=mats["rubber"])

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
        _flat_text("modelo", rec["modelo"], fonts["label"], 0.0082, (0, fy + 0.0018, H + 0.0003), mats["ink2"], spacing=1.12, offset=0.00018)
        _flat_text("tipo", rec["tipo_texto"], fonts["label"], 0.0042, (0, fy - 0.0052, H + 0.0003), mats["ink2"], spacing=1.5)
    elif shape == "padrao":
        sy = rec.get("pisante_y", -0.55) * L / 2
        _metal_stomp("pisante", (0, sy, H), mats)
        if rec.get("logo_script"):
            _flat_text("logo", rec["modelo"], fonts["script"], rec.get("logo_m", 0.024), (0, rec.get("logo_y", 0.0) * L / 2, H + 0.0001),
                       mats["ink2"])
        else:
            _flat_text("modelo", rec["modelo"], fonts["label"], 0.01, (0, rec.get("logo_y", 0.0) * L / 2, H + 0.0001), mats["ink2"])
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
        led = ((lx * W / 2, lyr * L / 2, (H if rec.get("led_no_basculante") else top) + 0.0018), 0.0030)
    return {"controls": controls, "size": (W, L, H), "front": -L / 2, "target": (0.0, 0.0, H * 0.6), "footswitch": foot,
            "led": led}
