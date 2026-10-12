#include <cstdlib>

#include "test.h"

using namespace sim;
using namespace test;

TEST("PLY-1", "player starts at spawn, facing up, with lives and shield") {
  World w(stage1());
  CHECK(w.player_alive);
  CHECK_EQ(w.player.x, kPlayerSpawnX);
  CHECK_EQ(w.player.y, kPlayerSpawnY);
  CHECK(w.player.facing == Dir::Up);
  CHECK_EQ(w.lives, 2);
  CHECK_EQ(w.player.shield_ticks, 180);
}

TEST("PLY-2", "holding a direction moves 1 px per tick and faces it") {
  struct Case { Dir d; int dx, dy; };
  for (Case c : {Case{Dir::Up, 0, -10}, Case{Dir::Down, 0, 10}, Case{Dir::Left, -10, 0},
                 Case{Dir::Right, 10, 0}}) {
    World w = blank_world();
    w.player.x = 64;
    w.player.y = 96;
    run(w, 10, press(c.d));
    CHECK_EQ(w.player.x, 64 + c.dx);
    CHECK_EQ(w.player.y, 96 + c.dy);
    CHECK(w.player.facing == c.d);
  }
}

TEST("PLY-3", "direction priority is up, down, left, right") {
  auto held = [](bool up, bool down, bool left, bool right) {
    World w = blank_world();
    w.player.x = 64;
    w.player.y = 96;
    w.player.facing = Dir::Up;
    Input in;
    in.up = up, in.down = down, in.left = left, in.right = right;
    w.tick(in);
    return w.player.facing;
  };
  CHECK(held(true, true, true, true) == Dir::Up);
  CHECK(held(false, true, true, true) == Dir::Down);
  CHECK(held(false, false, true, true) == Dir::Left);
  CHECK(held(true, false, false, true) == Dir::Up);
}

TEST("PLY-4", "perpendicular turns snap to the nearest multiple of 8") {
  struct Case { int y, snapped; };
  for (Case c : {Case{99, 96}, Case{100, 104}, Case{103, 104}, Case{97, 96}}) {
    World w = blank_world();
    w.player.x = 64;
    w.player.y = c.y;
    w.player.facing = Dir::Up;
    w.tick(press(Dir::Right));
    CHECK_EQ(w.player.y, c.snapped);
    CHECK_EQ(w.player.x, 65);
  }
  World w = blank_world();
  w.player.x = 67;
  w.player.y = 96;
  w.player.facing = Dir::Left;
  w.tick(press(Dir::Up));
  CHECK_EQ(w.player.x, 64);
  CHECK_EQ(w.player.y, 95);
}

TEST("PLY-5", "reversing on the same axis does not snap") {
  World w = blank_world();
  w.player.x = 64;
  w.player.y = 99;
  w.player.facing = Dir::Up;
  w.tick(press(Dir::Down));
  CHECK_EQ(w.player.y, 100);
  CHECK(w.player.facing == Dir::Down);
}

TEST("PLY-6", "player cannot leave the arena") {
  World w = blank_world();
  w.player.x = 0;
  w.player.y = 0;
  run(w, 5, press(Dir::Up));
  run(w, 5, press(Dir::Left));
  CHECK_EQ(w.player.x, 0);
  CHECK_EQ(w.player.y, 0);

  w.player.x = kArenaSize - kTankSize;
  w.player.y = kArenaSize - kTankSize;
  run(w, 5, press(Dir::Down));
  run(w, 5, press(Dir::Right));
  CHECK_EQ(w.player.x, kArenaSize - kTankSize);
  CHECK_EQ(w.player.y, kArenaSize - kTankSize);
}

TEST("PLY-7", "player stops flush against blocking terrain and the base") {
  for (Terrain t : {Terrain::Brick, Terrain::Steel, Terrain::Water}) {
    World w = blank_world();
    fill(w, 64, 80, 16, 8, t);  // cells (8..9, 10)
    w.player.x = 64;
    w.player.y = 100;
    run(w, 30, press(Dir::Up));
    CHECK_EQ(w.player.y, 88);
  }
  for (Terrain t : {Terrain::Trees, Terrain::Ice}) {
    World w = blank_world();
    fill(w, 64, 80, 16, 8, t);
    w.player.x = 64;
    w.player.y = 100;
    run(w, 30, press(Dir::Up));
    CHECK_EQ(w.player.y, 70);
  }
  World w = blank_world();  // spawn is left of the base
  run(w, 40, press(Dir::Right));
  CHECK_EQ(w.player.x, kBaseX - kTankSize);
}

TEST("PLY-8", "player faces a blocked direction without moving") {
  World w = blank_world();
  fill(w, 64, 80, 16, 8, Terrain::Steel);
  w.player.x = 64;
  w.player.y = 88;
  w.player.facing = Dir::Left;
  w.tick(press(Dir::Up));
  CHECK(w.player.facing == Dir::Up);
  CHECK_EQ(w.player.x, 64);
  CHECK_EQ(w.player.y, 88);
}

TEST("PLY-9", "tanks cannot drive into each other but can separate") {
  {
    World w = blank_world();
    w.enemy_random_actions = false;
    w.add_enemy(EnemyType::Power, 64, 20, Dir::Down);
    w.player.x = 64;
    w.player.y = 70;
    for (int i = 0; i < 60; ++i) {
      w.tick(press(Dir::Up));
      CHECK(std::abs(w.player.y - w.enemies[0].y) >= kTankSize ||
            std::abs(w.player.x - w.enemies[0].x) >= kTankSize);
    }
  }
  {
    World w = blank_world();
    w.enemy_random_actions = false;
    w.add_enemy(EnemyType::Basic, 64, 100, Dir::Up);
    w.player.x = 64;
    w.player.y = 90;  // overlapping by 6 px
    run(w, 20, press(Dir::Up));
    CHECK_EQ(w.player.y, 70);  // never held back while separating
  }
}

TEST("PLY-10", "no input means no movement") {
  World w = blank_world();
  w.player.x = 64;
  w.player.y = 99;
  w.player.facing = Dir::Left;
  run(w, 10);
  CHECK_EQ(w.player.x, 64);
  CHECK_EQ(w.player.y, 99);
  CHECK(w.player.facing == Dir::Left);
}

TEST("PLY-11", "shield absorbs enemy bullets and counts down") {
  World w = blank_world();
  w.player.shield_ticks = 100;
  w.player.x = 64;
  w.player.y = 96;
  shoot(w, 70, 84, Dir::Down, Side::Enemy);
  run(w, 4);
  CHECK(w.player_alive);
  CHECK(w.bullets.empty());
  CHECK_EQ(w.lives, 2);
  CHECK_EQ(w.player.shield_ticks, 96);
}

TEST("PLY-12", "destroyed player respawns after 60 ticks with a shield") {
  World w = blank_world();
  w.player.x = 150;
  w.player.y = 40;
  w.player.facing = Dir::Right;
  w.destroy_player();
  CHECK(!w.player_alive);
  CHECK_EQ(w.lives, 1);
  run(w, 59);
  CHECK(!w.player_alive);
  w.tick({});
  CHECK(w.player_alive);
  CHECK_EQ(w.player.x, kPlayerSpawnX);
  CHECK_EQ(w.player.y, kPlayerSpawnY);
  CHECK(w.player.facing == Dir::Up);
  CHECK_EQ(w.player.shield_ticks, kShieldTicks);
}

TEST("PLY-13", "a turn never snaps a tank into another tank") {
  // Enemy at y=53 heading down is blocked by the player at y=69. The nearest grid
  // line (56) would overlap the player, so it snaps the other way, to 48.
  for (uint32_t seed = 1; seed <= 20; ++seed) {
    World w = blank_world(seed);
    w.enemy_random_actions = false;
    w.add_enemy(EnemyType::Power, 64, 53, Dir::Down);
    w.player.x = 64;
    w.player.y = 69;
    w.tick({});
    const Tank& e = w.enemies[0];
    CHECK(e.facing != Dir::Down);
    CHECK_EQ(e.y, e.facing == Dir::Up ? 53 : 48);  // reversing never snaps (PLY-5)
  }

  // A second tank above blocks 48 as well, so the enemy turns without snapping.
  for (uint32_t seed = 1; seed <= 20; ++seed) {
    World w = blank_world(seed);
    w.enemy_random_actions = false;
    w.add_enemy(EnemyType::Power, 64, 53, Dir::Down);
    w.add_enemy(EnemyType::Power, 64, 36, Dir::Up);
    w.player.x = 64;
    w.player.y = 69;
    w.tick({});
    CHECK(w.enemies[0].facing != Dir::Down);
    CHECK_EQ(w.enemies[0].y, 53);
  }
}
