# Moana and the Lemon Apocalypse

Moana is a Neapolitan 6-year-old girl with pale skin, brown-red hair, and green-blue eyes, gathering as many lemons as possible from lemon trees before the chickens get them. Lemons fall in pseudo-random positions, expire after a short time, and come in four flavors:

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

Build a portable cartridge, using a sibling PRG32 checkout by default. For the 24x24 sprite firmware, use PRG32 branch `dev-sprite24x24-1`:

```bash
git -C /path/to/PRG32 checkout dev-sprite24x24-1
PRG32_REPO=/path/to/PRG32 scripts/build.sh
```

The cartridge is written to `dist/moana-lemon-apocalypse-esp32c6.prg32`.

## Notes

This project follows the cartridge layout used by `riscv-prg32/YouHaveGotPizza` and the C cartridge ABI documented by `riscv-prg32/PRG32`.

The project includes dependency-free generated 24x24 sprite sheets plus a polished AI-generated splash source at `assets/png/splash_moana_ischia_ai_320x200.png`. The splash background uses the Castello Aragonese of Ischia rather than a South Pacific volcano island.

The soundtrack is an original short PRG32 beep melody inspired by the cheerful feel of "Lemon Tree"; no copyrighted recording or copied audio is bundled.
