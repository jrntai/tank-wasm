# 08 — Stages and progression

## Stages

There are 10 stages, defined in `src/sim/stages.cpp`. Each has a 26×26 map following the stage format (ARN) and a lineup of 20 enemies, written compactly as a string of type letters: `B`asic, `F`ast, `P`ower, `A`rmor.

Every map keeps the fixed positions clear, as ARN-4 requires for stage 1: the three enemy spawns, the player spawn and the base. Every map also surrounds the base with the half-tile brick wall.

**Playability.** Every enemy spawn point must connect to the player spawn and to the base wall through space a tank can drive. Brick counts as passable for this check, because it can be shot away; steel and water don't.

**Difficulty.** A lineup's threat score is the sum of its enemies' weights: Basic 1, Fast 2, Power 3, Armor 4. Threat never decreases from one stage to the next, and stage 10 has the highest.

## Flow

The App (MNU) gains a **StageIntro** screen.

- A new game starts from Stage Select (SEL), which goes straight into play. The intro is shown when moving on to the next stage.
- The intro shows `STAGE N` for **90 ticks**, then play begins. The world doesn't tick during the intro. Back returns to Title.
- After `StageClear`, start shows the next stage's intro and then plays it. The new stage brings fresh terrain, a fresh base and its full lineup. The player's reserve lives and star level (PWR-11) carry over, and the player starts at the spawn point with a fresh shield.
- After stage 10 comes stage 11, played on stage 1's map. The stage number keeps counting; the map is stage `(N − 1) mod 10 + 1`.
- The intro doesn't latch keys (MNU-8): a direction pressed during the intro moves the tank as soon as play starts. Keys that were latched before the intro stay latched until released.

## Acceptance criteria

- [x] **PRG-1** There are 10 stages, numbered 1 to 10, each with a 20-enemy lineup.
- [x] **PRG-2** Every stage uses only valid terrain chars, keeps the spawns and the base clear, and walls the base (ARN-4).
- [x] **PRG-3** Every enemy spawn can reach the player spawn and the base wall, counting brick as passable but not steel or water.
- [x] **PRG-4** Threat never decreases from stage 1 to stage 10, and stage 10 has the highest.
- [x] **PRG-5** Moving on to the next stage shows its intro for 90 ticks without ticking the world, then play begins. Back during the intro returns to Title.
- [x] **PRG-6** Start after `StageClear` leads, through the intro, to the next stage with fresh terrain, base and lineup. Reserve lives carry over, and the player respawns at the spawn point with a shield.
- [x] **PRG-7** Stage 11 is played on stage 1's map, and the stage number keeps counting.
- [x] **PRG-8** A direction held when play begins after the intro moves the tank right away.
- [?] **PRG-9** The intro draws `STAGE N` in black on the gray screen, with the stage's name below it. *(manual)*
