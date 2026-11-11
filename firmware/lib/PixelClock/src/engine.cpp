// Port of simulator/engine.js. Keep the two in sync.
#include "pixelclock.h"

#include <cstdio>

namespace pc {

int32_t toInt32(real v) {
  if (!std::isfinite(v)) return 0;
  double t = std::trunc(double(v));
  double m = std::fmod(t, 4294967296.0);
  if (m < 0) m += 4294967296.0;
  uint32_t u = uint32_t(m);
  return int32_t(u);
}

uint32_t mix(uint32_t a, uint32_t b, real t) {
  t = clamp(t, 0, 1);
  return rgb(int(jsround(R8(a) + (R8(b) - R8(a)) * t)), int(jsround(G8(a) + (G8(b) - G8(a)) * t)),
             int(jsround(B8(a) + (B8(b) - B8(a)) * t)));
}

uint32_t dim(uint32_t c, real k) {
  return rgb(int(clamp(jsround(R8(c) * k), 0, 255)), int(clamp(jsround(G8(c) * k), 0, 255)),
             int(clamp(jsround(B8(c) * k), 0, 255)));
}

Palette::Palette(std::initializer_list<std::pair<char, uint32_t>> entries) {
  for (int i = 0; i < 128; i++) {
    col[i] = 0;
    has[i] = false;
  }
  for (const auto& e : entries) put(e.first, e.second);
}

// ---------- Canvas ----------
void Canvas::clear(uint32_t c) {
  for (int i = 0; i < W * H; i++) px[i] = c;
}

static inline bool roundCoord(real v, int& out) {
  if (!(v > -100000 && v < 100000)) return false;  // also rejects NaN
  out = int(jsround(v));
  return true;
}

void Canvas::set(real x, real y, uint32_t c) {
  int xi, yi;
  if (!roundCoord(x, xi) || !roundCoord(y, yi)) return;
  if (inside(xi, yi)) px[yi * W + xi] = c;
}

void Canvas::blend(real x, real y, uint32_t c, real a) {
  int xi, yi;
  if (!roundCoord(x, xi) || !roundCoord(y, yi)) return;
  if (!inside(xi, yi)) return;
  uint32_t& p = px[yi * W + xi];
  p = mix(p, c, a);
}

void Canvas::rect(real x0, real y0, real x1, real y1, uint32_t c) {
  for (real y = y0; y <= y1; y++)
    for (real x = x0; x <= x1; x++) set(x, y, c);
}

void Canvas::sprite(const char* const* rows, int nrows, real ox, real oy, const Palette& pal) {
  for (int y = 0; y < nrows; y++) {
    const char* row = rows[y];
    for (int x = 0; row[x]; x++) {
      unsigned char k = (unsigned char)row[x] & 127;
      if (pal.has[k]) set(ox + x, oy + y, pal.col[k]);
    }
  }
}

// ---------- text ----------
static const char* const DIGITS[10] = {
    "111101101101111",  // 0
    "010110010010111",  // 1
    "111001111100111",  // 2
    "111001111001111",  // 3
    "101101111001001",  // 4
    "111100111001111",  // 5
    "111100111101111",  // 6
    "111001001001001",  // 7
    "111101111101111",  // 8
    "111101111001111",  // 9
};

void drawDigit(Canvas& cv, int d, real ox, real oy, uint32_t c, int s) {
  if (d < 0 || d > 9) return;
  const char* bits = DIGITS[d];
  for (int i = 0; i < 15; i++) {
    if (bits[i] != '1') continue;
    real x = ox + (i % 3) * s;
    real y = oy + (i / 3) * s;
    cv.rect(x, y, x + s - 1, y + s - 1, c);
  }
}

void drawTime(Canvas& cv, int hours, int minutes, const TimeOpts& o) {
  int h = o.hour12 ? (hours % 12 ? hours % 12 : 12) : hours;
  int digits[4] = {h / 10, h % 10, minutes / 10, minutes % 10};
  bool hideLead = o.hour12 && digits[0] == 0;
  int x0 = (hideLead ? (Canvas::w - 26) / 2 - 8 : (Canvas::w - 34) / 2) + o.xOffset;
  int xs[4] = {x0, x0 + 8, x0 + 20, x0 + 28};

  auto pass = [&](int dx, int dy, uint32_t c) {
    for (int i = 0; i < 4; i++)
      if (!(i == 0 && hideLead)) drawDigit(cv, digits[i], xs[i] + dx, o.y + dy, c);
    if (o.colon) {
      cv.rect(x0 + 16 + dx, o.y + 2 + dy, x0 + 17 + dx, o.y + 3 + dy, c);
      cv.rect(x0 + 16 + dx, o.y + 6 + dy, x0 + 17 + dx, o.y + 7 + dy, c);
    }
  };
  if (o.hasShadow) pass(1, 1, o.shadow);
  pass(0, 0, o.color);
}

void drawSmallNumber(Canvas& cv, long value, real x, real y, uint32_t c) {
  char buf[24];
  std::snprintf(buf, sizeof buf, "%ld", value);
  for (const char* p = buf; *p; p++) {
    if (*p >= '0' && *p <= '9') drawDigit(cv, *p - '0', x, y, c, 1);
    x += 4;
  }
}

void drawClock(Canvas& cv, const Ctx& ctx, uint32_t color, uint32_t shadow, real y) {
  TimeOpts base;
  base.y = y;
  base.colon = ctx.ms < 500;
  base.hour12 = ctx.hour12;
  for (int dy = -1; dy <= 2; dy++) {
    for (int dx = -1; dx <= 2; dx++) {
      if (dx == 0 && dy == 0) continue;
      if ((dx == 2 || dy == 2) && !(dx >= 1 && dy >= 1)) continue;
      TimeOpts o = base;
      o.y = y + dy;
      o.xOffset = dx;
      o.color = shadow;
      drawTime(cv, ctx.hours, ctx.minutes, o);
    }
  }
  TimeOpts o = base;
  o.color = color;
  drawTime(cv, ctx.hours, ctx.minutes, o);
}

uint32_t hash(real x, real y, uint32_t n) {
  if (n == 0) return 0;
  uint32_t h = (uint32_t(toInt32(x)) * 73856093u) ^ (uint32_t(toInt32(y)) * 19349663u);
  h = (h ^ (h >> 13)) * 0x5bd1e995u;
  return (h ^ (h >> 15)) % n;
}

// ---------- sky ----------
const SkyKey SKY[] = {
    {0, 0x050b1f, 0x0d1a3a},     {real(5.5), 0x050b1f, 0x0d1a3a}, {7, 0x3b4a8c, 0xf2a66b},
    {9, 0x6fb2ea, 0xc9e3f6},     {real(15.5), 0x6fb2ea, 0xc9e3f6}, {17, 0x45457a, 0xe8835a},
    {real(18.5), 0x0d1236, 0x2a2a5c}, {24, 0x050b1f, 0x0d1a3a},
};
const int SKY_COUNT = int(sizeof(SKY) / sizeof(SKY[0]));

SkyColors skyAt(real hf, const SkyKey* keys, int count) {
  for (int i = 0; i < count - 1; i++) {
    const SkyKey& a = keys[i];
    const SkyKey& b = keys[i + 1];
    if (hf >= a.h && hf <= b.h) {
      real span = b.h - a.h;
      real t = (hf - a.h) / (span != 0 ? span : 1);
      return {mix(a.top, b.top, t), mix(a.bottom, b.bottom, t)};
    }
  }
  return {keys[0].top, keys[0].bottom};
}

real daylight(real hf) {
  if (hf < 6 || hf > real(18.5)) return 0;
  if (hf < 9) return (hf - 6) / 3;
  if (hf > real(15.5)) return clamp(1 - (hf - real(15.5)) / 3, 0, 1);
  return 1;
}

void vgradient(Canvas& cv, real x0, real y0, real x1, real y1, uint32_t top, uint32_t bottom) {
  for (real y = y0; y <= y1; y++) {
    uint32_t c = mix(top, bottom, y1 == y0 ? 0 : (y - y0) / (y1 - y0));
    for (real x = x0; x <= x1; x++) cv.set(x, y, c);
  }
}

void drawStars(Canvas& cv, real t, real strength, real x0, real y0, real x1, real y1, int count, uint32_t color) {
  if (strength <= 0) return;
  for (int i = 0; i < count; i++) {
    real x = x0 + hash(i, 1, uint32_t(x1 - x0 + 1));
    real y = y0 + hash(i, 2, uint32_t(y1 - y0 + 1));
    real tw = real(0.55) + real(0.45) * std::sin(t * (real(1.5) + (i % 5) * real(0.4)) + i * real(2.1));
    cv.blend(x, y, color, strength * tw);
  }
}

// ---------- fireworks ----------
Fireworks::Fireworks(Rng& r, const FireworksOptions& o)
    : rng(r), colors(o.colors), rate(o.rate), x0(o.x0), x1(o.x1), launchY(o.launchY), minY(o.minY), maxY(o.maxY) {}

void Fireworks::launch() { launchAt(false, 0); }
void Fireworks::launch(real x) { launchAt(true, x); }

void Fireworks::launchAt(bool hasX, real x) {
  // Same order of random draws as the JavaScript object literal.
  Rocket k;
  k.x = hasX ? x : x0 + rng.flt() * (x1 - x0);
  k.y = launchY;
  k.vx = (rng.flt() - real(0.5)) * 6;
  k.vy = -38 - rng.flt() * 14;
  k.targetY = minY + rng.flt() * (maxY - minY);
  k.color = colors[rng.intn(int(colors.size()))];
  rockets.push_back(k);
}

void Fireworks::burst(real x, real y, uint32_t color) { burstN(x, y, color, false, 0); }
void Fireworks::burst(real x, real y, uint32_t color, int count) { burstN(x, y, color, true, count); }

void Fireworks::burstN(real x, real y, uint32_t color, bool hasCount, int count) {
  int n = hasCount ? count : 26 + rng.intn(16);
  real speed = 11 + rng.flt() * 9;
  uint32_t second = color;
  if (rng.chance(real(0.35))) second = colors[rng.intn(int(colors.size()))];
  for (int i = 0; i < n; i++) {
    real a = (real(i) / n) * kPi * 2 + rng.flt() * real(0.2);
    real s = speed * (real(0.55) + rng.flt() * real(0.45));
    Spark p;
    p.x = x;
    p.y = y;
    p.vx = std::cos(a) * s;
    p.vy = std::sin(a) * s;
    p.life = real(1.1) + rng.flt() * real(0.8);
    p.age = 0;
    p.color = (i % 2) ? color : second;
    sparks.push_back(p);
  }
}

void Fireworks::update(real dt) {
  if (rng.chance(rate * dt)) launch();
  for (int i = int(rockets.size()) - 1; i >= 0; i--) {
    Rocket& k = rockets[i];
    k.x += k.vx * dt;
    k.y += k.vy * dt;
    k.vy += 18 * dt;
    if (k.y <= k.targetY || k.vy >= -4) {
      Rocket done = k;
      rockets.erase(rockets.begin() + i);
      burst(done.x, done.y, done.color);
    }
  }
  for (int i = int(sparks.size()) - 1; i >= 0; i--) {
    Spark& s = sparks[i];
    s.age += dt;
    s.vx *= real(0.96);
    s.vy = s.vy * real(0.96) + 14 * dt;
    s.x += s.vx * dt;
    s.y += s.vy * dt;
    if (s.age >= s.life) sparks.erase(sparks.begin() + i);
  }
}

void Fireworks::draw(Canvas& cv) {
  for (const Rocket& k : rockets) {
    cv.blend(k.x, k.y + 1, 0xffb060, real(0.5));
    cv.blend(k.x, k.y + 2, 0xff8030, real(0.25));
    cv.set(k.x, k.y, 0xfff0c0);
  }
  for (const Spark& s : sparks) {
    real left = 1 - s.age / s.life;
    real twinkle = (left < real(0.35) && rng.chance(real(0.4))) ? real(0.3) : 1;
    cv.blend(s.x, s.y, s.color, jsmin(1, left * real(1.6)) * twinkle);
  }
}

}  // namespace pc
