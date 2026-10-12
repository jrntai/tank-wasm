#include "sim/app.h"
#include "test.h"

using namespace sim;
using test::start_game;
using test::tap;

namespace {

Input key(bool Input::*k) {
  Input in;
  in.*k = true;
  return in;
}
}  // namespace

TEST("MNU-1", "app starts on title with the cursor on 1 PLAYER") {
  App app(1);
  CHECK(app.screen == Screen::Title);
  CHECK(app.cursor == MenuItem::OnePlayer);
}

TEST("MNU-2", "up and down move the cursor, wrapping; holding moves once") {
  App app(1);
  tap(app, &Input::down);
  CHECK(app.cursor == MenuItem::Controls);
  tap(app, &Input::down);
  CHECK(app.cursor == MenuItem::OnePlayer);
  tap(app, &Input::up);
  CHECK(app.cursor == MenuItem::Controls);
  tap(app, &Input::up);
  CHECK(app.cursor == MenuItem::OnePlayer);

  for (int i = 0; i < 30; ++i) app.update(key(&Input::down));
  CHECK(app.cursor == MenuItem::Controls);
}

TEST("MNU-3", "confirming 1 PLAYER opens stage select") {
  for (bool Input::*confirm : {&Input::start, &Input::fire}) {
    App app(1);
    app.update(key(confirm));
    CHECK(app.screen == Screen::StageSelect);
  }
}

TEST("MNU-4", "CONTROLS opens the controls screen; start, fire or back return") {
  for (bool Input::*leave : {&Input::start, &Input::fire, &Input::back}) {
    App app(1);
    tap(app, &Input::down);
    tap(app, &Input::start);
    CHECK(app.screen == Screen::Controls);
    tap(app, leave);
    CHECK(app.screen == Screen::Title);
  }
}

TEST("MNU-5", "the world does not tick on menu screens") {
  App app(1);
  const uint32_t ticks = app.world.tick_count;
  for (int i = 0; i < 100; ++i) app.update({});
  CHECK_EQ(app.world.tick_count, ticks);
  tap(app, &Input::down);
  tap(app, &Input::start);
  for (int i = 0; i < 100; ++i) app.update({});
  CHECK_EQ(app.world.tick_count, ticks);
}

TEST("MNU-6", "back abandons the game and returns to title") {
  App app(1);
  start_game(app);
  for (int i = 0; i < 50; ++i) app.update({});
  CHECK(app.world.tick_count > 0);
  tap(app, &Input::back);
  CHECK(app.screen == Screen::Title);
  CHECK(app.cursor == MenuItem::OnePlayer);
}

TEST("MNU-7", "start leaves a finished game; ignored while playing") {
  App app(1);
  start_game(app);
  tap(app, &Input::start);  // still playing: ignored
  CHECK(app.screen == Screen::Game);

  app.world.phase = Phase::GameOver;
  tap(app, &Input::start);
  CHECK(app.screen == Screen::Title);

  start_game(app);
  app.world.phase = Phase::StageClear;
  tap(app, &Input::start);
  CHECK(app.screen == Screen::StageIntro);
  CHECK_EQ(app.world.stage_number, 2);
}

TEST("MNU-8", "keys held across a screen change are ignored until re-pressed") {
  // Fire confirms stage select; still held in play, it must not shoot.
  App app(1);
  tap(app, &Input::fire);            // title -> stage select
  app.update(key(&Input::fire));     // stage select -> game, fire held
  CHECK(app.screen == Screen::Game);
  for (int i = 0; i < 20; ++i) app.update(key(&Input::fire));
  CHECK(app.world.bullets.empty());
  app.update({});
  app.update(key(&Input::fire));
  CHECK_EQ(app.world.bullets.size(), 1u);

  // Held movement is ignored too: up (which picked stage 2) is still held when
  // the game starts, so the player stays put until up is released and re-pressed.
  App moving(1);
  tap(moving, &Input::start);  // title -> stage select
  Input in;
  in.up = true;
  moving.update(in);  // stage 2
  in.start = true;
  moving.update(in);  // start with up held
  CHECK(moving.screen == Screen::Game);
  CHECK_EQ(moving.world.stage_number, 2);
  const int y = moving.world.player.y;
  for (int i = 0; i < 10; ++i) moving.update(key(&Input::up));
  CHECK_EQ(moving.world.player.y, y);
  moving.update({});
  for (int i = 0; i < 10; ++i) moving.update(key(&Input::up));
  CHECK_EQ(moving.world.player.y, y - 10);
}

TEST("MNU-9", "games get different seeds, reproducibly") {
  App a(5), b(5);
  start_game(a);
  start_game(b);
  const uint32_t first = a.world.rng_state;
  CHECK_EQ(first, b.world.rng_state);

  tap(a, &Input::back);
  start_game(a);
  tap(b, &Input::back);
  start_game(b);
  CHECK(a.world.rng_state != first);
  CHECK_EQ(a.world.rng_state, b.world.rng_state);
}
