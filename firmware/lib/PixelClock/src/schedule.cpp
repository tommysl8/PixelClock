// Port of simulator/schedule.js. Keep the two in sync.
#include "schedule.h"

#include "themes.h"

namespace pc {

// Days since 1970-01-01 (Howard Hinnant's days_from_civil).
long dayNumber(int y, int m, int d) {
  y -= m <= 2;
  const long era = (y >= 0 ? y : y - 399) / 400;
  const unsigned yoe = unsigned(y - era * 400);
  const unsigned doy = (153 * (m + (m > 2 ? -3 : 9)) + 2) / 5 + d - 1;
  const unsigned doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
  return era * 146097 + long(doe) - 719468;
}

MonthDay easter(int y) {
  int a = y % 19, b = y / 100, c = y % 100, d = b / 4, e = b % 4;
  int f = (b + 8) / 25, g = (b - f + 1) / 3;
  int h = (19 * a + b - d - g + 15) % 30;
  int i = c / 4, k = c % 4;
  int l = (32 + 2 * e + 2 * i - h - k) % 7;
  int m = (a + 11 * h + 22 * l) / 451;
  return {(h + l - 7 * m + 114) / 31, ((h + l - 7 * m + 114) % 31) + 1};
}

int nthWeekday(int y, int month, int weekday, int n) {
  int first = int(((dayNumber(y, month, 1) % 7) + 7 + 4) % 7);  // 1970-01-01 was a Thursday
  return 1 + ((weekday - first + 7) % 7) + (n - 1) * 7;
}

namespace {

struct LunarNewYear {
  int year, month, day;
};
const LunarNewYear LUNAR_NEW_YEAR[] = {
    {2025, 1, 29}, {2026, 2, 17}, {2027, 2, 6},  {2028, 1, 26}, {2029, 2, 13}, {2030, 2, 3},
    {2031, 1, 23}, {2032, 2, 11}, {2033, 1, 31}, {2034, 2, 19}, {2035, 2, 8},
};

bool holidayDate(const char* id, int y, MonthDay& out) {
  auto is = [&](const char* s) {
    const char* a = id;
    while (*a && *a == *s) a++, s++;
    return *a == *s;
  };
  if (is("newyear")) out = {12, 31};
  else if (is("lunarnewyear")) {
    for (const auto& l : LUNAR_NEW_YEAR)
      if (l.year == y) {
        out = {l.month, l.day};
        return true;
      }
    return false;
  } else if (is("valentines")) out = {2, 14};
  else if (is("stpatricks")) out = {3, 17};
  else if (is("easter")) out = easter(y);
  else if (is("july4")) out = {7, 4};
  else if (is("halloween")) out = {10, 31};
  else if (is("thanksgiving")) out = {11, nthWeekday(y, 11, 4, 4)};
  else if (is("christmas")) out = {12, 25};
  else return false;
  return true;
}

struct Holiday {
  const char* id;
  int before;
  int after;
};
// Same order and windows as HOLIDAYS in schedule.js.
const Holiday HOLIDAYS[] = {
    {"newyear", 0, 1},    {"lunarnewyear", 1, 3}, {"valentines", 3, 0},
    {"stpatricks", 3, 0}, {"easter", 3, 0},       {"july4", 3, 0},
    {"halloween", 16, 0}, {"thanksgiving", 3, 0}, {"christmas", 24, 1},
};

}  // namespace

const char* holidayFor(int y, int m, int d) {
  long today = dayNumber(y, m, d);
  for (const Holiday& h : HOLIDAYS) {
    for (int yy = y - 1; yy <= y + 1; yy++) {
      MonthDay date;
      if (!holidayDate(h.id, yy, date)) continue;
      long diff = today - dayNumber(yy, date.month, date.day);
      if (diff >= -h.before && diff <= h.after) return h.id;
    }
  }
  return nullptr;
}

const char* seasonFor(int m) {
  if (m == 12 || m <= 2) return "winter";
  if (m <= 5) return "spring";
  if (m <= 8) return "summer";
  return "autumn";
}

const char* themeIdForDate(int y, int m, int d) {
  const char* h = holidayFor(y, m, d);
  if (h && themeById(h)) return h;
  return seasonFor(m);
}

}  // namespace pc
