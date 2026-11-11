/*
 * Valentine's Day: a big beating heart, little hearts drifting upward, soft
 * bokeh and sparkles, and two lovebirds on a blossom branch who lean in for
 * a kiss now and then. Shake: a burst of hearts.
 */
(function (PC) {
  'use strict';

  const { mix, dim, clamp, daylight } = PC;

  const COL = {
    skyTopDay: 0x4e1838,
    skyBotDay: 0x8e3a64,
    skyTopNight: 0x1a0818,
    skyBotNight: 0x48163a,
    bokeh: 0xff9cc0,
    heart: 0xe0284a,
    heartLight: 0xff6b8a,
    heartShine: 0xffd0dc,
    heartDark: 0x9c1434,
    heartEdge: 0x4a0820,
    glow: 0xff5c8a,
    branch: 0x3e1e22,
    branchTop: 0x6e3c3c,
    branchDark: 0x2a1216,
    petal: 0xffc2d6,
    petalCenter: 0xffe066,
    sparkle: 0xfff0f6,
    clock: 0xffe6ef,
    clockShadow: 0x2a0716,
  };

  const HEART_COLORS = [0xff4d6d, 0xff8fb1, 0xe0284a, 0xffc2d6, 0xc2185b];

  // Lovebirds, facing right. Mirrored for the other bird.
  const BIRD = [
    '....OOO..',
    '...OOOOEK',
    '..OOOOOO.',
    'DDDDOOOL.',
    '.DDDDOLL.',
    '..DDDLL..',
    '....F.F..',
  ];
  const mirror = (rows) => rows.map((r) => r.split('').reverse().join(''));
  const BIRD_L = mirror(BIRD);
  const BIRD_A = { O: 0xff6f9a, D: 0xc23c6c, L: 0xffb8cc, E: 0x2a0a18, K: 0xffb347, F: 0x2a1216 };
  const BIRD_B = { O: 0xf4e4ee, D: 0xc99ab6, L: 0xffffff, E: 0x2a0a18, K: 0xffb347, F: 0x2a1216 };

  const SMALL_HEART = ['X.X', 'XXX', '.X.'];
  const MID_HEART = ['XX.XX', 'XXXXX', '.XXX.', '..X..'];

  // Branch rises gently from left to right; returns the top row at column x.
  const branchY = (x) => 58 - Math.floor((x * 5) / 63);

  // Big heart bounding box.
  const HX0 = 12;
  const HX1 = 51;
  const HY0 = 12;
  const HY1 = 47;
  const HW = HX1 - HX0 + 1;
  const HH = HY1 - HY0 + 1;

  function create(rng) {
    let t = 0;
    let beat = 0; // 0..1, pulse amount this frame
    const mask = new Uint8Array(HW * HH); // 1 inside, 2 outline, 3 glow
    const floaters = [];
    const sparkles = [];
    let kissTimer = 5 + rng.float() * 4;
    let kiss = 0; // seconds left in the current kiss

    function spawnFloater(x, y, vx, vy) {
      floaters.push({
        x: x ?? 2 + rng.float() * 60,
        y: y ?? 60,
        vx: vx ?? 0,
        vy: vy ?? -(4 + rng.float() * 4),
        ph: rng.float() * 6.283,
        big: rng.chance(0.3),
        color: HEART_COLORS[rng.int(HEART_COLORS.length)],
        age: 0,
        life: 7 + rng.float() * 4,
      });
    }
    for (let i = 0; i < 6; i++) {
      spawnFloater(undefined, 20 + rng.float() * 40);
    }

    function update(dt, ctx) {
      t += dt;

      // Heartbeat: two quick bumps ("lub-dub") every 1.1 seconds.
      const p = (t % 1.1) / 1.1;
      const bump = (c, w) => Math.exp(-(((p - c) / w) ** 2));
      beat = bump(0.05, 0.05) + 0.65 * bump(0.22, 0.05);

      if (rng.chance(1.3 * dt) && floaters.length < 20) spawnFloater();
      for (let i = floaters.length - 1; i >= 0; i--) {
        const f = floaters[i];
        f.age += dt;
        f.vx *= 0.97;
        f.x += (f.vx + Math.sin(t * 1.6 + f.ph) * 2.2) * dt;
        f.y += f.vy * dt;
        if (f.vy < -3) f.vy += 6 * dt; // burst hearts slow down to a drift
        if (f.y < 12 || f.age > f.life) floaters.splice(i, 1);
      }

      if (rng.chance(3 * dt)) {
        sparkles.push({ x: 2 + rng.int(60), y: 15 + rng.int(36), age: 0, life: 0.5 + rng.float() * 0.4 });
      }
      for (let i = sparkles.length - 1; i >= 0; i--) {
        sparkles[i].age += dt;
        if (sparkles[i].age > sparkles[i].life) sparkles.splice(i, 1);
      }

      kissTimer -= dt;
      if (kiss > 0) kiss -= dt;
      if (kissTimer <= 0) {
        kiss = 1.4;
        kissTimer = 7 + rng.float() * 5;
        spawnFloater(31.5, 46, 0, -5);
      }

      if (ctx.shake) {
        for (let i = 0; i < 26; i++) {
          const a = rng.float() * Math.PI * 2;
          const s = 14 + rng.float() * 16;
          spawnFloater(31.5, 30, Math.cos(a) * s, Math.sin(a) * s - 6);
          const f = floaters[floaters.length - 1];
          f.big = rng.chance(0.6);
          f.color = [0xffffff, 0xffd0dc, 0xff8fb1][rng.int(3)];
          f.life = 4 + rng.float() * 2;
        }
      }
    }

    // ---------- drawing ----------
    function drawBackground(cv, day) {
      const top = mix(COL.skyTopNight, COL.skyTopDay, day);
      const bot = mix(COL.skyBotNight, COL.skyBotDay, day);
      PC.vgradient(cv, 0, 0, 63, 63, top, bot);
      // soft bokeh circles that slowly breathe
      for (let i = 0; i < 12; i++) {
        const cx = PC.hash(i, 7, 64);
        const cy = 16 + PC.hash(i, 9, 44);
        const r = 1 + PC.hash(i, 3, 2);
        const a = (0.1 + 0.06 * Math.sin(t * 0.7 + i)) * (1.2 - 0.4 * day);
        for (let y = -r; y <= r; y++) {
          for (let x = -r; x <= r; x++) {
            if (x * x + y * y <= r * r + 0.5) cv.blend(cx + x, cy + y, COL.bokeh, a);
          }
        }
      }
      PC.drawStars(cv, t, (1 - day) * 0.6, 0, 14, 63, 50, 14, 0xffd6e6);
    }

    function drawBranch(cv) {
      for (let x = 0; x < 64; x++) {
        const y = branchY(x);
        cv.set(x, y, COL.branchTop);
        cv.set(x, y + 1, COL.branch);
        cv.set(x, y + 2, x < 40 ? COL.branch : COL.branchDark);
        if (x < 22) cv.set(x, y + 3, COL.branchDark);
      }
      // a little twig and blossoms
      for (let i = 0; i < 5; i++) cv.set(49 + i, branchY(49) - 1 - i, COL.branch);
      for (let i = 0; i < 4; i++) cv.set(8 - i, branchY(8) - 1 - i, COL.branch);
      for (const x of [5, 15, 44, 54, 60]) {
        const y = branchY(x) - (x === 54 ? 6 : x === 5 ? 5 : 1);
        cv.set(x, y - 1, COL.petal);
        cv.set(x - 1, y, COL.petal);
        cv.set(x + 1, y, COL.petal);
        cv.set(x, y + 1, COL.petal);
        cv.set(x, y, COL.petalCenter);
      }
    }

    function drawHeart(cv, day) {
      const cx = 31.5;
      const cy = 30;
      const s = 10 + beat * 1.5;
      mask.fill(0);
      for (let y = 0; y < HH; y++) {
        for (let x = 0; x < HW; x++) {
          const nx = (HX0 + x - cx) / s;
          const ny = -(HY0 + y - cy) / s;
          const q = nx * nx + ny * ny - 1;
          if (q * q * q - nx * nx * ny * ny * ny <= 0) mask[y * HW + x] = 1;
        }
      }
      // outline and glow rings
      const at = (x, y) => (x < 0 || y < 0 || x >= HW || y >= HH ? 0 : mask[y * HW + x]);
      for (let y = 0; y < HH; y++) {
        for (let x = 0; x < HW; x++) {
          if (mask[y * HW + x]) continue;
          if (at(x - 1, y) === 1 || at(x + 1, y) === 1 || at(x, y - 1) === 1 || at(x, y + 1) === 1) mask[y * HW + x] = 2;
        }
      }
      for (let y = 0; y < HH; y++) {
        for (let x = 0; x < HW; x++) {
          if (mask[y * HW + x]) continue;
          if (at(x - 1, y) === 2 || at(x + 1, y) === 2 || at(x, y - 1) === 2 || at(x, y + 1) === 2) mask[y * HW + x] = 3;
        }
      }

      const glowA = (0.18 + 0.25 * beat) * (1.3 - 0.5 * day);
      for (let y = 0; y < HH; y++) {
        for (let x = 0; x < HW; x++) {
          const m = mask[y * HW + x];
          if (!m) continue;
          const px = HX0 + x;
          const py = HY0 + y;
          if (m === 3) {
            cv.blend(px, py, COL.glow, glowA);
            continue;
          }
          if (m === 2) {
            cv.set(px, py, COL.heartEdge);
            continue;
          }
          const nx = (px - cx) / s;
          const ny = -(py - cy) / s;
          const shade = nx * 0.55 - ny * 0.5;
          let c = COL.heart;
          if (shade > 0.42) c = COL.heartDark;
          else if (shade < -0.42) c = COL.heartLight;
          if ((nx + 0.48) ** 2 + (ny - 0.5) ** 2 < 0.035) c = COL.heartShine;
          cv.set(px, py, mix(c, 0xffffff, beat * 0.12));
        }
      }
    }

    function drawBirds(cv) {
      const lean = kiss > 0 ? 1 : 0;
      const bobA = Math.sin(t * 2.2) > 0.6 ? 1 : 0;
      const bobB = Math.sin(t * 2.2 + 2.5) > 0.6 ? 1 : 0;
      // sprites are 9 wide; beaks sit at ax+8 and bx, two pixels apart until they kiss
      const ax = 21 + lean;
      const bx = 32 - lean;
      cv.sprite(BIRD, ax, branchY(ax + 5) - 7 + bobA, BIRD_A);
      cv.sprite(BIRD_L, bx, branchY(bx + 3) - 7 + bobB, BIRD_B);
      if (kiss > 0) {
        // little heart rising from between their beaks
        const k = clamp(kiss / 1.4, 0, 1);
        cv.sprite(SMALL_HEART, 30, branchY(31) - 12 - Math.round((1 - k) * 3), { X: mix(0xff4d6d, 0xffc2d6, 1 - k) });
      }
    }

    function drawFloaters(cv) {
      for (const f of floaters) {
        const fadeTop = clamp((f.y - 13) / 10, 0, 1);
        const fadeAge = clamp((f.life - f.age) / 1.5, 0, 1);
        const a = 0.95 * fadeTop * fadeAge;
        const rows = f.big ? MID_HEART : SMALL_HEART;
        const ox = Math.round(f.x) - (f.big ? 2 : 1);
        const oy = Math.round(f.y) - 1;
        for (let y = 0; y < rows.length; y++) {
          for (let x = 0; x < rows[y].length; x++) {
            if (rows[y][x] === 'X') cv.blend(ox + x, oy + y, f.color, a);
          }
        }
      }
    }

    function drawSparkles(cv) {
      for (const s of sparkles) {
        const k = Math.sin((s.age / s.life) * Math.PI);
        cv.blend(s.x, s.y, COL.sparkle, k);
        if (k > 0.6) {
          for (const [dx, dy] of [[1, 0], [-1, 0], [0, 1], [0, -1]]) cv.blend(s.x + dx, s.y + dy, COL.sparkle, (k - 0.6) * 1.5);
        }
      }
    }

    function draw(cv, ctx) {
      const day = daylight(ctx.hourFloat);
      drawBackground(cv, day);
      drawBranch(cv);
      drawHeart(cv, day);
      drawBirds(cv);
      drawFloaters(cv);
      drawSparkles(cv);
      PC.drawClock(cv, ctx, COL.clock, COL.clockShadow);
    }

    return { update, draw };
  }

  PC.registerTheme({ id: 'valentines', name: "Valentine's Day", kind: 'holiday', create });
})(globalThis.PixelClock);
