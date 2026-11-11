// Harvest field (Thanksgiving). Port of simulator/themes/thanksgiving.js; keep the two in sync.
#include <algorithm>
#include <cstring>

#include "../themes.h"

namespace pc {
namespace {

// A warmer, golden sky than the default.
const SkyKey SKY_GOLD[] = {
    {0, 0x0a0c24, 0x1c1a3a},          {real(5.5), 0x0a0c24, 0x1c1a3a}, {7, 0x4a4a8c, 0xf2a66b},
    {9, 0x7ab0de, 0xf2d9a8},          {15, 0x7ab0de, 0xf2d9a8},        {17, 0x6a5a9a, 0xf59a50},
    {real(18.5), 0x1c1840, 0x4a2a50}, {24, 0x0a0c24, 0x1c1a3a},
};

constexpr uint32_t NIGHT_TINT = 0x1a1a44;
namespace COL {
constexpr uint32_t sun = 0xffd870, moon = 0xeef0ff, hillDay = 0x8e9a48, hillNight = 0x3a3450, barn = 0xa8322a,
                   barnShade = 0x7e2420, roof = 0x4a2620, trim = 0xf0ead8, silo = 0x9a9aa4, siloShade = 0x6e6e7a,
                   loftLit = 0xffcc4a, post = 0x6a4a20, hat = 0x6a4a20, face = 0xe8c070, shirt = 0xb03a2a,
                   straw = 0xf0d060, bale = 0xf0d070, baleLight = 0xfff0a0, baleShade = 0xc8a040,
                   baleEdge = 0x7a5a20, twine = 0x9a6a28, stalk = 0x7a6a28, leaf = 0xb8a048, leafDark = 0x5e5020,
                   tassel = 0xf0dc90, corn = 0xf0c840, clock = 0xfff0d0, clockShadow = 0x2a1408;
const uint32_t field[3] = {0xc49040, 0xae7c34, 0x8a6228};
}  // namespace COL

namespace TAIL {
constexpr uint32_t band = 0x8e4a20, bandAlt = 0x6e3414, inner = 0x4e2a10, dark = 0x3a1e0a, edge = 0x24120a,
                   tip = 0xf4e6c4;
}
namespace TURKEY {
constexpr uint32_t body = 0x3e220e, sheen = 0x6e4826, wing = 0x5e3a1c, neck = 0xa8c8e4, head = 0xd0e4f6,
                   eye = 0x0a0a0a, beak = 0xf0c040, wattle = 0xe02828, leg = 0xe08a30;
}
const uint32_t LEAF_COLORS[] = {0xf08a24, 0xd84a2a, 0xf0c838, 0xa8261c};
constexpr int LEAF_COUNT = 4;

const char* const PUMPKIN[] = {"...s...", ".oOOOo.", "oOOoOOo", "oOOoOOo", "oOOoOOo", ".ooooo."};
const char* const PUMPKIN_SM[] = {".s..", "oOOo", "oOOo", ".oo."};
const char* const GOURD_WHITE[] = {".s.", "WWW", "WwW", ".W."};
const char* const GOURD_STRIPE[] = {"..s", ".YG", "YGY", "GYG", ".G."};

struct Corn {
  int x, base, h;
};
const Corn CORN[] = {{2, 51, 22}, {6, 52, 24}, {10, 51, 20}, {13, 53, 23}};

uint32_t shade(uint32_t c, real day) {
  return dim(mix(c, NIGHT_TINT, real(0.3) * (1 - day)), real(0.5) + real(0.5) * day);
}

class Thanksgiving : public Theme {
 public:
  explicit Thanksgiving(Rng& r) : rng(r) {
    // Same order of random draws as create() in thanksgiving.js.
    turkey.nextPeck = 3 + rng.flt() * 4;
    turkey.nextPuff = 6 + rng.flt() * 6;
    feathers.reserve(32);
    for (int i = 0; i < 6; i++) {
      Leaf l;
      spawnLeaf(l, true);
      leaves.push_back(l);
    }
  }

  void update(real dt, const Ctx& ctx) override {
    t += dt;

    if (turkey.peck > 0) turkey.peck -= dt;
    else if ((turkey.nextPeck -= dt) <= 0) {
      turkey.peck = real(0.7);
      turkey.nextPeck = 3 + rng.flt() * 5;
    }
    if (turkey.puff > 0) turkey.puff -= dt;
    else if ((turkey.nextPuff -= dt) <= 0) {
      turkey.puff = real(1.4);
      turkey.nextPuff = 8 + rng.flt() * 8;
    }
    if (turkey.flap > 0) turkey.flap -= dt;

    if (ctx.shake) {
      turkey.flap = real(1.6);
      for (int i = 0; i < 16; i++) {
        real a = kPi * (real(0.15) + rng.flt() * real(0.7));
        Feather f;
        f.x = 50 + std::cos(a) * 9;
        f.y = 51 - std::sin(a) * 9;
        f.vx = std::cos(a) * (8 + rng.flt() * 10);
        f.vy = -std::sin(a) * (8 + rng.flt() * 10);
        f.ph = rng.flt() * real(6.283);
        f.col = rng.chance(real(0.4)) ? TAIL::tip : TAIL::band;
        f.life = real(2.5) + rng.flt() * real(1.5);
        f.age = 0;
        feathers.push_back(f);
      }
    }
    for (int i = int(feathers.size()) - 1; i >= 0; i--) {
      Feather& f = feathers[i];
      f.age += dt;
      f.vx *= real(0.95);
      f.vy = jsmin(f.vy * real(0.95) + 10 * dt, 4);
      f.x += (f.vx + std::sin(t * 4 + f.ph) * 4) * dt;
      f.y += f.vy * dt;
      if (f.age > f.life) feathers.erase(feathers.begin() + i);
    }

    for (Leaf& l : leaves) {
      l.x += (l.vx + std::sin(t * 2 + l.ph) * 3) * dt;
      l.y += l.vy * dt;
      if (l.x > 66 || l.y > 62) spawnLeaf(l, false);
    }
  }

  void draw(Canvas& cv, const Ctx& ctx) override {
    real hf = ctx.hourFloat;
    long key = long(std::floor(hf * 30));
    if (key != bgKey) {
      bgKey = key;
      buildBackground(hf);
    }
    std::memcpy(cv.px, bg.px, sizeof(bg.px));
    real day = daylight(hf);
    night = 1 - day;
    amb = real(0.55) + real(0.45) * day;

    if (night > real(0.2)) {
      for (const Star& s : stars) {
        real tw = real(0.55) + real(0.45) * std::sin(t * (real(1.5) + (s.i % 5) * real(0.4)) + s.i * real(2.1));
        cv.blend(s.x, s.y, 0xdfe6ff, (night - real(0.2)) * tw);
      }
    }

    drawCorn(cv);
    drawTurkey(cv);

    for (const Leaf& l : leaves) cv.set(l.x, l.y, dim(l.col, jsmax(amb, real(0.8))));
    for (const Feather& f : feathers) {
      real a = clamp((f.life - f.age) / real(0.8), 0, 1);
      cv.blend(f.x, f.y, dim(f.col, jsmax(amb, real(0.8))), a);
      cv.blend(f.x, f.y + 1, dim(TAIL::band, jsmax(amb, real(0.8))), a);  // quill
    }

    drawClock(cv, ctx, COL::clock, COL::clockShadow);
  }

 private:
  struct Star {
    int x, y, i;
  };
  struct Turkey {
    real peck = 0, nextPeck = 0, puff = 0, nextPuff = 0, flap = 0;
  };
  struct Feather {
    real x, y, vx, vy, ph;
    uint32_t col;
    real life, age;
  };
  struct Leaf {
    real x = 0, y = 0, vx = 0, vy = 0, ph = 0;
    uint32_t col = 0;
  };

  Rng& rng;
  real t = 0;
  Canvas bg;
  long bgKey = -1;
  std::vector<Star> stars;
  uint8_t skyMask[64 * 64] = {};
  real amb = 1;
  real night = 0;
  Turkey turkey;
  std::vector<Feather> feathers;
  std::vector<Leaf> leaves;

  void solid(int x, int y, uint32_t c) {
    bg.set(x, y, c);
    if (x >= 0 && x < 64 && y >= 0 && y < 64) skyMask[y * 64 + x] = 0;
  }

  void bale(int x0, int y0, int x1, int y1, real day) {
    for (int y = y0; y <= y1; y++) {
      for (int x = x0; x <= x1; x++) {
        uint32_t c = COL::bale;
        if (y == y0) c = COL::baleLight;
        else if (y == y1 - 1) c = COL::baleShade;
        else if (hash(x, y, 5) == 0) c = COL::baleShade;
        if ((x - x0) % 4 == 2 && y != y0) c = COL::twine;
        if (y == y1 || x == x0 || x == x1) c = COL::baleEdge;
        solid(x, y, shade(c, day));
      }
    }
  }

  // ---------- static background ----------
  void buildBackground(real hf) {
    real day = daylight(hf);
    real nt = 1 - day;
    SkyColors sky = skyAt(hf, SKY_GOLD);
    vgradient(bg, 0, 0, 63, 42, sky.top, sky.bottom);
    std::memset(skyMask, 1, sizeof(skyMask));
    auto S = [&](uint32_t c) { return shade(c, day); };

    // sun by day, moon by night
    for (int y = 14; y <= 28; y++) {
      for (int x = 6; x <= 24; x++) {
        real d = std::hypot(real(x - 15), real(y - 20));
        if (day > 0) {
          if (d <= real(2.6)) bg.set(x, y, mix(bg.get(x, y), COL::sun, day));
          else if (d < 7) bg.blend(x, y, COL::sun, day * real(0.25) * (1 - (d - real(2.6)) / real(4.4)));
        }
        if (nt > 0 && d <= real(2.8)) {
          bool crescent = std::hypot(x - real(16.5), real(y - 19)) > real(2.4);
          if (crescent) bg.set(x, y, mix(bg.get(x, y), COL::moon, nt));
        }
      }
    }

    // rolling hills
    uint32_t hill = mix(COL::hillNight, COL::hillDay, day);
    for (int x = 0; x < 64; x++) {
      int hy = 37 + int(jsround(std::sin(x * real(0.1) + 1) * real(1.5) + std::sin(x * real(0.27)) * real(0.8)));
      for (int y = hy; y <= 42; y++) solid(x, y, hash(x, y, 7) == 0 ? dim(hill, real(0.9)) : hill);
    }

    // field with furrows
    for (int y = 42; y < 64; y++) {
      real depth = real(y - 42) / 21;
      for (int x = 0; x < 64; x++) {
        long row = long(std::floor(std::pow(real(y - 41), real(0.8))));
        uint32_t c = COL::field[row % 2 == 0 ? 0 : 1];
        if (hash(x, y, 9) == 0) c = COL::field[2];
        solid(x, y, S(mix(c, COL::field[2], depth * real(0.35))));
      }
    }

    // barn with gambrel roof
    for (int y = 30; y <= 41; y++)
      for (int x = 45; x <= 59; x++) solid(x, y, S(x >= 56 ? COL::barnShade : COL::barn));
    for (int y = 23; y <= 29; y++) {
      real half = y < 26 ? real(2 + (y - 23) * 2) : 7 + (y - 26) * real(0.5);
      for (int x = int(jsround(52 - half)); x <= int(jsround(52 + half)); x++) solid(x, y, S(COL::roof));
    }
    // door with white X trim
    for (int y = 34; y <= 41; y++) {
      for (int x = 49; x <= 55; x++) {
        bool edge = x == 49 || x == 55 || y == 34;
        int dg = int(jsround(real((y - 34) * 6) / 7));
        bool diag = x - 49 == dg || 55 - x == dg;
        solid(x, y, S(edge || diag ? COL::trim : COL::barnShade));
      }
    }
    // hayloft window with trim, lit at night
    for (int y = 26; y <= 29; y++)
      for (int x = 51; x <= 53; x++) solid(x, y, S(COL::trim));
    uint32_t loft = mix(0x2a1410, COL::loftLit, nt);
    solid(52, 27, loft);
    solid(52, 28, loft);

    // silo
    for (int y = 25; y <= 41; y++)
      for (int x = 60; x <= 63; x++) solid(x, y, S(x >= 62 ? COL::siloShade : COL::silo));
    solid(61, 24, S(COL::silo));
    solid(62, 24, S(COL::siloShade));
    for (int y = 27; y <= 41; y += 4)
      for (int x = 60; x <= 63; x++) solid(x, y, S(COL::siloShade));

    // scarecrow in the distance
    for (int y = 31; y <= 44; y++) solid(29, y, S(COL::post));
    for (int x = 25; x <= 33; x++) solid(x, 34, S(COL::shirt));
    solid(24, 34, S(COL::straw));
    solid(34, 34, S(COL::straw));
    for (int y = 33; y <= 38; y++)
      for (int x = 27; x <= 31; x++) solid(x, y, S((x + y) % 3 == 0 ? 0x7a2418 : COL::shirt));
    solid(28, 39, S(COL::straw));
    solid(30, 39, S(COL::straw));
    solid(28, 31, S(COL::face));
    solid(29, 31, S(COL::face));
    solid(30, 31, S(COL::face));
    solid(29, 32, S(COL::face));
    for (int x = 26; x <= 32; x++) solid(x, 30, S(COL::hat));
    for (int x = 28; x <= 30; x++) solid(x, 29, S(COL::hat));
    solid(28, 28, S(COL::hat));
    solid(29, 28, S(COL::hat));
    solid(30, 28, S(COL::hat));

    // hay bales
    bale(15, 52, 26, 57, day);
    bale(17, 47, 24, 51, day);

    // pumpkins and gourds
    Palette pal{{'O', S(0xe0701a)}, {'o', S(0xa84a10)}, {'s', S(0x3a6a20)}, {'W', S(0xf0ead8)},
                {'w', S(0xc8c0a8)}, {'Y', S(0xe8c040)}, {'G', S(0x7a9a30)}};
    bg.sprite(PUMPKIN, 30, 56, pal);
    bg.sprite(PUMPKIN_SM, 38, 58, pal);
    bg.sprite(GOURD_WHITE, 26, 58, pal);
    bg.sprite(GOURD_STRIPE, 42, 57, pal);

    stars.clear();
    for (int i = 0; i < 30 && stars.size() < 18; i++) {
      int x = int(hash(i, 51, 64));
      int y = 14 + int(hash(i, 53, 20));
      if (skyMask[y * 64 + x] && std::hypot(real(x - 15), real(y - 20)) > 5) stars.push_back({x, y, i});
    }
  }

  // ---------- turkey ----------
  void drawTurkey(Canvas& cv) {
    const int cx = 50;
    const int cy = 51;
    real puffAmt = 0;
    if (turkey.flap > 0) puffAmt = 2;
    else if (turkey.puff > 0)
      puffAmt = real(1.5) * jsmin(jsmin(1, turkey.puff * 3), (real(1.4) - turkey.puff) * 3 + 1);
    real R = 8 + puffAmt + std::sin(t * 2) * real(0.2);
    auto s = [&](uint32_t c) { return dim(c, amb); };

    // tail fan (upper half circle)
    for (int y = int(std::floor(cy - R)); y <= cy + 1; y++) {
      for (int x = int(std::floor(cx - R)); x <= int(std::ceil(cx + R)); x++) {
        int dx = x - cx;
        int dy = y - cy;
        real d = std::hypot(real(dx), real(dy));
        if (d > R) continue;
        real k = d / R;
        real ang = std::atan2(-real(dy), real(dx));  // -real(0) keeps JS's -0
        long feather = long(std::floor((ang / kPi) * 9));
        uint32_t c;
        if (k > real(0.86)) c = TAIL::tip;
        else if (k > real(0.74)) c = TAIL::edge;
        else if (k > real(0.45)) c = feather % 2 ? TAIL::band : TAIL::bandAlt;
        else c = feather % 2 ? TAIL::inner : TAIL::dark;
        cv.set(x, y, s(c));
      }
    }

    // body
    for (int y = 50; y <= 57; y++) {
      for (int x = 46; x <= 56; x++) {
        real ex = (x - 51) / real(4.6);
        real ey = (y - 54) / real(3.6);
        if (ex * ex + ey * ey > 1) continue;
        uint32_t c = TURKEY::body;
        if (y <= 51) c = TURKEY::sheen;
        else if (ex < real(-0.1) && ey > real(-0.3) && (x + y) % 2 == 0) c = TURKEY::wing;
        cv.set(x, y, s(c));
      }
    }

    // wings flapping on shake
    if (turkey.flap > 0) {
      bool up = long(std::floor(t * 10)) % 2 == 0;
      for (int i = 0; i < 4; i++) {
        int wy = up ? 50 - i : 52 + i / 2;
        uint32_t c = i == 3 ? TAIL::tip : TURKEY::sheen;
        cv.set(46 - i, wy, s(c));
        cv.set(46 - i, wy + 1, s(TURKEY::body));
        cv.set(56 + i, wy, s(c));
        cv.set(56 + i, wy + 1, s(TURKEY::body));
      }
    }

    // legs
    for (int y = 57; y <= 59; y++) {
      cv.set(49, y, s(TURKEY::leg));
      cv.set(53, y, s(TURKEY::leg));
    }
    cv.set(48, 59, s(TURKEY::leg));
    cv.set(54, 59, s(TURKEY::leg));

    // neck and head: bob while idle, dip down while pecking
    int bob = int(long(std::floor(t * real(2.2))) % 2);
    real p = turkey.peck > 0 ? std::sin(jsmin(1, turkey.peck / real(0.7)) * kPi) : 0;
    int hx = 56 + bob + int(jsround(p * 2));
    int hy = 47 + int(jsround(p * 7));
    const int nx = 55;
    for (int i = 0; i <= 3; i++) {
      real y = jsround(51 - i * (real(51 - hy - 1) / 3));
      real x = jsround(nx + real(i * (hx - nx)) / 3);
      cv.set(x, y, s(TURKEY::neck));
    }
    cv.rect(hx, hy, hx + 1, hy + 1, s(TURKEY::head));
    cv.set(hx + 1, hy, s(TURKEY::eye));
    cv.set(hx + 2, hy + 1, s(TURKEY::beak));
    cv.set(hx + 1, hy + 2, s(TURKEY::wattle));
    cv.set(hx + 2, hy + 2, s(TURKEY::wattle));
    cv.set(hx + 1, hy + 3, s(TURKEY::wattle));
  }

  // ---------- drifting leaves ----------
  void spawnLeaf(Leaf& l, bool anywhere) {
    l.x = anywhere ? rng.flt() * 64 : -2;
    l.y = 15 + rng.flt() * 30;
    l.vx = 4 + rng.flt() * 5;
    l.vy = 2 + rng.flt() * 3;
    l.ph = rng.flt() * real(6.283);
    l.col = LEAF_COLORS[rng.intn(LEAF_COUNT)];
  }

  void drawCorn(Canvas& cv) {
    for (int i = 0; i < 4; i++) {
      const Corn& c = CORN[i];
      int top = c.base - c.h;
      real sway = std::sin(t * real(1.3) + i * real(1.7)) * real(1.4) + std::sin(t * real(0.7) + i) * real(0.5);
      auto off = [&](int y) { return int(jsround(sway * std::pow(real(c.base - y) / c.h, real(1.5)))); };
      for (int y = top; y <= c.base; y++) {
        int x = c.x + off(y);
        cv.set(x, y, dim(COL::stalk, amb));
        int k = y - top;
        if (k > 3 && k % 4 == 0) {
          // leaves alternate sides and droop
          int dir = (k / 4) % 2 ? 1 : -1;
          cv.set(x + dir, y, dim(COL::leaf, amb));
          cv.set(x + dir * 2, y + 1, dim(COL::leafDark, amb));
        }
      }
      // tassel
      int tx = c.x + off(top);
      cv.set(tx, top - 1, dim(COL::tassel, amb));
      cv.set(tx - 1, top, dim(COL::tassel, amb));
      cv.set(tx + 1, top, dim(COL::tassel, amb));
      // an ear of corn
      int ey = top + int(std::floor(c.h * real(0.55)));
      int ex = c.x + off(ey) + (i % 2 ? 1 : -1);
      cv.set(ex, ey, dim(COL::corn, amb));
      cv.set(ex, ey + 1, dim(COL::corn, amb));
      cv.set(ex, ey + 2, dim(COL::leafDark, amb));
    }
  }
};

}  // namespace

std::unique_ptr<Theme> createThanksgiving(Rng& rng) { return std::unique_ptr<Theme>(new Thanksgiving(rng)); }

}  // namespace pc
