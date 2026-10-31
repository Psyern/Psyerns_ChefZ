#!/bin/sh
# Pruefdurchlauf mit vielen Fotos: packt ChefZ + COT_Comp + ChefZ_VisCheck, startet DayZDiag 1.30
# und fotografiert an jeder Logzeile "[VISCHECK] shot <name>" (vischeck_shots.ps1).
# usage: sh tools/chefz-autotest/vischeck_audit.sh <liste.txt> <ausgabeordner> <minuten> [nopack]
LIST="$1"; OUT="$2"; MINUTES="${3:-30}"; NOPACK="$4"
ROOT="E:/DayZ Modding/Psyerns_ChefZ"
TS="E:/DayZ Modding/TestServer"
SRC="$TS/src"
HUB="E:/DayZ Modding/Ninjins_Environment_Hub"
mkdir -p "$OUT"
taskkill //IM DayZDiag_x64.exe //F >/dev/null 2>&1
sleep 3
cp "$LIST" "$TS/profiles/server-130/chefz_vischeck.txt"
cp "$LIST" "$TS/profiles/client-130/chefz_vischeck.txt"
if [ -z "$NOPACK" ]; then
  for d in "$ROOT"/ChefZ_Core/Addons/*/; do
    robocopy "$d" "$SRC/$(basename "$d")" //MIR //NFL //NDL //NJH //NJS //NP >/dev/null
  done
  # Modelle liegen in ChefZ/ und muessen eigens in den Testbaum. Nur *.p3d: die Skripte
  # und Configs im Testbaum sind ein eigener Stand, und ChefZ_Core gehoert dem Skript-Addon.
  for n in ChefZ_Devices ChefZ_Food ChefZ_Items ChefZ_Plants ChefZ_Sealife; do
    d="$ROOT/ChefZ/$n/"
    robocopy "$d" "$SRC/$n" "*.p3d" //S //NFL //NDL //NJH //NJS //NP >/dev/null
  done
  robocopy "$ROOT/ChefZ_COT_Comp" "$SRC/ChefZ_COT_Comp" //MIR //NFL //NDL //NJH //NJS //NP >/dev/null
  rm -rf "$SRC/ChefZ_AutoTest"
  rm -f "$TS/target/@Client/addons/ChefZ_AutoTest.pbo" "$TS/target/@Server/addons/ChefZ_AutoTest.pbo"
  robocopy "$ROOT/tools/chefz-autotest/ChefZ_VisCheck" "$SRC/ChefZ_VisCheck" //MIR //NFL //NDL //NJH //NJS //NP >/dev/null
  cd "$HUB" && py -3 "$ROOT/tools/chefz-assets/hub-pack-test.py" > "$OUT/pack.log" 2>&1
  if grep -qi "build failed" "$OUT/pack.log"; then echo "PACK FAILED"; grep -i "fail" "$OUT/pack.log"; exit 1; fi
  echo "packed"
fi
powershell -NoProfile -ExecutionPolicy Bypass -File "$ROOT/tools/chefz-autotest/vischeck_shots.ps1" -Out "$OUT" -Minutes "$MINUTES" > "$OUT/shot.log" 2>&1 &
SHOT=$!
cd "$HUB" && timeout 120 py -3 "$TS/tools/hub_launch_diag.py" --preset "ChefZ Test 1.30" --mode both > "$OUT/launch.log" 2>&1
wait $SHOT
cat "$OUT/shot.log"
S=$(ls -t "$TS/profiles/server-130"/script_*.log | head -1)
C=$(ls -t "$TS/profiles/client-130"/script_*.log | head -1)
cp "$S" "$OUT/server.log"; cp "$C" "$OUT/client.log"
taskkill //IM DayZDiag_x64.exe //F >/dev/null 2>&1
rm -f "$TS/profiles/server-130/chefz_vischeck.txt" "$TS/profiles/client-130/chefz_vischeck.txt"
echo "done"
