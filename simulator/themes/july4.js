/*
 * Fourth of July: a waving flag on the shore of a lake, red, white and blue
 * fireworks over the treeline, and their reflections shimmering on the water.
 * Always dusk or night, because fireworks need the dark.
 * Shake: grand finale.
 */
(function (PC) {
  'use strict';

  const { mix, dim, hash, clamp } = PC;

  const COL = {
    duskTop: 0x1b1f4a,
    duskBottom: 0xa04a5e,
    nightTop: 0x04071a,
    nightBottom: 0x2a3468,
    trees: 0x05080c,
    treesEdge: 0x131c26,
    shore: 0x0a0d10,
    waterTop: 0x0c1838,
    waterBottom: 0x040816,
    pole: 0xb4b8c4,
    poleShade: 0x6a6e78,
    finial: 0xe8c547,
    red: 0xd8283a,
    white: 0xf2f2f2,
    blue: 0x1f3a8a,
    clock: 0xf4f6ff,
    clockShadow: 0x05070f,
  };

  const FIREWORK_COLORS = [0xff3b3b, 0xffffff, 0x4f8bff, 0xff3b3b, 0x4f8bff, 0xffe08a];

  const HORIZON = 46; // waterline: the reflection mirrors around this row
  const FLAG = { x: 11, y: 21, w: 17, h: 11 }; // flag cloth, attached to the pole at x - 1
  const POLE_X = 10;

  function create(rng) {
    let t = 0;
    let finale = 0; // seconds of finale left
    let volley = 0;
    let volleyTimer = 0;

    const fw = new PC.Fireworks(rng, {
      colors: FIREWORK_COLORS,
      rate: 0.9,
      x0: 24,
      x1: 58,
      launchY: HORIZON - 4,
      minY: 19,
      maxY: 30,
    });

    // Treeline heights per column: pines of varying sizes.
    const treeTop = new Int8Array(64);
    for (let x = 0; x < 64; x++) treeTop[x] = 44;
    let x = -2;
    while (x < 66) {
      const h = 3 + rng.int(6);
      const half = 1 + Math.floor(h / 2.5);
      for (let dx = -half; dx <= half; dx++) {
        const cx = x + dx;
        if (cx < 0 || cx > 63) continue;
        const top = 44 - h + Math.abs(dx) * 2;
        if (top < treeTop[cx]) treeTop[cx] = top;
      }
      x += 2 + rng.int(4);
    }

    function update(dt, ctx) {
      t += dt;
      if (ctx.shake) {
        finale = 5;
        volley += 10;
      }
      if (volley > 0) {
        volleyTimer -= dt;
        if (volleyTimer <= 0) {
          fw.launch(24 + rng.float() * 34);
          volley--;
          volleyTimer = 0.1;
        }
      }
      if (finale > 0) finale -= dt;
      fw.rate = finale > 0 ? 5 : 0.9;
      fw.update(dt);
    }

    function drawFlag(cv) {
      // pole
      cv.rect(POLE_X, FLAG.y - 1, POLE_X, HORIZON, COL.pole);
      cv.set(POLE_X, FLAG.y - 2, COL.finial);
      cv.rect(POLE_X - 1, HORIZON - 1, POLE_X + 1, HORIZON, COL.poleShade);

      for (let c = 0; c < FLAG.w; c++) {
        // wave travels away from the pole and grows toward the free end
        const phase = t * 4 - c * 0.45;
        const dy = Math.round(Math.sin(phase) * 1.3 * (c / FLAG.w));
        const shade = 0.78 + 0.22 * Math.cos(phase);
        for (let r = 0; r < FLAG.h; r++) {
          let col;
          if (c < 8 && r < 6) {
            col = COL.blue;
            if (c % 2 === 1 && r % 2 === 1 && c < 7) col = COL.white;
            else if (c % 2 === 0 && r % 2 === 0 && c > 0 && r > 0 && r < 5) col = COL.white;
          } else {
            col = r % 2 === 0 ? COL.red : COL.white;
          }
          cv.set(FLAG.x + c, FLAG.y + r + dy, dim(col, shade));
        }
      }
    }

    function drawWater(cv, night) {
      const top = mix(mix(0x2a2a52, COL.waterTop, night), COL.waterTop, 0.5);
      for (let y = HORIZON + 1; y < 64; y++) {
        const depth = (y - HORIZON - 1) / (63 - HORIZON - 1);
        const base = mix(top, COL.waterBottom, depth);
        // Mirrored source row, squashed so the whole sky fits in the shallow lake.
        const sy = Math.round(HORIZON - (y - HORIZON) * 2.2);
        const a = 0.55 * (1 - depth * 0.6);
        for (let x = 0; x < 64; x++) {
          const wob = Math.round(Math.sin(y * 0.9 + t * 3 + x * 0.15) * (0.6 + depth * 1.4));
          const src = cv.get(clamp(x + wob, 0, 63), sy);
          let c = mix(base, src, a);
          // little horizontal glints that drift across the surface
          if (hash(x + Math.floor(t * 4), y, 97) === 0) c = mix(c, 0x9fb8e8, 0.3);
          cv.set(x, y, c);
        }
      }
    }

    function draw(cv, ctx) {
      const night = 1 - PC.daylight(ctx.hourFloat);
      const top = mix(COL.duskTop, COL.nightTop, night);
      const bottom = mix(COL.duskBottom, COL.nightBottom, night);
      PC.vgradient(cv, 0, 0, 63, HORIZON, top, bottom);
      PC.drawStars(cv, t, 0.35 + 0.65 * night, 0, 0, 63, 34, 24);

      fw.draw(cv);

      // treeline and shore
      for (let x = 0; x < 64; x++) {
        cv.rect(x, treeTop[x], x, HORIZON, COL.trees);
        cv.set(x, treeTop[x], COL.treesEdge);
      }
      cv.rect(0, HORIZON - 1, 63, HORIZON, COL.shore);

      drawFlag(cv);
      drawWater(cv, night);

      PC.drawClock(cv, ctx, COL.clock, COL.clockShadow);
    }

    return { update, draw };
  }

  PC.registerTheme({ id: 'july4', name: 'Fourth of July', kind: 'holiday', create });
})(globalThis.PixelClock);
