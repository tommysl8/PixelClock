/*
 * Spring meadow (spring season): drifting clouds, a cherry blossom tree
 * shedding petals on the breeze, swaying grass and tulips, butterflies, and
 * the odd bird flying past. At night: moon, stars and a few fireflies.
 * Shake: a big swirl of blossom petals.
 */
(function (PC) {
  'use strict';

  const { mix, clamp, hash, skyAt, daylight } = PC;

  const BASE = {
    hillFar: 0xa3d29c,
    hillFarShade: 0x8fc28c,
    grass: 0x52a540,
    grassDark: 0x3f8a32,
    grassLight: 0x6cbd4f,
    blade: 0x76c95a,
    bladeDark: 0x3a7f2c,
    trunk: 0x5a3a26,
    trunkDark: 0x3f2818,
    bloom: 0xf39ac0,
    bloomDark: 0xcf6597,
    bloomDeep: 0xa94c7c,
    bloomLight: 0xffd3e5,
    stem: 0x2f7a2a,
    cloud: 0xffffff,
    cloudShade: 0xdbe5f2,
    daisy: 0xf6f6f0,
  };
  const NIGHT = 0x0b1430;
  const NIGHT_AMOUNT = 0.62;
  const NIGHT_PAL = {};
  for (const key in BASE) NIGHT_PAL[key] = mix(BASE[key], NIGHT, NIGHT_AMOUNT);
  NIGHT_PAL.cloud = 0x2c3760;
  NIGHT_PAL.cloudShade = 0x222c50;

  const PETALS = [0xff8fbf, 0xf26aa6, 0xffb3d4];
  const TULIPS = [0xe8414a, 0xffd23f, 0xa35ad8, 0xff7fb0, 0xff8a3a, 0xf4f4f4, 0xe8414a];
  const MOON = 0xf4f1d0;
  const FIREFLY = 0xd8ff6a;
  const CLOCK = 0xffffff;
  const CLOCK_SHADOW = 0x182340;

  const FAR = [];
  const GROUND = [];
  for (let x = 0; x < 64; x++) {
    FAR.push(Math.round(41 + 2 * Math.sin(x * 0.07 + 2) + 1.3 * Math.sin(x * 0.19)));
    GROUND.push(Math.round(48 + 1.2 * Math.sin(x * 0.09 + 0.4)));
  }

  // Cherry tree canopy as overlapping blobs: [cx, cy, r].
  const CANOPY = [
    [6, 31, 5.5],
    [20, 31, 6],
    [12, 29, 8],
    [15, 23, 6],
    [8, 24, 4.5],
  ];
  const CANOPY_TOP = 17;
  const CANOPY_BOTTOM = 38;
  const inCanopy = (x, y) => CANOPY.some(([cx, cy, r]) => (x - cx) ** 2 + (y - cy) ** 2 <= r * r + r * 0.5);

  const CLOUD = [
    '.....XXXX......',
    '...XXXXXXXX.XX.',
    '.XXXXXXXXXXXXXX',
    'XXXXXXXXXXXXXXX',
    '.SSSSSSSSSSSSS.',
  ];

  // Tulips at different depths: [x, base row, stem height, colour].
  const FLOWERS = [
    [30, 52, 4, TULIPS[1]],
    [42, 52, 4, TULIPS[3]],
    [53, 51, 4, TULIPS[2]],
    [24, 56, 5, TULIPS[0]],
    [36, 57, 5, TULIPS[2]],
    [47, 56, 5, TULIPS[4]],
    [59, 57, 5, TULIPS[5]],
    [20, 61, 6, TULIPS[3]],
    [30, 62, 6, TULIPS[4]],
    [41, 62, 6, TULIPS[0]],
    [53, 62, 6, TULIPS[1]],
  ].map(([x, base, h, c]) => ({ x, base, h, c }));
  const TULIP = ['A.A', 'AAB', 'ABB'];

  function create(rng) {
    let t = 0;
    let breeze = 0;

    const clouds = [
      { x: 4, y: 17, v: 1.1 },
      { x: 34, y: 21, v: 0.8 },
      { x: 58, y: 15, v: 1.4 },
    ];

    const petals = [];
    function spawnPetal(x, y, vx, vy) {
      petals.push({
        x,
        y,
        vx,
        vy,
        ph: rng.float() * 6.283,
        c: PETALS[rng.int(PETALS.length)],
        rest: 0,
        life: 10 + rng.float() * 6,
        age: 0,
      });
    }
    function canopyPoint() {
      const [cx, cy, r] = CANOPY[rng.int(CANOPY.length)];
      const a = rng.float() * 6.283;
      const d = rng.float() * r;
      return [cx + Math.cos(a) * d, cy + Math.sin(a) * d];
    }

    const butterflies = [
      { cx: 36, cy: 34, ax: 14, ay: 5, ph: 0.3, wing: 0xff9a2a, tip: 0x3a2010 },
      { cx: 46, cy: 40, ax: 12, ay: 4, ph: 2.6, wing: 0x6fb6ff, tip: 0x1c3a7a },
    ];

    let bird = null;
    let birdTimer = 4 + rng.float() * 6;

    const fireflies = [];
    for (let i = 0; i < 6; i++) {
      fireflies.push({ cx: 20 + rng.float() * 40, cy: 38 + rng.float() * 14, ph: rng.float() * 6.283, sp: 0.6 + rng.float() * 0.5 });
    }

    function update(dt, ctx) {
      t += dt;
      breeze = 2.2 + 1.6 * Math.sin(t * 0.21) + 0.8 * Math.sin(t * 0.63);
      const day = daylight(ctx.hourFloat);

      for (const c of clouds) {
        c.x += c.v * dt * (0.6 + breeze * 0.15);
        if (c.x > 66) c.x = -16;
      }

      if (petals.length < 40 && rng.chance(4 * dt)) {
        const [x, y] = canopyPoint();
        spawnPetal(x, y, 0, 2);
      }
      for (let i = petals.length - 1; i >= 0; i--) {
        const p = petals[i];
        p.age += dt;
        if (p.rest > 0) {
          p.rest += dt;
          if (p.rest > 3) petals.splice(i, 1);
          continue;
        }
        const tx = breeze + Math.sin(t * 2.1 + p.ph) * 2.5;
        p.vx += (tx - p.vx) * Math.min(1, dt * 2);
        p.vy += (3.5 + Math.sin(t * 3 + p.ph) * 1.5 - p.vy) * Math.min(1, dt * 1.5);
        p.x += p.vx * dt;
        p.y += p.vy * dt;
        const gx = clamp(Math.round(p.x), 0, 63);
        if (p.y >= GROUND[gx] + 1 + (p.ph % 3)) p.rest = 0.001;
        if (p.x > 66 || p.age > p.life) petals.splice(i, 1);
      }

      if (ctx.shake) {
        for (let i = 0; i < 110; i++) {
          const [x, y] = rng.chance(0.5) ? canopyPoint() : [rng.float() * 64, 16 + rng.float() * 30];
          const a = rng.float() * 6.283;
          const s = 12 + rng.float() * 22;
          spawnPetal(x, y, Math.cos(a) * s + 8, Math.sin(a) * s - 6);
        }
      }

      birdTimer -= dt;
      if (!bird && birdTimer <= 0) {
        if (day > 0.3) bird = { x: -4, y: 17 + rng.float() * 8, v: 13 + rng.float() * 5 };
        birdTimer = 12 + rng.float() * 14;
      }
      if (bird) {
        bird.x += bird.v * dt;
        if (bird.x > 68) bird = null;
      }
    }

    // ---------- drawing ----------
    function drawSky(cv, hf, day, P) {
      const [top, bottom] = skyAt(hf);
      PC.vgradient(cv, 0, 0, 63, 46, top, bottom);
      const night = 1 - day;
      PC.drawStars(cv, t, night, 0, 0, 63, 34, 26);
      if (night > 0.25) {
        const m = Math.min(1, (night - 0.25) * 2);
        for (let y = 6; y <= 12; y++) {
          for (let x = 53; x <= 59; x++) {
            const d = (x - 56) ** 2 + (y - 9) ** 2;
            if (d <= 7) cv.set(x, y, mix(top, MOON, m));
            else if (d <= 12) cv.blend(x, y, MOON, m * 0.15);
          }
        }
      }
      const ca = 0.35 + 0.6 * day;
      for (const c of clouds) {
        for (let r = 0; r < CLOUD.length; r++) {
          for (let i = 0; i < CLOUD[r].length; i++) {
            const k = CLOUD[r][i];
            if (k === '.') continue;
            cv.blend(Math.round(c.x) + i, c.y + r, k === 'S' ? P.cloudShade : P.cloud, ca);
          }
        }
      }
    }

    function drawLand(cv, P) {
      for (let x = 0; x < 64; x++) {
        for (let y = FAR[x]; y < GROUND[x]; y++) {
          cv.set(x, y, y === FAR[x] || hash(x, y, 8) !== 0 ? P.hillFar : P.hillFarShade);
        }
        for (let y = GROUND[x]; y < 64; y++) {
          let c = P.grass;
          const h = hash(x, y, 9);
          if (h === 0) c = P.grassDark;
          else if (h === 1) c = P.grassLight;
          if (y > 58 && hash(x, y, 4) === 0) c = P.grassDark;
          cv.set(x, y, c);
          if (hash(x, y, 47) === 0 && y > GROUND[x] + 2) cv.set(x, y, P.daisy);
        }
      }
      // grass blades along the top edge, swaying in the breeze
      for (let x = 0; x < 64; x++) {
        if (hash(x, 3, 3) === 0) continue;
        const h = 1 + hash(x, 5, 3);
        const sway = Math.round(Math.sin(t * 1.6 + x * 0.35) * 0.7 + breeze * 0.12);
        for (let i = 1; i <= h; i++) {
          const dx = i === h ? sway : 0;
          cv.set(x + dx, GROUND[x] - i, i === h ? P.blade : P.bladeDark);
        }
      }
    }

    function drawTree(cv, P) {
      // trunk with a slight lean and two branches
      cv.rect(12, 44, 14, 53, P.trunk);
      cv.rect(14, 44, 14, 53, P.trunkDark);
      cv.rect(12, 37, 13, 43, P.trunk);
      cv.set(11, 53, P.trunk);
      cv.set(15, 53, P.trunkDark);
      for (let i = 0; i < 5; i++) cv.set(11 - i, 36 - Math.floor(i * 0.6), P.trunk);
      for (let i = 0; i < 6; i++) cv.set(14 + i, 36 - Math.floor(i * 0.7), P.trunk);

      for (let y = CANOPY_TOP; y <= CANOPY_BOTTOM; y++) {
        for (let x = -2; x <= 28; x++) {
          if (!inCanopy(x, y)) continue;
          // soft gaps along the top edge so the canopy isn't a solid blob
          if (!inCanopy(x, y - 1) && hash(x, y, 3) === 0) continue;
          // light from the upper left, shadow underneath
          const lit = (x - 11) * 0.8 + (y - 26) * 1.2;
          let c = P.bloom;
          if (lit < -8) c = P.bloomLight;
          else if (lit > 9 || !inCanopy(x, y + 2)) c = P.bloomDeep;
          else if (lit > 3) c = P.bloomDark;
          // blossom clusters: 2x2 blocks of lighter/darker pink
          const cl = hash(x >> 1, y >> 1, 5);
          if (cl === 0) c = c === P.bloomDeep ? P.bloomDark : P.bloomLight;
          else if (cl === 1 && c !== P.bloomLight) c = P.bloomDeep;
          cv.set(x, y, c);
        }
      }
    }

    function drawFlowers(cv, P, day) {
      const bright = 0.45 + 0.55 * day;
      FLOWERS.forEach((f, i) => {
        const sway = Math.round(Math.sin(t * 1.4 + i * 1.3) * 0.6 + breeze * 0.12);
        const top = f.base - f.h;
        for (let y = top + 1; y <= f.base; y++) cv.set(f.x + (y === top + 1 ? sway : 0), y, P.stem);
        cv.set(f.x - 1, f.base - 1, P.stem);
        cv.set(f.x + 1, f.base - 2, P.stem);
        const c = mix(mix(f.c, NIGHT, 0.6), f.c, bright);
        cv.sprite(TULIP, f.x - 1 + sway, top - 2, { A: c, B: PC.dim(c, 0.7) });
      });
    }

    function drawButterflies(cv, day) {
      if (day <= 0.1) return;
      for (const b of butterflies) {
        const x = Math.round(b.cx + b.ax * Math.sin(t * 0.37 + b.ph) + 4 * Math.sin(t * 0.9 + b.ph * 2));
        const y = Math.round(b.cy + b.ay * Math.sin(t * 0.71 + b.ph) + Math.sin(t * 5 + b.ph));
        const open = Math.floor(t * 9 + b.ph * 3) % 2 === 0;
        cv.blend(x, y, b.tip, day);
        if (open) {
          for (const dx of [-1, 1]) {
            cv.blend(x + dx, y - 1, b.wing, day);
            cv.blend(x + dx, y, b.wing, day);
            cv.blend(x + dx * 2, y - 1, b.tip, day);
          }
        } else {
          cv.blend(x - 1, y - 1, b.wing, day);
          cv.blend(x + 1, y - 1, b.wing, day);
        }
      }
    }

    function drawBird(cv) {
      if (!bird) return;
      const x = Math.round(bird.x);
      const y = Math.round(bird.y + Math.sin(t * 2) * 1.2);
      const up = Math.floor(t * 6) % 2 === 0;
      const c = 0x2a2a3a;
      cv.set(x, y, c);
      cv.set(x - 1, y, c);
      cv.set(x + 1, y, c);
      cv.set(x - 2, y + (up ? -1 : 1), c);
      cv.set(x + 2, y + (up ? -1 : 1), c);
    }

    function drawFireflies(cv, day) {
      const night = 1 - day;
      if (night < 0.4) return;
      for (const f of fireflies) {
        const x = f.cx + 5 * Math.sin(t * 0.3 * f.sp + f.ph);
        const y = f.cy + 3 * Math.sin(t * 0.5 * f.sp + f.ph * 2);
        const glow = Math.max(0, Math.sin(t * 1.8 * f.sp + f.ph));
        const a = (night - 0.4) / 0.6 * glow;
        cv.blend(x, y, FIREFLY, a);
        cv.blend(x + 1, y, FIREFLY, a * 0.3);
        cv.blend(x - 1, y, FIREFLY, a * 0.3);
      }
    }

    function draw(cv, ctx) {
      const hf = ctx.hourFloat;
      const day = daylight(hf);
      const P = {};
      for (const key in BASE) P[key] = mix(NIGHT_PAL[key], BASE[key], day);
      const petalBright = 0.5 + 0.5 * day;

      drawSky(cv, hf, day, P);
      drawBird(cv);
      drawLand(cv, P);
      drawTree(cv, P);
      drawFlowers(cv, P, day);
      for (const p of petals) {
        const fade = p.rest > 0 ? 1 - p.rest / 3 : Math.min(1, (p.life - p.age) / 1.5);
        cv.blend(p.x, p.y, mix(mix(p.c, NIGHT, 0.55), p.c, petalBright), fade);
      }
      drawButterflies(cv, day);
      drawFireflies(cv, day);

      PC.drawClock(cv, ctx, CLOCK, CLOCK_SHADOW);
    }

    return { update, draw };
  }

  PC.registerTheme({ id: 'spring', name: 'Spring meadow', kind: 'season', create });
})(globalThis.PixelClock);
