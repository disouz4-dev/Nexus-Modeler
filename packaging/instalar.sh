#!/bin/bash
# Instalação do Nexus Modeler pelo terminal (Linux e Mac), sem administrador.
#
# Uso rápido (baixa o último lançamento do GitHub e instala):
#   curl -fsSL https://raw.githubusercontent.com/disouz4-dev/Nexus-Modeler/main/packaging/instalar.sh | bash
#
# Com opções:
#   curl -fsSL .../instalar.sh | bash -s -- --idioma pt-BR
#
# Opções:
#   --idioma CÓDIGO   idioma da tela do Nexus, sem perguntar (pt-BR, en, es, fr,
#                     it, de, ja, zh, ko, hi, ar, bn, ru, id, tr)
#   --versao ETIQUETA instala um lançamento específico (por exemplo v0.2.0)
#   --arquivo CAMINHO instala de um arquivo já baixado (.tar.gz no Linux,
#                     .zip no Mac) em vez de baixar
#   --desinstalar     remove o que este script instalou
#   --ajuda           mostra esta ajuda
#
# Onde instala (só na pasta do usuário):
#   Linux: ~/.vst3, ~/.lv2, ~/.local/bin/nexus-modeler e o atalho no menu
#   Mac:   ~/Library/Audio/Plug-Ins/VST3, ~/Library/Audio/Plug-Ins/Components
#          e ~/Applications
#
# O idioma escolhido vai para o arquivo de preferências que o Nexus lê; depois,
# o globo na tela do Nexus troca quando quiser.

set -euo pipefail

REPOSITORIO="disouz4-dev/Nexus-Modeler"
CODIGOS=(pt-BR en es fr it de ja zh ko hi ar bn ru id tr)
NOMES=("Português (Brasil)" "English" "Español" "Français" "Italiano" "Deutsch" "日本語"
       "中文（简体）" "한국어" "हिन्दी" "العربية" "বাংলা" "Русский" "Bahasa Indonesia" "Türkçe")

idioma="${NEXUS_IDIOMA:-}"
versao=""
arquivo=""
desinstalar=0

ajuda() {
  cat <<'FIM'
Instalação do Nexus Modeler pelo terminal (Linux e Mac), sem administrador.
  --idioma CÓDIGO    idioma da tela: pt-BR en es fr it de ja zh ko hi ar bn ru id tr
  --versao ETIQUETA  lançamento específico (por exemplo v0.2.0); padrão: o último
  --arquivo CAMINHO  instala de um arquivo já baixado (.tar.gz no Linux, .zip no Mac)
  --desinstalar      remove o que este script instalou
  --ajuda            mostra esta ajuda
FIM
}
erro() { echo "Erro: $*" >&2; exit 1; }

while [ $# -gt 0 ]; do
  case "$1" in
    --idioma) idioma="${2:-}"; shift 2 ;;
    --idioma=*) idioma="${1#*=}"; shift ;;
    --versao) versao="${2:-}"; shift 2 ;;
    --versao=*) versao="${1#*=}"; shift ;;
    --arquivo) arquivo="${2:-}"; shift 2 ;;
    --arquivo=*) arquivo="${1#*=}"; shift ;;
    --desinstalar) desinstalar=1; shift ;;
    -h|--ajuda|--help) ajuda; exit 0 ;;
    *) erro "opção desconhecida: $1 (veja --ajuda)" ;;
  esac
done

sistema="$(uname -s)"
case "$sistema" in
  Linux)
    [ "$(uname -m)" = "x86_64" ] || erro "por enquanto o Nexus para Linux é só para processadores x86_64 (Intel e AMD)."
    VST3_DIR="$HOME/.vst3"
    LV2_DIR="$HOME/.lv2"
    BIN_DIR="$HOME/.local/bin"
    APLICATIVOS_DIR="${XDG_DATA_HOME:-$HOME/.local/share}/applications"
    ICONES_DIR="${XDG_DATA_HOME:-$HOME/.local/share}/icons/hicolor/1024x1024/apps"
    # Pasta usada pelo JUCE no Linux para os ajustes do Nexus (fica na pasta pessoal).
    PREFERENCIAS="$HOME/Nexus Modeler/preferencias.settings"
    SUFIXO="-Linux-x86_64.tar.gz"
    ;;
  Darwin)
    VST3_DIR="$HOME/Library/Audio/Plug-Ins/VST3"
    AU_DIR="$HOME/Library/Audio/Plug-Ins/Components"
    APP_DIR="$HOME/Applications"
    PREFERENCIAS="$HOME/Library/Application Support/Nexus Modeler/preferencias.settings"
    SUFIXO="-Mac.zip"
    ;;
  *) erro "sistema não reconhecido ($sistema). No Windows, use instalar.ps1." ;;
esac

# --- Desinstalação ------------------------------------------------------------
if [ "$desinstalar" = 1 ]; then
  if [ "$sistema" = Linux ]; then
    rm -rf "$VST3_DIR/Nexus Modeler.vst3" "$LV2_DIR/Nexus Modeler.lv2" "$BIN_DIR/nexus-modeler" \
      "$APLICATIVOS_DIR/dev.disouz4.nexusmodeler.desktop" "$ICONES_DIR/nexus-modeler.png"
  else
    rm -rf "$VST3_DIR/Nexus Modeler.vst3" "$AU_DIR/Nexus Modeler.component" "$APP_DIR/Nexus Modeler.app"
  fi
  echo "Nexus Modeler removido. As preferências ficaram em: $(dirname "$PREFERENCIAS")"
  exit 0
fi

# --- Idioma ------------------------------------------------------------------
idioma_do_sistema() {
  local texto=""
  if [ "$sistema" = Darwin ]; then
    texto=$(defaults read -g AppleLanguages 2>/dev/null | sed -n '2p' | tr -d ' ",')
  else
    texto="${LANGUAGE:-${LC_ALL:-${LC_MESSAGES:-${LANG:-}}}}"
    texto="${texto%%:*}"
  fi
  texto=$(printf '%s' "$texto" | tr '[:upper:]' '[:lower:]')
  case "$texto" in
    pt*) echo "pt-BR"; return ;;
  esac
  for codigo in "${CODIGOS[@]}"; do
    case "$texto" in
      "$codigo"|"$codigo"-*|"$codigo"_*|"$codigo".*) echo "$codigo"; return ;;
    esac
  done
  echo "en"
}

codigo_valido() {
  local c
  for c in "${CODIGOS[@]}"; do [ "$c" = "$1" ] && return 0; done
  return 1
}

if [ -n "$idioma" ]; then
  codigo_valido "$idioma" || erro "idioma desconhecido: $idioma (use um destes: ${CODIGOS[*]})"
else
  padrao="$(idioma_do_sistema)"
  # Pergunta pelo terminal, mesmo quando o script chega pelo "curl | bash".
  if [ -r /dev/tty ] && { : < /dev/tty; } 2>/dev/null; then
    echo "Idioma da tela do Nexus / Nexus screen language:"
    for i in "${!CODIGOS[@]}"; do
      marca=" "
      [ "${CODIGOS[$i]}" = "$padrao" ] && marca="*"
      printf ' %s %2d) %s (%s)\n' "$marca" $((i + 1)) "${NOMES[$i]}" "${CODIGOS[$i]}"
    done
    printf 'Número [Enter = %s]: ' "$padrao"
    resposta=""
    read -r resposta < /dev/tty || true
    if [ -z "$resposta" ]; then
      idioma="$padrao"
    elif [[ "$resposta" =~ ^[0-9]+$ ]] && [ "$resposta" -ge 1 ] && [ "$resposta" -le ${#CODIGOS[@]} ]; then
      idioma="${CODIGOS[$((resposta - 1))]}"
    elif codigo_valido "$resposta"; then
      idioma="$resposta"
    else
      echo "Resposta não reconhecida; usando $padrao."
      idioma="$padrao"
    fi
  else
    idioma="$padrao"
  fi
fi

# --- Download ----------------------------------------------------------------
temp="$(mktemp -d)"
trap 'rm -rf "$temp"' EXIT

if [ -z "$arquivo" ]; then
  if [ -n "$versao" ]; then
    api="https://api.github.com/repos/$REPOSITORIO/releases/tags/$versao"
  else
    api="https://api.github.com/repos/$REPOSITORIO/releases/latest"
  fi
  echo "Procurando o lançamento em github.com/$REPOSITORIO ..."
  url=$(curl -fsSL "$api" | grep -o '"browser_download_url": *"[^"]*"' | sed 's/.*"\(http[^"]*\)"$/\1/' \
    | grep -- "$SUFIXO\$" | head -n 1) || true
  [ -n "$url" ] || erro "não achei o arquivo *$SUFIXO no lançamento. Veja https://github.com/$REPOSITORIO/releases"
  echo "Baixando $(basename "$url") ..."
  arquivo="$temp/$(basename "$url")"
  curl -fL --progress-bar -o "$arquivo" "$url"
fi
[ -f "$arquivo" ] || erro "arquivo não encontrado: $arquivo"

mkdir -p "$temp/pacote"
if [ "$sistema" = Darwin ]; then
  ditto -x -k "$arquivo" "$temp/pacote"
else
  tar -xzf "$arquivo" -C "$temp/pacote"
fi
origem="$temp/pacote/Nexus Modeler"
[ -d "$origem" ] || erro "o arquivo não tem a pasta \"Nexus Modeler\" esperada."

# --- Instalação --------------------------------------------------------------
# Troca um item instalado pelo novo (apaga a cópia antiga antes).
colocar() {
  local de="$1" para_dir="$2"
  mkdir -p "$para_dir"
  rm -rf "${para_dir:?}/$(basename "$de")"
  cp -R "$de" "$para_dir/"
}

if [ "$sistema" = Linux ]; then
  colocar "$origem/Nexus Modeler.vst3" "$VST3_DIR"
  colocar "$origem/Nexus Modeler.lv2" "$LV2_DIR"
  mkdir -p "$BIN_DIR" "$APLICATIVOS_DIR" "$ICONES_DIR"
  install -m 755 "$origem/nexus-modeler" "$BIN_DIR/nexus-modeler"
  install -m 644 "$origem/nexus-modeler.png" "$ICONES_DIR/nexus-modeler.png"
  # Atalho com caminhos completos: funciona mesmo se ~/.local/bin não estiver no PATH.
  sed -e "s#^Exec=.*#Exec=\"$BIN_DIR/nexus-modeler\"#" \
      -e "s#^Icon=.*#Icon=$ICONES_DIR/nexus-modeler.png#" \
      "$origem/nexus-modeler.desktop" > "$APLICATIVOS_DIR/dev.disouz4.nexusmodeler.desktop"
  command -v update-desktop-database >/dev/null 2>&1 && update-desktop-database "$APLICATIVOS_DIR" >/dev/null 2>&1 || true
  echo "Instalado:"
  echo "  VST3      $VST3_DIR/Nexus Modeler.vst3"
  echo "  LV2       $LV2_DIR/Nexus Modeler.lv2"
  echo "  Programa  $BIN_DIR/nexus-modeler (e no menu de aplicativos)"
else
  colocar "$origem/Nexus Modeler.vst3" "$VST3_DIR"
  colocar "$origem/Nexus Modeler.component" "$AU_DIR"
  colocar "$origem/Nexus Modeler.app" "$APP_DIR"
  # Arquivos baixados pelo terminal não ganham a marca de quarentena, mas um
  # .zip baixado pelo navegador e passado em --arquivo pode ter.
  xattr -dr com.apple.quarantine "$VST3_DIR/Nexus Modeler.vst3" "$AU_DIR/Nexus Modeler.component" \
    "$APP_DIR/Nexus Modeler.app" 2>/dev/null || true
  # Atualiza a lista de Audio Units do macOS (Logic e GarageBand).
  killall -9 AudioComponentRegistrar >/dev/null 2>&1 || true
  echo "Instalado:"
  echo "  VST3      $VST3_DIR/Nexus Modeler.vst3"
  echo "  AU        $AU_DIR/Nexus Modeler.component"
  echo "  Programa  $APP_DIR/Nexus Modeler.app"
fi

# --- Preferência de idioma ---------------------------------------------------
# Arquivo de ajustes do JUCE (XML). Se já existir, troca só o idioma.
mkdir -p "$(dirname "$PREFERENCIAS")"
if [ -f "$PREFERENCIAS" ] && grep -q 'name="idioma"' "$PREFERENCIAS"; then
  sed -i.bak "s#<VALUE name=\"idioma\" val=\"[^\"]*\"/>#<VALUE name=\"idioma\" val=\"$idioma\"/>#" "$PREFERENCIAS"
  rm -f "$PREFERENCIAS.bak"
elif [ -f "$PREFERENCIAS" ] && grep -q '</PROPERTIES>' "$PREFERENCIAS"; then
  sed -i.bak "s#</PROPERTIES>#  <VALUE name=\"idioma\" val=\"$idioma\"/>\\
</PROPERTIES>#" "$PREFERENCIAS"
  rm -f "$PREFERENCIAS.bak"
else
  cat > "$PREFERENCIAS" <<EOF
<?xml version="1.0" encoding="UTF-8"?>

<PROPERTIES>
  <VALUE name="idioma" val="$idioma"/>
</PROPERTIES>
EOF
fi
echo "  Idioma    $idioma (troque depois pelo globo na tela do Nexus)"
