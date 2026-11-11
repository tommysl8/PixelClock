/*
 * New Year's Eve: a city skyline at night with twinkling windows, fireworks
 * over the rooftops and confetti drifting down. The coming year sits under
 * the clock, and from 23:59 to 00:15 the sky goes wild.
 * Shake: a volley of fireworks.
 */
(function (PC) {
  'use strict';

  const { mix, dim, hash } = PC;

  const COL = {
    skyTop: 0x04051a,
    skyBottom: 0x2e2160,
    haze: 0x6a3f86,
    back: 0x15142c,
    backEdge: 0x2a2850,
    front: 0x07070d,
    frontEdge: 0x1a1a30,
    street: 0x07070e,
    winWarm: 0xffd27a,
    winCool: 0xbfe0ff,
    winOff: 0x161628,
    beacon: 0xff3030,
    year: 0xffd23f,
    yearShadow: 0x2a1a00,
    clock: 0xfff4d6,
    clockShadow: 0x05040c,
  };

  const FIREWORK_COLORS = [0xffd23f, 0xf2f2ff, 0xff4f6a, 0x5aa8ff, 0xff7fd0, 0x7dff8a, 0xffa53a];
  const CONFETTI_COLORS = [0xffd23f, 0xff7fd0, 0x4fd8ff, 0xffffff, 0x7dff8a, 0xff8a3a];

  // Skyline: [x0, x1, top row]. Back row is taller and lighter (further away).
  const BACK = [
    [0, 6, 40], [7, 12, 34], [13, 19, 38], [20, 25, 31], [26, 33, 37],
    [34, 39, 33], [40, 47, 39], [48, 53, 30], [54, 63, 36],
  ];
  const FRONT = [
    [0, 4, 50], [5, 11, 45], [12, 16, 52], [17, 24, 47], [25, 29, 54],
    [30, 37, 44], [38, 42, 51], [48, 55, 46], [56, 63, 50],
  ];
  const SPIRE = { x: 45, top: 24, base: 42 }; // needle tower between front buildings
  const STREET_Y = 61;

  function create(rng) {
    let t = 0;
    let lastHour = -1;
    let volley = 0; // rockets left to launch quickly
    let volleyTimer = 0;

    const fw = new PC.Fireworks(rng, {
      colors: FIREWORK_COLORS,
      rate: 0.8,
      x0: 8,
      x1: 56,
      launchY: 44,
      minY: 22,
      maxY: 32,
    });

    // Lit windows on the front buildings.
    const windows = [];
    for (const [x0, x1, top] of FRONT) {
      for (let y = top + 2; y < STREET_Y - 1; y += 3) {
        for (let x = x0 + 1; x < x1; x += 2) {
          if (hash(x, y, 5) === 0) continue; // some walls have no window
          windows.push({ x, y, on: hash(x, y, 3) !== 0, warm: hash(y, x, 4) !== 0 });
        }
      }
    }
    // Faint windows on the far buildings never change.
    const farWindows = [];
    for (const [x0, x1, top] of BACK) {
      for (let y = top + 2; y < 52; y += 4) {
        for (let x = x0 + 1; x < x1; x += 3) if (hash(x, y, 4) === 0) farWindows.push([x, y]);
      }
    }

    const confetti = [];
    function addConfetti(anywhere) {
      confetti.push({
        x: rng.float() * 64,
        y: anywhere ? rng.float() * 60 : -1 - rng.float() * 6,
        vy: 5 + rng.float() * 6,
        ph: rng.float() * 6.283,
        sway: 0.6 + rng.float() * 1.2,
        c: CONFETTI_COLORS[rng.int(CONFETTI_COLORS.length)],
      });
    }
    for (let i = 0; i < 14; i++) addConfetti(true);

    function isRush(ctx) {
      return (ctx.hours === 23 && ctx.minutes >= 59) || (ctx.hours === 0 && ctx.minutes < 15);
    }

    function update(dt, ctx) {
      t += dt;
      const rush = isRush(ctx);

      // The stroke of midnight: everything at once.
      if (lastHour === 23 && ctx.hours === 0) {
        volley += 10;
        for (let i = 0; i < 4; i++) fw.burst(10 + rng.float() * 44, 20 + rng.float() * 10, FIREWORK_COLORS[rng.int(FIREWORK_COLORS.length)]);
      }
      lastHour = ctx.hours;

      if (ctx.shake) volley += 8;
      if (volley > 0) {
        volleyTimer -= dt;
        if (volleyTimer <= 0) {
          fw.launch(8 + rng.float() * 48);
          volley--;
          volleyTimer = 0.12;
        }
      }

      fw.rate = rush ? 3.5 : 0.8;
      fw.update(dt);

      // Windows flick on and off now and then.
      if (rng.chance(dt * 2)) {
        const w = windows[rng.int(windows.length)];
        w.on = !w.on;
      }

      const target = rush ? 70 : 16;
      if (confetti.length < target && rng.chance(rush ? 0.9 : 0.25)) addConfetti(false);
      for (let i = confetti.length - 1; i >= 0; i--) {
        const p = confetti[i];
        p.y += p.vy * dt;
        p.x += Math.sin(t * 2.5 + p.ph) * p.sway * dt * 6;
        if (p.y > STREET_Y + 2) {
          if (confetti.length > target) confetti.splice(i, 1);
          else {
            p.y = -1;
            p.x = rng.float() * 64;
          }
        }
      }
    }

    function drawSkyline(cv) {
      for (const [x0, x1, top] of BACK) {
        cv.rect(x0, top, x1, STREET_Y, COL.back);
        cv.rect(x0, top, x1, top, COL.backEdge);
      }
      for (const [x, y] of farWindows) cv.set(x, y, 0x3a3a62);

      // needle tower with a blinking beacon
      cv.rect(SPIRE.x - 1, SPIRE.base - 8, SPIRE.x + 1, STREET_Y, COL.back);
      cv.rect(SPIRE.x - 2, SPIRE.base - 10, SPIRE.x + 2, SPIRE.base - 9, COL.backEdge);
      cv.rect(SPIRE.x, SPIRE.top, SPIRE.x, SPIRE.base - 11, COL.backEdge);
      cv.set(SPIRE.x, SPIRE.top - 1, Math.floor(t * 1.2) % 2 === 0 ? COL.beacon : dim(COL.beacon, 0.3));

      for (const [x0, x1, top] of FRONT) {
        cv.rect(x0, top, x1, STREET_Y, COL.front);
        cv.rect(x0, top, x1, top, COL.frontEdge);
      }
      for (const w of windows) cv.set(w.x, w.y, w.on ? (w.warm ? COL.winWarm : COL.winCool) : COL.winOff);

      cv.rect(0, STREET_Y + 1, 63, 63, COL.street);
      // street lights
      for (let x = 3; x < 64; x += 12) cv.set(x, STREET_Y + 1, 0xffc870);
    }

    function draw(cv, ctx) {
      PC.vgradient(cv, 0, 0, 63, STREET_Y, COL.skyTop, COL.skyBottom);
      // city glow just above the rooftops
      for (let y = 24; y < 46; y++) {
        const a = ((y - 24) / 22) * 0.5;
        for (let x = 0; x < 64; x++) cv.blend(x, y, COL.haze, a);
      }
      PC.drawStars(cv, t, 0.7, 0, 0, 63, 26, 22);

      fw.draw(cv);
      drawSkyline(cv);

      for (const p of confetti) {
        const flip = Math.sin(t * 9 + p.ph);
        cv.set(p.x, p.y, flip > 0 ? p.c : dim(p.c, 0.45));
      }

      // the year below the clock: on Dec 31 it's the year that's coming
      const year = ctx.month === 12 && ctx.day === 31 ? ctx.year + 1 : ctx.year;
      const text = String(year);
      const x = Math.floor((64 - (text.length * 4 - 1)) / 2);
      PC.drawSmallNumber(cv, text, x + 1, 17, COL.yearShadow);
      const glint = mix(COL.year, 0xfff6c8, Math.max(0, Math.sin(t * 1.3)) ** 6);
      PC.drawSmallNumber(cv, text, x, 16, glint);

      PC.drawClock(cv, ctx, COL.clock, COL.clockShadow);
    }

    return { update, draw };
  }

  PC.registerTheme({ id: 'newyear', name: "New Year's Eve", kind: 'holiday', create });
})(globalThis.PixelClock);
