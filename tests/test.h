#pragma once

// Minimal test registry. Each test is named after the spec criterion it covers:
//   TEST("PLY-4", "turning snaps to the 8 px grid") { CHECK(...); }
// Run `sim_tests [filter...]` to run only tests whose ID starts with a filter.

#include <cmath>
#include <cstdio>
#include <vector>

#include "sim/app.h"
#include "sim/world.h"

namespace test {

struct Case {
  const char* id;
  const char* name;
  void (*fn)();
};

std::vector<Case>& registry();
void fail(const char* file, int line, const char* expr);

struct Registrar {
  Registrar(const char* id, const char* name, void (*fn)()) { registry().push_back({id, name, fn}); }
};

// A world on the blank test stage with no enemies and no player shield.
inline sim::World blank_world(uint32_t seed = 1) {
  sim::World w(sim::empty_stage(), seed);
  w.player.shield_ticks = 0;
  return w;
}

inline sim::Input press(sim::Dir d, bool fire = false) {
  sim::Input in;
  in.up = d == sim::Dir::Up;
  in.down = d == sim::Dir::Down;
  in.left = d == sim::Dir::Left;
  in.right = d == sim::Dir::Right;
  in.fire = fire;
  return in;
}

inline sim::Input fire_only() {
  sim::Input in;
  in.fire = true;
  return in;
}

inline void run(sim::World& w, int ticks, const sim::Input& in = {}) {
  for (int i = 0; i < ticks; ++i) w.tick(in);
}

// Fills the cells covering a px rect.
inline void fill(sim::World& w, int x, int y, int width, int height, sim::Terrain t) {
  for (int cy = y / sim::kCellSize; cy < (y + height) / sim::kCellSize; ++cy)
    for (int cx = x / sim::kCellSize; cx < (x + width) / sim::kCellSize; ++cx)
      w.set_terrain(cx, cy, t);
}

}  // namespace test

#define TEST_CAT2(a, b) a##b
#define TEST_CAT(a, b) TEST_CAT2(a, b)
#define TEST(id, name)                                                         \
  static void TEST_CAT(test_fn_, __LINE__)();                                  \
  static test::Registrar TEST_CAT(test_reg_, __LINE__)(id, name, TEST_CAT(test_fn_, __LINE__)); \
  static void TEST_CAT(test_fn_, __LINE__)()

#define CHECK(cond)                                         \
  do {                                                      \
    if (!(cond)) test::fail(__FILE__, __LINE__, #cond);     \
  } while (0)
#define CHECK_EQ(a, b) CHECK((a) == (b))

namespace test {

inline sim::Bullet& shoot(sim::World& w, int x, int y, sim::Dir d, sim::Side side,
                          int speed = sim::kNormalBulletSpeed, int owner_id = 9999) {
  sim::Bullet b;
  b.x = x;
  b.y = y;
  b.dir = d;
  b.side = side;
  b.speed = speed;
  b.owner_id = owner_id;
  w.bullets.push_back(b);
  return w.bullets.back();
}

// Taps a key: pressed for one tick, released the next.
inline void tap(sim::App& app, bool sim::Input::*k) {
  sim::Input in;
  in.*k = true;
  app.update(in);
  app.update({});
}

// Title -> Stage Select -> play the selected stage (stage 1 in a new app).
inline void start_game(sim::App& app) {
  tap(app, &sim::Input::start);
  tap(app, &sim::Input::start);
}

inline sim::Stage stage_with_lineup(std::vector<sim::EnemyType> lineup) {
  sim::Stage s = sim::empty_stage();
  s.lineup = std::move(lineup);
  return s;
}

}  // namespace test
