/*
 * Winter cabin theme: crackling fireplace, twinkling tree, snow past the
 * window, a sleeping cat, and a sky that follows the real time of day.
 *
 * A theme is { id, name, create(rng) } where create() returns
 *   { update(dt, ctx), draw(canvas, ctx) }.
 * ctx carries the time (hours, minutes, seconds, hourFloat), hour12, and
 * one-shot events like ctx.shake (the accelerometer on the real board).
 */
(function (PC) {
  'use strict';

  const { mix, dim, clamp } = PC;

  // ---------- palette ----------
  const COL = {
    wallNight: 0x1a1222,
    wallDay: 0x3b2d3f,
    floorNight: 0x2b1a10,
    floorDay: 0x4a3020,
    plank: 0x1f120a,
    rugEdge: 0x8e2f3a,
    rug: 0x6b1f2a,
    rugDot: 0xc9a227,
    frame: 0x6b4428,
    sill: 0x84593a,
    snow: 0xeef4ff,
    snowPile: 0xdfe8f5,
    moon: 0xf4f1d0,
    star: 0xcfd8ff,
    brick: 0x7d2f20,
    brickAlt: 0x6e2a1c,
    mortar: 0x4f1d14,
    hearth: 0x514139,
    opening: 0x0b0605,
    mantel: 0x5b3a22,
    mantelTop: 0x74502f,
    log: 0x5a3418,
    logDark: 0x3e2410,
    ember: 0xff6a1a,
    leaf: 0x1f6b2e,
    leafLight: 0x2a8a3c,
    leafDark: 0x145022,
    trunk: 0x5a3418,
    starDim: 0xc9a227,
    starBright: 0xfff3a0,
    glow: 0xff8a2a,
    clock: 0xffe9c4,
    clockShadow: 0x0c0710,
  };

  // Fire heat 0..12 mapped to colour (0 = nothing drawn).
  const FIRE = [
    0, 0x1a0502, 0x3a0a03, 0x5e1204, 0x861b05, 0xa82806, 0xc93a08,
    0xe2560c, 0xf07514, 0xf8951f, 0xfcb33a, 0xfed062, 0xffe9a0,
  ];
  const MAX_HEAT = FIRE.length - 1;

  const LIGHT_COLORS = [0xff4545, 0xffd23f, 0x4fc3ff, 0xff7fd0, 0x7dff8a];

  // Sky through the day: [hour, top colour, bottom colour].
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

  function skyAt(hf) {
    for (let i = 0; i < SKY.length - 1; i++) {
      const [h0, t0, b0] = SKY[i];
      const [h1, t1, b1] = SKY[i + 1];
      if (hf >= h0 && hf <= h1) {
        const t = (hf - h0) / (h1 - h0 || 1);
        return [mix(t0, t1, t), mix(b0, b1, t)];
      }
    }
    return [SKY[0][1], SKY[0][2]];
  }

  // 0 at night, 1 in full daylight.
  function daylight(hf) {
    if (hf < 6 || hf > 18.5) return 0;
    if (hf < 9) return (hf - 6) / 3;
    if (hf > 15.5) return clamp(1 - (hf - 15.5) / 3, 0, 1);
    return 1;
  }

  // ---------- layout (all in LED coordinates) ----------
  const WIN = { x0: 2, y0: 17, x1: 17, y1: 34 }; // window frame, outer
  const SKY_BOX = { x0: 4, y0: 19, x1: 15, y1: 32 }; // glass
  const MULLION_X = 9; // vertical bar is MULLION_X..MULLION_X+1
  const TRANSOM_Y = 25; // horizontal bar is TRANSOM_Y..TRANSOM_Y+1
  const FP = { x0: 20, y0: 36, x1: 43, y1: 57 }; // brick fireplace
  const OPEN = { x0: 25, y0: 42, x1: 38, y1: 57 }; // firebox opening
  const FIREBOX = { x: 25, y: 43, w: 14, h: 14 }; // simulated flames
  const FLOOR_Y = 58;
  const TREE_X = 55;
  const TREE_TIERS = [
    [18, 29, 1, 5],
    [27, 39, 3, 6],
    [37, 50, 4, 7],
  ]; // [top row, bottom row, half-width at top, half-width at bottom]

  function treeHalfWidth(y) {
    let hw = -1;
    for (const [a, b, s, e] of TREE_TIERS) {
      if (y >= a && y <= b) hw = Math.max(hw, s + Math.floor(((y - a) * (e - s)) / (b - a)));
    }
    return hw;
  }

  const STARS = [
    [5, 20],
    [7, 23],
    [15, 24],
    [6, 28],
    [11, 23],
    [14, 29],
    [12, 31],
  ];

  // Sleeping cat, facing the fire. Two frames for breathing.
  const CAT_A = [
    '......O.O',
    '..OSOSOOO',
    '.OOOOOOdO',
    'TTOOOOOOO',
  ];
  const CAT_B = [
    '......O.O',
    '.OOSOSOOO',
    '.OOOOOOdO',
    'TTOOOOOOO',
  ];
  const CAT_TAIL_UP = [
    '......O.O',
    '..OSOSOOO',
    'TOOOOOOdO',
    '.TOOOOOOO',
  ];
  const CAT_PAL = { O: 0xd9822b, S: 0xa35a17, d: 0x4a2a12, T: 0xc0701f };

  const STOCKING = ['WW.', 'RR.', 'RR.', 'RRR'];

  function create(rng) {
    const heat = new Uint8Array(FIREBOX.w * FIREBOX.h);
    let fireLevel = 0.8;
    let flare = 0;

    let t = 0;

    // snow outside the window
    const flakes = [];
    function newFlake(anywhere) {
      return {
        bx: SKY_BOX.x0 + rng.float() * (SKY_BOX.x1 - SKY_BOX.x0 + 1),
        y: anywhere ? SKY_BOX.y0 + rng.float() * (SKY_BOX.y1 - SKY_BOX.y0) : SKY_BOX.y0 - 1,
        v: 2.5 + rng.float() * 3.5,
        ph: rng.float() * 6.283,
      };
    }
    for (let i = 0; i < 10; i++) flakes.push(newFlake(true));

    // tree lights at fixed spots inside the tree
    const lights = [];
    const taken = new Set();
    for (let tries = 0; lights.length < 18 && tries < 400; tries++) {
      const y = 20 + rng.int(30);
      const hw = treeHalfWidth(y);
      if (hw < 1) continue;
      const x = TREE_X - hw + 1 + rng.int(2 * hw - 1);
      const key = x * 100 + y;
      if (taken.has(key) || taken.has(key + 1) || taken.has(key - 1) || taken.has(key + 100) || taken.has(key - 100)) continue;
      taken.add(key);
      lights.push({ x, y, c: rng.int(LIGHT_COLORS.length), on: rng.chance(0.6) });
    }
    lights.sort((a, b) => a.y - b.y || a.x - b.x);
    let lightMode = 0; // 0 twinkle, 1 chase, 2 breathe
    let modeTimer = 0;

    // snow globe particles (shake the clock!)
    const globe = [];

    // cat
    let catTail = 0;

    function stepFire() {
      const { w, h } = FIREBOX;
      if (flare > 0) flare--;
      else if (rng.chance(0.01)) flare = 6 + rng.int(8);

      // Heat source along the bottom row, hottest where the logs cross.
      for (let x = 0; x < w; x++) {
        const edge = Math.abs(x - (w - 1) / 2) / ((w - 1) / 2);
        const base = Math.round(MAX_HEAT - edge * 5) + (flare > 0 ? 1 : 0);
        heat[(h - 1) * w + x] = clamp(base - rng.int(3), 0, MAX_HEAT);
      }
      // Each cell pulls heat from below (with a little sideways drift) and cools.
      for (let y = 0; y < h - 1; y++) {
        for (let x = 0; x < w; x++) {
          const sx = clamp(x + rng.int(3) - 1, 0, w - 1);
          const below = heat[(y + 1) * w + sx];
          // average cooling of ~0.8 per row lets the centre flames reach the top of the firebox
          const cool = rng.int(3) === 0 ? 0 : rng.chance(0.25) ? 2 : 1;
          heat[y * w + x] = Math.max(0, below - cool);
        }
      }
      // Fire brightness drives the room glow.
      let sum = 0;
      for (let y = h - 5; y < h; y++) for (let x = 0; x < w; x++) sum += heat[y * w + x];
      const level = sum / (5 * w * MAX_HEAT);
      fireLevel += (level - fireLevel) * 0.25;
    }

    function spawnGlobe() {
      for (let i = 0; i < 160; i++) {
        globe.push({
          x: rng.float() * 64,
          y: rng.float() * 64,
          vx: (rng.float() - 0.5) * 50,
          vy: -10 - rng.float() * 30,
          life: 6 + rng.float() * 4,
          age: 0,
          ph: rng.float() * 6.283,
        });
      }
    }

    function update(dt, ctx) {
      t += dt;
      stepFire();

      for (let i = 0; i < flakes.length; i++) {
        const f = flakes[i];
        f.y += f.v * dt;
        if (f.y > SKY_BOX.y1 - 0.5) flakes[i] = newFlake(false);
      }

      modeTimer += dt;
      if (modeTimer > 12) {
        modeTimer = 0;
        lightMode = (lightMode + 1) % 3;
      }
      if (lightMode === 0) {
        for (const l of lights) if (rng.chance(0.04)) l.on = !l.on;
      }

      if (catTail > 0) catTail -= dt;
      else if (rng.chance(0.004)) catTail = 0.35;

      if (ctx.shake) spawnGlobe();
      for (let i = globe.length - 1; i >= 0; i--) {
        const p = globe[i];
        p.age += dt;
        p.vx *= 0.96;
        p.vy = Math.min(p.vy + 14 * dt, 7);
        p.x += (p.vx + Math.sin(t * 2 + p.ph) * 3) * dt;
        p.y += p.vy * dt;
        if (p.y > 63) {
          p.y = 63;
          p.vy = 0;
        }
        if (p.age > p.life) globe.splice(i, 1);
      }
    }

    // ---------- drawing ----------
    function drawRoom(cv, day) {
      const wall = mix(COL.wallNight, COL.wallDay, day * 0.7);
      const stripe = dim(wall, 0.82);
      for (let y = 0; y < FLOOR_Y; y++) {
        for (let x = 0; x < 64; x++) cv.set(x, y, x % 6 === 0 ? stripe : wall);
      }
      const floor = mix(COL.floorNight, COL.floorDay, day * 0.6);
      cv.rect(0, FLOOR_Y, 63, 63, floor);
      cv.rect(0, 61, 63, 61, COL.plank);
      for (let x = 5; x < 64; x += 11) cv.set(x, 59 + ((x >> 2) & 1), COL.plank);

      // rug
      cv.rect(3, 59, 18, 62, COL.rugEdge);
      cv.rect(4, 60, 17, 61, COL.rug);
      for (let x = 5; x < 17; x += 3) cv.set(x, 60 + ((x / 3) & 1), COL.rugDot);
    }

    function drawWindow(cv, hf, day) {
      const [top, bottom] = skyAt(hf);
      cv.rect(WIN.x0, WIN.y0, WIN.x1, WIN.y1, COL.frame);
      for (let y = SKY_BOX.y0; y <= SKY_BOX.y1; y++) {
        const c = mix(top, bottom, (y - SKY_BOX.y0) / (SKY_BOX.y1 - SKY_BOX.y0));
        cv.rect(SKY_BOX.x0, y, SKY_BOX.x1, y, c);
      }

      const night = 1 - day;
      if (night > 0.3) {
        STARS.forEach(([x, y], i) => {
          const tw = 0.5 + 0.5 * Math.sin(t * 2.3 + i * 1.7);
          cv.blend(x, y, COL.star, (night - 0.3) * tw);
        });
        // moon with a soft edge
        const a = Math.min(1, (night - 0.3) * 2);
        cv.rect(12, 20, 14, 22, mix(top, COL.moon, a));
        for (const [x, y] of [[12, 20], [14, 20], [12, 22], [14, 22]]) cv.set(x, y, mix(top, COL.moon, a * 0.45));
      }

      const flakeCol = mix(0xb9c6e0, COL.snow, day);
      for (const f of flakes) {
        const x = f.bx + Math.sin(t * 1.5 + f.ph) * 0.8;
        if (x >= SKY_BOX.x0 && x <= SKY_BOX.x1 + 0.49) cv.set(x, f.y, flakeCol);
      }

      // snow piled on the bottom of the glass
      cv.rect(SKY_BOX.x0, SKY_BOX.y1, SKY_BOX.x1, SKY_BOX.y1, COL.snowPile);
      for (const x of [4, 5, 12, 13, 14]) cv.set(x, SKY_BOX.y1 - 1, COL.snowPile);

      // mullions and sill
      cv.rect(MULLION_X, SKY_BOX.y0, MULLION_X + 1, SKY_BOX.y1, COL.frame);
      cv.rect(SKY_BOX.x0, TRANSOM_Y, SKY_BOX.x1, TRANSOM_Y + 1, COL.frame);
      cv.rect(WIN.x0 - 1, WIN.y1 + 1, WIN.x1 + 1, WIN.y1 + 1, COL.sill);
    }

    function drawFireplace(cv) {
      for (let y = FP.y0; y <= FP.y1; y++) {
        const row = Math.floor((y - FP.y0) / 3);
        for (let x = FP.x0; x <= FP.x1; x++) {
          const mortar = (y - FP.y0) % 3 === 0 || (x - FP.x0 + (row % 2) * 3) % 6 === 0;
          const shade = ((Math.floor((x - FP.x0 + (row % 2) * 3) / 6) + row) % 3) === 0 ? COL.brickAlt : COL.brick;
          cv.set(x, y, mortar ? COL.mortar : shade);
        }
      }
      cv.rect(OPEN.x0, OPEN.y0, OPEN.x1, OPEN.y1, COL.opening);
      // rounded top corners of the opening
      cv.set(OPEN.x0, OPEN.y0, COL.brick);
      cv.set(OPEN.x1, OPEN.y0, COL.brick);

      // flames
      const { x: fx, y: fy, w, h } = FIREBOX;
      for (let y = 0; y < h; y++) {
        for (let x = 0; x < w; x++) {
          const v = heat[y * w + x];
          if (v > 0) cv.set(fx + x, fy + y, FIRE[v]);
        }
      }

      // logs with glowing embers
      cv.rect(26, 56, 37, 56, COL.log);
      cv.rect(28, 55, 35, 55, COL.logDark);
      for (const x of [27, 30, 33, 36]) {
        if (rng.chance(0.7)) cv.set(x, 56, mix(COL.log, COL.ember, 0.5 + rng.float() * 0.5));
      }

      cv.rect(FP.x0 - 1, FLOOR_Y, FP.x1 + 1, FLOOR_Y, COL.hearth);

      // mantel + garland
      cv.rect(19, 34, 44, 35, COL.mantel);
      cv.rect(19, 34, 44, 34, COL.mantelTop);
      cv.rect(19, 33, 44, 33, COL.leafDark);
      for (let x = 20, i = 0; x <= 43; x += 4, i++) {
        const on = (Math.floor(t * 1.5) + i) % 2 === 0;
        cv.set(x, 33, on ? LIGHT_COLORS[i % 2 === 0 ? 0 : 1] : COL.leaf);
      }

      // stockings
      cv.sprite(STOCKING, 22, 36, { W: 0xf2f2f2, R: 0xc0392b });
      cv.sprite(STOCKING, 39, 36, { W: 0xf2f2f2, R: 0x2e8b57 });
    }

    function drawTree(cv) {
      for (let y = 18; y <= 50; y++) {
        const hw = treeHalfWidth(y);
        if (hw < 0) continue;
        for (let x = TREE_X - hw; x <= TREE_X + hw; x++) {
          let c = x < TREE_X ? COL.leafLight : COL.leaf;
          if ((x * 7 + y * 3) % 5 === 0) c = COL.leafDark;
          if (x === TREE_X + hw) c = COL.leafDark;
          cv.set(x, y, c);
        }
      }
      cv.rect(54, 51, 56, 57, COL.trunk);

      lights.forEach((l, i) => {
        let on;
        if (lightMode === 0) on = l.on;
        else if (lightMode === 1) on = (((Math.floor(t * 8) - i) % 6) + 6) % 6 < 3;
        else on = true;
        let c = LIGHT_COLORS[l.c];
        if (lightMode === 2) c = mix(dim(c, 0.25), c, 0.5 + 0.5 * Math.sin(t * 2 + l.c));
        cv.set(l.x, l.y, on ? c : dim(c, 0.18));
      });

      // star on top
      const s = 0.5 + 0.5 * Math.sin(t * 3);
      const sc = mix(COL.starDim, COL.starBright, s);
      cv.set(TREE_X, 15, sc);
      cv.rect(TREE_X - 1, 16, TREE_X + 1, 16, sc);
      cv.set(TREE_X, 17, sc);
      if (s > 0.8) {
        cv.set(TREE_X - 2, 16, dim(sc, 0.6));
        cv.set(TREE_X + 2, 16, dim(sc, 0.6));
        cv.set(TREE_X, 14, dim(sc, 0.6));
      }

      // presents
      cv.rect(47, 54, 50, 57, 0xb03030);
      cv.rect(49, 54, 49, 57, 0xe8c547);
      cv.rect(47, 55, 50, 55, 0xe8c547);
      cv.set(48, 53, 0xe8c547);
      cv.set(50, 53, 0xe8c547);
      cv.rect(59, 53, 62, 57, 0x2f5fa8);
      cv.rect(61, 53, 61, 57, 0xf2f2f2);
      cv.rect(59, 55, 62, 55, 0xf2f2f2);
      cv.set(60, 52, 0xf2f2f2);
      cv.set(62, 52, 0xf2f2f2);
    }

    function drawCat(cv) {
      let frame = Math.floor(t / 1.6) % 2 === 0 ? CAT_A : CAT_B;
      if (catTail > 0) frame = CAT_TAIL_UP;
      cv.sprite(frame, 6, 55, CAT_PAL);
    }

    // Warm light from the fire spilling over the room.
    function drawGlow(cv, day) {
      const strength = fireLevel * 0.38 * (1 - 0.55 * day);
      const cx = 31.5;
      const cy = 50;
      const r = 32;
      for (let y = 14; y < 64; y++) {
        for (let x = 0; x < 64; x++) {
          if (x >= OPEN.x0 && x <= OPEN.x1 && y >= OPEN.y0 && y <= OPEN.y1) continue;
          if (x >= SKY_BOX.x0 && x <= SKY_BOX.x1 && y >= SKY_BOX.y0 && y <= SKY_BOX.y1) continue;
          const d = Math.hypot(x - cx, (y - cy) * 1.2);
          if (d >= r) continue;
          const k = 1 - d / r;
          cv.blend(x, y, COL.glow, strength * k * k);
        }
      }
    }

    function drawGlobe(cv) {
      for (const p of globe) {
        const fade = Math.min(1, (p.life - p.age) / 1.5);
        cv.blend(p.x, p.y, COL.snow, 0.9 * fade);
      }
    }

    function draw(cv, ctx) {
      const hf = ctx.hourFloat;
      const day = daylight(hf);

      drawRoom(cv, day);
      drawWindow(cv, hf, day);
      drawFireplace(cv);
      drawTree(cv);
      drawCat(cv);
      drawGlow(cv, day);
      drawGlobe(cv);

      PC.drawTime(cv, ctx.hours, ctx.minutes, {
        y: 3,
        color: COL.clock,
        shadow: COL.clockShadow,
        colon: ctx.ms < 500,
        hour12: ctx.hour12,
      });
    }

    return { update, draw };
  }

  PC.registerTheme({ id: 'winter', name: 'Winter cabin', create });
})(window.PixelClock);
