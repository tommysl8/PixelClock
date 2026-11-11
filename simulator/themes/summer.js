/*
 * Summer beach (summer season): rolling waves with foam, the sun crossing
 * the sky with a shimmering reflection (orange at sunset, a silver moon path
 * at night), a swaying palm, a striped umbrella and towel, a sandcastle,
 * a passing seagull, and a crab that scuttles by now and then.
 * Shake: a beach ball comes bouncing across the sand.
 */
(function (PC) {
  'use strict';

  const { mix, clamp, hash, skyAt, daylight } = PC;

  const HORIZON = 33; // last sky row
  const SHORE = 47; // average shoreline row

  const SUMMER_SKY = [
    [0, 0x061028, 0x12204a],
    [5, 0x061028, 0x12204a],
    [6.5, 0x3b4a8c, 0xffb27a],
    [8.5, 0x3d9be9, 0xa8dcf7],
    [16.5, 0x3d9be9, 0xa8dcf7],
    [18.5, 0x5a3d7a, 0xff8a4a],
    [19.8, 0x141638, 0x3a2a55],
    [21, 0x061028, 0x12204a],
    [24, 0x061028, 0x12204a],
  ];

  const BASE = {
    seaFar: 0x1f6fb5,
    seaNear: 0x3fb0d8,
    sand: 0xe9cf95,
    sandDark: 0xd6b878,
    sandLight: 0xf6e2b0,
    sandWet: 0xc4a46a,
    foam: 0xffffff,
    trunk: 0x8a5a2c,
    trunkDark: 0x664020,
    frond: 0x2e8b3a,
    frondDark: 0x1f6b2a,
    frondLight: 0x4aaa48,
    coconut: 0x5a3a1a,
    umbrellaA: 0xe8414a,
    umbrellaB: 0xf6f2ea,
    pole: 0x5e4026,
    towelA: 0x2f6fd8,
    towelB: 0xf6f2ea,
    towelC: 0xffd23f,
    shadow: 0xb89a62,
    castle: 0xc49c5e,
    castleDark: 0x9c7a44,
    flag: 0xe8414a,
    crab: 0xe0482e,
    crabDark: 0xa8301c,
    gull: 0xf2f2f2,
    gullGray: 0x9aa0a8,
  };
  const NIGHT_BASE = {
    seaFar: 0x0a1a3c,
    seaNear: 0x123060,
    sand: 0x3e4468,
    sandDark: 0x33395a,
    sandLight: 0x4a5076,
    sandWet: 0x2c3252,
    foam: 0x9aa8d0,
    shadow: 0x2c3252,
  };
  const NIGHT = 0x0b1430;
  const NIGHT_PAL = {};
  for (const key in BASE) NIGHT_PAL[key] = NIGHT_BASE[key] ?? mix(BASE[key], NIGHT, 0.62);

  const SUN_HIGH = 0xfff3b0;
  const SUN_LOW = 0xffc850;
  const MOON = 0xeeeedd;
  const BALL = [0xe8414a, 0xffffff, 0x2f6fd8, 0xffd23f];
  const CLOCK = 0xffffff;
  const CLOCK_SHADOW = 0x152040;

  // Palm fronds: [angle in degrees above horizontal, direction, length].
  const FRONDS = [
    [72, 1, 7],
    [32, -1, 11],
    [30, 1, 11],
    [4, -1, 12],
    [6, 1, 12],
    [-22, -1, 9],
    [-24, 1, 9],
  ];
  const PALM_TOP = [51, 29];

  // Summer days are long: full light from 7:30 to 18:30, fading to dark by 20:30.
  function summerLight(hf) {
    if (hf < 5.5 || hf > 20.5) return 0;
    if (hf < 7.5) return (hf - 5.5) / 2;
    if (hf > 18.5) return clamp(1 - (hf - 18.5) / 2, 0, 1);
    return 1;
  }

  function create(rng) {
    let t = 0;
    let gull = null;
    let gullTimer = 3 + rng.float() * 6;
    let crab = null;
    let crabTimer = 6 + rng.float() * 10;
    const balls = [];

    function update(dt, ctx) {
      t += dt;
      const day = summerLight(ctx.hourFloat);

      gullTimer -= dt;
      if (!gull && gullTimer <= 0) {
        if (day > 0.3) {
          const dir = rng.chance(0.5) ? 1 : -1;
          gull = { x: dir > 0 ? -4 : 68, y: 17 + rng.float() * 9, v: (10 + rng.float() * 5) * dir };
        }
        gullTimer = 10 + rng.float() * 12;
      }
      if (gull) {
        gull.x += gull.v * dt;
        if (gull.x < -6 || gull.x > 70) gull = null;
      }

      crabTimer -= dt;
      if (!crab && crabTimer <= 0) {
        const dir = rng.chance(0.5) ? 1 : -1;
        crab = { x: dir > 0 ? -4 : 67, dir, pause: 0 };
        crabTimer = 14 + rng.float() * 16;
      }
      if (crab) {
        if (crab.pause > 0) crab.pause -= dt;
        else {
          crab.x += crab.dir * 7 * dt;
          if (rng.chance(0.4 * dt)) crab.pause = 0.6 + rng.float() * 0.8;
        }
        if (crab.x < -6 || crab.x > 69) crab = null;
      }

      if (ctx.shake) balls.push({ x: -3, y: 30, vx: 15 + rng.float() * 5, vy: -6, spin: 0 });
      for (let i = balls.length - 1; i >= 0; i--) {
        const b = balls[i];
        b.vy += 70 * dt;
        b.x += b.vx * dt;
        b.y += b.vy * dt;
        b.spin += b.vx * dt * 0.6;
        if (b.y > 57) {
          b.y = 57;
          b.vy = Math.abs(b.vy) > 8 ? -Math.abs(b.vy) * 0.72 : 0;
          b.vx *= 0.97;
        }
        if (b.x > 70) balls.splice(i, 1);
      }
    }

    // ---------- drawing ----------
    // Where the sun or moon is: {x, y, color, kind} or null when below the horizon.
    function lightSource(hf) {
      if (hf >= 6 && hf <= 20) {
        const p = (hf - 6) / 14;
        const h = Math.sin(Math.PI * p);
        // rises on the right, sets on the left, clear of the palm at sunset
        return { x: 60 - p * 56, y: HORIZON + 1 - h * 15, color: mix(SUN_LOW, SUN_HIGH, clamp(h * 1.6, 0, 1)), r: 2.8, kind: 'sun', h };
      }
      const q = ((hf - 20 + 24) % 24) / 10;
      const h = Math.sin(Math.PI * q);
      return { x: 60 - q * 56, y: HORIZON + 1 - h * 15, color: MOON, r: 2.5, kind: 'moon', h };
    }

    function drawSky(cv, hf, day, L) {
      const [top, bottom] = skyAt(hf, SUMMER_SKY);
      PC.vgradient(cv, 0, 0, 63, HORIZON, top, bottom);
      PC.drawStars(cv, t, 1 - daylight(hf), 0, 0, 63, HORIZON - 4, 30);
      // halo then disc, clipped at the horizon
      const glow = L.kind === 'sun' ? 0.25 : 0.12;
      for (let y = Math.floor(L.y - 6); y <= Math.min(HORIZON, Math.ceil(L.y + 6)); y++) {
        for (let x = Math.floor(L.x - 6); x <= Math.ceil(L.x + 6); x++) {
          const d = Math.hypot(x - L.x, y - L.y);
          if (d <= L.r * 0.55 && L.kind === 'sun') cv.set(x, y, mix(L.color, 0xffffff, 0.6));
          else if (d <= L.r) cv.set(x, y, L.color);
          else if (d < 6) cv.blend(x, y, L.color, glow * (1 - d / 6));
        }
      }
      return bottom;
    }

    function shoreAt(x) {
      return SHORE + Math.round(1.5 * Math.sin(t * 0.7) + 0.8 * Math.sin(x * 0.15 + t * 0.7));
    }

    function drawSea(cv, P, day, L, skyBottom) {
      const far = mix(P.seaFar, skyBottom, 0.25);
      const near = P.seaNear;
      const sparkle = 0.2 + 0.25 * day;
      for (let y = HORIZON + 1; y <= SHORE + 3; y++) {
        const c = mix(far, near, (y - HORIZON - 1) / (SHORE - HORIZON));
        for (let x = 0; x < 64; x++) {
          let px = c;
          const s = Math.sin(x * 0.55 + t * 1.8 + y * 1.7) * Math.sin(x * 0.13 - t * 0.9 + y);
          if (s > 0.72) px = mix(c, 0xffffff, sparkle);
          else if (s < -0.8) px = PC.dim(c, 0.85);
          cv.set(x, y, px);
        }
      }
      // shimmering path of light on the water
      if (L.y <= HORIZON + 1 && L.h > 0) {
        const strength = L.kind === 'sun' ? 1 : 0.85;
        const frame = Math.floor(t * 5);
        for (let y = HORIZON + 1; y <= SHORE + 1; y++) {
          const w = 1 + (y - HORIZON) * 0.3;
          for (let x = Math.floor(L.x - w); x <= Math.ceil(L.x + w); x++) {
            if (hash(x, y * 7 + frame, 2) !== 0) continue;
            const a = strength * (1 - Math.abs(x - L.x) / (w + 1)) * (1 - (y - HORIZON) / 22);
            cv.blend(x, y, L.color, a);
          }
        }
      }
      // a wave line rolling in toward the shore
      const p = (t / 4) % 1;
      const wy = Math.round(HORIZON + 4 + p * (SHORE - HORIZON - 5));
      for (let x = 0; x < 64; x++) {
        if (hash(x, wy, 3) === 0) continue;
        cv.blend(x, wy + Math.round(Math.sin(x * 0.3 + t) * 0.6), P.foam, 0.35 * Math.sin(p * Math.PI));
      }
    }

    function drawSand(cv, P) {
      for (let x = 0; x < 64; x++) {
        const s = shoreAt(x);
        for (let y = s; y < 64; y++) {
          let c = P.sand;
          if (y <= s + 2) c = P.sandWet;
          else {
            const h = hash(x, y, 9);
            if (h === 0) c = P.sandDark;
            else if (h === 1) c = P.sandLight;
          }
          cv.set(x, y, c);
        }
        // foam along the edge of the wash
        cv.set(x, s, P.foam);
        if (hash(x, Math.floor(t * 2), 3) === 0) cv.blend(x, s - 1, P.foam, 0.7);
      }
    }

    function drawPalm(cv, P) {
      // curved trunk from the sand up to the crown
      const [tx, ty] = PALM_TOP;
      const base = 62;
      const len = base - ty;
      for (let i = 0; i <= len; i++) {
        const y = base - i;
        const x = Math.round(58 - ((i / len) ** 2) * (58 - tx));
        const ring = i % 3 === 0;
        cv.set(x, y, ring ? P.trunkDark : P.trunk);
        cv.set(x + 1, y, P.trunkDark);
      }
      // fronds droop from the crown and sway a little
      FRONDS.forEach(([deg, dir, L], i) => {
        const a = (deg * Math.PI) / 180 + Math.sin(t * 1.1 + i * 0.9) * 0.06;
        for (let s = 1; s <= L; s++) {
          const x = tx + dir * s * Math.cos(a);
          const y = ty - s * Math.sin(a) + 0.045 * s * s;
          cv.set(x, y, s > L - 3 ? P.frondLight : P.frond);
          if (s > 2 && s < L) {
            cv.set(x, y + 1, P.frondDark);
            if (s % 2 === 0) cv.set(x + dir * 0.6, y + 2, P.frondDark);
          }
        }
      });
      cv.rect(tx - 1, ty + 1, tx, ty + 2, P.coconut);
      cv.set(tx + 1, ty + 1, P.coconut);
    }

    function drawUmbrella(cv, P, day) {
      const cx = 21;
      const cy = 47;
      // shadow on the sand
      for (let y = 57; y <= 60; y++) {
        for (let x = 12; x <= 32; x++) {
          const e = ((x - 22) / 10) ** 2 + ((y - 58.5) / 2) ** 2;
          if (e <= 1) cv.blend(x, y, P.shadow, 0.6 * day);
        }
      }
      // towel with a fringe
      for (let y = 58; y <= 62; y++) {
        const band = (y - 58) % 2;
        cv.rect(24, y, 35, y, band === 0 ? P.towelA : P.towelB);
      }
      cv.rect(24, 60, 35, 60, P.towelC);
      for (let x = 24; x <= 35; x += 2) cv.set(x, 63, P.towelB);
      // pole
      cv.rect(cx, cy, cx, 59, P.pole);
      // dome with stripes
      for (let r = 0; r <= 5; r++) {
        const y = cy - 5 + r;
        const hw = Math.round(Math.sqrt(Math.max(0, 36 - (5 - r) * (5 - r))) * 1.5);
        for (let x = cx - hw; x <= cx + hw; x++) {
          const seg = Math.floor((x - cx + 12) / 3) % 2;
          let c = seg ? P.umbrellaA : P.umbrellaB;
          if (r === 5 && (x - cx) % 3 === 0) continue; // scalloped edge
          if (r === 0) c = P.umbrellaA;
          cv.set(x, y, c);
        }
      }
      cv.set(cx, cy - 6, P.pole);
    }

    function drawCastle(cv, P) {
      const x0 = 3;
      const g = 60;
      cv.rect(x0, g - 3, x0 + 8, g, P.castle);
      cv.rect(x0, g - 6, x0 + 2, g - 4, P.castle);
      cv.rect(x0 + 6, g - 6, x0 + 8, g - 4, P.castle);
      cv.rect(x0 + 3, g - 5, x0 + 5, g - 4, P.castleDark);
      cv.set(x0, g - 7, P.castle);
      cv.set(x0 + 2, g - 7, P.castle);
      cv.set(x0 + 6, g - 7, P.castle);
      cv.set(x0 + 8, g - 7, P.castle);
      cv.rect(x0 + 4, g - 2, x0 + 4, g, P.castleDark);
      cv.rect(x0 + 7, g - 10, x0 + 7, g - 7, P.castleDark);
      const wave = Math.floor(t * 4) % 2;
      cv.rect(x0 + 8, g - 10, x0 + 9 + wave, g - 10, P.flag);
      cv.set(x0 + 8, g - 9, P.flag);
    }

    function drawGull(cv, P) {
      if (!gull) return;
      const x = Math.round(gull.x);
      const y = Math.round(gull.y + Math.sin(t * 1.5) * 1.5);
      const up = Math.floor(t * 5) % 2 === 0;
      cv.set(x, y, P.gull);
      cv.set(x - 1, y, P.gull);
      cv.set(x + 1, y, P.gull);
      const wy = y + (up ? -1 : 1);
      cv.set(x - 2, wy, P.gull);
      cv.set(x + 2, wy, P.gull);
      cv.set(x - 3, wy + (up ? -1 : 0), P.gullGray);
      cv.set(x + 3, wy + (up ? -1 : 0), P.gullGray);
    }

    function drawCrab(cv, P) {
      if (!crab) return;
      const x = Math.round(crab.x);
      const y = 61;
      const step = crab.pause > 0 ? 0 : Math.floor(t * 10) % 2;
      cv.rect(x - 1, y - 1, x + 1, y, P.crab);
      cv.set(x - 1, y - 2, P.crabDark); // eyes
      cv.set(x + 1, y - 2, P.crabDark);
      cv.set(x - 2, y - 1 - step, P.crab); // claws
      cv.set(x + 2, y - 1 - (1 - step), P.crab);
      cv.set(x - 2, y + 1, P.crabDark); // legs
      cv.set(x + 2, y + 1, P.crabDark);
      cv.set(x - 1 + step, y + 1, P.crabDark);
    }

    function drawBall(cv, b, P, day) {
      // shadow shrinks as the ball rises
      const sh = clamp(1 - (57 - b.y) / 30, 0.2, 1);
      for (let dx = -3; dx <= 3; dx++) {
        if (Math.abs(dx) <= 3 * sh) cv.blend(b.x + dx, 61, P.shadow, 0.5 * sh * (0.4 + 0.6 * day));
      }
      for (let dy = -3; dy <= 3; dy++) {
        for (let dx = -3; dx <= 3; dx++) {
          if (dx * dx + dy * dy > 9) continue;
          const seg = Math.floor(((Math.atan2(dy, dx) + b.spin) / (Math.PI / 2)) % 4 + 4) % 4;
          const c = dx === 0 && dy === 0 ? 0xffffff : BALL[seg];
          cv.set(b.x + dx, b.y + dy, mix(mix(c, NIGHT, 0.5), c, 0.5 + 0.5 * day));
        }
      }
    }

    function draw(cv, ctx) {
      const hf = ctx.hourFloat;
      const day = summerLight(hf);
      const P = {};
      for (const key in BASE) P[key] = mix(NIGHT_PAL[key], BASE[key], day);
      // a warm cast over everything at sunrise and sunset
      const golden = clamp(1 - Math.abs(hf - 18.2) / 1.6, 0, 1) + clamp(1 - Math.abs(hf - 6.6) / 1.2, 0, 1);
      if (golden > 0) {
        for (const key of ['seaFar', 'seaNear', 'sand', 'sandDark', 'sandLight', 'sandWet']) P[key] = mix(P[key], 0xff8a4a, golden * 0.25);
      }

      const L = lightSource(hf);
      const skyBottom = drawSky(cv, hf, day, L);
      drawGull(cv, P);
      drawSea(cv, P, day, L, skyBottom);
      drawSand(cv, P);
      drawCastle(cv, P);
      drawUmbrella(cv, P, day);
      drawPalm(cv, P);
      drawCrab(cv, P);
      for (const b of balls) drawBall(cv, b, P, day);

      PC.drawClock(cv, ctx, CLOCK, CLOCK_SHADOW);
    }

    return { update, draw };
  }

  PC.registerTheme({ id: 'summer', name: 'Summer beach', kind: 'season', create });
})(globalThis.PixelClock);
