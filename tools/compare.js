#!/usr/bin/env node
/*
 * Checks the C++ firmware themes against the JavaScript originals.
 *
 * Compiles firmware/native/render.cpp plus the whole C++ engine to WebAssembly
 * with the Zig compiler (installed in firmware/.venv), runs every theme through
 * the same scenes in both languages inside Node, and compares the frames pixel
 * by pixel. WebAssembly keeps this portable and avoids Windows blocking freshly
 * built test programs.
 *
 *   node tools/compare.js                    all themes
 *   node tools/compare.js halloween easter   just these
 *   node tools/compare.js --out DIR          also write js|c++|diff PNGs for any mismatch
 *   node tools/compare.js --timing           also print C++ time per frame
 *   node tools/compare.js --float            float math like the ESP32: small drift is expected,
 *                                            so this mode reports but never fails
 *
 * Exit code 1 if any theme matches less than 99.5% of pixels.
 */
'use strict';

const fs = require('fs');
const os = require('os');
const path = require('path');
const { execFileSync } = require('child_process');
const { WASI } = require('node:wasi');
const { PC, run, sheet } = require('./render.js');

const ROOT = path.join(__dirname, '..');
const FW = path.join(ROOT, 'firmware');
const LIB = path.join(FW, 'lib', 'PixelClock', 'src');
const BUILD = path.join(FW, 'native', 'build');
const PY = path.join(FW, '.venv', process.platform === 'win32' ? 'Scripts/python.exe' : 'bin/python');
const THRESHOLD = 99.5;

const args = process.argv.slice(2);
const opt = (name) => {
  const i = args.indexOf(`--${name}`);
  return i >= 0 ? args[i + 1] : undefined;
};
const outDir = opt('out');
const floatMode = args.includes('--float');
const ids = args.filter((a, i) => !a.startsWith('--') && !(i > 0 && args[i - 1] === '--out'));
const WASM = path.join(BUILD, `pc_render${floatMode ? '-float' : ''}.wasm`);

// ---------- build ----------
function sources() {
  const list = [path.join(FW, 'native', 'render.cpp')];
  const walk = (d) => {
    for (const f of fs.readdirSync(d)) {
      const p = path.join(d, f);
      if (fs.statSync(p).isDirectory()) walk(p);
      else if (f.endsWith('.cpp')) list.push(p);
    }
  };
  walk(LIB);
  return list;
}

function build() {
  const srcs = sources();
  const headers = fs.readdirSync(LIB).filter((f) => f.endsWith('.h')).map((f) => path.join(LIB, f));
  const newest = Math.max(...[...srcs, ...headers, __filename].map((f) => fs.statSync(f).mtimeMs));
  if (fs.existsSync(WASM) && fs.statSync(WASM).mtimeMs > newest) return;
  fs.mkdirSync(BUILD, { recursive: true });
  process.stdout.write(`Building ${path.basename(WASM)}... `);
  execFileSync(PY, ['-m', 'ziglang', 'c++', '-target', 'wasm32-wasi', '-std=c++17', '-O2', '-Wall',
    '-fno-exceptions', ...(floatMode ? ['-DPIXELCLOCK_FLOAT'] : []), `-I${LIB}`, ...srcs, '-o', WASM],
    { stdio: ['ignore', 'inherit', 'inherit'] });
  console.log('done');
}

// ---------- run the C++ side ----------
let wasmModule = null;
const scratch = fs.mkdtempSync(path.join(os.tmpdir(), 'pc-compare-'));

function runNative(argv) {
  wasmModule = wasmModule || new WebAssembly.Module(fs.readFileSync(WASM));
  const wasi = new WASI({ version: 'preview1', args: ['pc_render', ...argv], preopens: { '/out': scratch }, returnOnExit: true });
  const inst = new WebAssembly.Instance(wasmModule, wasi.getImportObject());
  return wasi.start(inst);
}

function nativeFrames(id, s) {
  const a = [id, '--hour', String(s.hour), '--seconds', String(s.warmup), '--frames', String(s.frames),
    '--gap', String(s.gap), '--seed', String(s.seed), '--date', s.date.join('-'), '--raw', '/out/frame.bin'];
  if (s.shake) a.push('--shake');
  const code = runNative(a);
  if (code !== 0) throw new Error(`exit ${code}`);
  const buf = fs.readFileSync(path.join(scratch, 'frame.bin'));
  const frames = [];
  for (let f = 0; f < s.frames; f++) {
    const px = new Uint32Array(4096);
    for (let i = 0; i < 4096; i++) px[i] = buf.readUInt32LE((f * 4096 + i) * 4);
    frames.push(px);
  }
  return frames;
}

// ---------- scenes ----------
const SCENARIOS = [
  { name: 'noon', hour: 12.5 },
  { name: 'night', hour: 21 },
  { name: 'dusk frames', hour: 18.2, frames: 3, gap: 0.7 },
  { name: 'dawn', hour: 7.1, warmup: 5 },
  { name: 'shake', hour: 12, warmup: 4, shake: true },
  { name: 'long run', hour: 23.9, warmup: 40, date: [2026, 12, 31] },
];

function diffImage(a, b) {
  const d = new Uint32Array(4096);
  for (let i = 0; i < 4096; i++) d[i] = a[i] === b[i] ? 0x000000 : 0xff00ff;
  return d;
}

// ---------- main ----------
build();
const themes = ids.length ? ids.map((id) => PC.themeById(id)).filter(Boolean) : PC.themes;
let failed = 0;
for (const th of themes) {
  let same = 0;
  let total = 0;
  const notes = [];
  for (const base of SCENARIOS) {
    const s = { frames: 1, gap: 0.5, warmup: 3, seed: 12345, date: [2026, 11, 11], shake: false, ...base };
    const js = run(th, { ...s, year: s.date[0], month: s.date[1], day: s.date[2] });
    let cc;
    try {
      cc = nativeFrames(th.id, s);
    } catch (e) {
      notes.push(`${s.name}: C++ run failed (${e.message})`);
      total += 4096 * s.frames;
      continue;
    }
    let sceneSame = 0;
    for (let f = 0; f < s.frames; f++) {
      for (let i = 0; i < 4096; i++) if (js[f][i] === cc[f][i]) sceneSame++;
    }
    same += sceneSame;
    total += 4096 * s.frames;
    const pct = (100 * sceneSame) / (4096 * s.frames);
    if (pct < 100) {
      notes.push(`${s.name} ${pct.toFixed(2)}%`);
      if (outDir) {
        fs.mkdirSync(outDir, { recursive: true });
        const panels = [js[0], cc[0], diffImage(js[0], cc[0])];
        fs.writeFileSync(path.join(outDir, `${th.id}-${s.name.replace(/\s+/g, '-')}.png`), sheet(panels, 3, 4));
      }
    }
  }
  const pct = (100 * same) / total;
  const ok = floatMode || pct >= THRESHOLD;
  if (!ok) failed++;
  console.log(`${ok ? 'PASS' : 'FAIL'}  ${th.id.padEnd(13)} ${pct.toFixed(2)}% identical${notes.length ? '  (' + notes.join(', ') + ')' : ''}`);
  if (args.includes('--timing')) runNative([th.id, '--hour', '20', '--seconds', '20', '--timing']);
}
fs.rmSync(scratch, { recursive: true, force: true });
process.exit(failed ? 1 : 0);
