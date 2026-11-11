// Which theme to show on a date. Port of simulator/schedule.js; months are 1-12.
#pragma once

namespace pc {

struct MonthDay {
  int month;
  int day;
};

long dayNumber(int y, int m, int d);  // days since 1970-01-01
MonthDay easter(int y);
int nthWeekday(int y, int month, int weekday, int n);  // weekday 0 = Sunday
const char* holidayFor(int y, int m, int d);             // theme id or nullptr
const char* seasonFor(int m);
const char* themeIdForDate(int y, int m, int d);

}  // namespace pc
