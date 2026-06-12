# Moana and the Lemon Apocalypse

Moana is a 6-year-old girl gathering as many lemons as possible from lemon trees before the chickens get them. Lemons fall in pseudo-random positions, expire after a short time, and come in four flavors:

- normal lemons add score and fill Moana's basket
- magic lemons add 30 seconds
- energetic lemons make Moana faster
- bad lemons consume 15 seconds

The rooster periodically appears and attacks Moana. Press `A` to throw a lemon from the basket and scare him away. Each screen starts with 150 seconds; when the quota is complete, a door appears on one border and Moana must escape while the chickens and rooster pursue her.

## Controls

- D-pad: move Moana
- `A`: throw a lemon
- `START`: start or restart

## Build

Generate the original source assets:

```bash
python3 assets/generate_assets.py
```

Build a portable cartridge, using a sibling PRG32 checkout by default:

```bash
PRG32_REPO=/path/to/PRG32 scripts/build.sh
```

The cartridge is written to `dist/moana-lemon-apocalypse-esp32c6.prg32`.

## Notes

This project follows the cartridge layout used by `riscv-prg32/YouHaveGotPizza` and the C cartridge ABI documented by `riscv-prg32/PRG32`.

The project includes dependency-free generated 16x16 sprite sheets plus a polished AI-generated splash source at `assets/png/splash_moana_lemon_apocalypse_ai_320x200.png`.

The soundtrack is an original short PRG32 beep melody inspired by the cheerful feel of "Lemon Tree"; no copyrighted recording or copied audio is bundled.
