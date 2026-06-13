# Teaching Tutorial: Create, Test, and Publish Moana and the Lemon Apocalypse

This tutorial guides first-year computer science and computer engineering
students through the creation of **Moana and the Lemon Apocalypse**, a small
PRG32 cartridge game. Students begin from a working cartridge, study it as a
system of state and rules, then make controlled changes while preserving the
technical architecture.

The exercise is organized as one-hour laboratories. Each laboratory has a
practical deliverable, a conceptual target, and a visible test. The central
habit is simple: state an expected behavior, edit one small part, build,
deploy, observe, and debug from evidence.

## Learning Outcomes

By the end of the sequence, students should be able to:

- explain the PRG32 cartridge lifecycle: `init`, `update`, and `draw`;
- organize game state in C using constants, arrays, `enum`, and `struct`;
- read joystick input and convert it into movement and actions;
- implement timers, spawning, collection, collision, scoring, and reset logic;
- draw a complete 320x200 scene with primitive graphics operations;
- use pseudo-random state to create reproducible variation;
- build portable cartridges for ESP32-C6 hardware and QEMU;
- package metadata, icon, screenshot, architecture variants, and colophon for
  the Cartridge Store;
- publish a cartridge responsibly, including license and asset provenance.

## Course Setting

Recommended preparation:

- one introductory lecture on C variables, functions, arrays, and `struct`;
- one introductory lecture on CPU state, memory, and the fetch-execute cycle;
- a working PRG32 checkout and, when available, one PRG32 ESP32-C6 board per
  small group;
- QEMU available for students who do not have immediate hardware access.

Students may work in pairs. One student acts as the driver, typing and running
the build; the other acts as the observer, reading compiler messages, checking
the expected behavior, and keeping the lab notebook. Swap roles at least once
per laboratory.

## The Code-Deploy-Debug Cycle

Every laboratory uses the same disciplined loop.

1. **State the expected behavior.**
   Write one sentence in the lab notebook before touching the code. Example:
   "When Moana collects an energetic lemon, her movement speed becomes faster
   for a short time and the score increases by 50."

2. **Edit one small part.**
   Change a single function or a small related group of constants. Avoid
   changing movement, rendering, and packaging in the same step.

3. **Build locally.**
   For ESP32-C6:

   ```bash
   export PRG32_REPO=$HOME/src/PRG32
   export PRG32_ARCHITECTURE=esp32c6
   scripts/build.sh
   ```

   For QEMU:

   ```bash
   export PRG32_REPO=$HOME/src/PRG32
   export PRG32_ARCHITECTURE=qemu
   scripts/build.sh
   ```

4. **Deploy to the available target.**
   Upload to hardware:

   ```bash
   python3 "$PRG32_REPO/tools/prg32_game.py" upload \
     dist/moana-lemon-apocalypse-esp32c6.prg32 \
     --url http://192.168.4.1
   ```

   Stage for QEMU:

   ```bash
   python3 "$PRG32_REPO/tools/prg32_game.py" upload-qemu \
     dist/moana-lemon-apocalypse-qemu.prg32 \
     --flash "$PRG32_REPO/build-qemu/flash_image.bin" \
     --partitions "$PRG32_REPO/partitions_prg32.csv"
   ```

5. **Observe and record.**
   Test only the behavior named in step 1. Record pass/fail and one visible
   fact from the screen, serial monitor, or QEMU window.

6. **Debug from the symptom.**

   | Symptom | Likely location |
   |---|---|
   | Build fails | syntax, missing include, wrong symbol name |
   | Cartridge uploads but does not start | entry prefix or metadata mismatch |
   | Moana moves incorrectly | input mask, speed, coordinate clamp |
   | Lemons never appear | spawn timer, tree count, active lemon slots |
   | Collection feels unfair | hit boxes or sprite dimensions |
   | Rooster cannot be scared | basket count, shot direction, shot collision |
   | Store rejects the bundle | manifest fields, missing cartridges, token |

The loop is deliberately short. In a first-year course, many bugs become
unmanageable only because too many changes are made before the next test.

## Laboratory 1: Understand the Game and the Cartridge Contract

Estimated duration: 1 hour.

### Goal

Students understand what the game is and how PRG32 calls a cartridge.

### Activities

1. Play or watch the finished cartridge.
2. Read the first comment block in `c/game.c`.
3. Identify the three exported cartridge functions:
   `moana_lemon_c_init`, `moana_lemon_c_update`, and `moana_lemon_c_draw`.
4. Draw a loop diagram: `init` once, then repeated `update` and `draw`.
5. Discuss why rule changes belong in `update` and pixel drawing belongs in
   `draw`.

### Deliverable

A one-page design note containing:

- Moana's objective;
- the losing condition;
- the three cartridge functions and their responsibilities;
- one proposed personal variation that remains copyright-clean.

### Checkpoint

The student can answer: "Which function changes the timer, and which function
draws it?"

## Laboratory 2: Prepare the Workspace and Build the First Cartridge

Estimated duration: 1 hour.

### Goal

Students build the unmodified cartridge and learn the local directory layout.

### Activities

1. Clone or locate both repositories:

   ```bash
   mkdir -p $HOME/src
   cd $HOME/src
   git clone https://github.com/riscv-prg32/PRG32.git
   git clone https://github.com/riscv-prg32/MoanaAndTheLemonApocalypse.git
   ```

2. Use a PRG32 checkout with 24x24 sprite ABI support:

   ```bash
   cd $HOME/src/PRG32
   git checkout dev-sprite24x24-1
   ```

3. Enter the game repository and configure the PRG32 path:

   ```bash
   cd $HOME/src/MoanaAndTheLemonApocalypse
   export PRG32_REPO=$HOME/src/PRG32
   ```

4. Build the portable cartridge:

   ```bash
   export PRG32_ARCHITECTURE=esp32c6
   scripts/build.sh
   ```

5. Repeat for QEMU:

   ```bash
   export PRG32_ARCHITECTURE=qemu
   scripts/build.sh
   ```

### Deliverable

Two generated files:

- `dist/moana-lemon-apocalypse-esp32c6.prg32`;
- `dist/moana-lemon-apocalypse-qemu.prg32`.

### Debug Notes

If `scripts/build.sh` says that portable cartridge builds are unsupported, the
PRG32 checkout is too old for this workflow. Update PRG32 to a branch or
release with portable ABI-table cartridge tooling, or use the legacy format
shown in `docs/build-and-publish.md`.

## Laboratory 3: Model the Game State

Estimated duration: 1 hour.

### Goal

Students recognize the data model behind a visible game.

### Activities

1. Inspect the constants at the top of `c/game.c`: screen size, sprite size,
   counts, timers, quota, and final level.
2. Study the `Lemon`, `Shot`, `Chicken`, and `Rooster` structures.
3. Locate these global variables: `state`, `lemons`, `shots`, `chickens`,
   `rooster`, `frame_no`, `screen_timer`, `score`, `basket`, `screen_no`,
   `quota`, `moana_x`, and `moana_y`.
4. Fill a table in the lab notebook:

   | Variable | Type | Meaning | Changes in |
   |---|---|---|---|
   | `score` | `uint16_t` | player score | lemon collection, rooster hit, escape |
   | `basket` | `uint16_t` | throwable lemons | collection, throw, damage |
   | `screen_timer` | `uint32_t` | time remaining in frames | update, magic lemon, bad lemon |
   | `frame_no` | `uint32_t` | animation and music clock | every update |

5. Change one harmless constant, such as `QUOTA_BASE`, rebuild, and observe the
   HUD.

### Deliverable

A completed state table and one verified constant change.

### Concept Link

This laboratory connects directly to computer architecture: a game is state in
memory plus instructions that transform it. The screen is a visible projection
of that memory.

## Laboratory 4: Initialize the World

Estimated duration: 1 hour.

### Goal

Students implement reproducible starting conditions.

### Activities

1. Read `start_new_game`, `start_screen`, `reset_chickens`, and `choose_door`.
2. Draw the screen coordinate system on paper: origin at top-left, `x` grows
   right, `y` grows down.
3. Modify Moana's starting position in `start_screen`.
4. Rebuild, deploy, and verify that only the start position changed.
5. Restore the original position or choose a new position that improves the
   level.

### Deliverable

A level-map sketch showing trees, Moana's start, chickens, rooster spawn edges,
and possible door locations.

### Debug Notes

Coordinate bugs are common. PRG32 uses a top-left origin. If a sprite partly
leaves the screen, inspect the coordinate clamps in `update_moana`.

## Laboratory 5: Spawn and Collect Lemons

Estimated duration: 1 hour.

### Goal

Students learn timed spawning, object lifetimes, and collection rules.

### Activities

1. Read `pick_lemon_kind`, `spawn_lemon_from_tree`,
   `spawn_lemons_from_active_trees`, and `update_lemons`.
2. Explain why each lemon has `active`, `age`, `ttl`, and `kind`.
3. Change the normal lemon score from `20` to another value in
   `collect_lemon`.
4. Rebuild, deploy, collect one normal lemon, and verify the score.
5. Observe how magic, energetic, and bad lemons change timer or speed.

### Deliverable

A rule trace for one collection event:

```text
lemon kind = ...
score before = ...
timer before = ...
basket before = ...
score after = ...
timer after = ...
basket after = ...
```

### Debug Notes

If lemons stop appearing, check whether all `MAX_LEMONS` slots remain active.
If a lemon scores repeatedly, check whether `collect_lemon` clears
`l->active`.

## Laboratory 6: Read Input and Move Moana

Estimated duration: 1 hour.

### Goal

Students convert joystick bits into controlled movement and actions.

### Activities

1. Read `moana_lemon_c_update`.
2. Locate `prg32_input_read`, `BTN_MOVE_MASK`, `BTN_THROW`, and `BTN_SELECT`.
3. Study `update_moana`.
4. Identify these movement rules:
   - left and right change `moana_x`;
   - up and down change `moana_y`;
   - energy lemons temporarily increase speed;
   - clamps keep Moana inside the visible screen.
5. Experiment with the normal speed by changing `3` to `2` or `4`.

### Deliverable

A short test report:

- expected speed change;
- observed speed change;
- whether collection still feels fair.

### Debug Notes

If Moana moves diagonally too fast, inspect how simultaneous button bits are
handled. If Moana cannot reach the door, inspect the screen clamps and door
coordinates.

## Laboratory 7: Throw Lemons and Scare the Rooster

Estimated duration: 1 hour.

### Goal

Students connect inventory, projectiles, direction, and enemy state.

### Activities

1. Read `throw_lemon`, `update_shots`, and `update_rooster`.
2. Explain why throwing requires `basket > 0`.
3. Trace how `last_dir` selects projectile velocity.
4. Hit the rooster with a thrown lemon and observe `rooster.scared`.
5. Change the rooster hit score from `100` to another value and verify it.

### Deliverable

A projectile trace:

```text
basket before throw = ...
last_dir = ...
shot dx, dy = ...
rooster scared before = ...
rooster scared after hit = ...
score change = ...
```

### Debug Notes

If no shot appears, check basket count first. If the shot moves in the wrong
direction, inspect `last_dir` and the `vx`/`vy` tables.

## Laboratory 8: Add Enemies, Hazards, and Escape

Estimated duration: 1 hour.

### Goal

Students connect autonomous motion, collision detection, damage, and level
progression.

### Activities

1. Read `chicken_count_for_screen` and `update_chickens`.
2. Identify how chickens chase normal lemons during play and Moana during
   escape.
3. Read `check_hazards` and `hurt_moana`.
4. Read `check_escape`.
5. Tune the time penalty in `hurt_moana` and test whether the game feels easier
   or harder.

### Deliverable

A balancing note recommending one damage penalty and explaining why.

### Concept Link

Collision detection here uses rectangle overlap. That prepares students for
later courses where collision becomes geometry, bounding boxes, spatial
partitioning, or physics.

## Laboratory 9: Draw the Grove, Actors, and HUD

Estimated duration: 1 hour.

### Goal

Students learn that rendering is a sequence of deliberate drawing operations.

### Activities

1. Read `draw_field`, `draw_tree`, `draw_lemon`, `draw_moana_pose`,
   `draw_chicken`, `draw_rooster`, and `draw_hud`.
2. Identify the draw order in `moana_lemon_c_draw`.
3. Change one color constant and rebuild.
4. Add one small rectangle to the background.
5. Verify that the new drawing operation does not hide Moana, lemons, chickens,
   the rooster, the door, or the HUD.

### Deliverable

A screenshot or photo of the modified scene, plus the exact function where the
change was made.

### Concept Link

Draw order is the simplest form of layering. Later graphics systems provide
sprites, tiles, depth buffers, and compositors, but the basic question is the
same: what is drawn first, and what is drawn over it?

## Laboratory 10: Personalize Original Assets

Estimated duration: 1 hour.

### Goal

Students personalize the game without violating copyright or breaking the
runtime.

### Activities

1. Inspect `assets/generate_assets.py`.
2. Modify one generated visual element, such as the splash screen, background,
   sprite colors, or icon.
3. Regenerate assets:

   ```bash
   python3 assets/generate_assets.py
   ```

4. If the runtime should match the new asset style, update the rectangle
   drawing functions in `c/game.c`.
5. Record a provenance sentence for the changed asset.

### Deliverable

One personalized original asset and a provenance sentence explaining who made
it and how.

### Publication Rule

Do not copy sprites, logos, music, names, level art, or sound effects from
commercial games. A classroom imitation game imitates a design problem, not
protected assets.

## Laboratory 11: Metadata, Versioning, and Store Readiness

Estimated duration: 1 hour.

### Goal

Students prepare the cartridge as a publishable software artifact.

### Activities

1. Inspect `metadata/metadata.json`, `metadata/manifest.json`, and
   `metadata/colophon.json`.
2. Identify the title, version, authors, license, tags, architecture list,
   icon, screenshot, and colophon.
3. Confirm that `assets/icon.png` and `assets/screenshot.png` represent the
   current game.
4. Update the version only when the cartridge behavior or publishable content
   changes.
5. Build both architecture variants:

   ```bash
   export PRG32_REPO=$HOME/src/PRG32

   export PRG32_ARCHITECTURE=esp32c6
   scripts/build.sh

   export PRG32_ARCHITECTURE=qemu
   scripts/build.sh
   ```

6. Pack the Store bundle:

   ```bash
   scripts/pack-store-bundle.sh
   ```

### Deliverable

`dist/moana-lemon-apocalypse-store-bundle.zip`.

### Debug Notes

If packing fails, check that both architecture-specific cartridges exist in
`dist`. The manifest should list:

- `moana-lemon-apocalypse-esp32c6.prg32`;
- `moana-lemon-apocalypse-qemu.prg32`.

## Laboratory 12: Publish on the Cartridge Store

Estimated duration: 1 hour.

### Goal

Students publish, verify, and document the final cartridge.

### Activities

1. Confirm that the Store URL and token are available. The token may be provided
   through the environment:

   ```bash
   export PRG32_STORE_TOKEN=replace-with-the-class-token
   ```

2. Publish the bundle:

   ```bash
   python3 "$PRG32_REPO/tools/prg32_game.py" publish-bundle \
     dist/moana-lemon-apocalypse-store-bundle.zip \
     --store-url http://192.168.1.42:5080 \
     --token "$PRG32_STORE_TOKEN"
   ```

3. Open the Cartridge Store page.
4. Verify that the listing shows title, icon, screenshot, summary, version, and
   supported architectures.
5. Install or download the cartridge from the Store if the classroom setup
   supports that workflow.

### Deliverable

A publication report containing:

- Store URL;
- cartridge title and version;
- publication time;
- one screenshot of the Store listing;
- one sentence describing what the student changed from the base game.

### Debug Notes

If publication fails with an authentication error, check the token. If the Store
accepts the upload but the listing looks wrong, inspect the metadata and asset
files, rebuild both variants, repack the bundle, and publish again.

## Final Assessment Rubric

| Criterion | Excellent | Satisfactory | Needs work |
|---|---|---|---|
| Design clarity | Objective, rules, and feedback are clear | Main objective works | Player cannot infer rules |
| Code structure | Changes are localized and readable | Code works with minor roughness | Changes break existing structure |
| Debug discipline | Uses short code-deploy-debug cycles with notes | Tests after major steps | Makes many changes without evidence |
| Technical correctness | Build, deploy, collision, scoring, reset, and escape all work | Core game works with small issues | Game does not run reliably |
| Originality and ethics | Assets and theme are original and documented | Mostly original with minor omissions | Uses unclear or copied assets |
| Publication quality | Store bundle is complete and verified | Bundle publishes with minor metadata issues | Bundle cannot be published |

## Suggested Extensions

After the required laboratories, stronger groups may attempt one extension:

- add a new lemon kind with a timer;
- introduce a different enemy behavior after screen 4;
- change the quota curve and document the difficulty effect;
- convert one rectangle actor to a bitmap-backed sprite;
- add a new final victory message or sound pattern;
- add a classroom-specific generated splash detail.

Extensions should follow the same rule as the core tutorial: one expected
behavior, one small edit, one build, one deployment, one observation.
