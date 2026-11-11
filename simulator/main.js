/*
 * Simulator shell: runs the active theme at 30 updates per second and paints
 * the 64x64 framebuffer as individual LEDs. Everything theme-related lives in
 * engine.js + themes/, which is the part that gets ported to the ESP32.
 */
(function (PC) {
  'use strict';

  const STEP = 1 / 30; // fixed update rate, same as the firmware will use
  const LED = 8; // canvas pixels per LED
  const $ = (sel) => document.querySelector(sel);

  const display = $('#display');
  const g = display.getContext('2d');
  display.width = PC.W * LED;
  display.height = PC.H * LED;

  const fb = new PC.Canvas();
  const rng = new PC.Rng((Date.now() ^ 0x5bd1e995) >>> 0);

  const ui = {
    frame: $('#frame'),
    theme: $('#theme'),
    up: $('#btn-up'),
    down: $('#btn-down'),
    time: $('#time'),
    timeLabel: $('#time-label'),
    live: $('#live'),
    playDay: $('#play-day'),
    hour12: $('#hour12'),
    brightness: $('#brightness'),
    brightnessLabel: $('#brightness-label'),
    diffuser: $('#diffuser'),
    shake: $('#shake'),
  };

  const state = {
    themeIndex: 0,
    theme: null,
    hour12: true,
    brightness: 1,
    override: null, // minutes since midnight when previewing a time, else null
    playing: false,
    shake: false,
  };

  // ---------- themes ----------
  PC.themes.forEach((t, i) => {
    const opt = document.createElement('option');
    opt.value = String(i);
    opt.textContent = t.name;
    ui.theme.appendChild(opt);
  });

  function setTheme(i) {
    const n = PC.themes.length;
    state.themeIndex = ((i % n) + n) % n;
    state.theme = PC.themes[state.themeIndex].create(rng);
    ui.theme.value = String(state.themeIndex);
  }

  // ---------- time ----------
  function clockTime() {
    const d = new Date();
    let hours = d.getHours();
    let minutes = d.getMinutes();
    if (state.override !== null) {
      hours = Math.floor(state.override / 60) % 24;
      minutes = Math.floor(state.override % 60);
    }
    const seconds = d.getSeconds();
    return {
      hours,
      minutes,
      seconds,
      ms: d.getMilliseconds(),
      hourFloat: hours + minutes / 60 + seconds / 3600,
    };
  }

  function nowMinutes() {
    const d = new Date();
    return d.getHours() * 60 + d.getMinutes();
  }

  function fmt(h, m) {
    const hh = state.hour12 ? h % 12 || 12 : h;
    const suffix = state.hour12 ? (h < 12 ? ' am' : ' pm') : '';
    return `${state.hour12 ? hh : String(hh).padStart(2, '0')}:${String(m).padStart(2, '0')}${suffix}`;
  }

  function goLive() {
    state.override = null;
    state.playing = false;
    ui.live.checked = true;
    ui.playDay.setAttribute('aria-pressed', 'false');
  }

  function goPreview(minutes) {
    state.override = minutes;
    ui.live.checked = false;
  }

  // ---------- rendering ----------
  const hexCache = new Map();
  function css(c) {
    let s = hexCache.get(c);
    if (!s) {
      s = '#' + c.toString(16).padStart(6, '0');
      if (hexCache.size > 8192) hexCache.clear();
      hexCache.set(c, s);
    }
    return s;
  }

  const UNLIT = 0x0c0c0c;

  function paint() {
    g.fillStyle = '#030303';
    g.fillRect(0, 0, display.width, display.height);
    const k = state.brightness;
    for (let y = 0; y < PC.H; y++) {
      for (let x = 0; x < PC.W; x++) {
        let c = fb.px[y * PC.W + x];
        if (k < 1) c = PC.dim(c, k);
        const lum = ((c >> 16) & 255) + ((c >> 8) & 255) + (c & 255);
        g.fillStyle = lum < 24 ? css(UNLIT) : css(c);
        g.fillRect(x * LED + 1, y * LED + 1, LED - 2, LED - 2);
      }
    }
  }

  // ---------- main loop ----------
  let last = performance.now();
  let acc = 0;

  function loop(ts) {
    acc += Math.min(0.25, (ts - last) / 1000);
    last = ts;

    let stepped = false;
    while (acc >= STEP) {
      acc -= STEP;
      if (state.playing) {
        // a full day in about 24 seconds
        state.override = ((state.override ?? nowMinutes()) + 2) % 1440;
      }
      const t = clockTime();
      state.theme.update(STEP, { ...t, hour12: state.hour12, shake: state.shake });
      state.shake = false;
      stepped = true;
    }

    if (stepped) {
      const t = clockTime();
      state.theme.draw(fb, { ...t, hour12: state.hour12 });
      paint();
      ui.timeLabel.textContent = fmt(t.hours, t.minutes);
      if (document.activeElement !== ui.time) ui.time.value = String(t.hours * 60 + t.minutes);
    }
    requestAnimationFrame(loop);
  }

  // ---------- controls ----------
  function shake() {
    state.shake = true;
    ui.frame.classList.remove('shaking');
    void ui.frame.offsetWidth; // restart the CSS animation
    ui.frame.classList.add('shaking');
  }

  ui.theme.addEventListener('change', () => setTheme(Number(ui.theme.value)));
  ui.up.addEventListener('click', () => setTheme(state.themeIndex + 1));
  ui.down.addEventListener('click', () => setTheme(state.themeIndex - 1));

  ui.time.addEventListener('input', () => {
    state.playing = false;
    ui.playDay.setAttribute('aria-pressed', 'false');
    goPreview(Number(ui.time.value));
  });

  ui.live.addEventListener('change', () => {
    if (ui.live.checked) goLive();
    else goPreview(nowMinutes());
  });

  ui.playDay.addEventListener('click', () => {
    state.playing = !state.playing;
    ui.playDay.setAttribute('aria-pressed', String(state.playing));
    if (state.playing) goPreview(state.override ?? nowMinutes());
  });

  ui.hour12.addEventListener('change', () => {
    state.hour12 = ui.hour12.checked;
  });

  ui.brightness.addEventListener('input', () => {
    state.brightness = Number(ui.brightness.value) / 100;
    ui.brightnessLabel.textContent = `${ui.brightness.value}%`;
  });

  ui.diffuser.addEventListener('change', () => {
    ui.frame.classList.toggle('diffused', ui.diffuser.checked);
  });

  ui.shake.addEventListener('click', shake);

  document.addEventListener('keydown', (e) => {
    if (e.target instanceof HTMLInputElement || e.target instanceof HTMLSelectElement) return;
    if (e.key === 's' || e.key === 'S') shake();
    else if (e.key === 'ArrowUp') setTheme(state.themeIndex + 1);
    else if (e.key === 'ArrowDown') setTheme(state.themeIndex - 1);
    else if (e.key === 't' || e.key === 'T') {
      state.hour12 = !state.hour12;
      ui.hour12.checked = state.hour12;
    } else return;
    e.preventDefault();
  });

  setTheme(0);
  requestAnimationFrame(loop);
})(window.PixelClock);
