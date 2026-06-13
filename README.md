# Moana and the Lemon Apocalypse

**Moana and the Lemon Apocalypse** is an original PRG32 cartridge game. Moana is
a Neapolitan 6-year-old lemon collector racing through an Ischia-inspired lemon
grove before chickens and an angry rooster ruin the day.

The game is intentionally compact and readable: a single C cartridge implements
the PRG32 lifecycle, game state, input, spawning, collision, drawing, sound, and
Store scoring. That makes it useful both as a playable arcade cartridge and as a
first-year teaching example.

No copyrighted sprites, music, level art, or sounds from commercial games are
included. The committed PNG and WAV files are original generated assets, with an
AI-assisted Ischia splash source and short original beep-style melodies.

## Repository Layout

This is a **standalone third-party PRG32 game repository**. It is not expected
to live inside the PRG32 source tree.

```text
MoanaAndTheLemonApocalypse/
|-- README.md
|-- LICENSE
|-- c/
|   `-- game.c
|-- assets/
|   |-- icon.png
|   |-- screenshot.png
|   |-- generate_assets.py
|   |-- manifest.json
|   |-- png/
|   `-- wav/
|-- docs/
|   |-- build-and-publish.md
|   `-- tutorial-create-moana-lemon-apocalypse.md
|-- metadata/
|   |-- colophon.json
|   |-- manifest.json
|   `-- metadata.json
`-- scripts/
    |-- build.sh
    `-- pack-store-bundle.sh
```

## Game Design

Lemons fall from active trees in pseudo-random positions and expire after a
short time. Moana collects as many as possible before the screen timer runs out.
Each lemon type changes the state in a different way:

| Lemon | Effect |
|---|---|
| Normal | Adds score, fills Moana's basket, and advances the screen quota |
| Magic | Adds 30 seconds, adds score, and fills the basket |
| Energetic | Temporarily makes Moana faster, adds score, and fills the basket |
| Bad | Consumes 15 seconds and does not fill the basket |

Chickens start appearing after the first screens. During normal play they chase
ordinary lemons; during escape they chase Moana. The rooster appears on later
screens and attacks Moana directly. Press `A` to throw a lemon from the basket
and scare him away.

When the quota is complete, a door appears on one border. Moana must reach the
door while the chickens and rooster pursue her. After the final screen, the game
submits the score through the PRG32 scoreboard API.

## Controls

Use **Joystick 1**.

| Input | Action |
|---|---|
| D-pad | Move Moana |
| A | Throw a lemon from the basket |
| SELECT | Start, continue, or restart |
| ESC | Quit in emulator |

## PRG32 Narrative

PRG32 is an educational RISC-V gaming runtime. It turns low-level programming
from an abstract lecture into a visible loop: change state, rebuild a
cartridge, move a sprite, and see the result.

A PRG32 cartridge exports three functions:

```text
<prefix>_init
<prefix>_update
<prefix>_draw
```

`init` sets the initial state, `update` reads input and advances the model, and
`draw` renders the state. This cartridge uses:

| Entry prefix | Exported symbols |
|---|---|
| `moana_lemon_c` | `moana_lemon_c_init`, `moana_lemon_c_update`, `moana_lemon_c_draw` |

## Teaching Tutorial

For a complete first-year laboratory sequence, see
[`docs/tutorial-create-moana-lemon-apocalypse.md`](docs/tutorial-create-moana-lemon-apocalypse.md).
It guides students from the code-deploy-debug cycle through gameplay changes,
asset personalization, metadata, bundle packing, and final publication.

## Development Requirements

You need two separate repositories/directories:

1. this game repository;
2. a cloned PRG32 repository, used as the resident runtime and cartridge tool
   provider.

Example layout:

```bash
mkdir -p $HOME/src
cd $HOME/src
git clone https://github.com/riscv-prg32/PRG32.git
git clone https://github.com/riscv-prg32/MoanaAndTheLemonApocalypse.git
```

For the 24x24 sprite firmware used by this cartridge, use PRG32 branch
`dev-sprite24x24-1` or newer equivalent support:

```bash
cd $HOME/src/PRG32
git checkout dev-sprite24x24-1
```

Then tell the helper script where PRG32 is:

```bash
export PRG32_REPO=$HOME/src/PRG32
cd $HOME/src/MoanaAndTheLemonApocalypse
```

## Build a Portable Cartridge

Build for ESP32-C6 hardware:

```bash
export PRG32_ARCHITECTURE=esp32c6
scripts/build.sh
```

The output is:

```text
dist/moana-lemon-apocalypse-esp32c6.prg32
```

Build for QEMU:

```bash
export PRG32_ARCHITECTURE=qemu
scripts/build.sh
```

The output is:

```text
dist/moana-lemon-apocalypse-qemu.prg32
```

Portable builds are enabled by default with `PRG32_PORTABLE=1`. To build the
legacy firmware-specific absolute-import format for older firmware, set
`PRG32_PORTABLE=0` and pass the matching firmware ELF:

```bash
export PRG32_PORTABLE=0
export PRG32_ARCHITECTURE=esp32c6
scripts/build.sh "$PRG32_REPO/build/PRG32.elf"
```

## Deploy a Portable Cartridge

Upload to a PRG32 ESP32-C6 board:

```bash
python3 "$PRG32_REPO/tools/prg32_game.py" upload \
  dist/moana-lemon-apocalypse-esp32c6.prg32 \
  --url http://192.168.4.1
```

Stage the QEMU cartridge into a PRG32 QEMU flash image:

```bash
python3 "$PRG32_REPO/tools/prg32_game.py" upload-qemu \
  dist/moana-lemon-apocalypse-qemu.prg32 \
  --flash "$PRG32_REPO/build-qemu/flash_image.bin" \
  --partitions "$PRG32_REPO/partitions_prg32.csv"
```

## Publish a Cartridge Store Bundle

Build both architecture variants, pack the flat Store bundle, then publish:

```bash
export PRG32_REPO=$HOME/src/PRG32

export PRG32_ARCHITECTURE=esp32c6
scripts/build.sh

export PRG32_ARCHITECTURE=qemu
scripts/build.sh

scripts/pack-store-bundle.sh

python3 "$PRG32_REPO/tools/prg32_game.py" publish-bundle \
  dist/moana-lemon-apocalypse-store-bundle.zip \
  --store-url http://192.168.1.42:5080 \
  --token "$PRG32_STORE_TOKEN"
```

See [`docs/build-and-publish.md`](docs/build-and-publish.md) for the focused
build, deploy, QEMU, and Store workflow.

## Personalizing Graphics and Sounds

The current assets are generated. Rebuild them with:

```bash
python3 assets/generate_assets.py
```

Useful student exercises:

- change the splash screen title or classroom signature;
- recolor Moana, chickens, rooster, or lemons;
- adjust the lemon grove background;
- change short generated WAV motifs;
- keep artwork and sound original, and document provenance before publishing.

The running cartridge currently draws gameplay objects procedurally in
`c/game.c`, so visual changes that should appear during play may need matching
updates in the drawing functions.

## Teaching Notes

This project is intentionally small. It is not a generic game engine. That is a
feature for a classroom: students can trace the same ideas from state variables
to update rules to pixels on the screen.

Suggested lab path:

1. play the cartridge;
2. change a quota, speed, color, or sound;
3. inspect the related state variable in `c/game.c`;
4. rebuild and deploy to QEMU;
5. personalize one generated asset;
6. pack and publish a final cartridge bundle.

The reward is immediate: a low-level state change becomes a visible lemon,
timer, chicken, door, or final score.
