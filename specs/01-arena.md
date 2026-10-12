# 01 — Arena and terrain

## Coordinates

The arena is 208×208 px (13×13 tiles of 16 px). Origin is the top-left corner; x grows right, y grows down. All entity positions are the top-left corner of their rect.

Terrain is stored on a 26×26 grid of 8 px **cells** (each tile is 2×2 cells), so walls can be half a tile thick.

## Terrain types

| Type | Char | Blocks tanks | Blocks bullets | Notes |
|---|---|---|---|---|
| Empty | `.` | no | no | |
| Brick | `#` | yes | yes | Destroyed by bullets (BUL-6) |
| Steel | `@` | yes | yes | Indestructible (for now) |
| Water | `~` | yes | no | |
| Trees | `%` | no | no | Drawn above tanks (REN-3) |
| Ice | `-` | no | no | Sliding is in the backlog; behaves like Empty |

## Stage format

A stage is 26 strings of 26 terrain chars plus an **enemy lineup**: the ordered list of enemy types that will spawn. Stages are defined in `src/sim/stages.cpp`.

Fixed positions (px):
- Base (eagle): (96, 192), 16×16 — bottom center
- Player spawn: (64, 192)
- Enemy spawn points: (96, 0) center, (192, 0) right, (0, 0) left

## Acceptance criteria

- [x] **ARN-1** A stage's terrain chars load into the matching cells, row-major from the top-left.
- [x] **ARN-2** Brick, Steel and Water block tanks; Empty, Trees and Ice don't.
- [x] **ARN-3** Brick and Steel block bullets; Water, Trees, Ice and Empty don't.
- [x] **ARN-4** Stage 1 keeps the base, the player spawn and all three enemy spawn points free of terrain, and puts a brick wall around the base.
