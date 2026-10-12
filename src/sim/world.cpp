#include "sim/world.h"

#include <algorithm>

namespace sim {

namespace {

bool overlaps(int ax, int ay, int aw, int ah, int bx, int by, int bw, int bh) {
  return ax < bx + bw && bx < ax + aw && ay < by + bh && by < ay + ah;
}

bool tanks_overlap(int ax, int ay, int bx, int by) {
  return overlaps(ax, ay, kTankSize, kTankSize, bx, by, kTankSize, kTankSize);
}

bool vertical(Dir d) { return d == Dir::Up || d == Dir::Down; }

void step(Dir d, int& x, int& y) {
  switch (d) {
    case Dir::Up: --y; break;
    case Dir::Down: ++y; break;
    case Dir::Left: --x; break;
    case Dir::Right: ++x; break;
  }
}

// PLY-4: nearest multiple of 8, ties round up. Positions are never negative.
int snap8(int v) { return (v + kCellSize / 2) / kCellSize * kCellSize; }

Terrain terrain_from_char(char c) {
  switch (c) {
    case '#': return Terrain::Brick;
    case '@': return Terrain::Steel;
    case '~': return Terrain::Water;
    case '%': return Terrain::Trees;
    case '-': return Terrain::Ice;
    default: return Terrain::Empty;
  }
}

// ENM AI direction table.
constexpr Dir kRandomDirs[8] = {Dir::Down, Dir::Down, Dir::Down, Dir::Left,
                                Dir::Right, Dir::Left, Dir::Right, Dir::Up};

}  // namespace

bool blocks_tanks(Terrain t) {
  return t == Terrain::Brick || t == Terrain::Steel || t == Terrain::Water;
}

bool blocks_bullets(Terrain t) { return t == Terrain::Brick || t == Terrain::Steel; }

const EnemyStats& stats(EnemyType type) {
  static const EnemyStats table[] = {
      {1, kNormalBulletSpeed, 1},  // Basic
      {3, kNormalBulletSpeed, 1},  // Fast
      {2, kFastBulletSpeed, 1},    // Power
      {2, kNormalBulletSpeed, 4},  // Armor
  };
  return table[static_cast<int>(type)];
}

World::World(const Stage& stage, uint32_t seed)
    : stage_number(stage.number), rng_state(seed ? seed : 1), lineup(stage.lineup) {
  for (int cy = 0; cy < kGridSize; ++cy)
    for (int cx = 0; cx < kGridSize; ++cx)
      set_terrain(cx, cy, terrain_from_char(stage.rows[cy][cx]));
  player.side = Side::Player;
  spawn_player();
}

uint32_t World::random() {
  // xorshift32
  uint32_t x = rng_state;
  x ^= x << 13;
  x ^= x >> 17;
  x ^= x << 5;
  return rng_state = x;
}

bool World::bullet_in_flight(int owner_id) const { return bullets_in_flight(owner_id) > 0; }

int World::bullets_in_flight(int owner_id) const {
  return int(std::count_if(bullets.begin(), bullets.end(),
                           [&](const Bullet& b) { return b.alive && b.owner_id == owner_id; }));
}

Tank& World::add_enemy(EnemyType type, int x, int y, Dir facing) {
  Tank e;
  e.id = next_id_++;
  e.x = x;
  e.y = y;
  e.facing = facing;
  e.side = Side::Enemy;
  e.type = type;
  e.hp = stats(type).hp;
  enemies.push_back(e);
  return enemies.back();
}

void World::spawn_player() {
  player.id = next_id_++;  // a new tank: the old one's bullet no longer counts (BUL-2)
  player.x = kPlayerSpawnX;
  player.y = kPlayerSpawnY;
  player.facing = Dir::Up;
  player.move_acc = 0;
  player.shield_ticks = kShieldTicks;
  player_alive = true;
}

void World::destroy_player() {
  if (!player_alive) return;
  player_alive = false;
  player.level = 0;  // PWR-10
  add_explosion(player.x + kTankSize / 2, player.y + kTankSize / 2, true);
  if (lives > 0) {
    --lives;
    respawn_timer = kRespawnDelay;
  } else {
    phase = Phase::GameOver;  // GAM-3
  }
}

void World::destroy_base() {
  if (!base.alive) return;
  base.alive = false;
  add_explosion(base.x + kTankSize / 2, base.y + kTankSize / 2, true);
  phase = Phase::GameOver;  // GAM-2
}

void World::add_explosion(int cx, int cy, bool big) {
  explosions.push_back({cx, cy, big, big ? kBigExplosionTicks : kSmallExplosionTicks});
}

void World::tick(const Input& in) {
  ++tick_count;

  // GAM-6: explosions age in every phase.
  for (auto& e : explosions) --e.ticks_left;
  explosions.erase(std::remove_if(explosions.begin(), explosions.end(),
                                  [](const Explosion& e) { return e.ticks_left <= 0; }),
                   explosions.end());

  if (phase != Phase::Playing) return;  // GAM-5

  if (powerup && --powerup->ticks_left <= 0) powerup.reset();  // PWR-5
  if (shovel_ticks > 0 && --shovel_ticks == 0) set_base_wall(Terrain::Brick);  // PWR-16
  if (freeze_ticks > 0) --freeze_ticks;                                         // PWR-17

  update_spawning();
  update_player(in);
  update_enemies();
  update_bullets();

  bullets.erase(std::remove_if(bullets.begin(), bullets.end(),
                               [](const Bullet& b) { return !b.alive; }),
                bullets.end());
  enemies.erase(std::remove_if(enemies.begin(), enemies.end(),
                               [](const Tank& e) { return e.hp <= 0; }),
                enemies.end());

  // GAM-4
  if (phase == Phase::Playing && !lineup.empty() && kills == int(lineup.size()))
    phase = Phase::StageClear;
}

void World::update_spawning() {
  if (spawn_timer > 0) --spawn_timer;
  if (spawn_timer > 0 || next_lineup_index >= int(lineup.size()) ||
      int(enemies.size()) >= kMaxEnemiesOnField)
    return;

  // ENM-7: wait while any tank overlaps the spawn point.
  const int sx = kEnemySpawnX[next_spawn_point], sy = kEnemySpawnY;
  if (player_alive && tanks_overlap(player.x, player.y, sx, sy)) return;
  for (const auto& e : enemies)
    if (tanks_overlap(e.x, e.y, sx, sy)) return;

  const int slot = next_lineup_index++;
  Tank& e = add_enemy(lineup[slot], sx, sy, Dir::Down);
  for (int bonus_slot : kBonusLineupSlots)
    if (slot == bonus_slot) e.bonus = true;  // PWR-1
  next_spawn_point = (next_spawn_point + 1) % kEnemySpawnCount;
  spawn_timer = kEnemySpawnInterval;
}

void World::update_player(const Input& in) {
  if (!player_alive) {
    if (--respawn_timer <= 0) spawn_player();  // PLY-12
    return;
  }
  if (player.shield_ticks > 0) --player.shield_ticks;
  if (player.fire_cooldown > 0) --player.fire_cooldown;

  // PLY-3: up, down, left, right priority.
  bool held = true;
  Dir d = player.facing;
  if (in.up) d = Dir::Up;
  else if (in.down) d = Dir::Down;
  else if (in.left) d = Dir::Left;
  else if (in.right) d = Dir::Right;
  else held = false;

  if (held) {
    turn(player, d);
    move(player, kPlayerSpeed);
  }

  // PWR-6: collect an item by overlapping it.
  if (powerup && overlaps(player.x, player.y, kTankSize, kTankSize, powerup->x, powerup->y,
                          kPowerUpSize, kPowerUpSize)) {
    const PowerUpType type = powerup->type;
    powerup.reset();
    collect(type);
  }

  // BUL-2, PWR-7/8/9
  const int in_flight = bullets_in_flight(player.id);
  const int limit = player.level >= 2 ? 2 : 1;
  if (in.fire && in_flight < limit && (in_flight == 0 || player.fire_cooldown == 0)) {
    fire(player, player.level >= 1 ? kFastBulletSpeed : kNormalBulletSpeed, player.level >= 3);
    player.fire_cooldown = kSecondShotDelay;
  }
}

void World::update_enemies() {
  if (freeze_ticks > 0) return;  // PWR-17
  for (auto& e : enemies) {
    if (e.hp <= 0) continue;
    const EnemyStats& s = stats(e.type);
    if (move(e, s.speed) > 0) {
      // ENM-8: blocked, so pick a different direction.
      Dir d;
      do d = kRandomDirs[random() % 8];
      while (d == e.facing);
      turn(e, d);
    } else if (enemy_random_actions) {
      const int along = vertical(e.facing) ? e.y : e.x;
      if (along % kCellSize == 0 && random() % 16 == 0) turn(e, kRandomDirs[random() % 8]);
    }
    if (enemy_random_actions && !bullet_in_flight(e.id) && random() % 32 == 0) fire(e, s.bullet_speed);  // ENM-9
  }
}

void World::turn(Tank& t, Dir d) {
  if (vertical(d) != vertical(t.facing)) {
    // PLY-4: snap the axis we were moving on. Both grid lines around the tank are
    // terrain-safe, so only another tank can block a snap (PLY-13).
    int& v = vertical(t.facing) ? t.y : t.x;
    const int nearest = snap8(v);
    const int other = nearest > v ? nearest - kCellSize : nearest + kCellSize;
    for (int target : {nearest, other}) {
      if (target == v) break;
      const int nx = vertical(t.facing) ? t.x : target;
      const int ny = vertical(t.facing) ? target : t.y;
      if (can_occupy(t, nx, ny)) {
        v = target;
        break;
      }
    }
  }
  t.facing = d;
}

int World::move(Tank& t, int speed) {
  t.move_acc += speed;
  int px = t.move_acc / 2;
  t.move_acc %= 2;
  while (px > 0) {
    int nx = t.x, ny = t.y;
    step(t.facing, nx, ny);
    if (!can_occupy(t, nx, ny)) break;
    t.x = nx;
    t.y = ny;
    --px;
  }
  return px;
}

bool World::can_occupy(const Tank& t, int nx, int ny) const {
  // PLY-6
  if (nx < 0 || ny < 0 || nx + kTankSize > kArenaSize || ny + kTankSize > kArenaSize)
    return false;
  // PLY-7: terrain (ARN-2) and base.
  for (int cy = ny / kCellSize; cy <= (ny + kTankSize - 1) / kCellSize; ++cy)
    for (int cx = nx / kCellSize; cx <= (nx + kTankSize - 1) / kCellSize; ++cx)
      if (blocks_tanks(terrain_at(cx, cy))) return false;
  if (tanks_overlap(nx, ny, base.x, base.y)) return false;
  // PLY-9: blocked by tanks we'd newly overlap; free to separate from ones we already do.
  auto blocks = [&](const Tank& o) {
    return o.id != t.id && tanks_overlap(nx, ny, o.x, o.y) && !tanks_overlap(t.x, t.y, o.x, o.y);
  };
  if (player_alive && blocks(player)) return false;
  for (const auto& e : enemies)
    if (e.hp > 0 && blocks(e)) return false;
  return true;
}

void World::fire(Tank& t, int speed, bool breaks_steel) {
  // BUL-1
  Bullet b;
  switch (t.facing) {
    case Dir::Up: b.x = t.x + 6, b.y = t.y; break;
    case Dir::Down: b.x = t.x + 6, b.y = t.y + 12; break;
    case Dir::Left: b.x = t.x, b.y = t.y + 6; break;
    case Dir::Right: b.x = t.x + 12, b.y = t.y + 6; break;
  }
  b.dir = t.facing;
  b.side = t.side;
  b.owner_id = t.id;
  b.speed = speed;
  b.breaks_steel = breaks_steel;
  bullets.push_back(b);
}

void World::update_bullets() {
  // BUL-3/5/9: advance all bullets one px per sub-step, checking collisions each time.
  int max_speed = 0;
  for (const auto& b : bullets) max_speed = std::max(max_speed, b.speed);

  for (int s = 0; s < max_speed && phase == Phase::Playing; ++s) {
    for (auto& b : bullets) {
      if (!b.alive || s >= b.speed) continue;
      step(b.dir, b.x, b.y);
      if (resolve_bullet(b)) b.alive = false;
    }
    // BUL-9: opposing bullets cancel each other.
    for (auto& a : bullets) {
      if (!a.alive) continue;
      for (auto& b : bullets) {
        if (!b.alive || a.side == b.side) continue;
        if (overlaps(a.x, a.y, kBulletSize, kBulletSize, b.x, b.y, kBulletSize, kBulletSize)) {
          a.alive = b.alive = false;
          break;
        }
      }
    }
  }
}

bool World::resolve_bullet(Bullet& b) {
  const int bcx = b.x + kBulletSize / 2, bcy = b.y + kBulletSize / 2;

  // BUL-4
  if (b.x < 0 || b.y < 0 || b.x + kBulletSize > kArenaSize || b.y + kBulletSize > kArenaSize) {
    add_explosion(bcx, bcy, false);
    return true;
  }

  // BUL-6/7: terrain (ARN-3).
  const int c0 = b.x / kCellSize, c1 = (b.x + kBulletSize - 1) / kCellSize;
  const int r0 = b.y / kCellSize, r1 = (b.y + kBulletSize - 1) / kCellSize;
  bool hit = false;
  for (int r = r0; r <= r1; ++r)
    for (int c = c0; c <= c1; ++c)
      if (blocks_bullets(terrain_at(c, r))) hit = true;
  if (hit) {
    // Destroy bricks in the leading row/column, in the 16 px band across the path.
    auto in_band = [](int cell, int centre) {
      const int cell_centre = cell * kCellSize + kCellSize / 2;
      return cell_centre >= centre - 8 && cell_centre < centre + 8;
    };
    auto breakable = [&](Terrain t) {
      return t == Terrain::Brick || (b.breaks_steel && t == Terrain::Steel);  // PWR-9
    };
    if (vertical(b.dir)) {
      const int row = b.dir == Dir::Up ? r0 : r1;
      for (int c = 0; c < kGridSize; ++c)
        if (in_band(c, bcx) && breakable(terrain_at(c, row))) set_terrain(c, row, Terrain::Empty);
    } else {
      const int col = b.dir == Dir::Left ? c0 : c1;
      for (int r = 0; r < kGridSize; ++r)
        if (in_band(r, bcy) && breakable(terrain_at(col, r))) set_terrain(col, r, Terrain::Empty);
    }
    add_explosion(bcx, bcy, false);
    return true;
  }

  // GAM-2
  if (base.alive &&
      overlaps(b.x, b.y, kBulletSize, kBulletSize, base.x, base.y, kTankSize, kTankSize)) {
    add_explosion(bcx, bcy, false);
    destroy_base();
    return true;
  }

  // Tanks: player bullets hit enemies; enemy bullets hit the player (BUL-10).
  if (b.side == Side::Player) {
    for (auto& e : enemies) {
      if (e.hp > 0 &&
          overlaps(b.x, b.y, kBulletSize, kBulletSize, e.x, e.y, kTankSize, kTankSize)) {
        add_explosion(bcx, bcy, false);
        hit_enemy(e);
        return true;
      }
    }
  } else if (player_alive && overlaps(b.x, b.y, kBulletSize, kBulletSize, player.x, player.y,
                                      kTankSize, kTankSize)) {
    add_explosion(bcx, bcy, false);
    if (player.shield_ticks == 0) destroy_player();  // PLY-11
    return true;
  }
  return false;
}

void World::hit_enemy(Tank& e) {
  if (e.bonus) {  // PWR-2
    e.bonus = false;
    drop_powerup();
  }
  if (--e.hp > 0) return;  // ENM-3: armor absorbs the hit
  destroy_enemy(e);
}

void World::destroy_enemy(Tank& e) {
  e.hp = 0;
  ++kills;  // GAM-7
  add_explosion(e.x + kTankSize / 2, e.y + kTankSize / 2, true);
}

void World::collect(PowerUpType type) {
  switch (type) {
    case PowerUpType::Star:  // PWR-6
      player.level = std::min(player.level + 1, kMaxPlayerLevel);
      break;
    case PowerUpType::Grenade:  // PWR-14: no bonus drops
      for (auto& e : enemies)
        if (e.hp > 0) destroy_enemy(e);
      break;
    case PowerUpType::Helmet:  // PWR-15
      player.shield_ticks = std::max(player.shield_ticks, kHelmetTicks);
      break;
    case PowerUpType::Shovel:  // PWR-16
      shovel_ticks = kShovelTicks;
      set_base_wall(Terrain::Steel);
      break;
    case PowerUpType::Clock:  // PWR-17
      freeze_ticks = kClockTicks;
      break;
    case PowerUpType::Tank:  // PWR-18
      lives = std::min(lives + 1, kMaxLives);
      break;
  }
}

void World::set_base_wall(Terrain t) {
  // PWR-16: leave cells a tank is standing on, so nobody ends up inside a wall.
  auto covered = [&](int cx, int cy) {
    auto on = [&](const Tank& k) {
      return overlaps(cx * kCellSize, cy * kCellSize, kCellSize, kCellSize, k.x, k.y, kTankSize,
                      kTankSize);
    };
    if (player_alive && on(player)) return true;
    for (const auto& e : enemies)
      if (e.hp > 0 && on(e)) return true;
    return false;
  };
  for (auto [cx, cy] : kBaseWallCells)
    if (!covered(cx, cy)) set_terrain(cx, cy, t);
}

void World::drop_powerup() {
  // PWR-3: random 8 px grid position, inside the arena, off the base, steel and water.
  constexpr int kSlots = (kArenaSize - kPowerUpSize) / kCellSize + 1;
  auto valid = [&](int x, int y) {
    if (overlaps(x, y, kPowerUpSize, kPowerUpSize, base.x, base.y, kTankSize, kTankSize))
      return false;
    for (int cy = y / kCellSize; cy < (y + kPowerUpSize) / kCellSize; ++cy)
      for (int cx = x / kCellSize; cx < (x + kPowerUpSize) / kCellSize; ++cx) {
        const Terrain t = terrain_at(cx, cy);
        if (t == Terrain::Steel || t == Terrain::Water) return false;
      }
    return true;
  };
  PowerUp p{kPlayerSpawnX, kPlayerSpawnY, kPowerUpTicks};
  for (int attempt = 0; attempt < 64; ++attempt) {
    const int x = int(random() % kSlots) * kCellSize;
    const int y = int(random() % kSlots) * kCellSize;
    if (valid(x, y)) {
      p.x = x;
      p.y = y;
      break;
    }
  }
  p.type = static_cast<PowerUpType>(random() % kPowerUpTypeCount);  // PWR-13
  powerup = p;  // PWR-4: replaces any current item
}

}  // namespace sim
