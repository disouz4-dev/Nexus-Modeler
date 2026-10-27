#!/bin/bash
# Gera packaging/windows/nexus.ico (ícone do instalador do Windows) a partir de
# assets/icone/nexus_1024.png. Roda no Mac (usa o sips, que vem com o macOS).
# Rode de novo sempre que o ícone mudar e guarde o .ico no repositório.
#
# O .ico leva os tamanhos 16, 24, 32, 48, 64, 128 e 256, cada um como PNG
# dentro do arquivo (aceito pelo Windows Vista em diante).

set -euo pipefail
aqui="$(cd "$(dirname "$0")" && pwd)"
raiz="$(cd "$aqui/../.." && pwd)"
origem="$raiz/assets/icone/nexus_1024.png"
temp="$(mktemp -d)"
trap 'rm -rf "$temp"' EXIT

for t in 16 24 32 48 64 128 256; do
  sips -z "$t" "$t" "$origem" --out "$temp/$t.png" >/dev/null
done

python3 - "$temp" "$aqui/nexus.ico" <<'EOF'
import struct, sys, os
pasta, saida = sys.argv[1], sys.argv[2]
tamanhos = [16, 24, 32, 48, 64, 128, 256]
imagens = [open(os.path.join(pasta, f"{t}.png"), "rb").read() for t in tamanhos]
cabecalho = struct.pack("<HHH", 0, 1, len(imagens))
deslocamento = 6 + 16 * len(imagens)
entradas = b""
for t, dados in zip(tamanhos, imagens):
    lado = 0 if t >= 256 else t  # 0 significa 256 no formato .ico
    entradas += struct.pack("<BBBBHHII", lado, lado, 0, 0, 1, 32, len(dados), deslocamento)
    deslocamento += len(dados)
with open(saida, "wb") as f:
    f.write(cabecalho + entradas + b"".join(imagens))
print("Pronto:", saida)
EOF
