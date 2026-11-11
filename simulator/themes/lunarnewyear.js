/*
 * Lunar New Year: red lanterns swaying from a carved beam, strings of
 * firecrackers that pop, a plum blossom branch, gold sparkles drifting down,
 * red and gold fireworks, and now and then a little dragon dancing past.
 * Shake: the firecrackers all go off.
 */
(function (PC) {
  'use strict';

  const { mix, dim } = PC;

  const COL = {
    skyTop: 0x0c0a2a,
    skyBottom: 0x4a0f22,
    dayTop: 0x3a1a4a,
    dayBottom: 0x9a3a3a,
    beam: 0x6a3418,
    beamShade: 0x3a1a0c,
    trim: 0xc9a227,
    string: 0x8a6a2a,
    red: 0xd8202a,
    redDark: 0x8a1018,
    redLight: 0xff6a3a,
    gold: 0xe8b84a,
    goldLight: 0xffe08a,
    glow: 0xff3a20,
    branch: 0x3a2016,
    blossom: 0xff9ac0,
    blossomDeep: 0xe0608a,
    blossomCenter: 0xffe08a,
    roof: 0x2a0e18,
    roofAlt: 0x3a1420,
    smoke: 0x8a8090,
    clock: 0xffd75a,
    clockShadow: 0x1a0508,
  };

  const FIREWORK_COLORS = [0xff3030, 0xffc23a, 0xffe08a, 0xff6a3a, 0xff3030];

  const BEAM_Y = 15;
  const ROOF_Y = 55;

  // Lantern: 7 wide. G gold, R red, d dark red, h highlight rib, t tassel.
  const LANTERN = [
    '...GGG...',
    '.GGGGGGG.',
    '.dRRhRRd.',
    'dRRRhRRRd',
    'dRRRhRRRd',
    'dRRRhRRRd',
    'dRRRhRRRd',
    '.dRRhRRd.',
    '.GGGGGGG.',
    '...GGG...',
    '....t....',
    '...t.t...',
    '...t.t...',
  ];
  const LANTERN_PAL = { G: COL.gold, R: COL.red, d: COL.redDark, h: COL.redLight, t: COL.gold };

  // [hang x, string length, sway phase]
  const LANTERNS = [
    [15, 2, 0],
    [32, 8, 1.7],
    [49, 4, 3.1],
  ];

  // Firecracker strings: [x, top y, count]
  const STRINGS = [
    [4, BEAM_Y + 2, 7],
    [59, BEAM_Y + 2, 7],
  ];

  // Dragon head, facing right.
  const DRAGON_HEAD = [
    '.G.G..',
    '.RRRR.',
    'RRRRwR',
    'RRRRRRR',
    '.GG.G..',
  ];
  const DRAGON_PAL = { G: COL.gold, R: COL.red, w: 0xffffff };

  // Plum branch from the left edge: [x, y] points, blossoms at some of them.
  const BRANCH = [
    [0, 47], [1, 47], [2, 46], [3, 46], [4, 45], [5, 45], [6, 44], [7, 44], [8, 43],
    [9, 43], [10, 42], [11, 41], [12, 41], [13, 40], [7, 43], [7, 42], [8, 41], [4, 44], [4, 43],
  ];
  const BLOSSOMS = [[3, 44], [8, 40], [12, 39], [14, 41], [6, 46], [10, 44], [1, 45]];

  function create(rng) {
    let t = 0;

    const fw = new PC.Fireworks(rng, {
      colors: FIREWORK_COLORS,
      rate: 0.35,
      x0: 20,
      x1: 46,
      launchY: ROOF_Y - 2,
      minY: 22,
      maxY: 32,
    });

    // firecrackers: spent flags per string
    const crackers = STRINGS.map(([, , n]) => new Array(n).fill(false));
    let reload = [0, 0];
    const queue = []; // [stringIndex, crackerIndex, delay]
    const flashes = [];
    const smoke = [];

    const sparkles = [];
    for (let i = 0; i < 14; i++) {
      sparkles.push({ x: rng.float() * 64, y: 16 + rng.float() * 40, vy: 2 + rng.float() * 3, ph: rng.float() * 6.283 });
    }

    let dragonX = -60; // head position; off screen when idle
    let dragonWait = 6;

    function crackerPos(s, i) {
      const [x, y] = STRINGS[s];
      return [x + (i % 2 === 0 ? -1 : 1), y + 1 + i * 3];
    }

    function pop(s, i) {
      if (crackers[s][i]) return;
      crackers[s][i] = true;
      const [x, y] = crackerPos(s, i);
      flashes.push({ x, y, age: 0 });
      for (let k = 0; k < 3; k++) {
        smoke.push({ x: x + rng.float() * 2 - 1, y, vx: (rng.float() - 0.5) * 4, vy: -3 - rng.float() * 3, age: 0, life: 1.4 + rng.float() });
      }
    }

    function update(dt, ctx) {
      t += dt;
      fw.update(dt);

      if (ctx.shake) {
        STRINGS.forEach(([, , n], s) => {
          for (let i = n - 1; i >= 0; i--) queue.push([s, i, (n - 1 - i) * 0.08 + s * 0.04]);
        });
        for (let k = 0; k < 3; k++) fw.launch();
      }
      for (let q = queue.length - 1; q >= 0; q--) {
        queue[q][2] -= dt;
        if (queue[q][2] <= 0) {
          pop(queue[q][0], queue[q][1]);
          queue.splice(q, 1);
        }
      }

      // the odd pop from the bottom of a string, and reloading once a string is spent
      crackers.forEach((list, s) => {
        const next = list.lastIndexOf(false);
        if (next < 0) {
          reload[s] += dt;
          if (reload[s] > 4) {
            list.fill(false);
            reload[s] = 0;
          }
        } else if (rng.chance(dt * 0.25)) pop(s, next);
      });

      for (let i = flashes.length - 1; i >= 0; i--) {
        flashes[i].age += dt;
        if (flashes[i].age > 0.25) flashes.splice(i, 1);
      }
      for (let i = smoke.length - 1; i >= 0; i--) {
        const p = smoke[i];
        p.age += dt;
        p.x += p.vx * dt;
        p.y += p.vy * dt;
        p.vx *= 0.97;
        if (p.age > p.life) smoke.splice(i, 1);
      }

      for (const p of sparkles) {
        p.y += p.vy * dt;
        if (p.y > ROOF_Y) {
          p.y = BEAM_Y + 2;
          p.x = rng.float() * 64;
        }
      }

      if (dragonX > -60) {
        dragonX += 11 * dt;
        if (dragonX > 110) {
          dragonX = -60;
          dragonWait = 18 + rng.float() * 15;
        }
      } else {
        dragonWait -= dt;
        if (dragonWait <= 0) dragonX = -6;
      }
    }

    function drawLanterns(cv) {
      for (const [hx, len, ph] of LANTERNS) {
        const sway = Math.sin(t * 1.1 + ph) * 1.4;
        const lx = hx + Math.round(sway); // lantern centre
        const ly = BEAM_Y + 2 + len; // lantern top
        // string, leaning toward the lantern
        for (let y = BEAM_Y + 2; y < ly; y++) {
          const k = (y - BEAM_Y - 2) / Math.max(1, len);
          cv.set(hx + Math.round(sway * k), y, COL.string);
        }
        // soft glow
        const flick = 0.85 + 0.15 * Math.sin(t * 7 + ph * 3);
        for (let dy = -9; dy <= 9; dy++) {
          for (let dx = -10; dx <= 10; dx++) {
            const d = Math.hypot(dx, dy * 1.1);
            if (d < 10) cv.blend(lx + dx, ly + 5 + dy, COL.glow, 0.28 * (1 - d / 10) * flick);
          }
        }
        cv.sprite(LANTERN, lx - 4, ly, LANTERN_PAL);
        // lit centre
        cv.rect(lx, ly + 4, lx, ly + 5, mix(COL.redLight, COL.goldLight, 0.5 * flick));
      }
    }

    function drawCrackers(cv) {
      STRINGS.forEach(([x, y, n], s) => {
        cv.rect(x, y, x, y + n * 3, COL.string);
        for (let i = 0; i < n; i++) {
          if (crackers[s][i]) continue;
          const [cx, cy] = crackerPos(s, i);
          const left = cx < x ? cx - 1 : cx;
          cv.rect(left, cy, left + 1, cy + 1, COL.red);
          cv.set(left + (cx < x ? 0 : 1), cy + 1, COL.redDark);
          cv.rect(left, cy, left + 1, cy, COL.gold);
        }
        // a small gold knot where the string hangs
        cv.set(x, y, COL.trim);
      });
      for (const f of flashes) {
        const k = 1 - f.age / 0.25;
        cv.blend(f.x, f.y, 0xffffff, k);
        for (const [dx, dy] of [[1, 0], [-1, 0], [0, 1], [0, -1]]) cv.blend(f.x + dx, f.y + dy, 0xffd060, k * 0.9);
        for (const [dx, dy] of [[2, 0], [-2, 0], [0, 2], [0, -2], [1, 1], [-1, -1], [1, -1], [-1, 1]]) cv.blend(f.x + dx, f.y + dy, 0xff6a20, k * 0.5);
      }
      for (const p of smoke) cv.blend(p.x, p.y, COL.smoke, 0.45 * (1 - p.age / p.life));
    }

    function drawBranch(cv) {
      for (const [x, y] of BRANCH) cv.set(x, y, COL.branch);
      BLOSSOMS.forEach(([x, y], i) => {
        const c = i % 3 === 0 ? COL.blossomDeep : COL.blossom;
        cv.set(x - 1, y, c);
        cv.set(x + 1, y, c);
        cv.set(x, y - 1, c);
        cv.set(x, y + 1, c);
        cv.set(x, y, COL.blossomCenter);
      });
    }

    function drawRoof(cv) {
      for (let y = ROOF_Y; y < 64; y++) {
        for (let x = 0; x < 64; x++) cv.set(x, y, (x + (y % 2)) % 3 === 0 ? COL.roofAlt : COL.roof);
      }
      cv.rect(0, ROOF_Y, 63, ROOF_Y, COL.trim);
      for (let x = 1; x < 64; x += 3) cv.set(x, ROOF_Y + 1, COL.gold); // tile ends
      // upturned eave ends
      cv.set(0, ROOF_Y - 1, COL.trim);
      cv.set(63, ROOF_Y - 1, COL.trim);
      cv.set(0, ROOF_Y - 2, COL.trim);
      cv.set(63, ROOF_Y - 2, COL.trim);
    }

    function drawDragon(cv) {
      if (dragonX <= -60) return;
      const segs = 14;
      for (let i = segs; i >= 1; i--) {
        const x = dragonX - i * 3;
        if (x < -3 || x > 66) continue;
        const y = 49 + Math.sin(x * 0.28 - t * 5) * 2.2;
        const tail = i === segs;
        const scale = i % 2 === 0 ? COL.red : COL.redLight;
        cv.rect(Math.round(x) - 1, Math.round(y) - 1, Math.round(x) + 1, Math.round(y), tail ? COL.gold : scale);
        cv.set(x, y + 1, COL.gold); // belly
        if (i % 2 === 0) cv.set(x, y - 2, COL.gold); // spines
      }
      const hy = Math.round(49 + Math.sin(dragonX * 0.28 - t * 5) * 2.2) - 2;
      cv.sprite(DRAGON_HEAD, Math.round(dragonX) - 1, hy, DRAGON_PAL);
      // whisker
      cv.set(dragonX + 6, hy + 4 + Math.round(Math.sin(t * 6)), COL.goldLight);
    }

    function draw(cv, ctx) {
      const day = PC.daylight(ctx.hourFloat) * 0.5;
      PC.vgradient(cv, 0, 0, 63, ROOF_Y, mix(COL.skyTop, COL.dayTop, day), mix(COL.skyBottom, COL.dayBottom, day));
      PC.drawStars(cv, t, 0.5 * (1 - day * 2), 0, 0, 63, 40, 18, 0xffe0c0);

      fw.draw(cv);

      // carved beam under the clock
      cv.rect(0, BEAM_Y, 63, BEAM_Y + 1, COL.beam);
      cv.rect(0, BEAM_Y + 1, 63, BEAM_Y + 1, COL.beamShade);
      for (let x = 2; x < 64; x += 6) cv.set(x, BEAM_Y, COL.trim);

      drawLanterns(cv);
      drawCrackers(cv);
      drawBranch(cv);
      drawRoof(cv);
      drawDragon(cv);

      for (const p of sparkles) {
        const tw = 0.5 + 0.5 * Math.sin(t * 5 + p.ph);
        cv.blend(p.x + Math.sin(t + p.ph) * 0.8, p.y, tw > 0.85 ? COL.goldLight : COL.gold, 0.35 + 0.6 * tw);
      }

      PC.drawClock(cv, ctx, COL.clock, COL.clockShadow);
    }

    return { update, draw };
  }

  PC.registerTheme({ id: 'lunarnewyear', name: 'Lunar New Year', kind: 'holiday', create });
})(globalThis.PixelClock);
