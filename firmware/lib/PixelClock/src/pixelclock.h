// PixelClock core engine: a 64x64 framebuffer plus drawing helpers.
//
// This is a line-for-line port of simulator/engine.js. It mirrors JavaScript's
// semantics on purpose (Math.round, xorshift32 RNG, evaluation order of random
// draws) so the same theme produces the same pixels in the browser and here.
// tools/compare.js checks that.
//
// `real` is double on the PC (bit-exact with JavaScript) and float on the
// ESP32-S3, whose FPU is single precision.
#pragma once

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <memory>
#include <utility>
#include <vector>

namespace pc {

#ifdef PIXELCLOCK_FLOAT
using real = float;
#else
using real = double;
#endif

constexpr int W = 64;
constexpr int H = 64;
constexpr real kPi = real(3.14159265358979323846);

// ---------- numbers ----------
inline real clamp(real v, real lo, real hi) { return v < lo ? lo : v > hi ? hi : v; }
inline real jsround(real v) { return std::floor(v + real(0.5)); }  // JavaScript Math.round
inline real jsmin(real a, real b) { return a < b ? a : b; }
inline real jsmax(real a, real b) { return a > b ? a : b; }
// JavaScript's % on numbers (sign follows the dividend), for real operands.
inline real jsmod(real a, real b) { return std::fmod(a, b); }
// JavaScript ToInt32 (what `x | 0` does).
int32_t toInt32(real v);

// ---------- colours (0xRRGGBB) ----------
inline uint32_t rgb(int r, int g, int b) {
  return (uint32_t(r & 255) << 16) | (uint32_t(g & 255) << 8) | uint32_t(b & 255);
}
inline int R8(uint32_t c) { return (c >> 16) & 255; }
inline int G8(uint32_t c) { return (c >> 8) & 255; }
inline int B8(uint32_t c) { return c & 255; }
uint32_t mix(uint32_t a, uint32_t b, real t);  // blend a -> b, t in [0, 1]
uint32_t dim(uint32_t c, real k);              // scale brightness by k

// Sprite palette: maps a character to a colour; characters not listed are transparent.
struct Palette {
  uint32_t col[128];
  bool has[128];
  Palette(std::initializer_list<std::pair<char, uint32_t>> entries);
  void put(char k, uint32_t c) {
    col[(unsigned char)k & 127] = c;
    has[(unsigned char)k & 127] = true;
  }
};

// ---------- framebuffer ----------
class Canvas {
 public:
  static constexpr int w = W;
  static constexpr int h = H;
  uint32_t px[W * H];

  Canvas() { clear(0); }
  void clear(uint32_t c = 0);
  static bool inside(int x, int y) { return x >= 0 && y >= 0 && x < W && y < H; }
  void set(real x, real y, uint32_t c);
  uint32_t get(int x, int y) const { return inside(x, y) ? px[y * W + x] : 0; }
  void blend(real x, real y, uint32_t c, real a);
  void rect(real x0, real y0, real x1, real y1, uint32_t c);  // corners inclusive
  void sprite(const char* const* rows, int nrows, real ox, real oy, const Palette& pal);
  template <size_t N>
  void sprite(const char* const (&rows)[N], real ox, real oy, const Palette& pal) {
    sprite(rows, int(N), ox, oy, pal);
  }
};

// ---------- randomness ----------
// xorshift32, identical to the JavaScript version.
class Rng {
 public:
  uint32_t s;
  explicit Rng(uint32_t seed = 0x9e3779b9u) : s(seed ? seed : 1) {}
  uint32_t next() {
    uint32_t x = s;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    s = x;
    return x;
  }
  // JS rng.int(n)
  int intn(int n) { return n > 0 ? int(next() % uint32_t(n)) : 0; }
  // JS rng.float()
  real flt() {
#ifdef PIXELCLOCK_FLOAT
    return real(next()) * (1.0f / 4294967296.0f);
#else
    return real(next()) / 4294967296.0;
#endif
  }
  // JS rng.chance(p)
  bool chance(real p) { return flt() < p; }
};

// ---------- context passed to themes ----------
struct Ctx {
  int hours = 0;
  int minutes = 0;
  int seconds = 0;
  int ms = 0;
  real hourFloat = 0;
  bool hour12 = true;
  int year = 2026;
  int month = 1;
  int day = 1;
  bool shake = false;  // true for one update when the clock is shaken
};

// ---------- text ----------
void drawDigit(Canvas& cv, int d, real ox, real oy, uint32_t c, int s = 2);
struct TimeOpts {
  real y = 3;
  uint32_t color = 0xffffff;
  bool hour12 = true;
  bool colon = true;
  bool hasShadow = false;
  uint32_t shadow = 0;
  int xOffset = 0;
};
void drawTime(Canvas& cv, int hours, int minutes, const TimeOpts& o);
void drawSmallNumber(Canvas& cv, long value, real x, real y, uint32_t c);
void drawClock(Canvas& cv, const Ctx& ctx, uint32_t color, uint32_t shadow, real y = 3);

// Stable per-pixel noise in [0, n).
uint32_t hash(real x, real y, uint32_t n);

// ---------- sky ----------
struct SkyKey {
  real h;
  uint32_t top;
  uint32_t bottom;
};
struct SkyColors {
  uint32_t top;
  uint32_t bottom;
};
extern const SkyKey SKY[];
extern const int SKY_COUNT;
SkyColors skyAt(real hf, const SkyKey* keys, int count);
inline SkyColors skyAt(real hf) { return skyAt(hf, SKY, SKY_COUNT); }
template <size_t N>
SkyColors skyAt(real hf, const SkyKey (&keys)[N]) {
  return skyAt(hf, keys, int(N));
}
real daylight(real hf);
void vgradient(Canvas& cv, real x0, real y0, real x1, real y1, uint32_t top, uint32_t bottom);
void drawStars(Canvas& cv, real t, real strength, real x0 = 0, real y0 = 0, real x1 = 63, real y1 = 40,
               int count = 28, uint32_t color = 0xdfe6ff);

// ---------- fireworks ----------
struct FireworksOptions {
  std::vector<uint32_t> colors{0xff4545, 0xffd23f, 0x4fc3ff, 0xff7fd0, 0x7dff8a, 0xffffff};
  real rate = real(0.9);
  real x0 = 6;
  real x1 = 57;
  real launchY = 63;
  real minY = 16;
  real maxY = 30;
};

class Fireworks {
 public:
  struct Rocket {
    real x, y, vx, vy, targetY;
    uint32_t color;
  };
  struct Spark {
    real x, y, vx, vy, life, age;
    uint32_t color;
  };

  Fireworks(Rng& rng, const FireworksOptions& opts = FireworksOptions());
  void launch();
  void launch(real x);
  void burst(real x, real y, uint32_t color);
  void burst(real x, real y, uint32_t color, int count);
  void update(real dt);
  void draw(Canvas& cv);

  Rng& rng;
  std::vector<uint32_t> colors;
  real rate, x0, x1, launchY, minY, maxY;
  std::vector<Rocket> rockets;
  std::vector<Spark> sparks;

 private:
  void launchAt(bool hasX, real x);
  void burstN(real x, real y, uint32_t color, bool hasCount, int count);
};

}  // namespace pc
