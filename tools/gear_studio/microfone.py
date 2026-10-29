# Nexus Gear Studio: os cinco microfones que a tela põe na frente da caixa,
# no falante e na posição em que cada IR foi gravado. Sem marca: só o
# formato de cada família.
#
#   dinamico_bastao     bastão escuro com a grade na ponta (estilo SM57)
#   dinamico_lateral    corpo largo e chato, grade com frisos (estilo MD421)
#   condensador_grande  corpo grosso em pé, cabeça grande de tela (estilo U87)
#   condensador_lapis   tubo fino com a grade na ponta (estilo KM184)
#   fita                corpo chato em pé, janela de grade oval (estilo R121)
#
# Mesma luz e mesma lente dos equipamentos, vistos de frente (a câmera está
# onde fica quem olha a caixa). O microfone aponta para a caixa (+Y) com a
# ponta na origem, encostada na tela; o corpo vem na direção de quem olha,
# meio de lado, preso num braço de pedestal que sai por baixo.
#
# Uso: blender -b -P microfone.py -- --saida <pasta> [--previa]
# Gera <pasta>/microfone_<nome>_frente.png e microfone_<nome>_mapa.json com
# a ponta (onde a tela encosta no falante) e os pixels por milímetro.
import argparse
import json
import math
import os
import sys

import bpy
from mathutils import Euler, Vector

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import build  # noqa: E402
import studio as st  # noqa: E402

NAMES = ["dinamico_bastao", "dinamico_lateral", "condensador_grande", "condensador_lapis", "fita"]


def mats():
    return {
        "body_dark": st.mat_plastic("corpo_escuro", (0.035, 0.036, 0.04), rough=0.42, coat=0.15),
        "body_black": st.mat_plastic("corpo_preto", (0.012, 0.012, 0.013), rough=0.5, coat=0.1),
        "nickel": st.mat_metal("niquel", (0.72, 0.71, 0.68), rough=0.22),
        "chrome": st.mat_metal("cromo", (0.92, 0.92, 0.93), rough=0.08),
        "grille_dark": st.mat_perforated("grade_escura", (0.2, 0.2, 0.21), hole_mm=0.9, pitch_mm=1.4, rough=0.4),
        "grille_light": st.mat_perforated("grade_clara", (0.6, 0.6, 0.6), hole_mm=0.9, pitch_mm=1.4, rough=0.3),
        "stand": st.mat_metal("pedestal", (0.03, 0.03, 0.032), rough=0.35),
    }


def tube(name, r0, r1, length, y0, material, vertices=64):
    """Tronco de cone ao longo de Y, do y0 para trás (-Y), raio r0 na frente e r1 atrás."""
    bpy.ops.mesh.primitive_cone_add(vertices=vertices, radius1=r0, radius2=r1, depth=length,
                                    location=(0, y0 - length / 2, 0), rotation=(math.radians(90), 0, 0))
    obj = bpy.context.active_object
    obj.name = name
    bpy.ops.object.transform_apply(location=False, rotation=True, scale=False)
    # O cone do Blender sai com o raio1 embaixo (-Z antes de girar = +Y depois).
    mod = obj.modifiers.new("chanfro", "BEVEL")
    mod.width = min(r0, r1) * 0.12
    mod.segments = 3
    mod.limit_method = "ANGLE"
    st.apply_modifiers(obj)
    obj.data.materials.append(material)
    st.smooth(obj, 40)
    return obj


def sphere(name, radius, location, scale, material):
    bpy.ops.mesh.primitive_uv_sphere_add(segments=64, ring_count=32, radius=radius, location=location)
    obj = bpy.context.active_object
    obj.name = name
    obj.scale = scale
    bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)
    obj.data.materials.append(material)
    st.smooth(obj, 60)
    return obj


def rod(name, start, end, radius, material):
    a, b = Vector(start), Vector(end)
    d = b - a
    bpy.ops.mesh.primitive_cylinder_add(vertices=32, radius=radius, depth=d.length, location=(a + b) / 2)
    obj = bpy.context.active_object
    obj.name = name
    obj.rotation_euler = d.to_track_quat("Z", "Y").to_euler()
    obj.data.materials.append(material)
    st.smooth(obj, 40)
    return obj


# Cada microfone é montado com a ponta na origem apontando para +Y e devolve
# as peças e o ponto de onde sai o braço do pedestal (para o braço seguir).

def end_stick(m):
    """Bastão: grade de 50 mm na frente, corpo afinando para trás, anel do conector."""
    parts = [
        tube("grade", 0.0165, 0.0165, 0.046, 0.0, m["grille_dark"]),
        tube("anel", 0.0168, 0.0168, 0.004, -0.046, m["body_dark"]),
        tube("corpo", 0.0162, 0.0125, 0.098, -0.050, m["body_dark"]),
        tube("conector", 0.0118, 0.0118, 0.010, -0.148, m["nickel"]),
    ]
    return parts, (0, -0.110, -0.014)


def end_side(m):
    """Lateral: corpo largo e chato, a frente com grade de frisos."""
    body = st.box("corpo", (0.046, 0.150, 0.034), (0, -0.115, 0), bevel=0.012, segments=6, material=m["body_black"])
    head = st.box("grade", (0.047, 0.042, 0.035), (0, -0.021, 0), bevel=0.013, segments=6, material=m["grille_dark"])
    ring = st.box("frisos", (0.048, 0.006, 0.036), (0, -0.044, 0), bevel=0.012, segments=6, material=m["nickel"])
    tail = tube("conector", 0.0115, 0.0115, 0.012, -0.190, m["nickel"])
    return [body, head, ring, tail], (0, -0.120, -0.018)


def large_condenser(m):
    """Condensador grande, de lado (a cabeça olha para a caixa): corpo em pé,
    cabeça oval de tela em cima. A ponta é o centro da cabeça."""
    head = sphere("cabeca", 0.030, (0, -0.030, 0), (1.0, 0.85, 1.25), m["grille_light"])
    collar = st.cylinder("colar", 0.0295, 0.010, (0, -0.030, -0.040), axis="Z", material=m["nickel"], bevel=0.002)
    body = st.cylinder("corpo", 0.028, 0.150, (0, -0.030, -0.120), axis="Z", material=m["nickel"], bevel=0.003)
    base = st.cylinder("base", 0.0245, 0.016, (0, -0.030, -0.203), axis="Z", material=m["body_dark"], bevel=0.002)
    return [head, collar, body, base], (0, -0.030, -0.211)


def pencil(m):
    """Lápis: tubo fino e comprido, grade curta na ponta."""
    parts = [
        tube("grade", 0.0105, 0.0105, 0.022, 0.0, m["grille_light"]),
        tube("anel", 0.0108, 0.0108, 0.003, -0.022, m["nickel"]),
        tube("corpo", 0.0105, 0.0105, 0.085, -0.025, m["nickel"]),
    ]
    return parts, (0, -0.080, -0.012)


def ribbon(m):
    """Fita, de lado: corpo chato em pé com janela oval de grade escura em cima,
    vazada (aparece dos dois lados, como na fita de verdade, que capta frente e trás)."""
    body = st.box("corpo", (0.036, 0.026, 0.165), (0, -0.013, -0.050), bevel=0.012, segments=6, material=m["body_black"])
    window = sphere("janela", 0.018, (0, -0.013, 0), (0.85, 1.05, 1.7), m["grille_dark"])
    trim = sphere("moldura", 0.020, (0, -0.013, 0), (0.92, 0.78, 1.76), m["chrome"])
    tail = st.cylinder("conector", 0.010, 0.012, (0, -0.013, -0.138), axis="Z", material=m["nickel"])
    return [body, trim, window, tail], (0, -0.013, -0.144)


BUILDERS = [end_stick, end_side, large_condenser, pencil, ribbon]
# De frente o microfone mostra o lado: os de ponta giram mais, os de lado
# quase nada (a cabeça já está virada para a caixa).
POSE = [(-12, 0, 38), (-12, 0, 38), (0, 0, 22), (-12, 0, 40), (0, 0, 22)]


def build_mic(i, m):
    parts, mount = BUILDERS[i](m)
    rx, ry, rz = POSE[i]
    rot = Euler((math.radians(rx), math.radians(ry), math.radians(rz)))
    clip = st.box("presilha", (0.02, 0.026, 0.02), mount, bevel=0.006, segments=4, material=m["stand"])
    for obj in parts + [clip]:
        obj.rotation_euler.rotate(rot)
        obj.location.rotate(rot)
    # Haste do pedestal: do encaixe, desce reta até o chão da caixa mais alta (8x10)
    # (já com o microfone girado, para não sair pelo lado da imagem).
    top = Vector(mount)
    top.rotate(rot)
    rod("haste", top, top + Vector((0.02, -0.04, -1.5)), 0.0055, m["stand"])
    bpy.context.view_layer.update()


def main():
    argv = sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else []
    p = argparse.ArgumentParser()
    p.add_argument("--saida", required=True)
    p.add_argument("--previa", action="store_true")
    p.add_argument("--amostras", type=int, default=160)
    p.add_argument("--largura", type=int, default=900)
    p.add_argument("--so", default="", help="só este microfone (nome)")
    a = p.parse_args(argv)
    os.makedirs(a.saida, exist_ok=True)

    for i, name in enumerate(NAMES):
        if a.so and a.so != name:
            continue
        scene = st.reset_scene()
        m = mats()
        build_mic(i, m)
        # Imagem em pé: 42 cm de largura e 1,4 m de altura, com a ponta perto
        # de cima e a haste descendo até embaixo (a tela corta no chão da caixa).
        view, tall = 0.42, 3.4
        st.studio(scene, (0, -0.1, 0), view, "caixa")
        scale = 0.5 if a.previa else 1.0
        width = int(a.largura * scale)
        st.render_settings(scene, width, int(width * tall), max(24, int(a.amostras * (0.35 if a.previa else 1.0))))
        lens = 85
        dist = (view / 2) / math.tan(math.atan(18 / lens))
        # A mesma elevação das caixas (8 graus).
        cam = st.camera(scene, (0, 0, -view * tall * 0.36), dist, 8.0, lens)
        cam.data.sensor_fit = "HORIZONTAL"
        bpy.context.view_layer.update()

        out = os.path.join(os.path.abspath(a.saida), f"microfone_{name}_frente.png")
        scene.render.filepath = out
        bpy.ops.render.render(write_still=True)

        tip = build.to_pixels(scene, cam, (0, 0, 0))
        side = build.to_pixels(scene, cam, (0.1, 0, 0))
        with open(os.path.join(a.saida, f"microfone_{name}_mapa.json"), "w", encoding="utf-8") as f:
            json.dump({"nome": name, "familia": "microfone", "largura": width, "altura": int(width * tall), "posicoes": 1,
                       "botoes": [], "canais": [], "rotulos": [],
                       "ponta": [round(tip[0], 1), round(tip[1], 1)],
                       "pixels_por_mm": round((side[0] - tip[0]) / 100.0, 4)}, f, ensure_ascii=False, indent=2)
        print("[gear] microfone:", out)


main()
