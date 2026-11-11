/*
 * Picks the theme for a calendar date: a holiday theme in the days around
 * each holiday, otherwise the theme for the season.
 *
 * Pure date math with no browser APIs, so it ports straight to the ESP32.
 * Months are 1-12 throughout.
 */
(function (PC) {
  'use strict';

  const DAY_MS = 86400000;
  const dayNumber = (y, m, d) => Math.floor(Date.UTC(y, m - 1, d) / DAY_MS);

  // Easter Sunday (Gregorian), anonymous algorithm.
  function easter(y) {
    const a = y % 19;
    const b = Math.floor(y / 100);
    const c = y % 100;
    const d = Math.floor(b / 4);
    const e = b % 4;
    const f = Math.floor((b + 8) / 25);
    const g = Math.floor((b - f + 1) / 3);
    const h = (19 * a + b - d - g + 15) % 30;
    const i = Math.floor(c / 4);
    const k = c % 4;
    const l = (32 + 2 * e + 2 * i - h - k) % 7;
    const m = Math.floor((a + 11 * h + 22 * l) / 451);
    const month = Math.floor((h + l - 7 * m + 114) / 31);
    const day = ((h + l - 7 * m + 114) % 31) + 1;
    return [month, day];
  }

  // nth weekday of a month (weekday 0 = Sunday), e.g. 4th Thursday of November.
  function nthWeekday(y, month, weekday, n) {
    const first = new Date(Date.UTC(y, month - 1, 1)).getUTCDay();
    return 1 + ((weekday - first + 7) % 7) + (n - 1) * 7;
  }

  // Lunar New Year follows the lunisolar calendar, so it's a lookup table.
  const LUNAR_NEW_YEAR = {
    2025: [1, 29],
    2026: [2, 17],
    2027: [2, 6],
    2028: [1, 26],
    2029: [2, 13],
    2030: [2, 3],
    2031: [1, 23],
    2032: [2, 11],
    2033: [1, 31],
    2034: [2, 19],
    2035: [2, 8],
  };

  // Each holiday: its date in year y, and how many days before/after to show it.
  const HOLIDAYS = [
    { id: 'newyear', date: () => [12, 31], before: 0, after: 1 },
    { id: 'lunarnewyear', date: (y) => LUNAR_NEW_YEAR[y] || null, before: 1, after: 3 },
    { id: 'valentines', date: () => [2, 14], before: 3, after: 0 },
    { id: 'stpatricks', date: () => [3, 17], before: 3, after: 0 },
    { id: 'easter', date: (y) => easter(y), before: 3, after: 0 },
    { id: 'july4', date: () => [7, 4], before: 3, after: 0 },
    { id: 'halloween', date: () => [10, 31], before: 16, after: 0 },
    { id: 'thanksgiving', date: (y) => [11, nthWeekday(y, 11, 4, 4)], before: 3, after: 0 },
    { id: 'christmas', date: () => [12, 25], before: 24, after: 1 },
  ];

  function holidayFor(y, m, d) {
    const today = dayNumber(y, m, d);
    for (const h of HOLIDAYS) {
      // check neighbouring years so windows can cross New Year
      for (const yy of [y - 1, y, y + 1]) {
        const date = h.date(yy);
        if (!date) continue;
        const diff = today - dayNumber(yy, date[0], date[1]);
        if (diff >= -h.before && diff <= h.after) return h.id;
      }
    }
    return null;
  }

  function seasonFor(m) {
    if (m === 12 || m <= 2) return 'winter';
    if (m <= 5) return 'spring';
    if (m <= 8) return 'summer';
    return 'autumn';
  }

  // Theme id for a date. Falls back to the season if a holiday theme isn't installed.
  function themeIdForDate(y, m, d) {
    const h = holidayFor(y, m, d);
    if (h && PC.themeById(h)) return h;
    return seasonFor(m);
  }

  Object.assign(PC, { easter, nthWeekday, HOLIDAYS, holidayFor, seasonFor, themeIdForDate });
})(globalThis.PixelClock);
