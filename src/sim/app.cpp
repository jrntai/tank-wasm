#include "sim/app.h"

namespace sim {

namespace {

constexpr bool Input::*kKeys[] = {&Input::up,   &Input::down,  &Input::left, &Input::right,
                                  &Input::fire, &Input::start, &Input::back};

uint32_t next_random(uint32_t x) {
  // xorshift32
  x ^= x << 13;
  x ^= x >> 17;
  x ^= x << 5;
  return x;
}

}  // namespace

App::App(uint32_t seed) : world(stage1(), seed), next_seed_(seed ? seed : 1) {}

Input App::unlatch(const Input& raw) {
  Input in = raw;
  for (auto k : kKeys) {
    if (!(raw.*k)) latched_.*k = false;
    if (latched_.*k) in.*k = false;
  }
  return in;
}

void App::go(Screen s, const Input& raw) {
  screen = s;
  latched_ = raw;  // MNU-8
}

void App::begin_stage(int number, int lives, int level, bool intro, const Input& raw) {
  world = World(stage_for(number), next_seed_);
  next_seed_ = next_random(next_seed_);  // MNU-9
  world.stage_number = number;           // PRG-7: keeps counting past the last map
  world.lives = lives;                   // PRG-6
  world.player.level = level;            // PWR-11
  intro_ticks_left = kStageIntroTicks;   // PRG-5
  go(intro ? Screen::StageIntro : Screen::Game, raw);
}

void App::update(const Input& raw) {
  const Input in = unlatch(raw);
  const Input prev = prev_;
  prev_ = in;
  auto pressed = [&](bool Input::*k) { return in.*k && !(prev.*k); };

  switch (screen) {
    case Screen::Title: {
      // MNU-2
      int c = static_cast<int>(cursor);
      if (pressed(&Input::up)) c = (c + kMenuItemCount - 1) % kMenuItemCount;
      if (pressed(&Input::down)) c = (c + 1) % kMenuItemCount;
      cursor = static_cast<MenuItem>(c);
      if (pressed(&Input::start) || pressed(&Input::fire)) {
        if (cursor == MenuItem::OnePlayer) {  // MNU-3, SEL-1
          selected_stage = last_started_stage_;
          go(Screen::StageSelect, raw);
        } else {
          go(Screen::Controls, raw);  // MNU-4
        }
      }
      break;
    }
    case Screen::Controls:
      if (pressed(&Input::start) || pressed(&Input::fire) || pressed(&Input::back))
        go(Screen::Title, raw);
      break;
    case Screen::StageSelect: {
      // SEL-2: wraps between 1 and the number of stages.
      const int count = int(stages().size());
      if (pressed(&Input::up) || pressed(&Input::right)) selected_stage = selected_stage % count + 1;
      if (pressed(&Input::down) || pressed(&Input::left))
        selected_stage = (selected_stage + count - 2) % count + 1;
      if (pressed(&Input::back)) {
        go(Screen::Title, raw);  // SEL-4
      } else if (pressed(&Input::start) || pressed(&Input::fire)) {
        last_started_stage_ = selected_stage;  // SEL-5
        begin_stage(selected_stage, kPlayerReserveLives, 0, false, raw);  // SEL-3
      }
      break;
    }
    case Screen::StageIntro:
      if (pressed(&Input::back)) {
        cursor = MenuItem::OnePlayer;
        go(Screen::Title, raw);
      } else if (--intro_ticks_left <= 0) {
        screen = Screen::Game;  // PRG-8: no latch, so directions pressed now work at once
      }
      break;
    case Screen::Game:
      if (pressed(&Input::back) ||
          (world.phase == Phase::GameOver && pressed(&Input::start))) {  // MNU-6, MNU-7
        cursor = MenuItem::OnePlayer;
        go(Screen::Title, raw);
      } else if (world.phase == Phase::StageClear && pressed(&Input::start)) {
        begin_stage(world.stage_number + 1, world.lives, world.player.level, true, raw);  // PRG-6
      } else {
        world.tick(in);
      }
      break;
  }
}

}  // namespace sim
