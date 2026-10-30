#!/bin/bash
# Gera o instalador do Nexus Modeler para Mac: um .pkg (instalador padrão da
# Apple) com componentes escolhíveis, dentro de um .dmg (imagem de disco).
#
# Uso (na raiz do repositório, depois de compilar em Release):
#   packaging/macos/criar_dmg.sh [pasta_dos_artefatos] [pasta_de_saida]
#
# Padrões: build/plugin/NexusModeler_artefacts/Release e build/pacotes.
# A versão vem de NEXUS_VERSAO ou, se não houver, do project() no CMakeLists.txt.
#
# Componentes do .pkg (a pessoa escolhe em "Tipo de instalação"):
#   VST3   /Library/Audio/Plug-Ins/VST3/Nexus Modeler.vst3
#   AU     /Library/Audio/Plug-Ins/Components/Nexus Modeler.component
#   App    /Applications/Nexus Modeler.app
#   Idioma (escondido, sem arquivos): grava o idioma do macOS nas preferências
#          do Nexus do usuário, só se ainda não houver escolha. O globo na tela
#          do Nexus troca depois.
#
# Assinatura e notarização (autenticação da Apple): ainda não há certificado.
# Quando houver, defina as variáveis abaixo e descomente os trechos marcados
# com "ASSINATURA" e "NOTARIZACAO":
#   NEXUS_ASSINATURA_APP        "Developer ID Application: Nome (EQUIPE)"
#   NEXUS_ASSINATURA_INSTALADOR "Developer ID Installer: Nome (EQUIPE)"
#   NEXUS_NOTARIZACAO_PERFIL    perfil salvo com "xcrun notarytool store-credentials"

set -euo pipefail

aqui="$(cd "$(dirname "$0")" && pwd)"
raiz="$(cd "$aqui/../.." && pwd)"
artefatos="${1:-$raiz/build/plugin/NexusModeler_artefacts/Release}"
saida="${2:-$raiz/build/pacotes}"

if [ -z "${NEXUS_VERSAO:-}" ]; then
  NEXUS_VERSAO=$(sed -n 's/^project(NexusModeler VERSION \([0-9.]*\).*/\1/p' "$raiz/CMakeLists.txt")
fi
versao="$NEXUS_VERSAO"
id_base="dev.disouz4.nexusmodeler"

vst3="$artefatos/VST3/Nexus Modeler.vst3"
au="$artefatos/AU/Nexus Modeler.component"
app="$artefatos/Standalone/Nexus Modeler.app"
for item in "$vst3" "$au" "$app"; do
  if [ ! -d "$item" ]; then
    echo "Não achei: $item" >&2
    echo "Compile antes: cmake --build build --config Release" >&2
    exit 1
  fi
done

# Arquiteturas do binário (Intel, Apple Silicon ou as duas), para o instalador
# recusar um Mac em que o plugin não roda.
arquiteturas=$(lipo -archs "$app/Contents/MacOS/Nexus Modeler" | tr ' ' ',')

trabalho="$(mktemp -d -t nexus-pacote)"
trap 'rm -rf "$trabalho"' EXIT
mkdir -p "$saida"

echo "Nexus Modeler $versao ($arquiteturas)"

# --- ASSINATURA: assinar os pacotes de código antes de empacotar -------------
# if [ -n "${NEXUS_ASSINATURA_APP:-}" ]; then
#   for item in "$vst3" "$au" "$app"; do
#     codesign --force --deep --options runtime --timestamp \
#       --sign "$NEXUS_ASSINATURA_APP" "$item"
#   done
# fi

# Copia um item para uma raiz própria e gera o .pkg do componente.
#   $1 nome curto   $2 origem   $3 pasta de destino no Mac   $4 pasta de scripts (opcional)
componente() {
  local nome="$1" origem="$2" destino="$3" scripts="${4:-}"
  local raiz_pkg="$trabalho/raiz_$nome"
  mkdir -p "$raiz_pkg"
  ditto --noextattr --noqtn "$origem" "$raiz_pkg/$(basename "$origem")"

  # Pacotes de código não "mudam de lugar": sem isso, o instalador da Apple
  # atualizaria uma cópia antiga achada em outra pasta em vez de instalar aqui.
  local lista="$trabalho/$nome.plist"
  pkgbuild --analyze --root "$raiz_pkg" "$lista" >/dev/null
  local i=0
  while /usr/libexec/PlistBuddy -c "Print :$i" "$lista" >/dev/null 2>&1; do
    /usr/libexec/PlistBuddy -c "Delete :$i:BundleIsRelocatable" "$lista" >/dev/null 2>&1 || true
    /usr/libexec/PlistBuddy -c "Add :$i:BundleIsRelocatable bool false" "$lista"
    i=$((i + 1))
  done

  local extras=()
  [ -n "$scripts" ] && extras+=(--scripts "$scripts")
  pkgbuild --root "$raiz_pkg" \
    --component-plist "$lista" \
    --install-location "$destino" \
    --identifier "$id_base.$nome" \
    --version "$versao" \
    ${extras[@]+"${extras[@]}"} \
    "$trabalho/pacotes/$nome.pkg" >/dev/null
  echo "  componente $nome -> $destino"
}

mkdir -p "$trabalho/pacotes"
componente vst3 "$vst3" "/Library/Audio/Plug-Ins/VST3" "$aqui/scripts/vst3"
componente au "$au" "/Library/Audio/Plug-Ins/Components" "$aqui/scripts/au"
componente app "$app" "/Applications" "$aqui/scripts/app"

# Componente sem arquivos: só o script que grava o idioma.
pkgbuild --nopayload \
  --scripts "$aqui/scripts/idioma" \
  --identifier "$id_base.idioma" \
  --version "$versao" \
  "$trabalho/pacotes/idioma.pkg" >/dev/null
echo "  componente idioma (script)"

# Textos do instalador (português e inglês) e a licença GPL v3.
recursos="$trabalho/recursos"
ditto "$aqui/recursos" "$recursos"
cp "$raiz/LICENSE" "$recursos/LICENSE.txt"

cat > "$trabalho/distribuicao.xml" <<EOF
<?xml version="1.0" encoding="utf-8"?>
<installer-gui-script minSpecVersion="2">
    <title>TITULO</title>
    <welcome file="bem-vindo.html" mime-type="text/html"/>
    <license file="LICENSE.txt" mime-type="text/plain"/>
    <options customize="always" require-scripts="false" rootVolumeOnly="true" hostArchitectures="$arquiteturas"/>
    <domains enable_anywhere="false" enable_currentUserHome="false" enable_localSystem="true"/>
    <allowed-os-versions>
        <os-version min="11.0"/>
    </allowed-os-versions>
    <choices-outline>
        <line choice="vst3"/>
        <line choice="au"/>
        <line choice="app"/>
        <line choice="idioma"/>
    </choices-outline>
    <choice id="vst3" title="VST3_TITULO" description="VST3_DESCRICAO">
        <pkg-ref id="$id_base.vst3"/>
    </choice>
    <choice id="au" title="AU_TITULO" description="AU_DESCRICAO">
        <pkg-ref id="$id_base.au"/>
    </choice>
    <choice id="app" title="APP_TITULO" description="APP_DESCRICAO">
        <pkg-ref id="$id_base.app"/>
    </choice>
    <choice id="idioma" title="IDIOMA_TITULO" description="IDIOMA_DESCRICAO" visible="false">
        <pkg-ref id="$id_base.idioma"/>
    </choice>
    <pkg-ref id="$id_base.vst3" version="$versao" onConclusion="none">vst3.pkg</pkg-ref>
    <pkg-ref id="$id_base.au" version="$versao" onConclusion="none">au.pkg</pkg-ref>
    <pkg-ref id="$id_base.app" version="$versao" onConclusion="none">app.pkg</pkg-ref>
    <pkg-ref id="$id_base.idioma" version="$versao" onConclusion="none">idioma.pkg</pkg-ref>
</installer-gui-script>
EOF

pkg_final="$trabalho/dmg/Instalar Nexus Modeler.pkg"
mkdir -p "$trabalho/dmg"
assinatura_pkg=()
# --- ASSINATURA: assinar o instalador -----------------------------------------
# [ -n "${NEXUS_ASSINATURA_INSTALADOR:-}" ] && assinatura_pkg=(--sign "$NEXUS_ASSINATURA_INSTALADOR" --timestamp)
productbuild \
  --distribution "$trabalho/distribuicao.xml" \
  --resources "$recursos" \
  --package-path "$trabalho/pacotes" \
  ${assinatura_pkg[@]+"${assinatura_pkg[@]}"} \
  "$pkg_final" >/dev/null
echo "  instalador: $(basename "$pkg_final")"

# --- NOTARIZACAO: enviar o instalador para a Apple e grampear o resultado -----
# if [ -n "${NEXUS_NOTARIZACAO_PERFIL:-}" ]; then
#   xcrun notarytool submit "$pkg_final" --keychain-profile "$NEXUS_NOTARIZACAO_PERFIL" --wait
#   xcrun stapler staple "$pkg_final"
# fi

# Conteúdo da imagem de disco: o instalador, a licença e um leia-me curto.
cp "$raiz/LICENSE" "$trabalho/dmg/Licença (GPL v3).txt"
cat > "$trabalho/dmg/Leia-me.txt" <<'EOF'
Nexus Modeler — plugin de guitarra gratuito e de código aberto (GPL v3)

1. Abra "Instalar Nexus Modeler.pkg".
   Enquanto o instalador não for assinado pela Apple, o Mac pode recusar na
   primeira vez: clique com o botão direito no arquivo, escolha "Abrir" e
   confirme. (Ou em Ajustes do Sistema > Privacidade e Segurança > "Abrir
   Mesmo Assim".)
2. Em "Tipo de instalação", escolha VST3, AU e/ou o programa independente.
3. A tela do Nexus abre no idioma do seu Mac. Para trocar, clique no globo.

Para desinstalar, apague:
  /Library/Audio/Plug-Ins/VST3/Nexus Modeler.vst3
  /Library/Audio/Plug-Ins/Components/Nexus Modeler.component
  /Applications/Nexus Modeler.app

https://github.com/disouz4-dev/Nexus-Modeler
EOF

dmg="$saida/NexusModeler-$versao-Mac.dmg"
rm -f "$dmg"

# Ícone do disco (o do Nexus, de assets/icone/nexus_1024.png) no formato .icns.
icone="$raiz/assets/icone/nexus_1024.png"
if [ -f "$icone" ]; then
  conjunto="$trabalho/nexus.iconset"
  mkdir -p "$conjunto"
  for t in 16 32 128 256 512; do
    sips -z "$t" "$t" "$icone" --out "$conjunto/icon_${t}x${t}.png" >/dev/null
    sips -z $((t * 2)) $((t * 2)) "$icone" --out "$conjunto/icon_${t}x${t}@2x.png" >/dev/null
  done
  iconutil -c icns "$conjunto" -o "$trabalho/dmg/.VolumeIcon.icns"
fi

# A imagem é montada uma vez em modo de escrita só para marcar o ícone do
# disco (SetFile, das ferramentas do Xcode); depois vira a versão compactada.
rascunho="$trabalho/rascunho.dmg"
hdiutil create -quiet -volname "Nexus Modeler $versao" -srcfolder "$trabalho/dmg" \
  -fs HFS+ -format UDRW -ov "$rascunho"
if [ -f "$trabalho/dmg/.VolumeIcon.icns" ] && command -v SetFile >/dev/null 2>&1; then
  montagem="$trabalho/montagem"
  mkdir -p "$montagem"
  hdiutil attach -quiet -nobrowse -noverify -noautoopen -mountpoint "$montagem" "$rascunho"
  SetFile -a C "$montagem"
  hdiutil detach -quiet "$montagem"
fi
hdiutil convert -quiet "$rascunho" -format UDZO -imagekey zlib-level=9 -ov -o "$dmg"
# --- ASSINATURA: assinar a imagem de disco ------------------------------------
# [ -n "${NEXUS_ASSINATURA_APP:-}" ] && codesign --sign "$NEXUS_ASSINATURA_APP" --timestamp "$dmg"
echo "Pronto: $dmg"

# Arquivo compactado para a instalação pelo terminal (packaging/instalar.sh):
# os três itens soltos, sem instalador.
zip_saida="$saida/NexusModeler-$versao-Mac.zip"
rm -f "$zip_saida"
mkdir -p "$trabalho/zip/Nexus Modeler"
ditto --noextattr --noqtn "$vst3" "$trabalho/zip/Nexus Modeler/Nexus Modeler.vst3"
ditto --noextattr --noqtn "$au" "$trabalho/zip/Nexus Modeler/Nexus Modeler.component"
ditto --noextattr --noqtn "$app" "$trabalho/zip/Nexus Modeler/Nexus Modeler.app"
cp "$raiz/LICENSE" "$trabalho/zip/Nexus Modeler/LICENSE.txt"
(cd "$trabalho/zip" && ditto -c -k --norsrc --noextattr --keepParent "Nexus Modeler" "$zip_saida")
echo "Pronto: $zip_saida"
