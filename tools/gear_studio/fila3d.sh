#!/bin/bash
# Fila única da modelagem 3D do Nexus (Blender no lab2).
#
#   fila3d.sh pedir [--largura N] [--de "quem pediu"] [--nota "texto"] receita...
#   fila3d.sh ver            o que está rodando, o que espera e o que ficou pronto
#   fila3d.sh historico [N]  as últimas N peças prontas (padrão 40)
#   fila3d.sh tirar receita  tira da fila uma peça que ainda não começou
#   fila3d.sh retomar        sobe o trabalhador (o cron faz isso sozinho ao ligar o lab2)
#   pedir --repo DIR         usa o gerador e as receitas de outra cópia do repositório
#
# A receita é o nome em repo/tools/gear_studio/recipes/<nome>.json ou um
# caminho para um .json. Uma peça por vez; o trabalhador sobe sozinho ao pedir.
set -u
BASE=${NX3D_BASE:-$HOME/nexus-gear}
DIR=$BASE/fila3d
PEDIDOS=$DIR/pedidos.tsv   # id, pedido, receita, largura, de, estado, inicio, fim, nota, json, repo
TRAVA=$DIR/.trava
mkdir -p "$DIR/logs"
touch "$PEDIDOS"

agora() { date '+%d/%m %H:%M'; }

# Reescreve o campo $2 (1 = id) da linha com id $1 para o valor $3.
muda() {
  ( flock 9
    awk -F'\t' -v OFS='\t' -v id="$1" -v c="$2" -v v="$3" '$1==id{$c=v}1' "$PEDIDOS" >"$PEDIDOS.tmp" && mv "$PEDIDOS.tmp" "$PEDIDOS"
  ) 9>"$TRAVA"
}

trabalhador() {
  exec 8>"$DIR/.trabalhador"
  flock -n 8 || exit 0
  # Retomada: o que estava rodando quando o trabalhador caiu (luz, reinício) volta para a fila.
  ( flock 9
    awk -F'\t' -v OFS='\t' '$6=="rodando"{$6="na fila"; $7=""}1' "$PEDIDOS" >"$PEDIDOS.tmp" && mv "$PEDIDOS.tmp" "$PEDIDOS"
  ) 9>"$TRAVA"
  while :; do
    linha=$(awk -F'\t' '$6=="na fila"{print; exit}' "$PEDIDOS")
    [ -z "$linha" ] && break
    # A placa é uma só: espera se já houver dois Blenders abertos por fora.
    while [ "$(pgrep -fc 'blender/blender -b')" -ge 2 ]; do sleep 20; done
    # cut, e não read: tab conta como espaço no read e colunas vazias somem.
    id=$(cut -f1 <<<"$linha"); r=$(cut -f3 <<<"$linha"); larg=$(cut -f4 <<<"$linha")
    json=$(cut -f10 <<<"$linha"); repo=$(cut -f11 <<<"$linha")
    repo=${repo:-$BASE/repo}
    muda "$id" 6 rodando; muda "$id" 7 "$(agora)"
    log=$DIR/logs/$r.log
    cd "$repo"
    rm -rf "$BASE/saida/$r"
    nice -n 5 "$BASE/blender/blender" -b --factory-startup -P tools/gear_studio/build.py -- \
      --receita "$json" --saida "$BASE/saida/$r" --largura "$larg" >"$log" 2>&1
    if grep -q '\[gear\] pronto' "$log" && [ -f "$BASE/saida/$r/mapa.json" ]; then
      muda "$id" 6 pronta
    else
      muda "$id" 6 erro
    fi
    muda "$id" 8 "$(agora)"
  done
}

pedir() {
  local larg=2400 de="?" nota="" n=0 repo=$BASE/repo
  while [ $# -gt 0 ]; do
    case "$1" in
      --largura) larg=$2; shift 2 ;;
      --de) de=$2; shift 2 ;;
      --nota) nota=$2; shift 2 ;;
      --repo) repo=$2; shift 2 ;;
      *) break ;;
    esac
  done
  for r in "$@"; do
    local json=$r nome
    [[ $r == *.json ]] || json=$repo/tools/gear_studio/recipes/$r.json
    nome=$(basename "$json" .json)
    if [ ! -f "$json" ]; then echo "sem receita: $json (mande a receita para o lab2 antes)"; continue; fi
    if awk -F'\t' -v r="$nome" '$3==r && ($6=="na fila"||$6=="rodando"){f=1} END{exit !f}' "$PEDIDOS"; then
      echo "$nome já está na fila"; continue
    fi
    ( flock 9
      id=$(( $(cut -f1 "$PEDIDOS" | sort -n | tail -1 | grep -o '[0-9]*' || echo 0) + 1 ))
      printf '%s\t%s\t%s\t%s\t%s\tna fila\t\t\t%s\t%s\t%s\n' "$id" "$(agora)" "$nome" "$larg" "$de" "$nota" "$json" "$repo" >>"$PEDIDOS"
    ) 9>"$TRAVA"
    echo "na fila: $nome (largura $larg, pedido por $de)"; n=$((n+1))
  done
  [ $n -gt 0 ] && nohup "$0" _trabalhador >/dev/null 2>&1 &
}

ver() {
  echo "== RODANDO AGORA"
  awk -F'\t' '$6=="rodando"{printf "  %-22s desde %s  (%s)\n", $3, $7, $5}' "$PEDIDOS"
  # Blender aberto por fora da fila (scripts antigos fila-*.sh etc.).
  pgrep -af 'blender -b' | grep -v fila3d | while read -r _ args; do
    r=$(grep -o 'saida/[^ ]*' <<<"$args" | head -1 | cut -d/ -f2)
    [ -n "$r" ] || continue
    awk -F'\t' -v r="$r" '$3==r && $6=="rodando"{f=1} END{exit !f}' "$PEDIDOS" || echo "  $r  (fora da fila única)"
  done
  echo
  echo "== NA FILA"
  awk -F'\t' '$6=="na fila"{n++; printf "  %2d. %-22s pedido %s  (%s)%s\n", n, $3, $2, $5, ($9!=""?"  — "$9:"")} END{if(!n) print "  (vazia)"}' "$PEDIDOS"
  # Filas antigas: o que ainda falta em cada fila-*.sh vivo.
  local vistos=""
  for pid in $(pgrep -f 'fila-[a-z]*\.sh|fila[0-9]*\.sh|depois\.sh'); do
    args=$(tr '\0' ' ' </proc/$pid/cmdline 2>/dev/null)
    log=$(readlink /proc/$pid/fd/1 2>/dev/null)
    [ -f "$log" ] || continue
    [[ " $vistos " == *" $log "* ]] && continue
    vistos+="$log "
    feitas=$(grep -o 'terminou [^ ]*' "$log" 2>/dev/null | cut -d' ' -f2 | tr '\n' ' ')
    falta=""
    for r in $args; do
      [[ $r == *.sh || $r == /bin/bash || $r =~ ^[0-9]+$ || $r == *.log ]] && continue
      [[ " $feitas " == *" $r "* ]] || falta+="$r "
    done
    [ -n "$falta" ] && echo "  fila antiga $(basename "$log"): $falta"
  done
  echo
  echo "== COM ERRO"
  awk -F'\t' '$6=="erro"{n++; printf "  %-22s %s  log: fila3d/logs/%s.log\n", $3, $8, $3} END{if(!n) print "  (nenhuma)"}' "$PEDIDOS"
  echo
  echo "== PRONTAS POR ÚLTIMO"
  historico 15
}

# Tudo o que já saiu do Blender (tem mapa.json), pela hora em que terminou.
historico() {
  local n=${1:-40} rodando
  rodando=$(pgrep -af 'blender -b' | grep -o 'saida/[^ ]*' | cut -d/ -f2 | tr '\n' '|')
  ls -t --time-style='+%d/%m %H:%M' -l "$BASE"/saida/*/mapa.json 2>/dev/null |
    awk '{split($8, c, "/"); print "  "$6" "$7"  "c[length(c)-1]}' |
    grep -vwE "${rodando}^$" | head -"$n"
}

tirar() {
  for r in "$@"; do
    ( flock 9
      awk -F'\t' -v OFS='\t' -v r="$r" '$3==r && $6=="na fila"{$6="tirada"}1' "$PEDIDOS" >"$PEDIDOS.tmp" && mv "$PEDIDOS.tmp" "$PEDIDOS"
    ) 9>"$TRAVA"
    echo "tirada: $r"
  done
}

cmd=${1:-ver}; shift || true
case "$cmd" in
  pedir) pedir "$@" ;;
  ver) ver ;;
  historico) historico "$@" ;;
  tirar) tirar "$@" ;;
  _trabalhador) trabalhador ;;
  retomar) nohup "$0" _trabalhador >/dev/null 2>&1 & echo "trabalhador iniciado" ;;
  *) sed -n '2,10p' "$0" ;;
esac
