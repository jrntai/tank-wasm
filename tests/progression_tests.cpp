#include <cstring>
#include <queue>
#include <set>

#include "sim/app.h"
#include "test.h"

using namespace sim;
using test::start_game;
using test::tap;

namespace {

void skip_intro(App& app) {
  for (int i = 0; i < kStageIntroTicks; ++i) app.update({});
}

// From the title, into stage 1 play.
App playing_app(uint32_t seed = 1) {
  App app(seed);
  start_game(app);
  return app;
}

// Clears the current stage and starts the next one's intro.
void clear_stage(App& app) {
  app.world.phase = Phase::StageClear;
  tap(app, &Input::start);
}

bool tile_clear(const World& w, int px, int py) {
  for (int cy = py / kCellSize; cy < (py + kTankSize) / kCellSize; ++cy)
    for (int cx = px / kCellSize; cx < (px + kTankSize) / kCellSize; ++cx)
      if (w.terrain_at(cx, cy) != Terrain::Empty) return false;
  return true;
}

int threat(const Stage& s) {
  int t = 0;
  for (EnemyType e : s.lineup) t += 1 + static_cast<int>(e);  // Basic 1 ... Armor 4
  return t;
}

}  // namespace

TEST("PRG-1", "ten stages, numbered 1 to 10, 20 enemies each") {
  CHECK_EQ(stages().size(), 10u);
  for (size_t i = 0; i < stages().size(); ++i) {
    CHECK_EQ(stages()[i].number, int(i) + 1);
    CHECK_EQ(stages()[i].lineup.size(), 20u);
  }
}

TEST("PRG-2", "every stage is well-formed, keeps spawns clear and walls the base") {
  for (const Stage& s : stages()) {
    for (const auto& row : s.rows) {
      CHECK_EQ(row.size(), size_t(kGridSize));
      for (char c : row) CHECK(std::strchr(".#@~%-", c) != nullptr);
    }
    World w(s);
    CHECK(tile_clear(w, kBaseX, kBaseY));
    CHECK(tile_clear(w, kPlayerSpawnX, kPlayerSpawnY));
    for (int x : kEnemySpawnX) CHECK(tile_clear(w, x, kEnemySpawnY));
    for (int cx = 11; cx <= 14; ++cx) CHECK(w.terrain_at(cx, 23) == Terrain::Brick);
    for (int cy = 24; cy <= 25; ++cy) {
      CHECK(w.terrain_at(11, cy) == Terrain::Brick);
      CHECK(w.terrain_at(14, cy) == Terrain::Brick);
    }
  }
}

TEST("PRG-3", "every spawn can reach the player spawn and the base wall") {
  for (const Stage& s : stages()) {
    World w(s);
    // Tank positions on the 8 px grid. Brick is passable (it can be shot); steel and water aren't.
    auto open = [&](int x, int y) {
      if (x < 0 || y < 0 || x + kTankSize > kArenaSize || y + kTankSize > kArenaSize) return false;
      if (x < kBaseX + kTankSize && kBaseX < x + kTankSize && y < kBaseY + kTankSize &&
          kBaseY < y + kTankSize)
        return false;
      for (int cy = y / kCellSize; cy < (y + kTankSize) / kCellSize; ++cy)
        for (int cx = x / kCellSize; cx < (x + kTankSize) / kCellSize; ++cx) {
          const Terrain t = w.terrain_at(cx, cy);
          if (t == Terrain::Steel || t == Terrain::Water) return false;
        }
      return true;
    };
    for (int sx : kEnemySpawnX) {
      std::set<std::pair<int, int>> seen{{sx, kEnemySpawnY}};
      std::queue<std::pair<int, int>> q;
      q.push({sx, kEnemySpawnY});
      while (!q.empty()) {
        auto [x, y] = q.front();
        q.pop();
        const int dx[] = {8, -8, 0, 0}, dy[] = {0, 0, 8, -8};
        for (int i = 0; i < 4; ++i) {
          const std::pair<int, int> n{x + dx[i], y + dy[i]};
          if (open(n.first, n.second) && seen.insert(n).second) q.push(n);
        }
      }
      if (!seen.count({kPlayerSpawnX, kPlayerSpawnY}))
        std::fprintf(stderr, "  stage %d: spawn x=%d can't reach the player spawn\n", s.number, sx);
      CHECK(seen.count({kPlayerSpawnX, kPlayerSpawnY}));
      CHECK(seen.count({kBaseX, kBaseY - kTankSize}));  // just above the wall
    }
  }
}

TEST("PRG-4", "threat never decreases and peaks at stage 10") {
  for (size_t i = 1; i < stages().size(); ++i) CHECK(threat(stages()[i]) >= threat(stages()[i - 1]));
  for (size_t i = 0; i + 1 < stages().size(); ++i) CHECK(threat(stages()[i]) < threat(stages().back()));
}

TEST("PRG-5", "the next stage opens with a 90-tick intro") {
  App app = playing_app();
  app.world.phase = Phase::StageClear;
  Input start;
  start.start = true;
  app.update(start);
  CHECK(app.screen == Screen::StageIntro);
  CHECK_EQ(app.world.stage_number, 2);
  for (int i = 0; i < kStageIntroTicks - 1; ++i) app.update({});
  CHECK(app.screen == Screen::StageIntro);
  CHECK_EQ(app.world.tick_count, 0u);
  app.update({});
  CHECK(app.screen == Screen::Game);
  CHECK_EQ(app.world.tick_count, 0u);
  app.update({});
  CHECK_EQ(app.world.tick_count, 1u);

  App back = playing_app();
  clear_stage(back);
  tap(back, &Input::back);
  CHECK(back.screen == Screen::Title);
}

TEST("PRG-6", "clearing a stage moves on to the next with lives carried over") {
  App app = playing_app();
  app.world.lives = 1;
  app.world.set_terrain(0, 2, Terrain::Steel);
  app.world.player.x = 150;
  clear_stage(app);
  CHECK(app.screen == Screen::StageIntro);
  CHECK_EQ(app.world.stage_number, 2);
  CHECK(app.world.terrain == World(stage_for(2)).terrain);
  CHECK(app.world.base.alive);
  CHECK(app.world.phase == Phase::Playing);
  CHECK_EQ(app.world.enemies_waiting(), 20);
  CHECK_EQ(app.world.kills, 0);
  CHECK_EQ(app.world.lives, 1);
  CHECK(app.world.player_alive);
  CHECK_EQ(app.world.player.x, kPlayerSpawnX);
  CHECK_EQ(app.world.player.shield_ticks, kShieldTicks);
  skip_intro(app);
  CHECK(app.screen == Screen::Game);
}

TEST("PRG-7", "after stage 10, stage 11 replays stage 1's map") {
  App app = playing_app();
  for (int n = 2; n <= 11; ++n) {
    clear_stage(app);
    CHECK_EQ(app.world.stage_number, n);
    skip_intro(app);
  }
  CHECK(&stage_for(11) == &stage_for(1));
  CHECK(app.world.terrain == World(stage1()).terrain);
  CHECK_EQ(app.world.stage_number, 11);
}

TEST("PRG-8", "a direction pressed during the intro moves the tank once play starts") {
  App app = playing_app();
  clear_stage(app);  // start released; one intro tick used
  Input up;
  up.up = true;
  for (int i = 0; i < kStageIntroTicks - 1; ++i) app.update(up);
  CHECK(app.screen == Screen::Game);
  for (int i = 0; i < 10; ++i) app.update(up);
  CHECK_EQ(app.world.player.y, kPlayerSpawnY - 10);
}
