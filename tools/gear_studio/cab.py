# Nexus Gear Studio: família "caixa" (4x12, 2x12, 1x12). Frente com aba de
# couro, tela de tecido trançado, friso branco e placa com o logo.
import math

import bpy

import studio as st


def build(rec, fonts):
    W = rec["medidas_m"]["largura"]
    H = rec["medidas_m"]["altura"]
    D = rec["medidas_m"]["profundidade"]
    t = rec.get("aba_m", 0.03)
    front = -D / 2
    mats = {
        "tolex": st.mat_leather("couro", rec["couro"]["cor"], rec["couro"]["grao_mm"]),
        "cloth": st.mat_grille_cloth("tela", rec["tela"]["cor_a"], rec["tela"]["cor_b"], rec["tela"]["trama_mm"]),
        "piping": st.mat_plastic("friso", rec.get("friso") or (0.0, 0.0, 0.0), rough=0.32, coat=0.4),
        "logo": (st.mat_metal("logo", rec["logo"]["cor"], rough=rec["logo"].get("aspereza", 0.12)) if rec["logo"].get("metal")
                 else st.mat_plastic("logo", rec["logo"]["cor"], rough=0.25, coat=0.6)),
        "logo_edge": st.mat_plastic("logo_borda", (0.01, 0.01, 0.01), rough=0.2),
        "black_metal": st.mat_metal("metal_preto", (0.035, 0.035, 0.037), rough=0.32),
        "chrome": st.mat_metal("cromo", (0.92, 0.92, 0.93), rough=0.10),
    }

    shell = st.box("caixa", (W, D, H), (0, 0, H / 2), bevel=0.016, segments=6, material=mats["tolex"])
    cutter = st.box("corte_frente", (W - 2 * t, 0.05, H - 2 * t), (0, front, H / 2), bevel=0.006)
    st.cut(shell, cutter)
    st.smooth(shell, 35)

    inner_w, inner_h = W - 2 * t, H - 2 * t
    cloth_front = front + 0.012
    cloth = st.box("tela", (inner_w, 0.004, inner_h), (0, cloth_front + 0.002, H / 2), bevel=0.003, material=mats["cloth"])
    # A tela estufa um pouco no meio, como tecido esticado num quadro.
    sub = cloth.modifiers.new("sub", "SUBSURF")
    sub.levels = 3
    sub.subdivision_type = "SIMPLE"
    st.apply_modifiers(cloth)
    for v in cloth.data.vertices:
        if v.co.y < 0:
            nx = v.co.x / (inner_w / 2)
            nz = (v.co.z) / (inner_h / 2)
            v.co.y -= 0.002 * max(0.0, 1 - nx * nx) * max(0.0, 1 - nz * nz)
    # Friso em volta da tela (as caixas americanas de alto ganho não têm: "friso": null).
    if rec.get("friso"):
        st.rounded_rect_curve("friso", inner_w - 0.004, inner_h - 0.004, 0.008, 0.003,
                              (0, cloth_front - 0.001, H / 2), mats["piping"])

    lg = rec["logo"]
    lz = H / 2 + inner_h * lg.get("altura_rel", 0.36)
    lx = inner_w * lg.get("lado_rel", 0.0)
    lfont = fonts[lg.get("fonte", "script")]
    if lg.get("contorno", True):
        st.text("logo_contorno", lg["texto"], lfont, lg["altura_m"], (lx, cloth_front - 0.003, lz),
                mats["logo_edge"], extrude=0.0022, offset=0.002, bevel=0.0005)
    st.text("logo", lg["texto"], lfont, lg["altura_m"], (lx, cloth_front - 0.0065, lz),
            mats["logo"], extrude=0.002, bevel=0.0008)
    # Sublinhado em traço, igual ao do cabeçote da mesma família.
    if lg.get("sublinhado"):
        sub = lg["sublinhado"]
        bar = st.box("logo_sublinhado", (sub["largura_m"], 0.003, sub["espessura_m"]),
                     (lx + sub.get("desvio_m", 0.0), cloth_front - 0.0055, lz - lg["altura_m"] * sub.get("abaixo", 0.42)),
                     bevel=sub["espessura_m"] * 0.45, material=mats["logo"])
        bar.rotation_euler = (0, math.radians(sub.get("inclinacao", -4.0)), 0)

    for cx in (-W / 2, W / 2):
        for cz in (0.0, H):
            for cy in (front, -front):
                st.box("cantoneira", (0.05, 0.05, 0.05),
                       (cx - math.copysign(0.018, cx), cy - math.copysign(0.018, cy), cz - math.copysign(0.018, cz - H / 2)),
                       bevel=0.014, segments=5, material=mats["black_metal"])
                if cy == front:
                    st.cylinder("parafuso_canto", 0.0032, 0.0018,
                                (cx - math.copysign(0.013, cx), front - 0.0078, cz - math.copysign(0.013, cz - H / 2)),
                                material=mats["chrome"], bevel=0.0005)
    return {"controls": [], "size": (W, D, H), "front": front}
