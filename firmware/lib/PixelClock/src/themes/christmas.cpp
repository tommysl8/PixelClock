// Christmas cabin. Port of simulator/themes/christmas.js; keep the two in sync.
#include <algorithm>
#include <set>

#include "../themes.h"

namespace pc {
namespace {

namespace COL {
constexpr uint32_t wallNight = 0x1a1222, wallDay = 0x3b2d3f, floorNight = 0x2b1a10, floorDay = 0x4a3020,
                   plank = 0x1f120a, rugEdge = 0x8e2f3a, rug = 0x6b1f2a, rugDot = 0xc9a227, frame = 0x6b4428,
                   sill = 0x84593a, snow = 0xeef4ff, snowPile = 0xdfe8f5, moon = 0xf4f1d0, star = 0xcfd8ff,
                   brick = 0x7d2f20, brickAlt = 0x6e2a1c, mortar = 0x4f1d14, hearth = 0x514139, opening = 0x0b0605,
                   mantel = 0x5b3a22, mantelTop = 0x74502f, log = 0x5a3418, logDark = 0x3e2410, ember = 0xff6a1a,
                   leaf = 0x1f6b2e, leafLight = 0x2a8a3c, leafDark = 0x145022, trunk = 0x5a3418, starDim = 0xc9a227,
                   starBright = 0xfff3a0, glow = 0xff8a2a, clock = 0xffe9c4, clockShadow = 0x0c0710;
}

const uint32_t FIRE[] = {0,        0x1a0502, 0x3a0a03, 0x5e1204, 0x861b05, 0xa82806, 0xc93a08,
                         0xe2560c, 0xf07514, 0xf8951f, 0xfcb33a, 0xfed062, 0xffe9a0};
constexpr int MAX_HEAT = int(sizeof(FIRE) / sizeof(FIRE[0])) - 1;

const uint32_t LIGHT_COLORS[] = {0xff4545, 0xffd23f, 0x4fc3ff, 0xff7fd0, 0x7dff8a};
constexpr int LIGHT_COUNT = 5;

struct Box {
  int x0, y0, x1, y1;
};
constexpr Box WIN{2, 17, 17, 34};
constexpr Box SKY_BOX{4, 19, 15, 32};
constexpr int MULLION_X = 9, TRANSOM_Y = 25;
constexpr Box FP{20, 36, 43, 57};
constexpr Box OPEN{25, 42, 38, 57};
constexpr int FB_X = 25, FB_Y = 43, FB_W = 14, FB_H = 14;  // firebox
constexpr int FLOOR_Y = 58;
constexpr int TREE_X = 55;
const int TREE_TIERS[3][4] = {{18, 29, 1, 5}, {27, 39, 3, 6}, {37, 50, 4, 7}};

int treeHalfWidth(int y) {
  int hw = -1;
  for (const auto& tr : TREE_TIERS) {
    int a = tr[0], b = tr[1], s = tr[2], e = tr[3];
    if (y >= a && y <= b) hw = std::max(hw, s + ((y - a) * (e - s)) / (b - a));
  }
  return hw;
}

const int STARS[][2] = {{5, 20}, {7, 23}, {15, 24}, {6, 28}, {11, 23}, {14, 29}, {12, 31}};

const char* const CAT_A[] = {"......O.O", "..OSOSOOO", ".OOOOOOdO", "TTOOOOOOO"};
const char* const CAT_B[] = {"......O.O", ".OOSOSOOO", ".OOOOOOdO", "TTOOOOOOO"};
const char* const CAT_TAIL_UP[] = {"......O.O", "..OSOSOOO", "TOOOOOOdO", ".TOOOOOOO"};
const Palette CAT_PAL{{'O', 0xd9822b}, {'S', 0xa35a17}, {'d', 0x4a2a12}, {'T', 0xc0701f}};

const char* const STOCKING[] = {"WW.", "RR.", "RR.", "RRR"};
const Palette STOCKING_RED{{'W', 0xf2f2f2}, {'R', 0xc0392b}};
const Palette STOCKING_GREEN{{'W', 0xf2f2f2}, {'R', 0x2e8b57}};

class Christmas : public Theme {
 public:
  explicit Christmas(Rng& r) : rng(r) {
    for (int i = 0; i < 10; i++) flakes.push_back(newFlake(true));

    std::set<int> taken;
    for (int tries = 0; lights.size() < 18 && tries < 400; tries++) {
      int y = 20 + rng.intn(30);
      int hw = treeHalfWidth(y);
      if (hw < 1) continue;
      int x = TREE_X - hw + 1 + rng.intn(2 * hw - 1);
      int key = x * 100 + y;
      if (taken.count(key) || taken.count(key + 1) || taken.count(key - 1) || taken.count(key + 100) ||
          taken.count(key - 100))
        continue;
      taken.insert(key);
      Light l;
      l.x = x;
      l.y = y;
      l.c = rng.intn(LIGHT_COUNT);
      l.on = rng.chance(real(0.6));
      lights.push_back(l);
    }
    std::sort(lights.begin(), lights.end(),
              [](const Light& a, const Light& b) { return a.y != b.y ? a.y < b.y : a.x < b.x; });
  }

  void update(real dt, const Ctx& ctx) override {
    t += dt;
    stepFire();

    for (auto& f : flakes) {
      f.y += f.v * dt;
      if (f.y > SKY_BOX.y1 - real(0.5)) f = newFlake(false);
    }

    modeTimer += dt;
    if (modeTimer > 12) {
      modeTimer = 0;
      lightMode = (lightMode + 1) % 3;
    }
    if (lightMode == 0)
      for (auto& l : lights)
        if (rng.chance(real(0.04))) l.on = !l.on;

    if (catTail > 0) catTail -= dt;
    else if (rng.chance(real(0.004))) catTail = real(0.35);

    if (ctx.shake) spawnGlobe();
    for (int i = int(globe.size()) - 1; i >= 0; i--) {
      Particle& p = globe[i];
      p.age += dt;
      p.vx *= real(0.96);
      p.vy = jsmin(p.vy + 14 * dt, 7);
      p.x += (p.vx + std::sin(t * 2 + p.ph) * 3) * dt;
      p.y += p.vy * dt;
      if (p.y > 63) {
        p.y = 63;
        p.vy = 0;
      }
      if (p.age > p.life) globe.erase(globe.begin() + i);
    }
  }

  void draw(Canvas& cv, const Ctx& ctx) override {
    real hf = ctx.hourFloat;
    real day = daylight(hf);
    drawRoom(cv, day);
    drawWindow(cv, hf, day);
    drawFireplace(cv);
    drawTree(cv);
    drawCat(cv);
    drawGlow(cv, day);
    drawGlobe(cv);
    drawClock(cv, ctx, COL::clock, COL::clockShadow);
  }

 private:
  struct Flake {
    real bx, y, v, ph;
  };
  struct Light {
    int x, y, c;
    bool on;
  };
  struct Particle {
    real x, y, vx, vy, life, age, ph;
  };

  Rng& rng;
  uint8_t heat[FB_W * FB_H] = {};
  real fireLevel = real(0.8);
  int flare = 0;
  real t = 0;
  std::vector<Flake> flakes;
  std::vector<Light> lights;
  int lightMode = 0;  // 0 twinkle, 1 chase, 2 breathe
  real modeTimer = 0;
  std::vector<Particle> globe;
  real catTail = 0;

  Flake newFlake(bool anywhere) {
    Flake f;
    f.bx = SKY_BOX.x0 + rng.flt() * (SKY_BOX.x1 - SKY_BOX.x0 + 1);
    f.y = anywhere ? SKY_BOX.y0 + rng.flt() * (SKY_BOX.y1 - SKY_BOX.y0) : real(SKY_BOX.y0 - 1);
    f.v = real(2.5) + rng.flt() * real(3.5);
    f.ph = rng.flt() * real(6.283);
    return f;
  }

  void stepFire() {
    const int w = FB_W, h = FB_H;
    if (flare > 0) flare--;
    else if (rng.chance(real(0.01))) flare = 6 + rng.intn(8);

    for (int x = 0; x < w; x++) {
      real edge = std::fabs(x - real(w - 1) / 2) / (real(w - 1) / 2);
      int base = int(jsround(MAX_HEAT - edge * 5)) + (flare > 0 ? 1 : 0);
      heat[(h - 1) * w + x] = uint8_t(clamp(base - rng.intn(3), 0, MAX_HEAT));
    }
    for (int y = 0; y < h - 1; y++) {
      for (int x = 0; x < w; x++) {
        int sx = int(clamp(x + rng.intn(3) - 1, 0, w - 1));
        int below = heat[(y + 1) * w + sx];
        int cool = rng.intn(3) == 0 ? 0 : rng.chance(real(0.25)) ? 2 : 1;
        heat[y * w + x] = uint8_t(std::max(0, below - cool));
      }
    }
    int sum = 0;
    for (int y = h - 5; y < h; y++)
      for (int x = 0; x < w; x++) sum += heat[y * w + x];
    real level = real(sum) / (5 * w * MAX_HEAT);
    fireLevel += (level - fireLevel) * real(0.25);
  }

  void spawnGlobe() {
    for (int i = 0; i < 160; i++) {
      Particle p;
      p.x = rng.flt() * 64;
      p.y = rng.flt() * 64;
      p.vx = (rng.flt() - real(0.5)) * 50;
      p.vy = -10 - rng.flt() * 30;
      p.life = 6 + rng.flt() * 4;
      p.age = 0;
      p.ph = rng.flt() * real(6.283);
      globe.push_back(p);
    }
  }

  void drawRoom(Canvas& cv, real day) {
    uint32_t wall = mix(COL::wallNight, COL::wallDay, day * real(0.7));
    uint32_t stripe = dim(wall, real(0.82));
    for (int y = 0; y < FLOOR_Y; y++)
      for (int x = 0; x < 64; x++) cv.set(x, y, x % 6 == 0 ? stripe : wall);
    uint32_t floor = mix(COL::floorNight, COL::floorDay, day * real(0.6));
    cv.rect(0, FLOOR_Y, 63, 63, floor);
    cv.rect(0, 61, 63, 61, COL::plank);
    for (int x = 5; x < 64; x += 11) cv.set(x, 59 + ((x >> 2) & 1), COL::plank);

    cv.rect(3, 59, 18, 62, COL::rugEdge);
    cv.rect(4, 60, 17, 61, COL::rug);
    for (int x = 5; x < 17; x += 3) cv.set(x, 60 + ((x / 3) & 1), COL::rugDot);
  }

  void drawWindow(Canvas& cv, real hf, real day) {
    SkyColors sky = skyAt(hf);
    cv.rect(WIN.x0, WIN.y0, WIN.x1, WIN.y1, COL::frame);
    for (int y = SKY_BOX.y0; y <= SKY_BOX.y1; y++) {
      uint32_t c = mix(sky.top, sky.bottom, real(y - SKY_BOX.y0) / (SKY_BOX.y1 - SKY_BOX.y0));
      cv.rect(SKY_BOX.x0, y, SKY_BOX.x1, y, c);
    }

    real night = 1 - day;
    if (night > real(0.3)) {
      int i = 0;
      for (const auto& s : STARS) {
        real tw = real(0.5) + real(0.5) * std::sin(t * real(2.3) + i * real(1.7));
        cv.blend(s[0], s[1], COL::star, (night - real(0.3)) * tw);
        i++;
      }
      real a = jsmin(1, (night - real(0.3)) * 2);
      cv.rect(12, 20, 14, 22, mix(sky.top, COL::moon, a));
      const int corners[4][2] = {{12, 20}, {14, 20}, {12, 22}, {14, 22}};
      for (const auto& c : corners) cv.set(c[0], c[1], mix(sky.top, COL::moon, a * real(0.45)));
    }

    uint32_t flakeCol = mix(0xb9c6e0, COL::snow, day);
    for (const auto& f : flakes) {
      real x = f.bx + std::sin(t * real(1.5) + f.ph) * real(0.8);
      if (x >= SKY_BOX.x0 && x <= SKY_BOX.x1 + real(0.49)) cv.set(x, f.y, flakeCol);
    }

    cv.rect(SKY_BOX.x0, SKY_BOX.y1, SKY_BOX.x1, SKY_BOX.y1, COL::snowPile);
    for (int x : {4, 5, 12, 13, 14}) cv.set(x, SKY_BOX.y1 - 1, COL::snowPile);

    cv.rect(MULLION_X, SKY_BOX.y0, MULLION_X + 1, SKY_BOX.y1, COL::frame);
    cv.rect(SKY_BOX.x0, TRANSOM_Y, SKY_BOX.x1, TRANSOM_Y + 1, COL::frame);
    cv.rect(WIN.x0 - 1, WIN.y1 + 1, WIN.x1 + 1, WIN.y1 + 1, COL::sill);
  }

  void drawFireplace(Canvas& cv) {
    for (int y = FP.y0; y <= FP.y1; y++) {
      int row = (y - FP.y0) / 3;
      for (int x = FP.x0; x <= FP.x1; x++) {
        bool mortar = (y - FP.y0) % 3 == 0 || (x - FP.x0 + (row % 2) * 3) % 6 == 0;
        uint32_t shade = (((x - FP.x0 + (row % 2) * 3) / 6 + row) % 3) == 0 ? COL::brickAlt : COL::brick;
        cv.set(x, y, mortar ? COL::mortar : shade);
      }
    }
    cv.rect(OPEN.x0, OPEN.y0, OPEN.x1, OPEN.y1, COL::opening);
    cv.set(OPEN.x0, OPEN.y0, COL::brick);
    cv.set(OPEN.x1, OPEN.y0, COL::brick);

    for (int y = 0; y < FB_H; y++)
      for (int x = 0; x < FB_W; x++) {
        int v = heat[y * FB_W + x];
        if (v > 0) cv.set(FB_X + x, FB_Y + y, FIRE[v]);
      }

    cv.rect(26, 56, 37, 56, COL::log);
    cv.rect(28, 55, 35, 55, COL::logDark);
    for (int x : {27, 30, 33, 36})
      if (rng.chance(real(0.7))) cv.set(x, 56, mix(COL::log, COL::ember, real(0.5) + rng.flt() * real(0.5)));

    cv.rect(FP.x0 - 1, FLOOR_Y, FP.x1 + 1, FLOOR_Y, COL::hearth);

    cv.rect(19, 34, 44, 35, COL::mantel);
    cv.rect(19, 34, 44, 34, COL::mantelTop);
    cv.rect(19, 33, 44, 33, COL::leafDark);
    for (int x = 20, i = 0; x <= 43; x += 4, i++) {
      bool on = (long(std::floor(t * real(1.5))) + i) % 2 == 0;
      cv.set(x, 33, on ? LIGHT_COLORS[i % 2 == 0 ? 0 : 1] : COL::leaf);
    }

    cv.sprite(STOCKING, 22, 36, STOCKING_RED);
    cv.sprite(STOCKING, 39, 36, STOCKING_GREEN);
  }

  void drawTree(Canvas& cv) {
    for (int y = 18; y <= 50; y++) {
      int hw = treeHalfWidth(y);
      if (hw < 0) continue;
      for (int x = TREE_X - hw; x <= TREE_X + hw; x++) {
        uint32_t c = x < TREE_X ? COL::leafLight : COL::leaf;
        if ((x * 7 + y * 3) % 5 == 0) c = COL::leafDark;
        if (x == TREE_X + hw) c = COL::leafDark;
        cv.set(x, y, c);
      }
    }
    cv.rect(54, 51, 56, 57, COL::trunk);

    for (int i = 0; i < int(lights.size()); i++) {
      const Light& l = lights[i];
      bool on;
      if (lightMode == 0) on = l.on;
      else if (lightMode == 1) on = (((long(std::floor(t * 8)) - i) % 6) + 6) % 6 < 3;
      else on = true;
      uint32_t c = LIGHT_COLORS[l.c];
      if (lightMode == 2) c = mix(dim(c, real(0.25)), c, real(0.5) + real(0.5) * std::sin(t * 2 + l.c));
      cv.set(l.x, l.y, on ? c : dim(c, real(0.18)));
    }

    real s = real(0.5) + real(0.5) * std::sin(t * 3);
    uint32_t sc = mix(COL::starDim, COL::starBright, s);
    cv.set(TREE_X, 15, sc);
    cv.rect(TREE_X - 1, 16, TREE_X + 1, 16, sc);
    cv.set(TREE_X, 17, sc);
    if (s > real(0.8)) {
      cv.set(TREE_X - 2, 16, dim(sc, real(0.6)));
      cv.set(TREE_X + 2, 16, dim(sc, real(0.6)));
      cv.set(TREE_X, 14, dim(sc, real(0.6)));
    }

    cv.rect(47, 54, 50, 57, 0xb03030);
    cv.rect(49, 54, 49, 57, 0xe8c547);
    cv.rect(47, 55, 50, 55, 0xe8c547);
    cv.set(48, 53, 0xe8c547);
    cv.set(50, 53, 0xe8c547);
    cv.rect(59, 53, 62, 57, 0x2f5fa8);
    cv.rect(61, 53, 61, 57, 0xf2f2f2);
    cv.rect(59, 55, 62, 55, 0xf2f2f2);
    cv.set(60, 52, 0xf2f2f2);
    cv.set(62, 52, 0xf2f2f2);
  }

  void drawCat(Canvas& cv) {
    bool a = long(std::floor(t / real(1.6))) % 2 == 0;
    if (catTail > 0) cv.sprite(CAT_TAIL_UP, 6, 55, CAT_PAL);
    else if (a) cv.sprite(CAT_A, 6, 55, CAT_PAL);
    else cv.sprite(CAT_B, 6, 55, CAT_PAL);
  }

  void drawGlow(Canvas& cv, real day) {
    real strength = fireLevel * real(0.38) * (1 - real(0.55) * day);
    const real cx = real(31.5), cy = 50, r = 32;
    for (int y = 14; y < 64; y++) {
      for (int x = 0; x < 64; x++) {
        if (x >= OPEN.x0 && x <= OPEN.x1 && y >= OPEN.y0 && y <= OPEN.y1) continue;
        if (x >= SKY_BOX.x0 && x <= SKY_BOX.x1 && y >= SKY_BOX.y0 && y <= SKY_BOX.y1) continue;
        real d = std::hypot(x - cx, (y - cy) * real(1.2));
        if (d >= r) continue;
        real k = 1 - d / r;
        cv.blend(x, y, COL::glow, strength * k * k);
      }
    }
  }

  void drawGlobe(Canvas& cv) {
    for (const auto& p : globe) {
      real fade = jsmin(1, (p.life - p.age) / real(1.5));
      cv.blend(p.x, p.y, COL::snow, real(0.9) * fade);
    }
  }
};

}  // namespace

std::unique_ptr<Theme> createChristmas(Rng& rng) { return std::unique_ptr<Theme>(new Christmas(rng)); }

}  // namespace pc
