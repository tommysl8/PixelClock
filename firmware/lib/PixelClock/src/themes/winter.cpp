// Snowy hills (winter season). Port of simulator/themes/winter.js; keep the two in sync.
#include <algorithm>
#include <cstdlib>

#include "../themes.h"

namespace pc {
namespace {

// Palette entries (BASE keys in winter.js, same order).
enum Key {
  SnowFar, SnowFarShade, SnowMid, SnowMidShade, SnowMidRim, SnowNear, SnowNearShade, SnowNearRim,
  Pine, PineDark, PineSnow, PineSnowShade, Trunk, Cabin, CabinDark, Roof, RoofEdge, Chimney, Door,
  Snowman, SnowmanShade, SnowmanLine, Coal, Carrot, Scarf, ScarfDark, Stick, Hat, HatBand, KEY_COUNT
};

const uint32_t BASE[KEY_COUNT] = {
    0x8ba8d4, 0x7895c4, 0xc4d6ee, 0xa3bbdc, 0xe2ecf8, 0xf0f5fd, 0xcbd9ed, 0xffffff,  // snow
    0x22603a, 0x153f26, 0xf0f5fd, 0xc6d5ea, 0x4a2e1a, 0x7a4e2c, 0x55351e, 0xf2f6fc,
    0x6f7f9c, 0x6a3226, 0x3a2214, 0xffffff, 0xc9d6ea, 0x7d93b6, 0x1a1a22, 0xff8a1e,
    0xd63a3a, 0x9e2626, 0x5a3418, 0x1c1c24, 0xc03434,
};

constexpr uint32_t NIGHT = 0x0b1430;
constexpr real NIGHT_AMOUNT = real(0.6);

struct NightPal {
  uint32_t c[KEY_COUNT];
  NightPal() {
    for (int k = 0; k < KEY_COUNT; k++) c[k] = mix(BASE[k], NIGHT, NIGHT_AMOUNT);
    // moonlit snow (NIGHT_BASE in winter.js)
    c[SnowFar] = 0x26386a;
    c[SnowFarShade] = 0x1f2f5c;
    c[SnowMid] = 0x374b80;
    c[SnowMidShade] = 0x2b3d6e;
    c[SnowMidRim] = 0x5468a0;
    c[SnowNear] = 0x55699e;
    c[SnowNearShade] = 0x435689;
    c[SnowNearRim] = 0x7a8cc0;
    c[Pine] = 0x255c4e;
    c[PineDark] = 0x173e3c;
    c[PineSnow] = 0x6a7eb4;
    c[PineSnowShade] = 0x4c5f94;
    c[Roof] = 0x6a7eb4;
    c[RoofEdge] = 0x1c2648;
    c[Snowman] = 0x9aacd8;
    c[SnowmanShade] = 0x7184b8;
    c[SnowmanLine] = 0x2c3a68;
  }
};

const SkyKey WINTER_SKY[] = {
    {0, 0x050a1e, 0x142250},         {real(5.5), 0x050a1e, 0x142250}, {7, 0x34407e, 0xeea0a0},
    {9, 0x4c8ad2, 0xa6cbee},         {real(15.5), 0x4c8ad2, 0xa6cbee}, {17, 0x3e3e7a, 0xe58a7a},
    {real(18.5), 0x0c1238, 0x26305e}, {24, 0x050a1e, 0x142250},
};

constexpr uint32_t WINDOW_LIT = 0xffc35a, WINDOW_HOT = 0xffe08a, MOON = 0xf4f1d0, SUN = 0xfff4d0,
                   SMOKE = 0xb8c0cc, AURORA_A = 0x3cffb0, AURORA_B = 0x40b8ff, CLOCK = 0xffffff,
                   CLOCK_SHADOW = 0x121a33;

// Hill outlines (top row of each hill per column).
struct Terrain {
  int farY[64], midY[64], nearY[64];
  Terrain() {
    for (int x = 0; x < 64; x++) {
      farY[x] = int(jsround(37 + real(2.5) * std::sin(x * real(0.08) + real(0.5)) +
                           real(1.5) * std::sin(x * real(0.21) + 2)));
      midY[x] = int(jsround(45 + 3 * std::sin(x * real(0.06) + real(2.2)) + real(1.2) * std::sin(x * real(0.17))));
      nearY[x] = int(jsround(54 + real(1.5) * std::sin(x * real(0.05) + 1)));
    }
  }
};

constexpr int CABIN_X = 42;
const int PINES[][2] = {{27, 9}, {5, 19}, {19, 15}, {12, 12}, {62, 11}};  // [x, height], back to front
constexpr int SNOWMAN_X = 51;

void disc(Canvas& cv, real cx, real cy, real r, uint32_t c) {
  real rr = r * r + r * real(0.6);
  for (int y = int(std::floor(cy - r)); y <= int(std::ceil(cy + r)); y++)
    for (int x = int(std::floor(cx - r)); x <= int(std::ceil(cx + r)); x++)
      if ((x - cx) * (x - cx) + (y - cy) * (y - cy) <= rr) cv.set(x, y, c);
}

void line(Canvas& cv, int x0, int y0, int x1, int y1, uint32_t c) {
  int n = std::max(std::abs(x1 - x0), std::abs(y1 - y0));
  for (int i = 0; i <= n; i++) cv.set(x0 + real((x1 - x0) * i) / n, y0 + real((y1 - y0) * i) / n, c);
}

class Winter : public Theme {
 public:
  explicit Winter(Rng& r) : rng(r) {
    farFl.reserve(28);
    nearFl.reserve(16);
    for (int i = 0; i < 28; i++) farFl.push_back(newFlake(0, true));
    for (int i = 0; i < 16; i++) nearFl.push_back(newFlake(1, true));
  }

  void update(real dt, const Ctx& ctx) override {
    t += dt;
    real calm = real(2.5) * std::sin(t * real(0.13)) + real(1.2) * std::sin(t * real(0.37));
    real target = blizzard > 0 ? 26 : calm;
    wind += (target - wind) * jsmin(1, dt * real(1.5));
    if (blizzard > 0) blizzard -= dt;

    moveFlakes(farFl, 0, dt);
    moveFlakes(nearFl, 1, dt);

    smokeTimer -= dt;
    if (smokeTimer <= 0) {
      smokeTimer = real(0.45) + rng.flt() * real(0.3);
      Smoke s;
      s.x = CABIN_X + 2;
      s.y = T.farY[CABIN_X] - 11;
      s.age = 0;
      s.life = 3 + rng.flt() * real(1.5);
      s.ph = rng.flt() * real(6.283);
      smoke.push_back(s);
    }
    for (int i = int(smoke.size()) - 1; i >= 0; i--) {
      Smoke& s = smoke[i];
      s.age += dt;
      s.y -= real(2.2) * dt;
      s.x += (wind * real(0.35) - real(0.6) + std::sin(t * real(1.7) + s.ph) * real(0.4)) * dt;
      if (s.age > s.life) smoke.erase(smoke.begin() + i);
    }

    if (ctx.shake) {
      blizzard = 4;
      for (int i = 0; i < 150; i++) {
        Swirl p;
        p.x = rng.flt() * 64;
        p.y = rng.flt() * 64;
        p.vx = 15 + rng.flt() * 35;
        p.vy = (rng.flt() - real(0.5)) * 30;
        p.age = 0;
        p.life = 3 + rng.flt() * 3;
        p.ph = rng.flt() * real(6.283);
        swirl.push_back(p);
      }
    }
    for (int i = int(swirl.size()) - 1; i >= 0; i--) {
      Swirl& p = swirl[i];
      p.age += dt;
      p.vx *= real(0.985);
      p.vy = p.vy * real(0.97) + std::sin(t * 3 + p.ph) * 8 * dt + 4 * dt;
      p.x += p.vx * dt;
      p.y += p.vy * dt;
      if (p.x > 64) p.x -= 64;
      if (p.age > p.life) swirl.erase(swirl.begin() + i);
    }
  }

  void draw(Canvas& cv, const Ctx& ctx) override {
    real hf = ctx.hourFloat;
    real day = daylight(hf);
    real k = NIGHT_AMOUNT * (1 - day);
    uint32_t P[KEY_COUNT];
    for (int i = 0; i < KEY_COUNT; i++) P[i] = mix(NP.c[i], BASE[i], day);
    uint32_t flakeNear = mix(0xb4c2e0, 0xffffff, day);
    uint32_t flakeFar = mix(0x8696bc, 0xe8f0ff, day);

    drawSky(cv, hf, day);
    drawHills(cv, P);
    drawCabin(cv, P, day);
    for (const Smoke& s : smoke) {
      real a = real(0.55) * (1 - s.age / s.life);
      cv.blend(s.x, s.y, mix(SMOKE, NIGHT, k * real(0.6)), a);
      if (s.age > 1) cv.blend(s.x + 1, s.y, mix(SMOKE, NIGHT, k * real(0.6)), a * real(0.5));
    }
    for (const Flake& f : farFl)
      if (f.y < T.midY[int(clamp(jsround(f.x), 0, 63))]) cv.blend(f.x, f.y, flakeFar, real(0.55));
    drawMid(cv, P);
    for (const auto& p : PINES) drawPine(cv, P, p[0], T.midY[p[0]] + 2, p[1]);
    drawNear(cv, P, day);
    drawSnowman(cv, P);
    for (const Flake& f : nearFl) cv.set(f.x, f.y, flakeNear);
    for (const Swirl& p : swirl) cv.blend(p.x, p.y, flakeNear, real(0.9) * jsmin(1, (p.life - p.age) / real(1.2)));

    drawClock(cv, ctx, CLOCK, CLOCK_SHADOW);
  }

 private:
  struct Flake {
    real x, y, v, ph;
  };
  struct Smoke {
    real x, y, age, life, ph;
  };
  struct Swirl {
    real x, y, vx, vy, age, life, ph;
  };

  Rng& rng;
  const Terrain T;
  const NightPal NP;
  real t = 0, wind = 0, blizzard = 0;
  std::vector<Flake> farFl, nearFl;
  std::vector<Smoke> smoke;
  real smokeTimer = 0;
  std::vector<Swirl> swirl;

  Flake newFlake(int layer, bool anywhere) {
    Flake f;
    f.x = rng.flt() * 64;
    f.y = anywhere ? rng.flt() * 64 : -1 - rng.flt() * 4;
    f.v = layer == 0 ? 3 + rng.flt() * real(2.5) : 7 + rng.flt() * 5;
    f.ph = rng.flt() * real(6.283);
    return f;
  }

  void moveFlakes(std::vector<Flake>& list, int layer, real dt) {
    real drift = layer == 0 ? real(0.5) : 1;
    for (size_t i = 0; i < list.size(); i++) {
      Flake& f = list[i];
      f.y += f.v * dt * (blizzard > 0 ? real(1.6) : 1);
      f.x += (wind * drift + std::sin(t * real(1.3) + f.ph) * (layer == 0 ? real(0.6) : real(1.4))) * dt;
      if (f.x < -2) f.x += 66;
      if (f.x > 65) f.x -= 66;
      if (f.y > 64) list[i] = newFlake(layer, false);
    }
  }

  void drawSky(Canvas& cv, real hf, real day) {
    SkyColors sky = skyAt(hf, WINTER_SKY);
    vgradient(cv, 0, 0, 63, 44, sky.top, sky.bottom);
    real night = 1 - day;
    drawStars(cv, t, night, 0, 0, 63, 34, 30);

    if (night > real(0.2)) {
      real a = (night - real(0.2)) / real(0.8);
      for (int x = 0; x < 64; x++) {
        real cy = 21 + real(3.5) * std::sin(x * real(0.11) + t * real(0.45)) +
                  real(1.8) * std::sin(x * real(0.047) - t * real(0.27));
        real pulse = real(0.55) + real(0.45) * std::sin(x * real(0.27) + t * real(1.1));
        for (int dy = -3; dy <= 3; dy++) {
          real y = jsround(cy + dy);
          if (y < 15) continue;
          real k = 1 - std::abs(dy) / real(4);
          cv.blend(x, y, mix(AURORA_A, AURORA_B, real(dy + 3) / 6), a * real(0.32) * k * pulse);
        }
      }
      real m = jsmin(1, a * real(1.5));
      disc(cv, 56, 9, real(2.4), mix(sky.top, MOON, m));
      cv.set(55, 8, mix(sky.top, 0xffffff, m));
    }
    if (day > 0) disc(cv, 9, 21, real(2.2), mix(sky.bottom, SUN, day));
  }

  void drawPine(Canvas& cv, const uint32_t* P, int cx, int base, int h) {
    const int trunkH = 2;
    cv.rect(cx, base - trunkH, cx + (h > 13 ? 1 : 0), base, P[Trunk]);
    int body = h - trunkH;
    int top = base - trunkH - body;
    int tier = int(std::ceil(body / real(3)));
    for (int r = 0; r < body; r++) {
      int y = top + r;
      int i = r / tier;
      int j = r % tier;
      int hw = int(std::floor((real(j + 1) + i * real(1.4)) * real(0.72)));
      for (int x = cx - hw; x <= cx + hw; x++) {
        uint32_t c = x > cx ? P[PineDark] : P[Pine];
        if (j == 0 && std::abs(x - cx) < hw) c = x > cx ? P[PineSnowShade] : P[PineSnow];
        else if (j == tier - 1 && std::abs(x - cx) >= hw - 1 && x < cx) c = P[PineSnow];
        cv.set(x, y, c);
      }
    }
    cv.set(cx, top - 1, P[PineSnow]);
  }

  void drawCabin(Canvas& cv, const uint32_t* P, real day) {
    const int g = T.farY[CABIN_X];
    const int cx = CABIN_X;
    cv.rect(cx - 3, g - 5, cx + 3, g, P[Cabin]);
    cv.rect(cx + 2, g - 5, cx + 3, g, P[CabinDark]);
    cv.rect(cx + 2, g - 11, cx + 3, g - 8, P[Chimney]);
    cv.rect(cx + 2, g - 12, cx + 3, g - 12, P[Roof]);
    for (int r = 0; r < 4; r++) {
      int y = g - 9 + r;
      cv.rect(cx - 1 - r, y, cx + 1 + r, y, P[Roof]);
      cv.set(cx - 2 - r, y, P[RoofEdge]);
      cv.set(cx + 2 + r, y, P[RoofEdge]);
    }
    cv.rect(cx - 1, g - 10, cx + 1, g - 10, P[RoofEdge]);
    cv.rect(cx - 5, g - 5, cx + 5, g - 5, P[RoofEdge]);
    cv.rect(cx + 1, g - 3, cx + 1, g, P[Door]);
    real flicker = real(0.5) + real(0.5) * std::sin(t * 7) * std::sin(t * real(3.1));
    uint32_t win = mix(WINDOW_LIT, WINDOW_HOT, flicker);
    real night = 1 - day;
    if (night > real(0.05)) {
      for (int y = g - 6; y <= g + 1; y++) {
        for (int x = cx - 5; x <= cx + 1; x++) {
          real d = std::hypot(x - (cx - real(1.5)), (y - (g - real(2.5))) * real(1.2));
          if (d < 4) cv.blend(x, y, WINDOW_LIT, night * real(0.22) * (1 - d / 4));
        }
      }
    }
    cv.rect(cx - 2, g - 3, cx - 1, g - 2,
            mix(mix(0x3a3020, WINDOW_LIT, real(0.6)), win, real(0.35) + real(0.65) * night));
  }

  void drawSnowman(Canvas& cv, const uint32_t* P) {
    const int cx = SNOWMAN_X;
    const int ground = T.nearY[cx];
    struct Ball {
      int cy;
      real r;
    };
    const Ball balls[3] = {{ground - 4, real(4.6)}, {ground - 11, real(3.6)}, {ground - 17, real(2.8)}};
    line(cv, cx - 3, ground - 11, cx - 8, ground - 15, P[Stick]);
    cv.set(cx - 9, ground - 16, P[Stick]);
    cv.set(cx - 8, ground - 17, P[Stick]);
    line(cv, cx + 3, ground - 11, cx + 8, ground - 14, P[Stick]);
    cv.set(cx + 9, ground - 15, P[Stick]);
    cv.set(cx + 9, ground - 13, P[Stick]);
    for (const Ball& b : balls) disc(cv, cx, b.cy, b.r + 1, P[SnowmanLine]);
    for (const Ball& b : balls) {
      real r = b.r;
      real rr = r * r + r * real(0.6);
      for (int y = int(std::floor(b.cy - r)); y <= int(std::ceil(b.cy + r)); y++) {
        for (int x = int(std::floor(cx - r)); x <= int(std::ceil(cx + r)); x++) {
          int dx = x - cx;
          int dy = y - b.cy;
          if (dx * dx + dy * dy > rr) continue;
          bool shaded = dx + dy * real(0.6) > r * real(0.45);
          cv.set(x, y, shaded ? P[SnowmanShade] : P[Snowman]);
        }
      }
    }
    const int hy = ground - 17;
    cv.set(cx - 1, hy - 1, P[Coal]);
    cv.set(cx + 1, hy - 1, P[Coal]);
    cv.rect(cx + 1, hy, cx + 3, hy, P[Carrot]);
    cv.set(cx - 1, hy + 2, P[Coal]);
    cv.set(cx, hy + 2, P[Coal]);
    cv.set(cx + 1, hy + 2, P[Coal]);
    cv.rect(cx - 3, hy - 3, cx + 3, hy - 3, P[Hat]);
    cv.rect(cx - 2, hy - 7, cx + 2, hy - 4, P[Hat]);
    cv.rect(cx - 2, hy - 4, cx + 2, hy - 4, P[HatBand]);
    cv.rect(cx - 3, hy + 3, cx + 3, hy + 4, P[Scarf]);
    cv.rect(cx - 3, hy + 4, cx + 3, hy + 4, P[ScarfDark]);
    cv.rect(cx - 3, hy + 5, cx - 2, hy + 8, P[Scarf]);
    cv.set(cx - 2, hy + 8, P[ScarfDark]);
    cv.set(cx, ground - 12, P[Coal]);
    cv.set(cx, ground - 9, P[Coal]);
  }

  void drawHills(Canvas& cv, const uint32_t* P) {
    for (int x = 0; x < 64; x++)
      for (int y = T.farY[x]; y < T.midY[x]; y++) {
        bool shade = y - T.farY[x] > 3 || hash(x, y, 7) == 0;
        cv.set(x, y, shade ? P[SnowFarShade] : P[SnowFar]);
      }
  }

  void drawMid(Canvas& cv, const uint32_t* P) {
    for (int x = 0; x < 64; x++) {
      int slope = T.midY[std::min(63, x + 1)] - T.midY[std::max(0, x - 1)];
      for (int y = T.midY[x]; y < T.nearY[x]; y++) {
        uint32_t c = P[SnowMid];
        if (y == T.midY[x]) c = P[SnowMidRim];
        else if ((slope > 0 && y - T.midY[x] < 4) || y >= T.nearY[x] - 2 || hash(x, y, 9) == 0) c = P[SnowMidShade];
        cv.set(x, y, c);
      }
    }
  }

  void drawNear(Canvas& cv, const uint32_t* P, real day) {
    for (int x = 0; x < 64; x++) {
      for (int y = T.nearY[x]; y < 64; y++) {
        uint32_t c = P[SnowNear];
        if (y == T.nearY[x]) c = P[SnowNearRim];
        else if (y == T.nearY[x] + 1 || hash(x, y, 11) == 0) c = P[SnowNearShade];
        cv.set(x, y, c);
        if (hash(x, y, 61) == 0) {
          real s = std::sin(t * real(2.5) + x * real(1.7) + y);
          if (s > real(0.6)) cv.blend(x, y, 0xffffff, (s - real(0.6)) * real(2.2) * (real(0.4) + real(0.6) * day));
        }
      }
    }
  }
};

}  // namespace

std::unique_ptr<Theme> createWinter(Rng& rng) { return std::unique_ptr<Theme>(new Winter(rng)); }

}  // namespace pc
