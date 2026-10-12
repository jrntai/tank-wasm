#include "platform/sprites.h"

#include <cstddef>

namespace sprites {

namespace {

const char* const kTankRows[] = {
    ".......BB.......",
    ".......BB.......",
    ".......BB.......",
    "TTT....BB....TTT",
    "tTtggggBBggggtTt",
    "TTTgGGGBBGGGgTTT",
    "tTtgGhhBBhhGgtTt",
    "TTTgGhhhhhhGgTTT",
    "tTtgGhhhhhhGgtTt",
    "TTTgGhhhhhhGgTTT",
    "tTtgGGhhhhGGgtTt",
    "TTTgGGGGGGGGgTTT",
    "tTtggggggggggtTt",
    "TTTgGGGGGGGGgTTT",
    "tTtggggggggggtTt",
    "TTT..........TTT",
};

const char* const kBrickRows[] = {
    "RRRmRRRR",
    "RRRmRRRR",
    "rrrmrrrr",
    "mmmmmmmm",
    "RRRRRRRm",
    "RRRRRRRm",
    "rrrrrrrm",
    "mmmmmmmm",
};

const char* const kSteelRows[] = {
    "WWWWWWWd",
    "WsssssDd",
    "WsWWWdDd",
    "WsWssdDd",
    "WsWssdDd",
    "WsddddDd",
    "WDDDDDDd",
    "dddddddd",
};

const char* const kWater0Rows[] = {
    "bbbbbbbb",
    "bwwbbbbb",
    "bbbwwbbb",
    "bbbbbbbb",
    "bbbbbwwb",
    "bwbbbbbw",
    "bbwwbbbb",
    "bbbbbbbb",
};

const char* const kWater1Rows[] = {
    "bbbbbbbb",
    "bbbbbwwb",
    "bwbbbbbw",
    "bbbbbbbb",
    "bwwbbbbb",
    "bbbwwbbb",
    "bbbbbbbb",
    "bwwbbbbb",
};

const char* const kTreesRows[] = {
    "fFFfFFf.",
    "FFfFFfFf",
    "fFFffFFf",
    "FfFFfFFF",
    "FFfFFfF.",
    ".fFFfFFf",
    "FFfFFffF",
    "fFF.FFfF",
};

const char* const kIceRows[] = {
    "Iiiiiiii",
    "iIiiiiii",
    "iiIiiiii",
    "iiiIiiii",
    "iiiiIiii",
    "iiiiiIii",
    "iiiiiiIi",
    "iiiiiiiI",
};

const char* const kEagleRows[] = {
    "................",
    ".X............X.",
    ".XX..XXXXXX..XX.",
    ".XXX.XXXXXX.XXX.",
    ".XXXXX.XX.XXXXX.",
    "..XXXXXXXXXXXX..",
    "..XXX.XXXX.XXX..",
    "...XXXXXXXXXX...",
    "....XXXXXXXX....",
    ".....XXXXXX.....",
    "......XXXX......",
    ".....XXXXXX.....",
    "....XX.XX.XX....",
    "...XXXXXXXXXX...",
    "..XXXXXXXXXXXX..",
    "................",
};

const char* const kEagleDeadRows[] = {
    "................",
    "................",
    "..K.............",
    "..KOOO..........",
    "..KOOOOO........",
    "..KOOO..........",
    "..K.............",
    "..K.............",
    "..K.....X..X....",
    "..K...XXXXXXX...",
    "..K..XX.XX.XXX..",
    ".XXXXXXXXXXXXXX.",
    "XXXX.XXXXX.XXXXX",
    "XXXXXXXXXXXXXXXX",
    "................",
    "................",
};

const char* const kBulletRows[] = {
    ".XX.",
    "XXXX",
    "XXXX",
    ".XX.",
};

const char* const kExplosionRows[] = {
    "................",
    "......W.........",
    "..W...WW...W....",
    "...W.WRRW.W.....",
    "....WRRRRW......",
    "..WWRRPPRRW.WW..",
    "...WRPPPPRRW....",
    ".WWRRPPWPPRRWW..",
    "...WRPPPPPRW....",
    "..WWRRPPPRRWW...",
    "....WRRRRRW.....",
    "...W.WRRRW.W....",
    "..W...WWW...W...",
    "......W.........",
    "................",
    "................",
};

const char* const kTankIconRows[] = {
    "X..X..X.",
    "XXXXXXX.",
    "XX.X.XX.",
    "XXXXXXX.",
    "XX...XX.",
    "XXXXXXX.",
    "X.....X.",
    "........",
};

const char* const kFlagRows[] = {
    "................",
    ".K..............",
    ".KOOO...........",
    ".KOOOOOO........",
    ".KOOOOOOOOO.....",
    ".KOOOOOO........",
    ".KOOO...........",
    ".K..............",
    ".K..............",
    ".K..............",
    ".K..............",
    ".K..............",
    ".K..............",
    ".K..............",
    "KKK.............",
    "................",
};

const char* const kStarRows[] = {
    "................",
    ".......WW.......",
    "......WYYW......",
    "......WYYW......",
    ".....WYYYYW.....",
    "WWWWWWYYYYWWWWWW",
    ".WYYYYYYYYYYYYW.",
    "..WYYYYYYYYYYW..",
    "...WYYYYYYYYW...",
    "....WYYYYYYW....",
    "...WYYYYYYYYW...",
    "...WYYYWWYYYW...",
    "..WYYYW..WYYYW..",
    "..WYYW....WYYW..",
    ".WYW........WYW.",
    ".WW..........WW.",
};

const char* const kGrenadeRows[] = {
    "................",
    ".........WW.....",
    "........W..W....",
    "......DDDD......",
    ".....DDDDDD.....",
    "....GGGGGGGG....",
    "...GGDGGGGDGG...",
    "...GGGGGGGGGG...",
    "...GDGGGGGGDG...",
    "...GGGGGGGGGG...",
    "...GGDGGGGDGG...",
    "....GGGGGGGG....",
    ".....GGGGGG.....",
    "................",
    "................",
    "................",
};

const char* const kHelmetRows[] = {
    "................",
    "................",
    "......YYYY......",
    "....YYWWYYYY....",
    "...YYWYYYYYYY...",
    "..YYWYYYYYYYYY..",
    "..YYYYYYYYYYYY..",
    "..YYYYYYYYYYYY..",
    "..YYYYYYYYYYYY..",
    ".YYYYYYYYYYYYYY.",
    ".YYYYYYYYYYYYYY.",
    "YYYYYYYYYYYYYYYY",
    "YY............YY",
    "................",
    "................",
    "................",
};

const char* const kShovelRows[] = {
    "................",
    "..........BB....",
    ".........BBBB...",
    "..........BB....",
    ".........BB.....",
    "........BB......",
    ".......BB.......",
    "......SS........",
    ".....SSSS.......",
    "....SSSSSS......",
    "...SSSSSSS......",
    "...SSSSSS.......",
    "....SSSS........",
    ".....SS.........",
    "................",
    "................",
};

const char* const kClockRows[] = {
    "................",
    ".....RRRRRR.....",
    "....RWWWWWWR....",
    "...RWWWKWWWWR...",
    "..RWWWWKWWWWWR..",
    "..RWWWWKWWWWWR..",
    "..RWWWWKWWWWWR..",
    "..RWWWWKKKKWWR..",
    "..RWWWWWWWWWWR..",
    "..RWWWWWWWWWWR..",
    "...RWWWWWWWWR...",
    "....RWWWWWWR....",
    ".....RRRRRR.....",
    "................",
    "................",
    "................",
};

const char* const kExtraLifeRows[] = {
    "................",
    "................",
    "................",
    "......YYYYY.....",
    ".....YYYYYYYYYYY",
    "......YYYYY.....",
    "...YYYYYYYYYY...",
    "..YYYYYYYYYYYY..",
    ".DDDDDDDDDDDDDD.",
    "DKDDKDDKDDKDDKDD",
    ".DDDDDDDDDDDDDD.",
    "................",
    "................",
    "................",
    "................",
    "................",
};

struct Glyph {
  char c;
  const char* rows[7];
};

const Glyph kFont[] = {
    {'0', {".XXX.", "X...X", "X..XX", "X.X.X", "XX..X", "X...X", ".XXX."}},
    {'1', {"..X..", ".XX..", "..X..", "..X..", "..X..", "..X..", ".XXX."}},
    {'2', {".XXX.", "X...X", "....X", "...X.", "..X..", ".X...", "XXXXX"}},
    {'3', {"XXXXX", "...X.", "..X..", "...X.", "....X", "X...X", ".XXX."}},
    {'4', {"...X.", "..XX.", ".X.X.", "X..X.", "XXXXX", "...X.", "...X."}},
    {'5', {"XXXXX", "X....", "XXXX.", "....X", "....X", "X...X", ".XXX."}},
    {'6', {"..XX.", ".X...", "X....", "XXXX.", "X...X", "X...X", ".XXX."}},
    {'7', {"XXXXX", "....X", "...X.", "..X..", ".X...", ".X...", ".X..."}},
    {'8', {".XXX.", "X...X", "X...X", ".XXX.", "X...X", "X...X", ".XXX."}},
    {'9', {".XXX.", "X...X", "X...X", ".XXXX", "....X", "...X.", ".XX.."}},
    {'A', {".XXX.", "X...X", "X...X", "XXXXX", "X...X", "X...X", "X...X"}},
    {'C', {".XXX.", "X...X", "X....", "X....", "X....", "X...X", ".XXX."}},
    {'E', {"XXXXX", "X....", "X....", "XXXX.", "X....", "X....", "XXXXX"}},
    {'G', {".XXX.", "X...X", "X....", "X.XXX", "X...X", "X...X", ".XXXX"}},
    {'I', {".XXX.", "..X..", "..X..", "..X..", "..X..", "..X..", ".XXX."}},
    {'L', {"X....", "X....", "X....", "X....", "X....", "X....", "XXXXX"}},
    {'M', {"X...X", "XX.XX", "X.X.X", "X.X.X", "X...X", "X...X", "X...X"}},
    {'N', {"X...X", "XX..X", "X.X.X", "X..XX", "X...X", "X...X", "X...X"}},
    {'O', {".XXX.", "X...X", "X...X", "X...X", "X...X", "X...X", ".XXX."}},
    {'P', {"XXXX.", "X...X", "X...X", "XXXX.", "X....", "X....", "X...."}},
    {'R', {"XXXX.", "X...X", "X...X", "XXXX.", "X.X..", "X..X.", "X...X"}},
    {'S', {".XXXX", "X....", "X....", ".XXX.", "....X", "....X", "XXXX."}},
    {'T', {"XXXXX", "..X..", "..X..", "..X..", "..X..", "..X..", "..X.."}},
    {'V', {"X...X", "X...X", "X...X", "X...X", "X...X", ".X.X.", "..X.."}},
    {'B', {"XXXX.", "X...X", "X...X", "XXXX.", "X...X", "X...X", "XXXX."}},
    {'D', {"XXXX.", "X...X", "X...X", "X...X", "X...X", "X...X", "XXXX."}},
    {'F', {"XXXXX", "X....", "X....", "XXXX.", "X....", "X....", "X...."}},
    {'H', {"X...X", "X...X", "X...X", "XXXXX", "X...X", "X...X", "X...X"}},
    {'J', {"..XXX", "...X.", "...X.", "...X.", "...X.", "X..X.", ".XX.."}},
    {'K', {"X...X", "X..X.", "X.X..", "XX...", "X.X..", "X..X.", "X...X"}},
    {'Q', {".XXX.", "X...X", "X...X", "X...X", "X.X.X", "X..X.", ".XX.X"}},
    {'U', {"X...X", "X...X", "X...X", "X...X", "X...X", "X...X", ".XXX."}},
    {'W', {"X...X", "X...X", "X...X", "X.X.X", "X.X.X", "XX.XX", "X...X"}},
    {'X', {"X...X", "X...X", ".X.X.", "..X..", ".X.X.", "X...X", "X...X"}},
    {'Y', {"X...X", "X...X", ".X.X.", "..X..", "..X..", "..X..", "..X.."}},
    {'Z', {"XXXXX", "....X", "...X.", "..X..", ".X...", "X....", "XXXXX"}},
    {'/', {"....X", "....X", "...X.", "..X..", ".X...", "X....", "X...."}},
    {'-', {".....", ".....", ".....", "XXXXX", ".....", ".....", "....."}},
};

}  // namespace

const Art kTank{kTankRows, 16, 16};
const Art kBrick{kBrickRows, 8, 8};
const Art kSteel{kSteelRows, 8, 8};
const Art kWater0{kWater0Rows, 8, 8};
const Art kWater1{kWater1Rows, 8, 8};
const Art kTrees{kTreesRows, 8, 8};
const Art kIce{kIceRows, 8, 8};
const Art kEagle{kEagleRows, 16, 16};
const Art kEagleDead{kEagleDeadRows, 16, 16};
const Art kBullet{kBulletRows, 4, 4};
const Art kExplosion{kExplosionRows, 16, 16};
const Art kTankIcon{kTankIconRows, 8, 8};
const Art kFlag{kFlagRows, 16, 16};
const Art kStar{kStarRows, 16, 16};
const Art kGrenade{kGrenadeRows, 16, 16};
const Art kHelmet{kHelmetRows, 16, 16};
const Art kShovel{kShovelRows, 16, 16};
const Art kClock{kClockRows, 16, 16};
const Art kExtraLife{kExtraLifeRows, 16, 16};

const Art* glyph(char c) {
  static Art arts[sizeof kFont / sizeof kFont[0]];
  static bool init = false;
  if (!init) {
    for (size_t i = 0; i < sizeof kFont / sizeof kFont[0]; ++i) arts[i] = {kFont[i].rows, 5, 7};
    init = true;
  }
  for (size_t i = 0; i < sizeof kFont / sizeof kFont[0]; ++i)
    if (kFont[i].c == c) return &arts[i];
  return nullptr;
}

}  // namespace sprites
