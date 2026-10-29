#!/usr/bin/env python3
"""Troca a frente de cada equipamento (PNG com transparência, uns 3 MB) por
um JPEG da cor já multiplicada pela transparência (fundo preto) mais uma
máscara de transparência em PNG cinza. O programa junta as duas ao abrir
(ui/Gear.cpp): o visual é o mesmo e o tamanho cai umas cinco vezes, o que
importa porque a arte vai embutida no programa e em cada plugin.

Uso (com Pillow): frente_jpeg.py <pasta assets/gear> [qualidade]
Gera <prefixo>_frente.jpg e <prefixo>_frente_alfa.png e apaga o _frente.png."""
import os
import sys

from PIL import Image


def main():
    pasta = sys.argv[1]
    qualidade = int(sys.argv[2]) if len(sys.argv) > 2 else 92
    antes = depois = 0
    for nome in sorted(os.listdir(pasta)):
        if not nome.endswith("_frente.png"):
            continue
        caminho = os.path.join(pasta, nome)
        base = caminho[: -len(".png")]
        im = Image.open(caminho).convert("RGBA")
        cor = Image.new("RGB", im.size, (0, 0, 0))
        cor.paste(im, (0, 0), im)  # cor * transparência, como o JUCE guarda
        cor.save(base + ".jpg", quality=qualidade, optimize=True, progressive=True, subsampling=0)
        im.split()[3].save(base + "_alfa.png", optimize=True)
        antes += os.path.getsize(caminho)
        depois += os.path.getsize(base + ".jpg") + os.path.getsize(base + "_alfa.png")
        os.remove(caminho)
    print(f"{antes / 1e6:.1f} MB -> {depois / 1e6:.1f} MB")


if __name__ == "__main__":
    main()
