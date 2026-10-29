#!/usr/bin/env python3
"""Gera assets/gear/indice.json, o índice que o Nexus lê no GitHub para achar
equipamentos novos sem reinstalar (plugin/source/update/ArteNova.cpp).

Para cada peça (cada <prefixo>_mapa.json em assets/gear): os arquivos dela, o
hash de todos juntos, a data em que o hash mudou pela última vez e a versão
mínima do Nexus que entende a peça. A data só muda quando a arte muda, e o
Nexus só oferece a peça cuja data é mais nova que a que ele já tem.

Uso: indice_arte.py [versão mínima]   (padrão: a versão do CMakeLists.txt)
"""
import datetime
import hashlib
import json
import os
import re
import sys

RAIZ = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
GEAR = os.path.join(RAIZ, "assets", "gear")
INDICE = os.path.join(GEAR, "indice.json")


def versao_do_projeto():
    texto = open(os.path.join(RAIZ, "CMakeLists.txt"), encoding="utf-8").read()
    m = re.search(r"project\([^)]*VERSION\s+([0-9.]+)", texto)
    return m.group(1) if m else "0.0.0"


def main():
    minimo = sys.argv[1] if len(sys.argv) > 1 else versao_do_projeto()
    antigo = {}
    if os.path.exists(INDICE):
        antigo = json.load(open(INDICE, encoding="utf-8")).get("pecas", {})
    agora = datetime.datetime.now(datetime.timezone.utc).strftime("%Y-%m-%dT%H:%M:%SZ")
    nomes = sorted(os.listdir(GEAR))
    pecas = {}
    for mapa in nomes:
        if not mapa.endswith("_mapa.json"):
            continue
        prefixo = mapa[: -len("_mapa.json")]
        # Só os arquivos desta peça ("mark5_" não pode levar os da "mark5_412").
        resto = lambda n: n[len(prefixo) + 1:]
        arquivos = [n for n in nomes if n.startswith(prefixo + "_")
                    and (resto(n) in ("mapa.json", "frente.jpg", "frente_alfa.png", "frente.png") or resto(n).startswith("botao_"))]
        h = hashlib.sha256()
        for n in arquivos:
            h.update(n.encode())
            h.update(open(os.path.join(GEAR, n), "rb").read())
        hash_ = h.hexdigest()[:16]
        velho = antigo.get(prefixo, {})
        mesmo = velho.get("hash") == hash_
        pecas[prefixo] = {
            "nome": json.load(open(os.path.join(GEAR, mapa), encoding="utf-8")).get("nome", prefixo),
            "hash": hash_,
            "data": velho.get("data", agora) if mesmo else agora,
            "nexus_minimo": velho.get("nexus_minimo", minimo) if mesmo else minimo,
            "arquivos": arquivos,
        }
    json.dump({"gerado": agora, "pecas": pecas}, open(INDICE, "w", encoding="utf-8"), ensure_ascii=False, indent=1)
    print(f"{len(pecas)} peças em {os.path.relpath(INDICE, RAIZ)}")


if __name__ == "__main__":
    main()
