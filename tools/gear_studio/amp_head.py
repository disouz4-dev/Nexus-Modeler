# Nexus Gear Studio: família "cabeçote" (Brit 800 e parentes). Monta o
# modelo 3D a partir da receita. Medidas em metros; a frente do cabeçote
# olha para -Y, o chão é z = 0 e o centro fica em x = 0.
import math

import bpy

import knobs
import brand_plate
import studio as st


def _knob(rec, idx, ctl, center, panel_front, mats):
    """Botão giratório: saia preta larga, corpo estriado, tampa dourada de
    escova circular com o traço indicador gravado. Tudo pendurado num ponto
    de giro, que a ferramenta gira para renderizar as posições."""
    style = rec["botao_estilo"]
    r = style["diametro_m"] / 2
    x, z = center
    pivot = bpy.data.objects.new(f"giro_{ctl['id']}", None)
    bpy.context.collection.objects.link(pivot)
    pivot.location = (x, panel_front, z)
    bpy.context.view_layer.update()

    parts = []
    skirt = st.cylinder(f"saia_{idx}", r * 1.22, 0.0032, (x, panel_front - 0.0016, z), material=mats["knob"], bevel=0.0009)
    parts.append(skirt)
    body = knobs.knurled_body(f"corpo_{idx}", r, 0.0145, 30, mats["knob"])
    body.location = (x, panel_front - 0.003, z)
    parts.append(body)
    top_y = panel_front - 0.003 - 0.0145
    cap = st.cylinder(f"tampa_{idx}", r * 0.80, 0.0016, (x, top_y - 0.0004, z), material=mats["cap"], bevel=0.0004)
    parts.append(cap)
    # Traço indicador gravado na tampa e na lateral, apontando para cima.
    line = st.box(f"traco_{idx}", (r * 0.075, 0.0006, r * 0.62), (x, top_y - 0.0013, z + r * 0.42), material=mats["pointer"])
    parts.append(line)
    notch = st.box(f"traco_lat_{idx}", (r * 0.08, 0.012, r * 0.06), (x, panel_front - 0.0095, z + r * 0.985), material=mats["pointer"])
    parts.append(notch)
    for p in parts:
        p.parent = pivot
        p.matrix_parent_inverse = pivot.matrix_world.inverted()
    return pivot, r * 1.22


def _scale(name, center, y, mats, font_num, radius, num_size=0.0062):
    """Escala impressa em volta do botão: 11 traços de 0 a 10 e os números
    pares, num arco de 300 graus como nos amplificadores reais."""
    x, z = center
    objs = []
    for i in range(11):
        ang = math.radians(-150 + 30 * i)
        long_tick = i % 2 == 0
        length = 0.0034 if long_tick else 0.0022
        rr = radius + length / 2
        tick = st.box(f"{name}_traco{i}", (0.00055, 0.0001, length),
                      (x + rr * math.sin(ang), y, z + rr * math.cos(ang)), material=mats["ink"])
        tick.rotation_euler = (0, ang, 0)
        objs.append(tick)
        if long_tick:
            rn = radius + 0.0072
            num = st.text(f"{name}_n{i}", str(i), font_num, num_size,
                          (x + rn * math.sin(ang), y, z + rn * math.cos(ang)), mats["ink"])
            objs.append(num)
    return objs


def _slider(idx, ctl, x, z_mid, travel, panel_front, ink_y, mats, font_num, text_color):
    """Deslizante do equalizador gráfico: rasgo escuro no painel, escala de
    traços e o cursor (pendurado num ponto que a ferramenta sobe e desce)."""
    slot_h = travel + 0.008
    st.box(f"rasgo_{idx}", (0.0034, 0.004, slot_h), (x, panel_front + 0.0008, z_mid), bevel=0.0012, material=mats["hole"])
    for k in range(7):
        tz = z_mid - travel / 2 + travel * k / 6
        long_tick = k in (0, 3, 6)
        st.box(f"eq_traco_{idx}_{k}", (0.0042 if long_tick else 0.0026, 0.0001, 0.0005),
               (x - 0.0068, ink_y, tz), material=mats["ink"])
    pivot = bpy.data.objects.new(f"giro_{ctl['id']}", None)
    bpy.context.collection.objects.link(pivot)
    pivot.location = (x, panel_front, z_mid)
    bpy.context.view_layer.update()
    cap = st.box(f"cursor_{idx}", (0.0105, 0.010, 0.0062), (x, panel_front - 0.005, z_mid), bevel=0.0012, material=mats["slider"])
    line = st.box(f"cursor_traco_{idx}", (0.0105 * 0.86, 0.0004, 0.0007), (x, panel_front - 0.0102, z_mid), material=mats["pointer"])
    for part in (cap, line):
        part.parent = pivot
        part.matrix_parent_inverse = pivot.matrix_world.inverted()
    st.label(ctl["rotulo"], (x, ink_y, z_mid - travel / 2 - 0.0098), 0.0082, color=text_color)
    return {"id": ctl["id"], "pivot": pivot, "tipo": "deslizante", "z0": z_mid, "curso": travel,
            "center": (x, panel_front - 0.005, z_mid), "w": 0.0135, "h": travel + 0.012, "depth": 0.012}


def build(rec, fonts):
    W = rec["medidas_m"]["largura"]
    H = rec["medidas_m"]["altura"]
    D = rec["medidas_m"]["profundidade"]
    t = 0.019               # aba de couro em volta da frente
    front = -D / 2
    lay = {"liga_x": 0.036, "modelo_x": 0.118, "modelo_tamanho": 0.030, "modelo_inclinacao": 0.18,
           "botoes_x0": 0.232, "botoes_passo": 0.0615, "escala_raio": 0.0182, "escala_numero": 0.0062,
           "entradas_dir": 0.088, "entradas_passo": 0.05, "marca_dir": 0.062,
           "deslizantes_x0": 0.0, "deslizantes_passo": 0.021, "deslizantes_curso": 0.040}
    lay.update(rec.get("disposicao", {}))
    style = rec["botao_estilo"]
    fascia_cloth = rec["fachada"].get("tipo") == "tela"
    text_color = rec["painel"]["texto_cor"]
    mats = {
        "tolex": st.mat_leather("couro", rec["couro"]["cor"], rec["couro"]["grao_mm"]),
        "fascia": (st.mat_grille_cloth("fachada", rec["fachada"]["cor_a"], rec["fachada"]["cor_b"], rec["fachada"]["trama_mm"])
                   if fascia_cloth else
                   st.mat_diamond_plate("fachada", rec["fachada"]["cor"], rec["fachada"].get("passo_mm", 11.0))
                   if rec["fachada"].get("tipo") == "xadrez" else
                   st.mat_perforated("fachada", rec["fachada"]["cor"], rec["fachada"].get("furo_mm", 3.2), rec["fachada"].get("passo_mm", 4.8),
                                     shape=rec["fachada"].get("furo", "redondo"))
                   if rec["fachada"].get("tipo") == "grade" else
                   st.mat_leather("fachada", rec["fachada"]["cor"], rec["fachada"]["grao_mm"], rough=0.42, strength=0.35)),
        "piping": st.mat_plastic("friso", rec["fachada"].get("friso", (0.8, 0.78, 0.72)), rough=0.32, coat=0.4),
        "logo": (st.mat_metal("logo", rec["logo"]["cor"], rough=0.12) if rec["logo"].get("metal")
                 else st.mat_plastic("logo", rec["logo"]["cor"], rough=0.25, coat=0.6)),
        "logo_edge": st.mat_plastic("logo_borda", (0.01, 0.01, 0.01), rough=0.2),
        "panel": (st.mat_plastic("painel", rec["painel"]["cor"], rough=0.48, coat=0.15) if rec["painel"].get("pintado")
                  else st.mat_brushed("painel", rec["painel"]["cor"], rough=0.40, streak_axis="X")),
        "ink": st.mat_paint("tinta", rec["painel"]["texto_cor"], rough=0.6),
        # "corpo_metal": botão de metal recartilhado (os cromados do Rectifier).
        "knob": (st.mat_metal("botao", style["corpo"], rough=0.12) if style.get("corpo_metal")
                 else st.mat_plastic("botao", style["corpo"], rough=0.28, coat=0.3)),
        "cap": (st.mat_plastic("tampa", style["tampa"], rough=0.18, coat=0.5) if style.get("tampa_plastico")
                else st.mat_brushed("tampa", style["tampa"], rough=0.14, radial_axis="Y")),
        "pointer": st.mat_paint("traco", style.get("traco", (0.01, 0.01, 0.01)), rough=0.4),
        "slider": st.mat_plastic("cursor", style.get("cursor", (0.02, 0.02, 0.02)), rough=0.3, coat=0.3),
        "chrome": st.mat_metal("cromo", (0.92, 0.92, 0.93), rough=0.10),
        "black_metal": st.mat_metal("metal_preto", (0.035, 0.035, 0.037), rough=0.32),
        "hole": st.mat_paint("furo", (0.0, 0.0, 0.0), rough=0.9),
        "rocker": st.mat_glow("chave_luz", (0.85, 0.015, 0.008), 1.6),
        "rocker_house": st.mat_plastic("chave_caixa", (0.012, 0.012, 0.012), rough=0.3),
        "handle": st.mat_leather("alca", (0.012, 0.011, 0.010), 1.8, rough=0.38, strength=0.4),
    }

    # Caixa de madeira encapada, com a frente rebaixada.
    shell = st.box("caixa", (W, D, H), (0, 0, H / 2), bevel=0.013, segments=6, material=mats["tolex"])
    cutter = st.box("corte_frente", (W - 2 * t, 0.06, H - 2 * t), (0, front, H / 2), bevel=0.004)
    st.cut(shell, cutter)
    st.smooth(shell, 35)

    ph = rec["painel"]["altura_m"]
    inner_w = W - 2 * t
    panel_z0 = t
    panel_front = front + 0.016
    fascia_z0 = panel_z0 + ph + 0.003
    fascia_h = H - t - fascia_z0
    fascia_front = front + 0.007

    st.box("fundo_frente", (inner_w, 0.004, H - 2 * t), (0, front + 0.03, H / 2), material=mats["fascia"])
    st.box("fachada", (inner_w, 0.006, fascia_h), (0, fascia_front + 0.003, fascia_z0 + fascia_h / 2),
           bevel=0.002, material=mats["fascia"])
    if rec["fachada"].get("friso") is not None or rec["fachada"].get("tipo") is None:
        st.rounded_rect_curve("friso", inner_w - 0.006, fascia_h - 0.006, 0.006, 0.0027,
                              (0, fascia_front - 0.0006, fascia_z0 + fascia_h / 2), mats["piping"])

    # Logo de plástico creme com contorno preto, como as placas em script.
    lg = rec["logo"]
    logo_z = fascia_z0 + fascia_h * lg.get("altura_rel", 0.47)
    logo_x = inner_w * lg.get("lado_rel", 0.0)
    lsize = lg["altura_m"]
    lfont = fonts[lg.get("fonte", "script")]
    if lg.get("tipo") == "marca":
        # A marca do programa (triângulo + NEXUS + MODELER), em relevo.
        brand_plate.build((logo_x, logo_z), lg["altura_m"], fascia_front - 0.0045, fonts, lg.get("modeler", True))
        lg = dict(lg, sublinhado=None)
    elif lg.get("contorno", True):
        st.text("logo_contorno", lg["texto"], lfont, lsize, (logo_x, fascia_front - 0.002, logo_z),
                mats["logo_edge"], extrude=0.0018, offset=0.0016, bevel=0.0004, valign="CENTER")
    if lg.get("tipo") != "marca":
      st.text("logo", lg["texto"], lfont, lsize, (logo_x, fascia_front - 0.0045, logo_z),
              mats["logo"], extrude=0.0016, bevel=0.0007, valign="CENTER")
    # Número grande na grade, no canto de baixo à direita (como o 5150).
    if rec.get("selo_grade"):
        sg = rec["selo_grade"]
        st.text("selo_grade", sg["texto"], fonts["label"], sg.get("altura_m", 0.034),
                (inner_w / 2 - sg.get("margem_m", 0.05), fascia_front - 0.0035, fascia_z0 + sg.get("altura_m", 0.034) * 0.75),
                st.mat_brushed("selo", (0.86, 0.87, 0.9), rough=0.2), extrude=0.0015, bevel=0.0004, align="RIGHT")
        # Sublinhado em traço, como a assinatura das placas em script.
    if lg.get("sublinhado"):
        sub = lg["sublinhado"]
        bar = st.box("logo_sublinhado", (sub["largura_m"], 0.003, sub["espessura_m"]),
                     (logo_x + sub.get("desvio_m", 0.0), fascia_front - 0.0035, logo_z - lsize * sub.get("abaixo", 0.42)),
                     bevel=sub["espessura_m"] * 0.45, material=mats["logo"])
        bar.rotation_euler = (0, math.radians(sub.get("inclinacao", -4.0)), 0)

    # Painel de metal escovado.
    st.box("painel", (inner_w - 0.004, 0.003, ph), (0, panel_front + 0.0015, panel_z0 + ph / 2),
           bevel=0.0008, material=mats["panel"])
    ink_y = panel_front - 0.00012
    L = -inner_w / 2

    # Chave liga/desliga iluminada.
    sw_x, sw_z = L + lay["liga_x"], panel_z0 + 0.036
    st.box("chave_caixa", (0.024, 0.006, 0.032), (sw_x, panel_front - 0.003, sw_z), bevel=0.0015, material=mats["rocker_house"])
    rocker = st.box("chave", (0.019, 0.008, 0.026), (sw_x, panel_front - 0.0062, sw_z), bevel=0.002, material=mats["rocker"])
    rocker.rotation_euler = (math.radians(-9), 0, 0)
    st.label(rec["chave_liga"]["rotulo"], (sw_x, ink_y, panel_z0 + ph - 0.0155), 0.0112, color=text_color)

    # Nome do modelo em letras grandes e inclinadas.
    model = st.text("modelo", rec["painel"]["modelo"], fonts["label"], lay["modelo_tamanho"],
                    (L + lay["modelo_x"], ink_y, panel_z0 + lay.get("modelo_z", 0.036)), mats["ink"], offset=0.0002)
    model.data.shear = lay["modelo_inclinacao"]

    controls = []
    spacing = lay["botoes_passo"]
    first = L + lay["botoes_x0"]
    knob_z = panel_z0 + 0.036
    for i, ctl in enumerate(rec["botoes"]):
        cx = first + i * spacing
        pivot, outer_r = _knob(rec, i, ctl, (cx, knob_z), panel_front, mats)
        _scale(f"escala_{i}", (cx, knob_z), ink_y, mats, fonts["label"], lay["escala_raio"], lay["escala_numero"])
        st.label(ctl["rotulo"], (cx, ink_y, panel_z0 + ph - 0.0155), 0.0112, color=text_color)
        controls.append({"id": ctl["id"], "pivot": pivot, "center": (cx, panel_front - 0.009, knob_z),
                         "outer_r": outer_r, "depth": 0.018})
    # Linha impressa ligando o fim de uma escala ao começo da outra.
    line_z = knob_z - lay["escala_raio"] - 0.005
    st.box("linha_escalas", (spacing * (len(controls) - 1), 0.0001, 0.0006), (first + spacing * (len(controls) - 1) / 2, ink_y, line_z), material=mats["ink"])
    for c in controls:
        st.box(f"linha_v_{c['id']}", (0.0006, 0.0001, 0.0042), (c["center"][0], ink_y, line_z + 0.0021), material=mats["ink"])

    # Equalizador gráfico (deslizantes), se a receita tiver.
    for j, ctl in enumerate(rec.get("deslizantes", [])):
        sx = L + lay["deslizantes_x0"] + j * lay["deslizantes_passo"]
        controls.append(_slider(j, ctl, sx, panel_z0 + ph * 0.52, lay["deslizantes_curso"], panel_front, ink_y, mats,
                                fonts["label"], text_color))

    # Marca e entradas.
    right = L + inner_w
    st.text("marca", rec["painel"]["marca"], fonts["label"], 0.0068, (right - lay["marca_dir"], ink_y, panel_z0 + ph - 0.016), mats["ink"], spacing=1.15)
    for j, lbl in enumerate(rec["entradas"]):
        jx = right - lay["entradas_dir"] + j * lay["entradas_passo"]
        jz = panel_z0 + 0.036
        st.cylinder(f"porca_{j}", 0.0082, 0.0035, (jx, panel_front - 0.0017, jz), vertices=6, material=mats["chrome"], bevel=0.0006)
        st.cylinder(f"bucha_{j}", 0.0058, 0.006, (jx, panel_front - 0.004, jz), material=mats["chrome"], bevel=0.0005)
        st.cylinder(f"furo_{j}", 0.0034, 0.0005, (jx, panel_front - 0.0071, jz), material=mats["hole"])
        st.label(lbl, (jx, ink_y, jz - 0.0195), 0.0085, color=text_color)

    # Parafusos do painel.
    for sx in (L + 0.007, right - 0.007):
        for sz in (panel_z0 + 0.008, panel_z0 + ph - 0.008):
            bpy.ops.mesh.primitive_uv_sphere_add(radius=0.0032, location=(sx, panel_front - 0.0002, sz), segments=32, ring_count=16)
            s = bpy.context.active_object
            s.scale = (1, 0.35, 1)
            s.data.materials.append(mats["chrome"])
            st.smooth(s, 80)

    # Cantoneiras de metal nos quatro cantos da frente e de trás.
    for cx in (-W / 2, W / 2):
        for cz in (0.0, H):
            for cy in (front, -front):
                c = st.box("cantoneira", (0.042, 0.042, 0.042), (cx - math.copysign(0.0155, cx), cy - math.copysign(0.0155, cy), cz - math.copysign(0.0155, cz - H / 2)),
                           bevel=0.012, segments=5, material=mats["chrome"] if rec.get("cantos") == "cromo" else mats["black_metal"])
                c.scale = (1.0, 1.0, 1.0)
                if cy == front:
                    st.cylinder("parafuso_canto", 0.0028, 0.0016, (cx - math.copysign(0.011, cx), front - 0.0068, cz - math.copysign(0.011, cz - H / 2)),
                                material=mats["chrome"], bevel=0.0005)

    # Alça de couro em cima, com as pontas de metal.
    st.strap_arc("alca", 0.25, 0.022, H + 0.007, 0.0058, 2.7, mats["handle"])
    for hx in (-0.14, 0.14):
        st.box("ponta_alca", (0.045, 0.04, 0.008), (hx, 0, H + 0.004), bevel=0.003, material=mats["black_metal"])

    # Amplificador de vários canais: um LED por canal, apagado no modelo (a
    # tela acende o do canal ativo), com o nome do canal acima de cada um.
    channel_leds = []
    channels = rec.get("canais", [])
    if len(channels) > 1:
        led_mat = st.mat_plastic("led_canal", (0.30, 0.012, 0.008), rough=0.06, coat=1.0)
        for i, ch in enumerate(channels):
            lx = L + rec.get("leds_canais_x", 0.075) + i * rec.get("leds_canais_passo", 0.024)
            lz = panel_z0 + rec.get("leds_canais_z", 0.024)
            st.cylinder(f"led_canal_aro_{i}", 0.0042, 0.0016, (lx, panel_front - 0.0008, lz), material=mats["chrome"], bevel=0.0004)
            bpy.ops.mesh.primitive_uv_sphere_add(radius=0.0030, location=(lx, panel_front - 0.0016, lz), segments=32, ring_count=16)
            dome = bpy.context.active_object
            dome.scale = (1, 0.7, 1)
            dome.data.materials.append(led_mat)
            st.smooth(dome, 80)
            st.label(ch["nome"].upper(), (lx, ink_y, lz + 0.0105), 0.0068, color=text_color)
            channel_leds.append(((lx, panel_front - 0.0035, lz), 0.0030))
    return {"controls": controls, "size": (W, D, H), "front": front, "channel_leds": channel_leds}
