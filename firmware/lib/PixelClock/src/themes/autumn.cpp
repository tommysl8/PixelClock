// Autumn forest. Port of simulator/themes/autumn.js; keep the two in sync.
#include <algorithm>
#include <cstring>

#include "../themes.h"

namespace pc {
namespace {

// [dark, mid, light, highlight] per canopy colour
const uint32_t CANOPY[3][4] = {
    {0x9a3c12, 0xd2601c, 0xf08a24, 0xffbe5a},  // orange
    {0x6e1712, 0xa8261c, 0xd84a2a, 0xf47a50},  // red
    {0x9a7414, 0xd0a020, 0xf0c838, 0xfff07a},  // yellow
};
const uint32_t LEAF_COLORS[] = {0xf08a24, 0xd84a2a, 0xf0c838, 0xd2601c, 0xa8261c, 0xffbe5a};
constexpr int LEAF_COUNT = 6;

struct Tree {
  int nb;
  int blobs[4][3];  // cx, cy, r
  int trunk[4];     // x0, y0, x1, y1
  int pal;
  bool distant;
};
const Tree TREES[4] = {
    {4, {{7, 28, 8}, {15, 31, 6}, {2, 34, 5}, {11, 22, 5}}, {6, 33, 8, 53}, 0, false},
    {4, {{56, 28, 7}, {61, 33, 5}, {49, 33, 5}, {55, 22, 5}}, {54, 33, 56, 54}, 1, false},
    {2, {{27, 38, 3}, {28, 36, 2}}, {27, 40, 27, 46}, 2, true},
    {2, {{37, 39, 2}, {37, 37, 2}}, {37, 41, 37, 46}, 0, true},
};

constexpr uint32_t NIGHT_TINT = 0x1a1a44;
namespace COL {
constexpr uint32_t trunk = 0x4a2c18, trunkLight = 0x6a4026, hillDay = 0x9a8a6a, hillNight = 0x3a3048,
                   grassFar = 0x9a8c3e, grassNear = 0x5e5c22, path = 0xd0ac7c, pathEdge = 0x9c7a4c,
                   moon = 0xffc46a, moonShade = 0xe8a850, sun = 0xfff2c0, clock = 0xffe2b0,
                   clockShadow = 0x1a0e08;
}

// Squirrel facing right, plus the mirrored rows for running left.
const char* const SQ_STAND[] = {".LT..E.", "LT...HK", "TT.BBHH", ".TBBWB.", "...F.F."};
const char* const SQ_RUN_A[] = {".......", ".LT..E.", "LT.BBHK", "TTBBBHH", "..F...F"};
const char* const SQ_RUN_B[] = {".......", ".LT..E.", "LT.BBHK", "TTBBBHH", "...FF.."};
const char* const SQ_TWITCH[] = {"LT...E.", "LT...HK", ".T.BBHH", ".TBBWB.", "...F.F."};
const char* const SQ_STAND_L[] = {".E..TL.", "KH...TL", "HHBB.TT", ".BWBBT.", ".F.F..."};
const char* const SQ_RUN_A_L[] = {".......", ".E..TL.", "KHBB.TL", "HHBBBTT", "F...F.."};
const char* const SQ_RUN_B_L[] = {".......", ".E..TL.", "KHBB.TL", "HHBBBTT", "..FF..."};
const char* const SQ_TWITCH_L[] = {".E...TL", "KH...TL", "HHBB.T.", ".BWBBT.", ".F.F..."};
namespace SQ {
constexpr uint32_t T = 0x9a5a2a, L = 0xd8a070, B = 0x6a3c1c, H = 0x6a3c1c, W = 0xd8a070, E = 0x4a2810,
                   K = 0x0a0402, F = 0x4a2810;
}

int groundY(int x) { return 46 + int(jsround(std::sin(x * real(0.08) + 2) * real(1.5))); }
int hillY(int x) { return 40 + int(jsround(std::sin(x * real(0.12)) * 2 + std::sin(x * real(0.31) + 1))); }
real pathCenter(int y) { return 30 + std::sin(y * real(0.35)) * 3; }

uint32_t shadeNight(uint32_t c, real day) {
  return dim(mix(c, NIGHT_TINT, real(0.3) * (1 - day)), real(0.55) + real(0.45) * day);
}

class Autumn : public Theme {
 public:
  explicit Autumn(Rng& r) : rng(r) {
    // Same order of random draws as create() in autumn.js.
    gustTimer = 6 + rng.flt() * 6;
    leaves.reserve(12);
    for (int i = 0; i < 12; i++) {
      Leaf l;
      spawnLeaf(l);
      l.y = 18 + rng.flt() * 30;
      leaves.push_back(l);
    }
    flurry.reserve(64);
    sq.timer = 4 + rng.flt() * 6;
  }

  void update(real dt, const Ctx& ctx) override {
    t += dt;

    gustTimer -= dt;
    if (gustTimer <= 0 && gustLeft <= 0) {
      gustLeft = real(1.5) + rng.flt() * real(1.2);
      windTarget = 12 + rng.flt() * 6;
      gustTimer = 7 + rng.flt() * 8;
    }
    if (gustLeft > 0) {
      gustLeft -= dt;
      if (gustLeft <= 0) windTarget = real(1.5);
    }
    wind += (windTarget - wind) * jsmin(1, dt * real(1.5));

    for (Leaf& l : leaves) {
      if (l.landed) {
        l.rest -= dt;
        if (wind > 8 && rng.chance(dt * real(1.5))) {
          l.landed = false;  // gust picks it back up
          l.vy = -4;
        }
        if (l.rest <= 0) spawnLeaf(l);
        continue;
      }
      l.vy = jsmin(l.vy + 4 * dt, 7);
      l.x += (wind + std::sin(t * 2 + l.ph) * 4) * dt;
      l.y += l.vy * dt;
      if (l.y >= l.landY) {
        l.y = real(l.landY);
        l.landed = true;
        l.rest = 2 + rng.flt() * 4;
      }
      if (l.x > 66 || l.x < -3) spawnLeaf(l);
    }

    if (ctx.shake) spawnFlurry();
    for (int i = int(flurry.size()) - 1; i >= 0; i--) {
      Flurry& f = flurry[i];
      f.age += dt;
      f.vx *= real(0.99);
      f.vy += std::sin(t * 3 + f.ph) * 30 * dt + 3 * dt;
      f.x += f.vx * dt;
      f.y += f.vy * dt;
      if (f.age > f.life || f.x > 68) flurry.erase(flurry.begin() + i);
    }

    updateSquirrel(dt);
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
    real night = 1 - day;
    if (night > real(0.2)) {
      for (const Star& s : stars) {
        real tw = real(0.55) + real(0.45) * std::sin(t * (real(1.5) + (s.i % 5) * real(0.4)) + s.i * real(2.1));
        cv.blend(s.x, s.y, 0xdfe6ff, (night - real(0.2)) * tw);
      }
    }

    real amb = real(0.55) + real(0.45) * day;
    real leafAmb = jsmax(amb, real(0.8));  // moving leaves catch the light
    for (const Leaf& l : leaves) {
      uint32_t c = (l.landed || std::sin(t * 6 + l.ph) > 0) ? l.col : l.col2;
      cv.set(l.x, l.y, dim(c, l.landed ? amb : leafAmb));
    }
    drawSquirrel(cv, jsmax(amb, real(0.7)));
    for (const Flurry& f : flurry) {
      uint32_t c = std::sin(t * 8 + f.ph) > 0 ? f.col : f.col2;
      real fade = clamp((f.life - f.age) / 1, 0, 1);
      cv.blend(f.x, f.y, dim(c, leafAmb), fade);
    }

    drawClock(cv, ctx, COL::clock, COL::clockShadow);
  }

 private:
  struct Star {
    int x, y, i;
  };
  struct Leaf {
    real x = 0, y = 0, vy = 0, ph = 0;
    uint32_t col = 0, col2 = 0;
    int landY = 0;
    real rest = 0;
    bool landed = false;
  };
  struct Flurry {
    real x, y, vx, vy, ph;
    uint32_t col, col2;
    real life, age;
  };
  enum class SqState { Away, Run, Stop };
  struct Squirrel {
    SqState state = SqState::Away;
    real timer = 0;
    real x = -8;
    int dir = 1;
    int stopX = 30;
    real frameT = 0;
    bool stopped = false;
  };

  Rng& rng;
  real t = 0;
  Canvas bg;
  long bgKey = -1;
  uint8_t skyMask[64 * 64] = {};
  std::vector<Star> stars;

  real wind = real(1.5);
  real windTarget = real(1.5);
  real gustTimer = 0;
  real gustLeft = 0;
  std::vector<Leaf> leaves;
  std::vector<Flurry> flurry;
  Squirrel sq;

  // ---------- static background, rebuilt every couple of minutes of clock time ----------
  bool canopyPixel(const Tree& tree, int x, int y, uint32_t& out) {
    for (int b = 0; b < tree.nb; b++) {
      int cx = tree.blobs[b][0], cy = tree.blobs[b][1], r = tree.blobs[b][2];
      real rr = r - real(0.4) + hash(x, y, 4) * real(0.3);
      int dx = x - cx;
      int dy = y - cy;
      if (dx * dx + dy * dy <= rr * rr) {
        real light = real(-dx - dy) / r;
        int i = light > real(0.7) ? 3 : light > 0 ? 2 : light > real(-0.6) ? 1 : 0;
        uint32_t n = hash(x * 7, y * 3, 9);
        if (n == 0) i = std::max(0, i - 1);
        else if (n == 1) i = std::min(3, i + 1);
        if (hash(x * 3, y * 5, 13) == 0) i = 0;  // little gaps for depth
        out = CANOPY[tree.pal][i];
        return true;
      }
    }
    return false;
  }

  void buildBackground(real hf) {
    real day = daylight(hf);
    real night = 1 - day;
    SkyColors sky = skyAt(hf);
    vgradient(bg, 0, 0, 63, 47, sky.top, sky.bottom);
    std::memset(skyMask, 0, sizeof(skyMask));
    for (int y = 0; y < 48; y++)
      for (int x = 0; x < 64; x++) skyMask[y * 64 + x] = 1;

    // sun by day, harvest moon by night
    if (day > 0) {
      for (int y = 14; y <= 26; y++) {
        for (int x = 34; x <= 46; x++) {
          real d = std::hypot(real(x - 40), real(y - 20));
          if (d <= real(2.6)) bg.set(x, y, mix(bg.get(x, y), COL::sun, day));
          else if (d < 6) bg.blend(x, y, COL::sun, day * real(0.25) * (1 - (d - real(2.6)) / real(3.4)));
        }
      }
    }
    if (night > 0) {
      for (int y = 12; y <= 32; y++) {
        for (int x = 30; x <= 54; x++) {
          real d = std::hypot(real(x - 42), real(y - 22));
          if (d <= real(6.2)) {
            uint32_t c = COL::moon;
            if ((x == 40 && y == 20) || (x == 44 && y == 24) || (x == 45 && y == 19) || (x == 39 && y == 25))
              c = COL::moonShade;
            if (x - 42 + (y - 22) > 5) c = COL::moonShade;
            bg.set(x, y, mix(bg.get(x, y), c, night));
          } else if (d < 11) {
            bg.blend(x, y, 0xff9a40, night * real(0.22) * (1 - (d - real(6.2)) / real(4.8)));
          }
        }
      }
    }

    // far hills
    uint32_t hill = mix(COL::hillNight, COL::hillDay, day);
    for (int x = 0; x < 64; x++) {
      for (int y = hillY(x); y < 48; y++) {
        bg.set(x, y, hash(x, y, 6) == 0 ? dim(hill, real(0.9)) : hill);
        skyMask[y * 64 + x] = 0;
      }
    }

    // trees: far ones first, then the two big ones
    const int order[4] = {2, 3, 0, 1};
    for (int ti : order) {
      const Tree& tree = TREES[ti];
      int tx0 = tree.trunk[0], ty0 = tree.trunk[1], tx1 = tree.trunk[2], ty1 = tree.trunk[3];
      auto tint = [&](uint32_t c) { return shadeNight(tree.distant ? mix(c, hill, real(0.35)) : c, day); };
      for (int y = ty0; y <= ty1; y++) {
        for (int x = tx0; x <= tx1; x++) {
          bg.set(x, y, tint(x == tx0 ? COL::trunkLight : COL::trunk));
          skyMask[y * 64 + x] = 0;
        }
      }
      if (!tree.distant) {
        // a couple of branches poking out
        bg.set(tx0 - 1, ty0 + 3, tint(COL::trunk));
        bg.set(tx1 + 1, ty0 + 5, tint(COL::trunk));
      }
      int minX = 64, maxX = 0, minY = 64, maxY = 0;
      for (int b = 0; b < tree.nb; b++) {
        int cx = tree.blobs[b][0], cy = tree.blobs[b][1], r = tree.blobs[b][2];
        minX = std::min(minX, cx - r - 1);
        maxX = std::max(maxX, cx + r + 1);
        minY = std::min(minY, cy - r - 1);
        maxY = std::max(maxY, cy + r + 1);
      }
      for (int y = std::max(14, minY); y <= maxY; y++) {
        for (int x = std::max(0, minX); x <= std::min(63, maxX); x++) {
          uint32_t c;
          if (canopyPixel(tree, x, y, c)) {
            bg.set(x, y, tint(c));
            skyMask[y * 64 + x] = 0;
          }
        }
      }
    }

    // ground, path, scattered leaves
    for (int x = 0; x < 64; x++) {
      for (int y = groundY(x); y < 64; y++) {
        bool isTrunk = false;
        for (const Tree& tr : TREES)
          if (x >= tr.trunk[0] && x <= tr.trunk[2] && y <= tr.trunk[3] && y >= tr.trunk[1]) {
            isTrunk = true;
            break;
          }
        if (isTrunk) continue;
        real depth = real(y - 44) / 19;  // 0 far .. 1 near
        uint32_t c = mix(COL::grassFar, COL::grassNear, depth);
        if (hash(x, y, 7) == 0) c = dim(c, real(0.85));
        real pcx = pathCenter(y);
        real half = real(0.5) + depth * 5;
        real d = std::fabs(x - pcx);
        if (d <= half - 1) c = mix(COL::path, COL::pathEdge, hash(x, y * 2, 5) == 0 ? real(0.6) : depth * real(0.3));
        else if (d <= half) c = COL::pathEdge;
        else if (hash(x * 5, y * 7, 19) == 0) c = LEAF_COLORS[hash(x, y, LEAF_COUNT)];
        bg.set(x, y, shadeNight(c, day));
        skyMask[y * 64 + x] = 0;
      }
    }

    // leaf pile
    for (int x = 10; x <= 24; x++) {
      real k = real(x - 17) / real(7.5);
      int h = int(jsround(5 * (1 - k * k)));
      for (int y = 58 - h; y <= 58; y++) {
        uint32_t base = LEAF_COLORS[hash(x * 3, y * 11, 6)];
        uint32_t lit = y == 58 - h ? mix(base, 0xffe0a0, real(0.25)) : y == 58 ? dim(base, real(0.7)) : base;
        bg.set(x, y, shadeNight(lit, day));
      }
    }

    // star positions: only in open sky
    stars.clear();
    for (int i = 0; i < 26; i++) {
      int x = int(hash(i, 17, 64));
      int y = int(hash(i, 29, 36));
      if (skyMask[y * 64 + x] && !(x >= 30 && x <= 54 && y >= 12 && y <= 32)) stars.push_back({x, y, i});
    }
  }

  // ---------- falling leaves ----------
  void spawnLeaf(Leaf& leaf) {
    const Tree& tree = TREES[rng.intn(2)];
    const int* blob = tree.blobs[rng.intn(tree.nb)];
    int cx = blob[0], cy = blob[1], r = blob[2];
    uint32_t c = LEAF_COLORS[rng.intn(LEAF_COUNT)];
    leaf.x = cx + (rng.flt() - real(0.5)) * r * real(1.6);
    leaf.y = cy + (rng.flt() - real(0.3)) * r;
    leaf.vy = real(3.5) + rng.flt() * real(3.5);
    leaf.ph = rng.flt() * real(6.283);
    leaf.col = c;
    leaf.col2 = dim(c, real(0.65));
    leaf.landY = 50 + rng.intn(12);
    leaf.rest = 0;
    leaf.landed = false;
  }

  void spawnFlurry() {
    for (int i = 0; i < 48; i++) {
      uint32_t c = LEAF_COLORS[rng.intn(LEAF_COUNT)];
      Flurry f;
      f.x = -6 + rng.flt() * 52;
      f.y = 16 + rng.flt() * 44;
      f.vx = 10 + rng.flt() * 16;
      f.vy = (rng.flt() - real(0.6)) * 18;
      f.ph = rng.flt() * real(6.283);
      f.col = c;
      f.col2 = dim(c, real(0.65));
      f.life = 3 + rng.flt() * 2;
      f.age = 0;
      flurry.push_back(f);
    }
    windTarget = 26;
    gustLeft = 3;
  }

  // ---------- squirrel ----------
  void updateSquirrel(real dt) {
    sq.frameT += dt;
    if (sq.state == SqState::Away) {
      sq.timer -= dt;
      if (sq.timer <= 0) {
        sq.dir = rng.chance(real(0.5)) ? 1 : -1;
        sq.x = sq.dir > 0 ? -8 : 64;
        sq.stopX = 27 + rng.intn(18);  // stop on or near the path, clear of the leaf pile
        sq.state = SqState::Run;
        sq.stopped = false;
      }
    } else if (sq.state == SqState::Run) {
      sq.x += sq.dir * 22 * dt;
      if (!sq.stopped && ((sq.dir > 0 && sq.x >= sq.stopX) || (sq.dir < 0 && sq.x <= sq.stopX))) {
        sq.state = SqState::Stop;
        sq.timer = real(1.5) + rng.flt() * 2;
        sq.stopped = true;
      }
      if (sq.x < -10 || sq.x > 66) {
        sq.state = SqState::Away;
        sq.timer = 8 + rng.flt() * 10;
      }
    } else {
      sq.timer -= dt;
      if (sq.timer <= 0) sq.state = SqState::Run;
    }
  }

  void drawSquirrel(Canvas& cv, real amb) {
    if (sq.state == SqState::Away) return;
    const char* const* rows;
    bool left = sq.dir < 0;
    if (sq.state == SqState::Run) {
      bool a = long(std::floor(sq.frameT * 12)) % 2 != 0;
      rows = a ? (left ? SQ_RUN_A_L : SQ_RUN_A) : (left ? SQ_RUN_B_L : SQ_RUN_B);
    } else {
      bool tw = long(std::floor(sq.frameT * 3)) % 4 == 0;
      rows = tw ? (left ? SQ_TWITCH_L : SQ_TWITCH) : (left ? SQ_STAND_L : SQ_STAND);
    }
    Palette pal{{'T', dim(SQ::T, amb)}, {'L', dim(SQ::L, amb)}, {'B', dim(SQ::B, amb)}, {'H', dim(SQ::H, amb)},
                {'W', dim(SQ::W, amb)}, {'E', dim(SQ::E, amb)}, {'K', dim(SQ::K, amb)}, {'F', dim(SQ::F, amb)}};
    cv.sprite(rows, 5, jsround(sq.x), 56, pal);
  }
};

}  // namespace

std::unique_ptr<Theme> createAutumn(Rng& rng) { return std::unique_ptr<Theme>(new Autumn(rng)); }

}  // namespace pc
