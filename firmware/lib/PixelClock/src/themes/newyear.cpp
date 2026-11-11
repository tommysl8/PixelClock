// New Year's Eve. Port of simulator/themes/newyear.js; keep the two in sync.
#include <cstdio>

#include "../themes.h"

namespace pc {
namespace {

namespace COL {
constexpr uint32_t skyTop = 0x04051a, skyBottom = 0x2e2160, haze = 0x6a3f86, back = 0x15142c, backEdge = 0x2a2850,
                   front = 0x07070d, frontEdge = 0x1a1a30, street = 0x07070e, winWarm = 0xffd27a,
                   winCool = 0xbfe0ff, winOff = 0x161628, beacon = 0xff3030, year = 0xffd23f,
                   yearShadow = 0x2a1a00, clock = 0xfff4d6, clockShadow = 0x05040c;
}

const uint32_t FIREWORK_COLORS[] = {0xffd23f, 0xf2f2ff, 0xff4f6a, 0x5aa8ff, 0xff7fd0, 0x7dff8a, 0xffa53a};
constexpr int FIREWORK_COUNT = 7;
const uint32_t CONFETTI_COLORS[] = {0xffd23f, 0xff7fd0, 0x4fd8ff, 0xffffff, 0x7dff8a, 0xff8a3a};
constexpr int CONFETTI_COUNT = 6;

// Skyline: [x0, x1, top row]. Back row is taller and lighter (further away).
const int BACK[][3] = {{0, 6, 40},   {7, 12, 34},  {13, 19, 38}, {20, 25, 31}, {26, 33, 37},
                       {34, 39, 33}, {40, 47, 39}, {48, 53, 30}, {54, 63, 36}};
const int FRONT[][3] = {{0, 4, 50},   {5, 11, 45},  {12, 16, 52}, {17, 24, 47}, {25, 29, 54},
                        {30, 37, 44}, {38, 42, 51}, {48, 55, 46}, {56, 63, 50}};
constexpr int SPIRE_X = 45, SPIRE_TOP = 24, SPIRE_BASE = 42;
constexpr int STREET_Y = 61;

FireworksOptions fireworkOptions() {
  FireworksOptions o;
  o.colors.assign(FIREWORK_COLORS, FIREWORK_COLORS + FIREWORK_COUNT);
  o.rate = real(0.8);
  o.x0 = 8;
  o.x1 = 56;
  o.launchY = 44;
  o.minY = 22;
  o.maxY = 32;
  return o;
}

class NewYear : public Theme {
 public:
  explicit NewYear(Rng& r) : rng(r), fw(r, fireworkOptions()) {
    for (const auto& b : FRONT) {
      int x0 = b[0], x1 = b[1], top = b[2];
      for (int y = top + 2; y < STREET_Y - 1; y += 3) {
        for (int x = x0 + 1; x < x1; x += 2) {
          if (hash(x, y, 5) == 0) continue;  // some walls have no window
          Window w;
          w.x = x;
          w.y = y;
          w.on = hash(x, y, 3) != 0;
          w.warm = hash(y, x, 4) != 0;
          windows.push_back(w);
        }
      }
    }
    for (const auto& b : BACK) {
      int x0 = b[0], x1 = b[1], top = b[2];
      for (int y = top + 2; y < 52; y += 4)
        for (int x = x0 + 1; x < x1; x += 3)
          if (hash(x, y, 4) == 0) farWindows.push_back({x, y});
    }
    confetti.reserve(80);
    for (int i = 0; i < 14; i++) addConfetti(true);
  }

  void update(real dt, const Ctx& ctx) override {
    t += dt;
    bool rush = isRush(ctx);

    // The stroke of midnight: everything at once.
    if (lastHour == 23 && ctx.hours == 0) {
      volley += 10;
      for (int i = 0; i < 4; i++) {
        real bx = 10 + rng.flt() * 44;
        real by = 20 + rng.flt() * 10;
        uint32_t c = FIREWORK_COLORS[rng.intn(FIREWORK_COUNT)];
        fw.burst(bx, by, c);
      }
    }
    lastHour = ctx.hours;

    if (ctx.shake) volley += 8;
    if (volley > 0) {
      volleyTimer -= dt;
      if (volleyTimer <= 0) {
        fw.launch(8 + rng.flt() * 48);
        volley--;
        volleyTimer = real(0.12);
      }
    }

    fw.rate = rush ? real(3.5) : real(0.8);
    fw.update(dt);

    // Windows flick on and off now and then.
    if (rng.chance(dt * 2)) {
      Window& w = windows[rng.intn(int(windows.size()))];
      w.on = !w.on;
    }

    const int target = rush ? 70 : 16;
    if (int(confetti.size()) < target && rng.chance(rush ? real(0.9) : real(0.25))) addConfetti(false);
    for (int i = int(confetti.size()) - 1; i >= 0; i--) {
      Confetti& p = confetti[i];
      p.y += p.vy * dt;
      p.x += std::sin(t * real(2.5) + p.ph) * p.sway * dt * 6;
      if (p.y > STREET_Y + 2) {
        if (int(confetti.size()) > target) confetti.erase(confetti.begin() + i);
        else {
          p.y = -1;
          p.x = rng.flt() * 64;
        }
      }
    }
  }

  void draw(Canvas& cv, const Ctx& ctx) override {
    vgradient(cv, 0, 0, 63, STREET_Y, COL::skyTop, COL::skyBottom);
    // city glow just above the rooftops
    for (int y = 24; y < 46; y++) {
      real a = (real(y - 24) / 22) * real(0.5);
      for (int x = 0; x < 64; x++) cv.blend(x, y, COL::haze, a);
    }
    drawStars(cv, t, real(0.7), 0, 0, 63, 26, 22);

    fw.draw(cv);
    drawSkyline(cv);

    for (const auto& p : confetti) {
      real flip = std::sin(t * 9 + p.ph);
      cv.set(p.x, p.y, flip > 0 ? p.c : dim(p.c, real(0.45)));
    }

    // the year below the clock: on Dec 31 it's the year that's coming
    long year = (ctx.month == 12 && ctx.day == 31) ? ctx.year + 1 : ctx.year;
    char text[24];
    int len = std::snprintf(text, sizeof text, "%ld", year);
    int x = int(std::floor(real(64 - (len * 4 - 1)) / 2));
    drawSmallNumber(cv, year, x + 1, 17, COL::yearShadow);
    real s = jsmax(0, std::sin(t * real(1.3)));
    uint32_t glint = mix(COL::year, 0xfff6c8, std::pow(s, real(6)));
    drawSmallNumber(cv, year, x, 16, glint);

    drawClock(cv, ctx, COL::clock, COL::clockShadow);
  }

 private:
  struct Window {
    int x, y;
    bool on, warm;
  };
  struct Point {
    int x, y;
  };
  struct Confetti {
    real x, y, vy, ph, sway;
    uint32_t c;
  };

  Rng& rng;
  Fireworks fw;
  real t = 0;
  int lastHour = -1;
  int volley = 0;  // rockets left to launch quickly
  real volleyTimer = 0;
  std::vector<Window> windows;
  std::vector<Point> farWindows;
  std::vector<Confetti> confetti;

  void addConfetti(bool anywhere) {
    Confetti p;
    p.x = rng.flt() * 64;
    p.y = anywhere ? rng.flt() * 60 : -1 - rng.flt() * 6;
    p.vy = 5 + rng.flt() * 6;
    p.ph = rng.flt() * real(6.283);
    p.sway = real(0.6) + rng.flt() * real(1.2);
    p.c = CONFETTI_COLORS[rng.intn(CONFETTI_COUNT)];
    confetti.push_back(p);
  }

  static bool isRush(const Ctx& ctx) {
    return (ctx.hours == 23 && ctx.minutes >= 59) || (ctx.hours == 0 && ctx.minutes < 15);
  }

  void drawSkyline(Canvas& cv) {
    for (const auto& b : BACK) {
      cv.rect(b[0], b[2], b[1], STREET_Y, COL::back);
      cv.rect(b[0], b[2], b[1], b[2], COL::backEdge);
    }
    for (const auto& w : farWindows) cv.set(w.x, w.y, 0x3a3a62);

    // needle tower with a blinking beacon
    cv.rect(SPIRE_X - 1, SPIRE_BASE - 8, SPIRE_X + 1, STREET_Y, COL::back);
    cv.rect(SPIRE_X - 2, SPIRE_BASE - 10, SPIRE_X + 2, SPIRE_BASE - 9, COL::backEdge);
    cv.rect(SPIRE_X, SPIRE_TOP, SPIRE_X, SPIRE_BASE - 11, COL::backEdge);
    cv.set(SPIRE_X, SPIRE_TOP - 1,
           long(std::floor(t * real(1.2))) % 2 == 0 ? COL::beacon : dim(COL::beacon, real(0.3)));

    for (const auto& b : FRONT) {
      cv.rect(b[0], b[2], b[1], STREET_Y, COL::front);
      cv.rect(b[0], b[2], b[1], b[2], COL::frontEdge);
    }
    for (const auto& w : windows) cv.set(w.x, w.y, w.on ? (w.warm ? COL::winWarm : COL::winCool) : COL::winOff);

    cv.rect(0, STREET_Y + 1, 63, 63, COL::street);
    // street lights
    for (int x = 3; x < 64; x += 12) cv.set(x, STREET_Y + 1, 0xffc870);
  }
};

}  // namespace

std::unique_ptr<Theme> createNewYear(Rng& rng) { return std::unique_ptr<Theme>(new NewYear(rng)); }

}  // namespace pc
