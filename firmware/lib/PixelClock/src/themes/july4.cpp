// Fourth of July. Port of simulator/themes/july4.js; keep the two in sync.
#include "../themes.h"

namespace pc {
namespace {

namespace COL {
constexpr uint32_t duskTop = 0x1b1f4a, duskBottom = 0xa04a5e, nightTop = 0x04071a, nightBottom = 0x2a3468,
                   trees = 0x05080c, treesEdge = 0x131c26, shore = 0x0a0d10, waterTop = 0x0c1838,
                   waterBottom = 0x040816, pole = 0xb4b8c4, poleShade = 0x6a6e78, finial = 0xe8c547,
                   red = 0xd8283a, white = 0xf2f2f2, blue = 0x1f3a8a, clock = 0xf4f6ff, clockShadow = 0x05070f;
}

const uint32_t FIREWORK_COLORS[] = {0xff3b3b, 0xffffff, 0x4f8bff, 0xff3b3b, 0x4f8bff, 0xffe08a};
constexpr int FIREWORK_COUNT = 6;

constexpr int HORIZON = 46;  // waterline: the reflection mirrors around this row
constexpr int FLAG_X = 11, FLAG_Y = 21, FLAG_W = 17, FLAG_H = 11;  // flag cloth
constexpr int POLE_X = 10;

FireworksOptions fireworkOptions() {
  FireworksOptions o;
  o.colors.assign(FIREWORK_COLORS, FIREWORK_COLORS + FIREWORK_COUNT);
  o.rate = real(0.9);
  o.x0 = 24;
  o.x1 = 58;
  o.launchY = HORIZON - 4;
  o.minY = 19;
  o.maxY = 30;
  return o;
}

class July4 : public Theme {
 public:
  explicit July4(Rng& r) : rng(r), fw(r, fireworkOptions()) {
    // Treeline heights per column: pines of varying sizes.
    for (int x = 0; x < 64; x++) treeTop[x] = 44;
    int x = -2;
    while (x < 66) {
      int h = 3 + rng.intn(6);
      int half = 1 + int(std::floor(real(h) / real(2.5)));
      for (int dx = -half; dx <= half; dx++) {
        int cx = x + dx;
        if (cx < 0 || cx > 63) continue;
        int top = 44 - h + (dx < 0 ? -dx : dx) * 2;
        if (top < treeTop[cx]) treeTop[cx] = int8_t(top);
      }
      x += 2 + rng.intn(4);
    }
  }

  void update(real dt, const Ctx& ctx) override {
    t += dt;
    if (ctx.shake) {
      finale = 5;
      volley += 10;
    }
    if (volley > 0) {
      volleyTimer -= dt;
      if (volleyTimer <= 0) {
        fw.launch(24 + rng.flt() * 34);
        volley--;
        volleyTimer = real(0.1);
      }
    }
    if (finale > 0) finale -= dt;
    fw.rate = finale > 0 ? real(5) : real(0.9);
    fw.update(dt);
  }

  void draw(Canvas& cv, const Ctx& ctx) override {
    real night = 1 - daylight(ctx.hourFloat);
    uint32_t top = mix(COL::duskTop, COL::nightTop, night);
    uint32_t bottom = mix(COL::duskBottom, COL::nightBottom, night);
    vgradient(cv, 0, 0, 63, HORIZON, top, bottom);
    drawStars(cv, t, real(0.35) + real(0.65) * night, 0, 0, 63, 34, 24);

    fw.draw(cv);

    // treeline and shore
    for (int x = 0; x < 64; x++) {
      cv.rect(x, treeTop[x], x, HORIZON, COL::trees);
      cv.set(x, treeTop[x], COL::treesEdge);
    }
    cv.rect(0, HORIZON - 1, 63, HORIZON, COL::shore);

    drawFlag(cv);
    drawWater(cv, night);

    drawClock(cv, ctx, COL::clock, COL::clockShadow);
  }

 private:
  Rng& rng;
  Fireworks fw;
  real t = 0;
  real finale = 0;  // seconds of finale left
  int volley = 0;
  real volleyTimer = 0;
  int8_t treeTop[64];

  void drawFlag(Canvas& cv) {
    cv.rect(POLE_X, FLAG_Y - 1, POLE_X, HORIZON, COL::pole);
    cv.set(POLE_X, FLAG_Y - 2, COL::finial);
    cv.rect(POLE_X - 1, HORIZON - 1, POLE_X + 1, HORIZON, COL::poleShade);

    for (int c = 0; c < FLAG_W; c++) {
      // wave travels away from the pole and grows toward the free end
      real phase = t * 4 - c * real(0.45);
      real dy = jsround(std::sin(phase) * real(1.3) * (real(c) / FLAG_W));
      real shade = real(0.78) + real(0.22) * std::cos(phase);
      for (int r = 0; r < FLAG_H; r++) {
        uint32_t col;
        if (c < 8 && r < 6) {
          col = COL::blue;
          if (c % 2 == 1 && r % 2 == 1 && c < 7) col = COL::white;
          else if (c % 2 == 0 && r % 2 == 0 && c > 0 && r > 0 && r < 5) col = COL::white;
        } else {
          col = r % 2 == 0 ? COL::red : COL::white;
        }
        cv.set(FLAG_X + c, FLAG_Y + r + dy, dim(col, shade));
      }
    }
  }

  void drawWater(Canvas& cv, real night) {
    uint32_t top = mix(mix(0x2a2a52, COL::waterTop, night), COL::waterTop, real(0.5));
    for (int y = HORIZON + 1; y < 64; y++) {
      real depth = real(y - HORIZON - 1) / (63 - HORIZON - 1);
      uint32_t base = mix(top, COL::waterBottom, depth);
      // Mirrored source row, squashed so the whole sky fits in the shallow lake.
      int sy = int(jsround(HORIZON - (y - HORIZON) * real(2.2)));
      real a = real(0.55) * (1 - depth * real(0.6));
      for (int x = 0; x < 64; x++) {
        int wob = int(jsround(std::sin(y * real(0.9) + t * 3 + x * real(0.15)) * (real(0.6) + depth * real(1.4))));
        uint32_t src = cv.get(int(clamp(x + wob, 0, 63)), sy);
        uint32_t c = mix(base, src, a);
        // little horizontal glints that drift across the surface
        if (hash(x + std::floor(t * 4), y, 97) == 0) c = mix(c, 0x9fb8e8, real(0.3));
        cv.set(x, y, c);
      }
    }
  }
};

}  // namespace

std::unique_ptr<Theme> createJuly4(Rng& rng) { return std::unique_ptr<Theme>(new July4(rng)); }

}  // namespace pc
