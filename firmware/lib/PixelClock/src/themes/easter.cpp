// Easter. Port of simulator/themes/easter.js; keep the two in sync.
#include "../themes.h"

namespace pc {
namespace {

namespace COL {
constexpr uint32_t pastel = 0xe4d6f5, sun = 0xfff2a8, sunGlow = 0xffe9a0, moon = 0xf4f1d0, cloud = 0xffffff,
                   cloudShade = 0xc6cce6, hillBack = 0x9edb93, meadow = 0x78c96f, grassDark = 0x5aae55,
                   grassLight = 0xa8e69a, stem = 0x2e7a35, clock = 0xffffff, clockShadow = 0x3a2a5a;
}

// Bunny, facing right. 11 wide, 10 tall; feet on the bottom row.
const char* const BUNNY_SIT[] = {
    "......WP.WP", "......WP.WP", "......WW.WW", ".......WWWW", "......WWWEW",
    "...WWWWWWWN", "..WWWWWWWW.", ".TWWWWWWWW.", "..SSWWWWSS.", "...SS..SS..",
};
const char* const BUNNY_TWITCH[] = {
    "......WP...", "......WP.WW", "......WW.WP", ".......WWWW", "......WWWEW",
    "...WWWWWWWN", "..WWWWWWWW.", ".TWWWWWWWW.", "..SSWWWWSS.", "...SS..SS..",
};
const char* const BUNNY_HOP[] = {
    "....WP.WP..", ".....WP.WP.", "......WWWW.", "......WWWW.", "......WWWEW",
    "..WWWWWWWWN", ".TWWWWWWWW.", "..WWWWWWW..", ".SS....SS..", "SS......SS.",
};
const Palette BUNNY_PAL{{'W', 0xf6f2ee}, {'S', 0xcfc6d6}, {'P', 0xffb3c6},
                        {'E', 0x2a1a24}, {'N', 0xff8fa8}, {'T', 0xffffff}};
const Palette BUNNY_PAL_WIGGLE{{'W', 0xf6f2ee}, {'S', 0xcfc6d6}, {'P', 0xffb3c6},
                               {'E', 0x2a1a24}, {'N', 0xffb3c6}, {'T', 0xffffff}};

const char* const CHICK[] = {".YY..", "YYEYO", "YYYY.", ".YY..", ".o.o."};
const char* const CHICK_PECK[] = {".....", ".YY..", "YYEYO", "YYYY.", ".o.o."};
const Palette CHICK_PAL{{'Y', 0xffe24a}, {'E', 0x2a1a10}, {'O', 0xff9a2a}, {'o', 0xe07a1a}};

const char* const EGG[] = {".XXX.", ".XXX.", "XXXXX", "XXXXX", "XXXXX", ".XXX."};
enum Pattern { STRIPES, ZIGZAG, DOTS };
struct Egg {
  int x;
  uint32_t base, pat;
  Pattern kind;
};
const Egg EGGS[] = {
    {15, 0xff9ec4, 0xffffff, STRIPES}, {23, 0x9fd8ff, 0xffe066, ZIGZAG}, {34, 0xffe066, 0xb98aff, DOTS},
    {41, 0xc3a4ff, 0xffffff, ZIGZAG},  {57, 0xa8f0c0, 0xff8fb8, STRIPES},
};
constexpr int EGG_Y = 56;

struct Tulip {
  int x;
  uint32_t color;
};
const Tulip TULIPS[] = {{15, 0xff8fb8}, {21, 0xffd84a}, {39, 0xb79cff}, {50, 0xff7a7a}, {60, 0xffd84a}};

const char* const BASKET[] = {
    "...hhhhh...", "..h.....h..", ".h..1.3..h.", ".h.112334h.", "bbbbbbbbbbb",
    "bBbBbBbBbBb", ".bbbbbbbbb.", ".BbBbBbBbB.", "..bbbbbbb..",
};
struct PalEntry {
  char k;
  uint32_t c;
};
const PalEntry BASKET_PAL[] = {{'h', 0xb07a3a}, {'b', 0xd09a52}, {'B', 0xa06a30}, {'1', 0xff9ec4},
                               {'2', 0xffe066}, {'3', 0x9fd8ff}, {'4', 0xc3a4ff}};

const char* const CLOUD[] = {"...www......", ".wwwwwww.ww.", "wwwwwwwwwwww", ".ssssssssss."};

inline int backY(int x) { return 36 + int(jsround(2 * std::sin(x * real(0.11) + real(0.8)))); }
constexpr int MEADOW_Y = 42;
constexpr int BUNNY_FLOOR = 57;

bool eggPattern(Pattern kind, int x, int y) {
  if (kind == STRIPES) return y == 2 || y == 4;
  if (kind == ZIGZAG) return (y == 3 && x % 2 == 0) || (y == 2 && x % 2 == 1) || y == 5;
  return (x + y * 2) % 3 == 0 && y > 0 && y < 5;
}

class Easter : public Theme {
 public:
  explicit Easter(Rng& r) : rng(r), cloudPal{{'w', 0}, {'s', 0}}, basketPal{} {
    clouds[0] = {2, 17, real(1.1)};
    clouds[1] = {38, 22, real(0.7)};
  }

  void update(real dt, const Ctx& ctx) override {
    t += dt;
    for (auto& c : clouds) {
      c.x += c.v * dt;
      if (c.x > 66) c.x = -13;
    }

    Bunny& b = bunny;
    if (b.mode == AWAY) {
      b.wait -= dt;
      if (b.wait <= 0) {
        b.mode = HOP;
        b.x = -12;
        b.hopT = 0;
        b.paused = false;
      }
    } else if (b.mode == HOP) {
      b.hopT += dt;
      b.x += 16 * dt;
      if (b.hopT >= real(0.45)) b.hopT -= real(0.45);
      if (!b.paused && b.x >= 24 && b.hopT < real(0.05)) {
        b.mode = PAUSE;
        b.pause = 3 + rng.flt() * 2;
        b.paused = true;
      }
      if (b.x > 66) {
        b.mode = AWAY;
        b.wait = 8 + rng.flt() * 8;
      }
    } else if (b.mode == PAUSE) {
      b.pause -= dt;
      if (b.pause <= 0) {
        b.mode = HOP;
        b.hopT = 0;
      }
    }
    if (b.big > 0) b.big -= dt;
    if (wobble > 0) wobble -= dt;

    if (ctx.shake) {
      wobble = real(2.2);
      if (b.mode == AWAY) {
        b.mode = PAUSE;
        b.x = 26;
        b.pause = real(2.5);
        b.paused = true;
      }
      b.big = real(1.3);
    }
  }

  void draw(Canvas& cv, const Ctx& ctx) override {
    real hf = ctx.hourFloat;
    real day = daylight(hf);
    drawSky(cv, hf, day);
    drawMeadow(cv, day);
    drawTulips(cv, day);
    drawBasket(cv, day);
    drawBunny(cv);
    drawChicks(cv);
    drawEggs(cv);
    if (day < 1) {
      for (int y = 44; y < 64; y++)
        for (int x = 0; x < 64; x++) cv.blend(x, y, 0x101830, (1 - day) * real(0.2));
    }
    drawClock(cv, ctx, COL::clock, COL::clockShadow);
  }

 private:
  enum Mode { AWAY, HOP, PAUSE };
  struct Bunny {
    Mode mode = AWAY;
    real wait = real(1.5);
    real x = -12;
    real hopT = 0;
    real pause = 0;
    bool paused = false;
    real big = 0;
  };
  struct Cloud {
    real x;
    int y;
    real v;
  };

  Rng& rng;
  real t = 0;
  Cloud clouds[2];
  Bunny bunny;
  real wobble = 0;
  Palette cloudPal;
  Palette basketPal;

  void drawSky(Canvas& cv, real hf, real day) {
    SkyColors sky = skyAt(hf);
    vgradient(cv, 0, 0, 63, MEADOW_Y, mix(sky.top, COL::pastel, real(0.2) * day),
              mix(sky.bottom, COL::pastel, real(0.35) * day));
    drawStars(cv, t, 1 - day, 0, 0, 63, 38, 24);

    if (day > real(0.05)) {
      const int cx = 55, cy = 22;
      for (int y = cy - 6; y <= cy + 6; y++) {
        for (int x = cx - 6; x <= cx + 6; x++) {
          real d = std::hypot(real(x - cx), real(y - cy));
          if (d <= real(3.2)) cv.blend(x, y, COL::sun, day);
          else if (d < real(6.5)) cv.blend(x, y, COL::sunGlow, day * real(0.45) * (1 - (d - real(3.2)) / real(3.3)));
        }
      }
    } else {
      cv.rect(54, 20, 56, 22, COL::moon);
      static const int CORNERS[4][2] = {{54, 20}, {56, 20}, {54, 22}, {56, 22}};
      for (const auto& c : CORNERS) cv.blend(c[0], c[1], 0x000000, real(0.55));
    }

    cloudPal.put('w', mix(0x6a6a88, COL::cloud, day));
    cloudPal.put('s', mix(0x4a4a66, COL::cloudShade, day));
    for (const Cloud& c : clouds) cv.sprite(CLOUD, jsround(c.x), c.y, cloudPal);
  }

  void drawMeadow(Canvas& cv, real day) {
    real k = real(0.4) + real(0.6) * day;
    uint32_t back = dim(COL::hillBack, k);
    for (int x = 0; x < 64; x++)
      for (int y = backY(x); y <= MEADOW_Y; y++) cv.set(x, y, back);
    uint32_t m = dim(COL::meadow, k);
    uint32_t md = dim(COL::grassDark, k);
    uint32_t ml = dim(COL::grassLight, k);
    for (int y = MEADOW_Y; y < 64; y++) {
      for (int x = 0; x < 64; x++) {
        uint32_t n = hash(x, y, 11);
        cv.set(x, y, n == 0 ? md : (n == 1 && y < 56) ? ml : m);
      }
    }
    for (int i = 0; i < 9; i++) {
      int x = int(hash(i, 21, 64));
      int y = MEADOW_Y + 1 + int(hash(i, 22, 5));
      cv.set(x, y, dim(i % 2 ? 0xffffff : 0xfff2a0, k));
    }
  }

  void drawTulips(Canvas& cv, real day) {
    real k = real(0.45) + real(0.55) * day;
    for (int i = 0; i < 5; i++) {
      int x = TULIPS[i].x;
      uint32_t color = TULIPS[i].color;
      int sway = std::sin(t * real(1.3) + i * real(1.7)) > real(0.4) ? 1 : 0;
      uint32_t c = dim(color, k);
      uint32_t cd = dim(color, k * real(0.75));
      uint32_t stem = dim(COL::stem, k);
      for (int y = 46; y <= 55; y++) cv.set(x + (y < 49 ? sway : 0), y, stem);
      cv.set(x - 1, 52, stem);
      cv.set(x - 2, 51, stem);
      cv.set(x + 1, 50, stem);
      cv.set(x + 2, 49, stem);
      int bx = x + sway;
      cv.set(bx - 1, 41, c);
      cv.set(bx + 1, 41, c);
      cv.set(bx, 41, cd);
      cv.rect(bx - 1, 42, bx + 1, 44, c);
      cv.set(bx + 1, 43, cd);
      cv.set(bx + 1, 44, cd);
      cv.set(bx - 1, 42, dim(mix(color, 0xffffff, real(0.4)), k));
    }
  }

  void drawBasket(Canvas& cv, real day) {
    real k = real(0.5) + real(0.5) * day;
    for (const PalEntry& e : BASKET_PAL) basketPal.put(e.k, dim(e.c, k));
    cv.sprite(BASKET, 2, 49, basketPal);
  }

  void drawBunny(Canvas& cv) {
    const Bunny& b = bunny;
    if (b.mode == AWAY) return;
    const char* const* frame = BUNNY_SIT;
    real lift = 0;
    if (b.mode == HOP) {
      real p = b.hopT / real(0.45);
      lift = jsround(std::sin(p * kPi) * 4);
      frame = lift > 1 ? BUNNY_HOP : BUNNY_SIT;
    } else if (long(std::floor(t * 3)) % 4 == 0) {
      frame = BUNNY_TWITCH;
    }
    if (b.big > 0) {
      real p = 1 - b.big / real(1.3);
      lift = jsround(std::sin(p * kPi) * 13);
      frame = BUNNY_HOP;
    }
    bool wiggle = b.mode == PAUSE && long(std::floor(t * 6)) % 2;
    cv.sprite(frame, 10, jsround(b.x), BUNNY_FLOOR - 9 - lift, wiggle ? BUNNY_PAL_WIGGLE : BUNNY_PAL);
  }

  void drawChicks(Canvas& cv) {
    struct Chick {
      int x;
      real ph;
    };
    static const Chick CHICKS[] = {{46, 0}, {52, real(1.9)}};
    for (const Chick& ch : CHICKS) {
      bool peck = std::sin(t * real(1.8) + ch.ph) > real(0.75);
      if (peck) cv.sprite(CHICK_PECK, ch.x, 54, CHICK_PAL);
      else cv.sprite(CHICK, ch.x, 54, CHICK_PAL);
      if (std::sin(t * real(1.8) + ch.ph - real(0.6)) > real(0.95)) cv.blend(ch.x + 3, 52, 0xfff2a0, real(0.8));
    }
  }

  void drawEggs(Canvas& cv) {
    real w = wobble > 0 ? std::sin(t * 22) * clamp(wobble / real(2.2), 0, 1) : 0;
    for (int i = 0; i < 5; i++) {
      const Egg& e = EGGS[i];
      int tilt = int(jsround(w * (i % 2 ? 1 : -1) * real(1.4)));
      for (int y = 0; y < 6; y++) {
        int dx = y < 3 ? tilt : 0;
        for (int xx = 0; xx < 5; xx++) {
          if (EGG[y][xx] != 'X') continue;
          uint32_t c = eggPattern(e.kind, xx, y) ? e.pat : e.base;
          if (xx == 4 || y == 5) c = dim(c, real(0.85));
          cv.set(e.x + xx + dx, EGG_Y + y, c);
        }
      }
      cv.set(e.x + 1 + tilt, EGG_Y + 1, 0xffffff);
      cv.set(e.x - 1, EGG_Y + 5, COL::grassDark);
      cv.set(e.x + 5, EGG_Y + 4, COL::grassDark);
      cv.set(e.x + 5, EGG_Y + 5, COL::grassDark);
    }
  }
};

}  // namespace

std::unique_ptr<Theme> createEaster(Rng& rng) { return std::unique_ptr<Theme>(new Easter(rng)); }

}  // namespace pc
