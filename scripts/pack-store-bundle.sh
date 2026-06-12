#!/usr/bin/env bash
set -euo pipefail

repo_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
name="moana-lemon-apocalypse"
out="$repo_dir/dist/$name-store-bundle.zip"

mkdir -p "$repo_dir/dist"
cd "$repo_dir"
zip -qr "$out" \
  metadata/manifest.json \
  metadata/metadata.json \
  metadata/colophon.json \
  assets/icon.png \
  assets/screenshot.png \
  dist/*.prg32

echo "$out"
