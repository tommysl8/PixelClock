// Lunar New Year. Port of simulator/themes/lunarnewyear.js; keep the two in sync.
#include "../themes.h"

namespace pc {
namespace {

namespace COL {
constexpr uint32_t skyTop = 0x0c0a2a, skyBottom = 0x4a0f22, dayTop = 0x3a1a4a, dayBottom = 0x9a3a3a,
                   beam = 0x6a3418, beamShade = 0x3a1a0c, trim = 0xc9a227, string = 0x8a6a2a, red = 0xd8202a,
                   redDark = 0x8a1018, redLight = 0xff6a3a, gold = 0xe8b84a, goldLight = 0xffe08a,
                   glow = 0xff3a20, branch = 0x3a2016, blossom = 0xff9ac0, blossomDeep = 0xe0608a,
                   blossomCenter = 0xffe08a, roof = 0x2a0e18, roofAlt = 0x3a1420, smoke = 0x8a8090,
                   clock = 0xffd75a, clockShadow = 0x1a0508;
}

const uint32_t FIREWORK_COLORS[] = {0xff3030, 0xffc23a, 0xffe08a, 0xff6a3a, 0xff3030};
constexpr int FIREWORK_COUNT = 5;

constexpr int BEAM_Y = 15;
constexpr int ROOF_Y = 55;

// Lantern: 7 wide. G gold, R red, d dark red, h highlight rib, t tassel.
const char* const LANTERN[] = {
    "...GGG...", ".GGGGGGG.", ".dRRhRRd.", "dRRRhRRRd", "dRRRhRRRd", "dRRRhRRRd", "dRRRhRRRd",
    ".dRRhRRd.", ".GGGGGGG.", "...GGG...", "....t....", "...t.t...", "...t.t...",
};
const Palette LANTERN_PAL{{'G', COL::gold}, {'R', COL::red}, {'d', COL::redDark}, {'h', COL::redLight},
                          {'t', COL::gold}};

// [hang x, string length, sway phase]
struct LanternDef {
  int hx, len;
  real ph;
};
const LanternDef LANTERNS[] = {{15, 2, 0}, {32, 8, real(1.7)}, {49, 4, real(3.1)}};

// Firecracker strings: [x, top y, count]
struct StringDef {
  int x, y, n;
};
const StringDef STRINGS[] = {{4, BEAM_Y + 2, 7}, {59, BEAM_Y + 2, 7}};
constexpr int STRING_COUNT = 2;

// Dragon head, facing right.
const char* const DRAGON_HEAD[] = {".G.G..", ".RRRR.", "RRRRwR", "RRRRRRR", ".GG.G.."};
const Palette DRAGON_PAL{{'G', COL::gold}, {'R', COL::red}, {'w', 0xffffff}};

// Plum branch from the left edge, blossoms at some points.
const int BRANCH[][2] = {{0, 47},  {1, 47},  {2, 46},  {3, 46},  {4, 45},  {5, 45}, {6, 44},
                         {7, 44},  {8, 43},  {9, 43},  {10, 42}, {11, 41}, {12, 41}, {13, 40},
                         {7, 43},  {7, 42},  {8, 41},  {4, 44},  {4, 43}};
const int BLOSSOMS[][2] = {{3, 44}, {8, 40}, {12, 39}, {14, 41}, {6, 46}, {10, 44}, {1, 45}};

FireworksOptions fireworkOptions() {
  FireworksOptions o;
  o.colors.assign(FIREWORK_COLORS, FIREWORK_COLORS + FIREWORK_COUNT);
  o.rate = real(0.35);
  o.x0 = 20;
  o.x1 = 46;
  o.launchY = ROOF_Y - 2;
  o.minY = 22;
  o.maxY = 32;
  return o;
}

class LunarNewYear : public Theme {
 public:
  explicit LunarNewYear(Rng& r) : rng(r), fw(r, fireworkOptions()) {
    for (int s = 0; s < STRING_COUNT; s++) crackers[s].assign(STRINGS[s].n, false);
    queue.reserve(16);
    flashes.reserve(16);
    smoke.reserve(48);
    for (int i = 0; i < 14; i++) {
      Sparkle p;
      p.x = rng.flt() * 64;
      p.y = 16 + rng.flt() * 40;
      p.vy = 2 + rng.flt() * 3;
      p.ph = rng.flt() * real(6.283);
      sparkles.push_back(p);
    }
  }

  void update(real dt, const Ctx& ctx) override {
    t += dt;
    fw.update(dt);

    if (ctx.shake) {
      for (int s = 0; s < STRING_COUNT; s++) {
        int n = STRINGS[s].n;
        for (int i = n - 1; i >= 0; i--) queue.push_back({s, i, (n - 1 - i) * real(0.08) + s * real(0.04)});
      }
      for (int k = 0; k < 3; k++) fw.launch();
    }
    for (int q = int(queue.size()) - 1; q >= 0; q--) {
      queue[q].delay -= dt;
      if (queue[q].delay <= 0) {
        pop(queue[q].s, queue[q].i);
        queue.erase(queue.begin() + q);
      }
    }

    // the odd pop from the bottom of a string, and reloading once a string is spent
    for (int s = 0; s < STRING_COUNT; s++) {
      std::vector<bool>& list = crackers[s];
      int next = -1;
      for (int i = int(list.size()) - 1; i >= 0; i--)
        if (!list[i]) {
          next = i;
          break;
        }
      if (next < 0) {
        reload[s] += dt;
        if (reload[s] > 4) {
          list.assign(list.size(), false);
          reload[s] = 0;
        }
      } else if (rng.chance(dt * real(0.25))) {
        pop(s, next);
      }
    }

    for (int i = int(flashes.size()) - 1; i >= 0; i--) {
      flashes[i].age += dt;
      if (flashes[i].age > real(0.25)) flashes.erase(flashes.begin() + i);
    }
    for (int i = int(smoke.size()) - 1; i >= 0; i--) {
      Smoke& p = smoke[i];
      p.age += dt;
      p.x += p.vx * dt;
      p.y += p.vy * dt;
      p.vx *= real(0.97);
      if (p.age > p.life) smoke.erase(smoke.begin() + i);
    }

    for (auto& p : sparkles) {
      p.y += p.vy * dt;
      if (p.y > ROOF_Y) {
        p.y = BEAM_Y + 2;
        p.x = rng.flt() * 64;
      }
    }

    if (dragonX > -60) {
      dragonX += 11 * dt;
      if (dragonX > 110) {
        dragonX = -60;
        dragonWait = 18 + rng.flt() * 15;
      }
    } else {
      dragonWait -= dt;
      if (dragonWait <= 0) dragonX = -6;
    }
  }

  void draw(Canvas& cv, const Ctx& ctx) override {
    real day = daylight(ctx.hourFloat) * real(0.5);
    vgradient(cv, 0, 0, 63, ROOF_Y, mix(COL::skyTop, COL::dayTop, day), mix(COL::skyBottom, COL::dayBottom, day));
    drawStars(cv, t, real(0.5) * (1 - day * 2), 0, 0, 63, 40, 18, 0xffe0c0);

    fw.draw(cv);

    // carved beam under the clock
    cv.rect(0, BEAM_Y, 63, BEAM_Y + 1, COL::beam);
    cv.rect(0, BEAM_Y + 1, 63, BEAM_Y + 1, COL::beamShade);
    for (int x = 2; x < 64; x += 6) cv.set(x, BEAM_Y, COL::trim);

    drawLanterns(cv);
    drawCrackers(cv);
    drawBranch(cv);
    drawRoof(cv);
    drawDragon(cv);

    for (const auto& p : sparkles) {
      real tw = real(0.5) + real(0.5) * std::sin(t * 5 + p.ph);
      cv.blend(p.x + std::sin(t + p.ph) * real(0.8), p.y, tw > real(0.85) ? COL::goldLight : COL::gold,
               real(0.35) + real(0.6) * tw);
    }

    drawClock(cv, ctx, COL::clock, COL::clockShadow);
  }

 private:
  struct Queued {
    int s, i;
    real delay;
  };
  struct Flash {
    int x, y;
    real age;
  };
  struct Smoke {
    real x, y, vx, vy, age, life;
  };
  struct Sparkle {
    real x, y, vy, ph;
  };

  Rng& rng;
  Fireworks fw;
  real t = 0;
  std::vector<bool> crackers[STRING_COUNT];  // spent flags per string
  real reload[STRING_COUNT] = {0, 0};
  std::vector<Queued> queue;
  std::vector<Flash> flashes;
  std::vector<Smoke> smoke;
  std::vector<Sparkle> sparkles;
  real dragonX = -60;  // head position; off screen when idle
  real dragonWait = 6;

  static void crackerPos(int s, int i, int& x, int& y) {
    x = STRINGS[s].x + (i % 2 == 0 ? -1 : 1);
    y = STRINGS[s].y + 1 + i * 3;
  }

  void pop(int s, int i) {
    if (crackers[s][i]) return;
    crackers[s][i] = true;
    int x, y;
    crackerPos(s, i, x, y);
    flashes.push_back({x, y, 0});
    for (int k = 0; k < 3; k++) {
      Smoke p;
      p.x = x + rng.flt() * 2 - 1;
      p.y = y;
      p.vx = (rng.flt() - real(0.5)) * 4;
      p.vy = -3 - rng.flt() * 3;
      p.age = 0;
      p.life = real(1.4) + rng.flt();
      smoke.push_back(p);
    }
  }

  void drawLanterns(Canvas& cv) {
    for (const auto& L : LANTERNS) {
      real sway = std::sin(t * real(1.1) + L.ph) * real(1.4);
      real lx = L.hx + jsround(sway);  // lantern centre
      int ly = BEAM_Y + 2 + L.len;     // lantern top
      // string, leaning toward the lantern
      for (int y = BEAM_Y + 2; y < ly; y++) {
        real k = real(y - BEAM_Y - 2) / (L.len > 1 ? L.len : 1);
        cv.set(L.hx + jsround(sway * k), y, COL::string);
      }
      // soft glow
      real flick = real(0.85) + real(0.15) * std::sin(t * 7 + L.ph * 3);
      for (int dy = -9; dy <= 9; dy++) {
        for (int dx = -10; dx <= 10; dx++) {
          real d = std::hypot(real(dx), dy * real(1.1));
          if (d < 10) cv.blend(lx + dx, ly + 5 + dy, COL::glow, real(0.28) * (1 - d / 10) * flick);
        }
      }
      cv.sprite(LANTERN, lx - 4, ly, LANTERN_PAL);
      // lit centre
      cv.rect(lx, ly + 4, lx, ly + 5, mix(COL::redLight, COL::goldLight, real(0.5) * flick));
    }
  }

  void drawCrackers(Canvas& cv) {
    for (int s = 0; s < STRING_COUNT; s++) {
      const StringDef& S = STRINGS[s];
      cv.rect(S.x, S.y, S.x, S.y + S.n * 3, COL::string);
      for (int i = 0; i < S.n; i++) {
        if (crackers[s][i]) continue;
        int cx, cy;
        crackerPos(s, i, cx, cy);
        int left = cx < S.x ? cx - 1 : cx;
        cv.rect(left, cy, left + 1, cy + 1, COL::red);
        cv.set(left + (cx < S.x ? 0 : 1), cy + 1, COL::redDark);
        cv.rect(left, cy, left + 1, cy, COL::gold);
      }
      // a small gold knot where the string hangs
      cv.set(S.x, S.y, COL::trim);
    }
    static const int RING1[4][2] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};
    static const int RING2[8][2] = {{2, 0}, {-2, 0}, {0, 2}, {0, -2}, {1, 1}, {-1, -1}, {1, -1}, {-1, 1}};
    for (const auto& f : flashes) {
      real k = 1 - f.age / real(0.25);
      cv.blend(f.x, f.y, 0xffffff, k);
      for (const auto& d : RING1) cv.blend(f.x + d[0], f.y + d[1], 0xffd060, k * real(0.9));
      for (const auto& d : RING2) cv.blend(f.x + d[0], f.y + d[1], 0xff6a20, k * real(0.5));
    }
    for (const auto& p : smoke) cv.blend(p.x, p.y, COL::smoke, real(0.45) * (1 - p.age / p.life));
  }

  void drawBranch(Canvas& cv) {
    for (const auto& b : BRANCH) cv.set(b[0], b[1], COL::branch);
    int i = 0;
    for (const auto& b : BLOSSOMS) {
      int x = b[0], y = b[1];
      uint32_t c = i % 3 == 0 ? COL::blossomDeep : COL::blossom;
      cv.set(x - 1, y, c);
      cv.set(x + 1, y, c);
      cv.set(x, y - 1, c);
      cv.set(x, y + 1, c);
      cv.set(x, y, COL::blossomCenter);
      i++;
    }
  }

  void drawRoof(Canvas& cv) {
    for (int y = ROOF_Y; y < 64; y++)
      for (int x = 0; x < 64; x++) cv.set(x, y, (x + (y % 2)) % 3 == 0 ? COL::roofAlt : COL::roof);
    cv.rect(0, ROOF_Y, 63, ROOF_Y, COL::trim);
    for (int x = 1; x < 64; x += 3) cv.set(x, ROOF_Y + 1, COL::gold);  // tile ends
    // upturned eave ends
    cv.set(0, ROOF_Y - 1, COL::trim);
    cv.set(63, ROOF_Y - 1, COL::trim);
    cv.set(0, ROOF_Y - 2, COL::trim);
    cv.set(63, ROOF_Y - 2, COL::trim);
  }

  void drawDragon(Canvas& cv) {
    if (dragonX <= -60) return;
    const int segs = 14;
    for (int i = segs; i >= 1; i--) {
      real x = dragonX - i * 3;
      if (x < -3 || x > 66) continue;
      real y = 49 + std::sin(x * real(0.28) - t * 5) * real(2.2);
      bool tail = i == segs;
      uint32_t scale = i % 2 == 0 ? COL::red : COL::redLight;
      real rx = jsround(x), ry = jsround(y);
      cv.rect(rx - 1, ry - 1, rx + 1, ry, tail ? COL::gold : scale);
      cv.set(x, y + 1, COL::gold);                // belly
      if (i % 2 == 0) cv.set(x, y - 2, COL::gold);  // spines
    }
    real hy = jsround(49 + std::sin(dragonX * real(0.28) - t * 5) * real(2.2)) - 2;
    cv.sprite(DRAGON_HEAD, jsround(dragonX) - 1, hy, DRAGON_PAL);
    // whisker
    cv.set(dragonX + 6, hy + 4 + jsround(std::sin(t * 6)), COL::goldLight);
  }
};

}  // namespace

std::unique_ptr<Theme> createLunarNewYear(Rng& rng) { return std::unique_ptr<Theme>(new LunarNewYear(rng)); }

}  // namespace pc
