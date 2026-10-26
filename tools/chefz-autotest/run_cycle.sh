#!/bin/sh
# Full ChefZ autotest via Ninjins Environment Hub (pack + DayZDiag 1.30 launch from the hub folder).
# usage: TMPD=<scratch dir> sh tools/chefz-autotest/run_cycle.sh [ChefZ_Mod ...]   (no args = pack all)
# Logs: E:/DayZ Modding/TestServer/profiles/server-130 and client-130. Remove ChefZ_AutoTest.pbo from
# TestServer/target/@Client and @Server afterwards.
: "${TMPD:=$(mktemp -d)}"
taskkill //IM DayZDiag_x64.exe //F >/dev/null 2>&1
sleep 3
ROOT="E:/DayZ Modding/Psyerns_ChefZ"
SRC="E:/DayZ Modding/TestServer/src"
for d in "$ROOT"/ChefZ_Core/Addons/*/; do
  n=$(basename "$d")
  robocopy "$d" "$SRC/$n" //MIR //NFL //NDL //NJH //NJS //NP >/dev/null
done
robocopy "$ROOT/tools/chefz-autotest/ChefZ_AutoTest" "$SRC/ChefZ_AutoTest" //MIR //NFL //NDL //NJH //NJS //NP >/dev/null
HUB="E:/DayZ Modding/Ninjins_Environment_Hub"
cd "$HUB" && echo "hub cwd: $(pwd)" && py -3 "$ROOT/tools/chefz-assets/hub-pack-test.py" "$@" > "$TMPD/pack.log" 2>&1
if grep -qi "build failed" "$TMPD/pack.log"; then grep -i "fail" "$TMPD/pack.log"; exit 1; fi
echo "packed"
cd "$HUB" && echo "hub cwd: $(pwd)" && timeout 300 py -3 "E:/DayZ Modding/TestServer/tools/hub_launch_diag.py" --preset "ChefZ Test 1.30" --mode both --mission chefzauto.chernarusplus > "$TMPD/launch.log" 2>&1
echo "launched"
cd "E:/DayZ Modding/TestServer/profiles/server-130"
for i in $(seq 1 400); do
  f=$(ls -t script_*.log 2>/dev/null | head -1)
  if grep -q -e "CHEFZTEST. done" -e "compile" "$f" 2>/dev/null; then break; fi
  if ! tasklist | grep -qi DayZDiag; then echo "no dayz"; break; fi
  sleep 10
done
sleep 5
echo "$f"
grep -c "CHEFZTEST" "$f"
grep -e "CHEFZTEST. done" -e "CHEFZTEST. SUMMARY" -e "CHEFZTEST. INFO start" -e "CHEFZTEST. INFO player" "$f"
grep -m4 -e "SCRIPT    (E)" "$f"
