#pragma once

#include <GLES3/gl3.h>

#include <cstdint>

#include "platform/sprites.h"
#include "sim/app.h"

// Draws the world into a low-resolution offscreen framebuffer, then scales it
// up to the canvas by an integer factor with nearest-neighbor filtering.
// Layout and visuals are specified in specs/06-presentation.md.
class Renderer {
 public:
  static constexpr int kScreenW = 256, kScreenH = 224;
  static constexpr int kArenaX = 16, kArenaY = 8;  // arena offset on screen
  static constexpr int kHudX = 232;

  // Requires a current WebGL2 context. Returns false on failure.
  bool init();
  void draw(const sim::App& app, int canvas_width, int canvas_height);

 private:
  struct Color {
    float r, g, b, a;
  };

  GLuint fbo_ = 0, color_rb_ = 0;
  GLuint program_ = 0, vao_ = 0;
  GLint u_pos_ = -1, u_size_ = -1, u_screen_ = -1, u_facing_ = -1, u_tex_ = -1, u_tint_ = -1;

  GLuint white_ = 0;
  GLuint player_tank_[sim::kMaxPlayerLevel + 1] = {};  // by star level
  GLuint bonus_tank_ = 0;
  GLuint enemy_tank_[4] = {};  // by EnemyType
  GLuint armor_tank_[4] = {};  // by remaining hp - 1
  GLuint brick_ = 0, steel_ = 0, water_[2] = {}, trees_ = 0, ice_ = 0;
  GLuint eagle_ = 0, eagle_dead_ = 0;
  GLuint bullet_ = 0, explosion_ = 0, tank_icon_ = 0, flag_ = 0;
  GLuint powerup_[sim::kPowerUpTypeCount] = {};  // by PowerUpType
  GLuint glyphs_[128] = {};

  void quad(GLuint tex, int x, int y, int w, int h, int facing = 0,
            Color tint = {1, 1, 1, 1});
  void rect(int x, int y, int w, int h, Color c) { quad(white_, x, y, w, h, 0, c); }
  void text(const char* s, int x, int y, Color c);
  void text_centered(const char* s, int cx, int cy, Color c);
  void big_text(const char* s, int x, int y);  // glyph pixels drawn as brick cells

  void draw_game(const sim::World& world);
  void draw_title(const sim::App& app);
  void draw_controls();
  void draw_stage_curtain(int stage_number);  // PRG-9 / SEL-7

  void draw_terrain(const sim::World& world, bool trees);
  void draw_tank(GLuint tex, const sim::Tank& t);
  void draw_hud(const sim::World& world);
};
