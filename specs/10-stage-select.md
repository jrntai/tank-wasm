# 10 — Stage select

## Behaviour

Confirming `1 PLAYER` on the Title opens **Stage Select**. It's the same gray curtain as the stage intro, but it waits for the player. It shows `STAGE N`, the stage's name, and a hint line.

- **Up or right** raises N, **down or left** lowers it, wrapping between 1 and 10. Like all menus, it reacts to presses, not holds (MNU-2).
- **Start or fire** begins stage N at once, with no separate intro since the curtain was already showing. The player starts with full reserve lives (2) and star level 0. A fresh seed is drawn, as for every new game (MNU-9).
- **Back** returns to Title.

All 10 stages are selectable. (Unlocking only the stages you've reached would need saved progress between visits; that's in the backlog.)

Stage Select opens on the **last stage started from it** in this session, so retrying a stage takes one key press. A new app starts at stage 1.

A game started from stage N advances as usual: N+1 next, and stage 11 replays stage 1's map (PRG-6, PRG-7). The world doesn't tick on this screen.

## Acceptance criteria

- [x] **SEL-1** Confirming `1 PLAYER` opens Stage Select showing stage 1 in a new app, and the world doesn't tick there.
- [x] **SEL-2** Up and right raise the stage, down and left lower it, wrapping between 1 and 10. A held key moves it once.
- [x] **SEL-3** Start or fire begins the chosen stage straight away (Game, no intro) with a fresh world for that stage, 2 reserve lives and level 0.
- [x] **SEL-4** Back returns to Title.
- [x] **SEL-5** Reopening Stage Select shows the stage most recently started from it.
- [x] **SEL-6** A game started from a stage advances to the following stage on clear, wrapping its map after 10 (PRG-7).
- [~] **SEL-7** Stage Select draws the gray curtain with `STAGE N`, the stage name and the hint `UP/DOWN SELECT - ENTER START`. *(manual)*

## Verification log

- 2026-10-12, headless Chrome driven over the DevTools protocol with real key events. Seen: Title → Enter opened Stage Select; Up twice showed `STAGE 3 / LAKE` with the hint line (SEL-7); Enter started stage 3 directly, with the HUD flag showing 3.
