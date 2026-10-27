#!/bin/bash
# Gera o arquivo .tar.gz do Linux usado pela instalação pelo terminal
# (packaging/instalar.sh): os plugins e o programa soltos, sem instalador.
#
# Uso: packaging/linux/criar_tar.sh [pasta_dos_artefatos] [pasta_de_saida]
# Padrões: build/plugin/NexusModeler_artefacts/Release e build/pacotes.

set -euo pipefail
aqui="$(cd "$(dirname "$0")" && pwd)"
raiz="$(cd "$aqui/../.." && pwd)"
artefatos="${1:-$raiz/build/plugin/NexusModeler_artefacts/Release}"
saida="${2:-$raiz/build/pacotes}"
if [ -z "${NEXUS_VERSAO:-}" ]; then
  NEXUS_VERSAO=$(sed -n 's/^project(NexusModeler VERSION \([0-9.]*\).*/\1/p' "$raiz/CMakeLists.txt")
fi

temp="$(mktemp -d)"
trap 'rm -rf "$temp"' EXIT
pasta="$temp/Nexus Modeler"
mkdir -p "$pasta" "$saida"
cp -R "$artefatos/VST3/Nexus Modeler.vst3" "$pasta/"
cp -R "$artefatos/LV2/Nexus Modeler.lv2" "$pasta/"
cp "$artefatos/Standalone/Nexus Modeler" "$pasta/nexus-modeler"
cp "$aqui/nexus-modeler.desktop" "$pasta/"
cp "$raiz/assets/icone/nexus_1024.png" "$pasta/nexus-modeler.png"
cp "$raiz/LICENSE" "$pasta/LICENSE.txt"

arquivo="$saida/NexusModeler-$NEXUS_VERSAO-Linux-x86_64.tar.gz"
tar -C "$temp" -czf "$arquivo" "Nexus Modeler"
echo "Pronto: $arquivo"
