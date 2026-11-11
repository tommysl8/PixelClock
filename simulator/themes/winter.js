/*
 * Snowy hills (winter season): rolling snow hills, snow-capped pines, a
 * snowman in a top hat, a distant cabin with a warm window and chimney smoke,
 * and snow falling in two layers. At night: stars, moon, and a soft aurora.
 * Shake: blizzard.
 */
(function (PC) {
  'use strict';

  const { mix, clamp, hash, skyAt, daylight } = PC;

  // Scene colours in daylight.
  const BASE = {
    snowFar: 0x8ba8d4,
    snowFarShade: 0x7895c4,
    snowMid: 0xc4d6ee,
    snowMidShade: 0xa3bbdc,
    snowMidRim: 0xe2ecf8,
    snowNear: 0xf0f5fd,
    snowNearShade: 0xcbd9ed,
    snowNearRim: 0xffffff,
    pine: 0x22603a,
    pineDark: 0x153f26,
    pineSnow: 0xf0f5fd,
    pineSnowShade: 0xc6d5ea,
    trunk: 0x4a2e1a,
    cabin: 0x7a4e2c,
    cabinDark: 0x55351e,
    roof: 0xf2f6fc,
    roofEdge: 0x6f7f9c,
    chimney: 0x6a3226,
    door: 0x3a2214,
    snowman: 0xffffff,
    snowmanShade: 0xc9d6ea,
    snowmanLine: 0x7d93b6,
    coal: 0x1a1a22,
    carrot: 0xff8a1e,
    scarf: 0xd63a3a,
    scarfDark: 0x9e2626,
    stick: 0x5a3418,
    hat: 0x1c1c24,
    hatBand: 0xc03434,
  };
  // Moonlit versions of the snow; anything not listed is tinted toward NIGHT.
  const NIGHT_BASE = {
    snowFar: 0x26386a,
    snowFarShade: 0x1f2f5c,
    snowMid: 0x374b80,
    snowMidShade: 0x2b3d6e,
    snowMidRim: 0x5468a0,
    snowNear: 0x55699e,
    snowNearShade: 0x435689,
    snowNearRim: 0x7a8cc0,
    pine: 0x255c4e,
    pineDark: 0x173e3c,
    pineSnow: 0x6a7eb4,
    pineSnowShade: 0x4c5f94,
    roof: 0x6a7eb4,
    roofEdge: 0x1c2648,
    snowman: 0x9aacd8,
    snowmanShade: 0x7184b8,
    snowmanLine: 0x2c3a68,
  };
  const NIGHT = 0x0b1430;
  const NIGHT_AMOUNT = 0.6;
  const NIGHT_PAL = {};
  for (const key in BASE) NIGHT_PAL[key] = NIGHT_BASE[key] ?? mix(BASE[key], NIGHT, NIGHT_AMOUNT);

  // Crisp winter sky: deeper blue by day, pink at dawn and dusk.
  const WINTER_SKY = [
    [0, 0x050a1e, 0x142250],
    [5.5, 0x050a1e, 0x142250],
    [7, 0x34407e, 0xeea0a0],
    [9, 0x4c8ad2, 0xa6cbee],
    [15.5, 0x4c8ad2, 0xa6cbee],
    [17, 0x3e3e7a, 0xe58a7a],
    [18.5, 0x0c1238, 0x26305e],
    [24, 0x050a1e, 0x142250],
  ];

  const WINDOW_LIT = 0xffc35a;
  const WINDOW_HOT = 0xffe08a;
  const MOON = 0xf4f1d0;
  const SUN = 0xfff4d0;
  const SMOKE = 0xb8c0cc;
  const AURORA_A = 0x3cffb0;
  const AURORA_B = 0x40b8ff;
  const CLOCK = 0xffffff;
  const CLOCK_SHADOW = 0x121a33;

  // Hill outlines (top row of each hill per column).
  const FAR = [];
  const MID = [];
  const NEAR = [];
  for (let x = 0; x < 64; x++) {
    FAR.push(Math.round(37 + 2.5 * Math.sin(x * 0.08 + 0.5) + 1.5 * Math.sin(x * 0.21 + 2)));
    MID.push(Math.round(45 + 3 * Math.sin(x * 0.06 + 2.2) + 1.2 * Math.sin(x * 0.17)));
    NEAR.push(Math.round(54 + 1.5 * Math.sin(x * 0.05 + 1)));
  }
  const CABIN_X = 42; // centre column of the cabin on the far hill
  const PINES = [
    [27, 9],
    [5, 19],
    [19, 15],
    [12, 12],
    [62, 11],
  ]; // [x, height] on the middle hill, back to front
  const SNOWMAN_X = 51;

  function disc(cv, cx, cy, r, c) {
    const rr = r * r + r * 0.6;
    for (let y = Math.floor(cy - r); y <= Math.ceil(cy + r); y++) {
      for (let x = Math.floor(cx - r); x <= Math.ceil(cx + r); x++) {
        if ((x - cx) * (x - cx) + (y - cy) * (y - cy) <= rr) cv.set(x, y, c);
      }
    }
  }

  function line(cv, x0, y0, x1, y1, c) {
    const n = Math.max(Math.abs(x1 - x0), Math.abs(y1 - y0));
    for (let i = 0; i <= n; i++) cv.set(x0 + ((x1 - x0) * i) / n, y0 + ((y1 - y0) * i) / n, c);
  }

  function create(rng) {
    let t = 0;
    let wind = 0;
    let blizzard = 0;

    // two layers of snow: far (small, slow, dim) and near (bright, faster)
    const far = [];
    const near = [];
    const newFlake = (layer, anywhere) => ({
      x: rng.float() * 64,
      y: anywhere ? rng.float() * 64 : -1 - rng.float() * 4,
      v: layer === 0 ? 3 + rng.float() * 2.5 : 7 + rng.float() * 5,
      ph: rng.float() * 6.283,
    });
    for (let i = 0; i < 28; i++) far.push(newFlake(0, true));
    for (let i = 0; i < 16; i++) near.push(newFlake(1, true));

    const smoke = [];
    let smokeTimer = 0;

    const swirl = []; // blizzard particles after a shake

    function moveFlakes(list, layer, dt) {
      const drift = layer === 0 ? 0.5 : 1;
      for (let i = 0; i < list.length; i++) {
        const f = list[i];
        f.y += f.v * dt * (blizzard > 0 ? 1.6 : 1);
        f.x += (wind * drift + Math.sin(t * 1.3 + f.ph) * (layer === 0 ? 0.6 : 1.4)) * dt;
        if (f.x < -2) f.x += 66;
        if (f.x > 65) f.x -= 66;
        if (f.y > 64) list[i] = newFlake(layer, false);
      }
    }

    function update(dt, ctx) {
      t += dt;
      const calm = 2.5 * Math.sin(t * 0.13) + 1.2 * Math.sin(t * 0.37);
      const target = blizzard > 0 ? 26 : calm;
      wind += (target - wind) * Math.min(1, dt * 1.5);
      if (blizzard > 0) blizzard -= dt;

      moveFlakes(far, 0, dt);
      moveFlakes(near, 1, dt);

      // chimney smoke puffs drift up and with the wind
      smokeTimer -= dt;
      if (smokeTimer <= 0) {
        smokeTimer = 0.45 + rng.float() * 0.3;
        smoke.push({ x: CABIN_X + 2, y: FAR[CABIN_X] - 11, age: 0, life: 3 + rng.float() * 1.5, ph: rng.float() * 6.283 });
      }
      for (let i = smoke.length - 1; i >= 0; i--) {
        const s = smoke[i];
        s.age += dt;
        s.y -= 2.2 * dt;
        s.x += (wind * 0.35 - 0.6 + Math.sin(t * 1.7 + s.ph) * 0.4) * dt;
        if (s.age > s.life) smoke.splice(i, 1);
      }

      if (ctx.shake) {
        blizzard = 4;
        for (let i = 0; i < 150; i++) {
          swirl.push({
            x: rng.float() * 64,
            y: rng.float() * 64,
            vx: 15 + rng.float() * 35,
            vy: (rng.float() - 0.5) * 30,
            age: 0,
            life: 3 + rng.float() * 3,
            ph: rng.float() * 6.283,
          });
        }
      }
      for (let i = swirl.length - 1; i >= 0; i--) {
        const p = swirl[i];
        p.age += dt;
        p.vx *= 0.985;
        p.vy = p.vy * 0.97 + Math.sin(t * 3 + p.ph) * 8 * dt + 4 * dt;
        p.x += p.vx * dt;
        p.y += p.vy * dt;
        if (p.x > 64) p.x -= 64;
        if (p.age > p.life) swirl.splice(i, 1);
      }
    }

    // ---------- drawing ----------
    function drawSky(cv, hf, day) {
      const [top, bottom] = skyAt(hf, WINTER_SKY);
      PC.vgradient(cv, 0, 0, 63, 44, top, bottom);
      const night = 1 - day;
      PC.drawStars(cv, t, night, 0, 0, 63, 34, 30);

      if (night > 0.2) {
        // aurora: two soft wavy ribbons, kept below the clock
        const a = (night - 0.2) / 0.8;
        for (let x = 0; x < 64; x++) {
          const cy = 21 + 3.5 * Math.sin(x * 0.11 + t * 0.45) + 1.8 * Math.sin(x * 0.047 - t * 0.27);
          const pulse = 0.55 + 0.45 * Math.sin(x * 0.27 + t * 1.1);
          for (let dy = -3; dy <= 3; dy++) {
            const y = Math.round(cy + dy);
            if (y < 15) continue;
            const k = 1 - Math.abs(dy) / 4;
            cv.blend(x, y, mix(AURORA_A, AURORA_B, (dy + 3) / 6), a * 0.32 * k * pulse);
          }
        }
        // moon, right of the clock
        const m = Math.min(1, a * 1.5);
        disc(cv, 56, 9, 2.4, mix(top, MOON, m));
        cv.set(55, 8, mix(top, 0xffffff, m));
      }
      if (day > 0) {
        disc(cv, 9, 21, 2.2, mix(bottom, SUN, day));
      }
    }

    function drawPine(cv, P, cx, base, h) {
      const trunkH = 2;
      cv.rect(cx, base - trunkH, cx + (h > 13 ? 1 : 0), base, P.trunk);
      const body = h - trunkH;
      const top = base - trunkH - body;
      const tier = Math.ceil(body / 3);
      for (let r = 0; r < body; r++) {
        const y = top + r;
        const i = Math.floor(r / tier);
        const j = r % tier;
        const hw = Math.floor((j + 1 + i * 1.4) * 0.72);
        for (let x = cx - hw; x <= cx + hw; x++) {
          let c = x > cx ? P.pineDark : P.pine;
          // snow on each tier's top and drooping tips
          if (j === 0 && Math.abs(x - cx) < hw) c = x > cx ? P.pineSnowShade : P.pineSnow;
          else if (j === tier - 1 && Math.abs(x - cx) >= hw - 1 && x < cx) c = P.pineSnow;
          cv.set(x, y, c);
        }
      }
      cv.set(cx, top - 1, P.pineSnow);
    }

    function drawCabin(cv, P, day) {
      const g = FAR[CABIN_X];
      const cx = CABIN_X;
      cv.rect(cx - 3, g - 5, cx + 3, g, P.cabin);
      cv.rect(cx + 2, g - 5, cx + 3, g, P.cabinDark);
      cv.rect(cx + 2, g - 11, cx + 3, g - 8, P.chimney);
      cv.rect(cx + 2, g - 12, cx + 3, g - 12, P.roof);
      // snowy roof with a dark outline so it reads against the sky
      for (let r = 0; r < 4; r++) {
        const y = g - 9 + r;
        cv.rect(cx - 1 - r, y, cx + 1 + r, y, P.roof);
        cv.set(cx - 2 - r, y, P.roofEdge);
        cv.set(cx + 2 + r, y, P.roofEdge);
      }
      cv.rect(cx - 1, g - 10, cx + 1, g - 10, P.roofEdge);
      cv.rect(cx - 5, g - 5, cx + 5, g - 5, P.roofEdge);
      // door and glowing window
      cv.rect(cx + 1, g - 3, cx + 1, g, P.door);
      const flicker = 0.5 + 0.5 * Math.sin(t * 7) * Math.sin(t * 3.1);
      const win = mix(WINDOW_LIT, WINDOW_HOT, flicker);
      const night = 1 - day;
      if (night > 0.05) {
        for (let y = g - 6; y <= g + 1; y++) {
          for (let x = cx - 5; x <= cx + 1; x++) {
            const d = Math.hypot(x - (cx - 1.5), (y - (g - 2.5)) * 1.2);
            if (d < 4) cv.blend(x, y, WINDOW_LIT, night * 0.22 * (1 - d / 4));
          }
        }
      }
      cv.rect(cx - 2, g - 3, cx - 1, g - 2, mix(mix(0x3a3020, WINDOW_LIT, 0.6), win, 0.35 + 0.65 * night));
    }

    function drawSnowman(cv, P) {
      const cx = SNOWMAN_X;
      const ground = NEAR[cx];
      const balls = [
        [ground - 4, 4.6],
        [ground - 11, 3.6],
        [ground - 17, 2.8],
      ];
      // arms behind the body
      line(cv, cx - 3, ground - 11, cx - 8, ground - 15, P.stick);
      cv.set(cx - 9, ground - 16, P.stick);
      cv.set(cx - 8, ground - 17, P.stick);
      line(cv, cx + 3, ground - 11, cx + 8, ground - 14, P.stick);
      cv.set(cx + 9, ground - 15, P.stick);
      cv.set(cx + 9, ground - 13, P.stick);
      // outline first, one pixel bigger than each ball
      for (const [cy, r] of balls) disc(cv, cx, cy, r + 1, P.snowmanLine);
      for (const [cy, r] of balls) {
        const rr = r * r + r * 0.6;
        for (let y = Math.floor(cy - r); y <= Math.ceil(cy + r); y++) {
          for (let x = Math.floor(cx - r); x <= Math.ceil(cx + r); x++) {
            const dx = x - cx;
            const dy = y - cy;
            if (dx * dx + dy * dy > rr) continue;
            const shaded = dx + dy * 0.6 > r * 0.45;
            cv.set(x, y, shaded ? P.snowmanShade : P.snowman);
          }
        }
      }
      const hy = ground - 17; // head centre
      // face
      cv.set(cx - 1, hy - 1, P.coal);
      cv.set(cx + 1, hy - 1, P.coal);
      cv.rect(cx + 1, hy, cx + 3, hy, P.carrot);
      cv.set(cx - 1, hy + 2, P.coal);
      cv.set(cx, hy + 2, P.coal);
      cv.set(cx + 1, hy + 2, P.coal);
      // top hat
      cv.rect(cx - 3, hy - 3, cx + 3, hy - 3, P.hat);
      cv.rect(cx - 2, hy - 7, cx + 2, hy - 4, P.hat);
      cv.rect(cx - 2, hy - 4, cx + 2, hy - 4, P.hatBand);
      // scarf
      cv.rect(cx - 3, hy + 3, cx + 3, hy + 4, P.scarf);
      cv.rect(cx - 3, hy + 4, cx + 3, hy + 4, P.scarfDark);
      cv.rect(cx - 3, hy + 5, cx - 2, hy + 8, P.scarf);
      cv.set(cx - 2, hy + 8, P.scarfDark);
      // buttons
      cv.set(cx, ground - 12, P.coal);
      cv.set(cx, ground - 9, P.coal);
    }

    function drawHills(cv, P) {
      for (let x = 0; x < 64; x++) {
        for (let y = FAR[x]; y < MID[x]; y++) {
          const shade = y - FAR[x] > 3 || hash(x, y, 7) === 0;
          cv.set(x, y, shade ? P.snowFarShade : P.snowFar);
        }
      }
    }

    function drawMid(cv, P) {
      for (let x = 0; x < 64; x++) {
        const slope = MID[Math.min(63, x + 1)] - MID[Math.max(0, x - 1)];
        for (let y = MID[x]; y < NEAR[x]; y++) {
          let c = P.snowMid;
          if (y === MID[x]) c = P.snowMidRim;
          else if ((slope > 0 && y - MID[x] < 4) || y >= NEAR[x] - 2 || hash(x, y, 9) === 0) c = P.snowMidShade;
          cv.set(x, y, c);
        }
      }
    }

    function drawNear(cv, P, day) {
      for (let x = 0; x < 64; x++) {
        for (let y = NEAR[x]; y < 64; y++) {
          let c = P.snowNear;
          if (y === NEAR[x]) c = P.snowNearRim;
          else if (y === NEAR[x] + 1 || hash(x, y, 11) === 0) c = P.snowNearShade;
          cv.set(x, y, c);
          if (hash(x, y, 61) === 0) {
            const s = Math.sin(t * 2.5 + x * 1.7 + y);
            if (s > 0.6) cv.blend(x, y, 0xffffff, (s - 0.6) * 2.2 * (0.4 + 0.6 * day));
          }
        }
      }
    }

    function draw(cv, ctx) {
      const hf = ctx.hourFloat;
      const day = daylight(hf);
      const k = NIGHT_AMOUNT * (1 - day);
      const P = {};
      for (const key in BASE) P[key] = mix(NIGHT_PAL[key], BASE[key], day);
      const flakeNear = mix(0xb4c2e0, 0xffffff, day);
      const flakeFar = mix(0x8696bc, 0xe8f0ff, day);

      drawSky(cv, hf, day);
      drawHills(cv, P);
      drawCabin(cv, P, day);
      for (const s of smoke) {
        const a = 0.55 * (1 - s.age / s.life);
        cv.blend(s.x, s.y, mix(SMOKE, NIGHT, k * 0.6), a);
        if (s.age > 1) cv.blend(s.x + 1, s.y, mix(SMOKE, NIGHT, k * 0.6), a * 0.5);
      }
      for (const f of far) if (f.y < MID[clamp(Math.round(f.x), 0, 63)]) cv.blend(f.x, f.y, flakeFar, 0.55);
      drawMid(cv, P);
      for (const [x, h] of PINES) drawPine(cv, P, x, MID[x] + 2, h);
      drawNear(cv, P, day);
      drawSnowman(cv, P);
      for (const f of near) cv.set(f.x, f.y, flakeNear);
      for (const p of swirl) cv.blend(p.x, p.y, flakeNear, 0.9 * Math.min(1, (p.life - p.age) / 1.2));

      PC.drawClock(cv, ctx, CLOCK, CLOCK_SHADOW);
    }

    return { update, draw };
  }

  PC.registerTheme({ id: 'winter', name: 'Snowy hills', kind: 'season', create });
})(globalThis.PixelClock);
