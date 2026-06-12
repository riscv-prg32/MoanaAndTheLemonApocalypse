#!/usr/bin/env bash
set -euo pipefail

repo_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
prg32_repo="${PRG32_REPO:-"$repo_dir/../PRG32"}"
firmware_elf="${1:-"${PRG32_FIRMWARE_ELF:-"$prg32_repo/build/PRG32.elf"}"}"
architecture="${PRG32_ARCHITECTURE:-esp32c6}"
portable="${PRG32_PORTABLE:-1}"
game_tool="$prg32_repo/tools/prg32_game.py"
name="moana-lemon-apocalypse"

if [[ -f "$prg32_repo/components/prg32/include/prg32_abi_hash.h" ]] &&
   ! grep -q 'PRG32_ABI_MINOR 1u' "$prg32_repo/components/prg32/include/prg32_abi_hash.h"; then
  cat >&2 <<EOF
warning: this game uses 24x24 sprite assets designed for PRG32 ABI 1.1.
Use PRG32 branch dev-sprite24x24-1 for firmware builds:
  git -C "$prg32_repo" checkout dev-sprite24x24-1
EOF
fi

build_args=()
if [[ "$portable" == "1" || "$portable" == "true" || "$portable" == "yes" ]]; then
  if ! python3 "$game_tool" build --help | grep -q -- '--portable'; then
    echo "error: $game_tool does not support portable cartridge builds." >&2
    exit 2
  fi
  build_args+=(--portable)
else
  build_args+=(--firmware-elf "$firmware_elf" --legacy-absolute-imports)
fi

mkdir -p "$repo_dir/dist"

python3 "$game_tool" build \
  "$repo_dir/c/game.c" \
  --entry-prefix moana_lemon_c \
  --name "$name" \
  --out "$repo_dir/dist/$name-$architecture.raw.prg32" \
  "${build_args[@]}"

python3 "$game_tool" attach-metadata \
  "$repo_dir/dist/$name-$architecture.raw.prg32" \
  --out "$repo_dir/dist/$name-$architecture.prg32" \
  --metadata "$repo_dir/metadata/metadata.json" \
  --icon "$repo_dir/assets/icon.png" \
  --screenshot "$repo_dir/assets/screenshot.png" \
  --colophon "$repo_dir/metadata/colophon.json" \
  --architecture "$architecture"

echo "$repo_dir/dist/$name-$architecture.prg32"
