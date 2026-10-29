# Nexus Gear Studio: família "caixa" (4x12, 2x12, 1x12, 8x10...). Caixa de
# tolex com a frente rebaixada, tela (pano ou grade de metal), friso, placa
# com o logo e os detalhes do original. Tudo vem da receita; as chaves novas
# são opcionais e as receitas antigas continuam saindo iguais.
#
# Chaves da receita (além de medidas_m, aba_m, couro, tela, friso, logo, selo):
#   couro.tipo        "levant" (padrão) | "tweed" | "carbono" | "carpete"
#                     (cor_b: cor do fio escuro; passo_mm)
#   chanfro_m         raio das quinas da caixa (0.016)
#   aba_topo_m        aba de cima mais larga que as outras (tweed)
#   inclinada         {"recuo_m", "inicio_rel"}: parte de cima da frente
#                     inclinada para trás (4x12 "A"/slant)
#   tela.tipo         "pano" (padrão) | "grade"
#   tela.padrao       pano: "cesto" (padrão, sal e pimenta) | "losango" |
#                     "listrada" | "xadrez"; cor_linha, passo_mm, largura_rel
#   tela.grade        grade: "perfurada" | "losango" (metal expandido) |
#                     "quadrada"; cor, passo_mm, furo_mm/fio_mm,
#                     cones "papel" | "aluminio" (falantes atrás da grade)
#   friso_m, friso_onde ("tela" | "abertura" | "ambos")
#   painel            {"altura_m", "tipo": "tolex"|"chapa", "cor", "botoes": [n, ...]}
#   faixas            [{"onde": "acima_tela"|"abaixo_tela", "altura_m", "cor", "metal"}]
#   cantos            "preto" (padrão) | "cromo" | "nenhum"; cantos_m, cantos_onde
#   tachas            {"cor", "passo_m", "raio_m", "margem_m"}
#   rodizios          {"altura_m"}  ou  pes {"altura_m", "cor"} (contam na altura)
#   logo.placa        {"cor", "metal", "largura_m", "altura_m", "borda"}
#   logo.no_painel    true: a marca fica no meio do painel de cima
#   selo.lado, selo.placa   número ou plaqueta no canto de baixo
import math

import bmesh
import bpy
from mathutils import Matrix

import brand_plate
import studio as st

M = st._math


# ---------------------------------------------------------------- materiais

def _output(nodes):
    for n in nodes:
        if n.type == "OUTPUT_MATERIAL":
            return n
    return nodes.new("ShaderNodeOutputMaterial")


def _clamp01(nodes, links, v):
    return M(nodes, links, "MINIMUM", M(nodes, links, "MAXIMUM", v, 0.0), 1.0)


def mat_tolex(name, couro):
    """Tolex da caixa: levant (padrão), tweed laqueado, fibra de carbono ou carpete."""
    tipo = couro.get("tipo", "levant")
    if tipo == "levant":
        return st.mat_leather(name, couro["cor"], couro.get("grao_mm", 2.4))
    mat, nodes, links, bsdf = st._principled(name)
    coords = st._object_coords(nodes)
    sep = nodes.new("ShaderNodeSeparateXYZ")
    links.new(coords, sep.inputs[0])
    cor_b = couro.get("cor_b", [v * 0.45 for v in couro["cor"]])
    col = nodes.new("ShaderNodeMix")
    col.data_type = "RGBA"
    col.inputs["A"].default_value = (*couro["cor"], 1)
    col.inputs["B"].default_value = (*cor_b, 1)
    if tipo == "tweed":
        # Sarja diagonal: fios grossos em diagonal (x + y + z), o fio escuro
        # marcando cada diagonal, e verniz por cima (tweed laqueado).
        s = 1000.0 / couro.get("passo_mm", 2.6)
        diag = M(nodes, links, "ADD", M(nodes, links, "ADD", sep.outputs["X"], sep.outputs["Y"]), sep.outputs["Z"])
        f = M(nodes, links, "FRACT", M(nodes, links, "MULTIPLY", diag, s))
        stripe = M(nodes, links, "ABSOLUTE", M(nodes, links, "SUBTRACT", f, 0.5))
        band = nodes.new("ShaderNodeMapRange")
        band.inputs["From Min"].default_value = 0.3
        band.inputs["From Max"].default_value = 0.45
        links.new(stripe, band.inputs["Value"])
        noise = nodes.new("ShaderNodeTexNoise")
        noise.inputs["Scale"].default_value = s * 0.2
        noise.inputs["Detail"].default_value = 3.0
        links.new(coords, noise.inputs["Vector"])
        fac = M(nodes, links, "MULTIPLY", band.outputs["Result"],
                M(nodes, links, "ADD", 0.45, M(nodes, links, "MULTIPLY", noise.outputs["Fac"], 0.7)))
        links.new(fac, col.inputs["Factor"])
        bump = nodes.new("ShaderNodeBump")
        bump.inputs["Strength"].default_value = 0.45
        bump.inputs["Distance"].default_value = 0.0004
        links.new(stripe, bump.inputs["Height"])
        links.new(bump.outputs["Normal"], bsdf.inputs["Normal"])
        bsdf.inputs["Roughness"].default_value = 0.45
        bsdf.inputs["Coat Weight"].default_value = 0.6
        bsdf.inputs["Coat Roughness"].default_value = 0.2
    elif tipo == "carbono":
        # Trama de fibra de carbono: xadrez de fios deitados e em pé.
        s = 1000.0 / couro.get("passo_mm", 3.0)
        mp = nodes.new("ShaderNodeMapping")
        mp.inputs["Scale"].default_value = (s, s, s)
        links.new(coords, mp.inputs["Vector"])
        chk = nodes.new("ShaderNodeTexChecker")
        chk.inputs["Scale"].default_value = 1.0
        chk.inputs["Color1"].default_value = (1, 1, 1, 1)
        chk.inputs["Color2"].default_value = (0, 0, 0, 1)
        links.new(mp.outputs["Vector"], chk.inputs["Vector"])
        links.new(chk.outputs["Fac"], col.inputs["Factor"])
        bump = nodes.new("ShaderNodeBump")
        bump.inputs["Strength"].default_value = 0.25
        bump.inputs["Distance"].default_value = 0.0003
        links.new(chk.outputs["Fac"], bump.inputs["Height"])
        links.new(bump.outputs["Normal"], bsdf.inputs["Normal"])
        bsdf.inputs["Roughness"].default_value = 0.38
        bsdf.inputs["Metallic"].default_value = 0.25
        bsdf.inputs["Coat Weight"].default_value = 0.4
    else:  # carpete
        noise = nodes.new("ShaderNodeTexNoise")
        noise.inputs["Scale"].default_value = 2600.0
        noise.inputs["Detail"].default_value = 6.0
        links.new(coords, noise.inputs["Vector"])
        links.new(noise.outputs["Fac"], col.inputs["Factor"])
        bump = nodes.new("ShaderNodeBump")
        bump.inputs["Strength"].default_value = 0.8
        bump.inputs["Distance"].default_value = 0.0006
        links.new(noise.outputs["Fac"], bump.inputs["Height"])
        links.new(bump.outputs["Normal"], bsdf.inputs["Normal"])
        bsdf.inputs["Roughness"].default_value = 0.95
        bsdf.inputs["Sheen Weight"].default_value = 0.6
        bsdf.inputs["Sheen Roughness"].default_value = 0.5
    links.new(col.outputs["Result"], bsdf.inputs["Base Color"])
    return mat


def mat_cloth(name, tela):
    """Pano da tela: a trama de cesto do studio com um desenho por cima
    (losangos Vox, listras Fender, xadrez fino)."""
    padrao = tela.get("padrao", "cesto")
    cor_a, cor_b = tela["cor_a"], tela["cor_b"]
    if padrao == "xadrez":
        mat = st.mat_grille_cloth(name, cor_a, cor_a, tela["trama_mm"])
    else:
        mat = st.mat_grille_cloth(name, cor_a, cor_b, tela["trama_mm"])
    if padrao == "cesto":
        return mat
    nodes, links = mat.node_tree.nodes, mat.node_tree.links
    # Entra antes do escurecimento das bordas.
    scale = next(n for n in nodes if n.type == "VECT_MATH" and n.operation == "SCALE")
    old = scale.inputs[0].links[0].from_socket
    coords = st._object_coords(nodes)
    sep = nodes.new("ShaderNodeSeparateXYZ")
    links.new(coords, sep.inputs[0])
    x, z = sep.outputs["X"], sep.outputs["Z"]
    line = tela.get("cor_linha", cor_b)
    if padrao == "losango":
        a, b = [p / 1000.0 for p in tela.get("passo_mm", (90, 120))]
        w = tela.get("largura_rel", 0.035)
        p = M(nodes, links, "ADD", M(nodes, links, "DIVIDE", x, a), M(nodes, links, "DIVIDE", z, b))
        q = M(nodes, links, "SUBTRACT", M(nodes, links, "DIVIDE", x, a), M(nodes, links, "DIVIDE", z, b))
        dp = M(nodes, links, "ABSOLUTE", M(nodes, links, "SUBTRACT", M(nodes, links, "FRACT", p), 0.5))
        dq = M(nodes, links, "ABSOLUTE", M(nodes, links, "SUBTRACT", M(nodes, links, "FRACT", q), 0.5))
        d = M(nodes, links, "MAXIMUM", dp, dq)  # 0.5 em cima da linha
        fac = _clamp01(nodes, links, M(nodes, links, "DIVIDE", M(nodes, links, "SUBTRACT", d, 0.5 - w), w * 0.5))
    elif padrao == "listrada":
        pz = tela.get("passo_mm", 3.0) / 1000.0
        w = tela.get("largura_rel", 0.3)
        f = M(nodes, links, "FRACT", M(nodes, links, "DIVIDE", z, pz))
        fac = _clamp01(nodes, links, M(nodes, links, "DIVIDE", M(nodes, links, "SUBTRACT", w, f), 0.08))
    else:  # xadrez
        s = 1000.0 / tela.get("passo_mm", 2.0)
        mp = nodes.new("ShaderNodeMapping")
        mp.inputs["Scale"].default_value = (s, s, s)
        links.new(coords, mp.inputs["Vector"])
        chk = nodes.new("ShaderNodeTexChecker")
        chk.inputs["Scale"].default_value = 1.0
        chk.inputs["Color1"].default_value = (1, 1, 1, 1)
        chk.inputs["Color2"].default_value = (0, 0, 0, 1)
        links.new(mp.outputs["Vector"], chk.inputs["Vector"])
        fac = chk.outputs["Fac"]
        line = cor_b
    mix = nodes.new("ShaderNodeMix")
    mix.data_type = "RGBA"
    links.new(fac, mix.inputs["Factor"])
    links.new(old, mix.inputs["A"])
    mix.inputs["B"].default_value = (*line, 1)
    links.new(mix.outputs["Result"], scale.inputs[0])
    return mat


def mat_grade(name, g):
    """Grade de metal pintado com furos de verdade (transparentes): chapa
    perfurada, metal expandido em losango ou malha quadrada soldada."""
    mat, nodes, links, bsdf = st._principled(name)
    coords = st._object_coords(nodes)
    sep = nodes.new("ShaderNodeSeparateXYZ")
    links.new(coords, sep.inputs[0])
    kind = g.get("grade", "perfurada")
    passo = g.get("passo_mm", 5.0)
    if kind == "perfurada":
        pitch = passo / 1000.0
        z = M(nodes, links, "DIVIDE", sep.outputs["Z"], pitch * 0.866)
        row = M(nodes, links, "FLOOR", z)
        shift = M(nodes, links, "MULTIPLY", M(nodes, links, "MODULO", M(nodes, links, "ABSOLUTE", row), 2.0), 0.5)
        x = M(nodes, links, "ADD", M(nodes, links, "DIVIDE", sep.outputs["X"], pitch), shift)
        fx = M(nodes, links, "SUBTRACT", M(nodes, links, "FRACT", x), 0.5)
        fz = M(nodes, links, "MULTIPLY", M(nodes, links, "SUBTRACT", M(nodes, links, "FRACT", z), 0.5), 0.866)
        d = M(nodes, links, "SQRT", M(nodes, links, "ADD", M(nodes, links, "MULTIPLY", fx, fx), M(nodes, links, "MULTIPLY", fz, fz)))
        r = 0.5 * g.get("furo_mm", 3.5) / passo
        solid = _clamp01(nodes, links, M(nodes, links, "DIVIDE", M(nodes, links, "SUBTRACT", d, r), 0.05))
    else:
        if kind == "losango":
            a, b = [p / 1000.0 for p in (passo if isinstance(passo, (list, tuple)) else (passo, passo * 0.5))]
            xs, zs = M(nodes, links, "DIVIDE", sep.outputs["X"], a), M(nodes, links, "DIVIDE", sep.outputs["Z"], b)
            u, v = M(nodes, links, "ADD", xs, zs), M(nodes, links, "SUBTRACT", xs, zs)
            fio = g.get("fio_mm", 5.0) / 1000.0 / b  # fio medido na vertical
        else:  # quadrada
            p = passo / 1000.0
            u, v = M(nodes, links, "DIVIDE", sep.outputs["X"], p), M(nodes, links, "DIVIDE", sep.outputs["Z"], p)
            fio = g.get("fio_mm", 3.0) / 1000.0 / p
        du = M(nodes, links, "ABSOLUTE", M(nodes, links, "SUBTRACT", M(nodes, links, "FRACT", M(nodes, links, "ADD", u, 0.5)), 0.5))
        dv = M(nodes, links, "ABSOLUTE", M(nodes, links, "SUBTRACT", M(nodes, links, "FRACT", M(nodes, links, "ADD", v, 0.5)), 0.5))
        d = M(nodes, links, "MINIMUM", du, dv)  # 0 em cima do fio
        solid = _clamp01(nodes, links, M(nodes, links, "DIVIDE", M(nodes, links, "SUBTRACT", fio * 0.5, d), max(fio * 0.15, 0.004)))
    bsdf.inputs["Base Color"].default_value = (*g.get("cor", (0.02, 0.02, 0.022)), 1)
    bsdf.inputs["Metallic"].default_value = g.get("metalico", 0.55)
    bsdf.inputs["Roughness"].default_value = g.get("aspereza", 0.42)
    bump = nodes.new("ShaderNodeBump")
    bump.inputs["Strength"].default_value = 0.9
    bump.inputs["Distance"].default_value = 0.0015
    links.new(solid, bump.inputs["Height"])
    links.new(bump.outputs["Normal"], bsdf.inputs["Normal"])
    out = _output(nodes)
    clear = nodes.new("ShaderNodeBsdfTransparent")
    mix = nodes.new("ShaderNodeMixShader")
    links.new(solid, mix.inputs["Fac"])
    links.new(clear.outputs[0], mix.inputs[1])
    links.new(bsdf.outputs[0], mix.inputs[2])
    links.new(mix.outputs[0], out.inputs["Surface"])
    return mat


# ---------------------------------------------------------------- geometria

def prism(name, x0, x1, pts, material=None):
    """Sólido reto ao longo de X com o perfil pts [(y, z), ...]."""
    bm = bmesh.new()
    a = [bm.verts.new((x0, y, z)) for y, z in pts]
    b = [bm.verts.new((x1, y, z)) for y, z in pts]
    bm.faces.new(list(reversed(a)))
    bm.faces.new(b)
    n = len(pts)
    for i in range(n):
        j = (i + 1) % n
        bm.faces.new((a[i], a[j], b[j], b[i]))
    bmesh.ops.recalc_face_normals(bm, faces=bm.faces)
    me = bpy.data.meshes.new(name)
    bm.to_mesh(me)
    bm.free()
    obj = bpy.data.objects.new(name, me)
    bpy.context.collection.objects.link(obj)
    if material:
        obj.data.materials.append(material)
    return obj


def lathe(name, profile, location, materials, segs=72):
    """Peça de revolução em volta do eixo Y: profile [(raio, y, material)]."""
    bm = bmesh.new()
    rings = []
    for r, y, _ in profile:
        rr = max(r, 1e-4)
        rings.append([bm.verts.new((rr * math.cos(2 * math.pi * k / segs), y, rr * math.sin(2 * math.pi * k / segs)))
                      for k in range(segs)])
    for i in range(len(rings) - 1):
        for k in range(segs):
            k2 = (k + 1) % segs
            f = bm.faces.new((rings[i][k], rings[i][k2], rings[i + 1][k2], rings[i + 1][k]))
            f.material_index = profile[i + 1][2]
            f.smooth = True
    bmesh.ops.recalc_face_normals(bm, faces=bm.faces)
    me = bpy.data.meshes.new(name)
    bm.to_mesh(me)
    bm.free()
    obj = bpy.data.objects.new(name, me)
    bpy.context.collection.objects.link(obj)
    obj.location = location
    for m in materials:
        obj.data.materials.append(m)
    return obj


def speaker(name, radius, location, mats, aluminio=False):
    """Falante visto pela grade: aro, suspensão, cone e calota."""
    R = radius
    prof = [(R, 0.0, 0), (R * 0.95, 0.0, 0), (R * 0.95, 0.003, 1), (R * 0.915, -0.007, 1), (R * 0.875, 0.002, 1)]
    if aluminio:
        # Cone de alumínio (Hartke): anéis concêntricos marcados.
        steps = 9
        for i in range(1, steps + 1):
            prof.append((R * (0.875 - 0.53 * i / steps), 0.002 + 0.06 * i / steps + (0.003 if i % 2 else 0.0), 2))
    else:
        prof += [(R * 0.6, 0.035, 2), (R * 0.345, 0.062, 2)]
    prof += [(R * 0.34, 0.062, 3), (R * 0.25, 0.05, 3), (R * 0.12, 0.043, 3), (0.0, 0.041, 3)]
    return lathe(name, prof, location, mats)


def bend(obj, hinge_y, zk, theta):
    """Dobra a parte de cima (z > zk) para trás em volta da dobradiça (frente
    inclinada). Peças inteiras acima da dobra só giram."""
    st.select_only(obj)
    if obj.type != "MESH":
        bpy.ops.object.convert(target="MESH")
        obj = bpy.context.active_object
    bpy.ops.object.transform_apply(location=True, rotation=True, scale=True)
    me = obj.data
    zs = [v.co.z for v in me.vertices]
    if zs and max(zs) > zk + 1e-6:
        rot = Matrix.Translation((0, hinge_y, zk)) @ Matrix.Rotation(-theta, 4, "X") @ Matrix.Translation((0, -hinge_y, -zk))
        bm = bmesh.new()
        bm.from_mesh(me)
        if min(zs) < zk - 1e-6:
            bmesh.ops.bisect_plane(bm, geom=bm.verts[:] + bm.edges[:] + bm.faces[:], plane_co=(0, 0, zk), plane_no=(0, 0, 1))
        for v in bm.verts:
            if v.co.z > zk + 1e-6:
                v.co = rot @ v.co
        bm.to_mesh(me)
        bm.free()
        me.update()
    # Origem de volta ao meio da peça: os materiais em coordenadas do
    # objeto (escurecimento da tela, escova radial) contam a partir dela.
    bpy.ops.object.origin_set(type="ORIGIN_GEOMETRY", center="BOUNDS")
    return obj


def build(rec, fonts):
    W = rec["medidas_m"]["largura"]
    H = rec["medidas_m"]["altura"]
    D = rec["medidas_m"]["profundidade"]
    t = rec.get("aba_m", 0.03)
    tt = rec.get("aba_topo_m", t)
    front = -D / 2
    rod = rec.get("rodizios")
    pes = rec.get("pes")
    z0 = rod.get("altura_m", 0.07) if rod else (pes.get("altura_m", 0.012) if pes else 0.0)
    Hb = H - z0
    top = H
    cantos = rec.get("cantos", "preto")
    mats = {
        "tolex": mat_tolex("couro", rec["couro"]),
        "piping": (st.mat_metal("friso", rec["friso"], rough=0.18) if rec.get("friso_metal")
                   else st.mat_plastic("friso", rec.get("friso") or (0.0, 0.0, 0.0), rough=0.32, coat=0.4)),
        "logo": (st.mat_metal("logo", rec["logo"]["cor"], rough=rec["logo"].get("aspereza", 0.12)) if rec["logo"].get("metal")
                 else st.mat_plastic("logo", rec["logo"]["cor"], rough=0.25, coat=0.6)),
        "logo_edge": st.mat_plastic("logo_borda", (0.01, 0.01, 0.01), rough=0.2),
        "black_metal": st.mat_metal("metal_preto", (0.035, 0.035, 0.037), rough=0.32),
        "chrome": st.mat_metal("cromo", (0.92, 0.92, 0.93), rough=0.10),
    }

    # ---- frente inclinada: dobradiça em zk, recuo r no topo. A frente é
    # montada reta até vtop (o comprimento da frente desdobrada) e dobrada no fim.
    sl = rec.get("inclinada")
    if sl:
        r = sl.get("recuo_m", 0.11)
        zk = z0 + Hb * sl.get("inicio_rel", 0.47)
        theta = math.atan2(r, top - zk)
        vtop = zk + math.hypot(r, top - zk)
    else:
        r, zk, theta, vtop = 0.0, top, 0.0, top

    # ---- caixa
    bev = rec.get("chanfro_m", 0.016)
    if sl:
        shell = prism("caixa", -W / 2, W / 2, [(front, z0), (-front, z0), (-front, top), (front + r, top), (front, zk)], mats["tolex"])
        mod = shell.modifiers.new("chanfro", "BEVEL")
        mod.width = bev
        mod.segments = 6
        mod.limit_method = "ANGLE"
        mod.angle_limit = math.radians(25)
        st.apply_modifiers(shell)
    else:
        shell = st.box("caixa", (W, D, Hb), (0, 0, z0 + Hb / 2), bevel=bev, segments=6, material=mats["tolex"])
    oh = vtop - tt - z0 - t
    # Com grade de metal, o rebaixo vai até o fundo dos falantes.
    cut_d = 0.05 if rec["tela"].get("tipo") != "grade" else 0.24
    cutter = st.box("corte_frente", (W - 2 * t, cut_d, oh), (0, front, z0 + t + oh / 2), bevel=0.006)
    if sl:
        cutter = bend(cutter, front, zk, theta)
    st.cut(shell, cutter)
    st.smooth(shell, 35)

    before = set(bpy.data.objects)

    # ---- divisão da abertura, de baixo para cima: faixas de baixo, tela,
    # faixas de cima, painel.
    inner_w = W - 2 * t
    zb, zt = z0 + t, vtop - tt
    cloth_front = front + 0.012
    painel = rec.get("painel")
    ph = 0.0
    if painel:
        ph = painel.get("altura_m") or painel.get("altura_rel", 0.33) * (zt - zb)
    faixas = rec.get("faixas", [])
    fa = sum(f.get("altura_m", 0.012) for f in faixas if f.get("onde") == "acima_tela")
    fb = sum(f.get("altura_m", 0.012) for f in faixas if f.get("onde") != "acima_tela")
    cz0, cz1 = zb + fb, zt - ph - fa
    ch = cz1 - cz0
    ccz = (cz0 + cz1) / 2

    tela = rec["tela"]
    if tela.get("tipo") == "grade":
        st.box("tela", (inner_w, 0.003, ch), (0, cloth_front + 0.0015, ccz), material=mat_grade("grade", tela))
        # Fundo preto com os falantes atrás da grade.
        baffle_y = cloth_front + 0.05
        baffle = st.box("fundo", (inner_w, 0.01, ch), (0, baffle_y + 0.005, ccz),
                        material=st.mat_plastic("fundo", (0.006, 0.006, 0.006), rough=0.8))
        f = rec.get("falantes", {"linhas": 2, "colunas": 2, "polegadas": 12})
        rad = f["polegadas"] * 0.0254 / 2 * 0.97
        # Furos no fundo para o cone aparecer (o falante entra para trás).
        for i in range(f["linhas"]):
            for j in range(f["colunas"]):
                st.cut(baffle, st.cylinder("furo", rad * 0.94, 0.05,
                                           (-inner_w / 2 + inner_w * (j + 0.5) / f["colunas"], baffle_y, cz1 - ch * (i + 0.5) / f["linhas"])))
        alu = tela.get("cones") == "aluminio"
        smats = [st.mat_metal("aro_falante", (0.12, 0.12, 0.125), rough=0.35),
                 st.mat_plastic("suspensao", (0.02, 0.02, 0.021), rough=0.5, coat=0.2),
                 (st.mat_plastic("cone_aluminio", (0.62, 0.63, 0.65), rough=0.28, coat=0.6) if alu
                  else st.mat_plastic("cone", (0.03, 0.03, 0.03), rough=0.7, coat=0.15)),
                 (st.mat_brushed("calota", (0.7, 0.71, 0.72), rough=0.25, radial_axis="Y") if alu
                  else st.mat_plastic("calota", (0.012, 0.012, 0.012), rough=0.5, coat=0.4))]
        for i in range(f["linhas"]):
            for j in range(f["colunas"]):
                sx = -inner_w / 2 + inner_w * (j + 0.5) / f["colunas"]
                sz = cz1 - ch * (i + 0.5) / f["linhas"]
                speaker(f"falante_{i}_{j}", rad, (sx, baffle_y - 0.004, sz), smats, alu)
        cloth_tone = tela.get("cor", (0.02, 0.02, 0.02))
    else:
        cloth = st.box("tela", (inner_w, 0.004, ch), (0, cloth_front + 0.002, ccz), bevel=0.003, material=mat_cloth("tela", tela))
        # A tela estufa um pouco no meio, como tecido esticado num quadro.
        sub = cloth.modifiers.new("sub", "SUBSURF")
        sub.levels = 3
        sub.subdivision_type = "SIMPLE"
        st.apply_modifiers(cloth)
        for v in cloth.data.vertices:
            if v.co.y < 0:
                nx = v.co.x / (inner_w / 2)
                nz = v.co.z / (ch / 2)
                v.co.y -= 0.002 * max(0.0, 1 - nx * nx) * max(0.0, 1 - nz * nz)
        cloth_tone = [(a + b) / 2 for a, b in zip(tela["cor_a"], tela["cor_b"])]

    # ---- painel de cima (tolex ou chapa de controles)
    py = front
    if painel:
        pz = zt - ph / 2
        if painel.get("tipo") == "chapa":
            pm = st.mat_plastic("painel", painel.get("cor", (0.01, 0.01, 0.01)), rough=painel.get("aspereza", 0.45), coat=0.2)
            py = front + 0.008
        else:
            pm = mat_tolex("painel", dict(rec["couro"], cor=painel["cor"])) if painel.get("cor") else mats["tolex"]
            py = front + 0.0015
        st.box("painel", (inner_w, 0.03, ph), (0, py + 0.015, pz), bevel=0.002, material=pm)
        nb = painel.get("botoes")
        if nb:
            groups = nb if isinstance(nb, list) else [nb]
            total = sum(groups) + (len(groups) - 1) * 0.8
            span = inner_w * painel.get("botoes_largura_rel", 0.78)
            step = span / max(total - 1, 1)
            kr = painel.get("botao_raio_m", 0.011)
            kmat = st.mat_plastic("botao", painel.get("cor_botao", (0.01, 0.01, 0.01)), rough=0.3, coat=0.5)
            cap = st.mat_brushed("botao_tampa", painel.get("cor_tampa", (0.8, 0.8, 0.82)), rough=0.2, radial_axis="Y")
            kx = (inner_w / 2 - 0.03 - span if painel.get("botoes_direita") else -span / 2) + painel.get("botoes_desvio_m", 0.0)
            kz = pz + painel.get("botoes_altura_m", 0.0)
            pos = 0.0
            for n in groups:
                for _ in range(n):
                    x = kx + pos * step
                    st.cylinder("botao_saia", kr * 1.3, 0.004, (x, py - 0.002, kz), material=kmat, bevel=0.001)
                    st.cylinder("botao", kr, 0.016, (x, py - 0.01, kz), material=kmat, bevel=0.002)
                    st.cylinder("botao_tampa", kr * 0.55, 0.002, (x, py - 0.0185, kz), material=cap)
                    pos += 1
                pos += 0.8

    # ---- faixas de metal ou douradas em cima e embaixo da tela
    za, zbb = cz1, cz0
    for fx in faixas:
        hh = fx.get("altura_m", 0.012)
        if fx.get("onde") == "acima_tela":
            zc, za = za + hh / 2, za + hh
        else:
            zc, zbb = zbb - hh / 2, zbb - hh
        fm = (st.mat_brushed("faixa", fx["cor"], rough=fx.get("aspereza", 0.25)) if fx.get("metal", True)
              else st.mat_plastic("faixa", fx["cor"], rough=0.3, coat=0.5))
        st.box("faixa", (inner_w, 0.03, hh), (0, front + fx.get("recuo_m", 0.003) + 0.015, zc),
               bevel=min(0.002, hh * 0.3), material=fm)

    # ---- friso
    fw = rec.get("friso_m", 0.003)
    if rec.get("friso"):
        onde = rec.get("friso_onde", "abertura" if painel else "tela")
        if onde in ("tela", "ambos"):
            st.rounded_rect_curve("friso", inner_w - 0.004, ch - 0.004, 0.008, fw, (0, cloth_front - 0.001, ccz), mats["piping"])
        if onde == "ambos" and painel:
            st.rounded_rect_curve("friso_painel", inner_w - 0.004, ph - 0.004, 0.008, fw, (0, py - 0.001, zt - ph / 2), mats["piping"])
        if onde == "abertura":
            st.rounded_rect_curve("friso", inner_w - 0.004, zt - zb - 0.004, 0.008, fw, (0, front + 0.001, (zt + zb) / 2), mats["piping"])

    # ---- logo
    lg = rec["logo"]
    if painel and lg.get("no_painel"):
        lz = zt - ph / 2 + lg.get("desvio_m", 0.0)
        ly = py - 0.004
        tone = painel.get("cor", rec["couro"]["cor"])
    else:
        lz = ccz + ch * lg.get("altura_rel", 0.36)
        ly = cloth_front - 0.006
        tone = cloth_tone
    lx = inner_w * lg.get("lado_rel", 0.0)
    lfont = fonts[lg.get("fonte", "script")]
    placa = lg.get("placa")
    if lg.get("tipo") == "marca":
        if placa:
            pw = placa.get("largura_m", lg["altura_m"] * 6.6)
            phh = placa.get("altura_m", lg["altura_m"] * 2.3)
            pmat = (st.mat_brushed("placa", placa["cor"], rough=placa.get("aspereza", 0.25)) if placa.get("metal")
                    else st.mat_plastic("placa", placa["cor"], rough=0.3, coat=0.6))
            st.box("placa", (pw, 0.004, phh), (lx, ly + 0.003, lz), bevel=0.0015, material=pmat)
            if placa.get("borda"):
                st.rounded_rect_curve("placa_borda", pw - 0.008, phh - 0.008, 0.003, 0.0012, (lx, ly + 0.0008, lz),
                                      st.mat_metal("placa_borda", placa["borda"], rough=0.2))
            tone = placa["cor"]
            ly -= 0.0005
        brand_plate.build((lx, lz), lg["altura_m"], ly, fonts, lg.get("modeler", True), satin=True,
                          dark=lg.get("escuro", brand_plate.is_light(tone)))
        lg = dict(lg, sublinhado=None, contorno=False, texto="")
    if lg.get("contorno", True):
        st.text("logo_contorno", lg["texto"], lfont, lg["altura_m"], (lx, cloth_front - 0.003, lz),
                mats["logo_edge"], extrude=0.0022, offset=0.002, bevel=0.0005)
    if lg["texto"]:
        st.text("logo", lg["texto"], lfont, lg["altura_m"], (lx, cloth_front - 0.0065, lz),
                mats["logo"], extrude=0.002, bevel=0.0008)
    # Número ou plaqueta no canto de baixo (5150, 1960).
    if rec.get("selo"):
        sg = rec["selo"]
        sh = sg.get("altura_m", 0.05)
        right = sg.get("lado", "direita") == "direita"
        sgn = 1 if right else -1
        sx = (inner_w / 2 - sg.get("margem_m", 0.06)) * sgn
        sz = cz0 + sg.get("acima_m", sh * 0.9)
        if sg.get("placa"):
            pl = sg["placa"]
            pcx = sx - pl["largura_m"] / 2 * sgn
            st.box("selo_placa", (pl["largura_m"], 0.003, pl["altura_m"]), (pcx, cloth_front - 0.0025, sz), bevel=0.001,
                   material=st.mat_brushed("selo_placa", pl["cor"], rough=0.22))
            st.text("selo", sg["texto"], fonts["label"], sh, (pcx, cloth_front - 0.0042, sz),
                    st.mat_plastic("selo_letra", sg.get("cor", (0.02, 0.02, 0.02)), rough=0.3), extrude=0.0004)
        else:
            st.text("selo", sg["texto"], fonts["label"], sh, (sx, cloth_front - 0.004, sz),
                    st.mat_brushed("selo", sg.get("cor", (0.86, 0.87, 0.9)), rough=0.2), extrude=0.002, bevel=0.0005,
                    align="RIGHT" if right else "LEFT")
    if lg.get("sublinhado"):
        sub = lg["sublinhado"]
        bar = st.box("logo_sublinhado", (sub["largura_m"], 0.003, sub["espessura_m"]),
                     (lx + sub.get("desvio_m", 0.0), cloth_front - 0.0055, lz - lg["altura_m"] * sub.get("abaixo", 0.42)),
                     bevel=sub["espessura_m"] * 0.45, material=mats["logo"])
        bar.rotation_euler = (0, math.radians(sub.get("inclinacao", -4.0)), 0)

    # ---- tachas em volta da frente
    tc = rec.get("tachas")
    if tc:
        tmat = st.mat_metal("tacha", tc.get("cor", (0.85, 0.65, 0.3)), rough=0.2)
        tr = tc.get("raio_m", 0.0045)
        step = tc.get("passo_m", 0.06)
        m = tc.get("margem_m", t / 2)
        mx = W / 2 - m
        pts = []
        nx = max(1, round(2 * mx / step))
        for k in range(nx + 1):
            x = -mx + 2 * mx * k / nx
            pts += [(x, z0 + m), (x, vtop - m)]
        nz = max(1, round((vtop - z0 - 2 * m) / step))
        for k in range(1, nz):
            z = z0 + m + (vtop - z0 - 2 * m) * k / nz
            pts += [(-mx, z), (mx, z)]
        for x, z in pts:
            bpy.ops.mesh.primitive_uv_sphere_add(radius=tr, segments=16, ring_count=8, location=(x, front, z))
            o = bpy.context.active_object
            o.name = "tacha"
            o.scale = (1, 0.45, 1)
            o.data.materials.append(tmat)
            bpy.ops.object.shade_smooth()

    if sl:
        for o in list(set(bpy.data.objects) - before):
            if o.type in ("MESH", "CURVE", "FONT"):
                bend(o, front, zk, theta)

    # ---- cantoneiras
    if cantos != "nenhum":
        cs = rec.get("cantos_m", 0.05)
        onde = rec.get("cantos_onde", "todos")
        cmat = mats["chrome"] if cantos == "cromo" else mats["black_metal"]
        o = cs * 0.36
        zmid = (z0 + top) / 2
        for cx in (-W / 2, W / 2):
            for cz in (z0, top):
                if (onde == "cima" and cz == z0) or (onde == "baixo" and cz == top):
                    continue
                for cy in (front, -front):
                    yy = cy + (r if (cz == top and cy == front) else 0.0)
                    st.box("cantoneira", (cs, cs, cs),
                           (cx - math.copysign(o, cx), yy - math.copysign(o, cy), cz - math.copysign(o, cz - zmid)),
                           bevel=cs * 0.28, segments=5, material=cmat)
                    if cy == front:
                        st.cylinder("parafuso_canto", cs * 0.064, 0.0018,
                                    (cx - math.copysign(cs * 0.26, cx), yy - 0.0078 * cs / 0.05, cz - math.copysign(cs * 0.26, cz - zmid)),
                                    material=mats["chrome"], bevel=0.0005)

    # ---- rodízios ou pés
    if rod:
        wr = min(0.035, z0 * 0.42)
        steel = st.mat_metal("rodizio", (0.75, 0.75, 0.77), rough=0.2)
        wheel = st.mat_plastic("roda", (0.02, 0.02, 0.02), rough=0.55)
        for sx in (-1, 1):
            for sy in (-1, 1):
                x, y = sx * (W / 2 - 0.07), sy * (D / 2 - 0.07)
                st.box("rodizio_placa", (0.07, 0.07, 0.004), (x, y, z0 - 0.002), bevel=0.001, material=steel)
                st.cylinder("rodizio_eixo", 0.012, z0 - wr * 1.6, (x, y, z0 - (z0 - wr * 1.6) / 2), axis="Z", material=steel)
                st.box("rodizio_garfo", (0.05, 0.012, wr * 1.3), (x, y - wr * 0.35, wr * 1.25), bevel=0.003, material=steel)
                st.cylinder("rodizio_roda", wr, 0.025, (x, y - wr * 0.5, wr), axis="X", material=wheel, bevel=0.004)
    elif pes:
        pm = st.mat_plastic("pe", pes.get("cor", (0.012, 0.012, 0.013)), rough=0.6)
        for sx in (-1, 1):
            for sy in (-1, 1):
                st.cylinder("pe", 0.022, z0, (sx * (W / 2 - 0.05), sy * (D / 2 - 0.05), z0 / 2), axis="Z", material=pm, bevel=0.002)

    return {"controls": [], "size": (W, D, H), "front": front}
