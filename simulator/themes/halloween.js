/*
 * Haunted hill: a big full moon, a haunted house with flickering windows,
 * bats flapping across the sky, glowing jack-o'-lanterns, a little ghost,
 * tombstones and fog rolling along the ground. Always night.
 * Shake: lightning strikes and a swarm of bats bursts out of the house.
 */
(function (PC) {
  'use strict';

  const { mix, dim, clamp, vgradient } = PC;
  // PC.hash can return negatives (missing >>> 0 before the modulo); keep it in [0, n).
  const hash = (x, y, n) => ((PC.hash(x, y, n) % n) + n) % n;

  const COL = {
    skyTop: 0x120a26,
    skyBottom: 0x4a2460,
    moon: 0xf6d58a,
    moonShade: 0xdcb46a,
    moonGlow: 0xffb060,
    hill: 0x170c22,
    ground: 0x1c1028,
    tuft: 0x2a1a3e,
    house: 0x0a0610,
    window: 0xffcc4a,
    windowDim: 0x8a5a18,
    stone: 0x6a6a7e,
    stoneShade: 0x45455a,
    stoneLight: 0x9494ae,
    pumpkin: 0xe0701a,
    pumpkinShade: 0xa84a10,
    stem: 0x3a6a20,
    candle: 0xffd84a,
    candleLow: 0xffa020,
    glow: 0xff9a30,
    ghost: 0xe8ecff,
    ghostEye: 0x1a1030,
    bat: 0x05030a,
    fog: 0x9484b8,
    clock: 0xff8c1a,
    clockShadow: 0x1a0800,
  };

  const MOON = { x: 47, y: 24, r: 7.5 };
  const TOWER_WINDOW = [21, 25];

  const hillY = (x) => Math.min(47, Math.round(33 + ((x - 15) * (x - 15)) / 40));

  const PUMPKIN_BIG = [
    '...ss...',
    '.oOOOOo.',
    'oOyOOyOo',
    'oOOOOOOo',
    'oOyyyyOo',
    '.oOyOyo.',
    '..oooo..',
  ];
  const PUMPKIN_SMALL = ['..s..', '.oOo.', 'oyOyo', 'oyyyo', '.ooo.'];
  const GHOST_A = ['.WWW.', 'WkWkW', 'WWWWW', 'WWWWW', 'WWWWW', 'W.W.W'];
  const GHOST_B = ['.WWW.', 'WkWkW', 'WWWWW', 'WWWWW', 'WWWWW', '.W.W.'];
  const BAT_UP = ['B...B', '.BBB.', '..B..'];
  const BAT_DOWN = ['.....', '.BBB.', 'B.B.B'];

  const WINDOWS = [
    [9, 29],
    [15, 29],
    [9, 33],
    [16, 33],
    [13, 23],
    [21, 25],
    [21, 31],
  ];

  function create(rng) {
    let t = 0;
    const bg = new PC.Canvas();
    const skyMask = new Uint8Array(64 * 64);

    // ---------- static scene (always night, built once) ----------
    function setSolid(x, y, c) {
      bg.set(x, y, c);
      if (x >= 0 && x < 64 && y >= 0 && y < 64) skyMask[y * 64 + x] = 0;
    }

    function line(x0, y0, x1, y1, c) {
      const n = Math.max(Math.abs(x1 - x0), Math.abs(y1 - y0));
      for (let i = 0; i <= n; i++) setSolid(Math.round(x0 + ((x1 - x0) * i) / n), Math.round(y0 + ((y1 - y0) * i) / n), c);
    }

    function buildBackground() {
      vgradient(bg, 0, 0, 63, 47, COL.skyTop, COL.skyBottom);
      skyMask.fill(1);

      // moon with glow and craters
      for (let y = 10; y <= 40; y++) {
        for (let x = 32; x <= 63; x++) {
          const d = Math.hypot(x - MOON.x, y - MOON.y);
          if (d <= MOON.r) {
            let c = COL.moon;
            const crater = hash(Math.floor((x - 30) / 2), Math.floor((y - 10) / 2), 5) === 0;
            if (crater || (x - MOON.x) + (y - MOON.y) > 6) c = COL.moonShade;
            bg.set(x, y, c);
            skyMask[y * 64 + x] = 2; // moon: stars skip it
          } else if (d < 14) {
            bg.blend(x, y, COL.moonGlow, 0.2 * (1 - (d - MOON.r) / (14 - MOON.r)));
          }
        }
      }

      // hill
      for (let x = 0; x < 64; x++) for (let y = hillY(x); y < 64; y++) setSolid(x, y, COL.hill);

      // haunted house
      const H = COL.house;
      for (let y = 27; y <= 37; y++) for (let x = 7; x <= 20; x++) setSolid(x, y, H);
      for (let y = 20; y <= 26; y++) {
        const half = (y - 19) * 1.2;
        for (let x = Math.round(13.5 - half); x <= Math.round(13.5 + half); x++) setSolid(x, y, H);
      }
      for (let y = 22; y <= 37; y++) for (let x = 19; x <= 24; x++) setSolid(x, y, H);
      for (let y = 14; y <= 21; y++) {
        const half = (y - 14) * 0.45;
        for (let x = Math.round(21.5 - half); x <= Math.round(21.5 + half); x++) setSolid(x, y, H);
      }
      for (let y = 18; y <= 23; y++) for (let x = 9; x <= 10; x++) setSolid(x, y, H);
      setSolid(5, 26, H); // sagging eave
      setSolid(26, 22, H);

      // dead tree reaching across the moon
      const T = COL.house;
      line(58, 50, 58, 30, T);
      line(59, 50, 59, 36, T);
      line(58, 36, 52, 31, T);
      line(52, 31, 49, 30, T);
      line(54, 33, 53, 28, T);
      line(58, 33, 62, 27, T);
      line(60, 30, 63, 31, T);
      line(58, 31, 56, 25, T);
      line(56, 25, 54, 23, T);

      // foreground ground with tufts
      for (let x = 0; x < 64; x++) {
        const gy = 48 + Math.round(Math.sin(x * 0.2) * 0.8);
        for (let y = gy; y < 64; y++) setSolid(x, y, hash(x, y, 9) === 0 ? COL.tuft : COL.ground);
        if (hash(x, 3, 4) === 0) setSolid(x, gy - 1, COL.tuft);
      }

      // tombstones (moonlit from the right)
      const stone = (x0, y0, w, h, cross) => {
        for (let y = y0; y < y0 + h; y++) {
          for (let x = x0; x < x0 + w; x++) {
            const top = y === y0 && (x === x0 || x === x0 + w - 1);
            if (top && !cross) continue;
            let c = COL.stone;
            if (x === x0) c = COL.stoneShade;
            if (x === x0 + w - 1) c = COL.stoneLight;
            if (y === y0 + h - 1) c = COL.stoneShade;
            setSolid(x, y, c);
          }
        }
      };
      stone(31, 47, 5, 7, false);
      setSolid(33, 49, COL.stoneShade); // engraving
      setSolid(33, 51, COL.stoneShade);
      stone(51, 50, 4, 6, false);
      // cross
      for (let y = 47; y <= 54; y++) setSolid(42, y, y === 54 ? COL.stoneShade : COL.stoneLight);
      for (let y = 47; y <= 54; y++) setSolid(41, y, COL.stone);
      for (let x = 39; x <= 44; x++) setSolid(x, 49, x === 44 ? COL.stoneLight : COL.stone);
    }
    buildBackground();

    const stars = [];
    for (let i = 0; i < 40 && stars.length < 18; i++) {
      const x = hash(i, 41, 64);
      const y = 14 + hash(i, 43, 18);
      if (skyMask[y * 64 + x] === 1) stars.push([x, y, i]);
    }

    // ---------- moving things ----------
    const winLevel = WINDOWS.map(() => 1);
    let towerShadow = 0;

    const bats = [];
    for (let i = 0; i < 5; i++) {
      bats.push({
        x: rng.float() * 64,
        y0: 17 + rng.float() * 14,
        y: 0,
        vx: (rng.chance(0.5) ? 1 : -1) * (7 + rng.float() * 6),
        vy: 0,
        amp: 1.5 + rng.float() * 2.5,
        ph: rng.float() * 6.283,
        flap: rng.float(),
        free: false,
      });
    }

    const ghost = { x: -6, ph: 0 };
    let candle = 1;

    let sinceStrike = 99;
    let bolt = [];

    // Lightning brightness over time: a flash, a dip, a second flash, then fade.
    function flashLevel(s) {
      if (s < 0.12) return 1;
      if (s < 0.25) return 0.2;
      if (s < 0.45) return 0.85;
      if (s < 1.2) return 0.85 * (1 - (s - 0.45) / 0.75);
      return 0;
    }

    function strike() {
      sinceStrike = 0;
      bolt = [];
      let x = 30 + rng.int(14);
      for (let y = 15; y <= 40; y++) {
        bolt.push([x, y]);
        if (rng.chance(0.45)) x += rng.chance(0.5) ? 1 : -1;
        if (rng.chance(0.12) && y < 34) {
          // a small fork
          let fx = x;
          for (let k = 1; k < 5; k++) bolt.push([(fx += rng.chance(0.5) ? 1 : 2), y + k]);
        }
      }
      for (let i = 0; i < 10; i++) {
        const a = -Math.PI * (0.1 + rng.float() * 0.8);
        const s = 22 + rng.float() * 18;
        bats.push({
          x: TOWER_WINDOW[0],
          y: TOWER_WINDOW[1],
          vx: Math.cos(a) * s,
          vy: Math.sin(a) * s,
          ph: rng.float() * 6.283,
          flap: rng.float(),
          free: true,
        });
      }
    }

    function update(dt, ctx) {
      t += dt;

      for (let i = 0; i < WINDOWS.length; i++) {
        let target = 0.75 + rng.float() * 0.25;
        if (rng.chance(0.004)) winLevel[i] = 0.15; // a light gutters out for a moment
        winLevel[i] += (target - winLevel[i]) * Math.min(1, dt * 2.5);
      }
      if (towerShadow > 0) towerShadow -= dt;
      else if (rng.chance(dt / 9)) towerShadow = 1.2;

      candle += (0.7 + rng.float() * 0.3 - candle) * 0.5;

      for (let i = bats.length - 1; i >= 0; i--) {
        const b = bats[i];
        b.flap += dt * (b.free ? 10 : 7);
        if (b.free) {
          b.x += b.vx * dt;
          b.y += (b.vy + Math.sin(t * 9 + b.ph) * 10) * dt;
          if (b.x < -8 || b.x > 70 || b.y < -6 || b.y > 70) bats.splice(i, 1);
        } else {
          b.x += b.vx * dt;
          b.y = b.y0 + Math.sin(t * 1.7 + b.ph) * b.amp;
          if (b.x > 70) b.x = -7;
          if (b.x < -7) b.x = 70;
        }
      }

      ghost.x += 5 * dt;
      if (ghost.x > 72) ghost.x = -8;

      if (ctx.shake) strike();
      sinceStrike += dt;
    }

    function drawFog(cv) {
      for (let y = 42; y <= 60; y++) {
        const band = 1 - Math.abs(y - 51) / 9;
        if (band <= 0) continue;
        for (let x = 0; x < 64; x++) {
          const n = Math.sin(x * 0.23 + t * 0.6) + Math.sin(x * 0.11 - t * 0.35 + y * 0.6) * 0.8;
          const a = clamp((n + 0.6) / 2.4, 0, 1) * band * 0.32;
          if (a > 0.02) cv.blend(x, y, COL.fog, a);
        }
      }
    }

    function draw(cv, ctx) {
      cv.px.set(bg.px);

      for (const [x, y, i] of stars) {
        const tw = 0.4 + 0.4 * Math.sin(t * (1.3 + (i % 4) * 0.5) + i * 2.3);
        cv.blend(x, y, 0xd8d0ff, tw);
      }

      // windows
      WINDOWS.forEach(([x, y], i) => {
        const c = mix(COL.windowDim, COL.window, winLevel[i]);
        const h = i === 4 ? 2 : 2;
        cv.rect(x, y, x + 1, y + h - 1, c);
      });
      if (towerShadow > 0) {
        // someone walks past the tower window
        const sx = TOWER_WINDOW[0] + (towerShadow > 0.6 ? 0 : 1);
        cv.set(sx, TOWER_WINDOW[1], COL.house);
        cv.set(sx, TOWER_WINDOW[1] + 1, COL.house);
      }

      // ghost drifting over the hill
      const gy = Math.round(36 + Math.sin(t * 2) * 2);
      cv.sprite(Math.floor(t * 4) % 2 ? GHOST_A : GHOST_B, Math.round(ghost.x), gy, { W: COL.ghost, k: COL.ghostEye });

      drawFog(cv);

      // jack-o'-lanterns and their glow
      const lit = mix(COL.candleLow, COL.candle, candle);
      for (const [cx, cy, r] of [[8, 55, 9], [26, 57, 6]]) {
        for (let y = cy - r; y <= cy + r; y++) {
          for (let x = cx - r; x <= cx + r; x++) {
            const d = Math.hypot(x - cx, (y - cy) * 1.3);
            if (d < r) cv.blend(x, y, COL.glow, 0.28 * candle * (1 - d / r));
          }
        }
      }
      const pal = { O: COL.pumpkin, o: COL.pumpkinShade, s: COL.stem, y: lit };
      cv.sprite(PUMPKIN_BIG, 4, 52, pal);
      cv.sprite(PUMPKIN_SMALL, 24, 55, pal);

      for (const b of bats) {
        cv.sprite(Math.floor(b.flap * 2) % 2 ? BAT_UP : BAT_DOWN, Math.round(b.x) - 2, Math.round(b.y) - 1, { B: COL.bat });
      }

      const f = flashLevel(sinceStrike);
      if (f > 0) {
        for (let i = 14 * 64; i < cv.px.length; i++) cv.px[i] = mix(cv.px[i], 0xd8d8ff, f * 0.5);
        if (sinceStrike < 0.5) for (const [x, y] of bolt) cv.set(x, y, 0xffffff);
      }

      PC.drawClock(cv, ctx, COL.clock, COL.clockShadow);
    }

    return { update, draw };
  }

  PC.registerTheme({ id: 'halloween', name: 'Haunted hill', kind: 'holiday', create });
})(globalThis.PixelClock);
