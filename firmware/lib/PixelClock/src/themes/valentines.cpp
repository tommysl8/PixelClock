// Valentine's Day. Port of simulator/themes/valentines.js; keep the two in sync.
#include <cstring>

#include "../themes.h"

namespace pc {
namespace {

namespace COL {
constexpr uint32_t skyTopDay = 0x4e1838, skyBotDay = 0x8e3a64, skyTopNight = 0x1a0818, skyBotNight = 0x48163a,
                   bokeh = 0xff9cc0, heart = 0xe0284a, heartLight = 0xff6b8a, heartShine = 0xffd0dc,
                   heartDark = 0x9c1434, heartEdge = 0x4a0820, glow = 0xff5c8a, branch = 0x3e1e22,
                   branchTop = 0x6e3c3c, branchDark = 0x2a1216, petal = 0xffc2d6, petalCenter = 0xffe066,
                   sparkle = 0xfff0f6, clock = 0xffe6ef, clockShadow = 0x2a0716;
}

const uint32_t HEART_COLORS[] = {0xff4d6d, 0xff8fb1, 0xe0284a, 0xffc2d6, 0xc2185b};
constexpr int HEART_COLOR_COUNT = 5;
const uint32_t BURST_COLORS[] = {0xffffff, 0xffd0dc, 0xff8fb1};

// Lovebirds, facing right, and the mirrored copy for the other bird.
const char* const BIRD[] = {
    "....OOO..", "...OOOOEK", "..OOOOOO.", "DDDDOOOL.", ".DDDDOLL.", "..DDDLL..", "....F.F..",
};
const char* const BIRD_L[] = {
    "..OOO....", "KEOOOO...", ".OOOOOO..", ".LOOODDDD", ".LLODDDD.", "..LLDDD..", "..F.F....",
};
const Palette BIRD_A{{'O', 0xff6f9a}, {'D', 0xc23c6c}, {'L', 0xffb8cc},
                     {'E', 0x2a0a18}, {'K', 0xffb347}, {'F', 0x2a1216}};
const Palette BIRD_B{{'O', 0xf4e4ee}, {'D', 0xc99ab6}, {'L', 0xffffff},
                     {'E', 0x2a0a18}, {'K', 0xffb347}, {'F', 0x2a1216}};

const char* const SMALL_HEART[] = {"X.X", "XXX", ".X."};
const char* const MID_HEART[] = {"XX.XX", "XXXXX", ".XXX.", "..X.."};

// Branch rises gently from left to right; top row at column x (x >= 0).
inline int branchY(int x) { return 58 - (x * 5) / 63; }

constexpr int HX0 = 12, HX1 = 51, HY0 = 12, HY1 = 47;
constexpr int HW = HX1 - HX0 + 1, HH = HY1 - HY0 + 1;

class Valentines : public Theme {
 public:
  explicit Valentines(Rng& r) : rng(r), kissPal{{'X', 0}} {
    kissTimer = 5 + rng.flt() * 4;
    floaters.reserve(64);
    sparkles.reserve(16);
    for (int i = 0; i < 6; i++) {
      real y = 20 + rng.flt() * 40;  // the argument is evaluated before the call
      spawnFloater(false, 0, true, y, false, 0, false, 0);
    }
  }

  void update(real dt, const Ctx& ctx) override {
    t += dt;

    // Heartbeat: two quick bumps ("lub-dub") every 1.1 seconds.
    real p = jsmod(t, real(1.1)) / real(1.1);
    auto bump = [&](real c, real w) {
      real q = (p - c) / w;
      return std::exp(-(q * q));
    };
    beat = bump(real(0.05), real(0.05)) + real(0.65) * bump(real(0.22), real(0.05));

    if (rng.chance(real(1.3) * dt) && floaters.size() < 20) spawnFloater(false, 0, false, 0, false, 0, false, 0);
    for (int i = int(floaters.size()) - 1; i >= 0; i--) {
      Floater& f = floaters[i];
      f.age += dt;
      f.vx *= real(0.97);
      f.x += (f.vx + std::sin(t * real(1.6) + f.ph) * real(2.2)) * dt;
      f.y += f.vy * dt;
      if (f.vy < -3) f.vy += 6 * dt;
      if (f.y < 12 || f.age > f.life) floaters.erase(floaters.begin() + i);
    }

    if (rng.chance(3 * dt)) {
      Sparkle s;
      s.x = 2 + rng.intn(60);
      s.y = 15 + rng.intn(36);
      s.age = 0;
      s.life = real(0.5) + rng.flt() * real(0.4);
      sparkles.push_back(s);
    }
    for (int i = int(sparkles.size()) - 1; i >= 0; i--) {
      sparkles[i].age += dt;
      if (sparkles[i].age > sparkles[i].life) sparkles.erase(sparkles.begin() + i);
    }

    kissTimer -= dt;
    if (kiss > 0) kiss -= dt;
    if (kissTimer <= 0) {
      kiss = real(1.4);
      kissTimer = 7 + rng.flt() * 5;
      spawnFloater(true, real(31.5), true, 46, true, 0, true, -5);
    }

    if (ctx.shake) {
      for (int i = 0; i < 26; i++) {
        real a = rng.flt() * kPi * 2;
        real s = 14 + rng.flt() * 16;
        spawnFloater(true, real(31.5), true, 30, true, std::cos(a) * s, true, std::sin(a) * s - 6);
        Floater& f = floaters.back();
        f.big = rng.chance(real(0.6));
        f.color = BURST_COLORS[rng.intn(3)];
        f.life = 4 + rng.flt() * 2;
      }
    }
  }

  void draw(Canvas& cv, const Ctx& ctx) override {
    real day = daylight(ctx.hourFloat);
    drawBackground(cv, day);
    drawBranch(cv);
    drawHeart(cv, day);
    drawBirds(cv);
    drawFloaters(cv);
    drawSparkles(cv);
    drawClock(cv, ctx, COL::clock, COL::clockShadow);
  }

 private:
  struct Floater {
    real x, y, vx, vy, ph;
    bool big;
    uint32_t color;
    real age, life;
  };
  struct Sparkle {
    int x, y;
    real age, life;
  };

  Rng& rng;
  real t = 0;
  real beat = 0;
  uint8_t mask[HW * HH] = {};  // 1 inside, 2 outline, 3 glow
  std::vector<Floater> floaters;
  std::vector<Sparkle> sparkles;
  real kissTimer = 0;
  real kiss = 0;
  Palette kissPal;

  // Mirrors spawnFloater(x, y, vx, vy) with JS `??` defaults, in object-literal order.
  void spawnFloater(bool hasX, real x, bool hasY, real y, bool hasVx, real vx, bool hasVy, real vy) {
    Floater f;
    f.x = hasX ? x : 2 + rng.flt() * 60;
    f.y = hasY ? y : 60;
    f.vx = hasVx ? vx : 0;
    f.vy = hasVy ? vy : -(4 + rng.flt() * 4);
    f.ph = rng.flt() * real(6.283);
    f.big = rng.chance(real(0.3));
    f.color = HEART_COLORS[rng.intn(HEART_COLOR_COUNT)];
    f.age = 0;
    f.life = 7 + rng.flt() * 4;
    floaters.push_back(f);
  }

  void drawBackground(Canvas& cv, real day) {
    uint32_t top = mix(COL::skyTopNight, COL::skyTopDay, day);
    uint32_t bot = mix(COL::skyBotNight, COL::skyBotDay, day);
    vgradient(cv, 0, 0, 63, 63, top, bot);
    for (int i = 0; i < 12; i++) {
      int cx = int(hash(i, 7, 64));
      int cy = 16 + int(hash(i, 9, 44));
      int r = 1 + int(hash(i, 3, 2));
      real a = (real(0.1) + real(0.06) * std::sin(t * real(0.7) + i)) * (real(1.2) - real(0.4) * day);
      for (int y = -r; y <= r; y++)
        for (int x = -r; x <= r; x++)
          if (x * x + y * y <= r * r + real(0.5)) cv.blend(cx + x, cy + y, COL::bokeh, a);
    }
    drawStars(cv, t, (1 - day) * real(0.6), 0, 14, 63, 50, 14, 0xffd6e6);
  }

  void drawBranch(Canvas& cv) {
    for (int x = 0; x < 64; x++) {
      int y = branchY(x);
      cv.set(x, y, COL::branchTop);
      cv.set(x, y + 1, COL::branch);
      cv.set(x, y + 2, x < 40 ? COL::branch : COL::branchDark);
      if (x < 22) cv.set(x, y + 3, COL::branchDark);
    }
    for (int i = 0; i < 5; i++) cv.set(49 + i, branchY(49) - 1 - i, COL::branch);
    for (int i = 0; i < 4; i++) cv.set(8 - i, branchY(8) - 1 - i, COL::branch);
    for (int x : {5, 15, 44, 54, 60}) {
      int y = branchY(x) - (x == 54 ? 6 : x == 5 ? 5 : 1);
      cv.set(x, y - 1, COL::petal);
      cv.set(x - 1, y, COL::petal);
      cv.set(x + 1, y, COL::petal);
      cv.set(x, y + 1, COL::petal);
      cv.set(x, y, COL::petalCenter);
    }
  }

  uint8_t at(int x, int y) const { return (x < 0 || y < 0 || x >= HW || y >= HH) ? 0 : mask[y * HW + x]; }

  void drawHeart(Canvas& cv, real day) {
    const real cx = real(31.5), cy = 30;
    real s = 10 + beat * real(1.5);
    std::memset(mask, 0, sizeof mask);
    for (int y = 0; y < HH; y++) {
      for (int x = 0; x < HW; x++) {
        real nx = (HX0 + x - cx) / s;
        real ny = -(HY0 + y - cy) / s;
        real q = nx * nx + ny * ny - 1;
        if (q * q * q - nx * nx * ny * ny * ny <= 0) mask[y * HW + x] = 1;
      }
    }
    for (int y = 0; y < HH; y++)
      for (int x = 0; x < HW; x++) {
        if (mask[y * HW + x]) continue;
        if (at(x - 1, y) == 1 || at(x + 1, y) == 1 || at(x, y - 1) == 1 || at(x, y + 1) == 1) mask[y * HW + x] = 2;
      }
    for (int y = 0; y < HH; y++)
      for (int x = 0; x < HW; x++) {
        if (mask[y * HW + x]) continue;
        if (at(x - 1, y) == 2 || at(x + 1, y) == 2 || at(x, y - 1) == 2 || at(x, y + 1) == 2) mask[y * HW + x] = 3;
      }

    real glowA = (real(0.18) + real(0.25) * beat) * (real(1.3) - real(0.5) * day);
    for (int y = 0; y < HH; y++) {
      for (int x = 0; x < HW; x++) {
        int m = mask[y * HW + x];
        if (!m) continue;
        int px = HX0 + x;
        int py = HY0 + y;
        if (m == 3) {
          cv.blend(px, py, COL::glow, glowA);
          continue;
        }
        if (m == 2) {
          cv.set(px, py, COL::heartEdge);
          continue;
        }
        real nx = (px - cx) / s;
        real ny = -(py - cy) / s;
        real shade = nx * real(0.55) - ny * real(0.5);
        uint32_t c = COL::heart;
        if (shade > real(0.42)) c = COL::heartDark;
        else if (shade < real(-0.42)) c = COL::heartLight;
        real ax = nx + real(0.48), ay = ny - real(0.5);
        if (ax * ax + ay * ay < real(0.035)) c = COL::heartShine;
        cv.set(px, py, mix(c, 0xffffff, beat * real(0.12)));
      }
    }
  }

  void drawBirds(Canvas& cv) {
    int lean = kiss > 0 ? 1 : 0;
    int bobA = std::sin(t * real(2.2)) > real(0.6) ? 1 : 0;
    int bobB = std::sin(t * real(2.2) + real(2.5)) > real(0.6) ? 1 : 0;
    int ax = 21 + lean;
    int bx = 32 - lean;
    cv.sprite(BIRD, ax, branchY(ax + 5) - 7 + bobA, BIRD_A);
    cv.sprite(BIRD_L, bx, branchY(bx + 3) - 7 + bobB, BIRD_B);
    if (kiss > 0) {
      real k = clamp(kiss / real(1.4), 0, 1);
      kissPal.put('X', mix(0xff4d6d, 0xffc2d6, 1 - k));
      cv.sprite(SMALL_HEART, 30, branchY(31) - 12 - jsround((1 - k) * 3), kissPal);
    }
  }

  void drawFloaters(Canvas& cv) {
    for (const Floater& f : floaters) {
      real fadeTop = clamp((f.y - 13) / 10, 0, 1);
      real fadeAge = clamp((f.life - f.age) / real(1.5), 0, 1);
      real a = real(0.95) * fadeTop * fadeAge;
      const char* const* rows = f.big ? MID_HEART : SMALL_HEART;
      int nrows = f.big ? 4 : 3;
      real ox = jsround(f.x) - (f.big ? 2 : 1);
      real oy = jsround(f.y) - 1;
      for (int y = 0; y < nrows; y++)
        for (int x = 0; rows[y][x]; x++)
          if (rows[y][x] == 'X') cv.blend(ox + x, oy + y, f.color, a);
    }
  }

  void drawSparkles(Canvas& cv) {
    static const int DIRS[4][2] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};
    for (const Sparkle& s : sparkles) {
      real k = std::sin((s.age / s.life) * kPi);
      cv.blend(s.x, s.y, COL::sparkle, k);
      if (k > real(0.6))
        for (const auto& d : DIRS) cv.blend(s.x + d[0], s.y + d[1], COL::sparkle, (k - real(0.6)) * real(1.5));
    }
  }
};

}  // namespace

std::unique_ptr<Theme> createValentines(Rng& rng) { return std::unique_ptr<Theme>(new Valentines(rng)); }

}  // namespace pc
