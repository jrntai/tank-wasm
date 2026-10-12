#include "test.h"

using namespace sim;
using namespace test;

TEST("ENM-1", "enemies move at their type's speed") {
  struct Case { EnemyType type; int px_in_20_ticks; };
  for (Case c : {Case{EnemyType::Basic, 10}, Case{EnemyType::Fast, 30},
                 Case{EnemyType::Power, 20}, Case{EnemyType::Armor, 20}}) {
    World w = blank_world();
    w.enemy_random_actions = false;
    w.add_enemy(c.type, 0, 96, Dir::Right);
    run(w, 20);
    CHECK_EQ(w.enemies[0].x, c.px_in_20_ticks);
    CHECK_EQ(w.enemies[0].y, 96);
  }
}

TEST("ENM-2", "power enemies fire fast bullets, others normal") {
  for (EnemyType type : {EnemyType::Basic, EnemyType::Fast, EnemyType::Power, EnemyType::Armor}) {
    World w = blank_world(7);
    w.player_alive = false;  // keep the player out of the way
    w.respawn_timer = 1 << 30;
    w.add_enemy(type, 96, 96);
    for (int i = 0; i < 2000 && w.bullets.empty(); ++i) w.tick({});
    CHECK(!w.bullets.empty());
    if (w.bullets.empty()) continue;
    CHECK(w.bullets[0].side == Side::Enemy);
    CHECK_EQ(w.bullets[0].speed, type == EnemyType::Power ? kFastBulletSpeed : kNormalBulletSpeed);
  }
}

TEST("ENM-3", "armor takes 4 hits, others 1") {
  for (EnemyType type : {EnemyType::Basic, EnemyType::Fast, EnemyType::Power, EnemyType::Armor}) {
    World w = blank_world();
    w.enemy_random_actions = false;
    w.add_enemy(type, 96, 40, Dir::Up);  // blocked by nothing; drives up slowly
    const int hits_needed = type == EnemyType::Armor ? 4 : 1;
    for (int hit = 1; hit <= hits_needed; ++hit) {
      CHECK_EQ(w.enemies.size(), 1u);
      if (w.enemies.empty()) return;
      const Tank& e = w.enemies[0];
      shoot(w, e.x + 6, e.y + kTankSize + 2, Dir::Up, Side::Player, kFastBulletSpeed);
      w.tick({});
      CHECK(w.bullets.empty());
      if (hit < hits_needed) CHECK_EQ(w.enemies[0].hp, hits_needed - hit);
    }
    CHECK(w.enemies.empty());
  }
}

TEST("ENM-4", "first enemy spawns at center, facing down, on the first tick") {
  World w(stage_with_lineup({EnemyType::Fast, EnemyType::Basic}));
  w.enemy_random_actions = false;
  CHECK(w.enemies.empty());
  w.tick({});
  CHECK_EQ(w.enemies.size(), 1u);
  if (w.enemies.empty()) return;
  CHECK(w.enemies[0].type == EnemyType::Fast);
  CHECK(w.enemies[0].facing == Dir::Down);
  CHECK_EQ(w.enemies[0].x, 96);
  CHECK_EQ(w.enemies[0].y, 1);  // Fast: 3 half-px on its first tick
}

TEST("ENM-5", "spawn points cycle center, right, left, 120 ticks apart") {
  World w(stage_with_lineup({EnemyType::Basic, EnemyType::Basic, EnemyType::Basic,
                             EnemyType::Basic}));
  w.enemy_random_actions = false;
  std::vector<std::pair<uint32_t, int>> spawns;  // (tick, x)
  size_t count = 0;
  for (int i = 0; i < 400; ++i) {
    w.tick({});
    if (w.enemies.size() > count) spawns.push_back({w.tick_count, w.enemies.back().x});
    count = w.enemies.size();
  }
  CHECK_EQ(spawns.size(), 4u);
  if (spawns.size() != 4) return;
  CHECK(spawns[0] == std::make_pair(1u, 96));
  CHECK(spawns[1] == std::make_pair(121u, 192));
  CHECK(spawns[2] == std::make_pair(241u, 0));
  CHECK(spawns[3] == std::make_pair(361u, 96));
}

TEST("ENM-6", "at most 4 enemies on the field, each lineup entry spawns once") {
  World w(stage_with_lineup(std::vector<EnemyType>(6, EnemyType::Basic)));
  w.enemy_random_actions = false;
  for (int i = 0; i < 1000; ++i) {
    w.tick({});
    CHECK(w.enemies.size() <= 4);
  }
  CHECK_EQ(w.enemies.size(), 4u);
  CHECK_EQ(w.enemies_waiting(), 2);

  w.enemies[0].hp = 0;  // removed at the end of the next tick
  w.enemies[1].hp = 0;
  for (int i = 0; i < 1000; ++i) w.tick({});
  CHECK_EQ(w.enemies.size(), 4u);
  CHECK_EQ(w.enemies_waiting(), 0);
}

TEST("ENM-7", "spawn waits while a tank covers the spawn point") {
  World w(stage_with_lineup({EnemyType::Basic}));
  w.player.x = 100;  // overlaps the center spawn (96, 0)
  w.player.y = 10;
  run(w, 50);
  CHECK(w.enemies.empty());
  w.player.y = 40;
  w.tick({});
  CHECK_EQ(w.enemies.size(), 1u);
  if (!w.enemies.empty()) CHECK_EQ(w.enemies[0].x, 96);
}

TEST("ENM-8", "a blocked enemy turns to a different direction the same tick") {
  for (uint32_t seed = 1; seed <= 50; ++seed) {
    World w = blank_world(seed);
    w.enemy_random_actions = false;
    w.add_enemy(EnemyType::Power, 0, 0, Dir::Up);
    w.tick({});
    CHECK(w.enemies[0].facing != Dir::Up);
    CHECK_EQ(w.enemies[0].x, 0);
    CHECK_EQ(w.enemies[0].y, 0);
  }
}

TEST("ENM-9", "an enemy never has more than one bullet in flight") {
  World w = blank_world(3);
  w.player_alive = false;
  w.respawn_timer = 1 << 30;
  w.base.alive = false;  // let shots through the base instead of ending the game
  const int id = w.add_enemy(EnemyType::Power, 96, 64).id;
  int fired = 0;
  for (int i = 0; i < 3000 && w.phase == Phase::Playing; ++i) {
    const bool had = w.bullet_in_flight(id);
    w.tick({});
    int mine = 0;
    for (const auto& b : w.bullets) mine += b.owner_id == id;
    CHECK(mine <= 1);
    if (!had && mine) ++fired;
  }
  CHECK(fired > 1);
}

TEST("ENM-10", "same seed and inputs give identical worlds") {
  World a(stage1(), 42), b(stage1(), 42);
  for (int i = 0; i < 3000; ++i) {
    Input in;
    in.up = (i / 50) % 4 == 0;
    in.left = (i / 50) % 4 == 1;
    in.down = (i / 50) % 4 == 2;
    in.right = (i / 50) % 4 == 3;
    in.fire = i % 7 == 0;
    a.tick(in);
    b.tick(in);
  }
  CHECK(a.tick_count == b.tick_count);
  CHECK(a.phase == b.phase);
  CHECK(a.kills == b.kills);
  CHECK(a.terrain == b.terrain);
  CHECK(a.player.x == b.player.x && a.player.y == b.player.y);
  CHECK_EQ(a.enemies.size(), b.enemies.size());
  for (size_t i = 0; i < a.enemies.size() && i < b.enemies.size(); ++i)
    CHECK(a.enemies[i].x == b.enemies[i].x && a.enemies[i].y == b.enemies[i].y &&
          a.enemies[i].facing == b.enemies[i].facing);
  CHECK_EQ(a.bullets.size(), b.bullets.size());

  World c(stage1(), 43);  // a different seed should diverge
  for (int i = 0; i < 3000; ++i) c.tick({});
  World d(stage1(), 42);
  for (int i = 0; i < 3000; ++i) d.tick({});
  CHECK(c.rng_state != d.rng_state);
}
