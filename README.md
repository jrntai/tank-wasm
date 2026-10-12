# Tank 1990

A Battle City–style tank game for the browser. The game is written in C++17, compiled to WebAssembly with Emscripten, and drawn with WebGL2. A small Node server hosts the build.

It's built **spec-first**. Every gameplay rule is written down in `specs/` with an ID before it's coded, and every ID has a test.

## Quick start

```sh
brew install emscripten cmake node                              # once

emcmake cmake -S . -B build-web && cmake --build build-web      # build the game (or: npm run build)
npm start                                                       # http://127.0.0.1:8080/
```

Open http://127.0.0.1:8080/ and press Enter.

Tested with Emscripten 6.0, CMake 4.4 and Node 24. CMake ≥ 3.20 and Node ≥ 20 should work.

### Controls

| Key | Action |
|---|---|
| Arrows / WASD | Move (in menus: choose) |
| Space / J | Fire (in menus: confirm) |
| Enter | Start / confirm / continue after a stage |
| Esc | Back to the title |

## Building

There are two separate builds. The game logic doesn't depend on the browser, so it also builds and runs natively for tests.

| | Directory | Toolchain | Produces |
|---|---|---|---|
| Web | `build-web/` | `emcmake` (Emscripten) | `build-web/web/tank.{html,js,wasm}` |
| Native | `build-native/` | system `clang++` | `build-native/sim_tests` |

```sh
# Web, Release by default
emcmake cmake -S . -B build-web
cmake --build build-web

# Web, Debug: -sASSERTIONS=2 and source maps, in its own directory
emcmake cmake -S . -B build-web-debug -DCMAKE_BUILD_TYPE=Debug
cmake --build build-web-debug
node server/server.mjs --dir build-web-debug/web

# Native, for tests
cmake -S . -B build-native -DCMAKE_BUILD_TYPE=Debug
cmake --build build-native
```

After changing C++ code, rebuild with `cmake --build build-web` and reload the page. The server doesn't need restarting: it sends `Cache-Control: no-cache` with ETags, so the browser picks up new files.

## Running

```sh
npm start                                  # serves build-web/web on 127.0.0.1:8080
node server/server.mjs --port 9000         # another port (PORT=9000 also works)
node server/server.mjs --host 0.0.0.0      # reachable from your LAN
cmake --build build-web --target serve     # rebuild, then serve
```

The server only serves `build-web/web/`. Opening `tank.html` directly from disk (`file://`) won't work, because browsers refuse to load WebAssembly that way.

## Testing

```sh
ctest --test-dir build-native --output-on-failure   # everything: one entry per spec + the web server
build-native/sim_tests                              # all C++ tests
build-native/sim_tests PLY                          # one spec (prefix match)
build-native/sim_tests PLY-4                        # one criterion
npm run test:server                                 # web server tests (Node's built-in runner)
node --test --test-name-pattern=WEB-3 server/test/server.test.mjs
```

The C++ tests use a minimal homemade harness, `tests/test.h` (`TEST`, `CHECK`, `CHECK_EQ`), not a framework. Useful helpers in there:
- `blank_world()`: an empty arena with no enemies and an unshielded player
- `shoot()`: puts a bullet in flight
- `fill()`: paints terrain over a pixel rectangle
- `stage_with_lineup()`: an empty arena with a custom enemy lineup
- `tap()` and `start_game()`: drive the menu App

Rendering and input can't be unit-tested. Check them in the browser, and note what you checked in the spec's *Verification log*.

## Project layout

```
specs/            The rules: one file per area, each criterion with an ID (start at specs/README.md)
src/sim/          Game logic: no GL, no Emscripten, deterministic
  world.{h,cpp}     World: tanks, bullets, terrain, enemies, power-ups; one tick() per 1/60 s
  app.{h,cpp}       App: screens (title, controls, stage select, intro, game), owns the World
  stages.cpp        The 10 stage maps and enemy lineups
src/platform/     Browser side: input, main loop, WebGL2 rendering, pixel art
tests/            C++ tests, one file per spec area
server/           Node web server and its tests
CMakeLists.txt    Builds `sim` (lib), `tank` (web, Emscripten only), `sim_tests` (native only)
```

## Architecture

- **`sim` doesn't know about the browser.** `World` and `App` are plain C++ with public state, so the native tests and the renderer both read them directly. Anything that touches GL or Emscripten belongs in `src/platform/`.
- **Fixed timestep.** `main.cpp` adds up real time and calls `App::update(input)` once per 1/60 s tick, then the renderer draws the current state. Gameplay doesn't depend on frame rate.
- **Deterministic.** Positions are integer pixels. Tank speeds are in half-pixels per tick. All randomness comes from a seeded xorshift inside `World`. The same seed and inputs always give the same game (ENM-10). Never use clocks or `rand()` in `src/sim/`.
- **Movement feel.** Tanks move in four directions. When a tank turns onto the other axis, it snaps to the 8 px grid (PLY-4), which is what lets tanks slip through one-tile gaps.
- **Bullets** move 1 px per sub-step, in lockstep with each other. That way fast bullets can't skip through walls or pass through each other.
- **Rendering.** Everything is drawn into a 256×224 offscreen image, which is then scaled up by a whole number with nearest-neighbor filtering. Sprites are textured quads with no vertex buffers. Pixel art lives in `sprites.cpp` as ASCII rows plus color palettes.
- **Input.** `App` gets the raw held keys each tick and works out presses itself. A key held across a screen change is ignored until it's released (MNU-8). That stops, for example, the Enter that starts a game from also firing.

## Making changes

Follow the spec workflow in `specs/README.md`:

1. **Spec**: edit or add a criterion in the right `specs/NN-*.md` and give it a new ID. Never reuse an old ID.
2. **Test**: write a test named after the ID and watch it fail.
3. **Code**: implement until it passes. Code comments cite the IDs (`// PWR-9`).
4. **Status**: update the criterion's marker: `[x]` tested, `[~]` checked by hand, `[?]` not yet verified.

The tests don't prove they work by passing, so for important rules, break the rule on purpose in a scratch copy and check that its test fails.

### Common tasks

- **Edit a stage.** Change its 26 rows in `src/sim/stages.cpp`. Terrain chars: `.` empty, `#` brick, `@` steel, `~` water, `%` trees, `-` ice. The lineup string uses `B`asic, `F`ast, `P`ower and `A`rmor. Then run `sim_tests PRG ARN`: these check the format, clear spawn points, the base wall, that every spawn can reach the player and the base, and that difficulty never decreases. The spec fixes the count at exactly 10 stages (PRG-1), so adding an 11th starts with a spec change.
- **Change a tuning number** (speeds, timers, limits): the constants are at the top of `src/sim/world.h`. Update the spec that states the value, too.
- **Add a sprite.** Add ASCII rows to `src/platform/sprites.cpp`. Every row must be the same width. Then build a texture from it with a palette in `Renderer::init()`.

## Troubleshooting

- **`tank.html not found` when starting the server.** Build the web target first: `npm run build`.
- **`port 8080 ... already in use`.** Another server is running. Stop it, or use `--port`.
- **Black or blank page.** Check the browser console. The game needs WebGL2, and it logs shader or context errors there.
- **A change doesn't show up.** Make sure you rebuilt `build-web` (not only `build-native`), then reload.
