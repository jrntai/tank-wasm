# 03 — Bullets

## Behaviour

Bullets are 4×4 px. A tank can have **one bullet in flight** at a time; the exception is a player at star level 2 or higher, who can have two (PWR-8). A bullet keeps flying after its tank is destroyed, and still counts as that tank's bullet until it hits something.

**Firing.** A bullet appears centred on the tank's barrel, just inside its front edge:

| Facing | Bullet position |
|---|---|
| Up | (x+6, y) |
| Down | (x+6, y+12) |
| Left | (x, y+6) |
| Right | (x+12, y+6) |

The player fires while the fire button is held and fewer bullets than its limit are in flight. Holding fire fires again as soon as a slot frees up.

**Travel.** Speeds are in px per tick: normal bullets 3, fast bullets 5 (fired by Power enemies, ENM-2). Bullets move after tanks, so a bullet fired this tick also moves this tick. To avoid tunnelling, every bullet advances one px per sub-step, and collisions are checked after each sub-step. All bullets advance together: on sub-step *s*, every bullet with speed > *s* moves.

**Impacts.** After each px, in this order, a bullet:
1. leaving the arena is removed;
2. overlapping any Brick or Steel cell is removed and destroys bricks (below);
3. overlapping the base destroys it (GAM-2) and is removed;
4. overlapping an opposing tank is removed and hits that tank.

Then any two opposing bullets that overlap remove each other.

Player bullets hit enemies; enemy bullets hit the player. Enemy bullets pass through other enemies.

**Brick destruction.** On impact, the bullet destroys bricks in its **leading** row of cells (moving up or down) or column (moving left or right). That's the cells it just entered, one cell deep. Across the direction of travel, it destroys the cells whose centre is within 8 px of the bullet's centre line: a 16 px band, always exactly two cells. Steel in the same band is untouched, unless the bullet was fired by a level 3 player (PWR-9).

Every impact removal creates a small explosion at the bullet's centre (GAM-6). Bullets that cancel each other don't create one.

## Acceptance criteria

- [x] **BUL-1** Firing places the bullet per the table above, travelling in the tank's facing direction.
- [x] **BUL-2** A tank with as many bullets in flight as its limit allows (one, below star level 2) can't fire another. Once a bullet is gone, holding fire fires again.
- [x] **BUL-3** Normal bullets move 3 px per tick and fast bullets 5, including on the tick they're fired.
- [x] **BUL-4** A bullet leaving the arena is removed.
- [x] **BUL-5** A bullet can't skip past a one-cell brick wall, whatever its speed.
- [x] **BUL-6** A bullet hitting brick destroys the two cells across its path in the leading row or column and is removed. Cells farther along its path survive.
- [x] **BUL-7** A bullet hitting steel is removed and the steel stays, unless it was fired by a level 3 player (PWR-9).
- [x] **BUL-8** Bullets pass over Water, Trees and Ice.
- [x] **BUL-9** Opposing bullets that overlap remove each other, even head-on at combined speeds over 8 px per tick.
- [x] **BUL-10** Enemy bullets pass through other enemy tanks.
- [x] **BUL-11** A bullet keeps flying after its tank is destroyed.
