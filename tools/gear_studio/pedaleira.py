# Nexus Gear Studio: a pedaleira onde a tela monta os pedais da cadeia.
# Estrutura de alumínio preto: ripas inclinadas cobertas de velcro, com vãos
# entre elas, e duas cabeceiras arredondadas. Vista de cima, com a mesma luz
# e a mesma lente dos pedais, para os pedais "assentarem" nela.
#
# Uso: blender -b -P pedaleira.py -- --saida <pasta> [--previa]
# Gera <pasta>/pedaleira_frente.png (a tela junta com os pedais) e o mapa.
import argparse
import json
import math
import os
import sys

import bpy

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import build  # noqa: E402
import studio as st  # noqa: E402

W = 0.92   # largura (X)
D = 0.36   # fundo (Y)
SLATS = 4  # ripas
CAP = 0.028  # largura das cabeceiras


def mat_velcro(name):
    """Velcro de loop: cinza quase preto, fosco, com pelinho miúdo."""
    mat = bpy.data.materials.new(name)
    mat.use_nodes = True
    nodes, links = mat.node_tree.nodes, mat.node_tree.links
    bsdf = nodes["Principled BSDF"]
    bsdf.inputs["Base Color"].default_value = (0.022, 0.022, 0.024, 1)
    bsdf.inputs["Roughness"].default_value = 0.95
    bsdf.inputs["Sheen Weight"].default_value = 0.6
    bsdf.inputs["Sheen Roughness"].default_value = 0.6
    tc = nodes.new("ShaderNodeTexCoord")
    noise = nodes.new("ShaderNodeTexNoise")
    noise.inputs["Scale"].default_value = 2600.0
    noise.inputs["Detail"].default_value = 6.0
    links.new(tc.outputs["Object"], noise.inputs["Vector"])
    bump = nodes.new("ShaderNodeBump")
    bump.inputs["Strength"].default_value = 0.5
    bump.inputs["Distance"].default_value = 0.0004
    links.new(noise.outputs["Fac"], bump.inputs["Height"])
    links.new(bump.outputs["Normal"], bsdf.inputs["Normal"])
    # Leve variação de tom, como tecido que já foi pisado.
    big = nodes.new("ShaderNodeTexNoise")
    big.inputs["Scale"].default_value = 18.0
    links.new(tc.outputs["Object"], big.inputs["Vector"])
    ramp = nodes.new("ShaderNodeValToRGB")
    ramp.color_ramp.elements[0].color = (0.016, 0.016, 0.018, 1)
    ramp.color_ramp.elements[1].color = (0.034, 0.034, 0.037, 1)
    links.new(big.outputs["Fac"], ramp.inputs["Fac"])
    links.new(ramp.outputs["Color"], bsdf.inputs["Base Color"])
    return mat


def build_board(fonts):
    frame = st.mat_metal("aluminio_preto", (0.035, 0.036, 0.04), rough=0.38)
    edge = st.mat_metal("aluminio_chanfro", (0.42, 0.43, 0.45), rough=0.3)
    velcro = mat_velcro("velcro")
    rubber = st.mat_plastic("borracha", (0.012, 0.012, 0.012), rough=0.7)

    inner = W - 2 * CAP
    pitch = D / SLATS
    slat_d = pitch * 0.66
    # Inclinação: a ripa de trás fica mais alta (como numa pedaleira de palco).
    z_front, z_back = 0.022, 0.058
    for i in range(SLATS):
        y = -D / 2 + pitch * (i + 0.5)
        t = i / (SLATS - 1)
        z = z_front + (z_back - z_front) * t
        # Perfil da ripa: barra de alumínio com chanfro claro nas bordas.
        st.box(f"ripa_{i}", (inner, slat_d, 0.012), (0, y, z - 0.006), bevel=0.0025, segments=3, material=edge)
        st.box(f"ripa_topo_{i}", (inner - 0.004, slat_d - 0.006, 0.004), (0, y, z + 0.0005), bevel=0.0012, material=frame)
        # Velcro colado no meio da ripa (sem cobrir as pontas).
        st.box(f"velcro_{i}", (inner - 0.05, slat_d * 0.78, 0.0022), (0, y, z + 0.0035), bevel=0.0006, segments=2, material=velcro)
        # Pé de apoio da ripa até o chão (só aparece pelos vãos).
        st.box(f"apoio_{i}", (inner, 0.006, z - 0.012), (0, y + slat_d * 0.35, (z - 0.012) / 2), material=frame)

    # Cabeceiras: chapas laterais arredondadas, da frente baixa até o fundo alto.
    for side in (-1, 1):
        x = side * (W / 2 - CAP / 2)
        cap = st.box(f"cabeceira_{side}", (CAP, D + 0.012, z_back + 0.012), (x, 0, (z_back + 0.012) / 2), bevel=0.008, segments=6, material=frame)
        # Corta a quina de cima à frente para seguir a inclinação.
        bpy.ops.mesh.primitive_cube_add(size=1, location=(x, -D / 2 - 0.02, z_back + 0.03))
        cutter = bpy.context.active_object
        cutter.scale = (CAP * 2, 0.26, 0.06)
        cutter.rotation_euler = (math.atan2(z_back - z_front, D) + 0.02, 0, 0)
        bpy.context.view_layer.update()
        st.cut(cap, cutter)
        st.smooth(cap)
        # Borda clara do chanfro em cima da cabeceira.
        st.box(f"friso_{side}", (CAP * 0.5, D * 0.86, 0.002), (x, 0.01, z_back + 0.002), bevel=0.0008, material=edge).rotation_euler = (
            math.atan2(z_back - z_front, D) * 0.9, 0, 0)
        for y in (-D / 2 + 0.02, D / 2 - 0.02):
            st.cylinder(f"pe_{side}_{y:.2f}", 0.011, 0.006, (x, y, 0.003), material=rubber, axis="Z", bevel=0.002)

    # Plaquinha com a marca na cabeceira da esquerda, na frente.
    plate_mat = st.mat_metal("placa", (0.62, 0.63, 0.66), rough=0.22)
    ink = st.mat_plastic("tinta", (0.01, 0.01, 0.012), rough=0.5)
    px = -(W / 2 - CAP / 2)
    st.box("placa", (CAP * 0.62, 0.07, 0.0012), (px, -D / 2 + 0.07, 0.031), bevel=0.0006, material=plate_mat)
    st.text("marca", "NEXUS", fonts["label"], 0.0085, (px, -D / 2 + 0.07, 0.0325), ink,
            rotation=(0, 0, math.radians(90)), spacing=1.25)
    return {"size": (W, D, z_back)}


def main():
    argv = sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else []
    p = argparse.ArgumentParser()
    p.add_argument("--saida", required=True)
    p.add_argument("--previa", action="store_true")
    p.add_argument("--largura", type=int, default=2400)
    p.add_argument("--amostras", type=int, default=160)
    a = p.parse_args(argv)
    os.makedirs(a.saida, exist_ok=True)

    scene = st.reset_scene()
    built = build_board(build.FONTS)
    w, d, h = built["size"]
    st.studio(scene, (0, 0, h / 2), w * 0.45, "pedal")
    st.shadow_floor(0.0)

    scale = 0.5 if a.previa else 1.0
    width = int(a.largura * scale)
    height = int(width * 0.42)
    st.render_settings(scene, width, height, max(24, int(a.amostras * (0.35 if a.previa else 1.0))))
    lens = 85
    view_w = w * 1.06
    dist = (view_w / 2) / math.tan(math.atan(18 / lens))
    st.camera(scene, (0, 0, h / 2), dist, 90.0, lens)
    bpy.context.view_layer.update()

    out = os.path.join(os.path.abspath(a.saida), "pedaleira_frente.png")
    scene.render.filepath = out
    bpy.ops.render.render(write_still=True)
    build.clean_shadow(out)

    # Mapa: só o nome e o tamanho (a tela usa o contorno da imagem).
    with open(os.path.join(a.saida, "pedaleira_mapa.json"), "w", encoding="utf-8") as f:
        json.dump({"nome": "Pedaleira", "familia": "pedaleira", "largura": width, "altura": height,
                   "posicoes": 1, "botoes": [], "canais": [], "rotulos": []}, f, ensure_ascii=False, indent=2)
    print("[gear] pedaleira:", out)


main()
