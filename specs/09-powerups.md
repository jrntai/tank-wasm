# 09 — Bonus tanks and power-ups

Bonus tanks drop power-ups. There are six types: a **star** upgrades the player's tank, and the other five have one-off or timed effects.

## Bonus tanks

The **4th, 11th and 18th** enemies in a stage's lineup spawn as bonus tanks and flash red. The **first hit from a bullet** on a bonus tank drops a power-up and removes its bonus status, even if the hit doesn't destroy it (Armor). Enemies outside those slots never drop anything.

## Power-ups on the field

A power-up is a 16×16 item. **Only one** exists at a time: a new drop replaces the current one.

**Type.** Each drop's type is drawn uniformly from the six types with the world's RNG.

**Placement.** The item is placed at a random position on the 8 px grid, fully inside the arena, not overlapping the base, and not covering any Steel or Water cell. The world's RNG draws up to 64 candidates; if none fits, the item goes on the player spawn point, which every stage keeps clear.

**Lifetime.** An uncollected item disappears after **600 ticks** (10 s). Its timer is frozen outside `Playing`, like everything else (GAM-5).

**Pickup.** The player collects an item by overlapping it. Enemies drive over items without collecting them.

## Star levels

Collecting a star raises the player's **level** by 1, up to 3. Levels stack:

| Level | Effect |
|---|---|
| 0 | Starting tank: normal bullets, one in flight |
| 1 | Fast bullets (5 px/tick) |
| 2 | Up to **two** bullets in flight. The second can only be fired once **8 ticks** have passed since the previous shot, so holding fire doesn't release them back to back. |
| 3 | Bullets also destroy **Steel** in their impact band (BUL-6) |

Losing a life resets the level to 0. Clearing a stage keeps it: the App carries the level into the next stage, as it does with lives (PRG-6).

## Other power-ups

| Type | Effect |
|---|---|
| Grenade | Destroys every enemy on the field. Each counts as a kill, so a grenade can clear the stage (GAM-4). Bonus tanks destroyed this way don't drop anything. |
| Helmet | Shields the player for **600 ticks**, or leaves a longer shield already running as it is. |
| Shovel | Turns the 8 base-wall cells to **Steel** for **1200 ticks**, then back to **Brick**. The wall is rebuilt in full, including cells that had been shot away. A cell a tank is standing on at that moment is left as it is, so no tank ends up inside a wall. |
| Clock | Freezes every enemy for **600 ticks**, including enemies that spawn during the freeze. Frozen enemies don't move, turn or fire. Bullets already in flight keep going. |
| Tank | Adds one reserve life, up to **9**. |

Picking up another shovel or clock restarts its timer. Like all timers, they stop outside `Playing` (GAM-5). They belong to the current world, so they don't carry over to the next stage.

The base-wall cells are row 23, columns 11–14, plus rows 24–25 in columns 11 and 14 (ARN-4).

## Acceptance criteria

- [x] **PWR-1** The 4th, 11th and 18th enemies spawn as bonus tanks; no others do.
- [x] **PWR-2** The first bullet hit on a bonus tank drops a power-up and clears its bonus status, even if the tank survives the hit. Hits on non-bonus tanks drop nothing.
- [x] **PWR-3** A dropped item lies on the 8 px grid, inside the arena, off the base and off steel and water. The same seed gives the same position.
- [x] **PWR-4** A new drop replaces any item already on the field.
- [x] **PWR-5** An uncollected item disappears after 600 ticks, and its timer doesn't run outside `Playing`.
- [x] **PWR-6** The player collects an item by overlapping it. A star raises the player's level by 1, up to 3. Enemies don't collect items.
- [x] **PWR-7** At level 1 and above, player bullets are fast.
- [x] **PWR-8** At level 2 and above, the player can have two bullets in flight, and the second needs 8 ticks since the previous shot. Below level 2 the limit stays one.
- [x] **PWR-9** At level 3, player bullets destroy steel in their impact band as well as brick. Lower levels and enemy bullets don't.
- [x] **PWR-10** Losing a life resets the level to 0.
- [x] **PWR-11** The level carries over to the next stage.
- [?] **PWR-12** Bonus tanks flash red, items blink, each of the six types has its own icon, and the player's tank looks different at each level. *(manual)*
- [x] **PWR-13** Each drop's type is one of the six, drawn uniformly from the RNG: all six appear over many drops, and the same seed gives the same type.
- [x] **PWR-14** A grenade destroys every enemy on the field, counts each as a kill without any bonus drops, and can clear the stage.
- [x] **PWR-15** A helmet shields the player for 600 ticks, without shortening a longer shield already running.
- [x] **PWR-16** A shovel turns the base wall to steel for 1200 ticks, then rebuilds it in full as brick. Cells covered by a tank are left as they are. Another shovel restarts the timer.
- [x] **PWR-17** A clock freezes all enemies, including new spawns, for 600 ticks. Their bullets in flight keep moving. Another clock restarts the timer.
- [x] **PWR-18** A tank power-up adds a reserve life, up to 9.
- [x] **PWR-19** Shovel and clock timers don't run outside `Playing`.

## Verification log

- 2026-10-12: all six item icons were rendered from the sprite art to a PNG and reviewed. Each is distinct: the extra-life icon got its own side-view tank so it isn't confused with the HUD's enemy counter. Still to check in a real browser: the icons blinking in play, the bonus flash and the level colors.
