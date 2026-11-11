# PixelClock

A cozy 64×64 LED pixel-art desk clock with seasonal, animated themes: a crackling fireplace and twinkling tree in winter, a sky that follows the real time of day, and a snow-globe effect when you shake it.

Built on an ESP32-S3 (Adafruit Matrix Portal S3) driving an RGB LED matrix. Every theme can be designed and previewed in a browser simulator before it goes on the hardware.

## Try the simulator

Open `simulator/index.html` in any browser. No install or build step is needed.

- **Theme** and the ▲ ▼ buttons work like the two buttons on the real board.
- **Time** scrubs through the day so you can preview sunrise, daytime, sunset and night. **Play a day** runs a full day in about 24 seconds.
- **Diffuser** shows the soft glow you get with a smoked acrylic panel in front of the LEDs.
- **Shake it** (or press <kbd>S</kbd>) triggers the snow globe. On the real clock this comes from the board's accelerometer.

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

`ctx` carries the time of day, the 12/24-hour setting, and events like `shake`. The engine (`simulator/engine.js`) uses only integer colours and a deterministic xorshift RNG, so themes port directly to C++ on the ESP32.

```
simulator/
  index.html       simulator page
  main.js          render loop and controls (browser only)
  engine.js        framebuffer, colour helpers, RNG, clock font
  themes/
    winter.js      winter cabin theme
```

## Roadmap

- [x] Browser simulator
- [x] Winter cabin theme: fireplace, tree, snow, sleeping cat, day/night sky
- [ ] Firmware for the Matrix Portal S3 (Arduino + Adafruit Protomatter)
- [ ] WiFi time sync (NTP)
- [ ] Theme switching with the board buttons; snow globe with the accelerometer
- [ ] More themes: autumn, spooky, spring, starry night
- [ ] Auto-switch themes by date
- [ ] Phone control page: theme, brightness, night dimming
- [ ] Wiring photos and a printable frame
- [ ] Simulator hosted on GitHub Pages
