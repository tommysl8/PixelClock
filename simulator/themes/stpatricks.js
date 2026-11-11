/*
 * St. Patrick's Day: rolling green hills, a rainbow arcing down into a pot of
 * gold with glinting coins, swaying shamrocks and drifting clouds. At night
 * the rainbow fades, stars come out and the gold glows.
 * Shake: coins burst out of the pot and tumble back in.
 */
(function (PC) {
  'use strict';

  const { mix, dim, clamp, skyAt, daylight, hash } = PC;

  const COL = {
    hillFar: 0x4f9a5c,
    hillMid: 0x2f9a3e,
    hillNear: 0x1e7429,
    grassDark: 0x165a20,
    grassLight: 0x47b552,
    cloud: 0xf4f8ff,
    cloudShade: 0xc9d6e8,
    gold: 0xf2c230,
    goldDark: 0xc99a1e,
    goldShine: 0xfff2a0,
    glow: 0xffc940,
    leaf: 0x4cc85a,
    leafLight: 0x8ae67e,
    leafDark: 0x2a9a3c,
    stem: 0x1d6a2a,
    clock: 0xf4ffe8,
    clockShadow: 0x0b2a12,
  };

  // Rainbow, outermost band first.
  const RAINBOW = [0xe8413c, 0xf39237, 0xf7d23e, 0x4cb84a, 0x3d8fd6, 0x7d5bc4];
  const RB = { cx: 27, cy: 50, r: 28 };

  // Pot of gold, top-left at (POT_X, POT_Y).
  const POT_X = 45;
  const POT_Y = 45;
  const POT = [
    '....gGyGg....',
    '..gGGGgGGGg..',
    '.gGGyGGGGyGg.',
    'RRRRRRRRRRRRR',
    '.rrrrrrrrrrr.',
    '.BBBBBBBBBhB.',
    'BBBBBBBBBBhhB',
    'BBBBBBBBBBhhB',
    '.BBBBBBBBBhB.',
    '..BBBBBBBBB..',
    '..L.......L..',
  ];
  const POT_PAL = {
    g: COL.goldDark,
    G: COL.gold,
    y: COL.goldShine,
    R: 0x4a4a54,
    r: 0x16161a,
    B: 0x26262c,
    h: 0x5a5a66,
    L: 0x1c1c20,
  };

  // Three heart-shaped leaves on a stem.
  const SHAMROCK = [
    '..LL.LL..',
    '..GGGGG..',
    'LL.GGG.LL',
    'GGGGdGGGG',
    '.GGGdGGG.',
    '..G.s.G..',
    '....s....',
    '....s....',
  ];
  const SHAMROCK_PAL = { L: COL.leafLight, G: COL.leaf, d: COL.leafDark, s: COL.stem };

  const CLOUD = ['....www......', '..wwwwwww.ww.', '.wwwwwwwwwwww', 'wwwwwwwwwwwww', '.sssssssssss.'];

  const farY = (x) => 39 + Math.round(2.5 * Math.sin(x * 0.09 + 1));
  const midY = (x) => 46 + Math.round(2.5 * Math.sin(x * 0.12 + 3.2));
  const nearY = (x) => 55 + Math.round(1.5 * Math.sin(x * 0.16 + 0.5));

  function create(rng) {
    let t = 0;
    const clouds = [
      { x: 4, y: 16, v: 1.2 },
      { x: 40, y: 20, v: 0.8 },
    ];
    const coins = [];
    let glint = { x: 0, y: 0, age: 1, life: 0.3 };

    function update(dt, ctx) {
      t += dt;
      for (const c of clouds) {
        c.x += c.v * dt;
        if (c.x > 66) c.x = -14;
      }

      glint.age += dt;
      if (glint.age > glint.life && rng.chance(2.5 * dt)) {
        // pick a gold pixel in the heap
        const row = rng.int(3);
        const cols = POT[row];
        let x = rng.int(cols.length);
        for (let i = 0; i < cols.length && cols[x] === '.'; i++) x = (x + 1) % cols.length;
        glint = { x: POT_X + x, y: POT_Y + row, age: 0, life: 0.35 };
      }

      if (ctx.shake) {
        for (let i = 0; i < 18; i++) {
          const toPot = rng.chance(0.7);
          const vy = -(24 + rng.float() * 20);
          const g = 60;
          const flight = (2 * -vy) / g;
          const target = toPot ? POT_X + 3 + rng.float() * 7 : 4 + rng.float() * 36;
          coins.push({
            x: POT_X + 6.5,
            y: POT_Y + 1,
            vx: (target - (POT_X + 6.5)) / (toPot ? flight : flight * 1.25),
            vy,
            g,
            toPot,
            landed: 0,
            ph: rng.float() * 6.283,
          });
        }
      }
      for (let i = coins.length - 1; i >= 0; i--) {
        const c = coins[i];
        if (c.landed > 0) {
          c.landed += dt;
          if (c.landed > 3) coins.splice(i, 1);
          continue;
        }
        c.vy += c.g * dt;
        c.x += c.vx * dt;
        c.y += c.vy * dt;
        if (c.vy > 0) {
          if (c.toPot && c.y >= POT_Y + 1) coins.splice(i, 1);
          else if (!c.toPot && c.y >= nearY(Math.round(c.x)) - 1) {
            c.y = nearY(Math.round(c.x)) - 1;
            c.landed = 0.001;
          }
        }
      }
    }

    function drawSky(cv, hf, day) {
      const [top, bottom] = skyAt(hf);
      PC.vgradient(cv, 0, 0, 63, 50, top, bottom);
      PC.drawStars(cv, t, 1 - day, 0, 0, 63, 36, 26);
      const cc = mix(0x5a6070, COL.cloud, day);
      const cs = mix(0x3a4050, COL.cloudShade, day);
      for (const c of clouds) cv.sprite(CLOUD, Math.round(c.x), c.y, { w: cc, s: cs });
    }

    function drawHills(cv, day, which) {
      const k = 0.4 + 0.6 * day;
      if (which === 'far') {
        const c = dim(COL.hillFar, k);
        for (let x = 0; x < 64; x++) for (let y = farY(x); y <= 50; y++) cv.set(x, y, c);
      } else if (which === 'mid') {
        const c = dim(COL.hillMid, k);
        const cd = dim(COL.grassDark, k);
        for (let x = 0; x < 64; x++) {
          for (let y = midY(x); y < 64; y++) cv.set(x, y, hash(x, y, 9) === 0 ? cd : c);
        }
      } else {
        const c = dim(COL.hillNear, k);
        const cl = dim(COL.grassLight, k);
        const cd = dim(COL.grassDark, k);
        for (let x = 0; x < 64; x++) {
          const top = nearY(x);
          cv.set(x, top, cl);
          for (let y = top + 1; y < 64; y++) cv.set(x, y, hash(x, y, 6) === 0 ? cd : c);
          // grass tufts sway a little
          if (hash(x, 3, 4) === 0) cv.set(x + (Math.sin(t * 2 + x) > 0.3 ? 1 : 0), top - 1, cl);
        }
      }
    }

    function drawRainbow(cv, day) {
      const a = 0.12 + 0.73 * day;
      for (let y = RB.cy - RB.r; y <= RB.cy; y++) {
        for (let x = RB.cx - RB.r; x <= RB.cx + RB.r; x++) {
          const d = Math.hypot(x - RB.cx, y - RB.cy);
          const band = Math.floor(RB.r - d);
          if (band >= 0 && band < RAINBOW.length) cv.blend(x, y, RAINBOW[band], a);
        }
      }
    }

    function drawPot(cv, day) {
      const night = 1 - day;
      if (night > 0) {
        const flick = 0.85 + 0.15 * Math.sin(t * 5);
        const cx = POT_X + 6;
        const cy = POT_Y + 1;
        for (let y = cy - 9; y <= cy + 7; y++) {
          for (let x = cx - 11; x <= cx + 11; x++) {
            const d = Math.hypot(x - cx, (y - cy) * 1.3);
            if (d < 11) cv.blend(x, y, COL.glow, night * 0.35 * flick * (1 - d / 11) ** 2);
          }
        }
      }
      cv.sprite(POT, POT_X, POT_Y, POT_PAL);
      if (glint.age < glint.life) {
        const k = Math.sin((glint.age / glint.life) * Math.PI);
        cv.blend(glint.x, glint.y, 0xffffff, k);
        for (const [dx, dy] of [[1, 0], [-1, 0], [0, -1]]) cv.blend(glint.x + dx, glint.y + dy, 0xffffff, k * 0.6);
      }
    }

    function drawShamrocks(cv, day) {
      const k = 0.5 + 0.5 * day;
      const pal = {};
      for (const key in SHAMROCK_PAL) pal[key] = dim(SHAMROCK_PAL[key], k);
      [
        [2, 55, 0],
        [14, 57, 1.3],
        [29, 56, 2.1],
        [58, 57, 0.7],
      ].forEach(([x, y, ph]) => {
        const sway = Math.round(Math.sin(t * 1.4 + ph) * 0.9);
        cv.sprite(SHAMROCK.slice(0, 5), x + sway, y, pal);
        cv.sprite(SHAMROCK.slice(5), x, y + 5, pal);
      });
    }

    function drawCoins(cv) {
      for (const c of coins) {
        const fade = c.landed > 0 ? clamp(1 - (c.landed - 2) / 1, 0, 1) : 1;
        // coins spin: a 2x2 face, then a thin edge
        const wide = Math.sin(t * 14 + c.ph) > -0.3;
        if (wide) {
          cv.blend(c.x, c.y, COL.goldShine, fade);
          cv.blend(c.x + 1, c.y, COL.gold, fade);
          cv.blend(c.x, c.y + 1, COL.gold, fade);
          cv.blend(c.x + 1, c.y + 1, COL.goldDark, fade);
        } else {
          cv.blend(c.x, c.y, COL.goldShine, fade);
          cv.blend(c.x, c.y + 1, COL.goldDark, fade);
        }
      }
    }

    function draw(cv, ctx) {
      const hf = ctx.hourFloat;
      const day = daylight(hf);
      drawSky(cv, hf, day);
      drawHills(cv, day, 'far');
      drawRainbow(cv, day);
      drawHills(cv, day, 'mid');
      drawPot(cv, day);
      drawHills(cv, day, 'near');
      drawShamrocks(cv, day);
      drawCoins(cv);
      PC.drawClock(cv, ctx, COL.clock, COL.clockShadow);
    }

    return { update, draw };
  }

  PC.registerTheme({ id: 'stpatricks', name: "St. Patrick's Day", kind: 'holiday', create });
})(globalThis.PixelClock);
