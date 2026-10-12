#pragma once

#include <cstdint>

#include "sim/world.h"

// Screen flow around the game: title menu, controls, stage select, stage intros
// and play. Specified in specs/07-menu.md, 08-progression.md and 10-stage-select.md.
namespace sim {

enum class Screen : uint8_t { Title, Controls, StageSelect, StageIntro, Game };
enum class MenuItem : uint8_t { OnePlayer, Controls };
constexpr int kMenuItemCount = 2;
constexpr int kStageIntroTicks = 90;

struct App {
  explicit App(uint32_t seed);

  // One fixed tick. `in` is the raw held state of every key.
  void update(const Input& in);

  Screen screen = Screen::Title;
  MenuItem cursor = MenuItem::OnePlayer;
  World world;  // the current (or last) game; ticks only on the Game screen
  int intro_ticks_left = 0;
  int selected_stage = 1;  // shown on Stage Select

 private:
  uint32_t next_seed_;
  int last_started_stage_ = 1;  // SEL-5
  Input prev_;     // last tick's effective input, for press detection
  Input latched_;  // keys held at the last screen change, ignored until released (MNU-8)

  Input unlatch(const Input& raw);
  void go(Screen s, const Input& raw);
  // Starts stage `number` with a fresh world, through its intro or straight into play.
  void begin_stage(int number, int lives, int level, bool intro, const Input& raw);
};

}  // namespace sim
