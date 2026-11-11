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
    const x0 = hideLead ? Math.floor((cv.w - 26) / 2) - 8 : Math.floor((cv.w - 34) / 2);
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

  const themes = [];

  global.PixelClock = {
    W,
    H,
    clamp,
    rgb,
    mix,
    dim,
    Canvas,
    Rng,
    drawDigit,
    drawTime,
    themes,
    registerTheme(theme) {
      themes.push(theme);
    },
  };
})(window);
