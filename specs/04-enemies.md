# 04 — Enemies

## Types

| Type | Speed (half-px/tick) | Bullet | Hits to destroy |
|---|---|---|---|
| Basic | 1 | normal | 1 |
| Fast | 3 | normal | 1 |
| Power | 2 | fast | 1 |
| Armor | 2 | normal | 4 |

## Spawning

A stage's lineup spawns in order. At most **4 enemies** are on the field at once. A spawn is attempted when the spawn timer is 0 and fewer than 4 enemies are alive. The timer starts at 0, so the first enemy appears on the first tick.

The spawn point cycles center → right → left → center. If any tank overlaps the next spawn point, the spawn waits and is retried each tick at the same point. After a successful spawn, the timer resets to **120 ticks** and counts down one per tick. A new enemy faces down.

## AI

Each tick, every enemy:

1. **Moves** forward at its speed. If it moved fewer px than its speed allowed this tick, it's **blocked**: it turns to a random direction other than its current one, using the turn rules in PLY-4.
2. Otherwise, if its position along its axis of travel is a multiple of 8, there's a 1-in-16 chance it **turns** to a random direction. The new direction may be its current one.
3. **Fires** with a 1-in-32 chance if it has no bullet in flight.

Random directions are drawn from the weighted table `[Down, Down, Down, Left, Right, Left, Right, Up]`: down 3/8, left 2/8, right 2/8, up 1/8. This pushes enemies toward the base.

## Acceptance criteria

- [x] **ENM-1** Enemies move at their type's speed: Basic 1 px every 2 ticks, Fast 3 px every 2 ticks, Power and Armor 1 px per tick.
- [x] **ENM-2** Power enemies fire fast bullets; other types fire normal bullets.
- [x] **ENM-3** Armor enemies take 4 hits; the others take 1.
- [x] **ENM-4** The first enemy spawns at the center point, facing down, on the first tick.
- [x] **ENM-5** Spawn points cycle center, right, left, with at least 120 ticks between spawns.
- [x] **ENM-6** No more than 4 enemies are on the field at once, and each enemy in the lineup spawns exactly once.
- [x] **ENM-7** A spawn waits while any tank overlaps the spawn point, then spawns there once it's clear.
- [x] **ENM-8** A blocked enemy turns to a different direction on the same tick.
- [x] **ENM-9** An enemy fires only when it has no bullet in flight.
- [x] **ENM-10** With the same seed and inputs, two worlds stay identical: the AI is deterministic.
