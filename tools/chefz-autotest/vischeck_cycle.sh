#!/bin/sh
# Sichtpruefung: packt ChefZ + ChefZ_VisCheck, startet DayZDiag 1.30 (Server+Client),
# legt die Klassen aus <liste> neben den Spieler und fotografiert Inventar und Welt.
# usage: sh tools/chefz-autotest/vischeck_cycle.sh <liste.txt> <ausgabeordner> [prefix] [nopack]
LIST="$1"; OUT="$2"; PREFIX="${3:-vis}"; NOPACK="$4"
ROOT="E:/DayZ Modding/Psyerns_ChefZ"
TS="E:/DayZ Modding/TestServer"
SRC="$TS/src"
HUB="E:/DayZ Modding/Ninjins_Environment_Hub"
taskkill //IM DayZDiag_x64.exe //F >/dev/null 2>&1
sleep 3
cp "$LIST" "$TS/profiles/server-130/chefz_vischeck.txt"
cp "$LIST" "$TS/profiles/client-130/chefz_vischeck.txt"
if [ -z "$NOPACK" ]; then
  for d in "$ROOT"/ChefZ_Core/Addons/*/; do
    robocopy "$d" "$SRC/$(basename "$d")" //MIR //NFL //NDL //NJH //NJS //NP >/dev/null
  done
  rm -rf "$SRC/ChefZ_AutoTest"
  rm -f "$TS/target/@Client/addons/ChefZ_AutoTest.pbo" "$TS/target/@Server/addons/ChefZ_AutoTest.pbo"
  robocopy "$ROOT/tools/chefz-autotest/ChefZ_VisCheck" "$SRC/ChefZ_VisCheck" //MIR //NFL //NDL //NJH //NJS //NP >/dev/null
  cd "$HUB" && py -3 "$ROOT/tools/chefz-assets/hub-pack-test.py" > "$OUT/pack.log" 2>&1
  if grep -qi "build failed" "$OUT/pack.log"; then echo "PACK FAILED"; grep -i "fail" "$OUT/pack.log"; exit 1; fi
  echo "packed"
fi
powershell -NoProfile -ExecutionPolicy Bypass -File "$ROOT/tools/chefz-autotest/vischeck_shot.ps1" -Out "$OUT" -Prefix "$PREFIX" > "$OUT/shot.log" 2>&1 &
SHOT=$!
cd "$HUB" && timeout 300 py -3 "$TS/tools/hub_launch_diag.py" --preset "ChefZ Test 1.30" --mode both > "$OUT/launch.log" 2>&1
wait $SHOT
cat "$OUT/shot.log"
sleep 5
S=$(ls -t "$TS/profiles/server-130"/script_*.log | head -1)
# Liste mit "report N": auf die Berichtszeile warten (hoechstens N+60 s)
R=$(grep -E "^report " "$LIST" | awk '{print $2}' | sort -n | tail -1)
if [ -n "$R" ]; then
  for i in $(seq 1 $(( (R + 60) / 5 ))); do
    if grep -q "VISCHECK. report" "$S"; then break; fi
    sleep 5
  done
fi
C=$(ls -t "$TS/profiles/client-130"/script_*.log | head -1)
echo "server: $S"; grep -e "VISCHECK" -e "Schleuder" -e "Job gestartet" "$S" | grep -v gespawnt | head -30
grep -c "VISCHECK. gespawnt" "$S"
grep -m8 -e "SCRIPT    (E)" -e "Can't find" -e "NULL pointer" "$S" "$C" | grep -v -i "PluginConfigDebugProfile\|pluginmanager\|pluginitemdiagnostic\|missionbase.c : 21\|JSON ERROR DME\|init.c : 97\|somemission"
taskkill //IM DayZDiag_x64.exe //F >/dev/null 2>&1
rm -f "$TS/profiles/server-130/chefz_vischeck.txt" "$TS/profiles/client-130/chefz_vischeck.txt"
echo "done"
