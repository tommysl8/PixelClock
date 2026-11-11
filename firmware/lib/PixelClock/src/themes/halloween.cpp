// Haunted hill (Halloween). Port of simulator/themes/halloween.js; keep the two in sync.
#include <algorithm>
#include <cstdlib>
#include <cstring>

#include "../themes.h"

namespace pc {
namespace {

namespace COL {
constexpr uint32_t skyTop = 0x120a26, skyBottom = 0x4a2460, moon = 0xf6d58a, moonShade = 0xdcb46a,
                   moonGlow = 0xffb060, hill = 0x170c22, ground = 0x1c1028, tuft = 0x2a1a3e, house = 0x0a0610,
                   window = 0xffcc4a, windowDim = 0x8a5a18, stone = 0x6a6a7e, stoneShade = 0x45455a,
                   stoneLight = 0x9494ae, pumpkin = 0xe0701a, pumpkinShade = 0xa84a10, stem = 0x3a6a20,
                   candle = 0xffd84a, candleLow = 0xffa020, glow = 0xff9a30, ghost = 0xe8ecff, ghostEye = 0x1a1030,
                   bat = 0x05030a, fog = 0x9484b8, clock = 0xff8c1a, clockShadow = 0x1a0800;
}

constexpr int MOON_X = 47, MOON_Y = 24;
constexpr real MOON_R = real(7.5);
constexpr int TOWER_X = 21, TOWER_Y = 25;

int hillY(int x) { return int(jsmin(47, jsround(33 + real((x - 15) * (x - 15)) / 40))); }

const char* const PUMPKIN_BIG[] = {"...ss...", ".oOOOOo.", "oOyOOyOo", "oOOOOOOo",
                                   "oOyyyyOo", ".oOyOyo.", "..oooo.."};
const char* const PUMPKIN_SMALL[] = {"..s..", ".oOo.", "oyOyo", "oyyyo", ".ooo."};
const char* const GHOST_A[] = {".WWW.", "WkWkW", "WWWWW", "WWWWW", "WWWWW", "W.W.W"};
const char* const GHOST_B[] = {".WWW.", "WkWkW", "WWWWW", "WWWWW", "WWWWW", ".W.W."};
const char* const BAT_UP[] = {"B...B", ".BBB.", "..B.."};
const char* const BAT_DOWN[] = {".....", ".BBB.", "B.B.B"};
const Palette GHOST_PAL{{'W', COL::ghost}, {'k', COL::ghostEye}};
const Palette BAT_PAL{{'B', COL::bat}};

const int WINDOWS[][2] = {{9, 29}, {15, 29}, {9, 33}, {16, 33}, {13, 23}, {21, 25}, {21, 31}};
constexpr int WINDOW_COUNT = 7;

// Lightning brightness over time: a flash, a dip, a second flash, then fade.
real flashLevel(real s) {
  if (s < real(0.12)) return 1;
  if (s < real(0.25)) return real(0.2);
  if (s < real(0.45)) return real(0.85);
  if (s < real(1.2)) return real(0.85) * (1 - (s - real(0.45)) / real(0.75));
  return 0;
}

class Halloween : public Theme {
 public:
  explicit Halloween(Rng& r) : rng(r) {
    buildBackground();

    for (int i = 0; i < 40 && stars.size() < 18; i++) {
      int x = int(hash(i, 41, 64));
      int y = 14 + int(hash(i, 43, 18));
      if (skyMask[y * 64 + x] == 1) stars.push_back({x, y, i});
    }

    for (int i = 0; i < WINDOW_COUNT; i++) winLevel[i] = 1;

    // Same order of random draws as the JS object literal.
    bats.reserve(32);
    for (int i = 0; i < 5; i++) {
      Bat b;
      b.x = rng.flt() * 64;
      b.y0 = 17 + rng.flt() * 14;
      b.y = 0;
      real sign = rng.chance(real(0.5)) ? 1 : -1;
      b.vx = sign * (7 + rng.flt() * 6);
      b.vy = 0;
      b.amp = real(1.5) + rng.flt() * real(2.5);
      b.ph = rng.flt() * real(6.283);
      b.flap = rng.flt();
      b.free = false;
      bats.push_back(b);
    }
  }

  void update(real dt, const Ctx& ctx) override {
    t += dt;

    for (int i = 0; i < WINDOW_COUNT; i++) {
      real target = real(0.75) + rng.flt() * real(0.25);
      if (rng.chance(real(0.004))) winLevel[i] = real(0.15);  // a light gutters out for a moment
      winLevel[i] += (target - winLevel[i]) * jsmin(1, dt * real(2.5));
    }
    if (towerShadow > 0) towerShadow -= dt;
    else if (rng.chance(dt / 9)) towerShadow = real(1.2);

    candle += (real(0.7) + rng.flt() * real(0.3) - candle) * real(0.5);

    for (int i = int(bats.size()) - 1; i >= 0; i--) {
      Bat& b = bats[i];
      b.flap += dt * (b.free ? 10 : 7);
      if (b.free) {
        b.x += b.vx * dt;
        b.y += (b.vy + std::sin(t * 9 + b.ph) * 10) * dt;
        if (b.x < -8 || b.x > 70 || b.y < -6 || b.y > 70) bats.erase(bats.begin() + i);
      } else {
        b.x += b.vx * dt;
        b.y = b.y0 + std::sin(t * real(1.7) + b.ph) * b.amp;
        if (b.x > 70) b.x = -7;
        if (b.x < -7) b.x = 70;
      }
    }

    ghostX += 5 * dt;
    if (ghostX > 72) ghostX = -8;

    if (ctx.shake) strike();
    sinceStrike += dt;
  }

  void draw(Canvas& cv, const Ctx& ctx) override {
    std::memcpy(cv.px, bg.px, sizeof(bg.px));

    for (const Star& s : stars) {
      real tw = real(0.4) + real(0.4) * std::sin(t * (real(1.3) + (s.i % 4) * real(0.5)) + s.i * real(2.3));
      cv.blend(s.x, s.y, 0xd8d0ff, tw);
    }

    // windows
    for (int i = 0; i < WINDOW_COUNT; i++) {
      int x = WINDOWS[i][0], y = WINDOWS[i][1];
      uint32_t c = mix(COL::windowDim, COL::window, winLevel[i]);
      cv.rect(x, y, x + 1, y + 1, c);
    }
    if (towerShadow > 0) {
      // someone walks past the tower window
      int sx = TOWER_X + (towerShadow > real(0.6) ? 0 : 1);
      cv.set(sx, TOWER_Y, COL::house);
      cv.set(sx, TOWER_Y + 1, COL::house);
    }

    // ghost drifting over the hill
    real gy = jsround(36 + std::sin(t * 2) * 2);
    bool ga = long(std::floor(t * 4)) % 2 != 0;
    cv.sprite(ga ? GHOST_A : GHOST_B, 6, jsround(ghostX), gy, GHOST_PAL);

    drawFog(cv);

    // jack-o'-lanterns and their glow
    uint32_t lit = mix(COL::candleLow, COL::candle, candle);
    const int glows[2][3] = {{8, 55, 9}, {26, 57, 6}};
    for (const auto& g : glows) {
      int cx = g[0], cy = g[1], r = g[2];
      for (int y = cy - r; y <= cy + r; y++) {
        for (int x = cx - r; x <= cx + r; x++) {
          real d = std::hypot(real(x - cx), (y - cy) * real(1.3));
          if (d < r) cv.blend(x, y, COL::glow, real(0.28) * candle * (1 - d / r));
        }
      }
    }
    Palette pal{{'O', COL::pumpkin}, {'o', COL::pumpkinShade}, {'s', COL::stem}, {'y', lit}};
    cv.sprite(PUMPKIN_BIG, 4, 52, pal);
    cv.sprite(PUMPKIN_SMALL, 24, 55, pal);

    for (const Bat& b : bats) {
      bool up = long(std::floor(b.flap * 2)) % 2 != 0;
      cv.sprite(up ? BAT_UP : BAT_DOWN, 3, jsround(b.x) - 2, jsround(b.y) - 1, BAT_PAL);
    }

    real f = flashLevel(sinceStrike);
    if (f > 0) {
      for (int i = 14 * 64; i < W * H; i++) cv.px[i] = mix(cv.px[i], 0xd8d8ff, f * real(0.5));
      if (sinceStrike < real(0.5))
        for (const auto& p : bolt) cv.set(p.x, p.y, 0xffffff);
    }

    drawClock(cv, ctx, COL::clock, COL::clockShadow);
  }

 private:
  struct Star {
    int x, y, i;
  };
  struct Bat {
    real x = 0, y0 = 0, y = 0, vx = 0, vy = 0, amp = 0, ph = 0, flap = 0;
    bool free = false;
  };
  struct Point {
    int x, y;
  };

  Rng& rng;
  real t = 0;
  Canvas bg;
  uint8_t skyMask[64 * 64] = {};
  std::vector<Star> stars;
  real winLevel[WINDOW_COUNT] = {};
  real towerShadow = 0;
  std::vector<Bat> bats;
  real ghostX = -6;
  real candle = 1;
  real sinceStrike = 99;
  std::vector<Point> bolt;

  // ---------- static scene (always night, built once) ----------
  void setSolid(int x, int y, uint32_t c) {
    bg.set(x, y, c);
    if (x >= 0 && x < 64 && y >= 0 && y < 64) skyMask[y * 64 + x] = 0;
  }

  void line(int x0, int y0, int x1, int y1, uint32_t c) {
    int n = std::max(std::abs(x1 - x0), std::abs(y1 - y0));
    for (int i = 0; i <= n; i++)
      setSolid(int(jsround(x0 + real((x1 - x0) * i) / n)), int(jsround(y0 + real((y1 - y0) * i) / n)), c);
  }

  void stone(int x0, int y0, int w, int h, bool cross) {
    for (int y = y0; y < y0 + h; y++) {
      for (int x = x0; x < x0 + w; x++) {
        bool top = y == y0 && (x == x0 || x == x0 + w - 1);
        if (top && !cross) continue;
        uint32_t c = COL::stone;
        if (x == x0) c = COL::stoneShade;
        if (x == x0 + w - 1) c = COL::stoneLight;
        if (y == y0 + h - 1) c = COL::stoneShade;
        setSolid(x, y, c);
      }
    }
  }

  void buildBackground() {
    vgradient(bg, 0, 0, 63, 47, COL::skyTop, COL::skyBottom);
    std::memset(skyMask, 1, sizeof(skyMask));

    // moon with glow and craters
    for (int y = 10; y <= 40; y++) {
      for (int x = 32; x <= 63; x++) {
        real d = std::hypot(real(x - MOON_X), real(y - MOON_Y));
        if (d <= MOON_R) {
          uint32_t c = COL::moon;
          bool crater = hash((x - 30) / 2, (y - 10) / 2, 5) == 0;
          if (crater || (x - MOON_X) + (y - MOON_Y) > 6) c = COL::moonShade;
          bg.set(x, y, c);
          skyMask[y * 64 + x] = 2;  // moon: stars skip it
        } else if (d < 14) {
          bg.blend(x, y, COL::moonGlow, real(0.2) * (1 - (d - MOON_R) / (14 - MOON_R)));
        }
      }
    }

    // hill
    for (int x = 0; x < 64; x++)
      for (int y = hillY(x); y < 64; y++) setSolid(x, y, COL::hill);

    // haunted house
    const uint32_t H = COL::house;
    for (int y = 27; y <= 37; y++)
      for (int x = 7; x <= 20; x++) setSolid(x, y, H);
    for (int y = 20; y <= 26; y++) {
      real half = (y - 19) * real(1.2);
      for (int x = int(jsround(real(13.5) - half)); x <= int(jsround(real(13.5) + half)); x++) setSolid(x, y, H);
    }
    for (int y = 22; y <= 37; y++)
      for (int x = 19; x <= 24; x++) setSolid(x, y, H);
    for (int y = 14; y <= 21; y++) {
      real half = (y - 14) * real(0.45);
      for (int x = int(jsround(real(21.5) - half)); x <= int(jsround(real(21.5) + half)); x++) setSolid(x, y, H);
    }
    for (int y = 18; y <= 23; y++)
      for (int x = 9; x <= 10; x++) setSolid(x, y, H);
    setSolid(5, 26, H);  // sagging eave
    setSolid(26, 22, H);

    // dead tree reaching across the moon
    const uint32_t T = COL::house;
    line(58, 50, 58, 30, T);
    line(59, 50, 59, 36, T);
    line(58, 36, 52, 31, T);
    line(52, 31, 49, 30, T);
    line(54, 33, 53, 28, T);
    line(58, 33, 62, 27, T);
    line(60, 30, 63, 31, T);
    line(58, 31, 56, 25, T);
    line(56, 25, 54, 23, T);

    // foreground ground with tufts
    for (int x = 0; x < 64; x++) {
      int gy = 48 + int(jsround(std::sin(x * real(0.2)) * real(0.8)));
      for (int y = gy; y < 64; y++) setSolid(x, y, hash(x, y, 9) == 0 ? COL::tuft : COL::ground);
      if (hash(x, 3, 4) == 0) setSolid(x, gy - 1, COL::tuft);
    }

    // tombstones (moonlit from the right)
    stone(31, 47, 5, 7, false);
    setSolid(33, 49, COL::stoneShade);  // engraving
    setSolid(33, 51, COL::stoneShade);
    stone(51, 50, 4, 6, false);
    // cross
    for (int y = 47; y <= 54; y++) setSolid(42, y, y == 54 ? COL::stoneShade : COL::stoneLight);
    for (int y = 47; y <= 54; y++) setSolid(41, y, COL::stone);
    for (int x = 39; x <= 44; x++) setSolid(x, 49, x == 44 ? COL::stoneLight : COL::stone);
  }

  void strike() {
    sinceStrike = 0;
    bolt.clear();
    int x = 30 + rng.intn(14);
    for (int y = 15; y <= 40; y++) {
      bolt.push_back({x, y});
      if (rng.chance(real(0.45))) x += rng.chance(real(0.5)) ? 1 : -1;
      if (rng.chance(real(0.12)) && y < 34) {
        // a small fork
        int fx = x;
        for (int k = 1; k < 5; k++) {
          fx += rng.chance(real(0.5)) ? 1 : 2;
          bolt.push_back({fx, y + k});
        }
      }
    }
    for (int i = 0; i < 10; i++) {
      real a = -kPi * (real(0.1) + rng.flt() * real(0.8));
      real s = 22 + rng.flt() * 18;
      Bat b;
      b.x = TOWER_X;
      b.y = TOWER_Y;
      b.vx = std::cos(a) * s;
      b.vy = std::sin(a) * s;
      b.ph = rng.flt() * real(6.283);
      b.flap = rng.flt();
      b.free = true;
      bats.push_back(b);
    }
  }

  void drawFog(Canvas& cv) {
    for (int y = 42; y <= 60; y++) {
      real band = 1 - std::abs(y - 51) / real(9);
      if (band <= 0) continue;
      for (int x = 0; x < 64; x++) {
        real n = std::sin(x * real(0.23) + t * real(0.6)) +
                 std::sin(x * real(0.11) - t * real(0.35) + y * real(0.6)) * real(0.8);
        real a = clamp((n + real(0.6)) / real(2.4), 0, 1) * band * real(0.32);
        if (a > real(0.02)) cv.blend(x, y, COL::fog, a);
      }
    }
  }
};

}  // namespace

std::unique_ptr<Theme> createHalloween(Rng& rng) { return std::unique_ptr<Theme>(new Halloween(rng)); }

}  // namespace pc
