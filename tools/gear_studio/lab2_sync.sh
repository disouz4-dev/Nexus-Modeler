#!/bin/bash
# Manda o gerador, as receitas e as fontes deste ramo para ~/nexus-gear/repo-fiel
# no lab2, onde a fila 3D renderiza os pedidos feitos com --repo.
#   tools/gear_studio/lab2_sync.sh
set -e
RAIZ=$(cd "$(dirname "$0")/../.." && pwd)
ssh lab2 'mkdir -p ~/nexus-gear/repo-fiel/assets'
rsync -a --delete --exclude __pycache__ "$RAIZ/tools/" lab2:nexus-gear/repo-fiel/tools/
rsync -a --delete "$RAIZ/assets/fonts/" lab2:nexus-gear/repo-fiel/assets/fonts/
scp -q "$RAIZ/tools/gear_studio/fila3d.sh" lab2:nexus-gear/fila3d.sh
echo "lab2 atualizado: repo-fiel"
