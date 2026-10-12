# 06 — Presentation and controls

The platform layer (`src/platform/`) renders and reads input. It's verified manually in the browser.

## Screen layout

Internal resolution 256×224, upscaled by the largest integer factor with nearest-neighbor filtering, letterboxed in gray.
- Arena (black) at screen offset (16, 8).
- HUD panel on the gray area to the right of the arena, starting at x = 232.

## Acceptance criteria

- [~] **REN-1** The arena draws at (16, 8) on a gray screen, with terrain drawn per cell. Brick, steel, water, trees and ice each have a distinct 8×8 pattern.
- [?] **REN-2** Tanks draw facing their direction. The player is yellow. Enemies are colored by type, and Armor changes color as it loses hits.
- [?] **REN-3** Trees draw above tanks and bullets, so tanks can hide under them.
- [~] **REN-4** A shielded player flickers with a white outline.
- [?] **REN-5** The base draws as an eagle while alive and as a ruined flag once destroyed.
- [~] **REN-6** Explosions draw at their size for their duration.
- [~] **REN-7** The HUD shows one small icon per enemy still waiting to spawn (2 columns), the player's reserve lives as `IP` and a digit, and the stage number under a flag.
- [?] **REN-8** In `GameOver` the screen shows `GAME OVER`; in `StageClear` it shows `STAGE CLEAR`. Both show `PRESS ENTER`.
- [?] **REN-9** Controls: arrow keys or WASD to move, Space or J to fire, Enter for start, Esc for back (screen flow in MNU). Game keys don't scroll the page.

## Verification log

- 2026-10-12, headless Chrome screenshots of stage 1 at the start. Seen:
  - REN-1: all five terrain types (ice added to stage 1 for this).
  - REN-4: the shield outline.
  - REN-5: the eagle while alive.
  - REN-6: a small explosion.
  - REN-7: 19 icons, `IP 2` and the flag with `1`.
  - REN-2, in part: the player is yellow and a Basic enemy is gray, drawn facing down.

  Still to check in a real browser: the other enemy colors, trees over tanks, the ruined base, the end-of-game text and the controls.
- 2026-10-12, key-driven headless Chrome:
  - REN-9 in part: arrows, Enter, Esc and Space work, and Space fired at the brick above the stage 3 spawn, knocking out one row. WASD and J are still unchecked.
  - REN-2: Fast enemies are cyan.
