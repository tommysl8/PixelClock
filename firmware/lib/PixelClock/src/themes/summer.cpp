// Summer beach (summer season). Port of simulator/themes/summer.js; keep the two in sync.
#include <algorithm>
#include <cstdlib>

#include "../themes.h"

namespace pc {
namespace {

constexpr int HORIZON = 33;  // last sky row
constexpr int SHORE = 47;    // average shoreline row

const SkyKey SUMMER_SKY[] = {
    {0, 0x061028, 0x12204a},          {5, 0x061028, 0x12204a},  {real(6.5), 0x3b4a8c, 0xffb27a},
    {real(8.5), 0x3d9be9, 0xa8dcf7},  {real(16.5), 0x3d9be9, 0xa8dcf7}, {real(18.5), 0x5a3d7a, 0xff8a4a},
    {real(19.8), 0x141638, 0x3a2a55}, {21, 0x061028, 0x12204a}, {24, 0x061028, 0x12204a},
};

enum Key {
  SeaFar, SeaNear, Sand, SandDark, SandLight, SandWet, Foam, Trunk, TrunkDark, Frond, FrondDark,
  FrondLight, Coconut, UmbrellaA, UmbrellaB, Pole, TowelA, TowelB, TowelC, Shadow, Castle,
  CastleDark, Flag, Crab, CrabDark, Gull, GullGray, KEY_COUNT
};

const uint32_t BASE[KEY_COUNT] = {
    0x1f6fb5, 0x3fb0d8, 0xe9cf95, 0xd6b878, 0xf6e2b0, 0xc4a46a, 0xffffff, 0x8a5a2c, 0x664020,
    0x2e8b3a, 0x1f6b2a, 0x4aaa48, 0x5a3a1a, 0xe8414a, 0xf6f2ea, 0x5e4026, 0x2f6fd8, 0xf6f2ea,
    0xffd23f, 0xb89a62, 0xc49c5e, 0x9c7a44, 0xe8414a, 0xe0482e, 0xa8301c, 0xf2f2f2, 0x9aa0a8,
};
constexpr uint32_t NIGHT = 0x0b1430;

struct NightPal {
  uint32_t c[KEY_COUNT];
  NightPal() {
    for (int k = 0; k < KEY_COUNT; k++) c[k] = mix(BASE[k], NIGHT, real(0.62));
    c[SeaFar] = 0x0a1a3c;
    c[SeaNear] = 0x123060;
    c[Sand] = 0x3e4468;
    c[SandDark] = 0x33395a;
    c[SandLight] = 0x4a5076;
    c[SandWet] = 0x2c3252;
    c[Foam] = 0x9aa8d0;
    c[Shadow] = 0x2c3252;
  }
};

constexpr uint32_t SUN_HIGH = 0xfff3b0, SUN_LOW = 0xffc850, MOON = 0xeeeedd, CLOCK = 0xffffff,
                   CLOCK_SHADOW = 0x152040;
const uint32_t BALL[] = {0xe8414a, 0xffffff, 0x2f6fd8, 0xffd23f};

// Palm fronds: [angle in degrees above horizontal, direction, length].
const int FRONDS[][3] = {{72, 1, 7}, {32, -1, 11}, {30, 1, 11}, {4, -1, 12}, {6, 1, 12}, {-22, -1, 9}, {-24, 1, 9}};
constexpr int PALM_TX = 51, PALM_TY = 29;

// Summer days are long: full light from 7:30 to 18:30, fading to dark by 20:30.
real summerLight(real hf) {
  if (hf < real(5.5) || hf > real(20.5)) return 0;
  if (hf < real(7.5)) return (hf - real(5.5)) / 2;
  if (hf > real(18.5)) return clamp(1 - (hf - real(18.5)) / 2, 0, 1);
  return 1;
}

struct Light {
  real x, y;
  uint32_t color;
  real r;
  bool sun;
  real h;
};

class Summer : public Theme {
 public:
  explicit Summer(Rng& r) : rng(r) {
    gullTimer = 3 + rng.flt() * 6;
    crabTimer = 6 + rng.flt() * 10;
  }

  void update(real dt, const Ctx& ctx) override {
    t += dt;
    real day = summerLight(ctx.hourFloat);

    gullTimer -= dt;
    if (!gull.active && gullTimer <= 0) {
      if (day > real(0.3)) {
        int dir = rng.chance(real(0.5)) ? 1 : -1;
        gull.active = true;
        gull.x = dir > 0 ? -4 : 68;
        gull.y = 17 + rng.flt() * 9;
        gull.v = (10 + rng.flt() * 5) * dir;
      }
      gullTimer = 10 + rng.flt() * 12;
    }
    if (gull.active) {
      gull.x += gull.v * dt;
      if (gull.x < -6 || gull.x > 70) gull.active = false;
    }

    crabTimer -= dt;
    if (!crab.active && crabTimer <= 0) {
      int dir = rng.chance(real(0.5)) ? 1 : -1;
      crab.active = true;
      crab.x = dir > 0 ? -4 : 67;
      crab.dir = dir;
      crab.pause = 0;
      crabTimer = 14 + rng.flt() * 16;
    }
    if (crab.active) {
      if (crab.pause > 0) crab.pause -= dt;
      else {
        crab.x += crab.dir * 7 * dt;
        if (rng.chance(real(0.4) * dt)) crab.pause = real(0.6) + rng.flt() * real(0.8);
      }
      if (crab.x < -6 || crab.x > 69) crab.active = false;
    }

    if (ctx.shake) {
      Ball b;
      b.x = -3;
      b.y = 30;
      b.vx = 15 + rng.flt() * 5;
      b.vy = -6;
      b.spin = 0;
      balls.push_back(b);
    }
    for (int i = int(balls.size()) - 1; i >= 0; i--) {
      Ball& b = balls[i];
      b.vy += 70 * dt;
      b.x += b.vx * dt;
      b.y += b.vy * dt;
      b.spin += b.vx * dt * real(0.6);
      if (b.y > 57) {
        b.y = 57;
        b.vy = std::abs(b.vy) > 8 ? -std::abs(b.vy) * real(0.72) : 0;
        b.vx *= real(0.97);
      }
      if (b.x > 70) balls.erase(balls.begin() + i);
    }
  }

  void draw(Canvas& cv, const Ctx& ctx) override {
    real hf = ctx.hourFloat;
    real day = summerLight(hf);
    uint32_t P[KEY_COUNT];
    for (int i = 0; i < KEY_COUNT; i++) P[i] = mix(NP.c[i], BASE[i], day);
    real golden = clamp(1 - std::abs(hf - real(18.2)) / real(1.6), 0, 1) +
                  clamp(1 - std::abs(hf - real(6.6)) / real(1.2), 0, 1);
    if (golden > 0) {
      for (int k : {SeaFar, SeaNear, Sand, SandDark, SandLight, SandWet}) P[k] = mix(P[k], 0xff8a4a, golden * real(0.25));
    }

    Light L = lightSource(hf);
    uint32_t skyBottom = drawSky(cv, hf, L);
    drawGull(cv, P);
    drawSea(cv, P, day, L, skyBottom);
    drawSand(cv, P);
    drawCastle(cv, P);
    drawUmbrella(cv, P, day);
    drawPalm(cv, P);
    drawCrab(cv, P);
    for (const Ball& b : balls) drawBall(cv, b, P, day);

    drawClock(cv, ctx, CLOCK, CLOCK_SHADOW);
  }

 private:
  struct GullState {
    bool active = false;
    real x = 0, y = 0, v = 0;
  };
  struct CrabState {
    bool active = false;
    real x = 0;
    int dir = 1;
    real pause = 0;
  };
  struct Ball {
    real x, y, vx, vy, spin;
  };

  Rng& rng;
  const NightPal NP;
  real t = 0;
  GullState gull;
  real gullTimer = 0;
  CrabState crab;
  real crabTimer = 0;
  std::vector<Ball> balls;

  // Where the sun or moon is.
  Light lightSource(real hf) {
    if (hf >= 6 && hf <= 20) {
      real p = (hf - 6) / 14;
      real h = std::sin(kPi * p);
      return {60 - p * 56, HORIZON + 1 - h * 15, mix(SUN_LOW, SUN_HIGH, clamp(h * real(1.6), 0, 1)), real(2.8), true, h};
    }
    real q = jsmod(hf - 20 + 24, 24) / 10;
    real h = std::sin(kPi * q);
    return {60 - q * 56, HORIZON + 1 - h * 15, MOON, real(2.5), false, h};
  }

  uint32_t drawSky(Canvas& cv, real hf, const Light& L) {
    SkyColors sky = skyAt(hf, SUMMER_SKY);
    vgradient(cv, 0, 0, 63, HORIZON, sky.top, sky.bottom);
    drawStars(cv, t, 1 - daylight(hf), 0, 0, 63, HORIZON - 4, 30);
    real glow = L.sun ? real(0.25) : real(0.12);
    int yEnd = int(jsmin(HORIZON, std::ceil(L.y + 6)));
    for (int y = int(std::floor(L.y - 6)); y <= yEnd; y++) {
      for (int x = int(std::floor(L.x - 6)); x <= int(std::ceil(L.x + 6)); x++) {
        real d = std::hypot(x - L.x, y - L.y);
        if (d <= L.r * real(0.55) && L.sun) cv.set(x, y, mix(L.color, 0xffffff, real(0.6)));
        else if (d <= L.r) cv.set(x, y, L.color);
        else if (d < 6) cv.blend(x, y, L.color, glow * (1 - d / 6));
      }
    }
    return sky.bottom;
  }

  int shoreAt(int x) {
    return SHORE + int(jsround(real(1.5) * std::sin(t * real(0.7)) + real(0.8) * std::sin(x * real(0.15) + t * real(0.7))));
  }

  void drawSea(Canvas& cv, const uint32_t* P, real day, const Light& L, uint32_t skyBottom) {
    uint32_t far = mix(P[SeaFar], skyBottom, real(0.25));
    uint32_t nearC = P[SeaNear];
    real sparkle = real(0.2) + real(0.25) * day;
    for (int y = HORIZON + 1; y <= SHORE + 3; y++) {
      uint32_t c = mix(far, nearC, real(y - HORIZON - 1) / (SHORE - HORIZON));
      for (int x = 0; x < 64; x++) {
        uint32_t px = c;
        real s = std::sin(x * real(0.55) + t * real(1.8) + y * real(1.7)) * std::sin(x * real(0.13) - t * real(0.9) + y);
        if (s > real(0.72)) px = mix(c, 0xffffff, sparkle);
        else if (s < real(-0.8)) px = dim(c, real(0.85));
        cv.set(x, y, px);
      }
    }
    if (L.y <= HORIZON + 1 && L.h > 0) {
      real strength = L.sun ? 1 : real(0.85);
      long frame = long(std::floor(t * 5));
      for (int y = HORIZON + 1; y <= SHORE + 1; y++) {
        real w = 1 + (y - HORIZON) * real(0.3);
        for (int x = int(std::floor(L.x - w)); x <= int(std::ceil(L.x + w)); x++) {
          if (hash(x, real(y * 7 + frame), 2) != 0) continue;
          real a = strength * (1 - std::abs(x - L.x) / (w + 1)) * (1 - real(y - HORIZON) / 22);
          cv.blend(x, y, L.color, a);
        }
      }
    }
    real p = jsmod(t / 4, 1);
    int wy = int(jsround(HORIZON + 4 + p * (SHORE - HORIZON - 5)));
    for (int x = 0; x < 64; x++) {
      if (hash(x, wy, 3) == 0) continue;
      cv.blend(x, wy + jsround(std::sin(x * real(0.3) + t) * real(0.6)), P[Foam], real(0.35) * std::sin(p * kPi));
    }
  }

  void drawSand(Canvas& cv, const uint32_t* P) {
    for (int x = 0; x < 64; x++) {
      int s = shoreAt(x);
      for (int y = s; y < 64; y++) {
        uint32_t c = P[Sand];
        if (y <= s + 2) c = P[SandWet];
        else {
          uint32_t h = hash(x, y, 9);
          if (h == 0) c = P[SandDark];
          else if (h == 1) c = P[SandLight];
        }
        cv.set(x, y, c);
      }
      cv.set(x, s, P[Foam]);
      if (hash(x, std::floor(t * 2), 3) == 0) cv.blend(x, s - 1, P[Foam], real(0.7));
    }
  }

  void drawPalm(Canvas& cv, const uint32_t* P) {
    const int tx = PALM_TX, ty = PALM_TY;
    const int base = 62;
    const int len = base - ty;
    for (int i = 0; i <= len; i++) {
      int y = base - i;
      real f = real(i) / len;
      real x = jsround(58 - (f * f) * (58 - tx));
      bool ring = i % 3 == 0;
      cv.set(x, y, ring ? P[TrunkDark] : P[Trunk]);
      cv.set(x + 1, y, P[TrunkDark]);
    }
    for (int i = 0; i < 7; i++) {
      int deg = FRONDS[i][0], dir = FRONDS[i][1], L = FRONDS[i][2];
      real a = (deg * kPi) / 180 + std::sin(t * real(1.1) + i * real(0.9)) * real(0.06);
      for (int s = 1; s <= L; s++) {
        real x = tx + dir * s * std::cos(a);
        real y = ty - s * std::sin(a) + real(0.045) * s * s;
        cv.set(x, y, s > L - 3 ? P[FrondLight] : P[Frond]);
        if (s > 2 && s < L) {
          cv.set(x, y + 1, P[FrondDark]);
          if (s % 2 == 0) cv.set(x + dir * real(0.6), y + 2, P[FrondDark]);
        }
      }
    }
    cv.rect(tx - 1, ty + 1, tx, ty + 2, P[Coconut]);
    cv.set(tx + 1, ty + 1, P[Coconut]);
  }

  void drawUmbrella(Canvas& cv, const uint32_t* P, real day) {
    const int cx = 21, cy = 47;
    for (int y = 57; y <= 60; y++) {
      for (int x = 12; x <= 32; x++) {
        real ex = (x - 22) / real(10), ey = (y - real(58.5)) / 2;
        real e = ex * ex + ey * ey;
        if (e <= 1) cv.blend(x, y, P[Shadow], real(0.6) * day);
      }
    }
    for (int y = 58; y <= 62; y++) {
      int band = (y - 58) % 2;
      cv.rect(24, y, 35, y, band == 0 ? P[TowelA] : P[TowelB]);
    }
    cv.rect(24, 60, 35, 60, P[TowelC]);
    for (int x = 24; x <= 35; x += 2) cv.set(x, 63, P[TowelB]);
    cv.rect(cx, cy, cx, 59, P[Pole]);
    for (int r = 0; r <= 5; r++) {
      int y = cy - 5 + r;
      int hw = int(jsround(std::sqrt(jsmax(0, real(36 - (5 - r) * (5 - r)))) * real(1.5)));
      for (int x = cx - hw; x <= cx + hw; x++) {
        int seg = ((x - cx + 12) / 3) % 2;
        uint32_t c = seg ? P[UmbrellaA] : P[UmbrellaB];
        if (r == 5 && (x - cx) % 3 == 0) continue;  // scalloped edge
        if (r == 0) c = P[UmbrellaA];
        cv.set(x, y, c);
      }
    }
    cv.set(cx, cy - 6, P[Pole]);
  }

  void drawCastle(Canvas& cv, const uint32_t* P) {
    const int x0 = 3, g = 60;
    cv.rect(x0, g - 3, x0 + 8, g, P[Castle]);
    cv.rect(x0, g - 6, x0 + 2, g - 4, P[Castle]);
    cv.rect(x0 + 6, g - 6, x0 + 8, g - 4, P[Castle]);
    cv.rect(x0 + 3, g - 5, x0 + 5, g - 4, P[CastleDark]);
    cv.set(x0, g - 7, P[Castle]);
    cv.set(x0 + 2, g - 7, P[Castle]);
    cv.set(x0 + 6, g - 7, P[Castle]);
    cv.set(x0 + 8, g - 7, P[Castle]);
    cv.rect(x0 + 4, g - 2, x0 + 4, g, P[CastleDark]);
    cv.rect(x0 + 7, g - 10, x0 + 7, g - 7, P[CastleDark]);
    int wave = int(long(std::floor(t * 4)) % 2);
    cv.rect(x0 + 8, g - 10, x0 + 9 + wave, g - 10, P[Flag]);
    cv.set(x0 + 8, g - 9, P[Flag]);
  }

  void drawGull(Canvas& cv, const uint32_t* P) {
    if (!gull.active) return;
    real x = jsround(gull.x);
    real y = jsround(gull.y + std::sin(t * real(1.5)) * real(1.5));
    bool up = long(std::floor(t * 5)) % 2 == 0;
    cv.set(x, y, P[Gull]);
    cv.set(x - 1, y, P[Gull]);
    cv.set(x + 1, y, P[Gull]);
    real wy = y + (up ? -1 : 1);
    cv.set(x - 2, wy, P[Gull]);
    cv.set(x + 2, wy, P[Gull]);
    cv.set(x - 3, wy + (up ? -1 : 0), P[GullGray]);
    cv.set(x + 3, wy + (up ? -1 : 0), P[GullGray]);
  }

  void drawCrab(Canvas& cv, const uint32_t* P) {
    if (!crab.active) return;
    real x = jsround(crab.x);
    const int y = 61;
    int step = crab.pause > 0 ? 0 : int(long(std::floor(t * 10)) % 2);
    cv.rect(x - 1, y - 1, x + 1, y, P[Crab]);
    cv.set(x - 1, y - 2, P[CrabDark]);
    cv.set(x + 1, y - 2, P[CrabDark]);
    cv.set(x - 2, y - 1 - step, P[Crab]);
    cv.set(x + 2, y - 1 - (1 - step), P[Crab]);
    cv.set(x - 2, y + 1, P[CrabDark]);
    cv.set(x + 2, y + 1, P[CrabDark]);
    cv.set(x - 1 + step, y + 1, P[CrabDark]);
  }

  void drawBall(Canvas& cv, const Ball& b, const uint32_t* P, real day) {
    real sh = clamp(1 - (57 - b.y) / 30, real(0.2), 1);
    for (int dx = -3; dx <= 3; dx++)
      if (std::abs(dx) <= 3 * sh) cv.blend(b.x + dx, 61, P[Shadow], real(0.5) * sh * (real(0.4) + real(0.6) * day));
    for (int dy = -3; dy <= 3; dy++) {
      for (int dx = -3; dx <= 3; dx++) {
        if (dx * dx + dy * dy > 9) continue;
        int seg = int(long(std::floor(jsmod((std::atan2(real(dy), real(dx)) + b.spin) / (kPi / 2), 4) + 4)) % 4);
        uint32_t c = dx == 0 && dy == 0 ? 0xffffff : BALL[seg];
        cv.set(b.x + dx, b.y + dy, mix(mix(c, NIGHT, real(0.5)), c, real(0.5) + real(0.5) * day));
      }
    }
  }
};

}  // namespace

std::unique_ptr<Theme> createSummer(Rng& rng) { return std::unique_ptr<Theme>(new Summer(rng)); }

}  // namespace pc
