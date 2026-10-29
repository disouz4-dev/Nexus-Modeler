#!/usr/bin/env python3
"""Reduz as tiras dos botões a 256 cores (com transparência e pontilhado).
Na tela, no tamanho de verdade, não dá para ver diferença (erro médio de
3 em 255), e cada tira fica umas 10 vezes menor: é o que mantém o
instalador pequeno com dezenas de equipamentos. A frente fica intacta.

Uso (com Pillow): reduzir_cores.py arquivos...   (só PNG com transparência)
Pula a tira que já está em cores indexadas."""
import os
import sys

from PIL import Image


def main():
    antes = depois = 0
    for caminho in sys.argv[1:]:
        im = Image.open(caminho)
        if im.mode == "P":
            continue
        antes += os.path.getsize(caminho)
        q = im.convert("RGBA").quantize(256, method=Image.Quantize.FASTOCTREE, dither=Image.Dither.FLOYDSTEINBERG)
        q.save(caminho, optimize=True)
        depois += os.path.getsize(caminho)
    print(f"{antes / 1e6:.1f} MB -> {depois / 1e6:.1f} MB")


if __name__ == "__main__":
    main()
