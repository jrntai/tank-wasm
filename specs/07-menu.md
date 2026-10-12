# 07 — Menu and screens

## Behaviour

The app has five screens: **Title**, **Controls**, **StageSelect** (see 10-stage-select), **StageIntro** (see 08-progression) and **Game**. It starts on Title.

**Title** shows the game's name and a two-item menu: `1 PLAYER` and `CONTROLS`, with a tank cursor beside the selected item. Up and down move the cursor, wrapping around. Start (Enter) or fire confirms.

**Controls** lists the keys. Start, fire or back (Esc) returns to Title.

**Game** runs the world. Back abandons the game and returns to Title. After `GameOver`, start returns to Title; after `StageClear`, it moves on to the next stage (see 08-progression).

**Presses, not holds.** Menu actions react to a key going down, not to it being held: holding down moves the cursor once. Keys that are already held when the screen changes are ignored on the new screen until they're released. So the Enter that starts a game doesn't also count as a press in the game, and holding fire through the menu doesn't fire a bullet.

**Seeds.** The app is created with a seed. Each new game uses the next value from a deterministic sequence, so two games in a row play differently but every run is reproducible from the app seed.

Menu state lives in `sim::App` (`src/sim/app.{h,cpp}`), which is platform-independent and unit-tested. The platform layer sends it held input every tick.

## Acceptance criteria

- [x] **MNU-1** The app starts on Title with the cursor on `1 PLAYER`.
- [x] **MNU-2** Up and down move the cursor between the two items and wrap around. A held key moves it once.
- [x] **MNU-3** Confirming `1 PLAYER` with start or fire opens Stage Select (SEL-1).
- [x] **MNU-4** Confirming `CONTROLS` shows Controls. Start, fire or back returns to Title.
- [x] **MNU-5** The world doesn't tick while Title or Controls is showing.
- [x] **MNU-6** In Game, back returns to Title with the cursor on `1 PLAYER`.
- [x] **MNU-7** After `GameOver`, start returns to Title. After `StageClear`, start moves on to the next stage (PRG-6). While `Playing`, start does nothing.
- [x] **MNU-8** Keys held during a screen change are ignored on the new screen until released and pressed again.
- [x] **MNU-9** Consecutive games get different seeds; two apps with the same seed produce the same sequence of games.
- [~] **MNU-10** Title draws "TANK 1990" in large brick letters on black, with the two items and a yellow tank cursor beside the selection. Controls lists every key binding. *(manual)*

## Verification log

- 2026-10-12, headless Chrome screenshot: the title screen matches MNU-10 (brick "TANK 1990", both items, tank cursor on `1 PLAYER`). The controls screen and live keyboard navigation still need checking in a real browser.
- 2026-10-12, same driver with real key events: Down then Enter opened the Controls screen listing every binding; Esc returned to Title. MNU-10 is verified.
