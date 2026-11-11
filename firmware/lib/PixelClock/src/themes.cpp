#include "themes.h"

#include <cstring>

namespace pc {

// Seasons first (in calendar order), then holidays in calendar order.
const ThemeInfo THEMES[] = {
    {"winter", "Snowy hills", Kind::Season, createWinter},
    {"spring", "Spring meadow", Kind::Season, createSpring},
    {"summer", "Summer beach", Kind::Season, createSummer},
    {"autumn", "Autumn forest", Kind::Season, createAutumn},
    {"newyear", "New Year's Eve", Kind::Holiday, createNewYear},
    {"lunarnewyear", "Lunar New Year", Kind::Holiday, createLunarNewYear},
    {"valentines", "Valentine's Day", Kind::Holiday, createValentines},
    {"stpatricks", "St. Patrick's Day", Kind::Holiday, createStPatricks},
    {"easter", "Easter", Kind::Holiday, createEaster},
    {"july4", "Fourth of July", Kind::Holiday, createJuly4},
    {"halloween", "Haunted hill", Kind::Holiday, createHalloween},
    {"thanksgiving", "Harvest field", Kind::Holiday, createThanksgiving},
    {"christmas", "Christmas cabin", Kind::Holiday, createChristmas},
};
const int THEME_COUNT = int(sizeof(THEMES) / sizeof(THEMES[0]));

const ThemeInfo* themeById(const char* id) {
  if (!id) return nullptr;
  for (const ThemeInfo& t : THEMES)
    if (std::strcmp(t.id, id) == 0) return &t;
  return nullptr;
}

}  // namespace pc
