#!/usr/bin/env python3
# Nexus Gear Studio: escreve as receitas fiéis dos cabeçotes (formato novo,
# com "fileiras") a partir das fichas em fichas_cabecotes/<lote>.py.
#
#   python3 tools/gear_studio/cabecotes_fiel.py [id ...]
#
# Cada ficha diz o formato, as medidas, as cores e o painel do original. A
# receita antiga dá o nome, a família, os canais e o logo; os botões que
# tocam (ganho, grave, médio, agudo, presença, volume e os deslizantes) têm
# de continuar todos na receita nova, com o mesmo id: o script confere.
import glob
import importlib.util
import json
import math
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
RECIPES = os.path.join(HERE, "recipes")
FICHAS = os.path.join(HERE, "fichas_cabecotes")

# Rótulos que a tela já traduz (tabela de idiomas).
STD = {"ganho": "GANHO", "grave": "GRAVE", "medio": "MÉDIO", "agudo": "AGUDO",
       "presenca": "PRESENÇA", "volume": "VOLUME"}


# ---------------------------------------------------------------- peças

def K(id, rotulo=None, **kw):
    """Botão que toca (id do mapa)."""
    return dict(tipo="botao", id=id, rotulo=STD.get(id) if rotulo is None else rotulo, **kw)


def F(rotulo, **kw):
    """Botão do original que não toca: desenhado fixo."""
    return dict(tipo="botao", fixo=True, rotulo=rotulo, **kw)


def S(id, rotulo, **kw):
    return dict(tipo="deslizante", id=id, rotulo=rotulo, **kw)


def SF(rotulo, **kw):
    return dict(tipo="deslizante", fixo=True, rotulo=rotulo, **kw)


def J(rotulo=None, **kw):
    return dict(tipo="entrada", rotulo=rotulo, **kw)


def R(rotulo=None, cor=(0.85, 0.015, 0.008), aceso=True, **kw):
    return dict(tipo="basculante", rotulo=rotulo, cor=list(cor), aceso=aceso, **kw)


def RB(rotulo=None, **kw):
    """Basculante preta, apagada."""
    return R(rotulo, cor=(0.012, 0.012, 0.012), aceso=False, **kw)


def T(rotulo=None, **kw):
    return dict(tipo="alavanca", rotulo=rotulo, **kw)


def MT(rotulo=None, **kw):
    return dict(tipo="minichave", rotulo=rotulo, **kw)


def P(rotulo=None, cor=(0.02, 0.02, 0.02), aceso=False, **kw):
    return dict(tipo="push", rotulo=rotulo, cor=list(cor), aceso=aceso, **kw)


def L(rotulo=None, cor=(0.9, 0.05, 0.02), aceso=False, **kw):
    return dict(tipo="led", rotulo=rotulo, cor=list(cor), aceso=aceso, **kw)


def LC(rotulo=None, **kw):
    """LED de canal: a tela acende o do canal ativo."""
    d = dict(tipo="led_canal", **kw)
    if rotulo is not None:
        d["rotulo"] = rotulo
    return d


def JW(cor=(0.9, 0.06, 0.02), rotulo=None, **kw):
    return dict(tipo="joia", cor=list(cor), rotulo=rotulo, **kw)


def VU(rotulo=None, **kw):
    return dict(tipo="vu", rotulo=rotulo, **kw)


def M(texto=None, tamanho_m=0.022, **kw):
    d = dict(tipo="modelo", tamanho_m=tamanho_m, **kw)
    if texto:
        d["texto"] = texto
    return d


def MARCA(**kw):
    return dict(tipo="marca", **kw)


def LOGO(altura_m=0.012, **kw):
    return dict(tipo="logo", altura_m=altura_m, **kw)


def TX(rotulo, **kw):
    return dict(tipo="texto", rotulo=rotulo, **kw)


def GAP(peso=1.0):
    return dict(tipo="espaco", peso=peso)


def ROW(z, x0, x1, *itens, **kw):
    return dict(z=z, x0=x0, x1=x1, itens=list(itens), **kw)


def RISCO(x0, z0, x1, z1, esp_m=0.0006, cor=None):
    d = dict(x0=x0, z0=z0, x1=x1, z1=z1, esp_m=esp_m)
    if cor is not None:
        d["cor"] = list(cor)
    return d


# ---------------------------------------------------------------- cores e estilos prontos

def rgb(*c):
    return [round(v, 4) for v in c]


PRETO = rgb(0.006, 0.006, 0.006)


def estilo(forma, d, corpo, tampa=None, **kw):
    e = dict(forma=forma, diametro_m=d, corpo=list(corpo), **kw)
    if tampa is not None:
        e["tampa"] = list(tampa)
    for k in ("traco", "saia", "numeros", "anel", "cursor"):
        if k in e:
            e[k] = list(e[k])
    return e


def tela(a, b, trama=2.0, friso=None):
    d = dict(tipo="tela", cor_a=list(a), cor_b=list(b), trama_mm=trama)
    if friso is not None:
        d["friso"] = list(friso)
    return d


# ---------------------------------------------------------------- montagem

def quadro(fmt, W, H, D, painel):
    """Enquadramento: pela largura, com folga para a alça e a sombra."""
    top = 0.04 if fmt in ("cabecote", "combo") else (0.035 if fmt == "caixinha" else 0.012)
    prop = (H + top + 0.03) * 1.05 / (W * 1.10)
    elev = 10.0
    if painel.get("posicao") == "topo":
        # Painel no tampo (Vox, tweed): câmera mais alta para os botões aparecerem.
        elev = 34.0
        e = math.radians(elev)
        prop = (H * math.cos(e) + D * math.sin(e) + 0.06) * 1.04 / (W * 1.10)
    return {"largura_rel": 1.10, "proporcao": round(max(0.16, prop), 3), "elevacao": elev}


def make(old, ficha):
    rec = {k: old[k] for k in ("tipo", "nome", "familia", "canais") if k in old}
    for key in ("transformacao", "aparencia"):
        if key in old:
            rec[key] = old[key]
    rec["inspiracao"] = ficha["inspiracao"]
    rec["formato"] = ficha.get("formato", "cabecote")
    W, H, D = ficha["mm"]
    rec["medidas_m"] = {"largura": W / 1000, "altura": H / 1000, "profundidade": D / 1000}
    if ficha.get("rack_u"):
        rec["rack_u"] = ficha["rack_u"]
    rec["couro"] = ficha["couro"]
    if ficha.get("fachada") is not None:
        rec["fachada"] = ficha["fachada"]
    logo = dict(old.get("logo", {}))
    logo.update(ficha.get("logo", {}))
    rec["logo"] = logo
    painel = {"marca": "NEXUS MODELER", "modelo": old["painel"].get("modelo", old["nome"].upper())}
    painel.update(ficha["painel"])
    rec["painel"] = painel
    rec["estilos"] = ficha["estilos"]
    rec["fileiras"] = ficha["fileiras"]
    for key in ("riscos", "cantos", "alca", "aba_m", "recuo_frente_m", "chanfro_m", "selo_grade", "parafusos_painel"):
        if key in ficha:
            rec[key] = ficha[key]
    if "selo_grade" in old and "selo_grade" not in ficha and ficha.get("manter_selo"):
        rec["selo_grade"] = old["selo_grade"]
    rec["quadro"] = ficha.get("quadro") or quadro(rec["formato"], W / 1000, H / 1000, D / 1000, painel)
    if ficha.get("notas"):
        rec["notas_modelagem"] = ficha["notas"]
    check(old, rec)
    return rec


def check(old, rec):
    want = {b["id"] for b in old.get("botoes", [])} | {b["id"] for b in old.get("deslizantes", [])}
    if not want:  # receita já convertida: os ids que ela tem
        want = {it["id"] for r in old.get("fileiras", []) for it in r["itens"] if it.get("id") and not it.get("fixo")}
    got = [it["id"] for r in rec["fileiras"] for it in r["itens"] if it.get("id") and not it.get("fixo")]
    if set(got) != want or len(got) != len(set(got)):
        raise SystemExit(f"{rec['nome']}: botões que tocam {sorted(got)} != {sorted(want)}")
    leds = sum(1 for r in rec["fileiras"] for it in r["itens"] if it.get("tipo") == "led_canal")
    n = len(rec.get("canais", []))
    if n > 1 and leds != n:
        raise SystemExit(f"{rec['nome']}: {leds} LEDs de canal para {n} canais")
    for r in rec["fileiras"]:
        for it in r["itens"]:
            st = it.get("estilo", r.get("estilo"))
            if it.get("tipo", "botao") == "botao" and (st or "padrao") not in rec["estilos"]:
                raise SystemExit(f"{rec['nome']}: estilo {st} não existe")


def load_fichas():
    fichas = {}
    for path in sorted(glob.glob(os.path.join(FICHAS, "*.py"))):
        spec = importlib.util.spec_from_file_location(os.path.basename(path)[:-3], path)
        mod = importlib.util.module_from_spec(spec)
        sys.modules["cabecotes_fiel"] = sys.modules[__name__]
        spec.loader.exec_module(mod)
        fichas.update(mod.FICHAS)
    return fichas


def main():
    fichas = load_fichas()
    ids = sys.argv[1:] or sorted(fichas)
    for rid in ids:
        path = os.path.join(RECIPES, f"{rid}.json")
        old = json.load(open(path, encoding="utf-8"))
        rec = make(old, fichas[rid])
        with open(path, "w", encoding="utf-8") as f:
            json.dump(rec, f, ensure_ascii=False, indent=2)
            f.write("\n")
        print("ok", rid)


if __name__ == "__main__":
    main()
