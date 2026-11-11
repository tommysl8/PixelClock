/*
 * Simulator shell: runs the active theme at 30 updates per second and paints
 * the 64x64 framebuffer as individual LEDs. Everything theme-related lives in
 * engine.js, schedule.js and themes/, which is the part that gets ported to
 * the ESP32.
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
    autoLabel: $('#auto-label'),
    up: $('#btn-up'),
    down: $('#btn-down'),
    date: $('#date'),
    today: $('#today'),
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
    selection: 'auto', // 'auto' or a theme id
    activeId: null,
    theme: null,
    date: null, // {y, m, d} when previewing a date, else null (today)
    hour12: true,
    brightness: 1,
    override: null, // minutes since midnight when previewing a time, else null
    playing: false,
    shake: false,
  };

  // ---------- theme list ----------
  const SEASON_ORDER = ['winter', 'spring', 'summer', 'autumn'];
  const seasons = SEASON_ORDER.map(PC.themeById).filter(Boolean);
  const holidays = PC.HOLIDAYS.map((h) => PC.themeById(h.id)).filter(Boolean);
  const extras = PC.themes.filter((t) => !seasons.includes(t) && !holidays.includes(t));
  const cycle = ['auto', ...seasons.map((t) => t.id), ...holidays.map((t) => t.id), ...extras.map((t) => t.id)];

  function addGroup(label, list) {
    if (!list.length) return;
    const group = document.createElement('optgroup');
    group.label = label;
    for (const t of list) {
      const opt = document.createElement('option');
      opt.value = t.id;
      opt.textContent = t.name;
      group.appendChild(opt);
    }
    ui.theme.appendChild(group);
  }
  {
    const auto = document.createElement('option');
    auto.value = 'auto';
    auto.textContent = 'Auto (by date)';
    ui.theme.appendChild(auto);
    addGroup('Seasons', seasons);
    addGroup('Holidays', holidays);
    addGroup('Other', extras);
  }

  // ---------- date and time ----------
  function today() {
    const d = new Date();
    return { y: d.getFullYear(), m: d.getMonth() + 1, d: d.getDate() };
  }

  function currentDate() {
    return state.date || today();
  }

  function clockTime() {
    const d = new Date();
    let hours = d.getHours();
    let minutes = d.getMinutes();
    if (state.override !== null) {
      hours = Math.floor(state.override / 60) % 24;
      minutes = Math.floor(state.override % 60);
    }
    const seconds = d.getSeconds();
    const date = currentDate();
    return {
      hours,
      minutes,
      seconds,
      ms: d.getMilliseconds(),
      hourFloat: hours + minutes / 60 + seconds / 3600,
      year: date.y,
      month: date.m,
      day: date.d,
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

  const pad2 = (n) => String(n).padStart(2, '0');
  function syncDateInput() {
    const d = currentDate();
    ui.date.value = `${d.y}-${pad2(d.m)}-${pad2(d.d)}`;
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

  // ---------- theme switching ----------
  function wantedId() {
    if (state.selection !== 'auto') return state.selection;
    const d = currentDate();
    return PC.themeIdForDate(d.y, d.m, d.d);
  }

  function refreshTheme() {
    let id = wantedId();
    if (!PC.themeById(id)) id = PC.themes[0].id;
    if (id !== state.activeId) {
      state.activeId = id;
      state.theme = PC.themeById(id).create(rng);
    }
    ui.autoLabel.textContent = state.selection === 'auto' ? `Showing ${PC.themeById(id).name}` : '';
  }

  function select(selection) {
    state.selection = selection;
    ui.theme.value = selection;
    refreshTheme();
  }

  function step(dir) {
    const i = cycle.indexOf(state.selection);
    select(cycle[(i + dir + cycle.length) % cycle.length]);
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
  let lastSecond = -1;

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
      if (t.seconds !== lastSecond) {
        lastSecond = t.seconds;
        refreshTheme(); // auto mode follows the date across midnight
        if (!state.date && document.activeElement !== ui.date) syncDateInput();
      }
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

  ui.theme.addEventListener('change', () => select(ui.theme.value));
  ui.up.addEventListener('click', () => step(1));
  ui.down.addEventListener('click', () => step(-1));

  ui.date.addEventListener('change', () => {
    const [y, m, d] = ui.date.value.split('-').map(Number);
    state.date = y && m && d ? { y, m, d } : null;
    refreshTheme();
  });

  ui.today.addEventListener('click', () => {
    state.date = null;
    syncDateInput();
    goLive();
    refreshTheme();
  });

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
    else if (e.key === 'ArrowUp') step(1);
    else if (e.key === 'ArrowDown') step(-1);
    else if (e.key === 't' || e.key === 'T') {
      state.hour12 = !state.hour12;
      ui.hour12.checked = state.hour12;
    } else return;
    e.preventDefault();
  });

  syncDateInput();
  select('auto');
  requestAnimationFrame(loop);
})(globalThis.PixelClock);
