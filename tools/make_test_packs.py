#!/usr/bin/env python3
"""Gera pacotes de captura de teste a partir do modelo A2 de exemplo do NAM.

Ninguém publica um pacote de ganho livre que possa entrar no repositório, então
os testes usam pacotes sintéticos feitos a partir do A2.nam do próprio motor do
NAM (licença MIT):

  ganho/   11 capturas "Ganho 0" a "Ganho 10": o A2 com um ganho de entrada de
           -15 dB a +15 dB na frente (modelo Sequential = Linear + A2). Soa como
           um botão de ganho de verdade: mais distorção e mais volume.
  iguais/  3 cópias idênticas do A2 nas posições 0, 5 e 10. Como as capturas
           são iguais, a saída com o botão andando tem de ser igual à de uma
           captura parada: qualquer diferença é erro do motor de troca.
  fullrig/ 3 capturas marcadas no próprio .nam como cabeçote + caixa
           ("gear_type": "amp_cab"), sem pack.json: o Nexus tem de perceber
           sozinho e trancar a caixa.
  ir_teste.wav  resposta de caixa sintética (ruído com queda e filtro), para
           testar o bloco da caixa.

Uso: make_test_packs.py <A2.nam> <pasta_de_saida>
"""

import json
import math
import random
import shutil
import struct
import sys
import wave
from pathlib import Path


def write_test_ir(path: Path, sample_rate: int) -> None:
    """IR de 50 ms: ruído que cai depressa, passado por um filtro passa-baixas
    simples. Soa como uma caixa abafada; serve só para o teste mudar o som."""
    rng = random.Random(7)
    n = int(0.05 * sample_rate)
    smooth = 0.0
    frames = bytearray()
    for i in range(n):
        x = rng.uniform(-1.0, 1.0) * math.exp(-i / (0.006 * sample_rate))
        smooth += 0.25 * (x - smooth)
        frames += struct.pack("<h", int(max(-1.0, min(1.0, smooth * 3.0)) * 32767))
    with wave.open(str(path), "wb") as w:
        w.setnchannels(1)
        w.setsampwidth(2)
        w.setframerate(sample_rate)
        w.writeframes(bytes(frames))


def main() -> int:
    if len(sys.argv) != 3:
        print(__doc__)
        return 2
    source = Path(sys.argv[1])
    out = Path(sys.argv[2])
    a2 = json.loads(source.read_text())
    sample_rate = a2.get("sample_rate", 48000.0)

    gain_dir = out / "ganho"
    gain_dir.mkdir(parents=True, exist_ok=True)
    for position in range(11):
        gain_db = -15.0 + 3.0 * position
        linear = {
            "version": "0.7.0",
            "architecture": "Linear",
            "config": {"receptive_field": 1, "bias": False},
            "weights": [10.0 ** (gain_db / 20.0)],
            "sample_rate": sample_rate,
        }
        model = {
            "version": "0.7.0",
            "architecture": "Sequential",
            "config": {"models": [linear, a2]},
            "weights": [],
            "sample_rate": sample_rate,
            "metadata": {"name": f"Teste Ganho {position}"},
        }
        (gain_dir / f"Ganho {position}.nam").write_text(json.dumps(model))

    manifest = {
        "nome": "Pacote de teste (ganho)",
        "botao": "Ganho",
        "circuito": "marshall",
        "equalizador_da_captura": {"grave": 5, "medio": 5, "agudo": 5, "presenca": 5},
        "capturas": {str(p): f"Ganho {p}.nam" for p in range(11)},
    }
    (gain_dir / "pack.json").write_text(json.dumps(manifest, ensure_ascii=False, indent=2))

    same_dir = out / "iguais"
    same_dir.mkdir(parents=True, exist_ok=True)
    for position in (0, 5, 10):
        shutil.copyfile(source, same_dir / f"Ganho {position}.nam")

    rig_dir = out / "fullrig"
    rig_dir.mkdir(parents=True, exist_ok=True)
    for position in (0, 5, 10):
        rig = dict(a2)
        rig["metadata"] = dict(a2.get("metadata") or {}, gear_type="amp_cab", name=f"Rig {position}")
        (rig_dir / f"Rig Gain {position}.nam").write_text(json.dumps(rig))

    write_test_ir(out / "ir_teste.wav", int(sample_rate))
    print(f"Pacotes de teste em {out}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
