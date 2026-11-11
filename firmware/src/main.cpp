// PixelClock firmware for the Adafruit Matrix Portal S3 driving a 64x64 HUB75 panel.
//
// The themes, schedule and drawing engine live in lib/PixelClock (shared with the
// PC test harness). This file is the hardware side: the LED panel, WiFi + time,
// the two buttons, the shake sensor, brightness and the power limit.
//
// Buttons:
//   UP / DOWN (short press)     next / previous theme (first entry is Auto by date)
//   UP (hold 1 s)               cycle brightness
//   UP + DOWN (hold 5 s)        forget WiFi and restart into setup mode
//
// First boot: the clock opens a WiFi network called "PixelClock-Setup". Join it
// from your phone, pick your home WiFi and (optionally) your time zone, and the
// clock sets itself from the internet.
#include <Adafruit_LIS3DH.h>
#include <Adafruit_Protomatter.h>
#include <Arduino.h>
#include <Preferences.h>
#include <WiFi.h>
#include <WiFiManager.h>
#include <Wire.h>
#include <esp_random.h>
#include <time.h>

#include "pixelclock.h"
#include "schedule.h"
#include "themes.h"

using namespace pc;

// ---------- settings ----------
static const char* DEFAULT_TZ = "CST6CDT,M3.2.0,M11.1.0";  // US Central; the README lists others
static const char* NTP_1 = "pool.ntp.org";
static const char* NTP_2 = "time.nist.gov";
static const char* AP_NAME = "PixelClock-Setup";
static const float GAMMA = 2.0f;        // LEDs look washed out without gamma correction
// Power limit: average LED duty. Adafruit rates a 64x64 panel at up to 4A at full white,
// so 40% keeps the panel near 1.6A and the whole clock well under the 5.1V 3A supply.
static const float MAX_DUTY = 0.40f;
static const uint8_t BRIGHTNESS_LEVELS[] = {255, 160, 90, 40};
static const int NIGHT_START = 23 * 60;  // dim between 11 pm ...
static const int NIGHT_END = 6 * 60 + 30;  // ... and 6:30 am
static const float NIGHT_DIM = 0.4f;
static const float SHAKE_THRESHOLD = 7.0f;  // m/s^2 away from 1 g
constexpr int BIT_DEPTH = 6;
constexpr real STEP = real(1) / 30;

// ---------- Matrix Portal S3 wiring (from the Adafruit Protomatter examples) ----------
uint8_t rgbPins[] = {42, 41, 40, 38, 39, 37};
uint8_t addrPins[] = {45, 36, 48, 35, 21};
uint8_t clockPin = 2;
uint8_t latchPin = 47;
uint8_t oePin = 14;
Adafruit_Protomatter matrix(64, BIT_DEPTH, 1, rgbPins, 5, addrPins, clockPin, latchPin, oePin, true);

#ifdef PIN_BUTTON_UP  // defined by the Matrix Portal S3 board variant
static const int PIN_UP = PIN_BUTTON_UP;
static const int PIN_DOWN = PIN_BUTTON_DOWN;
#else
static const int PIN_UP = 6;
static const int PIN_DOWN = 7;
#endif

Adafruit_LIS3DH accel;
bool hasAccel = false;
Preferences prefs;
WiFiManager wm;
char tz[64];
WiFiManagerParameter tzParam("tz", "Time zone (POSIX TZ string)", DEFAULT_TZ, 63);

// ---------- state ----------
Rng rng(1);
Canvas fb;
std::unique_ptr<Theme> theme;
const char* activeId = nullptr;
int selection = 0;  // 0 = auto, 1..THEME_COUNT = THEMES[selection - 1]
int brightnessIndex = 0;
uint8_t lut[256];
float lutLevel = -1;
real acc = 0;
uint32_t lastMs = 0;
int lastSecond = -1;
uint32_t secondStartMs = 0;
uint32_t overlayUntil = 0;
bool shakePending = false;
uint32_t lastShakeMs = 0;

// ---------- LED output ----------
static void buildLut(float level) {
  for (int i = 0; i < 256; i++) lut[i] = uint8_t(powf(i / 255.0f, GAMMA) * 255.0f * level + 0.5f);
  lutLevel = level;
}

// Gamma-correct, apply brightness and the power limit, and hand the frame to the panel.
static void present(float level) {
  if (level != lutLevel) buildLut(level);
  static uint8_t out[W * H * 3];
  uint32_t sum = 0;
  for (int i = 0; i < W * H; i++) {
    uint32_t c = fb.px[i];
    uint8_t r = lut[(c >> 16) & 255], g = lut[(c >> 8) & 255], b = lut[c & 255];
    out[i * 3] = r;
    out[i * 3 + 1] = g;
    out[i * 3 + 2] = b;
    sum += r + g + b;
  }
  const uint32_t budget = uint32_t(MAX_DUTY * W * H * 3 * 255);
  uint32_t scale = sum > budget ? uint32_t(256.0f * budget / sum) : 256;

  uint16_t* buf = matrix.getBuffer();
  for (int i = 0; i < W * H; i++) {
    uint32_t r = out[i * 3] * scale >> 8, g = out[i * 3 + 1] * scale >> 8, b = out[i * 3 + 2] * scale >> 8;
    buf[i] = uint16_t(((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3));
  }
  matrix.show();
}

// ---------- settings storage ----------
static void loadSettings() {
  prefs.begin("pixelclock", false);
  selection = prefs.getInt("selection", 0);
  if (selection < 0 || selection > THEME_COUNT) selection = 0;
  brightnessIndex = prefs.getInt("bright", 0);
  if (brightnessIndex < 0 || brightnessIndex >= int(sizeof(BRIGHTNESS_LEVELS))) brightnessIndex = 0;
  String saved = prefs.getString("tz", DEFAULT_TZ);
  strncpy(tz, saved.c_str(), sizeof(tz) - 1);
  tz[sizeof(tz) - 1] = 0;
}

static void saveParams() {
  const char* v = tzParam.getValue();
  if (v && *v) {
    strncpy(tz, v, sizeof(tz) - 1);
    tz[sizeof(tz) - 1] = 0;
    prefs.putString("tz", tz);
    configTzTime(tz, NTP_1, NTP_2);
    Serial.printf("Time zone set to %s\n", tz);
  }
}

// ---------- time ----------
static bool timeValid() { return time(nullptr) > 1700000000; }

static Ctx makeCtx() {
  Ctx c;
  time_t now = time(nullptr);
  struct tm t;
  localtime_r(&now, &t);
  if (t.tm_sec != lastSecond) {
    lastSecond = t.tm_sec;
    secondStartMs = millis();
  }
  c.hours = t.tm_hour;
  c.minutes = t.tm_min;
  c.seconds = t.tm_sec;
  uint32_t intoSecond = millis() - secondStartMs;
  c.ms = int(intoSecond > 999 ? 999 : intoSecond);
  c.hourFloat = real(t.tm_hour) + real(t.tm_min) / 60 + real(t.tm_sec) / 3600;
  c.hour12 = true;
  c.year = t.tm_year + 1900;
  c.month = t.tm_mon + 1;
  c.day = t.tm_mday;
  return c;
}

// ---------- themes ----------
static void refreshTheme(const Ctx& ctx) {
  const char* id = selection == 0 ? themeIdForDate(ctx.year, ctx.month, ctx.day) : THEMES[selection - 1].id;
  const ThemeInfo* info = themeById(id);
  if (!info) info = &THEMES[0];
  if (info->id != activeId) {
    theme = info->create(rng);
    activeId = info->id;
    Serial.printf("Theme: %s%s\n", info->name, selection == 0 ? " (auto)" : "");
  }
}

static void selectTheme(int s) {
  selection = (s + THEME_COUNT + 1) % (THEME_COUNT + 1);
  prefs.putInt("selection", selection);
  activeId = nullptr;  // force a fresh theme even if the id is unchanged
}

// ---------- buttons ----------
struct Button {
  int pin;
  bool down = false;
  uint32_t since = 0;
  bool longFired = false;
};
Button up{PIN_UP}, dn{PIN_DOWN};
uint32_t bothSince = 0;

static void pollButton(Button& b, void (*shortPress)(), void (*longPress)()) {
  bool pressed = digitalRead(b.pin) == LOW;
  uint32_t now = millis();
  if (pressed && !b.down) {
    b.down = true;
    b.since = now;
    b.longFired = false;
  } else if (pressed && b.down && !b.longFired && longPress && now - b.since > 1000) {
    b.longFired = true;
    longPress();
  } else if (!pressed && b.down) {
    b.down = false;
    if (!b.longFired && now - b.since > 30 && shortPress) shortPress();
  }
}

static void nextTheme() { selectTheme(selection + 1); }
static void prevTheme() { selectTheme(selection - 1); }
static void cycleBrightness() {
  brightnessIndex = (brightnessIndex + 1) % int(sizeof(BRIGHTNESS_LEVELS));
  prefs.putInt("bright", brightnessIndex);
  overlayUntil = millis() + 1200;
}

static void handleButtons() {
  if (digitalRead(PIN_UP) == LOW && digitalRead(PIN_DOWN) == LOW) {
    if (!bothSince) bothSince = millis();
    if (millis() - bothSince > 5000) {
      Serial.println("Resetting WiFi settings");
      wm.resetSettings();
      delay(200);
      ESP.restart();
    }
    up.longFired = dn.longFired = true;  // don't treat the combo as single presses
    return;
  }
  bothSince = 0;
  pollButton(up, nextTheme, cycleBrightness);
  pollButton(dn, prevTheme, nullptr);
}

// ---------- shake ----------
static void pollShake() {
  if (!hasAccel) return;
  sensors_event_t e;
  accel.getEvent(&e);
  float m = sqrtf(e.acceleration.x * e.acceleration.x + e.acceleration.y * e.acceleration.y +
                  e.acceleration.z * e.acceleration.z);
  if (fabsf(m - 9.81f) > SHAKE_THRESHOLD && millis() - lastShakeMs > 1500) {
    lastShakeMs = millis();
    shakePending = true;
  }
}

// ---------- screens ----------
// Shown until the clock knows the time: a WiFi symbol that pulses while setup is open.
static void drawSetupScreen() {
  fb.clear(0x050814);
  bool portal = wm.getConfigPortalActive();
  real t = millis() / real(1000);
  for (int y = 16; y < 48; y++) {
    for (int x = 12; x < 52; x++) {
      real dx = x - real(31.5), dy = y - real(44);
      real d = std::sqrt(dx * dx + dy * dy);
      if (dy > 0 || std::fabs(dx) > -dy * real(1.2)) continue;  // upward-facing wedge
      int ring = int(d / 7);
      if (int(d) % 7 > 3 || ring > 3) continue;
      real pulse = portal ? real(0.4) + real(0.6) * ((int(t * 2) % 4) >= ring) : 1;
      fb.set(x, y, mix(0x050814, ring == 0 ? 0xffffff : 0x4fc3ff, pulse));
    }
  }
  // dots underneath: three while connecting, solid when the setup network is open
  for (int i = 0; i < 3; i++) {
    bool on = portal ? true : (int(t * 3) % 3) == i;
    fb.rect(25 + i * 6, 54, 26 + i * 6, 55, on ? 0xffd23f : 0x2a2a3a);
  }
}

static void drawBrightnessOverlay() {
  int n = int(sizeof(BRIGHTNESS_LEVELS));
  for (int i = 0; i < n; i++) {
    bool on = i < n - brightnessIndex;
    fb.rect(20 + i * 6, 58, 23 + i * 6, 61, on ? 0xffffff : 0x303030);
  }
}

static bool isNight(const Ctx& c) {
  int m = c.hours * 60 + c.minutes;
  return NIGHT_START > NIGHT_END ? (m >= NIGHT_START || m < NIGHT_END) : (m >= NIGHT_START && m < NIGHT_END);
}

// ---------- setup / loop ----------
void setup() {
  Serial.begin(115200);
  rng = Rng(esp_random() | 1);

  ProtomatterStatus status = matrix.begin();
  Serial.printf("Protomatter begin: %d\n", int(status));

  pinMode(PIN_UP, INPUT_PULLUP);
  pinMode(PIN_DOWN, INPUT_PULLUP);

  Wire.begin();
  hasAccel = accel.begin(0x19);
  if (hasAccel) accel.setRange(LIS3DH_RANGE_4_G);
  Serial.printf("Accelerometer: %s\n", hasAccel ? "found" : "not found");

  loadSettings();
  configTzTime(tz, NTP_1, NTP_2);

  WiFi.mode(WIFI_STA);
  tzParam.setValue(tz, 63);
  wm.addParameter(&tzParam);
  wm.setSaveParamsCallback(saveParams);
  wm.setConfigPortalBlocking(false);
  wm.setTitle("PixelClock");
  if (wm.autoConnect(AP_NAME)) Serial.println("WiFi connected");
  else Serial.printf("WiFi setup open: join \"%s\"\n", AP_NAME);

  drawSetupScreen();
  present(1);
  lastMs = millis();
}

void loop() {
  wm.process();
  handleButtons();
  pollShake();

  uint32_t now = millis();
  acc += (now - lastMs) / real(1000);
  lastMs = now;
  if (acc > real(0.25)) acc = real(0.25);
  if (acc < STEP) {
    delay(2);
    return;
  }

  if (!timeValid()) {
    acc = 0;
    drawSetupScreen();
    present(float(BRIGHTNESS_LEVELS[brightnessIndex]) / 255);
    return;
  }

  Ctx ctx = makeCtx();
  static int lastRefresh = -1;
  if (ctx.seconds != lastRefresh || !activeId) {
    lastRefresh = ctx.seconds;
    refreshTheme(ctx);  // auto mode follows the date across midnight
  }

  while (acc >= STEP) {
    acc -= STEP;
    Ctx c = ctx;
    c.shake = shakePending;
    shakePending = false;
    theme->update(STEP, c);
  }
  theme->draw(fb, ctx);
  if (millis() < overlayUntil) drawBrightnessOverlay();

  float level = float(BRIGHTNESS_LEVELS[brightnessIndex]) / 255;
  if (isNight(ctx)) level *= NIGHT_DIM;
  present(level);
}
