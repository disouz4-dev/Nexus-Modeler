#!/usr/bin/env python3
"""Monta o catálogo completo do site (docs/site/index.html) a partir do que o
programa tem: as listas de equipamentos de plugin/source/PluginEditor.cpp,
os nomes dos mapas em assets/gear e a arte de frente de cada peça.

Gera as miniaturas em docs/site/img/catalogo/<peça>.webp e troca o trecho
entre <!-- catalogo:inicio --> e <!-- catalogo:fim --> do index.html.
Rodar de novo sempre que entrar equipamento novo.

Uso (com Pillow): python3 tools/site/catalogo.py [raiz do projeto]
"""
import html
import json
import os
import re
import sys

from PIL import Image

RAIZ = os.path.abspath(sys.argv[1] if len(sys.argv) > 1 else os.path.join(os.path.dirname(__file__), "..", ".."))
GEAR = os.path.join(RAIZ, "assets", "gear")
SITE = os.path.join(RAIZ, "docs", "site")
SAIDA = os.path.join(SITE, "img", "catalogo")

# Categorias em inglês (o site tem as duas línguas).
CATEGORIA_EN = {
    "Britânicos": "British", "Americanos de alto ganho": "American high gain", "Americanos boutique": "American boutique",
    "Americanos clássicos": "American classics", "Alemães de alto ganho": "German high gain",
    "Limpos de transistor": "Solid-state cleans", "Baixo": "Bass", "Drive e distorção": "Drive and distortion",
    "Modulação": "Modulation", "Delay": "Delay", "Reverb": "Reverb", "Utilidades": "Utilities", "Afinação": "Pitch",
}


def lista(codigo, nome):
    """Conteúdo entre chaves de uma tabela C++ (const ... nome[...] = { ... };)."""
    m = re.search(re.escape(nome) + r"\[[^\]]*\]\s*=\s*\{(.*?)\};", codigo, re.S)
    return m.group(1)


def textos(trecho):
    return re.findall(r'"((?:[^"\\]|\\.)*)"', trecho)


def nome_do_mapa(prefixo):
    caminho = os.path.join(GEAR, prefixo + "_mapa.json")
    return json.load(open(caminho, encoding="utf-8")).get("nome", prefixo) if os.path.exists(caminho) else prefixo


def miniatura(prefixo, largura):
    """Frente da peça (JPEG + máscara, ou PNG) em WebP com transparência."""
    jpg, alfa, png = (os.path.join(GEAR, prefixo + s) for s in ("_frente.jpg", "_frente_alfa.png", "_frente.png"))
    if os.path.exists(jpg) and os.path.exists(alfa):
        # A cor vem multiplicada pela transparência: só muda a borda
        # semitransparente (um pouco mais escura), invisível no fundo escuro.
        cor = Image.open(jpg).convert("RGB")
        im = Image.merge("RGBA", (*cor.split(), Image.open(alfa).convert("L")))
    elif os.path.exists(png):
        im = Image.open(png).convert("RGBA")
    else:
        return None
    im = im.crop(im.getbbox())
    altura = round(im.height * largura / im.width)
    im = im.resize((largura, altura), Image.LANCZOS)
    os.makedirs(SAIDA, exist_ok=True)
    destino = os.path.join(SAIDA, prefixo + ".webp")
    im.save(destino, "WEBP", quality=82, method=6)
    return largura, altura


def cartao(prefixo, nome, inspiracao, largura, classe):
    medida = miniatura(prefixo, largura)
    if medida is None:
        return ""
    w, h = medida
    n, i = html.escape(nome), html.escape(inspiracao)
    return (f'      <article class="{classe}"><img src="img/catalogo/{prefixo}.webp" width="{w}" height="{h}" loading="lazy" '
            f'alt="{n}" data-alt-en="{n}"><h4>{n}</h4>'
            f'<p><span class="pt">inspirado no {i}</span><span class="en" lang="en">inspired by the {i}</span></p></article>\n')


def grupo(titulo_pt, titulo_en, cartoes):
    return (f'    <h3 class="catalogo-titulo"><span class="pt">{titulo_pt}</span><span class="en" lang="en">{titulo_en}</span>'
            f' <span class="catalogo-conta">{len(cartoes)}</span></h3>\n    <div class="catalogo">\n' + "".join(cartoes) + "    </div>\n")


def main():
    codigo = open(os.path.join(RAIZ, "plugin", "source", "PluginEditor.cpp"), encoding="utf-8").read()
    amp_prefixos = textos(lista(codigo, "kAmpPrefixes"))
    amp_info = re.findall(r'\{"([^"]*)",\s*"([^"]*)"\}', lista(codigo, "kAmpInfo"))
    cab_prefixos = textos(lista(codigo, "kCabPrefixes"))
    cab_info = textos(lista(codigo, "kCabInfo"))
    ped_prefixos = textos(lista(codigo, "kPedalPrefixes"))
    ped_info = re.findall(r'\{"([^"]*)",\s*"([^"]*)",\s*"([^"]*)",\s*"([^"]*)"\}', lista(codigo, "kPedalInfo"))

    guitarra, baixo = [], []
    for prefixo, (insp, cat) in zip(amp_prefixos, amp_info):
        (baixo if cat == "Baixo" else guitarra).append(cartao(prefixo, nome_do_mapa(prefixo), insp, 420, "cat-cabecote"))
    caixas = [cartao(p, nome_do_mapa(p), insp, 220, "cat-caixa") for p, insp in zip(cab_prefixos, cab_info)]
    pedais = [cartao(p, nome, insp, 150, "cat-pedal") for p, (nome, insp, cat, fonte) in zip(ped_prefixos, ped_info)]

    trecho = ('    <div class="cabeca-secao catalogo-cabeca">\n'
              '      <h3><span class="pt">Catálogo completo</span><span class="en" lang="en">Full catalog</span></h3>\n'
              '      <p class="pt">Ao escolher uma peça, o Nexus baixa sozinho a captura mais curtida pela comunidade da TONE3000; '
              'cada cabeçote entra com a caixa feita para ele.</p>\n'
              '      <p class="en" lang="en">Pick a piece and Nexus downloads the community\'s most-liked TONE3000 capture on its own; '
              'each head comes with the cabinet made for it.</p>\n    </div>\n'
              + grupo("Cabeçotes de guitarra", "Guitar heads", guitarra)
              + grupo("Cabeçotes de baixo", "Bass heads", baixo)
              + grupo("Caixas", "Cabinets", caixas)
              + grupo("Pedais", "Pedals", pedais))
    caminho = os.path.join(SITE, "index.html")
    pagina = open(caminho, encoding="utf-8").read()
    inicio, fim = "<!-- catalogo:inicio -->", "<!-- catalogo:fim -->"
    a, b = pagina.index(inicio) + len(inicio), pagina.index(fim)
    pagina = pagina[:a] + "\n" + trecho + "    " + pagina[b:]
    open(caminho, "w", encoding="utf-8").write(pagina)
    print(f"{len(guitarra)} cabeçotes de guitarra, {len(baixo)} de baixo, {len(caixas)} caixas, {len(pedais)} pedais")


if __name__ == "__main__":
    main()
