# Nexus Gear Studio: estúdio de render (luz, câmera, materiais) comum a
# todos os equipamentos. Roda dentro do Blender 4.5.
import math
import os

import bpy
from mathutils import Vector


def reset_scene():
    bpy.ops.wm.read_factory_settings(use_empty=True)
    scene = bpy.context.scene
    scene.unit_settings.system = "METRIC"
    return scene


# ---------------------------------------------------------------- materiais

def _principled(name):
    mat = bpy.data.materials.new(name)
    mat.use_nodes = True
    nodes = mat.node_tree.nodes
    bsdf = nodes["Principled BSDF"]
    return mat, nodes, mat.node_tree.links, bsdf


def _object_coords(nodes):
    tc = nodes.new("ShaderNodeTexCoord")
    return tc.outputs["Object"]


def mat_plastic(name, color, rough=0.35, coat=0.0):
    mat, _, _, bsdf = _principled(name)
    bsdf.inputs["Base Color"].default_value = (*color, 1)
    bsdf.inputs["Roughness"].default_value = rough
    bsdf.inputs["Coat Weight"].default_value = coat
    bsdf.inputs["Coat Roughness"].default_value = 0.08
    return mat


def mat_metal(name, color, rough=0.15):
    mat, _, _, bsdf = _principled(name)
    bsdf.inputs["Base Color"].default_value = (*color, 1)
    bsdf.inputs["Metallic"].default_value = 1.0
    bsdf.inputs["Roughness"].default_value = rough
    return mat


def mat_leather(name, color, grain_mm, rough=0.5, strength=0.55):
    """Couro de amplificador (grão "levant"): células irregulares com vales
    entre elas, brilho que varia um pouco de célula para célula."""
    mat, nodes, links, bsdf = _principled(name)
    coords = _object_coords(nodes)
    scale = 1000.0 / grain_mm

    vor = nodes.new("ShaderNodeTexVoronoi")
    vor.feature = "DISTANCE_TO_EDGE"
    vor.inputs["Scale"].default_value = scale
    vor.inputs["Randomness"].default_value = 1.0
    links.new(coords, vor.inputs["Vector"])

    # Distorce as células para não parecerem colmeia.
    noise = nodes.new("ShaderNodeTexNoise")
    noise.inputs["Scale"].default_value = scale * 0.35
    noise.inputs["Detail"].default_value = 4.0
    mix_vec = nodes.new("ShaderNodeVectorMath")
    mix_vec.operation = "MULTIPLY_ADD"
    mix_vec.inputs[1].default_value = (0.0006, 0.0006, 0.0006)
    links.new(coords, noise.inputs["Vector"])
    links.new(noise.outputs["Color"], mix_vec.inputs[0])
    links.new(coords, mix_vec.inputs[2])
    links.new(mix_vec.outputs["Vector"], vor.inputs["Vector"])

    ramp = nodes.new("ShaderNodeMapRange")
    ramp.inputs["From Min"].default_value = 0.0
    ramp.inputs["From Max"].default_value = 0.22
    links.new(vor.outputs["Distance"], ramp.inputs["Value"])

    bump = nodes.new("ShaderNodeBump")
    bump.inputs["Strength"].default_value = strength
    bump.inputs["Distance"].default_value = 0.0004
    links.new(ramp.outputs["Result"], bump.inputs["Height"])

    fine = nodes.new("ShaderNodeTexNoise")
    fine.inputs["Scale"].default_value = scale * 4
    fine.inputs["Detail"].default_value = 2.0
    links.new(coords, fine.inputs["Vector"])
    bump2 = nodes.new("ShaderNodeBump")
    bump2.inputs["Strength"].default_value = 0.12
    bump2.inputs["Distance"].default_value = 0.0001
    links.new(fine.outputs["Fac"], bump2.inputs["Height"])
    links.new(bump2.outputs["Normal"], bump.inputs["Normal"])
    links.new(bump.outputs["Normal"], bsdf.inputs["Normal"])

    # Os vales ficam mais foscos e escuros; o alto das células brilha.
    rough_map = nodes.new("ShaderNodeMapRange")
    rough_map.inputs["To Min"].default_value = rough + 0.25
    rough_map.inputs["To Max"].default_value = rough - 0.08
    links.new(ramp.outputs["Result"], rough_map.inputs["Value"])
    links.new(rough_map.outputs["Result"], bsdf.inputs["Roughness"])

    col = nodes.new("ShaderNodeMix")
    col.data_type = "RGBA"
    col.inputs["A"].default_value = (*[c * 0.55 for c in color], 1)
    col.inputs["B"].default_value = (*color, 1)
    links.new(ramp.outputs["Result"], col.inputs["Factor"])
    links.new(col.outputs["Result"], bsdf.inputs["Base Color"])
    bsdf.inputs["Specular IOR Level"].default_value = 0.6
    return mat


def mat_brushed(name, color, rough=0.28, radial_axis=None, streak_axis="X"):
    """Metal escovado. Com radial_axis, a escova é circular (tampa de botão);
    senão, em linhas retas ao longo de streak_axis (painel)."""
    mat, nodes, links, bsdf = _principled(name)
    coords = _object_coords(nodes)
    bsdf.inputs["Base Color"].default_value = (*color, 1)
    bsdf.inputs["Metallic"].default_value = 1.0
    bsdf.inputs["Anisotropic"].default_value = 0.75

    tangent = nodes.new("ShaderNodeTangent")
    tangent.direction_type = "RADIAL"
    tangent.axis = radial_axis or "Z"
    links.new(tangent.outputs["Tangent"], bsdf.inputs["Tangent"])

    noise = nodes.new("ShaderNodeTexNoise")
    noise.inputs["Detail"].default_value = 8.0
    noise.inputs["Roughness"].default_value = 0.7
    if radial_axis:
        # Riscos circulares: ruído em função do raio.
        sep = nodes.new("ShaderNodeVectorMath")
        sep.operation = "LENGTH"
        links.new(coords, sep.inputs[0])
        noise.noise_dimensions = "1D"
        noise.inputs["Scale"].default_value = 9000.0
        links.new(sep.outputs["Value"], noise.inputs["W"])
    else:
        mapping = nodes.new("ShaderNodeMapping")
        sx = {"X": (0.4, 600.0, 600.0), "Z": (600.0, 600.0, 0.4)}[streak_axis]
        mapping.inputs["Scale"].default_value = sx
        links.new(coords, mapping.inputs["Vector"])
        links.new(mapping.outputs["Vector"], noise.inputs["Vector"])
        noise.inputs["Scale"].default_value = 1.0

    # Riscos visíveis também na cor: faixas um pouco mais claras e escuras.
    cmap = nodes.new("ShaderNodeMapRange")
    cmap.inputs["To Min"].default_value = 0.72
    cmap.inputs["To Max"].default_value = 1.12
    links.new(noise.outputs["Fac"], cmap.inputs["Value"])
    tint = nodes.new("ShaderNodeVectorMath")
    tint.operation = "SCALE"
    tint.inputs[0].default_value = color
    links.new(cmap.outputs["Result"], tint.inputs["Scale"])
    links.new(tint.outputs["Vector"], bsdf.inputs["Base Color"])

    rmap = nodes.new("ShaderNodeMapRange")
    rmap.inputs["To Min"].default_value = rough - 0.08
    rmap.inputs["To Max"].default_value = rough + 0.10
    links.new(noise.outputs["Fac"], rmap.inputs["Value"])
    links.new(rmap.outputs["Result"], bsdf.inputs["Roughness"])

    bump = nodes.new("ShaderNodeBump")
    bump.inputs["Strength"].default_value = 0.05
    bump.inputs["Distance"].default_value = 0.00005
    links.new(noise.outputs["Fac"], bump.inputs["Height"])
    links.new(bump.outputs["Normal"], bsdf.inputs["Normal"])
    return mat


def mat_paint(name, color, rough=0.55):
    mat, _, _, bsdf = _principled(name)
    bsdf.inputs["Base Color"].default_value = (*color, 1)
    bsdf.inputs["Roughness"].default_value = rough
    return mat


def mat_glow(name, color, strength, base_rough=0.15):
    mat, _, _, bsdf = _principled(name)
    bsdf.inputs["Base Color"].default_value = (*color, 1)
    bsdf.inputs["Roughness"].default_value = base_rough
    bsdf.inputs["Emission Color"].default_value = (*color, 1)
    bsdf.inputs["Emission Strength"].default_value = strength
    bsdf.inputs["Coat Weight"].default_value = 1.0
    bsdf.inputs["Coat Roughness"].default_value = 0.05
    return mat


def mat_grille_cloth(name, color_a, color_b, weave_mm):
    """Tela de caixa em trama de cesto: quadradinhos alternando fios
    deitados e em pé, com fios de duas cores misturados ("sal e pimenta")."""
    mat, nodes, links, bsdf = _principled(name)
    coords = _object_coords(nodes)
    s = 1000.0 / weave_mm
    flat = nodes.new("ShaderNodeMapping")
    flat.inputs["Scale"].default_value = (s, 0.0, s)
    links.new(coords, flat.inputs["Vector"])

    checker = nodes.new("ShaderNodeTexChecker")
    checker.inputs["Scale"].default_value = 1.0
    checker.inputs["Color1"].default_value = (1, 1, 1, 1)
    checker.inputs["Color2"].default_value = (0, 0, 0, 1)
    links.new(flat.outputs["Vector"], checker.inputs["Vector"])

    waves = []
    for axis in ("X", "Z"):
        w = nodes.new("ShaderNodeTexWave")
        w.wave_type = "BANDS"
        w.bands_direction = axis
        w.wave_profile = "SIN"
        w.inputs["Scale"].default_value = 3.0 * 2.0 / math.pi * 1.6
        w.inputs["Distortion"].default_value = 0.6
        w.inputs["Detail"].default_value = 1.0
        links.new(flat.outputs["Vector"], w.inputs["Vector"])
        waves.append(w)
    mix = nodes.new("ShaderNodeMix")
    mix.data_type = "FLOAT"
    links.new(checker.outputs["Fac"], mix.inputs["Factor"])
    links.new(waves[0].outputs["Fac"], mix.inputs["A"])
    links.new(waves[1].outputs["Fac"], mix.inputs["B"])

    bump = nodes.new("ShaderNodeBump")
    bump.inputs["Strength"].default_value = 0.9
    bump.inputs["Distance"].default_value = 0.0006
    links.new(mix.outputs["Result"], bump.inputs["Height"])
    links.new(bump.outputs["Normal"], bsdf.inputs["Normal"])

    # Cor dos fios: ruído fino escolhe fio claro ou escuro; os vales escurecem.
    speck = nodes.new("ShaderNodeTexNoise")
    speck.inputs["Scale"].default_value = s * 1.3
    speck.inputs["Detail"].default_value = 1.0
    links.new(coords, speck.inputs["Vector"])
    sel = nodes.new("ShaderNodeMapRange")
    sel.inputs["From Min"].default_value = 0.56
    sel.inputs["From Max"].default_value = 0.64
    links.new(speck.outputs["Fac"], sel.inputs["Value"])
    col = nodes.new("ShaderNodeMix")
    col.data_type = "RGBA"
    col.inputs["A"].default_value = (*color_a, 1)
    col.inputs["B"].default_value = (*color_b, 1)
    links.new(sel.outputs["Result"], col.inputs["Factor"])
    shade = nodes.new("ShaderNodeMix")
    shade.data_type = "RGBA"
    shade.blend_type = "MULTIPLY"
    shade.inputs["Factor"].default_value = 0.45
    links.new(col.outputs["Result"], shade.inputs["A"])
    links.new(mix.outputs["Result"], shade.inputs["B"])

    # Tecido fica mais escuro perto das bordas do quadro (poeira e sombra da
    # aba), como nas caixas de verdade fotografadas em estúdio.
    sep = nodes.new("ShaderNodeSeparateXYZ")
    links.new(coords, sep.inputs["Vector"])
    radial = nodes.new("ShaderNodeMath")
    radial.operation = "ADD"
    sq_x = nodes.new("ShaderNodeMath")
    sq_x.operation = "POWER"
    sq_x.inputs[1].default_value = 2.0
    sq_z = nodes.new("ShaderNodeMath")
    sq_z.operation = "POWER"
    sq_z.inputs[1].default_value = 2.0
    links.new(sep.outputs["X"], sq_x.inputs[0])
    links.new(sep.outputs["Z"], sq_z.inputs[0])
    links.new(sq_x.outputs[0], radial.inputs[0])
    links.new(sq_z.outputs[0], radial.inputs[1])
    vign = nodes.new("ShaderNodeMapRange")
    vign.inputs["From Min"].default_value = 0.0
    vign.inputs["From Max"].default_value = 0.16
    vign.inputs["To Min"].default_value = 1.0
    vign.inputs["To Max"].default_value = 0.45
    links.new(radial.outputs[0], vign.inputs["Value"])
    dark = nodes.new("ShaderNodeVectorMath")
    dark.operation = "SCALE"
    links.new(shade.outputs["Result"], dark.inputs[0])
    links.new(vign.outputs["Result"], dark.inputs["Scale"])
    links.new(dark.outputs["Vector"], bsdf.inputs["Base Color"])
    bsdf.inputs["Roughness"].default_value = 0.85
    bsdf.inputs["Sheen Weight"].default_value = 0.3
    bsdf.inputs["Sheen Roughness"].default_value = 0.4
    return mat


# ---------------------------------------------------------------- geometria

def select_only(obj):
    for o in list(bpy.context.selected_objects):
        if o is not None:
            o.select_set(False)
    obj.select_set(True)
    bpy.context.view_layer.objects.active = obj


def apply_modifiers(obj):
    select_only(obj)
    for m in list(obj.modifiers):
        bpy.ops.object.modifier_apply(modifier=m.name)


def smooth(obj, angle=35):
    select_only(obj)
    bpy.ops.object.shade_smooth_by_angle(angle=math.radians(angle))


def box(name, size, location, bevel=0.0, segments=4, material=None):
    bpy.ops.mesh.primitive_cube_add(size=1, location=location)
    obj = bpy.context.active_object
    obj.name = name
    obj.scale = size
    bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)
    if bevel > 0:
        mod = obj.modifiers.new("chanfro", "BEVEL")
        mod.width = bevel
        mod.segments = segments
        mod.limit_method = "ANGLE"
        apply_modifiers(obj)
    if material:
        obj.data.materials.append(material)
    smooth(obj)
    return obj


def cut(obj, cutter):
    mod = obj.modifiers.new("corte", "BOOLEAN")
    mod.operation = "DIFFERENCE"
    mod.solver = "EXACT"
    mod.object = cutter
    apply_modifiers(obj)
    bpy.data.objects.remove(cutter, do_unlink=True)


def cylinder(name, radius, depth, location, vertices=96, material=None, axis="Y", bevel=0.0):
    rot = {"Y": (math.radians(90), 0, 0), "Z": (0, 0, 0), "X": (0, math.radians(90), 0)}[axis]
    bpy.ops.mesh.primitive_cylinder_add(vertices=vertices, radius=radius, depth=depth, location=location, rotation=rot)
    obj = bpy.context.active_object
    obj.name = name
    bpy.ops.object.transform_apply(location=False, rotation=True, scale=False)
    if bevel > 0:
        mod = obj.modifiers.new("chanfro", "BEVEL")
        mod.width = bevel
        mod.segments = 3
        mod.limit_method = "ANGLE"
        apply_modifiers(obj)
    if material:
        obj.data.materials.append(material)
    smooth(obj, 40)
    return obj


# Rótulos (GANHO, GRAVE, LIGA...): não entram no modelo 3D. A ferramenta só
# anota onde ficam; a tela do plugin escreve por cima, no idioma escolhido.
LABELS = []


def label(text, location, size, up="Z", color=(0.0, 0.0, 0.0), spacing=1.0):
    """Anota um rótulo: centro do texto, altura da letra em metros, para onde
    é "para cima" no texto (Z na frente do amplificador, Y no topo do pedal)
    e a cor da tinta (linear, como no Blender)."""
    LABELS.append({"texto": text, "local": tuple(location), "tamanho": size, "cima": up,
                   "cor": tuple(color), "espaco": spacing})


_fonts = {}


def font(path):
    if path not in _fonts:
        _fonts[path] = bpy.data.fonts.load(path)
    return _fonts[path]


def text(name, body, font_path, size, location, material, extrude=0.00005, align="CENTER",
         bevel=0.0, offset=0.0, rotation=(math.radians(90), 0, 0), spacing=1.0, valign="CENTER"):
    """Texto na frente do equipamento (plano XZ, virado para -Y)."""
    curve = bpy.data.curves.new(name, "FONT")
    curve.body = body
    curve.font = font(font_path)
    curve.size = size
    curve.align_x = align
    curve.align_y = valign
    curve.extrude = extrude
    curve.bevel_depth = bevel
    curve.bevel_resolution = 3
    curve.offset = offset
    curve.space_character = spacing
    obj = bpy.data.objects.new(name, curve)
    bpy.context.collection.objects.link(obj)
    obj.location = location
    obj.rotation_euler = rotation
    obj.data.materials.append(material)
    return obj


def rounded_rect_curve(name, width, height, radius, thickness, location, material, segments=10):
    """Friso: cordão arredondado contornando um retângulo no plano XZ."""
    pts = []
    corners = [(width / 2 - radius, height / 2 - radius, 0),
               (-width / 2 + radius, height / 2 - radius, 90),
               (-width / 2 + radius, -height / 2 + radius, 180),
               (width / 2 - radius, -height / 2 + radius, 270)]
    for cx, cz, start in corners:
        for i in range(segments + 1):
            a = math.radians(start + 90 * i / segments)
            pts.append((cx + radius * math.cos(a), 0.0, cz + radius * math.sin(a)))
    curve = bpy.data.curves.new(name, "CURVE")
    curve.dimensions = "3D"
    curve.bevel_depth = thickness
    curve.bevel_resolution = 6
    spline = curve.splines.new("POLY")
    spline.points.add(len(pts) - 1)
    for p, co in zip(spline.points, pts):
        p.co = (*co, 1)
    spline.use_cyclic_u = True
    obj = bpy.data.objects.new(name, curve)
    bpy.context.collection.objects.link(obj)
    obj.location = location
    obj.data.materials.append(material)
    return obj


def strap_arc(name, span, rise, z, radius, flatten, material, segments=32):
    """Alça de couro: arco no plano XZ, achatado na largura (eixo Y)."""
    curve = bpy.data.curves.new(name, "CURVE")
    curve.dimensions = "3D"
    curve.bevel_depth = radius
    curve.bevel_resolution = 6
    curve.use_fill_caps = True
    spline = curve.splines.new("POLY")
    spline.points.add(segments)
    for i, p in enumerate(spline.points):
        u = i / segments
        x = (u - 0.5) * span
        p.co = (x, 0.0, z + rise * math.sin(math.pi * u) ** 0.6, 1)
    obj = bpy.data.objects.new(name, curve)
    bpy.context.collection.objects.link(obj)
    obj.scale = (1.0, flatten, 1.0)
    obj.data.materials.append(material)
    return obj


# ---------------------------------------------------------------- luz e câmera

def area_light(name, location, target, size, strength, color=(1, 1, 1), size_y=None):
    data = bpy.data.lights.new(name, "AREA")
    data.energy = strength
    data.color = color
    if size_y:
        data.shape = "RECTANGLE"
        data.size = size
        data.size_y = size_y
    else:
        data.size = size
    obj = bpy.data.objects.new(name, data)
    bpy.context.collection.objects.link(obj)
    obj.location = location
    direction = Vector(target) - Vector(location)
    obj.rotation_euler = direction.to_track_quat("-Z", "Y").to_euler()
    return obj


# Multiplicador geral da luz do estúdio (ajustado olhando os renders).
LUZ = 0.16


def studio(scene, center, width, kind="cabecote"):
    """Luz de estúdio de produto: caixa de luz grande acima e à frente,
    preenchimento suave, dois recortes atrás para desenhar as bordas de
    cima, e o fundo quase preto."""
    world = bpy.data.worlds.new("estudio")
    world.use_nodes = True
    world.node_tree.nodes["Background"].inputs["Color"].default_value = (0.02, 0.02, 0.022, 1)
    world.node_tree.nodes["Background"].inputs["Strength"].default_value = 1.0
    scene.world = world

    cx, cy, cz = center
    w = width
    if kind == "pedal":
        # Pedal visto de cima: caixa de luz grande quase em cima, um pouco à
        # frente; preenchimentos fracos dos lados e recortes atrás. Sem a
        # faixa baixa, que lavaria a frente da caixa de branco.
        # Com a câmera bem em cima, a luz principal fica à frente e mais baixa:
        # assim o reflexo do verniz não volta direto para a câmera.
        area_light("principal", (cx, cy - 2.4 * w, cz + 1.7 * w), center, 2.2 * w, 360 * w * w * LUZ, (1.0, 0.97, 0.92), 1.0 * w)
        # Luz frontal: caixa de luz grande na direção de quem olha (acima, um
        # pouco à frente), que tira a sombra dos botões e clareia o pedal por
        # igual, como numa foto de produto.
        area_light("frontal", (cx, cy - 0.5 * w, cz + 3.2 * w), center, 4.0 * w, 300 * w * w * LUZ, (1.0, 0.98, 0.95), 4.0 * w)
        area_light("preenche_esq", (cx - 2.0 * w, cy - 0.4 * w, cz + 1.2 * w), center, 1.0 * w, 28 * w * w * LUZ, (0.92, 0.95, 1.0))
        area_light("preenche_dir", (cx + 2.0 * w, cy - 0.2 * w, cz + 1.0 * w), center, 1.0 * w, 16 * w * w * LUZ, (1.0, 0.95, 0.9))
        area_light("recorte_esq", (cx - 1.2 * w, cy + 1.6 * w, cz + 1.2 * w), center, 0.6 * w, 90 * w * w * LUZ, (0.9, 0.95, 1.0))
        area_light("recorte_dir", (cx + 1.2 * w, cy + 1.6 * w, cz + 1.2 * w), center, 0.6 * w, 90 * w * w * LUZ, (1.0, 0.95, 0.9))
        return
    area_light("principal", (cx, cy - 1.4 * w, cz + 1.3 * w), center, 1.6 * w, 260 * w * w * LUZ, (1.0, 0.97, 0.92), 0.7 * w)
    area_light("preenche_esq", (cx - 1.6 * w, cy - 1.2 * w, cz + 0.2 * w), center, 0.9 * w, 25 * w * w * LUZ, (0.92, 0.95, 1.0))
    area_light("preenche_dir", (cx + 1.6 * w, cy - 1.0 * w, cz + 0.1 * w), center, 0.9 * w, 14 * w * w * LUZ, (1.0, 0.95, 0.9))
    area_light("recorte_esq", (cx - 1.0 * w, cy + 0.9 * w, cz + 0.8 * w), (cx, cy, cz + 0.1), 0.4 * w, 90 * w * w * LUZ, (0.9, 0.95, 1.0))
    area_light("recorte_dir", (cx + 1.0 * w, cy + 0.9 * w, cz + 0.8 * w), (cx, cy, cz + 0.1), 0.4 * w, 90 * w * w * LUZ, (1.0, 0.95, 0.9))
    # Faixa baixa e frontal: faz o painel metálico acender por igual.
    area_light("faixa_painel", (cx, cy - 1.0 * w, cz - 0.15 * w), (cx, cy, cz - 0.05), 1.4 * w, 22 * w * w * LUZ, (1.0, 0.96, 0.9), 0.08 * w)


def camera(scene, target, distance, elevation_deg, lens=85):
    data = bpy.data.cameras.new("camera")
    data.lens = lens
    data.sensor_width = 36
    obj = bpy.data.objects.new("camera", data)
    bpy.context.collection.objects.link(obj)
    e = math.radians(elevation_deg)
    tx, ty, tz = target
    obj.location = (tx, ty - distance * math.cos(e), tz + distance * math.sin(e))
    if elevation_deg >= 89.9:
        # Vista de cima, reta: a câmera olha para baixo e o topo da imagem é
        # o fundo do pedal (+Y). (A conta de apontar fica indefinida a 90°.)
        obj.rotation_euler = (0.0, 0.0, 0.0)
    else:
        direction = Vector(target) - obj.location
        obj.rotation_euler = direction.to_track_quat("-Z", "Y").to_euler()
    scene.camera = obj
    return obj


def shadow_floor(z):
    bpy.ops.mesh.primitive_plane_add(size=6, location=(0, 0, z))
    floor = bpy.context.active_object
    floor.name = "chao_sombra"
    floor.is_shadow_catcher = True
    return floor


def use_gpu():
    """Liga a placa de vídeo (Metal no Mac, depois OptiX/CUDA/HIP) para o
    Cycles. Sem placa compatível (ou com NEXUS_GEAR_CPU=1), fica no processador."""
    if os.environ.get("NEXUS_GEAR_CPU"):
        return False
    prefs = bpy.context.preferences.addons["cycles"].preferences
    for kind in ("METAL", "OPTIX", "CUDA", "HIP", "ONEAPI"):
        try:
            prefs.compute_device_type = kind
        except TypeError:
            continue
        prefs.get_devices()
        gpus = [d for d in prefs.devices if d.type == kind]
        if gpus:
            for d in prefs.devices:
                # Só a placa dedicada: a integrada da Intel deixa tudo mais lento.
                d.use = d.type == kind and "Intel" not in d.name
            print("[gear] placa de vídeo:", kind, [d.name for d in gpus if d.use])
            return any(d.use for d in gpus)
    return False


def render_settings(scene, width, height, samples):
    scene.render.engine = "CYCLES"
    scene.cycles.device = "GPU" if use_gpu() else "CPU"
    scene.cycles.samples = samples
    scene.cycles.use_adaptive_sampling = True
    scene.cycles.adaptive_threshold = 0.01
    scene.cycles.use_denoising = True
    scene.cycles.denoiser = "OPENIMAGEDENOISE"
    scene.cycles.max_bounces = 8
    scene.render.film_transparent = True
    scene.render.resolution_x = width
    scene.render.resolution_y = height
    scene.render.resolution_percentage = 100
    scene.render.image_settings.file_format = "PNG"
    scene.render.image_settings.color_mode = "RGBA"
    scene.render.image_settings.color_depth = "8"
    scene.view_settings.view_transform = "AgX"
    scene.view_settings.look = "AgX - Medium High Contrast"
    scene.render.threads_mode = "AUTO"


def _math(nodes, links, op, a, b=None):
    """Nó de conta: a e b são saídas de outros nós ou números."""
    n = nodes.new("ShaderNodeMath")
    n.operation = op
    for i, v in enumerate((a, b)):
        if v is None:
            continue
        if isinstance(v, (int, float)):
            n.inputs[i].default_value = v
        else:
            links.new(v, n.inputs[i])
    return n.outputs[0]


def mat_diamond_plate(name, color, pitch_mm=11.0, rough=0.16):
    """Chapa de alumínio xadrez (diamond plate): gomos em relevo alternando
    +45 e -45 graus, alumínio polido (frente dos Rectifier)."""
    mat, nodes, links, bsdf = _principled(name)
    coords = _object_coords(nodes)
    bsdf.inputs["Base Color"].default_value = (*color, 1)
    bsdf.inputs["Metallic"].default_value = 1.0
    bsdf.inputs["Roughness"].default_value = rough
    s = 1000.0 / pitch_mm
    sep = nodes.new("ShaderNodeSeparateXYZ")
    links.new(coords, sep.inputs[0])
    x = _math(nodes, links, "MULTIPLY", sep.outputs["X"], s)
    z = _math(nodes, links, "MULTIPLY", sep.outputs["Z"], s)
    fx = _math(nodes, links, "SUBTRACT", _math(nodes, links, "FRACT", x), 0.5)
    fz = _math(nodes, links, "SUBTRACT", _math(nodes, links, "FRACT", z), 0.5)
    parity = _math(nodes, links, "MODULO", _math(nodes, links, "ADD", _math(nodes, links, "FLOOR", x), _math(nodes, links, "FLOOR", z)), 2.0)
    sign = _math(nodes, links, "SUBTRACT", 1.0, _math(nodes, links, "MULTIPLY", _math(nodes, links, "ABSOLUTE", parity), 2.0))
    sfz = _math(nodes, links, "MULTIPLY", fz, sign)
    u = _math(nodes, links, "MULTIPLY", _math(nodes, links, "ADD", fx, sfz), 0.7071)
    v = _math(nodes, links, "MULTIPLY", _math(nodes, links, "SUBTRACT", fx, sfz), 0.7071)
    du = _math(nodes, links, "POWER", _math(nodes, links, "DIVIDE", u, 0.36), 2.0)
    dv = _math(nodes, links, "POWER", _math(nodes, links, "DIVIDE", v, 0.085), 2.0)
    h = _math(nodes, links, "SUBTRACT", 1.0, _math(nodes, links, "ADD", du, dv))
    h = _math(nodes, links, "POWER", _math(nodes, links, "MAXIMUM", h, 0.0), 0.5)
    bump = nodes.new("ShaderNodeBump")
    bump.inputs["Strength"].default_value = 1.0
    bump.inputs["Distance"].default_value = 0.0012
    links.new(h, bump.inputs["Height"])
    # Riscos leves de polimento por cima.
    noise = nodes.new("ShaderNodeTexNoise")
    noise.inputs["Scale"].default_value = 900.0
    rmap = nodes.new("ShaderNodeMapRange")
    rmap.inputs["To Min"].default_value = rough - 0.05
    rmap.inputs["To Max"].default_value = rough + 0.08
    links.new(noise.outputs["Fac"], rmap.inputs["Value"])
    links.new(rmap.outputs["Result"], bsdf.inputs["Roughness"])
    links.new(bump.outputs["Normal"], bsdf.inputs["Normal"])
    return mat


def mat_perforated(name, color, hole_mm=3.2, pitch_mm=4.8, rough=0.38):
    """Grade de metal perfurado pintado: furos redondos em fileiras
    desencontradas, com o fundo escuro aparecendo (grade do Soldano)."""
    mat, nodes, links, bsdf = _principled(name)
    coords = _object_coords(nodes)
    bsdf.inputs["Metallic"].default_value = 0.6
    bsdf.inputs["Roughness"].default_value = rough
    s = 1000.0 / pitch_mm
    sep = nodes.new("ShaderNodeSeparateXYZ")
    links.new(coords, sep.inputs[0])
    z = _math(nodes, links, "MULTIPLY", sep.outputs["Z"], s)
    row = _math(nodes, links, "FLOOR", z)
    shift = _math(nodes, links, "MULTIPLY", _math(nodes, links, "MODULO", row, 2.0), 0.5)
    x = _math(nodes, links, "ADD", _math(nodes, links, "MULTIPLY", sep.outputs["X"], s), shift)
    fx = _math(nodes, links, "SUBTRACT", _math(nodes, links, "FRACT", x), 0.5)
    fz = _math(nodes, links, "SUBTRACT", _math(nodes, links, "FRACT", z), 0.5)
    d = _math(nodes, links, "SQRT", _math(nodes, links, "ADD", _math(nodes, links, "MULTIPLY", fx, fx), _math(nodes, links, "MULTIPLY", fz, fz)))
    r = 0.5 * hole_mm / pitch_mm
    solid = _math(nodes, links, "MINIMUM", _math(nodes, links, "MAXIMUM", _math(nodes, links, "DIVIDE", _math(nodes, links, "SUBTRACT", d, r), 0.06), 0.0), 1.0)
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
