#include "test.h"

using namespace sim;
using namespace test;

TEST("BUL-1", "bullet spawns at the barrel and travels the facing direction") {
  struct Case { Dir d; int x, y; };
  // Player at (64, 96); a fired bullet also moves 3 px on its first tick.
  for (Case c : {Case{Dir::Up, 70, 96 - 3}, Case{Dir::Down, 70, 108 + 3},
                 Case{Dir::Left, 64 - 3, 102}, Case{Dir::Right, 76 + 3, 102}}) {
    World w = blank_world();
    w.player.x = 64;
    w.player.y = 96;
    w.player.facing = c.d;
    w.tick(fire_only());
    CHECK_EQ(w.bullets.size(), 1u);
    if (w.bullets.empty()) continue;
    CHECK_EQ(w.bullets[0].x, c.x);
    CHECK_EQ(w.bullets[0].y, c.y);
    CHECK(w.bullets[0].dir == c.d);
    CHECK(w.bullets[0].side == Side::Player);
  }
}

TEST("BUL-2", "one bullet in flight; holding fire refires once it is gone") {
  World w = blank_world();
  w.player.x = 64;
  w.player.y = 96;
  int refires = 0, empty_streak = 0;
  for (int i = 0; i < 100; ++i) {
    const bool had = !w.bullets.empty();
    w.tick(fire_only());
    CHECK(w.bullets.size() <= 1);
    if (!had && !w.bullets.empty() && i > 0) ++refires;
    // The bullet dies during the bullet phase; the refire comes on the next tick.
    empty_streak = w.bullets.empty() ? empty_streak + 1 : 0;
    CHECK(empty_streak <= 1);
  }
  CHECK(refires >= 2);
}

TEST("BUL-3", "normal bullets move 3 px per tick, fast bullets 5") {
  World w = blank_world();
  w.player.x = 64;
  w.player.y = 96;
  w.tick(fire_only());
  CHECK_EQ(w.bullets[0].y, 93);
  w.tick({});
  CHECK_EQ(w.bullets[0].y, 90);

  World f = blank_world();
  shoot(f, 100, 100, Dir::Right, Side::Enemy, kFastBulletSpeed);
  f.tick({});
  CHECK_EQ(f.bullets[0].x, 105);
  f.tick({});
  CHECK_EQ(f.bullets[0].x, 110);
}

TEST("BUL-4", "a bullet leaving the arena is removed") {
  World w = blank_world();
  shoot(w, 2, 100, Dir::Left, Side::Player);
  w.tick({});
  CHECK(w.bullets.empty());
  World e = blank_world();
  shoot(e, 100, kArenaSize - kBulletSize - 1, Dir::Down, Side::Enemy, kFastBulletSpeed);
  e.tick({});
  CHECK(e.bullets.empty());
}

TEST("BUL-5", "fast bullets cannot tunnel through a one-cell brick wall") {
  World w = blank_world();
  fill(w, 104, 0, 8, kArenaSize, Terrain::Brick);  // column 13
  shoot(w, 90, 100, Dir::Right, Side::Player, kFastBulletSpeed);
  for (int i = 0; i < 10; ++i) {
    w.tick({});
    for (const auto& b : w.bullets) CHECK(b.x + kBulletSize <= 108);
  }
  CHECK(w.bullets.empty());
  CHECK(w.terrain_at(13, 12) == Terrain::Empty);
  CHECK(w.terrain_at(13, 13) == Terrain::Empty);
  CHECK(w.terrain_at(13, 11) == Terrain::Brick);
  CHECK(w.terrain_at(13, 14) == Terrain::Brick);
}

TEST("BUL-6", "brick hit destroys two cells in the leading row or column") {
  {
    World w = blank_world();
    fill(w, 64, 32, 32, 32, Terrain::Brick);  // cells (8..11, 4..7)
    shoot(w, 70, 80, Dir::Up, Side::Player);  // centre x 72: band covers columns 8 and 9
    run(w, 10);
    CHECK(w.bullets.empty());
    CHECK(w.terrain_at(8, 7) == Terrain::Empty);
    CHECK(w.terrain_at(9, 7) == Terrain::Empty);
    CHECK(w.terrain_at(10, 7) == Terrain::Brick);
    CHECK(w.terrain_at(7, 7) == Terrain::Empty);  // was never brick
    CHECK(w.terrain_at(8, 6) == Terrain::Brick);
    CHECK(w.terrain_at(9, 6) == Terrain::Brick);
  }
  {
    World w = blank_world();
    fill(w, 64, 32, 32, 32, Terrain::Brick);
    shoot(w, 120, 37, Dir::Left, Side::Player);  // centre y 39: band covers rows 4 and 5
    run(w, 10);
    CHECK(w.bullets.empty());
    CHECK(w.terrain_at(11, 4) == Terrain::Empty);
    CHECK(w.terrain_at(11, 5) == Terrain::Empty);
    CHECK(w.terrain_at(11, 6) == Terrain::Brick);
    CHECK(w.terrain_at(10, 4) == Terrain::Brick);
  }
}

TEST("BUL-7", "steel stops bullets and survives") {
  World w = blank_world();
  fill(w, 64, 32, 16, 8, Terrain::Steel);
  fill(w, 80, 32, 8, 8, Terrain::Brick);  // brick beside steel, inside the band
  shoot(w, 78, 60, Dir::Up, Side::Player);  // centre 80: band covers columns 9 and 10
  run(w, 10);
  CHECK(w.bullets.empty());
  CHECK(w.terrain_at(8, 4) == Terrain::Steel);
  CHECK(w.terrain_at(9, 4) == Terrain::Steel);
  CHECK(w.terrain_at(10, 4) == Terrain::Empty);
}

TEST("BUL-8", "bullets pass over water, trees and ice") {
  World w = blank_world();
  fill(w, 72, 0, 8, kArenaSize, Terrain::Water);
  fill(w, 80, 0, 8, kArenaSize, Terrain::Trees);
  fill(w, 88, 0, 8, kArenaSize, Terrain::Ice);
  shoot(w, 60, 100, Dir::Right, Side::Player);
  run(w, 15);
  CHECK_EQ(w.bullets.size(), 1u);
  if (!w.bullets.empty()) CHECK_EQ(w.bullets[0].x, 105);
}

TEST("BUL-9", "opposing bullets cancel, even head-on at high speed") {
  World w = blank_world();
  shoot(w, 50, 100, Dir::Right, Side::Player, kFastBulletSpeed);
  shoot(w, 71, 100, Dir::Left, Side::Enemy, kFastBulletSpeed);  // odd gap
  run(w, 5);
  CHECK(w.bullets.empty());
  CHECK(w.explosions.empty());

  World same = blank_world();  // same side: no cancel
  shoot(same, 50, 100, Dir::Right, Side::Enemy);
  shoot(same, 60, 100, Dir::Left, Side::Enemy);
  run(same, 3);
  CHECK_EQ(same.bullets.size(), 2u);
}

TEST("BUL-10", "enemy bullets pass through enemy tanks") {
  World w = blank_world();
  w.enemy_random_actions = false;
  w.add_enemy(EnemyType::Basic, 100, 40);
  shoot(w, 106, 20, Dir::Down, Side::Enemy);
  run(w, 15);
  CHECK_EQ(w.bullets.size(), 1u);
  CHECK_EQ(w.enemies.size(), 1u);
  if (!w.enemies.empty()) CHECK_EQ(w.enemies[0].hp, 1);
}

TEST("BUL-11", "a bullet keeps flying after its tank is destroyed") {
  World w = blank_world();
  w.player.x = 64;
  w.player.y = 96;
  w.tick(fire_only());
  w.destroy_player();
  const int y = w.bullets[0].y;
  w.tick({});
  CHECK_EQ(w.bullets.size(), 1u);
  if (!w.bullets.empty()) CHECK_EQ(w.bullets[0].y, y - 3);

  World e = blank_world();
  e.enemy_random_actions = false;
  const int enemy_id = e.add_enemy(EnemyType::Basic, 100, 40).id;
  shoot(e, 106, 100, Dir::Down, Side::Enemy, kNormalBulletSpeed, enemy_id);
  shoot(e, 106, 60, Dir::Up, Side::Player);  // destroys the enemy
  run(e, 3);
  CHECK(e.enemies.empty());
  CHECK_EQ(e.bullets.size(), 1u);
  if (!e.bullets.empty()) CHECK_EQ(e.bullets[0].owner_id, enemy_id);
}
