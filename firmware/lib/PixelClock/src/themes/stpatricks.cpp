// St. Patrick's Day. Port of simulator/themes/stpatricks.js; keep the two in sync.
#include <cstring>

#include "../themes.h"

namespace pc {
namespace {

namespace COL {
constexpr uint32_t hillFar = 0x4f9a5c, hillMid = 0x2f9a3e, hillNear = 0x1e7429, grassDark = 0x165a20,
                   grassLight = 0x47b552, cloud = 0xf4f8ff, cloudShade = 0xc9d6e8, gold = 0xf2c230,
                   goldDark = 0xc99a1e, goldShine = 0xfff2a0, glow = 0xffc940, leaf = 0x4cc85a,
                   leafLight = 0x8ae67e, leafDark = 0x2a9a3c, stem = 0x1d6a2a, clock = 0xf4ffe8,
                   clockShadow = 0x0b2a12;
}

// Rainbow, outermost band first.
const uint32_t RAINBOW[] = {0xe8413c, 0xf39237, 0xf7d23e, 0x4cb84a, 0x3d8fd6, 0x7d5bc4};
constexpr int RAINBOW_COUNT = 6;
constexpr int RB_CX = 27, RB_CY = 50, RB_R = 28;

constexpr int POT_X = 45, POT_Y = 45;
const char* const POT[] = {
    "....gGyGg....", "..gGGGgGGGg..", ".gGGyGGGGyGg.", "RRRRRRRRRRRRR", ".rrrrrrrrrrr.", ".BBBBBBBBBhB.",
    "BBBBBBBBBBhhB", "BBBBBBBBBBhhB", ".BBBBBBBBBhB.", "..BBBBBBBBB..", "..L.......L..",
};
const Palette POT_PAL{{'g', COL::goldDark}, {'G', COL::gold}, {'y', COL::goldShine}, {'R', 0x4a4a54},
                      {'r', 0x16161a},      {'B', 0x26262c},  {'h', 0x5a5a66},       {'L', 0x1c1c20}};

// Three heart-shaped leaves on a stem; drawn as leaves (rows 0-4) and stem (rows 5-7).
const char* const SHAMROCK_TOP[] = {"..LL.LL..", "..GGGGG..", "LL.GGG.LL", "GGGGdGGGG", ".GGGdGGG."};
const char* const SHAMROCK_STEM[] = {"..G.s.G..", "....s....", "....s...."};

const char* const CLOUD[] = {"....www......", "..wwwwwww.ww.", ".wwwwwwwwwwww", "wwwwwwwwwwwww", ".sssssssssss."};

inline int farY(real x) { return 39 + int(jsround(real(2.5) * std::sin(x * real(0.09) + 1))); }
inline int midY(real x) { return 46 + int(jsround(real(2.5) * std::sin(x * real(0.12) + real(3.2)))); }
inline int nearY(real x) { return 55 + int(jsround(real(1.5) * std::sin(x * real(0.16) + real(0.5)))); }

class StPatricks : public Theme {
 public:
  explicit StPatricks(Rng& r)
      : rng(r), cloudPal{{'w', 0}, {'s', 0}}, leafPal{{'L', 0}, {'G', 0}, {'d', 0}, {'s', 0}} {
    clouds[0] = {4, 16, real(1.2)};
    clouds[1] = {40, 20, real(0.8)};
    glint = {0, 0, 1, real(0.3)};
    coins.reserve(32);
  }

  void update(real dt, const Ctx& ctx) override {
    t += dt;
    for (auto& c : clouds) {
      c.x += c.v * dt;
      if (c.x > 66) c.x = -14;
    }

    glint.age += dt;
    if (glint.age > glint.life && rng.chance(real(2.5) * dt)) {
      int row = rng.intn(3);
      const char* cols = POT[row];
      int len = int(std::strlen(cols));
      int x = rng.intn(len);
      for (int i = 0; i < len && cols[x] == '.'; i++) x = (x + 1) % len;
      glint = {real(POT_X + x), real(POT_Y + row), 0, real(0.35)};
    }

    if (ctx.shake) {
      for (int i = 0; i < 18; i++) {
        bool toPot = rng.chance(real(0.7));
        real vy = -(24 + rng.flt() * 20);
        const real g = 60;
        real flight = (2 * -vy) / g;
        real target = toPot ? POT_X + 3 + rng.flt() * 7 : 4 + rng.flt() * 36;
        Coin c;
        c.x = POT_X + real(6.5);
        c.y = POT_Y + 1;
        c.vx = (target - (POT_X + real(6.5))) / (toPot ? flight : flight * real(1.25));
        c.vy = vy;
        c.g = g;
        c.toPot = toPot;
        c.landed = 0;
        c.ph = rng.flt() * real(6.283);
        coins.push_back(c);
      }
    }
    for (int i = int(coins.size()) - 1; i >= 0; i--) {
      Coin& c = coins[i];
      if (c.landed > 0) {
        c.landed += dt;
        if (c.landed > 3) coins.erase(coins.begin() + i);
        continue;
      }
      c.vy += c.g * dt;
      c.x += c.vx * dt;
      c.y += c.vy * dt;
      if (c.vy > 0) {
        if (c.toPot && c.y >= POT_Y + 1) coins.erase(coins.begin() + i);
        else if (!c.toPot && c.y >= nearY(jsround(c.x)) - 1) {
          c.y = real(nearY(jsround(c.x)) - 1);
          c.landed = real(0.001);
        }
      }
    }
  }

  void draw(Canvas& cv, const Ctx& ctx) override {
    real hf = ctx.hourFloat;
    real day = daylight(hf);
    drawSky(cv, hf, day);
    drawHillsFar(cv, day);
    drawRainbow(cv, day);
    drawHillsMid(cv, day);
    drawPot(cv, day);
    drawHillsNear(cv, day);
    drawShamrocks(cv, day);
    drawCoins(cv);
    drawClock(cv, ctx, COL::clock, COL::clockShadow);
  }

 private:
  struct Cloud {
    real x;
    int y;
    real v;
  };
  struct Glint {
    real x, y, age, life;
  };
  struct Coin {
    real x, y, vx, vy, g;
    bool toPot;
    real landed, ph;
  };

  Rng& rng;
  real t = 0;
  Cloud clouds[2];
  Glint glint;
  std::vector<Coin> coins;
  Palette cloudPal;
  Palette leafPal;

  void drawSky(Canvas& cv, real hf, real day) {
    SkyColors sky = skyAt(hf);
    vgradient(cv, 0, 0, 63, 50, sky.top, sky.bottom);
    drawStars(cv, t, 1 - day, 0, 0, 63, 36, 26);
    cloudPal.put('w', mix(0x5a6070, COL::cloud, day));
    cloudPal.put('s', mix(0x3a4050, COL::cloudShade, day));
    for (const Cloud& c : clouds) cv.sprite(CLOUD, jsround(c.x), c.y, cloudPal);
  }

  void drawHillsFar(Canvas& cv, real day) {
    real k = real(0.4) + real(0.6) * day;
    uint32_t c = dim(COL::hillFar, k);
    for (int x = 0; x < 64; x++)
      for (int y = farY(x); y <= 50; y++) cv.set(x, y, c);
  }

  void drawHillsMid(Canvas& cv, real day) {
    real k = real(0.4) + real(0.6) * day;
    uint32_t c = dim(COL::hillMid, k);
    uint32_t cd = dim(COL::grassDark, k);
    for (int x = 0; x < 64; x++)
      for (int y = midY(x); y < 64; y++) cv.set(x, y, hash(x, y, 9) == 0 ? cd : c);
  }

  void drawHillsNear(Canvas& cv, real day) {
    real k = real(0.4) + real(0.6) * day;
    uint32_t c = dim(COL::hillNear, k);
    uint32_t cl = dim(COL::grassLight, k);
    uint32_t cd = dim(COL::grassDark, k);
    for (int x = 0; x < 64; x++) {
      int top = nearY(x);
      cv.set(x, top, cl);
      for (int y = top + 1; y < 64; y++) cv.set(x, y, hash(x, y, 6) == 0 ? cd : c);
      if (hash(x, 3, 4) == 0) cv.set(x + (std::sin(t * 2 + x) > real(0.3) ? 1 : 0), top - 1, cl);
    }
  }

  void drawRainbow(Canvas& cv, real day) {
    real a = real(0.12) + real(0.73) * day;
    for (int y = RB_CY - RB_R; y <= RB_CY; y++) {
      for (int x = RB_CX - RB_R; x <= RB_CX + RB_R; x++) {
        real d = std::hypot(real(x - RB_CX), real(y - RB_CY));
        real band = std::floor(RB_R - d);
        if (band >= 0 && band < RAINBOW_COUNT) cv.blend(x, y, RAINBOW[int(band)], a);
      }
    }
  }

  void drawPot(Canvas& cv, real day) {
    real night = 1 - day;
    if (night > 0) {
      real flick = real(0.85) + real(0.15) * std::sin(t * 5);
      const int cx = POT_X + 6, cy = POT_Y + 1;
      for (int y = cy - 9; y <= cy + 7; y++) {
        for (int x = cx - 11; x <= cx + 11; x++) {
          real d = std::hypot(real(x - cx), (y - cy) * real(1.3));
          if (d < 11) {
            real f = 1 - d / 11;
            cv.blend(x, y, COL::glow, night * real(0.35) * flick * (f * f));
          }
        }
      }
    }
    cv.sprite(POT, POT_X, POT_Y, POT_PAL);
    if (glint.age < glint.life) {
      real k = std::sin((glint.age / glint.life) * kPi);
      cv.blend(glint.x, glint.y, 0xffffff, k);
      static const int DIRS[3][2] = {{1, 0}, {-1, 0}, {0, -1}};
      for (const auto& d : DIRS) cv.blend(glint.x + d[0], glint.y + d[1], 0xffffff, k * real(0.6));
    }
  }

  void drawShamrocks(Canvas& cv, real day) {
    real k = real(0.5) + real(0.5) * day;
    leafPal.put('L', dim(COL::leafLight, k));
    leafPal.put('G', dim(COL::leaf, k));
    leafPal.put('d', dim(COL::leafDark, k));
    leafPal.put('s', dim(COL::stem, k));
    struct Spot {
      int x, y;
      real ph;
    };
    static const Spot SPOTS[] = {{2, 55, 0}, {14, 57, real(1.3)}, {29, 56, real(2.1)}, {58, 57, real(0.7)}};
    for (const Spot& s : SPOTS) {
      real sway = jsround(std::sin(t * real(1.4) + s.ph) * real(0.9));
      cv.sprite(SHAMROCK_TOP, s.x + sway, s.y, leafPal);
      cv.sprite(SHAMROCK_STEM, s.x, s.y + 5, leafPal);
    }
  }

  void drawCoins(Canvas& cv) {
    for (const Coin& c : coins) {
      real fade = c.landed > 0 ? clamp(1 - (c.landed - 2) / 1, 0, 1) : 1;
      bool wide = std::sin(t * 14 + c.ph) > real(-0.3);
      if (wide) {
        cv.blend(c.x, c.y, COL::goldShine, fade);
        cv.blend(c.x + 1, c.y, COL::gold, fade);
        cv.blend(c.x, c.y + 1, COL::gold, fade);
        cv.blend(c.x + 1, c.y + 1, COL::goldDark, fade);
      } else {
        cv.blend(c.x, c.y, COL::goldShine, fade);
        cv.blend(c.x, c.y + 1, COL::goldDark, fade);
      }
    }
  }
};

}  // namespace

std::unique_ptr<Theme> createStPatricks(Rng& rng) { return std::unique_ptr<Theme>(new StPatricks(rng)); }

}  // namespace pc
