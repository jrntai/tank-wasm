#include "platform/renderer.h"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstring>
#include <vector>

namespace {

// Vertex positions come from gl_VertexID (triangle strip, no buffers).
// Art faces up; u_facing rotates the texture coordinates to match
// sim::Dir (0 Up, 1 Down, 2 Left, 3 Right).
const char* kVS = R"(#version 300 es
uniform vec2 u_pos;
uniform vec2 u_size;
uniform vec2 u_screen;
uniform int u_facing;
out vec2 v_uv;
void main() {
  vec2 p = vec2(float(gl_VertexID & 1), float(gl_VertexID >> 1));
  if (u_facing == 0) v_uv = p;
  else if (u_facing == 1) v_uv = 1.0 - p;
  else if (u_facing == 2) v_uv = vec2(1.0 - p.y, p.x);
  else v_uv = vec2(p.y, 1.0 - p.x);
  vec2 px = u_pos + p * u_size;
  gl_Position = vec4(px.x / u_screen.x * 2.0 - 1.0, 1.0 - px.y / u_screen.y * 2.0, 0.0, 1.0);
}
)";

const char* kFS = R"(#version 300 es
precision mediump float;
uniform sampler2D u_tex;
uniform vec4 u_tint;
in vec2 v_uv;
out vec4 out_color;
void main() {
  vec4 c = texture(u_tex, v_uv);
  if (c.a < 0.5) discard;
  out_color = c * u_tint;
}
)";

// Colors are packed as 0xRRGGBB.
struct PaletteEntry {
  char c;
  uint32_t rgb;
};
using Palette = std::vector<PaletteEntry>;

Palette tank_palette(uint32_t light, uint32_t mid, uint32_t turret) {
  return {{'T', 0x282828}, {'t', 0x5A5A5A}, {'g', light},
          {'G', mid},      {'h', turret},   {'B', 0xDCDCDC}};
}

GLuint make_texture(const sprites::Art& art, const Palette& palette) {
  std::vector<uint8_t> pixels(art.width * art.height * 4, 0);
  for (int y = 0; y < art.height; ++y) {
    for (int x = 0; x < art.width; ++x) {
      const char c = art.rows[y][x];
      for (const auto& e : palette) {
        if (e.c != c) continue;
        uint8_t* p = &pixels[(y * art.width + x) * 4];
        p[0] = (e.rgb >> 16) & 0xFF;
        p[1] = (e.rgb >> 8) & 0xFF;
        p[2] = e.rgb & 0xFF;
        p[3] = 0xFF;
      }
    }
  }
  GLuint tex;
  glGenTextures(1, &tex);
  glBindTexture(GL_TEXTURE_2D, tex);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, art.width, art.height, 0, GL_RGBA, GL_UNSIGNED_BYTE,
               pixels.data());
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  return tex;
}

GLuint compile(GLenum type, const char* src) {
  GLuint s = glCreateShader(type);
  glShaderSource(s, 1, &src, nullptr);
  glCompileShader(s);
  GLint ok = 0;
  glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
  if (!ok) {
    char log[1024];
    glGetShaderInfoLog(s, sizeof log, nullptr, log);
    std::fprintf(stderr, "shader compile failed: %s\n", log);
    glDeleteShader(s);
    return 0;
  }
  return s;
}

GLuint link(const char* vs_src, const char* fs_src) {
  GLuint vs = compile(GL_VERTEX_SHADER, vs_src);
  GLuint fs = compile(GL_FRAGMENT_SHADER, fs_src);
  if (!vs || !fs) return 0;
  GLuint p = glCreateProgram();
  glAttachShader(p, vs);
  glAttachShader(p, fs);
  glLinkProgram(p);
  glDeleteShader(vs);
  glDeleteShader(fs);
  GLint ok = 0;
  glGetProgramiv(p, GL_LINK_STATUS, &ok);
  if (!ok) {
    char log[1024];
    glGetProgramInfoLog(p, sizeof log, nullptr, log);
    std::fprintf(stderr, "program link failed: %s\n", log);
    glDeleteProgram(p);
    return 0;
  }
  return p;
}

constexpr float kGray = 0x63 / 255.0f;

}  // namespace

bool Renderer::init() {
  glGenRenderbuffers(1, &color_rb_);
  glBindRenderbuffer(GL_RENDERBUFFER, color_rb_);
  glRenderbufferStorage(GL_RENDERBUFFER, GL_RGBA8, kScreenW, kScreenH);

  glGenFramebuffers(1, &fbo_);
  glBindFramebuffer(GL_FRAMEBUFFER, fbo_);
  glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, color_rb_);
  if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
    std::fprintf(stderr, "offscreen framebuffer incomplete\n");
    return false;
  }
  glBindFramebuffer(GL_FRAMEBUFFER, 0);

  program_ = link(kVS, kFS);
  if (!program_) return false;
  u_pos_ = glGetUniformLocation(program_, "u_pos");
  u_size_ = glGetUniformLocation(program_, "u_size");
  u_screen_ = glGetUniformLocation(program_, "u_screen");
  u_facing_ = glGetUniformLocation(program_, "u_facing");
  u_tex_ = glGetUniformLocation(program_, "u_tex");
  u_tint_ = glGetUniformLocation(program_, "u_tint");
  glGenVertexArrays(1, &vao_);

  static const char* const kWhiteRows[] = {"X"};
  const Palette ink = {{'X', 0xFFFFFF}};
  white_ = make_texture({kWhiteRows, 1, 1}, ink);

  // REN-2
  // PWR-12: the player brightens with each star level; bonus tanks flash red.
  player_tank_[0] = make_texture(sprites::kTank, tank_palette(0xE8D040, 0xA88C1C, 0xFCF088));
  player_tank_[1] = make_texture(sprites::kTank, tank_palette(0xE8D040, 0xA88C1C, 0xFCFCFC));
  player_tank_[2] = make_texture(sprites::kTank, tank_palette(0xF0B030, 0xB8780C, 0xFCFCFC));
  player_tank_[3] = make_texture(sprites::kTank, tank_palette(0xFCE8A0, 0xD0A020, 0xFCFCFC));
  bonus_tank_ = make_texture(sprites::kTank, tank_palette(0xE84030, 0xA81808, 0xFC9080));
  enemy_tank_[int(sim::EnemyType::Basic)] =
      make_texture(sprites::kTank, tank_palette(0xBCBCBC, 0x7C7C7C, 0xE8E8E8));
  enemy_tank_[int(sim::EnemyType::Fast)] =
      make_texture(sprites::kTank, tank_palette(0x7CD8F0, 0x2C88B0, 0xC8F4FC));
  enemy_tank_[int(sim::EnemyType::Power)] =
      make_texture(sprites::kTank, tank_palette(0xE85C40, 0xA82C10, 0xFCA890));
  armor_tank_[3] = make_texture(sprites::kTank, tank_palette(0x5CC85C, 0x1C881C, 0xA8F0A8));
  armor_tank_[2] = make_texture(sprites::kTank, tank_palette(0xE8D040, 0xA88C1C, 0xFCF088));
  armor_tank_[1] = make_texture(sprites::kTank, tank_palette(0xF09C3C, 0xB05C0C, 0xFCD0A0));
  armor_tank_[0] = enemy_tank_[int(sim::EnemyType::Basic)];
  enemy_tank_[int(sim::EnemyType::Armor)] = armor_tank_[3];

  // REN-1
  brick_ = make_texture(sprites::kBrick, {{'R', 0xB84C1C}, {'r', 0x883010}, {'m', 0x9C9C9C}});
  const Palette steel = {{'W', 0xFCFCFC}, {'s', 0xBCBCBC}, {'D', 0x7C7C7C}, {'d', 0x5C5C5C}};
  steel_ = make_texture(sprites::kSteel, steel);
  const Palette water = {{'b', 0x2038EC}, {'w', 0xA8C8FC}};
  water_[0] = make_texture(sprites::kWater0, water);
  water_[1] = make_texture(sprites::kWater1, water);
  trees_ = make_texture(sprites::kTrees, {{'f', 0x005800}, {'F', 0x38A800}});
  ice_ = make_texture(sprites::kIce, {{'i', 0xA8E0FC}, {'I', 0xFCFCFC}});

  // REN-5
  eagle_ = make_texture(sprites::kEagle, {{'X', 0x9C9C9C}});
  eagle_dead_ = make_texture(sprites::kEagleDead,
                             {{'X', 0x7C7C7C}, {'K', 0x202020}, {'O', 0xC83C0C}});

  bullet_ = make_texture(sprites::kBullet, {{'X', 0xE8E8E8}});
  explosion_ =
      make_texture(sprites::kExplosion, {{'W', 0xFCFCFC}, {'R', 0xD82800}, {'P', 0x6C1C9C}});
  tank_icon_ = make_texture(sprites::kTankIcon, ink);
  flag_ = make_texture(sprites::kFlag, {{'K', 0x000000}, {'O', 0xE45C10}});
  // PWR-12: one icon per power-up type.
  using P = sim::PowerUpType;
  powerup_[int(P::Star)] = make_texture(sprites::kStar, {{'Y', 0xFCE020}, {'W', 0xE45C10}});
  powerup_[int(P::Grenade)] =
      make_texture(sprites::kGrenade, {{'G', 0x58A818}, {'D', 0x2C5C0C}, {'W', 0xFCFCFC}});
  powerup_[int(P::Helmet)] = make_texture(sprites::kHelmet, {{'Y', 0xE8B020}, {'W', 0xFCF0B0}});
  powerup_[int(P::Shovel)] = make_texture(sprites::kShovel, {{'S', 0xBCBCBC}, {'B', 0x9C5C1C}});
  powerup_[int(P::Clock)] =
      make_texture(sprites::kClock, {{'R', 0xD82800}, {'W', 0xFCFCFC}, {'K', 0x000000}});
  powerup_[int(P::Tank)] =
      make_texture(sprites::kExtraLife, {{'Y', 0xE8D040}, {'D', 0x6C5C10}, {'K', 0x000000}});
  for (int c = 0; c < 128; ++c)
    if (const sprites::Art* g = sprites::glyph(char(c))) glyphs_[c] = make_texture(*g, ink);
  return true;
}

void Renderer::quad(GLuint tex, int x, int y, int w, int h, int facing, Color tint) {
  glUniform2f(u_pos_, x, y);
  glUniform2f(u_size_, w, h);
  glUniform1i(u_facing_, facing);
  glUniform4f(u_tint_, tint.r, tint.g, tint.b, tint.a);
  glBindTexture(GL_TEXTURE_2D, tex);
  glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

void Renderer::text(const char* s, int x, int y, Color c) {
  for (; *s; ++s, x += 6) {
    const unsigned char ch = static_cast<unsigned char>(*s);
    if (ch < 128 && glyphs_[ch]) quad(glyphs_[ch], x, y, 5, 7, 0, c);
  }
}

void Renderer::text_centered(const char* s, int cx, int cy, Color c) {
  const int width = int(std::strlen(s)) * 6 - 1;
  text(s, cx - width / 2, cy - 3, c);
}

void Renderer::big_text(const char* s, int x, int y) {
  constexpr int kPixel = 8;
  for (; *s; ++s, x += 6 * kPixel) {
    const sprites::Art* g = sprites::glyph(*s);
    if (!g) continue;
    for (int gy = 0; gy < g->height; ++gy)
      for (int gx = 0; gx < g->width; ++gx)
        if (g->rows[gy][gx] == 'X') quad(brick_, x + gx * kPixel, y + gy * kPixel, kPixel, kPixel);
  }
}

void Renderer::draw_terrain(const sim::World& world, bool trees) {
  const GLuint water = water_[(world.tick_count / 30) % 2];
  for (int cy = 0; cy < sim::kGridSize; ++cy) {
    for (int cx = 0; cx < sim::kGridSize; ++cx) {
      const sim::Terrain t = world.terrain_at(cx, cy);
      if ((t == sim::Terrain::Trees) != trees) continue;
      GLuint tex = 0;
      switch (t) {
        case sim::Terrain::Brick: tex = brick_; break;
        case sim::Terrain::Steel: tex = steel_; break;
        case sim::Terrain::Water: tex = water; break;
        case sim::Terrain::Trees: tex = trees_; break;
        case sim::Terrain::Ice: tex = ice_; break;
        case sim::Terrain::Empty: break;
      }
      if (tex)
        quad(tex, kArenaX + cx * sim::kCellSize, kArenaY + cy * sim::kCellSize, sim::kCellSize,
             sim::kCellSize);
    }
  }
}

void Renderer::draw_tank(GLuint tex, const sim::Tank& t) {
  quad(tex, kArenaX + t.x, kArenaY + t.y, sim::kTankSize, sim::kTankSize, int(t.facing));
}

void Renderer::draw_hud(const sim::World& world) {
  const Color dark = {0, 0, 0, 1};
  // REN-7: enemies still waiting to spawn, two columns.
  for (int i = 0; i < world.enemies_waiting(); ++i)
    quad(tank_icon_, kHudX + (i % 2) * 8, 16 + (i / 2) * 8, 8, 8, 0, dark);

  char digit[2] = {char('0' + std::clamp(world.lives, 0, 9)), 0};
  text("IP", kHudX, 120, dark);
  quad(tank_icon_, kHudX, 129, 8, 8, 0, {0.91f, 0.36f, 0.06f, 1});
  text(digit, kHudX + 10, 130, dark);

  quad(flag_, kHudX, 160, 16, 16);
  char stage[4];
  std::snprintf(stage, sizeof stage, "%d", world.stage_number % 100);
  text(stage, kHudX + 8, 178, dark);
}

void Renderer::draw_game(const sim::World& world) {
  rect(kArenaX, kArenaY, sim::kArenaSize, sim::kArenaSize, {0, 0, 0, 1});
  draw_terrain(world, false);
  quad(world.base.alive ? eagle_ : eagle_dead_, kArenaX + world.base.x, kArenaY + world.base.y,
       sim::kTankSize, sim::kTankSize);

  if (world.player_alive) {
    draw_tank(player_tank_[std::clamp(world.player.level, 0, sim::kMaxPlayerLevel)], world.player);
    // REN-4: flickering outline while shielded.
    if (world.player.shield_ticks > 0 && (world.tick_count / 2) % 2 == 0) {
      const int x = kArenaX + world.player.x, y = kArenaY + world.player.y;
      const Color w = {1, 1, 1, 1};
      rect(x - 1, y - 1, 18, 1, w);
      rect(x - 1, y + 16, 18, 1, w);
      rect(x - 1, y, 1, 16, w);
      rect(x + 16, y, 1, 16, w);
    }
  }
  for (const auto& e : world.enemies) {
    const bool flash = e.bonus && (world.tick_count / 8) % 2 == 0;
    const GLuint tex = flash ? bonus_tank_
                       : e.type == sim::EnemyType::Armor ? armor_tank_[std::clamp(e.hp, 1, 4) - 1]
                                                         : enemy_tank_[int(e.type)];
    draw_tank(tex, e);
  }
  for (const auto& b : world.bullets)
    quad(bullet_, kArenaX + b.x, kArenaY + b.y, sim::kBulletSize, sim::kBulletSize);

  draw_terrain(world, true);  // REN-3: trees cover tanks and bullets

  // PWR-12: items sit above trees so they can always be found, and blink.
  if (world.powerup && (world.tick_count / 8) % 2 == 0) {
    quad(powerup_[int(world.powerup->type)], kArenaX + world.powerup->x,
         kArenaY + world.powerup->y, sim::kPowerUpSize, sim::kPowerUpSize);
  }

  // REN-6
  for (const auto& e : world.explosions) {
    const int size = e.big ? 32 : 16;
    quad(explosion_, kArenaX + e.cx - size / 2, kArenaY + e.cy - size / 2, size, size);
  }

  draw_hud(world);

  // REN-8
  if (world.phase != sim::Phase::Playing) {
    const int mid = kArenaY + sim::kArenaSize / 2;
    const bool over = world.phase == sim::Phase::GameOver;
    rect(kArenaX + 40, mid - 16, sim::kArenaSize - 80, 34, {0, 0, 0, 1});
    const int cx = kArenaX + sim::kArenaSize / 2;
    text_centered(over ? "GAME OVER" : "STAGE CLEAR", cx, mid - 6,
                  over ? Color{0.85f, 0.16f, 0, 1} : Color{1, 1, 1, 1});
    text_centered("PRESS ENTER", cx, mid + 8, {0.74f, 0.74f, 0.74f, 1});
  }
}

void Renderer::draw_title(const sim::App& app) {
  // MNU-10
  rect(0, 0, kScreenW, kScreenH, {0, 0, 0, 1});
  constexpr int kTitleW = 4 * 48 - 8;  // four big glyphs, 48 px advance
  big_text("TANK", (kScreenW - kTitleW) / 2, 24);
  big_text("1990", (kScreenW - kTitleW) / 2, 88);

  const char* items[sim::kMenuItemCount] = {"1 PLAYER", "CONTROLS"};
  const Color white = {1, 1, 1, 1};
  for (int i = 0; i < sim::kMenuItemCount; ++i) {
    const int y = 164 + i * 18;
    text(items[i], 100, y, white);
    if (int(app.cursor) == i)
      quad(player_tank_[0], 76, y - 4, sim::kTankSize, sim::kTankSize, int(sim::Dir::Right));
  }
}

void Renderer::draw_controls() {
  rect(0, 0, kScreenW, kScreenH, {0, 0, 0, 1});
  const Color white = {1, 1, 1, 1}, gray = {0.74f, 0.74f, 0.74f, 1};
  text_centered("CONTROLS", kScreenW / 2, 40, {0.91f, 0.36f, 0.06f, 1});
  struct Row {
    const char* action;
    const char* keys;
  };
  const Row rows[] = {
      {"MOVE", "ARROWS / WASD"}, {"FIRE", "SPACE / J"}, {"START", "ENTER"}, {"BACK", "ESC"}};
  int y = 72;
  for (const auto& r : rows) {
    text(r.action, 48, y, gray);
    text(r.keys, 112, y, white);
    y += 18;
  }
  text_centered("PRESS ENTER", kScreenW / 2, 184, gray);
}

void Renderer::draw_stage_curtain(int stage_number) {
  // Gray curtain with the stage number and name.
  char title[16];
  std::snprintf(title, sizeof title, "STAGE %d", stage_number);
  text_centered(title, kScreenW / 2, kScreenH / 2 - 6, {0, 0, 0, 1});
  char name[32] = {};
  const std::string_view n = sim::stage_for(stage_number).name;
  for (size_t i = 0; i < n.size() && i + 1 < sizeof name; ++i)
    name[i] = char(std::toupper(static_cast<unsigned char>(n[i])));
  text_centered(name, kScreenW / 2, kScreenH / 2 + 10, {0.2f, 0.2f, 0.2f, 1});
}

void Renderer::draw(const sim::App& app, int canvas_width, int canvas_height) {
  // Pass 1: the current screen at internal resolution.
  glBindFramebuffer(GL_FRAMEBUFFER, fbo_);
  glViewport(0, 0, kScreenW, kScreenH);
  glClearColor(kGray, kGray, kGray, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT);

  glUseProgram(program_);
  glBindVertexArray(vao_);
  glUniform2f(u_screen_, kScreenW, kScreenH);
  glActiveTexture(GL_TEXTURE0);
  glUniform1i(u_tex_, 0);

  switch (app.screen) {
    case sim::Screen::Title: draw_title(app); break;
    case sim::Screen::Controls: draw_controls(); break;
    case sim::Screen::StageIntro: draw_stage_curtain(app.world.stage_number); break;
    case sim::Screen::StageSelect:
      draw_stage_curtain(app.selected_stage);
      text_centered("UP/DOWN SELECT - ENTER START", kScreenW / 2, kScreenH - 24,
                    {0.2f, 0.2f, 0.2f, 1});  // SEL-7
      break;
    case sim::Screen::Game: draw_game(app.world); break;
  }

  // Pass 2: integer-scale blit to the canvas, letterboxed in gray.
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  glViewport(0, 0, canvas_width, canvas_height);
  glClearColor(kGray, kGray, kGray, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT);

  const int scale = std::max(1, std::min(canvas_width / kScreenW, canvas_height / kScreenH));
  const int w = kScreenW * scale, h = kScreenH * scale;
  const int x0 = (canvas_width - w) / 2, y0 = (canvas_height - h) / 2;
  glBindFramebuffer(GL_READ_FRAMEBUFFER, fbo_);
  glBlitFramebuffer(0, 0, kScreenW, kScreenH, x0, y0, x0 + w, y0 + h, GL_COLOR_BUFFER_BIT,
                    GL_NEAREST);
  glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
}
