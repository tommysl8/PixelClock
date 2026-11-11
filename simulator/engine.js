/*
 * PixelClock engine: a tiny 64x64 framebuffer plus drawing helpers.
 *
 * Deliberately written so it ports 1:1 to C++ on the ESP32-S3:
 *   - colours are plain 0xRRGGBB integers
 *   - randomness comes from a deterministic xorshift32 RNG
 *   - no browser APIs in here (the page renderer lives in main.js)
 */
(function (global) {
  'use strict';

  const W = 64;
  const H = 64;

  const clamp = (v, lo, hi) => (v < lo ? lo : v > hi ? hi : v);
  const rgb = (r, g, b) => ((r & 255) << 16) | ((g & 255) << 8) | (b & 255);
  const R8 = (c) => (c >> 16) & 255;
  const G8 = (c) => (c >> 8) & 255;
  const B8 = (c) => c & 255;

  // Linear blend from colour a to colour b, t in [0, 1].
  function mix(a, b, t) {
    t = clamp(t, 0, 1);
    return rgb(
      Math.round(R8(a) + (R8(b) - R8(a)) * t),
      Math.round(G8(a) + (G8(b) - G8(a)) * t),
      Math.round(B8(a) + (B8(b) - B8(a)) * t)
    );
  }

  // Scale a colour's brightness by k.
  function dim(c, k) {
    return rgb(
      clamp(Math.round(R8(c) * k), 0, 255),
      clamp(Math.round(G8(c) * k), 0, 255),
      clamp(Math.round(B8(c) * k), 0, 255)
    );
  }

  class Canvas {
    constructor(w = W, h = H) {
      this.w = w;
      this.h = h;
      this.px = new Uint32Array(w * h);
    }

    clear(c = 0) {
      this.px.fill(c);
    }

    inside(x, y) {
      return x >= 0 && y >= 0 && x < this.w && y < this.h;
    }

    set(x, y, c) {
      x = Math.round(x);
      y = Math.round(y);
      if (this.inside(x, y)) this.px[y * this.w + x] = c;
    }

    get(x, y) {
      return this.inside(x, y) ? this.px[y * this.w + x] : 0;
    }

    // Blend colour c over the existing pixel with opacity a.
    blend(x, y, c, a) {
      x = Math.round(x);
      y = Math.round(y);
      if (!this.inside(x, y)) return;
      const i = y * this.w + x;
      this.px[i] = mix(this.px[i], c, a);
    }

    // Filled rectangle, corners inclusive.
    rect(x0, y0, x1, y1, c) {
      for (let y = y0; y <= y1; y++) for (let x = x0; x <= x1; x++) this.set(x, y, c);
    }

    // Draw a sprite from strings: each character is a palette key, '.' is transparent.
    sprite(rows, ox, oy, palette) {
      for (let y = 0; y < rows.length; y++) {
        const row = rows[y];
        for (let x = 0; x < row.length; x++) {
          const c = palette[row[x]];
          if (c !== undefined) this.set(ox + x, oy + y, c);
        }
      }
    }
  }

  // xorshift32: tiny, fast, and identical on the ESP32.
  class Rng {
    constructor(seed = 0x9e3779b9) {
      this.s = seed >>> 0 || 1;
    }
    next() {
      let x = this.s;
      x ^= x << 13;
      x ^= x >>> 17;
      x ^= x << 5;
      this.s = x >>> 0;
      return this.s;
    }
    int(n) {
      return this.next() % n;
    }
    float() {
      return this.next() / 4294967296;
    }
    chance(p) {
      return this.float() < p;
    }
  }

  // 3x5 digit font, drawn at 2x so each digit is 6x10 LEDs.
  const DIGITS = [
    '111101101101111', // 0
    '010110010010111', // 1
    '111001111100111', // 2
    '111001111001111', // 3
    '101101111001001', // 4
    '111100111001111', // 5
    '111100111101111', // 6
    '111001001001001', // 7
    '111101111101111', // 8
    '111101111001111', // 9
  ];

  function drawDigit(cv, d, ox, oy, c, s = 2) {
    const bits = DIGITS[d];
    for (let i = 0; i < 15; i++) {
      if (bits[i] !== '1') continue;
      const x = ox + (i % 3) * s;
      const y = oy + Math.floor(i / 3) * s;
      cv.rect(x, y, x + s - 1, y + s - 1, c);
    }
  }

  // Draw HH:MM centred horizontally with its top edge at row y.
  function drawTime(cv, hours, minutes, opts) {
    const o = opts || {};
    const y = o.y ?? 3;
    const color = o.color ?? 0xffffff;
    const hour12 = o.hour12 ?? true;
    const colon = o.colon ?? true;

    const h = hour12 ? hours % 12 || 12 : hours;
    const digits = [Math.floor(h / 10), h % 10, Math.floor(minutes / 10), minutes % 10];
    const hideLead = hour12 && digits[0] === 0; // "9:41", not "09:41"

    // Full layout is 34 LEDs wide: 6 + 2 + 6 + 2 + colon 2 + 2 + 6 + 2 + 6.
    const x0 = (hideLead ? Math.floor((cv.w - 26) / 2) - 8 : Math.floor((cv.w - 34) / 2)) + (o.xOffset || 0);
    const xs = [x0, x0 + 8, x0 + 20, x0 + 28];

    const pass = (dx, dy, c) => {
      digits.forEach((d, i) => {
        if (!(i === 0 && hideLead)) drawDigit(cv, d, xs[i] + dx, y + dy, c);
      });
      if (colon) {
        cv.rect(x0 + 16 + dx, y + 2 + dy, x0 + 17 + dx, y + 3 + dy, c);
        cv.rect(x0 + 16 + dx, y + 6 + dy, x0 + 17 + dx, y + 7 + dy, c);
      }
    };

    if (o.shadow !== undefined && o.shadow !== null) pass(1, 1, o.shadow);
    pass(0, 0, color);
  }

  // Small number (e.g. a year) in the 3x5 font at 1x, left edge at x.
  function drawSmallNumber(cv, text, x, y, c) {
    for (const ch of String(text)) {
      if (ch >= '0' && ch <= '9') drawDigit(cv, ch.charCodeAt(0) - 48, x, y, c, 1);
      x += 4;
    }
  }

  // The standard clock for every theme: HH:MM at the top with a blinking colon.
  // The shadow colour is also used as a 1-LED outline so the digits stay readable
  // on bright daytime skies.
  function drawClock(cv, ctx, color, shadow, y = 3) {
    const base = { y, colon: ctx.ms < 500, hour12: ctx.hour12 };
    if (shadow !== undefined && shadow !== null) {
      for (let dy = -1; dy <= 2; dy++) {
        for (let dx = -1; dx <= 2; dx++) {
          if (dx === 0 && dy === 0) continue;
          // outline on all sides, plus a 2-LED drop shadow to the lower right
          if ((dx === 2 || dy === 2) && !(dx >= 1 && dy >= 1)) continue;
          drawTime(cv, ctx.hours, ctx.minutes, { ...base, y: y + dy, xOffset: dx, color: shadow });
        }
      }
    }
    drawTime(cv, ctx.hours, ctx.minutes, { ...base, color });
  }

  // Cheap deterministic per-pixel noise in [0, n): for textures that must not flicker.
  function hash(x, y, n) {
    let h = (Math.imul(x | 0, 73856093) ^ Math.imul(y | 0, 19349663)) >>> 0;
    h = Math.imul(h ^ (h >>> 13), 0x5bd1e995) >>> 0;
    return ((h ^ (h >>> 15)) >>> 0) % n;
  }

  // ---------- sky and daylight ----------
  // Default sky through the day: [hour, top colour, bottom colour].
  const SKY = [
    [0, 0x050b1f, 0x0d1a3a],
    [5.5, 0x050b1f, 0x0d1a3a],
    [7, 0x3b4a8c, 0xf2a66b],
    [9, 0x6fb2ea, 0xc9e3f6],
    [15.5, 0x6fb2ea, 0xc9e3f6],
    [17, 0x45457a, 0xe8835a],
    [18.5, 0x0d1236, 0x2a2a5c],
    [24, 0x050b1f, 0x0d1a3a],
  ];

  // Interpolate [top, bottom] sky colours for hour-of-day hf (0..24).
  function skyAt(hf, keys = SKY) {
    for (let i = 0; i < keys.length - 1; i++) {
      const [h0, t0, b0] = keys[i];
      const [h1, t1, b1] = keys[i + 1];
      if (hf >= h0 && hf <= h1) {
        const t = (hf - h0) / (h1 - h0 || 1);
        return [mix(t0, t1, t), mix(b0, b1, t)];
      }
    }
    return [keys[0][1], keys[0][2]];
  }

  // 0 at night, 1 in full daylight, smooth ramps at dawn and dusk.
  function daylight(hf) {
    if (hf < 6 || hf > 18.5) return 0;
    if (hf < 9) return (hf - 6) / 3;
    if (hf > 15.5) return clamp(1 - (hf - 15.5) / 3, 0, 1);
    return 1;
  }

  // Vertical gradient fill, corners inclusive.
  function vgradient(cv, x0, y0, x1, y1, top, bottom) {
    for (let y = y0; y <= y1; y++) {
      const c = mix(top, bottom, y1 === y0 ? 0 : (y - y0) / (y1 - y0));
      for (let x = x0; x <= x1; x++) cv.set(x, y, c);
    }
  }

  // Fixed star field that twinkles; strength 0..1 (usually 1 - daylight).
  function drawStars(cv, t, strength, x0 = 0, y0 = 0, x1 = 63, y1 = 40, count = 28, color = 0xdfe6ff) {
    if (strength <= 0) return;
    for (let i = 0; i < count; i++) {
      const x = x0 + hash(i, 1, x1 - x0 + 1);
      const y = y0 + hash(i, 2, y1 - y0 + 1);
      const tw = 0.55 + 0.45 * Math.sin(t * (1.5 + (i % 5) * 0.4) + i * 2.1);
      cv.blend(x, y, color, strength * tw);
    }
  }

  // ---------- fireworks ----------
  // Rockets launch from launchY, burst between minY and maxY into fading sparks.
  class Fireworks {
    constructor(rng, opts = {}) {
      this.rng = rng;
      this.colors = opts.colors || [0xff4545, 0xffd23f, 0x4fc3ff, 0xff7fd0, 0x7dff8a, 0xffffff];
      this.rate = opts.rate ?? 0.9; // launches per second
      this.x0 = opts.x0 ?? 6;
      this.x1 = opts.x1 ?? 57;
      this.launchY = opts.launchY ?? 63;
      this.minY = opts.minY ?? 16;
      this.maxY = opts.maxY ?? 30;
      this.rockets = [];
      this.sparks = [];
    }

    launch(x) {
      const r = this.rng;
      this.rockets.push({
        x: x ?? this.x0 + r.float() * (this.x1 - this.x0),
        y: this.launchY,
        vx: (r.float() - 0.5) * 6,
        vy: -38 - r.float() * 14,
        targetY: this.minY + r.float() * (this.maxY - this.minY),
        color: this.colors[r.int(this.colors.length)],
      });
    }

    burst(x, y, color, count) {
      const r = this.rng;
      const n = count ?? 26 + r.int(16);
      const speed = 11 + r.float() * 9;
      const second = r.chance(0.35) ? this.colors[r.int(this.colors.length)] : color;
      for (let i = 0; i < n; i++) {
        const a = (i / n) * Math.PI * 2 + r.float() * 0.2;
        const s = speed * (0.55 + r.float() * 0.45);
        this.sparks.push({
          x,
          y,
          vx: Math.cos(a) * s,
          vy: Math.sin(a) * s,
          life: 1.1 + r.float() * 0.8,
          age: 0,
          color: i % 2 ? color : second,
        });
      }
    }

    update(dt) {
      if (this.rng.chance(this.rate * dt)) this.launch();
      for (let i = this.rockets.length - 1; i >= 0; i--) {
        const k = this.rockets[i];
        k.x += k.vx * dt;
        k.y += k.vy * dt;
        k.vy += 18 * dt;
        if (k.y <= k.targetY || k.vy >= -4) {
          this.burst(k.x, k.y, k.color);
          this.rockets.splice(i, 1);
        }
      }
      for (let i = this.sparks.length - 1; i >= 0; i--) {
        const s = this.sparks[i];
        s.age += dt;
        s.vx *= 0.96;
        s.vy = s.vy * 0.96 + 14 * dt;
        s.x += s.vx * dt;
        s.y += s.vy * dt;
        if (s.age >= s.life) this.sparks.splice(i, 1);
      }
    }

    draw(cv) {
      for (const k of this.rockets) {
        cv.blend(k.x, k.y + 1, 0xffb060, 0.5);
        cv.blend(k.x, k.y + 2, 0xff8030, 0.25);
        cv.set(k.x, k.y, 0xfff0c0);
      }
      for (const s of this.sparks) {
        const left = 1 - s.age / s.life;
        const twinkle = left < 0.35 && this.rng.chance(0.4) ? 0.3 : 1;
        cv.blend(s.x, s.y, s.color, Math.min(1, left * 1.6) * twinkle);
      }
    }
  }

  // ---------- theme registry ----------
  // A theme is { id, name, kind: 'season' | 'holiday', create(rng) } and create()
  // returns { update(dt, ctx), draw(canvas, ctx) }. See README "Writing a theme".
  const themes = [];

  global.PixelClock = {
    W,
    H,
    clamp,
    rgb,
    mix,
    dim,
    hash,
    Canvas,
    Rng,
    drawDigit,
    drawTime,
    drawSmallNumber,
    drawClock,
    SKY,
    skyAt,
    daylight,
    vgradient,
    drawStars,
    Fireworks,
    themes,
    registerTheme(theme) {
      themes.push(theme);
    },
    themeById(id) {
      return themes.find((t) => t.id === id) || null;
    },
  };
})(globalThis);
