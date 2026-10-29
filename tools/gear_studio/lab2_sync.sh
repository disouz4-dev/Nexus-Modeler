#!/bin/bash
# Manda o gerador, as receitas e as fontes desta pasta para ~/nexus-gear/<destino>
# no lab2 (padrão: repo-fiel), onde a fila 3D renderiza os pedidos feitos com
# --repo ~/nexus-gear/<destino>.
#   tools/gear_studio/lab2_sync.sh [destino]
set -e
RAIZ=$(cd "$(dirname "$0")/../.." && pwd)
D=${1:-repo-fiel}
ssh lab2 "mkdir -p ~/nexus-gear/$D/assets"
rsync -a --delete --exclude __pycache__ "$RAIZ/tools/" "lab2:nexus-gear/$D/tools/"
rsync -a --delete "$RAIZ/assets/fonts/" "lab2:nexus-gear/$D/assets/fonts/"
scp -q "$RAIZ/tools/gear_studio/fila3d.sh" lab2:nexus-gear/fila3d.sh
echo "lab2 atualizado: $D"
