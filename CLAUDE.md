# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project

A Battle City / "Tank 1990"-style game that runs in the browser. C++17 is compiled to WebAssembly with Emscripten and renders through WebGL2.

## Spec-driven workflow

Gameplay rules live in `specs/` before they live in code. `specs/README.md` has the index and the status markers. For any behavior change:

1. Edit the spec. Add or change an acceptance criterion with a stable ID (`PLY-4`); never reuse a retired ID.
2. Add a test named after the ID, `TEST("PLY-4", "...")`, in the matching `tests/<area>_tests.cpp`, and watch it fail.
3. Implement until it passes, then update the criterion's status marker.

Code comments cite spec IDs (`// PLY-4`). If code and spec disagree, the spec wins; or fix the spec first if it's wrong. Rendering and controls (REN) can't be unit-tested; record manual checks in the verification log in `specs/06-presentation.md`.

## Prerequisites

- Emscripten (`em++`, `emcmake`) and CMake ≥ 3.20, both installed via Homebrew here
- Node ≥ 20, for the web server and its tests
- A host C++ compiler (`clang++`) for native tests

## Commands

Web build, output in `build-web/web/` (`tank.html`, `tank.js`, `tank.wasm`):
```sh
emcmake cmake -S . -B build-web -DCMAKE_BUILD_TYPE=Release   # Debug adds -sASSERTIONS=2 and source maps
cmake --build build-web
npm start                               # serve build-web/web at http://127.0.0.1:8080/ (--port, --host, --dir)
cmake --build build-web --target serve  # or: rebuild, then serve
```
The server is `server/server.mjs`, Node with no dependencies, specified in `specs/11-web-server.md`. It serves only `build-web/web/`, never the CMake internals in `build-web/`. Its tests use Node's built-in runner: `npm run test:server`, or a single criterion with `node --test --test-name-pattern=WEB-3 server/test/server.test.mjs`.

Native build + tests (simulation only, no GL/Emscripten):
```sh
cmake -S . -B build-native -DCMAKE_BUILD_TYPE=Debug
cmake --build build-native
ctest --test-dir build-native --output-on-failure   # one CTest entry per spec: ARN PLY BUL ENM GAM MNU PRG PWR SEL WEB
build-native/sim_tests PLY-4                         # single criterion (prefix match: `PLY` runs the spec)
```
The test harness is homemade (`tests/test.h`: `TEST`, `CHECK`, `CHECK_EQ`), not a framework. Helpers there include `blank_world()` (empty arena, no enemies, player unshielded), `shoot()`, `fill()`, `stage_with_lineup()`, and for `App`, `tap()` and `start_game()` (Title → Stage Select → stage 1).

## Build layout

`CMakeLists.txt` branches on `EMSCRIPTEN`:
- `sim`: static lib from `src/sim/`, built in both configurations. Must not include GL or Emscripten headers.
- `tank`: `src/platform/*`, Emscripten only. Emits `.html/.js/.wasm` into `build-web/web/`, and adds a `serve` target. WebGL2 is required, so use the `GLES3/gl3.h` API.
- `sim_tests`: native only.

New `.cpp` files must be added to the matching target's source list.

## Architecture

- **Screen flow (`src/sim/app.{h,cpp}`)**: `sim::App` owns the screens (Title → Controls, or Title → StageSelect → Game → StageIntro → Game …) and the current `World`. When a stage begins, the App builds a fresh `World` and carries over the stage number and reserve lives. It takes raw held keys each tick, turns them into presses itself, and ignores keys that were held during a screen change until they're released (MNU-8); the intro → Game switch is the one exception and doesn't latch. It lives in `sim` so it's unit-tested; the platform calls only `app.update(input)` and `renderer.draw(app, …)`.
- **Simulation (`src/sim/world.{h,cpp}`)**: one `World` struct with public state, so tests and the renderer read it directly. It advances one fixed 60 Hz tick per `tick(Input)`. Tick order: explosions age → (stop here unless `Playing`) → enemy spawning → player → enemies → bullets → remove the dead → stage-clear check.
- **Integer, deterministic sim.** Positions are integer px. Tank speeds are in half-px per tick through a per-tank accumulator. All randomness comes from `World::random()` (xorshift, seeded in the constructor). Never read clocks or use `rand()` inside `sim`; ENM-10 checks determinism.
- **Geometry.** The arena is 208×208 px. Terrain is a 26×26 grid of 8 px cells; tanks are 16 px, bullets 4 px. Grid snapping on perpendicular turns (`World::turn`) is central to how the game feels; see PLY-4 and PLY-13.
- **Power-ups (PWR)**: enemies in lineup slots 4, 11 and 18 spawn with `Tank::bonus`. The first bullet hit on one calls `drop_powerup()`, which places the single `World::powerup` item (`std::optional`) with a random `PowerUpType`. `World::collect()` applies the effect. The star raises `Tank::level` (0–3), which sets bullet speed, the bullet limit and `Bullet::breaks_steel`; the App carries the level between stages along with lives, and losing a life resets it. Timed effects live on `World` (`shovel_ticks`, `freeze_ticks`) and stop outside `Playing`. `drop_powerup()` is public so tests can place items directly.
- **Bullets** advance 1 px per sub-step in lockstep, with collisions checked after each sub-step, so fast bullets can't tunnel through walls or past each other.
- **Stages** (`src/sim/stages.cpp`): a table of 10 `StageDef`s, each with 26 rows of terrain chars, a name and a lineup string (`B`/`F`/`P`/`A`). `stage_for(n)` wraps past stage 10 (PRG-7). `empty_stage()` gives a blank arena for tests. Stages 2–10 were drawn on a 13×13 tile grid and doubled to cells. When adding or editing a map, PRG-2, PRG-3 and PRG-4 check its format, that every spawn can reach the player and the base, and that difficulty keeps rising.
- `enemy_random_actions = false` turns off the enemies' random turns and shots, so tests can isolate movement and hits. Blocked enemies still turn.
- **Rendering (`src/platform/renderer.cpp`)**: everything draws into a 256×224 offscreen framebuffer, with the arena at (16, 8) and the HUD at x=232. That image is then blitted to the canvas at the largest integer scale with `GL_NEAREST`. Every sprite is a buffer-less textured quad (`gl_VertexID`) with a tint. Art is drawn facing up, and the shader rotates it by `int(sim::Dir)`, so don't reorder that enum.
- **Pixel art (`src/platform/sprites.cpp`)** is ASCII rows; the renderer turns chars into colors through palettes. Every row of a sprite must be the same width.
- **Platform (`src/platform/main.cpp`)**: keyboard → `sim::Input` (Enter = `start`, Esc = `back`), with a fixed-timestep accumulator driven by `emscripten_set_main_loop`.
