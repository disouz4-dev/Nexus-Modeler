#!/usr/bin/env python3
"""Gera a ficha (manifesto) de cada equipamento de fábrica em
assets/gear/<id>_manifesto.json, a partir das tabelas que ficavam no código
(kPedalPrefixes, kPedalInfo, kPedalKinds, kAmpPrefixes, kAmpInfo, kCabOfAmp,
kCabPrefixes, kCabInfo e hasGraphicEq em plugin/source/PluginEditor.cpp e
PluginProcessor.h).

Desde o catálogo unificado (plugin/source/gear/GearCatalog.cpp), as fichas é
que valem: o programa lê só elas. O script serve para refazer as fichas do
zero a partir de um commit que ainda tinha as tabelas (padrão: 3901d61, o
último antes do catálogo). Equipamento novo ganha a ficha pelo empacotador
(tools/empacotar/empacotar.py nova), não por aqui.

O nome do arquivo segue o padrão <id>_*, como o resto da arte, para que
tools/renomear_pedais.py (ramo modelagem-fiel) troque o código de um pedal
renomeando os arquivos e o texto de dentro.

Uso: manifestos.py [--commit 3901d61 | --pasta <raiz com as tabelas>] [--conferir]
--conferir só compara com as fichas que já existem (sai com erro se mudou).
"""
import argparse
import json
import os
import re
import subprocess
import sys

RAIZ = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
GEAR = os.path.join(RAIZ, "assets", "gear")

# Nome de cada motor de pedal na ficha, na ordem do enum PedalKind.
MOTORES = {
    "capture": "captura", "chorus": "chorus", "delay": "delay", "reverb": "reverb", "gate": "gate", "drop": "drop",
    "phaser": "phaser", "flanger": "flanger", "tremolo": "tremolo", "vibe": "vibe", "wah": "wah", "rotary": "rotativo",
    "analogdelay": "delay_analogico", "octave": "oitava", "comp": "compressor", "geq": "equalizador_grafico",
    "sky": "reverb_modos", "echo": "delay_modos", "multiMod": "modulacao_modos", "triple": "delay_triplo",
}


def ler(args, caminho):
    if args.pasta:
        return open(os.path.join(args.pasta, caminho), encoding="utf-8").read()
    return subprocess.run(["git", "-C", RAIZ, "show", f"{args.commit}:{caminho}"], check=True, capture_output=True).stdout.decode("utf-8")


def lista(codigo, nome):
    """Conteúdo entre chaves de uma tabela C++ (... nome[...] = { ... };)."""
    m = re.search(re.escape(nome) + r"\[[^\]]*\]\s*=\s*\{(.*?)\};", codigo, re.S)
    if not m:
        sys.exit(f"tabela {nome} não encontrada")
    return m.group(1)


def textos(trecho):
    return [s.encode("latin-1", "backslashreplace").decode("unicode_escape").encode("latin-1").decode("utf-8")
            if "\\" in s else s for s in re.findall(r'"((?:[^"\\]|\\.)*)"', trecho)]


def nome_do_mapa(prefixo):
    caminho = os.path.join(GEAR, prefixo + "_mapa.json")
    return json.load(open(caminho, encoding="utf-8")).get("nome", "") if os.path.exists(caminho) else ""


def ficha(id_, tipo, indice, nome, inspiracao, categoria, fonte, motor, **resto):
    f = {
        "formato": 1,
        "id": id_,
        "versao": "1.0.0",
        "tipo": tipo,
        "nome": nome,
        "inspiracao": inspiracao,
        "categoria": categoria,
        "fonte": fonte,
        "versao_minima_do_programa": "0.2.0",
        "motor": motor,
        "regulagem": {},
        # Fábrica: as capturas recomendadas continuam em assets/tone3000/recomendadas.json.
        "capturas": {"origem": "tone3000"},
        "indice_de_fabrica": indice,
        "signature": None,
    }
    f.update(resto)
    return f


def gerar(args):
    editor = ler(args, "plugin/source/PluginEditor.cpp")
    proc = ler(args, "plugin/source/PluginProcessor.h")
    ped = textos(lista(editor, "kPedalPrefixes"))
    ped_info = re.findall(r'\{"([^"]*)",\s*"([^"]*)",\s*"([^"]*)",\s*"([^"]*)"\}', lista(editor, "kPedalInfo"))
    kinds = re.findall(r"PedalKind::(\w+)", lista(proc, "kPedalKinds"))
    amps = textos(lista(editor, "kAmpPrefixes"))
    amp_info = re.findall(r'\{"([^"]*)",\s*"([^"]*)"\}', lista(editor, "kAmpInfo"))
    cab_of_amp = [int(x) for x in re.findall(r"-?\d+", lista(editor, "kCabOfAmp"))]
    cabs = textos(lista(editor, "kCabPrefixes"))
    cab_info = textos(lista(editor, "kCabInfo"))
    m = re.search(r"static bool hasGraphicEq\(int look\)\s*\{\s*return (.*?);", proc)
    mark5 = int(re.search(r"kMarkFive\s*=\s*(\d+)", proc).group(1))
    grafico = {mark5 if n == "kMarkFive" else int(n) for n in re.findall(r"look == (\w+)", m.group(1))}
    contagens = {n: int(re.search(rf"{n}\s*=\s*(\d+)", proc).group(1)) for n in ("kPedalLooks", "kAmpLooks", "kCabLooks")}
    if not (len(ped) == len(ped_info) == len(kinds) == contagens["kPedalLooks"]):
        sys.exit(f"pedais não batem: {len(ped)} {len(ped_info)} {len(kinds)} {contagens['kPedalLooks']}")
    if not (len(amps) == len(amp_info) == len(cab_of_amp) == contagens["kAmpLooks"]):
        sys.exit("cabeçotes não batem")
    if not (len(cabs) == len(cab_info) == contagens["kCabLooks"]):
        sys.exit("caixas não batem")

    fichas = []
    for i, (p, (nome, insp, cat, fonte), k) in enumerate(zip(ped, ped_info, kinds)):
        fichas.append(ficha(p, "pedal", i, nome, insp, cat, fonte, MOTORES[k]))
    for i, (p, (insp, cat)) in enumerate(zip(amps, amp_info)):
        extra = {"caixa_padrao": cabs[cab_of_amp[i]]}
        if i in grafico:
            extra["equalizador_grafico"] = True
        fichas.append(ficha(p, "cabecote", i, nome_do_mapa(p) or p, insp, cat, "Captura da TONE3000", "captura", **extra))
    for i, (p, insp) in enumerate(zip(cabs, cab_info)):
        # A família na lista é o tamanho no fim do nome ("4x12", "8x10").
        fichas.append(ficha(p, "caixa", i, nome_do_mapa(p) or p, insp, insp.rsplit(" ", 1)[-1], "IR ou captura full rig", "caixa"))
    return fichas


def main():
    a = argparse.ArgumentParser()
    a.add_argument("--commit", default="3901d61")
    a.add_argument("--pasta")
    a.add_argument("--conferir", action="store_true")
    args = a.parse_args()
    mudou = 0
    fichas = gerar(args)
    for f in fichas:
        caminho = os.path.join(GEAR, f["id"] + "_manifesto.json")
        texto = json.dumps(f, ensure_ascii=False, indent=2) + "\n"
        antigo = open(caminho, encoding="utf-8").read() if os.path.exists(caminho) else None
        if antigo != texto:
            mudou += 1
            if args.conferir:
                print("diferente:", os.path.relpath(caminho, RAIZ))
            else:
                open(caminho, "w", encoding="utf-8").write(texto)
    tipos = {t: sum(f["tipo"] == t for f in fichas) for t in ("pedal", "cabecote", "caixa")}
    print(f"{tipos['pedal']} pedais, {tipos['cabecote']} cabeçotes, {tipos['caixa']} caixas; {mudou} fichas "
          + ("diferentes" if args.conferir else "gravadas"))
    sys.exit(1 if args.conferir and mudou else 0)


if __name__ == "__main__":
    main()
