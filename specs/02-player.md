# 02 — Player tank

## Behaviour

The player tank is 16×16 px. It moves in four directions only, at 1 px per tick.

Speeds are in **half-pixels per tick** so slow tanks can move at fractional speeds with integer positions: each tick a tank adds its speed to an accumulator and moves one px for every 2 accumulated.

**Grid snapping.** When a tank turns onto the perpendicular axis, the coordinate of the axis it was moving on snaps to the nearest multiple of 8 (ties round up). This is what lets tanks slip into one-tile gaps the way they do in the original game. Reversing direction on the same axis doesn't snap. A snap never moves a tank into another tank: if the nearest grid line would overlap one, the tank snaps to the grid line on the other side; if both would, it turns without snapping. Terrain, the base and the arena edge can never block a snap, because they sit on the 8 px grid and both candidate positions lie within the cells the tank already covers.

**Collision.** A tank moves one px at a time and stops at the first px that would be blocked. A position is blocked if the tank's rect would leave the arena, overlap a cell that blocks tanks, overlap the base, or overlap another tank it isn't already overlapping. (The last rule lets tanks that ended up overlapping drive apart instead of locking together.)

**Lives.** The player starts with 2 reserve lives (3 tanks in total). When destroyed, the player respawns at the spawn point after 60 ticks, facing up, if a reserve life remains; otherwise the game ends (GAM-3).

**Shield.** On every spawn the player is shielded for 180 ticks. Enemy bullets that hit a shielded player are absorbed without harm.

## Acceptance criteria

- [x] **PLY-1** The player starts at the player spawn, facing up, with 2 reserve lives and a 180-tick shield.
- [x] **PLY-2** Holding a direction moves the player 1 px per tick in that direction and turns it to face that direction.
- [x] **PLY-3** If several directions are held, priority is up, down, left, right.
- [x] **PLY-4** Turning onto the perpendicular axis snaps the old axis's coordinate to the nearest multiple of 8, ties rounding up.
- [x] **PLY-5** Reversing on the same axis doesn't snap.
- [x] **PLY-6** The player can't leave the arena.
- [x] **PLY-7** The player stops flush against terrain that blocks tanks, and against the base.
- [x] **PLY-8** The player turns to face a pressed direction even when it can't move that way.
- [x] **PLY-9** The player can't drive into an enemy tank, but two tanks that already overlap can drive apart.
- [x] **PLY-10** With no direction held, the player stays still and keeps facing the same way.
- [x] **PLY-11** A shielded player hit by an enemy bullet is unharmed and the bullet is consumed. The shield counts down one per tick.
- [x] **PLY-12** A destroyed player with reserve lives loses one and respawns at the spawn point 60 ticks later, facing up and shielded for 180 ticks.
- [x] **PLY-13** A turn never snaps a tank into another tank. It snaps to the other grid line instead, or not at all if both would overlap.
