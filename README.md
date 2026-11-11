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

| Part | Notes | Approx. cost |
|---|---|---|
| Adafruit Matrix Portal S3 | ESP32-S3 with WiFi, 2 buttons, accelerometer, USB-C. Plugs straight onto the panel. | $20 |
| 64×64 RGB LED matrix, **P2** pitch, HUB75, 1/32 scan | P2 makes it about 12.8 cm (5") square | $25–40 |
| 5V 4A USB-C power supply | LEDs are power hungry; a phone charger may not be enough | $10 |
| USB-C data cable | Must carry data, not just charge | — |
| Optional: smoked acrylic sheet or diffuser film | Turns individual dots into a soft glow | $10 |
| Optional: frame | Shadow-box frame, wood, or 3D-printed | — |

Finished size is roughly 14.8 cm square with a frame and 3.5–4 cm deep.

## Wiring

1. Plug the Matrix Portal S3 onto the panel's **input** HUB75 connector (the arrows printed on the panel point away from the input).
2. Screw the panel's power cable into the Matrix Portal's screw terminals: red to `+`, black to `−`.
3. Bridge the **address E** jumper on the Matrix Portal with a small blob of solder. 64×64 panels need this extra address line. It's the only soldering in the project.
4. Connect USB-C to the 5V 4A supply.

Photos and a step-by-step guide will be added once the build is done.

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
tools/
  render.js        renders themes to PNG without a browser
```

### Writing a theme

1. Copy an existing theme in `simulator/themes/` and give it a new `id`, `name` and `kind` (`'season'` or `'holiday'`).
2. Keep rows 0–13 calm: that's where the clock sits. Draw the clock last with `PC.drawClock(cv, ctx, color, shadow)`.
3. Use the theme's `rng` for all randomness (never `Math.random`), so it behaves the same on the ESP32.
4. Add a `<script>` tag for it in `simulator/index.html`. For a holiday, add its dates to `HOLIDAYS` in `schedule.js`.

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
- [ ] Firmware for the Matrix Portal S3 (Arduino + Adafruit Protomatter)
- [ ] WiFi time sync (NTP)
- [ ] Theme switching with the board buttons; shake effects with the accelerometer
- [ ] Phone control page: theme, brightness, night dimming
- [ ] Wiring photos and a printable frame
