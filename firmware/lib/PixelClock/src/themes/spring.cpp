// Spring meadow (spring season). Port of simulator/themes/spring.js; keep the two in sync.
#include <algorithm>
#include <cstdlib>

#include "../themes.h"

namespace pc {
namespace {

enum Key {
  HillFar, HillFarShade, Grass, GrassDark, GrassLight, Blade, BladeDark, Trunk, TrunkDark,
  Bloom, BloomDark, BloomDeep, BloomLight, Stem, Cloud, CloudShade, Daisy, KEY_COUNT
};

const uint32_t BASE[KEY_COUNT] = {
    0xa3d29c, 0x8fc28c, 0x52a540, 0x3f8a32, 0x6cbd4f, 0x76c95a, 0x3a7f2c, 0x5a3a26, 0x3f2818,
    0xf39ac0, 0xcf6597, 0xa94c7c, 0xffd3e5, 0x2f7a2a, 0xffffff, 0xdbe5f2, 0xf6f6f0,
};
constexpr uint32_t NIGHT = 0x0b1430;
constexpr real NIGHT_AMOUNT = real(0.62);

struct NightPal {
  uint32_t c[KEY_COUNT];
  NightPal() {
    for (int k = 0; k < KEY_COUNT; k++) c[k] = mix(BASE[k], NIGHT, NIGHT_AMOUNT);
    c[Cloud] = 0x2c3760;
    c[CloudShade] = 0x222c50;
  }
};

const uint32_t PETALS[] = {0xff8fbf, 0xf26aa6, 0xffb3d4};
const uint32_t TULIPS[] = {0xe8414a, 0xffd23f, 0xa35ad8, 0xff7fb0, 0xff8a3a, 0xf4f4f4, 0xe8414a};
constexpr uint32_t MOON = 0xf4f1d0, FIREFLY = 0xd8ff6a, CLOCK = 0xffffff, CLOCK_SHADOW = 0x182340;

struct Terrain {
  int farY[64], ground[64];
  Terrain() {
    for (int x = 0; x < 64; x++) {
      farY[x] = int(jsround(41 + 2 * std::sin(x * real(0.07) + 2) + real(1.3) * std::sin(x * real(0.19))));
      ground[x] = int(jsround(48 + real(1.2) * std::sin(x * real(0.09) + real(0.4))));
    }
  }
};

struct Blob {
  real cx, cy, r;
};
const Blob CANOPY[] = {{6, 31, real(5.5)}, {20, 31, 6}, {12, 29, 8}, {15, 23, 6}, {8, 24, real(4.5)}};
constexpr int CANOPY_N = 5;
constexpr int CANOPY_TOP = 17, CANOPY_BOTTOM = 38;
bool inCanopy(int x, int y) {
  for (const Blob& b : CANOPY) {
    real dx = x - b.cx, dy = y - b.cy;
    if (dx * dx + dy * dy <= b.r * b.r + b.r * real(0.5)) return true;
  }
  return false;
}

const char* const CLOUD[] = {
    ".....XXXX......", "...XXXXXXXX.XX.", ".XXXXXXXXXXXXXX", "XXXXXXXXXXXXXXX", ".SSSSSSSSSSSSS.",
};
constexpr int CLOUD_ROWS = 5;

struct Flower {
  int x, base, h;
  uint32_t c;
};
const Flower FLOWERS[] = {
    {30, 52, 4, TULIPS[1]}, {42, 52, 4, TULIPS[3]}, {53, 51, 4, TULIPS[2]}, {24, 56, 5, TULIPS[0]},
    {36, 57, 5, TULIPS[2]}, {47, 56, 5, TULIPS[4]}, {59, 57, 5, TULIPS[5]}, {20, 61, 6, TULIPS[3]},
    {30, 62, 6, TULIPS[4]}, {41, 62, 6, TULIPS[0]}, {53, 62, 6, TULIPS[1]},
};
const char* const TULIP[] = {"A.A", "AAB", "ABB"};

struct Butterfly {
  real cx, cy, ax, ay, ph;
  uint32_t wing, tip;
};
const Butterfly BUTTERFLIES[] = {
    {36, 34, 14, 5, real(0.3), 0xff9a2a, 0x3a2010},
    {46, 40, 12, 4, real(2.6), 0x6fb6ff, 0x1c3a7a},
};

class Spring : public Theme {
 public:
  explicit Spring(Rng& r) : rng(r) {
    clouds[0] = {4, 17, real(1.1)};
    clouds[1] = {34, 21, real(0.8)};
    clouds[2] = {58, 15, real(1.4)};
    petals.reserve(160);
    birdTimer = 4 + rng.flt() * 6;
    for (int i = 0; i < 6; i++) {
      Firefly f;
      f.cx = 20 + rng.flt() * 40;
      f.cy = 38 + rng.flt() * 14;
      f.ph = rng.flt() * real(6.283);
      f.sp = real(0.6) + rng.flt() * real(0.5);
      fireflies[i] = f;
    }
  }

  void update(real dt, const Ctx& ctx) override {
    t += dt;
    breeze = real(2.2) + real(1.6) * std::sin(t * real(0.21)) + real(0.8) * std::sin(t * real(0.63));
    real day = daylight(ctx.hourFloat);

    for (CloudPos& c : clouds) {
      c.x += c.v * dt * (real(0.6) + breeze * real(0.15));
      if (c.x > 66) c.x = -16;
    }

    if (petals.size() < 40 && rng.chance(4 * dt)) {
      real x, y;
      canopyPoint(x, y);
      spawnPetal(x, y, 0, 2);
    }
    for (int i = int(petals.size()) - 1; i >= 0; i--) {
      Petal& p = petals[i];
      p.age += dt;
      if (p.rest > 0) {
        p.rest += dt;
        if (p.rest > 3) petals.erase(petals.begin() + i);
        continue;
      }
      real tx = breeze + std::sin(t * real(2.1) + p.ph) * real(2.5);
      p.vx += (tx - p.vx) * jsmin(1, dt * 2);
      p.vy += (real(3.5) + std::sin(t * 3 + p.ph) * real(1.5) - p.vy) * jsmin(1, dt * real(1.5));
      p.x += p.vx * dt;
      p.y += p.vy * dt;
      int gx = int(clamp(jsround(p.x), 0, 63));
      if (p.y >= T.ground[gx] + 1 + jsmod(p.ph, 3)) p.rest = real(0.001);
      if (p.x > 66 || p.age > p.life) petals.erase(petals.begin() + i);
    }

    if (ctx.shake) {
      for (int i = 0; i < 110; i++) {
        real x, y;
        if (rng.chance(real(0.5))) canopyPoint(x, y);
        else {
          x = rng.flt() * 64;
          y = 16 + rng.flt() * 30;
        }
        real a = rng.flt() * real(6.283);
        real s = 12 + rng.flt() * 22;
        spawnPetal(x, y, std::cos(a) * s + 8, std::sin(a) * s - 6);
      }
    }

    birdTimer -= dt;
    if (!bird.active && birdTimer <= 0) {
      if (day > real(0.3)) {
        bird.active = true;
        bird.x = -4;
        bird.y = 17 + rng.flt() * 8;
        bird.v = 13 + rng.flt() * 5;
      }
      birdTimer = 12 + rng.flt() * 14;
    }
    if (bird.active) {
      bird.x += bird.v * dt;
      if (bird.x > 68) bird.active = false;
    }
  }

  void draw(Canvas& cv, const Ctx& ctx) override {
    real hf = ctx.hourFloat;
    real day = daylight(hf);
    uint32_t P[KEY_COUNT];
    for (int i = 0; i < KEY_COUNT; i++) P[i] = mix(NP.c[i], BASE[i], day);
    real petalBright = real(0.5) + real(0.5) * day;

    drawSky(cv, hf, day, P);
    drawBird(cv);
    drawLand(cv, P);
    drawTree(cv, P);
    drawFlowers(cv, P, day);
    for (const Petal& p : petals) {
      real fade = p.rest > 0 ? 1 - p.rest / 3 : jsmin(1, (p.life - p.age) / real(1.5));
      cv.blend(p.x, p.y, mix(mix(p.c, NIGHT, real(0.55)), p.c, petalBright), fade);
    }
    drawButterflies(cv, day);
    drawFireflies(cv, day);

    drawClock(cv, ctx, CLOCK, CLOCK_SHADOW);
  }

 private:
  struct CloudPos {
    real x, y, v;
  };
  struct Petal {
    real x, y, vx, vy, ph;
    uint32_t c;
    real rest, life, age;
  };
  struct Bird {
    bool active = false;
    real x = 0, y = 0, v = 0;
  };
  struct Firefly {
    real cx, cy, ph, sp;
  };

  Rng& rng;
  const Terrain T;
  const NightPal NP;
  real t = 0, breeze = 0;
  CloudPos clouds[3];
  std::vector<Petal> petals;
  Bird bird;
  real birdTimer = 0;
  Firefly fireflies[6];
  Palette tulipPal{{'A', 0}, {'B', 0}};

  void spawnPetal(real x, real y, real vx, real vy) {
    Petal p;
    p.x = x;
    p.y = y;
    p.vx = vx;
    p.vy = vy;
    p.ph = rng.flt() * real(6.283);
    p.c = PETALS[rng.intn(3)];
    p.rest = 0;
    p.life = 10 + rng.flt() * 6;
    p.age = 0;
    petals.push_back(p);
  }

  void canopyPoint(real& x, real& y) {
    const Blob& b = CANOPY[rng.intn(CANOPY_N)];
    real a = rng.flt() * real(6.283);
    real d = rng.flt() * b.r;
    x = b.cx + std::cos(a) * d;
    y = b.cy + std::sin(a) * d;
  }

  void drawSky(Canvas& cv, real hf, real day, const uint32_t* P) {
    SkyColors sky = skyAt(hf);
    vgradient(cv, 0, 0, 63, 46, sky.top, sky.bottom);
    real night = 1 - day;
    drawStars(cv, t, night, 0, 0, 63, 34, 26);
    if (night > real(0.25)) {
      real m = jsmin(1, (night - real(0.25)) * 2);
      for (int y = 6; y <= 12; y++) {
        for (int x = 53; x <= 59; x++) {
          int d = (x - 56) * (x - 56) + (y - 9) * (y - 9);
          if (d <= 7) cv.set(x, y, mix(sky.top, MOON, m));
          else if (d <= 12) cv.blend(x, y, MOON, m * real(0.15));
        }
      }
    }
    real ca = real(0.35) + real(0.6) * day;
    for (const CloudPos& c : clouds) {
      for (int r = 0; r < CLOUD_ROWS; r++) {
        for (int i = 0; CLOUD[r][i]; i++) {
          char k = CLOUD[r][i];
          if (k == '.') continue;
          cv.blend(jsround(c.x) + i, c.y + r, k == 'S' ? P[CloudShade] : P[Cloud], ca);
        }
      }
    }
  }

  void drawLand(Canvas& cv, const uint32_t* P) {
    for (int x = 0; x < 64; x++) {
      for (int y = T.farY[x]; y < T.ground[x]; y++)
        cv.set(x, y, y == T.farY[x] || hash(x, y, 8) != 0 ? P[HillFar] : P[HillFarShade]);
      for (int y = T.ground[x]; y < 64; y++) {
        uint32_t c = P[Grass];
        uint32_t h = hash(x, y, 9);
        if (h == 0) c = P[GrassDark];
        else if (h == 1) c = P[GrassLight];
        if (y > 58 && hash(x, y, 4) == 0) c = P[GrassDark];
        cv.set(x, y, c);
        if (hash(x, y, 47) == 0 && y > T.ground[x] + 2) cv.set(x, y, P[Daisy]);
      }
    }
    for (int x = 0; x < 64; x++) {
      if (hash(x, 3, 3) == 0) continue;
      int h = 1 + int(hash(x, 5, 3));
      real sway = jsround(std::sin(t * real(1.6) + x * real(0.35)) * real(0.7) + breeze * real(0.12));
      for (int i = 1; i <= h; i++) {
        real dx = i == h ? sway : 0;
        cv.set(x + dx, T.ground[x] - i, i == h ? P[Blade] : P[BladeDark]);
      }
    }
  }

  void drawTree(Canvas& cv, const uint32_t* P) {
    cv.rect(12, 44, 14, 53, P[Trunk]);
    cv.rect(14, 44, 14, 53, P[TrunkDark]);
    cv.rect(12, 37, 13, 43, P[Trunk]);
    cv.set(11, 53, P[Trunk]);
    cv.set(15, 53, P[TrunkDark]);
    for (int i = 0; i < 5; i++) cv.set(11 - i, 36 - std::floor(i * real(0.6)), P[Trunk]);
    for (int i = 0; i < 6; i++) cv.set(14 + i, 36 - std::floor(i * real(0.7)), P[Trunk]);

    for (int y = CANOPY_TOP; y <= CANOPY_BOTTOM; y++) {
      for (int x = -2; x <= 28; x++) {
        if (!inCanopy(x, y)) continue;
        if (!inCanopy(x, y - 1) && hash(x, y, 3) == 0) continue;
        real lit = (x - 11) * real(0.8) + (y - 26) * real(1.2);
        uint32_t c = P[Bloom];
        if (lit < -8) c = P[BloomLight];
        else if (lit > 9 || !inCanopy(x, y + 2)) c = P[BloomDeep];
        else if (lit > 3) c = P[BloomDark];
        uint32_t cl = hash(x >> 1, y >> 1, 5);
        if (cl == 0) c = c == P[BloomDeep] ? P[BloomDark] : P[BloomLight];
        else if (cl == 1 && c != P[BloomLight]) c = P[BloomDeep];
        cv.set(x, y, c);
      }
    }
  }

  void drawFlowers(Canvas& cv, const uint32_t* P, real day) {
    real bright = real(0.45) + real(0.55) * day;
    int i = 0;
    for (const Flower& f : FLOWERS) {
      real sway = jsround(std::sin(t * real(1.4) + i * real(1.3)) * real(0.6) + breeze * real(0.12));
      int top = f.base - f.h;
      for (int y = top + 1; y <= f.base; y++) cv.set(f.x + (y == top + 1 ? sway : 0), y, P[Stem]);
      cv.set(f.x - 1, f.base - 1, P[Stem]);
      cv.set(f.x + 1, f.base - 2, P[Stem]);
      uint32_t c = mix(mix(f.c, NIGHT, real(0.6)), f.c, bright);
      tulipPal.put('A', c);
      tulipPal.put('B', dim(c, real(0.7)));
      cv.sprite(TULIP, f.x - 1 + sway, top - 2, tulipPal);
      i++;
    }
  }

  void drawButterflies(Canvas& cv, real day) {
    if (day <= real(0.1)) return;
    for (const Butterfly& b : BUTTERFLIES) {
      real x = jsround(b.cx + b.ax * std::sin(t * real(0.37) + b.ph) + 4 * std::sin(t * real(0.9) + b.ph * 2));
      real y = jsround(b.cy + b.ay * std::sin(t * real(0.71) + b.ph) + std::sin(t * 5 + b.ph));
      bool open = long(std::floor(t * 9 + b.ph * 3)) % 2 == 0;
      cv.blend(x, y, b.tip, day);
      if (open) {
        for (int dx : {-1, 1}) {
          cv.blend(x + dx, y - 1, b.wing, day);
          cv.blend(x + dx, y, b.wing, day);
          cv.blend(x + dx * 2, y - 1, b.tip, day);
        }
      } else {
        cv.blend(x - 1, y - 1, b.wing, day);
        cv.blend(x + 1, y - 1, b.wing, day);
      }
    }
  }

  void drawBird(Canvas& cv) {
    if (!bird.active) return;
    real x = jsround(bird.x);
    real y = jsround(bird.y + std::sin(t * 2) * real(1.2));
    bool up = long(std::floor(t * 6)) % 2 == 0;
    const uint32_t c = 0x2a2a3a;
    cv.set(x, y, c);
    cv.set(x - 1, y, c);
    cv.set(x + 1, y, c);
    cv.set(x - 2, y + (up ? -1 : 1), c);
    cv.set(x + 2, y + (up ? -1 : 1), c);
  }

  void drawFireflies(Canvas& cv, real day) {
    real night = 1 - day;
    if (night < real(0.4)) return;
    for (const Firefly& f : fireflies) {
      real x = f.cx + 5 * std::sin(t * real(0.3) * f.sp + f.ph);
      real y = f.cy + 3 * std::sin(t * real(0.5) * f.sp + f.ph * 2);
      real glow = jsmax(0, std::sin(t * real(1.8) * f.sp + f.ph));
      real a = (night - real(0.4)) / real(0.6) * glow;
      cv.blend(x, y, FIREFLY, a);
      cv.blend(x + 1, y, FIREFLY, a * real(0.3));
      cv.blend(x - 1, y, FIREFLY, a * real(0.3));
    }
  }
};

}  // namespace

std::unique_ptr<Theme> createSpring(Rng& rng) { return std::unique_ptr<Theme>(new Spring(rng)); }

}  // namespace pc
