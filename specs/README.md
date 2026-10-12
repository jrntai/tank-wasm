# Specs

This game is built spec-first. Every gameplay rule lives here before it lives in code.

## Workflow

1. **Spec**: add or change a requirement in the relevant spec file. Give each acceptance criterion a stable ID (`PLY-4`). Never reuse a retired ID.
2. **Test**: write a test in `tests/` named after the ID (`TEST("PLY-4", "...")`). It should fail.
3. **Code**: implement until the test passes.
4. **Status**: update the criterion's status marker.

Status markers: `[x]` implemented and tested · `[~]` implemented, verified manually (rendering/input) · `[?]` implemented, not yet verified · `[ ]` specified, not implemented.

Run one criterion's test with `build-native/sim_tests PLY-4`, or a whole spec with `build-native/sim_tests PLY`. Web server tests use Node's built-in runner: `node --test --test-name-pattern=WEB-3 server/test/server.test.mjs`.

## Index

| File | Prefix | Scope |
|---|---|---|
| [01-arena.md](01-arena.md) | ARN | Coordinate system, terrain grid, stage format |
| [02-player.md](02-player.md) | PLY | Player tank: spawn, movement, grid snapping, collision, lives |
| [03-bullets.md](03-bullets.md) | BUL | Firing, bullet travel, impacts, brick destruction |
| [04-enemies.md](04-enemies.md) | ENM | Enemy types, spawning, AI |
| [05-game-flow.md](05-game-flow.md) | GAM | Base, win/lose, phases, explosions |
| [06-presentation.md](06-presentation.md) | REN | Rendering, HUD, controls (manual verification) |
| [07-menu.md](07-menu.md) | MNU | Title and controls screens, screen flow |
| [08-progression.md](08-progression.md) | PRG | The 10 stages, difficulty curve, stage intro, advancing stages |
| [09-powerups.md](09-powerups.md) | PWR | Bonus tanks, the six power-ups, player upgrade levels |
| [10-stage-select.md](10-stage-select.md) | SEL | Choosing the starting stage |
| [11-web-server.md](11-web-server.md) | WEB | Serving the web build over HTTP |

## Shared definitions

- **Tick**: one fixed simulation step, 1/60 s. All durations and speeds are in ticks.
- **px**: one pixel at the internal resolution. Positions are integers.
- **Rect overlap**: half-open rectangles `[x, x+w) × [y, y+h)` overlap if they share at least one pixel.
- **Determinism**: given the same stage, seed and input sequence, the simulation produces identical state. All randomness comes from the world's seeded RNG.

## Backlog (not yet specified)

Ice sliding · score and high score · unlocking stages as you reach them (saved progress) · two-player mode · sound · spawn sparkle animation · custom HTML shell.
