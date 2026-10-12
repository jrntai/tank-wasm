#include <set>

#include "sim/app.h"
#include "test.h"

using namespace sim;
using namespace test;

namespace {

// A world where the player can't wander into items.
World world_without_player(uint32_t seed = 1) {
  World w = blank_world(seed);
  w.player_alive = false;
  w.respawn_timer = 1 << 30;
  return w;
}

bool covers(const World& w, int x, int y, Terrain t) {
  for (int cy = y / kCellSize; cy < (y + kPowerUpSize) / kCellSize; ++cy)
    for (int cx = x / kCellSize; cx < (x + kPowerUpSize) / kCellSize; ++cx)
      if (w.terrain_at(cx, cy) == t) return true;
  return false;
}

// Shoots enemy `e` from just below with a fast player bullet; resolves this tick.
void hit_from_below(World& w, const Tank& e) {
  shoot(w, e.x + 6, e.y + kTankSize + 2, Dir::Up, Side::Player, kFastBulletSpeed);
  w.tick({});
}

}  // namespace

TEST("PWR-1", "the 4th, 11th and 18th enemies are bonus tanks") {
  World w(stage_with_lineup(std::vector<EnemyType>(20, EnemyType::Basic)));
  w.enemy_random_actions = false;
  std::vector<bool> bonus;
  for (int i = 0; i < 5000 && bonus.size() < 20; ++i) {
    w.tick({});
    for (const auto& e : w.enemies) bonus.push_back(e.bonus);
    w.enemies.clear();  // make room for the next spawn
  }
  CHECK_EQ(bonus.size(), 20u);
  for (size_t i = 0; i < bonus.size(); ++i) CHECK_EQ(bonus[i], i == 3 || i == 10 || i == 17);
}

TEST("PWR-2", "the first hit on a bonus tank drops a star") {
  World w = blank_world();
  w.enemy_random_actions = false;
  w.add_enemy(EnemyType::Armor, 96, 40, Dir::Up).bonus = true;
  hit_from_below(w, w.enemies[0]);
  CHECK(w.powerup.has_value());
  CHECK_EQ(w.enemies.size(), 1u);  // armor survived the hit
  CHECK(!w.enemies[0].bonus);

  w.powerup.reset();
  hit_from_below(w, w.enemies[0]);
  CHECK(!w.powerup.has_value());  // no longer a bonus tank

  World plain = blank_world();
  plain.enemy_random_actions = false;
  plain.add_enemy(EnemyType::Basic, 96, 40, Dir::Up);
  hit_from_below(plain, plain.enemies[0]);
  CHECK(plain.enemies.empty());
  CHECK(!plain.powerup.has_value());
}

TEST("PWR-3", "items land on the grid, off the base, steel and water") {
  std::set<std::pair<int, int>> positions;
  for (uint32_t seed = 1; seed <= 300; ++seed) {
    World w = world_without_player(seed);
    fill(w, 0, 64, kArenaSize, 16, Terrain::Steel);
    fill(w, 0, 120, kArenaSize, 24, Terrain::Water);
    w.drop_powerup();
    CHECK(w.powerup.has_value());
    if (!w.powerup) continue;
    const auto [x, y] = std::pair{w.powerup->x, w.powerup->y};
    positions.insert({x, y});
    CHECK(x % 8 == 0 && y % 8 == 0);
    CHECK(x >= 0 && y >= 0 && x + kPowerUpSize <= kArenaSize && y + kPowerUpSize <= kArenaSize);
    CHECK(!(x < kBaseX + kTankSize && kBaseX < x + kPowerUpSize && y < kBaseY + kTankSize &&
            kBaseY < y + kPowerUpSize));
    CHECK(!covers(w, x, y, Terrain::Steel));
    CHECK(!covers(w, x, y, Terrain::Water));

    World again = world_without_player(seed);
    fill(again, 0, 64, kArenaSize, 16, Terrain::Steel);
    fill(again, 0, 120, kArenaSize, 24, Terrain::Water);
    again.drop_powerup();
    CHECK(again.powerup && again.powerup->x == x && again.powerup->y == y);
  }
  CHECK(positions.size() > 50);  // actually random

  World full = world_without_player();  // nowhere valid: falls back to the player spawn
  fill(full, 0, 0, kArenaSize, kArenaSize, Terrain::Steel);
  full.drop_powerup();
  CHECK(full.powerup && full.powerup->x == kPlayerSpawnX && full.powerup->y == kPlayerSpawnY);
}

TEST("PWR-4", "a new drop replaces the current item") {
  World w = world_without_player();
  w.drop_powerup();
  w.powerup->ticks_left = 5;
  w.drop_powerup();
  CHECK(w.powerup.has_value());
  CHECK_EQ(w.powerup->ticks_left, kPowerUpTicks);
}

TEST("PWR-5", "items expire after 600 ticks; frozen outside Playing") {
  World w = world_without_player();
  w.drop_powerup();
  run(w, kPowerUpTicks - 1);
  CHECK(w.powerup.has_value());
  w.tick({});
  CHECK(!w.powerup.has_value());

  World frozen = world_without_player();
  frozen.drop_powerup();
  frozen.phase = Phase::GameOver;
  run(frozen, 2 * kPowerUpTicks);
  CHECK(frozen.powerup && frozen.powerup->ticks_left == kPowerUpTicks);
}

TEST("PWR-6", "the player collects stars, up to level 3; enemies don't") {
  World w = blank_world();
  w.player.x = 64;
  w.player.y = 96;
  for (int expected : {1, 2, 3, 3}) {
    w.powerup = PowerUp{64, 80, kPowerUpTicks};
    w.tick(press(Dir::Up));
    CHECK(!w.powerup.has_value());
    CHECK_EQ(w.player.level, expected);
    w.player.y = 96;
  }

  World e = blank_world();
  e.enemy_random_actions = false;
  e.add_enemy(EnemyType::Power, 96, 40, Dir::Down);
  e.powerup = PowerUp{96, 64, kPowerUpTicks};
  run(e, 40);  // drives straight through the item
  CHECK(e.enemies[0].y > 64);
  CHECK(e.powerup.has_value());
  CHECK_EQ(e.player.level, 0);
}

TEST("PWR-7", "level 1 fires fast bullets") {
  for (int level : {0, 1, 2, 3}) {
    World w = blank_world();
    w.player.x = 64;
    w.player.y = 96;
    w.player.level = level;
    w.tick(fire_only());
    CHECK_EQ(w.bullets.size(), 1u);
    if (!w.bullets.empty())
      CHECK_EQ(w.bullets[0].speed, level >= 1 ? kFastBulletSpeed : kNormalBulletSpeed);
  }
}

TEST("PWR-8", "level 2 allows a second bullet 8 ticks after the first") {
  World w = blank_world();
  w.player.x = 64;
  w.player.y = 160;
  w.player.level = 2;
  run(w, 8, fire_only());
  CHECK_EQ(w.bullets.size(), 1u);
  w.tick(fire_only());
  CHECK_EQ(w.bullets.size(), 2u);
  for (int i = 0; i < 100; ++i) {
    w.tick(fire_only());
    CHECK(w.bullets.size() <= 2);
  }

  World one = blank_world();
  one.player.x = 64;
  one.player.y = 160;
  one.player.level = 1;
  for (int i = 0; i < 60; ++i) {
    one.tick(fire_only());
    CHECK(one.bullets.size() <= 1);
  }
}

TEST("PWR-9", "level 3 bullets break steel; others don't") {
  struct Case { int level; Side side; bool breaks; };
  for (Case c : {Case{3, Side::Player, true}, Case{2, Side::Player, false},
                 Case{0, Side::Enemy, false}}) {
    World w = blank_world();
    fill(w, 64, 32, 16, 16, Terrain::Steel);  // cells (8..9, 4..5)
    w.player.x = 64;
    w.player.y = 96;
    w.player.level = c.level;
    if (c.side == Side::Player) {
      w.tick(fire_only());
    } else {
      shoot(w, 70, 90, Dir::Up, Side::Enemy);
    }
    run(w, 20);
    CHECK(w.bullets.empty());
    const Terrain leading = c.breaks ? Terrain::Empty : Terrain::Steel;
    CHECK(w.terrain_at(8, 5) == leading);
    CHECK(w.terrain_at(9, 5) == leading);
    CHECK(w.terrain_at(8, 4) == Terrain::Steel);  // one cell deep
  }
}

TEST("PWR-10", "losing a life resets the level") {
  World w = blank_world();
  w.player.level = 2;
  w.destroy_player();
  run(w, kRespawnDelay);
  CHECK(w.player_alive);
  CHECK_EQ(w.player.level, 0);
}

TEST("PWR-11", "the level carries over to the next stage") {
  App app(1);
  start_game(app);
  app.world.player.level = 2;
  app.world.phase = Phase::StageClear;
  tap(app, &Input::start);
  CHECK_EQ(app.world.stage_number, 2);
  CHECK_EQ(app.world.player.level, 2);
}

namespace {

// Puts an item of `type` under the player and ticks once, collecting it.
void collect(World& w, PowerUpType type) {
  w.powerup = PowerUp{w.player.x, w.player.y, kPowerUpTicks, type};
  w.tick({});
}

// Stage 1's terrain, including the base wall, with no enemies.
World walled_world() {
  Stage s = stage1();
  s.lineup.clear();
  World w(s);
  w.player.shield_ticks = 0;
  return w;
}

bool wall_is(const World& w, Terrain t) {
  for (auto [cx, cy] : kBaseWallCells)
    if (w.terrain_at(cx, cy) != t) return false;
  return true;
}

}  // namespace

TEST("PWR-13", "item types are uniform over the six, seeded") {
  int counts[kPowerUpTypeCount] = {};
  for (uint32_t seed = 1; seed <= 600; ++seed) {
    World w = world_without_player(seed);
    w.drop_powerup();
    ++counts[int(w.powerup->type)];
    World again = world_without_player(seed);
    again.drop_powerup();
    CHECK(again.powerup->type == w.powerup->type);
  }
  for (int c : counts) CHECK(c >= 60 && c <= 140);  // ~100 each
}

TEST("PWR-14", "grenade destroys all enemies, counting kills, no bonus drops") {
  World w = blank_world();
  w.enemy_random_actions = false;
  w.add_enemy(EnemyType::Basic, 0, 40);
  w.add_enemy(EnemyType::Armor, 96, 40).bonus = true;
  w.add_enemy(EnemyType::Fast, 192, 40);
  collect(w, PowerUpType::Grenade);
  CHECK(w.enemies.empty());
  CHECK_EQ(w.kills, 3);
  CHECK(!w.powerup.has_value());
  int big = 0;
  for (const auto& e : w.explosions) big += e.big;
  CHECK_EQ(big, 3);

  World clear(stage_with_lineup({EnemyType::Basic}));
  clear.enemy_random_actions = false;
  clear.tick({});  // the only enemy spawns
  collect(clear, PowerUpType::Grenade);
  CHECK(clear.phase == Phase::StageClear);
}

TEST("PWR-15", "helmet shields for 600 ticks without shortening a longer shield") {
  World w = blank_world();
  collect(w, PowerUpType::Helmet);
  CHECK_EQ(w.player.shield_ticks, kHelmetTicks);

  World longer = blank_world();
  longer.player.shield_ticks = 1000;
  collect(longer, PowerUpType::Helmet);
  CHECK_EQ(longer.player.shield_ticks, 999);
}

TEST("PWR-16", "shovel steels the base wall, then rebuilds it as brick") {
  World w = walled_world();
  w.set_terrain(12, 23, Terrain::Empty);  // shot away earlier
  collect(w, PowerUpType::Shovel);
  CHECK(wall_is(w, Terrain::Steel));
  run(w, kShovelTicks - 1);
  CHECK(wall_is(w, Terrain::Steel));
  w.tick({});
  CHECK(wall_is(w, Terrain::Brick));

  // A tank standing in a gap keeps the gap.
  World gap = walled_world();
  gap.enemy_random_actions = false;
  gap.set_terrain(11, 23, Terrain::Empty);
  gap.add_enemy(EnemyType::Basic, 80, 176, Dir::Left);  // covers cell (11, 23)
  collect(gap, PowerUpType::Shovel);
  CHECK(gap.terrain_at(11, 23) == Terrain::Empty);
  CHECK(gap.terrain_at(12, 23) == Terrain::Steel);

  // A second shovel restarts the timer.
  World again = walled_world();
  collect(again, PowerUpType::Shovel);
  run(again, 600);
  collect(again, PowerUpType::Shovel);
  run(again, kShovelTicks - 1);
  CHECK(wall_is(again, Terrain::Steel));
}

TEST("PWR-17", "clock freezes enemies, including new spawns") {
  World w(stage_with_lineup({EnemyType::Power, EnemyType::Power}));
  collect(w, PowerUpType::Clock);  // the first enemy spawns this tick, then freezes
  shoot(w, 20, 100, Dir::Right, Side::Enemy);
  run(w, 10);
  CHECK_EQ(w.bullets.size(), 1u);
  if (!w.bullets.empty()) CHECK_EQ(w.bullets[0].x, 50);  // bullets in flight keep moving
  run(w, 290);  // the second enemy spawns at tick 121
  CHECK_EQ(w.enemies.size(), 2u);
  for (const auto& e : w.enemies) CHECK_EQ(e.y, 0);
  CHECK(w.bullets.empty());  // nobody fired
  run(w, kClockTicks - 301);
  for (const auto& e : w.enemies) CHECK_EQ(e.y, 0);
  run(w, 2);
  bool moved = false;
  for (const auto& e : w.enemies) moved |= e.y != 0 || e.facing != Dir::Down;
  CHECK(moved);

  World again = blank_world();
  again.enemy_random_actions = false;
  again.add_enemy(EnemyType::Power, 96, 40);
  collect(again, PowerUpType::Clock);
  run(again, 300);
  collect(again, PowerUpType::Clock);
  run(again, kClockTicks - 1);
  CHECK_EQ(again.enemies[0].y, 40);
}

TEST("PWR-18", "tank power-up adds a life, up to 9") {
  World w = blank_world();
  collect(w, PowerUpType::Tank);
  CHECK_EQ(w.lives, kPlayerReserveLives + 1);
  w.lives = kMaxLives;
  collect(w, PowerUpType::Tank);
  CHECK_EQ(w.lives, kMaxLives);
}

TEST("PWR-19", "shovel and clock timers stop outside Playing") {
  World w = walled_world();
  collect(w, PowerUpType::Shovel);
  collect(w, PowerUpType::Clock);
  const int shovel = w.shovel_ticks, freeze = w.freeze_ticks;
  w.phase = Phase::GameOver;
  run(w, 3000);
  CHECK_EQ(w.shovel_ticks, shovel);
  CHECK_EQ(w.freeze_ticks, freeze);
  CHECK(wall_is(w, Terrain::Steel));
}
