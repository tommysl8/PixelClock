# PixelClock

A cozy 64×64 LED pixel-art desk clock with animated themes for every season and major holiday. It switches themes by itself through the year, the sky follows the real time of day, and each theme has a surprise when you shake it.

Built on an ESP32-S3 (Adafruit Matrix Portal S3) driving an RGB LED matrix. Every theme can be designed and previewed in a browser simulator before it goes on the hardware.

## Try the simulator

**Live: [tommysl8.github.io/PixelClock](https://tommysl8.github.io/PixelClock/)**

Or open `simulator/index.html` in any browser. No install or build step is needed.

- **Theme** defaults to **Auto (by date)**, which picks the holiday or season for the date. The ▲ ▼ buttons work like the two buttons on the real board.
- **Date** previews any day of the year, so you can see what the clock will show on Halloween or New Year's Eve.
- **Time** scrubs through the day so you can preview sunrise, daytime, sunset and night. **Play a day** runs a full day in about 24 seconds.
- **Diffuser** shows the soft glow you get with a smoked acrylic panel in front of the LEDs.
- **Shake it** (or press <kbd>S</kbd>) triggers the theme's shake effect. On the real clock this comes from the board's accelerometer.

## Themes

In auto mode the clock shows a holiday theme in the days around each holiday, and the season's theme the rest of the time.

**Seasons**

| Theme | Shown |
|---|---|
| Snowy hills | December–February |
| Spring meadow | March–May |
| Summer beach | June–August |
| Autumn forest | September–November |

**Holidays**

| Theme | Shown |
|---|---|
| New Year's Eve | Dec 31–Jan 1 |
| Lunar New Year | The day before to 3 days after (date varies; table covers 2025–2035) |
| Valentine's Day | Feb 11–14 |
| St. Patrick's Day | Mar 14–17 |
| Easter | Thursday before Easter through Easter Sunday |
| Fourth of July | Jul 1–4 |
| Haunted hill (Halloween) | Oct 15–31 |
| Harvest field (Thanksgiving) | The Monday before through Thanksgiving Day |
| Christmas cabin | Dec 1–26 |

The rules live in `simulator/schedule.js`, so changing a date window is a one-line edit.

## Hardware

All from Adafruit, which only guarantees its own panels with the Matrix Portal.

| Part | Notes | Price |
|---|---|---|
| [Adafruit Matrix Portal S3](https://www.adafruit.com/product/5778) (PID 5778) | ESP32-S3 with WiFi, 2 buttons, accelerometer, USB-C. Plugs straight onto the panel. The PCB-antenna version. | $19.95 |
| [64x64 RGB LED Matrix Panel, 2.5mm pitch, 45° curb-cut](https://www.adafruit.com/product/5407) (PID 5407) | 160 × 160 mm, 1/32 scan, 5 address lines. Includes the power cable and ribbon cable. "Curb-cut" only means the edges are bevelled. | $54.95 |
| [Official Raspberry Pi USB-C power supply, 5.1V 3A](https://www.adafruit.com/product/4298) (PID 4298) | Thick 1.5 m 18 AWG cable, US plug. The firmware's power limit keeps the clock well under 3A. | $8.74 |
| [Adjustable bent-wire stand](https://www.adafruit.com/product/1679) (PID 1679) | Holds the panel upright on a desk | $4.95 |
| [USB-A to USB-C cable, 1 m](https://www.adafruit.com/product/4474) (PID 4474) | For flashing from a PC. Skip it if you already have a USB-C *data* cable. | $4.95 |
| Optional: [black LED diffusion acrylic, 12" × 12"](https://www.adafruit.com/product/4594) (PID 4594) | Cuts glare and sharpens pixels. Has to be cut to 160 × 160 mm with a laser cutter or saw. | $9.95 |

Prices and stock as checked on Adafruit in November 2026. Adafruit's 2 mm 64×64 panel (PID 5362, 128 mm) also works with this firmware if it's back in stock.

You also need a soldering iron for one solder blob (see Wiring).

Finished size is about 16 cm square, plus a frame if you add one.

## Wiring

1. Plug the Matrix Portal S3 onto the panel's **input** HUB75 connector (the arrows printed on the panel point away from the input).
2. Screw the panel's power cable into the Matrix Portal's screw terminals: red to `+`, black to `−`.
3. Close the **address E** jumper on the bottom of the Matrix Portal with a small blob of solder, so the middle pad connects to the pad marked **8**. 64×64 panels need this extra address line, and pin 8 is right for Adafruit's 64×64 panels. It's the only soldering in the project.
4. Connect the Raspberry Pi USB-C power supply.

Photos and a step-by-step guide will be added once the build is done.

## Firmware

The firmware in `firmware/` runs on the Matrix Portal S3. It uses the same themes as the simulator, ported to C++ and checked pixel for pixel against the JavaScript.

**First boot:** the clock shows a WiFi symbol and opens a WiFi network called **PixelClock-Setup**. Join it from your phone, pick your home WiFi, and set your time zone if you're not on US Central time. The clock then sets its time from the internet and remembers everything.

**Buttons**

| Press | Does |
|---|---|
| ▲ or ▼ | Next or previous theme. The first entry is Auto (by date). |
| Hold ▲ for 1 s | Cycle through 4 brightness levels |
| Hold ▲ and ▼ for 5 s | Forget WiFi and restart into setup |

Shake the clock to trigger the theme's shake effect. Between 11 pm and 6:30 am the display dims automatically.

**Time zones** use POSIX TZ strings:

| Zone | String |
|---|---|
| US Eastern | `EST5EDT,M3.2.0,M11.1.0` |
| US Central | `CST6CDT,M3.2.0,M11.1.0` |
| US Mountain | `MST7MDT,M3.2.0,M11.1.0` |
| Arizona | `MST7` |
| US Pacific | `PST8PDT,M3.2.0,M11.1.0` |
| UK | `GMT0BST,M3.5.0/1,M10.5.0` |
| Central Europe | `CET-1CEST,M3.5.0,M10.5.0/3` |

**Build and flash** with [PlatformIO](https://platformio.org/). Plug the Matrix Portal S3 into your computer with a USB-C data cable, then:

```bash
cd firmware
.venv/Scripts/pio run -t upload
```

(Use plain `pio` if PlatformIO is installed globally. Add `pio device monitor` to watch the serial log.)

Brightness levels, the night-dimming hours, gamma and the power limit are constants at the top of `firmware/src/main.cpp`. The power limit caps the panel's average brightness at 40% of full white (Adafruit rates a 64×64 panel at up to 4A at full white), so the 5.1V 3A supply is never overloaded, even on bright daytime scenes.

**Checking the C++ port against the simulator:**

```bash
node tools/compare.js
```

This builds the C++ themes for your PC (with the Zig compiler from `firmware/.venv`), runs every theme through the same scenes in both languages, and reports how many pixels match. Add `--out DIR` to save side-by-side images of any differences, or `--timing` to see how long each frame takes.

To set up the tools on a new machine:

```bash
python -m venv firmware/.venv
firmware/.venv/Scripts/pip install platformio ziglang
```

## How themes work

The display is a 64×64 framebuffer of `0xRRGGBB` colours. A theme is a small module with two functions:

- `update(dt, ctx)` runs 30 times a second and advances the animation (fire, snow, lights).
- `draw(canvas, ctx)` paints the current frame.

`ctx` carries the time of day, the date, the 12/24-hour setting, and events like `shake`. The engine (`simulator/engine.js`) uses only integer colours and a deterministic xorshift RNG, so themes port directly to C++ on the ESP32.

```
simulator/
  index.html       simulator page
  main.js          render loop and controls (browser only)
  engine.js        framebuffer, colours, RNG, clock font, sky, stars, fireworks
  schedule.js      which theme to show on which date
  themes/          one file per theme
firmware/
  platformio.ini   board, libraries and build flags
  src/main.cpp     panel, WiFi, time, buttons, shake, brightness
  lib/PixelClock/  C++ port of engine.js, schedule.js and every theme
  native/          PC test harness used by tools/compare.js
tools/
  render.js        renders themes to PNG without a browser
  compare.js       checks the C++ themes against the JavaScript ones
```

When you change a theme, change both the `.js` and the `.cpp` and run `node tools/compare.js` to confirm they still match.

### Writing a theme

1. Copy an existing theme in `simulator/themes/` and give it a new `id`, `name` and `kind` (`'season'` or `'holiday'`).
2. Keep rows 0–13 calm: that's where the clock sits. Draw the clock last with `PC.drawClock(cv, ctx, color, shadow)`.
3. Use the theme's `rng` for all randomness (never `Math.random`), so it behaves the same on the ESP32.
4. Add a `<script>` tag for it in `simulator/index.html`. For a holiday, add its dates to `HOLIDAYS` in `schedule.js`.
5. To put it on the clock, port it to `firmware/lib/PixelClock/src/themes/` (copy `christmas.cpp` as a pattern), add it to `themes.h`, `themes.cpp`, the schedule in `schedule.cpp` and `FACTORY` in `tools/compare.js`, then run `node tools/compare.js <id>` until it matches 100%.

To preview themes without a browser:

```bash
node tools/render.js all --hour 21
```

That writes a contact sheet of every theme at 9 pm to `render-all.png`. You can also render one theme (`node tools/render.js halloween`), whatever auto mode picks on a date (`node tools/render.js auto --date 2026-12-31`), several frames in a row (`--frames 4`), or the shake effect (`--shake`).

## Roadmap

- [x] Browser simulator
- [x] A theme for each season and major holiday
- [x] Auto-switch themes by date
- [x] Simulator hosted on GitHub Pages
- [x] Firmware for the Matrix Portal S3 (Arduino + Adafruit Protomatter), all themes ported to C++
- [x] WiFi setup from your phone, time sync (NTP) with time zones
- [x] Theme switching with the board buttons; shake effects with the accelerometer
- [x] Brightness levels, night dimming and a power limit
- [ ] Tune colours and gamma on the real panel
- [ ] Phone control page: theme, brightness, night dimming
- [ ] Wiring photos and a printable frame
