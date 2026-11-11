/*
 * Autumn forest: orange, red and yellow trees, leaves drifting down and
 * blowing about in gusts, a leaf pile, a winding path, and a squirrel that
 * scampers across now and then. Harvest moon at night.
 * Shake: a big gust swirls a flurry of leaves.
 */
(function (PC) {
  'use strict';

  const { mix, dim, clamp, skyAt, daylight, vgradient } = PC;
  // PC.hash can return negatives (missing >>> 0 before the modulo); keep it in [0, n).
  const hash = (x, y, n) => ((PC.hash(x, y, n) % n) + n) % n;

  // [dark, mid, light, highlight] per canopy colour
  const CANOPY = [
    [0x9a3c12, 0xd2601c, 0xf08a24, 0xffbe5a], // orange
    [0x6e1712, 0xa8261c, 0xd84a2a, 0xf47a50], // red
    [0x9a7414, 0xd0a020, 0xf0c838, 0xfff07a], // yellow
  ];
  const LEAF_COLORS = [0xf08a24, 0xd84a2a, 0xf0c838, 0xd2601c, 0xa8261c, 0xffbe5a];

  const TREES = [
    { blobs: [[7, 28, 8], [15, 31, 6], [2, 34, 5], [11, 22, 5]], trunk: [6, 33, 8, 53], pal: 0, far: false },
    { blobs: [[56, 28, 7], [61, 33, 5], [49, 33, 5], [55, 22, 5]], trunk: [54, 33, 56, 54], pal: 1, far: false },
    { blobs: [[27, 38, 3], [28, 36, 2]], trunk: [27, 40, 27, 46], pal: 2, far: true },
    { blobs: [[37, 39, 2], [37, 37, 2]], trunk: [37, 41, 37, 46], pal: 0, far: true },
  ];

  const NIGHT_TINT = 0x1a1a44;
  const COL = {
    trunk: 0x4a2c18,
    trunkLight: 0x6a4026,
    hillDay: 0x9a8a6a,
    hillNight: 0x3a3048,
    grassFar: 0x9a8c3e,
    grassNear: 0x5e5c22,
    path: 0xd0ac7c,
    pathEdge: 0x9c7a4c,
    moon: 0xffc46a,
    moonShade: 0xe8a850,
    sun: 0xfff2c0,
    clock: 0xffe2b0,
    clockShadow: 0x1a0e08,
  };

  // Squirrel facing right; mirrored when running left.
  const SQ_STAND = ['.LT..E.', 'LT...HK', 'TT.BBHH', '.TBBWB.', '...F.F.'];
  const SQ_RUN_A = ['.......', '.LT..E.', 'LT.BBHK', 'TTBBBHH', '..F...F'];
  const SQ_RUN_B = ['.......', '.LT..E.', 'LT.BBHK', 'TTBBBHH', '...FF..'];
  const SQ_TWITCH = ['LT...E.', 'LT...HK', '.T.BBHH', '.TBBWB.', '...F.F.'];
  const SQ_PAL = { T: 0x9a5a2a, L: 0xd8a070, B: 0x6a3c1c, H: 0x6a3c1c, W: 0xd8a070, E: 0x4a2810, K: 0x0a0402, F: 0x4a2810 };
  const mirror = (rows) => rows.map((r) => r.split('').reverse().join(''));

  const groundY = (x) => 46 + Math.round(Math.sin(x * 0.08 + 2) * 1.5);
  const hillY = (x) => 40 + Math.round(Math.sin(x * 0.12) * 2 + Math.sin(x * 0.31 + 1));
  const pathCenter = (y) => 30 + Math.sin(y * 0.35) * 3;

  function create(rng) {
    let t = 0;
    const bg = new PC.Canvas();
    let bgKey = -1;
    const skyMask = new Uint8Array(64 * 64);
    let stars = [];

    // ---------- static background, rebuilt every couple of minutes of clock time ----------
    function shadeNight(c, day) {
      return dim(mix(c, NIGHT_TINT, 0.3 * (1 - day)), 0.55 + 0.45 * day);
    }

    function canopyPixel(tree, x, y) {
      for (const [cx, cy, r] of tree.blobs) {
        const rr = r - 0.4 + hash(x, y, 4) * 0.3;
        const dx = x - cx;
        const dy = y - cy;
        if (dx * dx + dy * dy <= rr * rr) {
          const light = (-dx - dy) / r;
          let i = light > 0.7 ? 3 : light > 0 ? 2 : light > -0.6 ? 1 : 0;
          const n = hash(x * 7, y * 3, 9);
          if (n === 0) i = Math.max(0, i - 1);
          else if (n === 1) i = Math.min(3, i + 1);
          if (hash(x * 3, y * 5, 13) === 0) i = 0; // little gaps for depth
          return CANOPY[tree.pal][i];
        }
      }
      return -1;
    }

    function buildBackground(hf) {
      const day = daylight(hf);
      const night = 1 - day;
      const [top, bottom] = skyAt(hf);
      vgradient(bg, 0, 0, 63, 47, top, bottom);
      skyMask.fill(0);
      for (let y = 0; y < 48; y++) for (let x = 0; x < 64; x++) skyMask[y * 64 + x] = 1;

      // sun by day, harvest moon by night
      if (day > 0) {
        for (let y = 14; y <= 26; y++) {
          for (let x = 34; x <= 46; x++) {
            const d = Math.hypot(x - 40, y - 20);
            if (d <= 2.6) bg.set(x, y, mix(bg.get(x, y), COL.sun, day));
            else if (d < 6) bg.blend(x, y, COL.sun, day * 0.25 * (1 - (d - 2.6) / 3.4));
          }
        }
      }
      if (night > 0) {
        for (let y = 12; y <= 32; y++) {
          for (let x = 30; x <= 54; x++) {
            const d = Math.hypot(x - 42, y - 22);
            if (d <= 6.2) {
              let c = COL.moon;
              if ((x === 40 && y === 20) || (x === 44 && y === 24) || (x === 45 && y === 19) || (x === 39 && y === 25)) c = COL.moonShade;
              if (x - 42 + (y - 22) > 5) c = COL.moonShade;
              bg.set(x, y, mix(bg.get(x, y), c, night));
            } else if (d < 11) {
              bg.blend(x, y, 0xff9a40, night * 0.22 * (1 - (d - 6.2) / 4.8));
            }
          }
        }
      }

      // far hills
      const hill = mix(COL.hillNight, COL.hillDay, day);
      for (let x = 0; x < 64; x++) {
        for (let y = hillY(x); y < 48; y++) {
          bg.set(x, y, hash(x, y, 6) === 0 ? dim(hill, 0.9) : hill);
          skyMask[y * 64 + x] = 0;
        }
      }

      // trees: far ones first, then the two big ones
      for (const tree of [TREES[2], TREES[3], TREES[0], TREES[1]]) {
        const [tx0, ty0, tx1, ty1] = tree.trunk;
        const tint = (c) => shadeNight(tree.far ? mix(c, hill, 0.35) : c, day);
        for (let y = ty0; y <= ty1; y++) {
          for (let x = tx0; x <= tx1; x++) {
            bg.set(x, y, tint(x === tx0 ? COL.trunkLight : COL.trunk));
            skyMask[y * 64 + x] = 0;
          }
        }
        if (!tree.far) {
          // a couple of branches poking out
          bg.set(tx0 - 1, ty0 + 3, tint(COL.trunk));
          bg.set(tx1 + 1, ty0 + 5, tint(COL.trunk));
        }
        let minX = 64;
        let maxX = 0;
        let minY = 64;
        let maxY = 0;
        for (const [cx, cy, r] of tree.blobs) {
          minX = Math.min(minX, cx - r - 1);
          maxX = Math.max(maxX, cx + r + 1);
          minY = Math.min(minY, cy - r - 1);
          maxY = Math.max(maxY, cy + r + 1);
        }
        for (let y = Math.max(14, minY); y <= maxY; y++) {
          for (let x = Math.max(0, minX); x <= Math.min(63, maxX); x++) {
            const c = canopyPixel(tree, x, y);
            if (c >= 0) {
              bg.set(x, y, tint(c));
              skyMask[y * 64 + x] = 0;
            }
          }
        }
      }

      // ground, path, scattered leaves
      for (let x = 0; x < 64; x++) {
        for (let y = groundY(x); y < 64; y++) {
          const isTrunk = TREES.some((tr) => x >= tr.trunk[0] && x <= tr.trunk[2] && y <= tr.trunk[3] && y >= tr.trunk[1]);
          if (isTrunk) continue;
          const depth = (y - 44) / 19; // 0 far .. 1 near
          let c = mix(COL.grassFar, COL.grassNear, depth);
          if (hash(x, y, 7) === 0) c = dim(c, 0.85);
          const pc = pathCenter(y);
          const half = 0.5 + depth * 5;
          const d = Math.abs(x - pc);
          if (d <= half - 1) c = mix(COL.path, COL.pathEdge, hash(x, y * 2, 5) === 0 ? 0.6 : depth * 0.3);
          else if (d <= half) c = COL.pathEdge;
          else if (hash(x * 5, y * 7, 19) === 0) c = LEAF_COLORS[hash(x, y, LEAF_COLORS.length)];
          bg.set(x, y, shadeNight(c, day));
          skyMask[y * 64 + x] = 0;
        }
      }

      // leaf pile
      for (let x = 10; x <= 24; x++) {
        const k = (x - 17) / 7.5;
        const h = Math.round(5 * (1 - k * k));
        for (let y = 58 - h; y <= 58; y++) {
          const i = hash(x * 3, y * 11, 6);
          const base = LEAF_COLORS[i];
          const lit = y === 58 - h ? mix(base, 0xffe0a0, 0.25) : y === 58 ? dim(base, 0.7) : base;
          bg.set(x, y, shadeNight(lit, day));
        }
      }

      // star positions: only in open sky
      stars = [];
      for (let i = 0; i < 26; i++) {
        const x = hash(i, 17, 64);
        const y = hash(i, 29, 36);
        if (skyMask[y * 64 + x] && !(x >= 30 && x <= 54 && y >= 12 && y <= 32)) stars.push([x, y, i]);
      }
    }

    // ---------- falling leaves ----------
    let wind = 1.5;
    let windTarget = 1.5;
    let gustTimer = 6 + rng.float() * 6;
    let gustLeft = 0;

    function spawnLeaf(leaf) {
      const tree = TREES[rng.int(2)];
      const [cx, cy, r] = tree.blobs[rng.int(tree.blobs.length)];
      const c = LEAF_COLORS[rng.int(LEAF_COLORS.length)];
      leaf.x = cx + (rng.float() - 0.5) * r * 1.6;
      leaf.y = cy + (rng.float() - 0.3) * r;
      leaf.vy = 3.5 + rng.float() * 3.5;
      leaf.ph = rng.float() * 6.283;
      leaf.col = c;
      leaf.col2 = dim(c, 0.65);
      leaf.landY = 50 + rng.int(12);
      leaf.rest = 0;
      leaf.landed = false;
      return leaf;
    }
    const leaves = [];
    for (let i = 0; i < 12; i++) {
      const l = spawnLeaf({});
      l.y = 18 + rng.float() * 30;
      leaves.push(l);
    }

    const flurry = [];
    function spawnFlurry() {
      for (let i = 0; i < 48; i++) {
        const c = LEAF_COLORS[rng.int(LEAF_COLORS.length)];
        flurry.push({
          x: -6 + rng.float() * 52,
          y: 16 + rng.float() * 44,
          vx: 10 + rng.float() * 16,
          vy: (rng.float() - 0.6) * 18,
          ph: rng.float() * 6.283,
          col: c,
          col2: dim(c, 0.65),
          life: 3 + rng.float() * 2,
          age: 0,
        });
      }
      windTarget = 26;
      gustLeft = 3;
    }

    // ---------- squirrel ----------
    const sq = { state: 'away', timer: 4 + rng.float() * 6, x: -8, dir: 1, stopX: 30, frameT: 0 };

    function updateSquirrel(dt) {
      sq.frameT += dt;
      if (sq.state === 'away') {
        sq.timer -= dt;
        if (sq.timer <= 0) {
          sq.dir = rng.chance(0.5) ? 1 : -1;
          sq.x = sq.dir > 0 ? -8 : 64;
          sq.stopX = 27 + rng.int(18); // stop on or near the path, clear of the leaf pile
          sq.state = 'run';
          sq.stopped = false;
        }
      } else if (sq.state === 'run') {
        sq.x += sq.dir * 22 * dt;
        if (!sq.stopped && ((sq.dir > 0 && sq.x >= sq.stopX) || (sq.dir < 0 && sq.x <= sq.stopX))) {
          sq.state = 'stop';
          sq.timer = 1.5 + rng.float() * 2;
          sq.stopped = true;
        }
        if (sq.x < -10 || sq.x > 66) {
          sq.state = 'away';
          sq.timer = 8 + rng.float() * 10;
        }
      } else if (sq.state === 'stop') {
        sq.timer -= dt;
        if (sq.timer <= 0) sq.state = 'run';
      }
    }

    function drawSquirrel(cv, amb) {
      if (sq.state === 'away') return;
      let rows;
      if (sq.state === 'run') rows = Math.floor(sq.frameT * 12) % 2 ? SQ_RUN_A : SQ_RUN_B;
      else rows = Math.floor(sq.frameT * 3) % 4 === 0 ? SQ_TWITCH : SQ_STAND;
      if (sq.dir < 0) rows = mirror(rows);
      const pal = {};
      for (const k in SQ_PAL) pal[k] = dim(SQ_PAL[k], amb);
      cv.sprite(rows, Math.round(sq.x), 56, pal);
    }

    // ---------- update / draw ----------
    function update(dt, ctx) {
      t += dt;

      gustTimer -= dt;
      if (gustTimer <= 0 && gustLeft <= 0) {
        gustLeft = 1.5 + rng.float() * 1.2;
        windTarget = 12 + rng.float() * 6;
        gustTimer = 7 + rng.float() * 8;
      }
      if (gustLeft > 0) {
        gustLeft -= dt;
        if (gustLeft <= 0) windTarget = 1.5;
      }
      wind += (windTarget - wind) * Math.min(1, dt * 1.5);

      for (const l of leaves) {
        if (l.landed) {
          l.rest -= dt;
          if (wind > 8 && rng.chance(dt * 1.5)) {
            l.landed = false; // gust picks it back up
            l.vy = -4;
          }
          if (l.rest <= 0) spawnLeaf(l);
          continue;
        }
        l.vy = Math.min(l.vy + 4 * dt, 7);
        l.x += (wind + Math.sin(t * 2 + l.ph) * 4) * dt;
        l.y += l.vy * dt;
        if (l.y >= l.landY) {
          l.y = l.landY;
          l.landed = true;
          l.rest = 2 + rng.float() * 4;
        }
        if (l.x > 66 || l.x < -3) spawnLeaf(l);
      }

      if (ctx.shake) spawnFlurry();
      for (let i = flurry.length - 1; i >= 0; i--) {
        const f = flurry[i];
        f.age += dt;
        f.vx *= 0.99;
        f.vy += Math.sin(t * 3 + f.ph) * 30 * dt + 3 * dt;
        f.x += f.vx * dt;
        f.y += f.vy * dt;
        if (f.age > f.life || f.x > 68) flurry.splice(i, 1);
      }

      updateSquirrel(dt);
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
      const night = 1 - day;
      if (night > 0.2) {
        for (const [x, y, i] of stars) {
          const tw = 0.55 + 0.45 * Math.sin(t * (1.5 + (i % 5) * 0.4) + i * 2.1);
          cv.blend(x, y, 0xdfe6ff, (night - 0.2) * tw);
        }
      }

      const amb = 0.55 + 0.45 * day;
      const leafAmb = Math.max(amb, 0.8); // moving leaves catch the light
      for (const l of leaves) {
        const c = l.landed || Math.sin(t * 6 + l.ph) > 0 ? l.col : l.col2;
        cv.set(l.x, l.y, dim(c, l.landed ? amb : leafAmb));
      }
      drawSquirrel(cv, Math.max(amb, 0.7));
      for (const f of flurry) {
        const c = Math.sin(t * 8 + f.ph) > 0 ? f.col : f.col2;
        const fade = clamp((f.life - f.age) / 1, 0, 1);
        cv.blend(f.x, f.y, dim(c, leafAmb), fade);
      }

      PC.drawClock(cv, ctx, COL.clock, COL.clockShadow);
    }

    return { update, draw };
  }

  PC.registerTheme({ id: 'autumn', name: 'Autumn forest', kind: 'season', create });
})(globalThis.PixelClock);
