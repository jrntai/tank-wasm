#include <emscripten.h>
#include <emscripten/html5.h>

#include <cstdint>
#include <cstdio>
#include <cstring>

#include "platform/renderer.h"
#include "sim/app.h"

namespace {

constexpr int kCanvasScale = 3;

sim::App app(1);
Renderer renderer;
double last_ms = 0.0;
double accumulator = 0.0;

// REN-9: arrow keys or WASD, Space or J to fire, Enter = start, Esc = back.
struct Keys {
  bool up, down, left, right, w, a, s, d, space, j, enter, esc;
} keys = {};

sim::Input current_input() {
  sim::Input in;
  in.up = keys.up || keys.w;
  in.down = keys.down || keys.s;
  in.left = keys.left || keys.a;
  in.right = keys.right || keys.d;
  in.fire = keys.space || keys.j;
  in.start = keys.enter;
  in.back = keys.esc;
  return in;
}

bool on_key(int type, const EmscriptenKeyboardEvent* e, void*) {
  const bool down = type == EMSCRIPTEN_EVENT_KEYDOWN;
  struct Binding {
    const char* code;
    bool* key;
  };
  const Binding bindings[] = {
      {"ArrowUp", &keys.up}, {"ArrowDown", &keys.down}, {"ArrowLeft", &keys.left},
      {"ArrowRight", &keys.right}, {"KeyW", &keys.w}, {"KeyA", &keys.a},
      {"KeyS", &keys.s}, {"KeyD", &keys.d}, {"Space", &keys.space},
      {"KeyJ", &keys.j}, {"Enter", &keys.enter}, {"NumpadEnter", &keys.enter},
      {"Escape", &keys.esc},
  };
  for (const auto& b : bindings) {
    if (!std::strcmp(e->code, b.code)) {
      *b.key = down;
      return true;  // consume game keys so the page doesn't scroll
    }
  }
  return false;
}

void frame() {
  const double now = emscripten_get_now();
  accumulator += (now - last_ms) / 1000.0;
  last_ms = now;
  if (accumulator > 0.25) accumulator = 0.25;  // avoid spiral after tab switch

  const sim::Input in = current_input();
  while (accumulator >= sim::kTickSeconds) {
    app.update(in);
    accumulator -= sim::kTickSeconds;
  }

  int w = 0, h = 0;
  emscripten_get_canvas_element_size("#canvas", &w, &h);
  renderer.draw(app, w, h);
}

}  // namespace

int main() {
  EmscriptenWebGLContextAttributes attrs;
  emscripten_webgl_init_context_attributes(&attrs);
  attrs.majorVersion = 2;
  attrs.antialias = false;

  const auto ctx = emscripten_webgl_create_context("#canvas", &attrs);
  if (ctx <= 0) {
    std::fprintf(stderr, "WebGL2 context creation failed\n");
    return 1;
  }
  emscripten_webgl_make_context_current(ctx);
  emscripten_set_canvas_element_size("#canvas", Renderer::kScreenW * kCanvasScale,
                                     Renderer::kScreenH * kCanvasScale);
  if (!renderer.init()) return 1;

  emscripten_set_keydown_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, nullptr, true, on_key);
  emscripten_set_keyup_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, nullptr, true, on_key);

  app = sim::App(uint32_t(emscripten_get_now() * 1000.0));
  last_ms = emscripten_get_now();
  emscripten_set_main_loop(frame, 0, true);
  return 0;
}
