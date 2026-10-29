#!/usr/bin/env python3
"""Prepara as imagens de um pedal para entrar no plugin, a partir do render
completo em build/gear/<pedal>/:

- reduz tudo pela escala (0,5 = metade: o pedal já fica nítido na tela Retina);
- o botão que troca capturas (drive, "ganho") só para em 11 posições (0 a 10),
  então a tira dele fica só com 11 quadros em vez de 101;
- corrige o mapa (posições de botões, rótulos, pisante e LED).

Uso (com Pillow): reduzir_pedal.py <pedal> [escala]   ex.: reduzir_pedal.py lemon_drive 0.5
Grava em assets/gear/<pedal>_*.
"""
import json
import os
import sys

from PIL import Image

RAIZ = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
DEGRAUS = {"ganho": 11, "modo": 11}  # botões em degraus: quantos quadros ficam


def main():
    pedal = sys.argv[1]
    escala = float(sys.argv[2]) if len(sys.argv) > 2 else 0.5
    reduzir(pedal, escala, os.path.join(RAIZ, "build", "gear", pedal), os.path.join(RAIZ, "assets", "gear"))


def reduzir(pedal, escala, origem, pasta_destino):
    """Lê o render de <origem> (frente.png, botao_<id>.png, mapa.json) e grava
    <pasta_destino>/<pedal>_* (também usado por montar_equipamento.py)."""
    destino = os.path.join(pasta_destino, pedal + "_")
    mapa = json.load(open(os.path.join(origem, "mapa.json"), encoding="utf-8"))
    s = lambda v: round(v * escala, 1)

    frente = Image.open(os.path.join(origem, "frente.png"))
    w, h = round(frente.width * escala), round(frente.height * escala)
    frente.resize((w, h), Image.LANCZOS).save(destino + "frente.png", optimize=True)
    total = mapa["posicoes"]
    mapa["largura"], mapa["altura"] = w, h

    for b in mapa["botoes"]:
        tira = Image.open(os.path.join(origem, b["arquivo"]))
        lado = b["tamanho"]
        novo = round(lado * escala)
        quadros = DEGRAUS.get(b["id"], total)
        # Quadros escolhidos espalhados pela volta toda (0, 10, 20... de 100).
        escolhidos = [round(i * (total - 1) / (quadros - 1)) for i in range(quadros)] if quadros > 1 else [0]
        saida = Image.new("RGBA", (novo, novo * len(escolhidos)))
        for n, q in enumerate(escolhidos):
            quadro = tira.crop((0, q * lado, lado, (q + 1) * lado)).resize((novo, novo), Image.LANCZOS)
            saida.paste(quadro, (0, n * novo))
        saida.save(destino + "botao_" + b["id"] + ".png", optimize=True)
        b["x"], b["y"] = round(b["x"] * escala), round(b["y"] * escala)
        b["tamanho"] = novo
        if quadros != total:
            b["posicoes"] = quadros

    for r in mapa.get("rotulos", []):
        r["x"], r["y"], r["altura"] = s(r["x"]), s(r["y"]), round(r["altura"] * escala, 2)
    for chave in ("pisante", "led"):
        if chave in mapa:
            m = mapa[chave]
            m["x"], m["y"], m["raio"] = s(m["x"]), s(m["y"]), s(m["raio"])
    if "visor" in mapa:
        v = mapa["visor"]
        v["x"], v["y"], v["w"], v["h"] = s(v["x"]), s(v["y"]), s(v["w"]), s(v["h"])
    for led in mapa.get("leds_canais", []):
        led["x"], led["y"], led["raio"] = s(led["x"]), s(led["y"]), s(led["raio"])

    json.dump(mapa, open(destino + "mapa.json", "w", encoding="utf-8"), ensure_ascii=False, indent=2)
    print(pedal, "pronto:", w, "x", h)


if __name__ == "__main__":
    main()
