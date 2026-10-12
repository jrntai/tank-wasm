#pragma once

#include <array>
#include <cstdint>
#include <optional>
#include <string_view>
#include <vector>

// Game simulation. Rules are specified in specs/; comments cite spec IDs.
namespace sim {

// --- Geometry (ARN) ---
constexpr int kCellSize = 8;
constexpr int kGridSize = 26;  // cells per side
constexpr int kArenaSize = kCellSize * kGridSize;  // 208 px
constexpr int kTankSize = 16;
constexpr int kBulletSize = 4;

constexpr int kBaseX = 96, kBaseY = 192;
constexpr int kPlayerSpawnX = 64, kPlayerSpawnY = 192;
constexpr int kEnemySpawnCount = 3;
constexpr int kEnemySpawnX[kEnemySpawnCount] = {96, 192, 0};  // center, right, left
constexpr int kEnemySpawnY = 0;

// --- Timing ---
constexpr float kTickSeconds = 1.0f / 60.0f;
constexpr int kPlayerSpeed = 2;  // half-px per tick
constexpr int kPlayerReserveLives = 2;
constexpr int kRespawnDelay = 60;
constexpr int kShieldTicks = 180;
constexpr int kNormalBulletSpeed = 3;  // px per tick
constexpr int kFastBulletSpeed = 5;
constexpr int kMaxEnemiesOnField = 4;
constexpr int kEnemySpawnInterval = 120;
constexpr int kSmallExplosionTicks = 8;
// PWR
constexpr int kPowerUpSize = 16;
constexpr int kPowerUpTicks = 600;
constexpr int kMaxPlayerLevel = 3;
constexpr int kSecondShotDelay = 8;
constexpr int kBonusLineupSlots[] = {3, 10, 17};  // 4th, 11th, 18th enemy
constexpr int kHelmetTicks = 600;
constexpr int kShovelTicks = 1200;
constexpr int kClockTicks = 600;
constexpr int kMaxLives = 9;
// The half-tile brick wall around the base, as (column, row) cells (ARN-4, PWR-16).
constexpr int kBaseWallCells[8][2] = {{11, 23}, {12, 23}, {13, 23}, {14, 23},
                                      {11, 24}, {14, 24}, {11, 25}, {14, 25}};
constexpr int kBigExplosionTicks = 16;

enum class Terrain : uint8_t { Empty, Brick, Steel, Water, Trees, Ice };
bool blocks_tanks(Terrain t);
bool blocks_bullets(Terrain t);

// The renderer rotates sprites by int(Dir); keep this order.
enum class Dir : uint8_t { Up, Down, Left, Right };
enum class Side : uint8_t { Player, Enemy };
enum class EnemyType : uint8_t { Basic, Fast, Power, Armor };
enum class Phase : uint8_t { Playing, GameOver, StageClear };
enum class PowerUpType : uint8_t { Star, Grenade, Helmet, Shovel, Clock, Tank };
constexpr int kPowerUpTypeCount = 6;

struct EnemyStats {
  int speed;  // half-px per tick
  int bullet_speed;
  int hp;
};
const EnemyStats& stats(EnemyType type);

struct Input {
  bool up = false, down = false, left = false, right = false;
  bool fire = false;
  bool start = false, back = false;  // menu keys (MNU); World ignores them
};

struct Tank {
  int id = 0;
  int x = 0, y = 0;
  Dir facing = Dir::Up;
  Side side = Side::Player;
  EnemyType type = EnemyType::Basic;  // enemies only
  int hp = 1;
  int move_acc = 0;  // accumulated half-px
  int shield_ticks = 0;
  bool bonus = false;     // enemies: flashing tank that drops a power-up (PWR-1)
  int level = 0;          // player: star level 0..3 (PWR)
  int fire_cooldown = 0;  // player: ticks until a second bullet is allowed (PWR-8)
};

struct Bullet {
  int x = 0, y = 0;
  Dir dir = Dir::Up;
  Side side = Side::Player;
  int owner_id = 0;
  int speed = kNormalBulletSpeed;
  bool breaks_steel = false;  // PWR-9
  bool alive = true;
};

struct PowerUp {
  int x = 0, y = 0;  // top-left, on the 8 px grid
  int ticks_left = kPowerUpTicks;
  PowerUpType type = PowerUpType::Star;
};

struct Explosion {
  int cx = 0, cy = 0;  // centre
  bool big = false;
  int ticks_left = 0;
};

struct Base {
  int x = kBaseX, y = kBaseY;
  bool alive = true;
};

struct Stage {
  int number = 1;
  std::string_view name;
  std::array<std::string_view, kGridSize> rows;  // kGridSize chars each (ARN)
  std::vector<EnemyType> lineup;
};

// All stages, in order (PRG). stage_for() wraps numbers past the last stage.
const std::vector<Stage>& stages();
const Stage& stage_for(int number);
const Stage& stage1();
// No terrain, no enemies: a blank arena for tests.
Stage empty_stage();

struct World {
  explicit World(const Stage& stage, uint32_t seed = 1);

  // Advances the simulation by exactly one fixed tick.
  void tick(const Input& in);

  Terrain terrain_at(int cx, int cy) const { return terrain[cy * kGridSize + cx]; }
  void set_terrain(int cx, int cy, Terrain t) { terrain[cy * kGridSize + cx] = t; }
  bool bullet_in_flight(int owner_id) const;
  int enemies_waiting() const { return int(lineup.size()) - next_lineup_index; }

  // Adds an enemy directly, bypassing the lineup. For tests and debugging.
  Tank& add_enemy(EnemyType type, int x, int y, Dir facing = Dir::Down);
  // Destroys the player as if hit by an unshielded enemy bullet.
  void destroy_player();
  // Drops a random power-up at a random valid position, replacing any current item (PWR-3/4/13).
  void drop_powerup();

  int stage_number = 1;
  Phase phase = Phase::Playing;
  uint32_t tick_count = 0;
  uint32_t rng_state;
  // When false, enemies skip their random turns and shots (ENM AI steps 2 and 3).
  // Tests use this to measure movement and hits in isolation.
  bool enemy_random_actions = true;

  std::array<Terrain, kGridSize * kGridSize> terrain{};
  Base base;

  Tank player;
  bool player_alive = true;
  int lives = kPlayerReserveLives;
  int respawn_timer = 0;

  std::vector<Tank> enemies;
  std::vector<EnemyType> lineup;
  int next_lineup_index = 0;
  int spawn_timer = 0;
  int next_spawn_point = 0;
  int kills = 0;

  std::vector<Bullet> bullets;
  std::vector<Explosion> explosions;
  std::optional<PowerUp> powerup;
  int shovel_ticks = 0;  // base wall is steel while > 0 (PWR-16)
  int freeze_ticks = 0;  // enemies frozen while > 0 (PWR-17)

 private:
  int next_id_ = 1;

  uint32_t random();
  void spawn_player();
  void update_spawning();
  void update_player(const Input& in);
  void update_enemies();
  void update_bullets();
  bool resolve_bullet(Bullet& b);  // returns true if the bullet was removed
  void hit_enemy(Tank& e);
  void destroy_enemy(Tank& e);
  void collect(PowerUpType type);
  void set_base_wall(Terrain t);
  void destroy_base();
  void fire(Tank& t, int speed, bool breaks_steel = false);
  int bullets_in_flight(int owner_id) const;
  void turn(Tank& t, Dir d);
  int move(Tank& t, int speed);  // returns px it failed to move (0 = not blocked)
  bool can_occupy(const Tank& t, int nx, int ny) const;
  void add_explosion(int cx, int cy, bool big);
};

}  // namespace sim
