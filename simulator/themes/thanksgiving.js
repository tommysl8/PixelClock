/*
 * Harvest field: a golden field with a red barn and silo, a scarecrow, hay
 * bales, pumpkins and gourds, corn stalks swaying in the breeze, and a
 * turkey that bobs, pecks and puffs up its tail. Day and night follow the clock.
 * Shake: the turkey flaps its wings and feathers fly.
 */
(function (PC) {
  'use strict';

  const { mix, dim, clamp, skyAt, daylight, vgradient } = PC;
  // PC.hash can return negatives (missing >>> 0 before the modulo); keep it in [0, n).
  const hash = (x, y, n) => ((PC.hash(x, y, n) % n) + n) % n;

  // A warmer, golden sky than the default.
  const SKY_GOLD = [
    [0, 0x0a0c24, 0x1c1a3a],
    [5.5, 0x0a0c24, 0x1c1a3a],
    [7, 0x4a4a8c, 0xf2a66b],
    [9, 0x7ab0de, 0xf2d9a8],
    [15, 0x7ab0de, 0xf2d9a8],
    [17, 0x6a5a9a, 0xf59a50],
    [18.5, 0x1c1840, 0x4a2a50],
    [24, 0x0a0c24, 0x1c1a3a],
  ];

  const NIGHT_TINT = 0x1a1a44;
  const COL = {
    sun: 0xffd870,
    moon: 0xeef0ff,
    hillDay: 0x8e9a48,
    hillNight: 0x3a3450,
    barn: 0xa8322a,
    barnShade: 0x7e2420,
    roof: 0x4a2620,
    trim: 0xf0ead8,
    silo: 0x9a9aa4,
    siloShade: 0x6e6e7a,
    loftLit: 0xffcc4a,
    field: [0xc49040, 0xae7c34, 0x8a6228],
    post: 0x6a4a20,
    hat: 0x6a4a20,
    face: 0xe8c070,
    shirt: 0xb03a2a,
    straw: 0xf0d060,
    bale: 0xf0d070,
    baleLight: 0xfff0a0,
    baleShade: 0xc8a040,
    baleEdge: 0x7a5a20,
    twine: 0x9a6a28,
    stalk: 0x7a6a28,
    leaf: 0xb8a048,
    leafDark: 0x5e5020,
    tassel: 0xf0dc90,
    corn: 0xf0c840,
    clock: 0xfff0d0,
    clockShadow: 0x2a1408,
  };

  const TAIL = { band: 0x8e4a20, bandAlt: 0x6e3414, inner: 0x4e2a10, dark: 0x3a1e0a, edge: 0x24120a, tip: 0xf4e6c4 };
  const TURKEY = { body: 0x3e220e, sheen: 0x6e4826, wing: 0x5e3a1c, neck: 0xa8c8e4, head: 0xd0e4f6, eye: 0x0a0a0a, beak: 0xf0c040, wattle: 0xe02828, leg: 0xe08a30 };
  const LEAF_COLORS = [0xf08a24, 0xd84a2a, 0xf0c838, 0xa8261c];

  const PUMPKIN = ['...s...', '.oOOOo.', 'oOOoOOo', 'oOOoOOo', 'oOOoOOo', '.ooooo.'];
  const PUMPKIN_SM = ['.s..', 'oOOo', 'oOOo', '.oo.'];
  const GOURD_WHITE = ['.s.', 'WWW', 'WwW', '.W.'];
  const GOURD_STRIPE = ['..s', '.YG', 'YGY', 'GYG', '.G.'];

  const CORN = [
    { x: 2, base: 51, h: 22 },
    { x: 6, base: 52, h: 24 },
    { x: 10, base: 51, h: 20 },
    { x: 13, base: 53, h: 23 },
  ];

  function create(rng) {
    let t = 0;
    const bg = new PC.Canvas();
    let bgKey = -1;
    let stars = [];
    const skyMask = new Uint8Array(64 * 64);
    let amb = 1;
    let night = 0;

    function shade(c, day) {
      return dim(mix(c, NIGHT_TINT, 0.3 * (1 - day)), 0.5 + 0.5 * day);
    }

    function solid(x, y, c) {
      bg.set(x, y, c);
      if (x >= 0 && x < 64 && y >= 0 && y < 64) skyMask[y * 64 + x] = 0;
    }

    // ---------- static background ----------
    function buildBackground(hf) {
      const day = daylight(hf);
      const nt = 1 - day;
      const [top, bottom] = skyAt(hf, SKY_GOLD);
      vgradient(bg, 0, 0, 63, 42, top, bottom);
      skyMask.fill(1);
      const S = (c) => shade(c, day);

      // sun by day, moon by night
      for (let y = 14; y <= 28; y++) {
        for (let x = 6; x <= 24; x++) {
          const d = Math.hypot(x - 15, y - 20);
          if (day > 0) {
            if (d <= 2.6) bg.set(x, y, mix(bg.get(x, y), COL.sun, day));
            else if (d < 7) bg.blend(x, y, COL.sun, day * 0.25 * (1 - (d - 2.6) / 4.4));
          }
          if (nt > 0 && d <= 2.8) {
            const crescent = Math.hypot(x - 16.5, y - 19) > 2.4;
            if (crescent) bg.set(x, y, mix(bg.get(x, y), COL.moon, nt));
          }
        }
      }

      // rolling hills
      const hill = mix(COL.hillNight, COL.hillDay, day);
      for (let x = 0; x < 64; x++) {
        const hy = 37 + Math.round(Math.sin(x * 0.1 + 1) * 1.5 + Math.sin(x * 0.27) * 0.8);
        for (let y = hy; y <= 42; y++) solid(x, y, hash(x, y, 7) === 0 ? dim(hill, 0.9) : hill);
      }

      // field with furrows
      for (let y = 42; y < 64; y++) {
        const depth = (y - 42) / 21;
        for (let x = 0; x < 64; x++) {
          const row = Math.floor(Math.pow(y - 41, 0.8));
          let c = COL.field[row % 2 === 0 ? 0 : 1];
          if (hash(x, y, 9) === 0) c = COL.field[2];
          solid(x, y, S(mix(c, COL.field[2], depth * 0.35)));
        }
      }

      // barn with gambrel roof
      for (let y = 30; y <= 41; y++) for (let x = 45; x <= 59; x++) solid(x, y, S(x >= 56 ? COL.barnShade : COL.barn));
      for (let y = 23; y <= 29; y++) {
        const half = y < 26 ? 2 + (y - 23) * 2 : 7 + (y - 26) * 0.5;
        for (let x = Math.round(52 - half); x <= Math.round(52 + half); x++) solid(x, y, S(COL.roof));
      }
      // door with white X trim
      for (let y = 34; y <= 41; y++) {
        for (let x = 49; x <= 55; x++) {
          const edge = x === 49 || x === 55 || y === 34;
          const diag = x - 49 === Math.round(((y - 34) * 6) / 7) || 55 - x === Math.round(((y - 34) * 6) / 7);
          solid(x, y, S(edge || diag ? COL.trim : COL.barnShade));
        }
      }
      // hayloft window with trim, lit at night
      for (let y = 26; y <= 29; y++) for (let x = 51; x <= 53; x++) solid(x, y, S(COL.trim));
      const loft = mix(0x2a1410, COL.loftLit, nt);
      solid(52, 27, loft);
      solid(52, 28, loft);

      // silo
      for (let y = 25; y <= 41; y++) for (let x = 60; x <= 63; x++) solid(x, y, S(x >= 62 ? COL.siloShade : COL.silo));
      solid(61, 24, S(COL.silo));
      solid(62, 24, S(COL.siloShade));
      for (let y = 27; y <= 41; y += 4) for (let x = 60; x <= 63; x++) solid(x, y, S(COL.siloShade));

      // scarecrow in the distance
      for (let y = 31; y <= 44; y++) solid(29, y, S(COL.post));
      for (let x = 25; x <= 33; x++) solid(x, 34, S(COL.shirt));
      solid(24, 34, S(COL.straw));
      solid(34, 34, S(COL.straw));
      for (let y = 33; y <= 38; y++) for (let x = 27; x <= 31; x++) solid(x, y, S((x + y) % 3 === 0 ? 0x7a2418 : COL.shirt));
      solid(28, 39, S(COL.straw));
      solid(30, 39, S(COL.straw));
      solid(28, 31, S(COL.face));
      solid(29, 31, S(COL.face));
      solid(30, 31, S(COL.face));
      solid(29, 32, S(COL.face));
      for (let x = 26; x <= 32; x++) solid(x, 30, S(COL.hat));
      for (let x = 28; x <= 30; x++) solid(x, 29, S(COL.hat));
      solid(28, 28, S(COL.hat));
      solid(29, 28, S(COL.hat));
      solid(30, 28, S(COL.hat));

      // hay bales
      const bale = (x0, y0, x1, y1) => {
        for (let y = y0; y <= y1; y++) {
          for (let x = x0; x <= x1; x++) {
            let c = COL.bale;
            if (y === y0) c = COL.baleLight;
            else if (y === y1 - 1) c = COL.baleShade;
            else if (hash(x, y, 5) === 0) c = COL.baleShade;
            if ((x - x0) % 4 === 2 && y !== y0) c = COL.twine;
            if (y === y1 || x === x0 || x === x1) c = COL.baleEdge;
            solid(x, y, S(c));
          }
        }
      };
      bale(15, 52, 26, 57);
      bale(17, 47, 24, 51);

      // pumpkins and gourds
      const pal = { O: S(0xe0701a), o: S(0xa84a10), s: S(0x3a6a20), W: S(0xf0ead8), w: S(0xc8c0a8), Y: S(0xe8c040), G: S(0x7a9a30) };
      bg.sprite(PUMPKIN, 30, 56, pal);
      bg.sprite(PUMPKIN_SM, 38, 58, pal);
      bg.sprite(GOURD_WHITE, 26, 58, pal);
      bg.sprite(GOURD_STRIPE, 42, 57, pal);

      stars = [];
      for (let i = 0; i < 30 && stars.length < 18; i++) {
        const x = hash(i, 51, 64);
        const y = 14 + hash(i, 53, 20);
        if (skyMask[y * 64 + x] && Math.hypot(x - 15, y - 20) > 5) stars.push([x, y, i]);
      }
    }

    // ---------- turkey ----------
    const turkey = { peck: 0, nextPeck: 3 + rng.float() * 4, puff: 0, nextPuff: 6 + rng.float() * 6, flap: 0 };
    const feathers = [];

    function drawTurkey(cv) {
      const cx = 50;
      const cy = 51;
      const puffAmt = turkey.flap > 0 ? 2 : turkey.puff > 0 ? 1.5 * Math.min(1, turkey.puff * 3, (1.4 - turkey.puff) * 3 + 1) : 0;
      const R = 8 + puffAmt + Math.sin(t * 2) * 0.2;
      const s = (c) => dim(c, amb);

      // tail fan (upper half circle)
      for (let y = Math.floor(cy - R); y <= cy + 1; y++) {
        for (let x = Math.floor(cx - R); x <= Math.ceil(cx + R); x++) {
          const dx = x - cx;
          const dy = y - cy;
          const d = Math.hypot(dx, dy);
          if (d > R) continue;
          const k = d / R;
          const ang = Math.atan2(-dy, dx);
          const feather = Math.floor((ang / Math.PI) * 9);
          let c;
          if (k > 0.86) c = TAIL.tip;
          else if (k > 0.74) c = TAIL.edge;
          else if (k > 0.45) c = feather % 2 ? TAIL.band : TAIL.bandAlt;
          else c = feather % 2 ? TAIL.inner : TAIL.dark;
          cv.set(x, y, s(c));
        }
      }

      // body
      for (let y = 50; y <= 57; y++) {
        for (let x = 46; x <= 56; x++) {
          const ex = (x - 51) / 4.6;
          const ey = (y - 54) / 3.6;
          if (ex * ex + ey * ey > 1) continue;
          let c = TURKEY.body;
          if (y <= 51) c = TURKEY.sheen;
          else if (ex < -0.1 && ey > -0.3 && (x + y) % 2 === 0) c = TURKEY.wing;
          cv.set(x, y, s(c));
        }
      }

      // wings flapping on shake
      if (turkey.flap > 0) {
        const up = Math.floor(t * 10) % 2 === 0;
        for (let i = 0; i < 4; i++) {
          const wy = up ? 50 - i : 52 + Math.floor(i / 2);
          const c = i === 3 ? TAIL.tip : TURKEY.sheen;
          cv.set(46 - i, wy, s(c));
          cv.set(46 - i, wy + 1, s(TURKEY.body));
          cv.set(56 + i, wy, s(c));
          cv.set(56 + i, wy + 1, s(TURKEY.body));
        }
      }

      // legs
      for (let y = 57; y <= 59; y++) {
        cv.set(49, y, s(TURKEY.leg));
        cv.set(53, y, s(TURKEY.leg));
      }
      cv.set(48, 59, s(TURKEY.leg));
      cv.set(54, 59, s(TURKEY.leg));

      // neck and head: bob while idle, dip down while pecking
      const bob = Math.floor(t * 2.2) % 2;
      const p = turkey.peck > 0 ? Math.sin(Math.min(1, turkey.peck / 0.7) * Math.PI) : 0;
      const hx = 56 + bob + Math.round(p * 2);
      const hy = 47 + Math.round(p * 7);
      const nx = 55;
      for (let i = 0; i <= 3; i++) {
        const y = Math.round(51 - i * ((51 - hy - 1) / 3));
        const x = Math.round(nx + (i * (hx - nx)) / 3);
        cv.set(x, y, s(TURKEY.neck));
      }
      cv.rect(hx, hy, hx + 1, hy + 1, s(TURKEY.head));
      cv.set(hx + 1, hy, s(TURKEY.eye));
      cv.set(hx + 2, hy + 1, s(TURKEY.beak));
      cv.set(hx + 1, hy + 2, s(TURKEY.wattle));
      cv.set(hx + 2, hy + 2, s(TURKEY.wattle));
      cv.set(hx + 1, hy + 3, s(TURKEY.wattle));
    }

    // ---------- drifting leaves ----------
    const leaves = [];
    function spawnLeaf(l, anywhere) {
      l.x = anywhere ? rng.float() * 64 : -2;
      l.y = 15 + rng.float() * 30;
      l.vx = 4 + rng.float() * 5;
      l.vy = 2 + rng.float() * 3;
      l.ph = rng.float() * 6.283;
      l.col = LEAF_COLORS[rng.int(LEAF_COLORS.length)];
      return l;
    }
    for (let i = 0; i < 6; i++) leaves.push(spawnLeaf({}, true));

    function update(dt, ctx) {
      t += dt;

      if (turkey.peck > 0) turkey.peck -= dt;
      else if ((turkey.nextPeck -= dt) <= 0) {
        turkey.peck = 0.7;
        turkey.nextPeck = 3 + rng.float() * 5;
      }
      if (turkey.puff > 0) turkey.puff -= dt;
      else if ((turkey.nextPuff -= dt) <= 0) {
        turkey.puff = 1.4;
        turkey.nextPuff = 8 + rng.float() * 8;
      }
      if (turkey.flap > 0) turkey.flap -= dt;

      if (ctx.shake) {
        turkey.flap = 1.6;
        for (let i = 0; i < 16; i++) {
          const a = Math.PI * (0.15 + rng.float() * 0.7);
          feathers.push({
            x: 50 + Math.cos(a) * 9,
            y: 51 - Math.sin(a) * 9,
            vx: Math.cos(a) * (8 + rng.float() * 10),
            vy: -Math.sin(a) * (8 + rng.float() * 10),
            ph: rng.float() * 6.283,
            col: rng.chance(0.4) ? TAIL.tip : TAIL.band,
            life: 2.5 + rng.float() * 1.5,
            age: 0,
          });
        }
      }
      for (let i = feathers.length - 1; i >= 0; i--) {
        const f = feathers[i];
        f.age += dt;
        f.vx *= 0.95;
        f.vy = Math.min(f.vy * 0.95 + 10 * dt, 4);
        f.x += (f.vx + Math.sin(t * 4 + f.ph) * 4) * dt;
        f.y += f.vy * dt;
        if (f.age > f.life) feathers.splice(i, 1);
      }

      for (const l of leaves) {
        l.x += (l.vx + Math.sin(t * 2 + l.ph) * 3) * dt;
        l.y += l.vy * dt;
        if (l.x > 66 || l.y > 62) spawnLeaf(l, false);
      }
    }

    function drawCorn(cv) {
      CORN.forEach((c, i) => {
        const top = c.base - c.h;
        const sway = Math.sin(t * 1.3 + i * 1.7) * 1.4 + Math.sin(t * 0.7 + i) * 0.5;
        const off = (y) => Math.round(sway * Math.pow((c.base - y) / c.h, 1.5));
        for (let y = top; y <= c.base; y++) {
          const x = c.x + off(y);
          cv.set(x, y, dim(COL.stalk, amb));
          const k = y - top;
          if (k > 3 && k % 4 === 0) {
            // leaves alternate sides and droop
            const dir = (k / 4) % 2 ? 1 : -1;
            cv.set(x + dir, y, dim(COL.leaf, amb));
            cv.set(x + dir * 2, y + 1, dim(COL.leafDark, amb));
          }
        }
        // tassel
        const tx = c.x + off(top);
        cv.set(tx, top - 1, dim(COL.tassel, amb));
        cv.set(tx - 1, top, dim(COL.tassel, amb));
        cv.set(tx + 1, top, dim(COL.tassel, amb));
        // an ear of corn
        const ey = top + Math.floor(c.h * 0.55);
        const ex = c.x + off(ey) + (i % 2 ? 1 : -1);
        cv.set(ex, ey, dim(COL.corn, amb));
        cv.set(ex, ey + 1, dim(COL.corn, amb));
        cv.set(ex, ey + 2, dim(COL.leafDark, amb));
      });
    }

    function draw(cv, ctx) {
      const hf = ctx.hourFloat;
      const key = Math.floor(hf * 30);
      if (key !== bgKey) {
        bgKey = key;
        buildBackground(hf);
      }
      cv.px.set(bg.px);
      const day = daylight(hf);
      night = 1 - day;
      amb = 0.55 + 0.45 * day;

      if (night > 0.2) {
        for (const [x, y, i] of stars) {
          const tw = 0.55 + 0.45 * Math.sin(t * (1.5 + (i % 5) * 0.4) + i * 2.1);
          cv.blend(x, y, 0xdfe6ff, (night - 0.2) * tw);
        }
      }

      drawCorn(cv);
      drawTurkey(cv);

      for (const l of leaves) cv.set(l.x, l.y, dim(l.col, Math.max(amb, 0.8)));
      for (const f of feathers) {
        const a = clamp((f.life - f.age) / 0.8, 0, 1);
        cv.blend(f.x, f.y, dim(f.col, Math.max(amb, 0.8)), a);
        cv.blend(f.x, f.y + 1, dim(TAIL.band, Math.max(amb, 0.8)), a); // quill
      }

      PC.drawClock(cv, ctx, COL.clock, COL.clockShadow);
    }

    return { update, draw };
  }

  PC.registerTheme({ id: 'thanksgiving', name: 'Harvest field', kind: 'holiday', create });
})(globalThis.PixelClock);
