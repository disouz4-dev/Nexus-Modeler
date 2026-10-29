#!/usr/bin/env python3
"""Transforma o render de uma peça em equipamento pronto para o Nexus baixar,
sem versão nova do programa. Uma ferramenta só, de linha de comando, que roda
no lab2 (Linux, python3 + Pillow) logo depois do render:

  1. lê a saída do render (<saida>/frente.png, botao_<id>.png, mapa.json) e a
     receita (tools/gear_studio/recipes/<id>.json);
  2. prepara a arte como as outras peças: pedal passa por reduzir_pedal.py
     (metade do tamanho, 11 quadros no botão de ganho); a frente vira
     frente.jpg + frente_alfa.png (frente_jpeg.py); as tiras vão a 256 cores
     (reduzir_cores.py);
  3. confere o mapa contra as tiras e escreve a ficha <id>_manifesto.json
     (nova, ou a que já existe com a versão subindo);
  4. troca os arquivos assets/gear/<id>_* e refaz assets/gear/indice.json
     (indice_arte.py), que é o que os Nexus instalados leem no GitHub;
  5. opcional: --pacote <pasta> monta <id>-<versao>.equipamento (zip para
     instalar arrastando na janela, bom para testar antes de publicar) e
     --commit grava o commit (sem enviar: o envio é da automação).

Exemplos:
  montar_equipamento.py kiwi_drive --saida ~/nexus-gear/saida/kiwi_drive
  montar_equipamento.py fig_fuzz --saida ~/nexus-gear/saida/fig_fuzz \\
      --inspiracao "Fuzz Face" --categoria "Drive e distorção" --busca "Fuzz Face" --commit

Campos da ficha (todos opcionais na linha de comando; uma peça nova precisa
de inspiração e categoria, que também podem vir no bloco "ficha" da receita):
--nome, --inspiracao, --categoria, --fonte, --motor, --caixa-padrao,
--busca (palavra da pesquisa na TONE3000), --recomendadas (arquivo JSON com o
bloco {"busca", "tons"} no formato do assets/tone3000/recomendadas.json),
--versao (padrão: sobe o último número se a peça mudou), --versao-minima.
"""
import argparse
import glob
import json
import os
import shutil
import subprocess
import sys
import tempfile
import zipfile

AQUI = os.path.dirname(os.path.abspath(__file__))
RAIZ = os.path.abspath(os.path.join(AQUI, "..", ".."))
GEAR = os.path.join(RAIZ, "assets", "gear")
sys.path.insert(0, AQUI)

# Motores que o Nexus sabe tocar (plugin/source/gear/GearCatalog.h).
MOTORES_PEDAL = ["captura", "chorus", "delay", "reverb", "gate", "drop", "phaser", "flanger", "tremolo", "vibe", "wah",
                 "rotativo", "delay_analogico", "oitava", "compressor", "equalizador_grafico", "reverb_modos",
                 "delay_modos", "modulacao_modos", "delay_triplo"]
FONTE_PADRAO = {"pedal": "Captura da TONE3000", "cabecote": "Captura da TONE3000", "caixa": "IR ou captura full rig"}
MOTOR_PADRAO = {"pedal": "captura", "cabecote": "captura", "caixa": "caixa"}
EXTENSOES = (".json", ".png", ".jpg", ".wav", ".nam")


def falha(texto):
    sys.exit("montar_equipamento: " + texto)


def rodar(*args):
    subprocess.run([sys.executable, *args], check=True, stdout=subprocess.DEVNULL)


def preparar_arte(id_, tipo, saida, destino, escala):
    """Arte do render -> <destino>/<id>_* no formato das outras peças."""
    for nome in ("frente.png", "mapa.json"):
        if not os.path.exists(os.path.join(saida, nome)):
            falha(f"falta {nome} em {saida}")
    if tipo == "pedal":
        import reduzir_pedal
        reduzir_pedal.reduzir(id_, escala, saida, destino)
    else:
        shutil.copy(os.path.join(saida, "frente.png"), os.path.join(destino, id_ + "_frente.png"))
        shutil.copy(os.path.join(saida, "mapa.json"), os.path.join(destino, id_ + "_mapa.json"))
        for tira in glob.glob(os.path.join(saida, "botao_*.png")):
            shutil.copy(tira, os.path.join(destino, id_ + "_" + os.path.basename(tira)))
    rodar(os.path.join(AQUI, "frente_jpeg.py"), destino)
    tiras = sorted(glob.glob(os.path.join(destino, id_ + "_botao_*.png")))
    if tiras:
        rodar(os.path.join(AQUI, "reduzir_cores.py"), *tiras)


def conferir(id_, pasta):
    mapa = json.load(open(os.path.join(pasta, id_ + "_mapa.json"), encoding="utf-8"))
    for b in mapa.get("botoes", []):
        if not os.path.exists(os.path.join(pasta, f"{id_}_botao_{b['id']}.png")):
            falha(f"o mapa tem o botão {b['id']} e falta a tira dele")
    for nome in os.listdir(pasta):
        if not nome.startswith(id_ + "_") or not nome.endswith(EXTENSOES):
            falha(f"arquivo fora do padrão: {nome}")
    return mapa


def subir(versao):
    partes = [int(x) for x in versao.split(".")] + [0, 0]
    return f"{partes[0]}.{partes[1]}.{partes[2] + 1}"


def ficha(args, id_, tipo, receita, mapa, mudou):
    caminho = os.path.join(GEAR, id_ + "_manifesto.json")
    antiga = json.load(open(caminho, encoding="utf-8")) if os.path.exists(caminho) else None
    base = dict(receita.get("ficha", {}))
    f = dict(antiga) if antiga else {
        "formato": 1, "id": id_, "versao": "1.0.0", "tipo": tipo,
        "nome": mapa.get("nome") or receita.get("nome") or id_,
        "inspiracao": base.get("inspiracao", ""), "categoria": base.get("categoria", ""),
        "fonte": base.get("fonte", FONTE_PADRAO[tipo]),
        "versao_minima_do_programa": base.get("versao_minima_do_programa", "0.2.0"),
        "motor": base.get("motor", MOTOR_PADRAO[tipo]), "regulagem": base.get("regulagem", {}),
        "capturas": base.get("capturas", {"origem": "tone3000"}), "signature": None,
    }
    if tipo == "cabecote" and "caixa_padrao" in base and not antiga:
        f["caixa_padrao"] = base["caixa_padrao"]
    for campo, valor in (("nome", args.nome), ("inspiracao", args.inspiracao), ("categoria", args.categoria),
                         ("fonte", args.fonte), ("motor", args.motor), ("caixa_padrao", args.caixa_padrao),
                         ("versao_minima_do_programa", args.versao_minima)):
        if valor:
            f[campo] = valor
    if args.recomendadas:
        f["capturas"] = {"origem": "tone3000", "recomendadas": json.load(open(args.recomendadas, encoding="utf-8"))}
    elif args.busca:
        rec = dict(f.get("capturas", {}).get("recomendadas") or {"tons": []})
        rec["busca"] = args.busca
        f["capturas"] = {"origem": "tone3000", "recomendadas": rec}
    if args.versao:
        f["versao"] = args.versao
    elif antiga and (mudou or f != antiga):
        f["versao"] = subir(antiga.get("versao", "1.0.0"))
    if f["tipo"] != tipo:
        falha(f"a ficha diz {f['tipo']} e a receita diz {tipo}")
    if not f.get("inspiracao") or not f.get("categoria"):
        falha("peça nova precisa de --inspiracao e --categoria (ou do bloco \"ficha\" na receita)")
    motores = MOTORES_PEDAL if tipo == "pedal" else [MOTOR_PADRAO[tipo]]
    if f["motor"] not in motores:
        falha(f"motor {f['motor']} não existe no Nexus (aceitos: {', '.join(motores)})")
    return f


def da_peca(id_, nome, com_ficha=False):
    """O arquivo é da peça (e não de outra com o mesmo começo: mark5 e mark5_412)?"""
    if not nome.startswith(id_ + "_"):
        return False
    resto = nome[len(id_) + 1:]
    return (resto in ("mapa.json", "frente.jpg", "frente_alfa.png", "frente.png") or resto.startswith("botao_")
            or (com_ficha and resto == "manifesto.json"))


def mesmos_arquivos(id_, pasta):
    """A arte nova é igual à que já está em assets/gear?"""
    novos = sorted(os.listdir(pasta))
    velhos = sorted(n for n in os.listdir(GEAR) if da_peca(id_, n))
    if novos != velhos:
        return False
    return all(open(os.path.join(pasta, n), "rb").read() == open(os.path.join(GEAR, n), "rb").read() for n in novos)


def pacote(id_, versao, destino):
    os.makedirs(destino, exist_ok=True)
    caminho = os.path.join(destino, f"{id_}-{versao}.equipamento")
    with zipfile.ZipFile(caminho, "w", zipfile.ZIP_DEFLATED) as z:
        for n in sorted(n for n in os.listdir(GEAR) if da_peca(id_, n, True)):
            z.write(os.path.join(GEAR, n), n)
    return caminho


def main():
    a = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    a.add_argument("id")
    a.add_argument("--saida", required=True, help="pasta do render (frente.png, botao_<id>.png, mapa.json)")
    a.add_argument("--receita", help="padrão: tools/gear_studio/recipes/<id>.json")
    a.add_argument("--tipo", choices=["pedal", "cabecote", "caixa"])
    a.add_argument("--escala", type=float, default=0.5, help="pedais: escala da arte (padrão 0,5)")
    for campo in ("nome", "inspiracao", "categoria", "fonte", "motor", "caixa-padrao", "busca", "recomendadas", "versao",
                  "versao-minima"):
        a.add_argument("--" + campo)
    a.add_argument("--sem-indice", action="store_true", help="não refaz o assets/gear/indice.json")
    a.add_argument("--pacote", help="pasta onde gravar <id>-<versao>.equipamento")
    a.add_argument("--commit", action="store_true", help="grava o commit da peça (não envia)")
    args = a.parse_args()
    id_ = args.id
    if not id_ or not all(c.islower() or c.isdigit() or c == "_" for c in id_):
        falha("id só com letras minúsculas, números e _")
    receita_ = args.receita or os.path.join(AQUI, "recipes", id_ + ".json")
    receita = json.load(open(receita_, encoding="utf-8")) if os.path.exists(receita_) else {}
    tipo = args.tipo or receita.get("tipo")
    if tipo not in FONTE_PADRAO:
        falha("tipo desconhecido (pedal, cabecote ou caixa): passe --tipo ou a receita")

    with tempfile.TemporaryDirectory() as pasta:
        preparar_arte(id_, tipo, os.path.expanduser(args.saida), pasta, args.escala)
        mapa = conferir(id_, pasta)
        mudou = not mesmos_arquivos(id_, pasta)
        f = ficha(args, id_, tipo, receita, mapa, mudou)
        # Troca a arte inteira da peça (sem deixar tira de botão que saiu).
        for velho in os.listdir(GEAR):
            if da_peca(id_, velho):
                os.remove(os.path.join(GEAR, velho))
        for nome in os.listdir(pasta):
            shutil.move(os.path.join(pasta, nome), os.path.join(GEAR, nome))
    with open(os.path.join(GEAR, id_ + "_manifesto.json"), "w", encoding="utf-8") as out:
        out.write(json.dumps(f, ensure_ascii=False, indent=2) + "\n")
    if not args.sem_indice:
        subprocess.run([sys.executable, os.path.join(AQUI, "indice_arte.py")], check=True)
    print(f"{id_} ({f['nome']}, {tipo}, versão {f['versao']}) pronto em assets/gear")
    if args.pacote:
        print("pacote:", pacote(id_, f["versao"], args.pacote))
    if args.commit:
        # "add -A" com os nomes: entra também a tira de botão que saiu.
        arquivos = [os.path.join("assets", "gear", n) for n in sorted(set(os.listdir(GEAR)) | set(
            os.path.basename(x) for x in subprocess.run(["git", "-C", RAIZ, "ls-files", "assets/gear"], capture_output=True,
                                                        text=True).stdout.split())) if da_peca(id_, n, True)]
        arquivos.append(os.path.join("assets", "gear", "indice.json"))
        subprocess.run(["git", "-C", RAIZ, "add", "-A", "--", *arquivos], check=True)
        subprocess.run(["git", "-C", RAIZ, "commit", "-q", "-m",
                        f"Equipamento {f['nome']} ({id_} {f['versao']}): arte, ficha e índice para os Nexus instalados baixarem"],
                       check=True)
        print("commit gravado (falta enviar)")


if __name__ == "__main__":
    main()
