# Nexus Gear Studio: peças de botão giratório comuns a amplificadores e
# pedais. O botão é montado olhando para -Y (para a câmera do amplificador);
# no pedal ele é deitado para olhar para cima.
import math

import bmesh
import bpy

import studio as st


def knurled_body(name, radius, depth, flutes, material):
    """Corpo de botão com estrias (as ranhuras laterais que dão pegada)."""
    mesh = bpy.data.meshes.new(name)
    bm = bmesh.new()
    ring_back, ring_front = [], []
    steps = flutes * 4
    for i in range(steps):
        a = 2 * math.pi * i / steps
        # Estria: dois pontos no raio cheio, dois um pouco para dentro.
        r = radius if (i % 4) in (0, 1) else radius * 0.94
        x, z = r * math.sin(a), r * math.cos(a)
        ring_back.append(bm.verts.new((x, 0.0, z)))
        ring_front.append(bm.verts.new((x * 0.97, -depth, z * 0.97)))
    for i in range(steps):
        j = (i + 1) % steps
        bm.faces.new((ring_back[i], ring_back[j], ring_front[j], ring_front[i]))
    bm.faces.new(list(reversed(ring_back)))
    bm.faces.new(ring_front)
    bmesh.ops.recalc_face_normals(bm, faces=bm.faces)
    bm.to_mesh(mesh)
    bm.free()
    obj = bpy.data.objects.new(name, mesh)
    bpy.context.collection.objects.link(obj)
    mod = obj.modifiers.new("chanfro", "BEVEL")
    mod.width = radius * 0.05
    mod.segments = 2
    mod.limit_method = "ANGLE"
    mod.angle_limit = math.radians(50)
    st.apply_modifiers(obj)
    obj.data.materials.append(material)
    st.smooth(obj, 30)
    return obj




def knob(name, radius, height, mats, skirt=False, cap=True, side_notch=True, flutes=24):
    """Botão de pedal montado na origem, olhando para -Y (como no
    amplificador). Devolve o ponto de giro; quem chama posiciona e deita.
    mats: body, pointer e, se houver tampa, cap."""
    pivot = bpy.data.objects.new(f"giro_{name}", None)
    bpy.context.collection.objects.link(pivot)
    bpy.context.view_layer.update()
    parts = []
    base_y = 0.0
    if skirt:
        parts.append(st.cylinder(f"saia_{name}", radius * 1.28, 0.0025, (0, -0.00125, 0), material=mats["body"], bevel=0.0007))
        base_y = -0.0025
    body = knurled_body(f"corpo_{name}", radius, height, flutes, mats["body"])
    body.location = (0, base_y, 0)
    parts.append(body)
    top_y = base_y - height
    if cap:
        parts.append(st.cylinder(f"tampa_{name}", radius * 0.78, 0.0012, (0, top_y - 0.0003, 0), material=mats["cap"], bevel=0.0003))
        line_y = top_y - 0.0010
    else:
        line_y = top_y - 0.0002
    parts.append(st.box(f"traco_{name}", (radius * 0.09, 0.0005, radius * 0.66), (0, line_y, radius * 0.40), material=mats["pointer"]))
    if side_notch:
        parts.append(st.box(f"traco_lat_{name}", (radius * 0.09, height * 0.8, radius * 0.06),
                            (0, base_y - height * 0.45, radius * 0.985), material=mats["pointer"]))
    for p in parts:
        p.parent = pivot
        p.matrix_parent_inverse = pivot.matrix_world.inverted()
    return pivot


def lay_on_top(pivot, location):
    """Deita o botão sobre uma face de cima (olhando para +Z) no ponto dado.
    O traço em zero aponta para o fundo do pedal (12 horas, vendo de frente)."""
    mount = bpy.data.objects.new(pivot.name + "_base", None)
    bpy.context.collection.objects.link(mount)
    mount.location = location
    mount.rotation_euler = (math.radians(-90), 0, 0)
    bpy.context.view_layer.update()
    pivot.parent = mount
    return mount
