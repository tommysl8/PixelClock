// Theme interface and registry.
#pragma once

#include <memory>

#include "pixelclock.h"

namespace pc {

// A running theme. Port of the { update, draw } object that create(rng) returns in JS.
class Theme {
 public:
  virtual ~Theme() = default;
  virtual void update(real dt, const Ctx& ctx) = 0;
  virtual void draw(Canvas& cv, const Ctx& ctx) = 0;
};

enum class Kind { Season, Holiday };

struct ThemeInfo {
  const char* id;
  const char* name;
  Kind kind;
  std::unique_ptr<Theme> (*create)(Rng& rng);  // rng must outlive the theme
};

extern const ThemeInfo THEMES[];
extern const int THEME_COUNT;
const ThemeInfo* themeById(const char* id);

// One factory per theme file in themes/.
std::unique_ptr<Theme> createAutumn(Rng& rng);
std::unique_ptr<Theme> createChristmas(Rng& rng);
std::unique_ptr<Theme> createEaster(Rng& rng);
std::unique_ptr<Theme> createHalloween(Rng& rng);
std::unique_ptr<Theme> createJuly4(Rng& rng);
std::unique_ptr<Theme> createLunarNewYear(Rng& rng);
std::unique_ptr<Theme> createNewYear(Rng& rng);
std::unique_ptr<Theme> createSpring(Rng& rng);
std::unique_ptr<Theme> createStPatricks(Rng& rng);
std::unique_ptr<Theme> createSummer(Rng& rng);
std::unique_ptr<Theme> createThanksgiving(Rng& rng);
std::unique_ptr<Theme> createValentines(Rng& rng);
std::unique_ptr<Theme> createWinter(Rng& rng);

}  // namespace pc
