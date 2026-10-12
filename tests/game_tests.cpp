#include "test.h"

using namespace sim;
using namespace test;

TEST("GAM-1", "a new world is playing with the base alive") {
  World w(stage1());
  CHECK(w.phase == Phase::Playing);
  CHECK(w.base.alive);
}

TEST("GAM-2", "any bullet hitting the base ends the game") {
  for (Side side : {Side::Player, Side::Enemy}) {
    World w = blank_world();
    shoot(w, 90, 198, Dir::Right, side);
    run(w, 2);
    CHECK(!w.base.alive);
    CHECK(w.phase == Phase::GameOver);
    CHECK(w.bullets.empty());
  }
}

TEST("GAM-3", "losing the last life ends the game") {
  World w = blank_world();
  w.lives = 0;
  w.player.x = 64;
  w.player.y = 96;
  shoot(w, 70, 84, Dir::Down, Side::Enemy);
  run(w, 4);
  CHECK(!w.player_alive);
  CHECK(w.phase == Phase::GameOver);

  World r = blank_world();  // with a life left, the game goes on
  r.destroy_player();
  CHECK(r.phase == Phase::Playing);
}

TEST("GAM-4", "destroying the whole lineup clears the stage") {
  World w(stage_with_lineup({EnemyType::Basic}));
  w.enemy_random_actions = false;
  w.tick({});
  CHECK_EQ(w.enemies.size(), 1u);
  const Tank& e = w.enemies[0];
  shoot(w, e.x + 6, e.y + kTankSize + 2, Dir::Up, Side::Player, kFastBulletSpeed);
  w.tick({});
  CHECK(w.phase == Phase::StageClear);

  World empty = blank_world();
  run(empty, 300);
  CHECK(empty.phase == Phase::Playing);
}

TEST("GAM-5", "outside Playing everything but explosions is frozen") {
  World w(stage_with_lineup(std::vector<EnemyType>(4, EnemyType::Fast)));
  run(w, 130);  // two enemies on the field
  shoot(w, 20, 100, Dir::Right, Side::Enemy);
  shoot(w, 90, 198, Dir::Right, Side::Enemy);  // hits the base
  w.tick({});
  CHECK(w.phase == Phase::GameOver);
  CHECK(!w.explosions.empty());

  const World before = w;
  Input in;
  in.up = in.fire = true;
  run(w, kBigExplosionTicks, in);
  CHECK(w.player.x == before.player.x && w.player.y == before.player.y);
  CHECK_EQ(w.enemies.size(), before.enemies.size());
  for (size_t i = 0; i < w.enemies.size() && i < before.enemies.size(); ++i)
    CHECK(w.enemies[i].x == before.enemies[i].x && w.enemies[i].y == before.enemies[i].y);
  CHECK_EQ(w.bullets.size(), before.bullets.size());
  for (size_t i = 0; i < w.bullets.size() && i < before.bullets.size(); ++i)
    CHECK(w.bullets[i].x == before.bullets[i].x);
  CHECK_EQ(w.enemies_waiting(), before.enemies_waiting());
  CHECK(w.explosions.empty());
  CHECK(w.tick_count == before.tick_count + kBigExplosionTicks);
}

TEST("GAM-6", "impacts make small explosions, destruction makes big ones") {
  World w = blank_world();
  fill(w, 64, 32, 8, 8, Terrain::Steel);
  shoot(w, 66, 60, Dir::Up, Side::Player);
  run(w, 10);
  CHECK_EQ(w.explosions.size(), 1u);
  if (w.explosions.empty()) return;
  CHECK(!w.explosions[0].big);
  CHECK(w.explosions[0].ticks_left <= kSmallExplosionTicks);
  run(w, kSmallExplosionTicks);
  CHECK(w.explosions.empty());

  World k = blank_world();
  k.enemy_random_actions = false;
  k.add_enemy(EnemyType::Basic, 96, 40, Dir::Up);
  shoot(k, 102, 58, Dir::Up, Side::Player);
  k.tick({});
  CHECK_EQ(k.explosions.size(), 2u);
  int big = 0;
  for (const auto& e : k.explosions) {
    if (!e.big) continue;
    ++big;
    CHECK_EQ(e.ticks_left, kBigExplosionTicks);
  }
  CHECK_EQ(big, 1);
  run(k, kBigExplosionTicks);
  CHECK(k.explosions.empty());
}

TEST("GAM-7", "kills count destroyed enemies only") {
  World w = blank_world();
  w.enemy_random_actions = false;
  w.add_enemy(EnemyType::Armor, 96, 40, Dir::Up);
  shoot(w, 102, 58, Dir::Up, Side::Player);
  w.tick({});
  CHECK_EQ(w.kills, 0);
  w.add_enemy(EnemyType::Basic, 150, 40, Dir::Up);
  shoot(w, 156, 58, Dir::Up, Side::Player);
  w.tick({});
  CHECK_EQ(w.kills, 1);
}
