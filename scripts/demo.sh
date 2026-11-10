#!/usr/bin/env bash
set -euo pipefail
root="$(cd "$(dirname "$0")/.." && pwd)"
work="$root/build/demo"
mkdir -p "$work/output" "$work/logs"
"${CXX:-c++}" -std=c++17 -O2 "$root/standalone/gauss_with_clusters.cpp" -o "$work/terrain_demo"
printf 'WIDTH=64\nHEIGHT=64\n' > "$work/demo_config.txt"
printf 'GAUSS 1 32 32 6 6 0\nSCAN\nBMP_WRITE demo.bmp\n' > "$work/demo_commands.txt"
(cd "$work" && ./terrain_demo demo_commands.txt --config demo_config.txt)
test -s "$work/output/demo.bmp"
printf 'Generated native demo image: %s\n' "$work/output/demo.bmp"
