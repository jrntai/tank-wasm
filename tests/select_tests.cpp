#include "sim/app.h"
#include "test.h"

using namespace sim;
using test::tap;

namespace {

App at_select(uint32_t seed = 1) {
  App app(seed);
  tap(app, &Input::start);
  return app;
}

}  // namespace

TEST("SEL-1", "1 PLAYER opens stage select on stage 1; the world doesn't tick") {
  App app = at_select();
  CHECK(app.screen == Screen::StageSelect);
  CHECK_EQ(app.selected_stage, 1);
  const uint32_t ticks = app.world.tick_count;
  for (int i = 0; i < 100; ++i) app.update({});
  CHECK_EQ(app.world.tick_count, ticks);
}

TEST("SEL-2", "up/right raise, down/left lower, wrapping 1..10; holds move once") {
  App app = at_select();
  tap(app, &Input::up);
  CHECK_EQ(app.selected_stage, 2);
  tap(app, &Input::right);
  CHECK_EQ(app.selected_stage, 3);
  tap(app, &Input::down);
  tap(app, &Input::left);
  CHECK_EQ(app.selected_stage, 1);
  tap(app, &Input::down);
  CHECK_EQ(app.selected_stage, 10);
  tap(app, &Input::up);
  CHECK_EQ(app.selected_stage, 1);

  Input up;
  up.up = true;
  for (int i = 0; i < 30; ++i) app.update(up);
  CHECK_EQ(app.selected_stage, 2);
}

TEST("SEL-3", "start or fire begins the chosen stage at once, fresh") {
  for (bool Input::*confirm : {&Input::start, &Input::fire}) {
    App app = at_select();
    for (int i = 0; i < 4; ++i) tap(app, &Input::up);  // stage 5
    Input in;
    in.*confirm = true;
    app.update(in);
    CHECK(app.screen == Screen::Game);
    CHECK_EQ(app.world.stage_number, 5);
    CHECK_EQ(app.world.tick_count, 0u);
    CHECK(app.world.terrain == World(stage_for(5)).terrain);
    CHECK_EQ(app.world.enemies_waiting(), 20);
    CHECK_EQ(app.world.lives, kPlayerReserveLives);
    CHECK_EQ(app.world.player.level, 0);
  }

  // Lives and level from an abandoned game don't leak into the next one.
  App again = at_select();
  tap(again, &Input::start);
  again.world.lives = 0;
  again.world.player.level = 3;
  tap(again, &Input::back);
  tap(again, &Input::start);
  tap(again, &Input::start);
  CHECK_EQ(again.world.lives, kPlayerReserveLives);
  CHECK_EQ(again.world.player.level, 0);
}

TEST("SEL-4", "back returns to title") {
  App app = at_select();
  tap(app, &Input::back);
  CHECK(app.screen == Screen::Title);
}

TEST("SEL-5", "stage select reopens on the last stage started from it") {
  App app = at_select();
  for (int i = 0; i < 6; ++i) tap(app, &Input::up);  // stage 7
  tap(app, &Input::start);
  tap(app, &Input::back);   // abandon the game
  tap(app, &Input::start);  // title -> stage select
  CHECK(app.screen == Screen::StageSelect);
  CHECK_EQ(app.selected_stage, 7);

  // Browsing to another stage and backing out doesn't change what's remembered.
  tap(app, &Input::up);
  tap(app, &Input::back);
  tap(app, &Input::start);
  CHECK_EQ(app.selected_stage, 7);
}

TEST("SEL-6", "a game started mid-way advances and wraps") {
  App app = at_select();
  for (int i = 0; i < 9; ++i) tap(app, &Input::up);  // stage 10
  tap(app, &Input::start);
  CHECK_EQ(app.world.stage_number, 10);
  app.world.phase = Phase::StageClear;
  tap(app, &Input::start);
  CHECK(app.screen == Screen::StageIntro);
  CHECK_EQ(app.world.stage_number, 11);
  CHECK(app.world.terrain == World(stage1()).terrain);
}
