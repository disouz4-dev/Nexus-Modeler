# Nexus Gear Studio: monta um equipamento a partir da receita e renderiza o
# pacote visual que o plugin usa.
#
#   blender -b --factory-startup -P tools/gear_studio/build.py -- \
#       --receita tools/gear_studio/recipes/brit800.json --saida build/gear/brit800 [--previa]
#
# Saída:
#   frente.png       o equipamento inteiro, fundo transparente com sombra
#   botao_<id>.png   cada botão em N posições, empilhadas de cima para baixo
#   mapa.json        tamanho da imagem e onde fica cada botão
import argparse
import json
import math
import os
import sys
import time

import bpy
import numpy as np
from bpy_extras.object_utils import world_to_camera_view
from mathutils import Vector

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import amp_head  # noqa: E402
import cab  # noqa: E402
import pedal  # noqa: E402
import studio as st  # noqa: E402

ROOT = os.path.abspath(os.path.join(HERE, "..", ".."))
FONTS = {
    "label": os.path.join(ROOT, "assets", "fonts", "oswald-Oswald[wght].ttf"),
    "script": os.path.join(ROOT, "assets", "fonts", "yellowtail-Yellowtail-Regular.ttf"),
    # Script grosso e inclinado, para os cabeçotes de inspiração Mesa/Boogie.
    "mesa": os.path.join(ROOT, "assets", "fonts", "lobster-Lobster-Regular.ttf"),
    "ui": os.path.join(ROOT, "assets", "fonts", "inter-Inter[opsz,wght].ttf"),
}


def args():
    argv = sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else []
    p = argparse.ArgumentParser()
    p.add_argument("--receita", required=True)
    p.add_argument("--saida", required=True)
    p.add_argument("--previa", action="store_true", help="metade da resolução, poucas posições")
    p.add_argument("--so-frente", action="store_true")
    p.add_argument("--so-mapa", action="store_true", help="refaz só o mapa.json, sem renderizar")
    p.add_argument("--posicoes", type=int, default=101)
    p.add_argument("--amostras", type=int, default=160)
    p.add_argument("--largura", type=int, default=2400)
    return p.parse_args(argv)


def to_pixels(scene, cam, point):
    co = world_to_camera_view(scene, cam, Vector(point))
    return co.x * scene.render.resolution_x, (1.0 - co.y) * scene.render.resolution_y


def slider_box(scene, cam, ctl):
    """Retângulo em pixels que cobre o rasgo inteiro do deslizante (o cursor
    em qualquer posição) e a profundidade do cursor."""
    cx, cy, cz = ctl["center"]
    pts = []
    for dx in (-ctl["w"] / 2, ctl["w"] / 2):
        for dz in (-ctl["h"] / 2, ctl["h"] / 2):
            for dy in (-ctl["depth"] / 2, ctl["depth"] / 2):
                pts.append(to_pixels(scene, cam, (cx + dx, cy + dy, cz + dz)))
    xs = [p[0] for p in pts]
    ys = [p[1] for p in pts]
    x0, y0 = int(math.floor(min(xs))) - 2, int(math.floor(min(ys))) - 2
    w, h = int(math.ceil(max(xs))) + 2 - x0, int(math.ceil(max(ys))) + 2 - y0
    return {"x": x0, "y": y0, "size": max(w, h), "w": w, "h": h, "rect": True}


def knob_box(scene, cam, ctl):
    if ctl.get("tipo") == "deslizante":
        return slider_box(scene, cam, ctl)
    """Quadrado em pixels que cobre o botão inteiro (frente, lateral e
    saia) com folga, e o raio do círculo de recorte. O eixo do botão é Y no
    amplificador (olha para a frente) e Z no pedal (olha para cima)."""
    center = Vector(ctl["center"])
    axis = Vector(ctl.get("axis", (0.0, 1.0, 0.0))).normalized()
    u = axis.orthogonal().normalized()
    v = axis.cross(u).normalized()
    r = ctl["outer_r"] * 1.06
    pts = []
    for k in range(48):
        a = 2 * math.pi * k / 48
        ring = u * (r * math.cos(a)) + v * (r * math.sin(a))
        for off in (-ctl["depth"] / 2 - 0.001, ctl["depth"] / 2):
            pts.append(to_pixels(scene, cam, center + axis * off + ring))
    xs = [p[0] for p in pts]
    ys = [p[1] for p in pts]
    mx, my = (min(xs) + max(xs)) / 2, (min(ys) + max(ys)) / 2
    radius = max(max(xs) - min(xs), max(ys) - min(ys)) / 2 + 2
    size = int(math.ceil(radius * 2 + 4))
    x0 = int(round(mx - size / 2))
    y0 = int(round(my - size / 2))
    return {"x": x0, "y": y0, "size": size, "cx": mx - x0, "cy": my - y0, "radius": radius}


def load_pixels(path):
    img = bpy.data.images.load(path, check_existing=False)
    w, h = img.size
    arr = np.empty(w * h * 4, dtype=np.float32)
    img.pixels.foreach_get(arr)
    bpy.data.images.remove(img)
    # O Blender guarda de baixo para cima; viramos para cima para baixo.
    return arr.reshape(h, w, 4)[::-1].copy()


def save_pixels(path, arr):
    h, w, _ = arr.shape
    img = bpy.data.images.new("saida", w, h, alpha=True)
    img.colorspace_settings.name = "sRGB"
    img.alpha_mode = "STRAIGHT"
    img.pixels.foreach_set(arr[::-1].astype(np.float32).ravel())
    img.filepath_raw = path
    img.file_format = "PNG"
    img.save()
    bpy.data.images.remove(img)


def clean_shadow(path):
    """O chão de sombra pega um véu fraco em toda a imagem; sobra só a sombra
    de contato embaixo do equipamento. Pixels opacos (o equipamento) não mudam."""
    arr = load_pixels(path)
    a = arr[..., 3]
    soft = a < 0.995
    a[soft] = np.clip((a[soft] - 0.16) / 0.84, 0.0, 1.0) ** 1.3
    save_pixels(path, arr)


def set_knobs(controls, value):
    for c in controls:
        if c.get("tipo") == "deslizante":
            x, y, _ = c["pivot"].location
            c["pivot"].location = (x, y, c["z0"] + (value - 0.5) * c["curso"])
        else:
            c["pivot"].rotation_euler = (0, math.radians(-150 + 300 * value), 0)


def main():
    a = args()
    rec = json.load(open(a.receita, encoding="utf-8"))
    os.makedirs(a.saida, exist_ok=True)
    scale = 0.5 if a.previa else 1.0
    frames = 11 if a.previa else a.posicoes
    samples = max(24, int(a.amostras * (0.35 if a.previa else 1.0)))

    scene = st.reset_scene()
    builder = {"cabecote": amp_head, "caixa": cab, "pedal": pedal}[rec["tipo"]]
    built = builder.build(rec, FONTS)
    frame = rec.get("quadro", {"largura_rel": 1.10, "proporcao": 0.44, "elevacao": 10.0})
    W, D, H = built["size"]
    center = (0, 0, H / 2)
    st.studio(scene, center, W, rec["tipo"])
    st.shadow_floor(0.0)

    width = int(a.largura * scale)
    height = int(width * frame["proporcao"])
    st.render_settings(scene, width, height, samples)
    # Cores muito saturadas (amarelo) desbotam no contraste padrão: a receita
    # pode pedir outro "look" do AgX só para ela.
    if rec.get("transformacao"):
        scene.view_settings.view_transform = rec["transformacao"]
        scene.view_settings.look = "None"
    if rec.get("aparencia"):
        scene.view_settings.look = rec["aparencia"]
    # Enquadra pela largura com folga para a sombra e a alça.
    lens = 85
    view_w = W * frame["largura_rel"]
    dist = (view_w / 2) / math.tan(math.atan(18 / lens))
    cam = st.camera(scene, built.get("target", (0, built["front"], H * 0.52)), dist, frame["elevacao"], lens)

    controls = built["controls"]
    bpy.context.view_layer.update()  # câmera e peças no lugar antes de medir
    if not a.so_mapa:
        bpy.ops.wm.save_as_mainfile(filepath=os.path.join(os.path.abspath(a.saida), "cena.blend"))
        set_knobs(controls, 0.5)
        t0 = time.time()
        scene.render.filepath = os.path.join(os.path.abspath(a.saida), "frente.png")
        bpy.ops.render.render(write_still=True)
        clean_shadow(scene.render.filepath)
        print(f"[gear] frente em {time.time() - t0:.0f} s")

    layout = {"nome": rec["nome"], "familia": rec["familia"], "largura": width, "altura": height,
              "posicoes": frames, "botoes": [],
              # Canais do amplificador original (nome de cada um), se houver.
              "canais": [c["nome"] for c in rec.get("canais", [])]}
    boxes = []
    for c in controls:
        b = knob_box(scene, cam, c)
        boxes.append(b)
        entry = {"id": c["id"], "x": b["x"], "y": b["y"], "tamanho": b["size"],
                 "arquivo": f"botao_{c['id']}.png", "angulo_min": -150, "angulo_max": 150}
        if b.get("rect"):
            # Deslizante: quadro retangular (largura x altura), arrasta na vertical.
            entry.update({"tipo": "deslizante", "largura": b["w"], "altura": b["h"]})
        layout["botoes"].append(entry)
    # Rótulos para a tela escrever: centro, altura da letra em pixels,
    # achatamento da perspectiva e cor da tinta (sRGB).
    def srgb(c):
        c = max(0.0, min(1.0, c))
        return c * 12.92 if c <= 0.0031308 else 1.055 * c ** (1 / 2.4) - 0.055
    layout["rotulos"] = []
    for lab in st.LABELS:
        p = Vector(lab["local"])
        up = Vector((0, 0, 1)) if lab["cima"] == "Z" else Vector((0, 1, 0))
        s = lab["tamanho"]
        cx, cy = to_pixels(scene, cam, p)
        rx, ry = to_pixels(scene, cam, p + Vector((s, 0, 0)))
        ux, uy = to_pixels(scene, cam, p + up * s)
        horizontal = math.hypot(rx - cx, ry - cy)
        vertical = math.hypot(ux - cx, uy - cy)
        color = "#" + "".join(f"{round(srgb(v) * 255):02x}" for v in lab["cor"])
        layout["rotulos"].append({"texto": lab["texto"], "x": round(cx, 1), "y": round(cy, 1),
                                  "altura": round(horizontal, 2), "achatamento": round(vertical / max(horizontal, 1e-6), 3),
                                  "cor": color, "espaco": lab["espaco"]})
    if "footswitch" in built:
        (fx, fy, fz), fr = built["footswitch"]
        px, py = to_pixels(scene, cam, (fx, fy, fz))
        ex, _ = to_pixels(scene, cam, (fx + fr, fy, fz))
        layout["pisante"] = {"x": round(px), "y": round(py), "raio": round(abs(ex - px))}
    if built.get("channel_leds"):
        layout["leds_canais"] = []
        for (lx, ly, lz), lr in built["channel_leds"]:
            px, py = to_pixels(scene, cam, (lx, ly, lz))
            ex, _ = to_pixels(scene, cam, (lx + lr, ly, lz))
            layout["leds_canais"].append({"x": round(px, 1), "y": round(py, 1), "raio": round(abs(ex - px), 1)})
    if built.get("led"):
        (lx, ly, lz), lr = built["led"]
        px, py = to_pixels(scene, cam, (lx, ly, lz))
        ex, _ = to_pixels(scene, cam, (lx + lr, ly, lz))
        layout["led"] = {"x": round(px, 1), "y": round(py, 1), "raio": round(abs(ex - px), 1)}
    with open(os.path.join(a.saida, "mapa.json"), "w", encoding="utf-8") as f:
        json.dump(layout, f, ensure_ascii=False, indent=2)
    if a.so_frente or a.so_mapa or not controls:
        return

    # Uma faixa de render cobre todos os botões; cada posição gira todos
    # juntos e recorta cada botão dessa faixa.
    x0 = min(b["x"] for b in boxes)
    x1 = max(b["x"] + b.get("w", b["size"]) for b in boxes)
    y0 = min(b["y"] for b in boxes)
    y1 = max(b["y"] + b.get("h", b["size"]) for b in boxes)
    r = scene.render
    r.use_border = True
    r.use_crop_to_border = True
    r.border_min_x, r.border_max_x = x0 / width, x1 / width
    r.border_min_y, r.border_max_y = 1 - y1 / height, 1 - y0 / height

    strips = [np.zeros((b.get("h", b["size"]) * frames, b.get("w", b["size"]), 4), dtype=np.float32) for b in boxes]
    tmp = os.path.join(os.path.abspath(a.saida), "_faixa.png")
    t0 = time.time()
    for f in range(frames):
        set_knobs(controls, f / (frames - 1))
        r.filepath = tmp
        bpy.ops.render.render(write_still=True)
        band = load_pixels(tmp)
        # A faixa pode sair um pixel diferente do pedido (arredondamento do
        # Blender); alinhamos pelo canto de cima à esquerda.
        for b, strip in zip(boxes, strips):
            ox, oy = b["x"] - x0, b["y"] - y0
            bw, bh = b.get("w", b["size"]), b.get("h", b["size"])
            # A faixa pode sair um pixel menor que o pedido; o que faltar
            # fica transparente (cai fora do círculo do botão).
            crop = np.zeros((bh, bw, 4), dtype=np.float32)
            part = band[oy:oy + bh, ox:ox + bw]
            crop[:part.shape[0], :part.shape[1]] = part
            if not b.get("rect"):
                yy, xx = np.mgrid[0:bh, 0:bw]
                d = np.sqrt((xx + 0.5 - b["cx"]) ** 2 + (yy + 0.5 - b["cy"]) ** 2)
                mask = np.clip((b["radius"] - d) / 2.0, 0.0, 1.0)
                crop[..., 3] *= mask
            strip[f * bh:(f + 1) * bh] = crop
        print(f"[gear] posição {f + 1}/{frames} ({time.time() - t0:.0f} s)")
    os.remove(tmp)
    for c, strip in zip(controls, strips):
        save_pixels(os.path.join(a.saida, f"botao_{c['id']}.png"), strip)
    print("[gear] pronto")


if __name__ == "__main__":
    if "--limpar" in sys.argv:
        clean_shadow(sys.argv[-1])
    else:
        main()
