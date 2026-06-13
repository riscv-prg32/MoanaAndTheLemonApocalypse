# Build and Publish

This repository builds **Moana and the Lemon Apocalypse** as a normal
uploadable PRG32 cartridge. The portable build uses the C cartridge entry
prefix `moana_lemon_c`.

## Prerequisites

- A local checkout of `riscv-prg32/PRG32`.
- A PRG32 checkout from the portable ABI-table tooling branch or newer.
- For this game, use PRG32 firmware with 24x24 sprite ABI support, for example
  branch `dev-sprite24x24-1`.
- The RISC-V toolchain used by `PRG32/tools/prg32_game.py`.
- A running Cartridge Store instance when publishing.

Set `PRG32_REPO` if the PRG32 repository is not next to this repository:

```bash
export PRG32_REPO=/path/to/PRG32
```

## Regenerate Assets

The PNG and WAV assets are generated and committed. Regenerate them after
editing `assets/generate_assets.py`:

```bash
python3 assets/generate_assets.py
```

This rewrites `assets/png`, `assets/wav`, and `assets/manifest.json`.

## Build for ESP32-C6 Hardware

```bash
export PRG32_REPO=/path/to/PRG32
export PRG32_ARCHITECTURE=esp32c6
scripts/build.sh
```

The script writes:

```text
dist/moana-lemon-apocalypse-esp32c6.prg32
```

By default this is a portable ABI-table cartridge and is not tied to one
firmware ELF.

## Build for QEMU

```bash
export PRG32_REPO=/path/to/PRG32
export PRG32_ARCHITECTURE=qemu
scripts/build.sh
```

The script writes:

```text
dist/moana-lemon-apocalypse-qemu.prg32
```

## Build the Legacy Absolute-Import Format

Use this only for firmware images that do not yet support portable ABI-table
cartridges:

```bash
export PRG32_PORTABLE=0
export PRG32_ARCHITECTURE=esp32c6
scripts/build.sh "$PRG32_REPO/build/PRG32.elf"
```

The legacy cartridge is tied to the firmware ELF passed to the script.

## Upload to a Board

```bash
python3 "$PRG32_REPO/tools/prg32_game.py" upload \
  dist/moana-lemon-apocalypse-esp32c6.prg32 \
  --url http://192.168.4.1
```

Use the board address shown in PRG32 setup mode when the board is on classroom
Wi-Fi.

## Stage in QEMU

```bash
python3 "$PRG32_REPO/tools/prg32_game.py" upload-qemu \
  dist/moana-lemon-apocalypse-qemu.prg32 \
  --flash "$PRG32_REPO/build-qemu/flash_image.bin" \
  --partitions "$PRG32_REPO/partitions_prg32.csv"
```

## Pack a Cartridge Store Bundle

Build both architecture variants first, then pack the flat Store bundle:

```bash
export PRG32_REPO=/path/to/PRG32

export PRG32_ARCHITECTURE=esp32c6
scripts/build.sh

export PRG32_ARCHITECTURE=qemu
scripts/build.sh

scripts/pack-store-bundle.sh
```

The bundle is:

```text
dist/moana-lemon-apocalypse-store-bundle.zip
```

The bundle contains Store metadata, icon, screenshot, colophon, and every
`.prg32` cartridge currently in `dist`.

## Publish to Cartridge Store

```bash
python3 "$PRG32_REPO/tools/prg32_game.py" publish-bundle \
  dist/moana-lemon-apocalypse-store-bundle.zip \
  --store-url http://192.168.1.42:5080 \
  --token "$PRG32_STORE_TOKEN"
```

The store URL and token may also be kept in `~/.prg32/config.json`. Keep tokens
out of Git, screenshots, and lab reports.

## Release Checklist

- `metadata/metadata.json` describes the current game.
- `metadata/manifest.json` lists both `esp32c6` and `qemu` cartridges.
- `metadata/colophon.json` credits code, assets, and acknowledgements.
- `assets/icon.png` and `assets/screenshot.png` match the current version.
- Both architecture builds exist in `dist`.
- The Store bundle installs and starts on at least one target.
