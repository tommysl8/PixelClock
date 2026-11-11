// PC harness for the C++ engine: runs a theme exactly like tools/render.js and
// writes the raw framebuffers, so tools/compare.js can diff C++ against JavaScript.
//
//   pc_render <themeId|auto> [--hour H] [--date Y-M-D] [--seconds S] [--frames N]
//             [--gap S] [--seed N] [--shake] --raw out.bin
//
// out.bin holds N frames of 64*64 little-endian uint32 0xRRGGBB pixels.
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

#include "pixelclock.h"
#include "schedule.h"
#include "themes.h"

using namespace pc;

int main(int argc, char** argv) {
  std::string target = argc > 1 && argv[1][0] != '-' ? argv[1] : "christmas";
  double hour = 12, warmup = 3, gap = 0.5;
  int frames = 1, year = 2026, month = 11, day = 11;
  unsigned seed = 12345;
  bool shake = false, timing = false;
  std::string raw;
  for (int i = 1; i < argc; i++) {
    std::string a = argv[i];
    auto val = [&]() { return i + 1 < argc ? std::string(argv[++i]) : std::string(); };
    if (a == "--hour") hour = std::atof(val().c_str());
    else if (a == "--seconds") warmup = std::atof(val().c_str());
    else if (a == "--gap") gap = std::atof(val().c_str());
    else if (a == "--frames") frames = std::atoi(val().c_str());
    else if (a == "--seed") seed = unsigned(std::strtoul(val().c_str(), nullptr, 10));
    else if (a == "--date") std::sscanf(val().c_str(), "%d-%d-%d", &year, &month, &day);
    else if (a == "--shake") shake = true;
    else if (a == "--raw") raw = val();
    else if (a == "--timing") timing = true;
  }

  const char* id = target == "auto" ? themeIdForDate(year, month, day) : target.c_str();
  const ThemeInfo* info = themeById(id);
  if (!info) {
    std::fprintf(stderr, "No theme \"%s\"\n", target.c_str());
    return 1;
  }

  const real STEP = real(1) / 30;
  auto ctxAt = [&](double extraSeconds) {
    Ctx c;
    double hf = std::fmod(hour + extraSeconds / 3600, 24.0);
    c.hours = int(std::floor(hf));
    c.minutes = int(std::floor((hf - c.hours) * 60));
    c.seconds = 0;
    c.ms = 0;
    c.hourFloat = real(hf);
    c.hour12 = true;
    c.shake = false;
    c.year = year;
    c.month = month;
    c.day = day;
    return c;
  };
  auto steps = [&](double s) { return int(std::floor(s / STEP + 0.5)); };

  Rng rng(seed);
  std::unique_ptr<Theme> theme = info->create(rng);
  Canvas fb;
  std::vector<uint32_t> out;
  double t = 0;
  double maxMs = 0, totalMs = 0;
  int timed = 0;
  auto timedStep = [&](const Ctx& c, bool drawToo) {
    auto t0 = std::chrono::steady_clock::now();
    theme->update(STEP, c);
    if (drawToo) theme->draw(fb, c);
    double ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count();
    maxMs = ms > maxMs ? ms : maxMs;
    totalMs += ms;
    timed++;
  };

  const int warm = steps(warmup);
  for (int i = 0; i < warm; i++, t += STEP) {
    Ctx c = ctxAt(t);
    c.shake = shake && i == warm - steps(1);
    if (timing) timedStep(c, true);
    else theme->update(STEP, c);
  }
  for (int f = 0; f < frames; f++) {
    if (f > 0)
      for (int i = 0; i < steps(gap); i++, t += STEP) theme->update(STEP, ctxAt(t));
    theme->draw(fb, ctxAt(t));
    out.insert(out.end(), fb.px, fb.px + W * H);
  }

  if (timing) std::printf("%s: avg %.3f ms, max %.3f ms per update+draw\n", info->id, totalMs / timed, maxMs);
  if (!raw.empty()) {
    FILE* fp = std::fopen(raw.c_str(), "wb");
    if (!fp) return 2;
    for (uint32_t v : out) {
      unsigned char b[4] = {(unsigned char)(v & 255), (unsigned char)((v >> 8) & 255),
                            (unsigned char)((v >> 16) & 255), (unsigned char)((v >> 24) & 255)};
      std::fwrite(b, 1, 4, fp);
    }
    std::fclose(fp);
  }
  return 0;
}
