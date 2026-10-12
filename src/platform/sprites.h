#pragma once

// Pixel art as ASCII rows. Each char maps to a color through a palette; '.' is
// transparent. Sprites that face a direction are drawn facing up.
namespace sprites {

struct Art {
  const char* const* rows;
  int width, height;
};

extern const Art kTank;  // chars: T t (treads) g G (hull) h (turret) B (barrel)
extern const Art kBrick, kSteel, kWater0, kWater1, kTrees, kIce;
extern const Art kEagle, kEagleDead;
extern const Art kBullet;
extern const Art kExplosion;
extern const Art kTankIcon;  // HUD, single color 'X'
extern const Art kFlag;      // K pole, O cloth
extern const Art kStar;      // Y fill, W outline
// Power-up icons. Grenade: G D W; Helmet: Y W; Shovel: S B; Clock: R W K.
extern const Art kGrenade;
extern const Art kHelmet;
extern const Art kShovel;
extern const Art kClock;
extern const Art kExtraLife;  // Y body, D tracks, K wheels

// 5×7 glyph for c, or nullptr if the font doesn't have it. 'X' is ink.
const Art* glyph(char c);

}  // namespace sprites
