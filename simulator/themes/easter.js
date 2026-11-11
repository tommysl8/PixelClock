/*
 * Easter: a pastel spring meadow with decorated eggs in the grass, tulips,
 * two bobbing chicks, and a bunny that hops across now and then, stopping to
 * twitch its ears. Soft clouds by day, moon and stars by night.
 * Shake: the bunny does a big jump and the eggs wobble.
 */
(function (PC) {
  'use strict';

  const { mix, dim, clamp, skyAt, daylight, hash } = PC;

  const COL = {
    pastel: 0xe4d6f5,
    sun: 0xfff2a8,
    sunGlow: 0xffe9a0,
    moon: 0xf4f1d0,
    cloud: 0xffffff,
    cloudShade: 0xc6cce6,
    hillBack: 0x9edb93,
    meadow: 0x78c96f,
    grassDark: 0x5aae55,
    grassLight: 0xa8e69a,
    stem: 0x2e7a35,
    clock: 0xffffff,
    clockShadow: 0x3a2a5a,
  };

  // Bunny, facing right. 11 wide, 10 tall; feet on the bottom row.
  const BUNNY_SIT = [
    '......WP.WP',
    '......WP.WP',
    '......WW.WW',
    '.......WWWW',
    '......WWWEW',
    '...WWWWWWWN',
    '..WWWWWWWW.',
    '.TWWWWWWWW.',
    '..SSWWWWSS.',
    '...SS..SS..',
  ];
  const BUNNY_TWITCH = [
    '......WP...',
    '......WP.WW',
    '......WW.WP',
    '.......WWWW',
    '......WWWEW',
    '...WWWWWWWN',
    '..WWWWWWWW.',
    '.TWWWWWWWW.',
    '..SSWWWWSS.',
    '...SS..SS..',
  ];
  const BUNNY_HOP = [
    '....WP.WP..',
    '.....WP.WP.',
    '......WWWW.',
    '......WWWW.',
    '......WWWEW',
    '..WWWWWWWWN',
    '.TWWWWWWWW.',
    '..WWWWWWW..',
    '.SS....SS..',
    'SS......SS.',
  ];
  const BUNNY_PAL = { W: 0xf6f2ee, S: 0xcfc6d6, P: 0xffb3c6, E: 0x2a1a24, N: 0xff8fa8, T: 0xffffff };

  const CHICK = ['.YY..', 'YYEYO', 'YYYY.', '.YY..', '.o.o.'];
  const CHICK_PECK = ['.....', '.YY..', 'YYEYO', 'YYYY.', '.o.o.'];
  const CHICK_PAL = { Y: 0xffe24a, E: 0x2a1a10, O: 0xff9a2a, o: 0xe07a1a };

  const EGG = ['.XXX.', '.XXX.', 'XXXXX', 'XXXXX', 'XXXXX', '.XXX.'];
  // [x, base colour, pattern colour, pattern]
  const EGGS = [
    [15, 0xff9ec4, 0xffffff, 'stripes'],
    [23, 0x9fd8ff, 0xffe066, 'zigzag'],
    [34, 0xffe066, 0xb98aff, 'dots'],
    [41, 0xc3a4ff, 0xffffff, 'zigzag'],
    [57, 0xa8f0c0, 0xff8fb8, 'stripes'],
  ];
  const EGG_Y = 56;

  const TULIPS = [
    [15, 0xff8fb8],
    [21, 0xffd84a],
    [39, 0xb79cff],
    [50, 0xff7a7a],
    [60, 0xffd84a],
  ];

  const BASKET = [
    '...hhhhh...',
    '..h.....h..',
    '.h..1.3..h.',
    '.h.112334h.',
    'bbbbbbbbbbb',
    'bBbBbBbBbBb',
    '.bbbbbbbbb.',
    '.BbBbBbBbB.',
    '..bbbbbbb..',
  ];
  const BASKET_PAL = { h: 0xb07a3a, b: 0xd09a52, B: 0xa06a30, 1: 0xff9ec4, 2: 0xffe066, 3: 0x9fd8ff, 4: 0xc3a4ff };

  const CLOUD = ['...www......', '.wwwwwww.ww.', 'wwwwwwwwwwww', '.ssssssssss.'];

  const backY = (x) => 36 + Math.round(2 * Math.sin(x * 0.11 + 0.8));
  const MEADOW_Y = 42;
  const BUNNY_FLOOR = 57; // bunny feet row

  function eggPattern(kind, x, y) {
    if (kind === 'stripes') return y === 2 || y === 4;
    if (kind === 'zigzag') return (y === 3 && x % 2 === 0) || (y === 2 && x % 2 === 1) || y === 5;
    return (x + y * 2) % 3 === 0 && y > 0 && y < 5;
  }

  function create(rng) {
    let t = 0;
    const clouds = [
      { x: 2, y: 17, v: 1.1 },
      { x: 38, y: 22, v: 0.7 },
    ];

    // Bunny state machine: 'away' -> 'hop' -> 'pause' -> 'hop' -> 'away'
    const bunny = { mode: 'away', wait: 1.5, x: -12, hopT: 0, pause: 0, paused: false, big: 0 };
    let wobble = 0;

    function update(dt, ctx) {
      t += dt;
      for (const c of clouds) {
        c.x += c.v * dt;
        if (c.x > 66) c.x = -13;
      }

      const b = bunny;
      if (b.mode === 'away') {
        b.wait -= dt;
        if (b.wait <= 0) {
          b.mode = 'hop';
          b.x = -12;
          b.hopT = 0;
          b.paused = false;
        }
      } else if (b.mode === 'hop') {
        b.hopT += dt;
        b.x += 16 * dt;
        if (b.hopT >= 0.45) b.hopT -= 0.45;
        if (!b.paused && b.x >= 24 && b.hopT < 0.05) {
          b.mode = 'pause';
          b.pause = 3 + rng.float() * 2;
          b.paused = true;
        }
        if (b.x > 66) {
          b.mode = 'away';
          b.wait = 8 + rng.float() * 8;
        }
      } else if (b.mode === 'pause') {
        b.pause -= dt;
        if (b.pause <= 0) {
          b.mode = 'hop';
          b.hopT = 0;
        }
      }
      if (b.big > 0) b.big -= dt;
      if (wobble > 0) wobble -= dt;

      if (ctx.shake) {
        wobble = 2.2;
        if (b.mode === 'away') {
          b.mode = 'pause';
          b.x = 26;
          b.pause = 2.5;
          b.paused = true;
        }
        b.big = 1.3;
      }
    }

    // ---------- drawing ----------
    function drawSky(cv, hf, day) {
      const [top, bottom] = skyAt(hf);
      PC.vgradient(cv, 0, 0, 63, MEADOW_Y, mix(top, COL.pastel, 0.2 * day), mix(bottom, COL.pastel, 0.35 * day));
      PC.drawStars(cv, t, 1 - day, 0, 0, 63, 38, 24);

      if (day > 0.05) {
        // soft sun on the right, below the clock
        const cx = 55;
        const cy = 22;
        for (let y = cy - 6; y <= cy + 6; y++) {
          for (let x = cx - 6; x <= cx + 6; x++) {
            const d = Math.hypot(x - cx, y - cy);
            if (d <= 3.2) cv.blend(x, y, COL.sun, day);
            else if (d < 6.5) cv.blend(x, y, COL.sunGlow, day * 0.45 * (1 - (d - 3.2) / 3.3));
          }
        }
      } else {
        cv.rect(54, 20, 56, 22, COL.moon);
        for (const [x, y] of [[54, 20], [56, 20], [54, 22], [56, 22]]) cv.blend(x, y, 0x000000, 0.55);
      }

      const cc = mix(0x6a6a88, COL.cloud, day);
      const cs = mix(0x4a4a66, COL.cloudShade, day);
      for (const c of clouds) cv.sprite(CLOUD, Math.round(c.x), c.y, { w: cc, s: cs });
    }

    function drawMeadow(cv, day) {
      const k = 0.4 + 0.6 * day;
      const back = dim(COL.hillBack, k);
      for (let x = 0; x < 64; x++) for (let y = backY(x); y <= MEADOW_Y; y++) cv.set(x, y, back);
      const m = dim(COL.meadow, k);
      const md = dim(COL.grassDark, k);
      const ml = dim(COL.grassLight, k);
      for (let y = MEADOW_Y; y < 64; y++) {
        for (let x = 0; x < 64; x++) {
          const n = hash(x, y, 11);
          cv.set(x, y, n === 0 ? md : n === 1 && y < 56 ? ml : m);
        }
      }
      // tiny daisies dotted in the far meadow
      for (let i = 0; i < 9; i++) {
        const x = hash(i, 21, 64);
        const y = MEADOW_Y + 1 + hash(i, 22, 5);
        cv.set(x, y, dim(i % 2 ? 0xffffff : 0xfff2a0, k));
      }
    }

    function drawTulips(cv, day) {
      const k = 0.45 + 0.55 * day;
      TULIPS.forEach(([x, color], i) => {
        const sway = Math.sin(t * 1.3 + i * 1.7) > 0.4 ? 1 : 0;
        const c = dim(color, k);
        const cd = dim(color, k * 0.75);
        const stem = dim(COL.stem, k);
        for (let y = 46; y <= 55; y++) cv.set(x + (y < 49 ? sway : 0), y, stem);
        // two leaves
        cv.set(x - 1, 52, stem);
        cv.set(x - 2, 51, stem);
        cv.set(x + 1, 50, stem);
        cv.set(x + 2, 49, stem);
        // cup-shaped bloom with three petal tips
        const bx = x + sway;
        cv.set(bx - 1, 41, c);
        cv.set(bx + 1, 41, c);
        cv.set(bx, 41, cd);
        cv.rect(bx - 1, 42, bx + 1, 44, c);
        cv.set(bx + 1, 43, cd);
        cv.set(bx + 1, 44, cd);
        cv.set(bx - 1, 42, dim(mix(color, 0xffffff, 0.4), k));
      });
    }

    function drawBasket(cv, day) {
      const k = 0.5 + 0.5 * day;
      const pal = {};
      for (const key in BASKET_PAL) pal[key] = dim(BASKET_PAL[key], k);
      cv.sprite(BASKET, 2, 49, pal);
    }

    function drawBunny(cv) {
      const b = bunny;
      if (b.mode === 'away') return;
      let frame = BUNNY_SIT;
      let lift = 0;
      if (b.mode === 'hop') {
        const p = b.hopT / 0.45;
        lift = Math.round(Math.sin(p * Math.PI) * 4);
        frame = lift > 1 ? BUNNY_HOP : BUNNY_SIT;
      } else if (Math.floor(t * 3) % 4 === 0) {
        frame = BUNNY_TWITCH;
      }
      if (b.big > 0) {
        const p = 1 - b.big / 1.3;
        lift = Math.round(Math.sin(p * Math.PI) * 13);
        frame = BUNNY_HOP;
      }
      // nose wiggle while paused
      const pal = b.mode === 'pause' && Math.floor(t * 6) % 2 ? { ...BUNNY_PAL, N: 0xffb3c6 } : BUNNY_PAL;
      cv.sprite(frame, Math.round(b.x), BUNNY_FLOOR - 9 - lift, pal);
    }

    function drawChicks(cv) {
      [
        [46, 0],
        [52, 1.9],
      ].forEach(([x, ph]) => {
        const peck = Math.sin(t * 1.8 + ph) > 0.75;
        cv.sprite(peck ? CHICK_PECK : CHICK, x, 54, CHICK_PAL);
        // a little "peep" sparkle above the chick that just pecked
        if (Math.sin(t * 1.8 + ph - 0.6) > 0.95) cv.blend(x + 3, 52, 0xfff2a0, 0.8);
      });
    }

    function drawEggs(cv) {
      const w = wobble > 0 ? Math.sin(t * 22) * clamp(wobble / 2.2, 0, 1) : 0;
      EGGS.forEach(([x, base, pat, kind], i) => {
        const tilt = Math.round(w * (i % 2 ? 1 : -1) * 1.4);
        for (let y = 0; y < EGG.length; y++) {
          // top rows lean further than the bottom when wobbling
          const dx = y < 3 ? tilt : 0;
          for (let xx = 0; xx < 5; xx++) {
            if (EGG[y][xx] !== 'X') continue;
            let c = eggPattern(kind, xx, y) ? pat : base;
            if (xx === 4 || y === 5) c = dim(c, 0.85);
            cv.set(x + xx + dx, EGG_Y + y, c);
          }
        }
        cv.set(x + 1 + tilt, EGG_Y + 1, 0xffffff);
        // grass blades in front of each egg
        cv.set(x - 1, EGG_Y + 5, COL.grassDark);
        cv.set(x + 5, EGG_Y + 4, COL.grassDark);
        cv.set(x + 5, EGG_Y + 5, COL.grassDark);
      });
    }

    function draw(cv, ctx) {
      const hf = ctx.hourFloat;
      const day = daylight(hf);
      drawSky(cv, hf, day);
      drawMeadow(cv, day);
      drawTulips(cv, day);
      drawBasket(cv, day);
      drawBunny(cv);
      drawChicks(cv);
      drawEggs(cv);
      if (day < 1) {
        // gentle night dimming over the foreground sprites
        for (let y = 44; y < 64; y++) for (let x = 0; x < 64; x++) cv.blend(x, y, 0x101830, (1 - day) * 0.2);
      }
      PC.drawClock(cv, ctx, COL.clock, COL.clockShadow);
    }

    return { update, draw };
  }

  PC.registerTheme({ id: 'easter', name: 'Easter', kind: 'holiday', create });
})(globalThis.PixelClock);
