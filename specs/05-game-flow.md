# 05 — Base, game flow and explosions

## Base

The base (eagle) is a 16×16 entity at (96, 192). It blocks tanks whether alive or destroyed. Any bullet that hits it, including a player bullet, destroys it.

## Phases

`Playing` → `GameOver` | `StageClear`

- **GameOver**: the base is destroyed, or the player is destroyed with no reserve lives.
- **StageClear**: every enemy in the lineup has been destroyed.

Outside `Playing`, the simulation is frozen: tanks, bullets, spawning and timers stop. Only explosions keep counting down so the last one finishes playing. The tick counter keeps running for animation.

## Explosions

Explosions are visual only and have no effect on gameplay. Each has a centre point, a size and a remaining-ticks counter that drops by one per tick; it's removed at 0.
- **Small** (16×16, 8 ticks): bullet impacts.
- **Big** (32×32, 16 ticks): a tank or the base is destroyed.

## Acceptance criteria

- [x] **GAM-1** A new world starts in `Playing` with the base alive.
- [x] **GAM-2** Any bullet that hits the base destroys it and the phase becomes `GameOver`.
- [x] **GAM-3** Destroying the player with no reserve lives makes the phase `GameOver`.
- [x] **GAM-4** Destroying every enemy in the lineup makes the phase `StageClear`. A stage with an empty lineup never clears.
- [x] **GAM-5** Outside `Playing`, tanks, bullets and spawning are frozen; explosions finish.
- [x] **GAM-6** Bullet impacts create small explosions; destroyed tanks and the base create big ones. Explosions disappear after their duration.
- [x] **GAM-7** Destroying an enemy increments the kill count; a hit on armor that doesn't destroy it doesn't.
