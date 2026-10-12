#include <string>

#include "test.h"

using namespace sim;

TEST("ARN-1", "stage chars load into matching cells") {
  static const std::string top = "#@~%-" + std::string(kGridSize - 5, '.');
  static const std::string bottom = std::string(kGridSize - 1, '.') + "#";
  Stage s = empty_stage();
  s.rows[0] = top;
  s.rows[kGridSize - 1] = bottom;
  World w(s);
  CHECK(w.terrain_at(0, 0) == Terrain::Brick);
  CHECK(w.terrain_at(1, 0) == Terrain::Steel);
  CHECK(w.terrain_at(2, 0) == Terrain::Water);
  CHECK(w.terrain_at(3, 0) == Terrain::Trees);
  CHECK(w.terrain_at(4, 0) == Terrain::Ice);
  CHECK(w.terrain_at(5, 0) == Terrain::Empty);
  CHECK(w.terrain_at(kGridSize - 1, kGridSize - 1) == Terrain::Brick);
  CHECK(w.terrain_at(0, kGridSize - 1) == Terrain::Empty);
}

TEST("ARN-2", "which terrain blocks tanks") {
  CHECK(blocks_tanks(Terrain::Brick));
  CHECK(blocks_tanks(Terrain::Steel));
  CHECK(blocks_tanks(Terrain::Water));
  CHECK(!blocks_tanks(Terrain::Empty));
  CHECK(!blocks_tanks(Terrain::Trees));
  CHECK(!blocks_tanks(Terrain::Ice));
}

TEST("ARN-3", "which terrain blocks bullets") {
  CHECK(blocks_bullets(Terrain::Brick));
  CHECK(blocks_bullets(Terrain::Steel));
  CHECK(!blocks_bullets(Terrain::Water));
  CHECK(!blocks_bullets(Terrain::Empty));
  CHECK(!blocks_bullets(Terrain::Trees));
  CHECK(!blocks_bullets(Terrain::Ice));
}

TEST("ARN-4", "stage 1 keeps spawns and base clear and walls the base") {
  World w(stage1());
  auto clear_tile = [&](int px, int py) {
    for (int cy = py / kCellSize; cy < (py + kTankSize) / kCellSize; ++cy)
      for (int cx = px / kCellSize; cx < (px + kTankSize) / kCellSize; ++cx)
        if (w.terrain_at(cx, cy) != Terrain::Empty) return false;
    return true;
  };
  CHECK(clear_tile(kBaseX, kBaseY));
  CHECK(clear_tile(kPlayerSpawnX, kPlayerSpawnY));
  for (int x : kEnemySpawnX) CHECK(clear_tile(x, kEnemySpawnY));

  // Half-tile brick wall: the row above the base and the columns beside it.
  for (int cx = 11; cx <= 14; ++cx) CHECK(w.terrain_at(cx, 23) == Terrain::Brick);
  for (int cy = 24; cy <= 25; ++cy) {
    CHECK(w.terrain_at(11, cy) == Terrain::Brick);
    CHECK(w.terrain_at(14, cy) == Terrain::Brick);
  }
}
