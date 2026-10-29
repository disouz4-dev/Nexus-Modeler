#!/usr/bin/env python3
"""Monta a galeria de equipamentos do site (docs/site/index.html) a partir do
que o programa tem: as fichas dos equipamentos (assets/gear/<id>_manifesto.json),
os nomes dos mapas em assets/gear e a arte de frente de cada peça.

Gera a miniatura de cada peça em docs/site/img/catalogo/<peça>.webp e a foto
grande, aberta ao clicar, em docs/site/img/catalogo/grande/<peça>.webp, e troca
o trecho entre <!-- catalogo:inicio --> e <!-- catalogo:fim --> do index.html.
Rodar de novo sempre que entrar equipamento novo.

Uso (com Pillow): python3 tools/site/catalogo.py [--fonte DIR] [raiz do projeto]
--fonte lê as peças de outra pasta do projeto (por exemplo, o worktree do ramo
que já juntou as peças novas); o site gerado continua sendo o da raiz.
--arte DIR (pode repetir) procura ali a arte que a fonte ainda não tem.
"""
import argparse
import html
import json
import os
import re
from multiprocessing import Pool

from PIL import Image

_args = argparse.ArgumentParser()
_args.add_argument("raiz", nargs="?", default=os.path.join(os.path.dirname(__file__), "..", ".."))
_args.add_argument("--fonte")
_args.add_argument("--arte", action="append", default=[])
_args = _args.parse_args()
RAIZ = os.path.abspath(_args.raiz)
FONTE = os.path.abspath(_args.fonte or RAIZ)
# Onde procurar a arte de cada peça: a fonte e, depois, as pastas de --arte
# (peças desenhadas em outro worktree que ainda não entraram no ramo).
GEARS = [os.path.join(d, "assets", "gear") for d in [FONTE] + [os.path.abspath(a) for a in _args.arte]]
SITE = os.path.join(RAIZ, "docs", "site")
SAIDA = os.path.join(SITE, "img", "catalogo")

# Categorias em inglês (o site tem as duas línguas).
CATEGORIA_EN = {
    "Britânicos": "British", "Americanos de alto ganho": "American high gain", "Americanos boutique": "American boutique",
    "Americanos clássicos": "American classics", "Alemães de alto ganho": "German high gain",
    "Limpos de transistor": "Solid-state cleans", "Baixo": "Bass", "Drive e distorção": "Drive and distortion",
    "Modulação": "Modulation", "Delay": "Delay", "Reverb": "Reverb", "Utilidades": "Utilities", "Afinação": "Pitch",
    "Caixas": "Cabinets",
}

# Largura da foto grande, a que abre na galeria maximizada.
GRANDE = {"cat-cabecote": 1800, "cat-caixa": 1100, "cat-pedal": 900}


def lista(codigo, nome):
    """Conteúdo entre chaves de uma tabela C++ (const ... nome[...] = { ... };)."""
    m = re.search(re.escape(nome) + r"\[[^\]]*\]\s*=\s*\{(.*?)\};", codigo, re.S)
    return m.group(1)


def textos(trecho):
    return re.findall(r'"((?:[^"\\]|\\.)*)"', trecho)


def arquivo(nome):
    """Caminho do arquivo de arte na primeira pasta que o tiver."""
    for gear in GEARS:
        if os.path.exists(os.path.join(gear, nome)):
            return os.path.join(gear, nome)
    return os.path.join(GEARS[0], nome)


def nome_do_mapa(prefixo):
    caminho = arquivo(prefixo + "_mapa.json")
    return json.load(open(caminho, encoding="utf-8")).get("nome", prefixo) if os.path.exists(caminho) else prefixo


def frente(prefixo):
    """Frente da peça (JPEG + máscara, ou PNG) com transparência, recortada."""
    jpg, alfa, png = (arquivo(prefixo + s) for s in ("_frente.jpg", "_frente_alfa.png", "_frente.png"))
    if os.path.exists(jpg) and os.path.exists(alfa):
        # A cor vem multiplicada pela transparência: só muda a borda
        # semitransparente (um pouco mais escura), invisível no fundo escuro.
        cor = Image.open(jpg).convert("RGB")
        im = Image.merge("RGBA", (*cor.split(), Image.open(alfa).convert("L")))
    elif os.path.exists(png):
        im = Image.open(png).convert("RGBA")
    else:
        return None
    return im.crop(im.getbbox())


def salvar(im, largura, destino):
    largura = min(largura, im.width)
    altura = round(im.height * largura / im.width)
    os.makedirs(os.path.dirname(destino), exist_ok=True)
    im.resize((largura, altura), Image.LANCZOS).save(destino, "WEBP", quality=82, method=6)
    return largura, altura


def imagens(prefixo, largura, classe):
    """Miniatura e foto grande de uma peça; devolve as medidas das duas."""
    im = frente(prefixo)
    if im is None:
        return None
    return (salvar(im, largura, os.path.join(SAIDA, prefixo + ".webp"))
            + salvar(im, GRANDE[classe], os.path.join(SAIDA, "grande", prefixo + ".webp")))


MEDIDAS = {}  # prefixo -> (w, h, gw, gh), preenchido em paralelo antes de montar os cartões


def cartao(prefixo, nome, inspiracao, categoria, largura, classe):
    if MEDIDAS.get(prefixo) is None:
        return ""
    w, h, gw, gh = MEDIDAS[prefixo]
    n, i = html.escape(nome), html.escape(inspiracao)
    c, ce = html.escape(categoria), html.escape(CATEGORIA_EN.get(categoria, categoria))
    return (f'      <article class="{classe}"><button type="button" class="cat-abrir" data-grande="img/catalogo/grande/{prefixo}.webp" '
            f'data-largura="{gw}" data-altura="{gh}" data-inspiracao="{i}" data-categoria="{c}" data-categoria-en="{ce}">'
            f'<img src="img/catalogo/{prefixo}.webp" width="{w}" height="{h}" loading="lazy" alt="{n}" data-alt-en="{n}"><h4>{n}</h4>'
            f'<p><span class="pt">inspirado no {i}</span><span class="en" lang="en">inspired by the {i}</span></p></button></article>\n')


def grupo(chave, titulo_pt, titulo_en, cartoes):
    return (f'    <div class="catalogo-grupo" data-grupo="{chave}">\n'
            f'    <h3 class="catalogo-titulo"><span class="pt">{titulo_pt}</span><span class="en" lang="en">{titulo_en}</span>'
            f' <span class="catalogo-conta">{len(cartoes)}</span></h3>\n    <div class="catalogo">\n' + "".join(cartoes)
            + "    </div>\n    </div>\n")


def filtro(chave, pt, en, conta, ativo=False):
    return (f'      <button type="button" class="filtro" data-filtro="{chave}" aria-pressed="{"true" if ativo else "false"}">'
            f'<span class="pt">{pt}</span><span class="en" lang="en">{en}</span> <span class="catalogo-conta">{conta}</span></button>\n')


def fichas():
    """Fichas de fábrica (assets/gear/<id>_manifesto.json) por tipo, na ordem dos parâmetros."""
    por_tipo = {"pedal": [], "cabecote": [], "caixa": []}
    gear = os.path.join(FONTE, "assets", "gear")
    for nome in sorted(os.listdir(gear)):
        if nome.endswith("_manifesto.json"):
            f = json.load(open(os.path.join(gear, nome), encoding="utf-8"))
            por_tipo[f["tipo"]].append(f)
    for lista_ in por_tipo.values():
        # Fábrica na ordem antiga; as sem índice (novas) no fim, pelo id.
        lista_.sort(key=lambda f: (f.get("indice_de_fabrica") is None, f.get("indice_de_fabrica") or 0, f["id"]))
    return por_tipo


def listas():
    """Peças do programa: das fichas e, numa fonte antiga sem fichas, das tabelas do PluginEditor.cpp."""
    f = fichas()
    if f["pedal"]:
        return ([x["id"] for x in f["cabecote"]], [(x["inspiracao"], x["categoria"]) for x in f["cabecote"]],
                [x["id"] for x in f["caixa"]], [x["inspiracao"] for x in f["caixa"]],
                [x["id"] for x in f["pedal"]], [(x["nome"], x["inspiracao"], x["categoria"], x["fonte"]) for x in f["pedal"]])
    codigo = open(os.path.join(FONTE, "plugin", "source", "PluginEditor.cpp"), encoding="utf-8").read()
    return (textos(lista(codigo, "kAmpPrefixes")), re.findall(r'\{"([^"]*)",\s*"([^"]*)"\}', lista(codigo, "kAmpInfo")),
            textos(lista(codigo, "kCabPrefixes")), textos(lista(codigo, "kCabInfo")), textos(lista(codigo, "kPedalPrefixes")),
            re.findall(r'\{"([^"]*)",\s*"([^"]*)",\s*"([^"]*)",\s*"([^"]*)"\}', lista(codigo, "kPedalInfo")))


def main():
    amp_prefixos, amp_info, cab_prefixos, cab_info, ped_prefixos, ped_info = listas()

    # As imagens são a parte demorada: uma peça por núcleo.
    pedidos = ([(p, 420, "cat-cabecote") for p in amp_prefixos] + [(p, 220, "cat-caixa") for p in cab_prefixos]
               + [(p, 150, "cat-pedal") for p in ped_prefixos])
    with Pool() as fila:
        MEDIDAS.update(zip((p for p, _, _ in pedidos), fila.starmap(imagens, pedidos)))

    guitarra, baixo = [], []
    for prefixo, (insp, cat) in zip(amp_prefixos, amp_info):
        (baixo if cat == "Baixo" else guitarra).append(cartao(prefixo, nome_do_mapa(prefixo), insp, cat, 420, "cat-cabecote"))
    caixas = [cartao(p, nome_do_mapa(p), insp, "Caixas", 220, "cat-caixa") for p, insp in zip(cab_prefixos, cab_info)]
    pedais = [cartao(p, nome, insp, cat, 150, "cat-pedal") for p, (nome, insp, cat, fonte) in zip(ped_prefixos, ped_info)]
    guitarra, baixo, caixas, pedais = ([c for c in g if c] for g in (guitarra, baixo, caixas, pedais))
    total = len(guitarra) + len(baixo) + len(caixas) + len(pedais)

    trecho = ('    <div class="cabeca-secao catalogo-cabeca" id="galeria">\n'
              '      <h3><span class="pt">Galeria de equipamentos</span><span class="en" lang="en">Gear gallery</span></h3>\n'
              '      <p class="pt">Toque em uma peça para ver a foto grande; as setas passam para a próxima. Ao escolher uma peça, '
              'o Nexus baixa sozinho a captura mais curtida pela comunidade da TONE3000, e cada cabeçote entra com a caixa feita para ele.</p>\n'
              '      <p class="en" lang="en">Tap a piece to see the large photo; the arrows move to the next one. Pick a piece and Nexus '
              'downloads the community\'s most-liked TONE3000 capture on its own, and each head comes with the cabinet made for it.</p>\n'
              '    </div>\n'
              '    <div class="filtros" role="group" aria-label="Mostrar" data-aria-en="Show">\n'
              + filtro("todos", "Todos", "All", total, True)
              + filtro("guitarra", "Cabeçotes de guitarra", "Guitar heads", len(guitarra))
              + filtro("baixo", "Cabeçotes de baixo", "Bass heads", len(baixo))
              + filtro("caixas", "Caixas", "Cabinets", len(caixas))
              + filtro("pedais", "Pedais", "Pedals", len(pedais))
              + '    </div>\n'
              + grupo("guitarra", "Cabeçotes de guitarra", "Guitar heads", guitarra)
              + grupo("baixo", "Cabeçotes de baixo", "Bass heads", baixo)
              + grupo("caixas", "Caixas", "Cabinets", caixas)
              + grupo("pedais", "Pedais", "Pedals", pedais))
    caminho = os.path.join(SITE, "index.html")
    pagina = open(caminho, encoding="utf-8").read()
    inicio, fim = "<!-- catalogo:inicio -->", "<!-- catalogo:fim -->"
    a, b = pagina.index(inicio) + len(inicio), pagina.index(fim)
    pagina = pagina[:a] + "\n" + trecho + "    " + pagina[b:]
    open(caminho, "w", encoding="utf-8").write(pagina)
    print(f"{len(guitarra)} cabeçotes de guitarra, {len(baixo)} de baixo, {len(caixas)} caixas, {len(pedais)} pedais")


if __name__ == "__main__":
    main()
