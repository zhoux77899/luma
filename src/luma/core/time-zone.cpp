#include "luma/core/time-zone.h"

#include <cstring>

namespace luma {
namespace {

struct ZoneRule {
    const char* id;
    int32_t std_east_sec;
    int32_t dst_east_sec;
    uint8_t start_month;
    uint8_t start_week;
    uint8_t start_dow;
    uint8_t start_hour;
    uint8_t end_month;
    uint8_t end_week;
    uint8_t end_dow;
    uint8_t end_hour;
};

constexpr ZoneRule kZones[kTimeZoneCount] = {
    {"UTC", 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {"Asia/Shanghai", 8 * 3600, 8 * 3600, 0, 0, 0, 0, 0, 0, 0, 0},
    {"Asia/Tokyo", 9 * 3600, 9 * 3600, 0, 0, 0, 0, 0, 0, 0, 0},
    {"Asia/Singapore", 8 * 3600, 8 * 3600, 0, 0, 0, 0, 0, 0, 0, 0},
    {"Asia/Kolkata", 5 * 3600 + 1800, 5 * 3600 + 1800, 0, 0, 0, 0, 0, 0, 0, 0},
    {"Australia/Sydney", 10 * 3600, 11 * 3600, 10, 1, 0, 2, 4, 1, 0, 3},
    {"Europe/London", 0, 3600, 3, 5, 0, 1, 10, 5, 0, 2},
    {"Europe/Berlin", 3600, 2 * 3600, 3, 5, 0, 2, 10, 5, 0, 3},
    {"America/New_York", -5 * 3600, -4 * 3600, 3, 2, 0, 2, 11, 1, 0, 2},
    {"America/Chicago", -6 * 3600, -5 * 3600, 3, 2, 0, 2, 11, 1, 0, 2},
    {"America/Denver", -7 * 3600, -6 * 3600, 3, 2, 0, 2, 11, 1, 0, 2},
    {"America/Los_Angeles", -8 * 3600, -7 * 3600, 3, 2, 0, 2, 11, 1, 0, 2},
};

bool isLeap(int year) {
    return (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
}

int daysInMonth(int year, int month) {
    static const int kDays[] = {0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    if (month == 2 && isLeap(year)) {
        return 29;
    }
    return kDays[month];
}

int weekdayYmd(int year, int month, int day) {
    int y = year;
    int m = month;
    if (m < 3) {
        m += 12;
        --y;
    }
    const int k = y % 100;
    const int j = y / 100;
    const int h = (day + (13 * (m + 1)) / 5 + k + k / 4 + j / 4 + 5 * j) % 7;
    return (h + 6) % 7;
}

int nthWeekdayDay(int year, int month, int week, int dow) {
    if (week >= 5) {
        const int last = daysInMonth(year, month);
        const int last_dow = weekdayYmd(year, month, last);
        int delta = (last_dow - dow + 7) % 7;
        return last - delta;
    }
    const int first_dow = weekdayYmd(year, month, 1);
    int day = 1 + (dow - first_dow + 7) % 7 + (week - 1) * 7;
    return day;
}

int64_t unixFromUtc(int year, int month, int day, int hour, int minute, int second) {
    int64_t days = 0;
    for (int y = 1970; y < year; ++y) {
        days += isLeap(y) ? 366 : 365;
    }
    for (int m = 1; m < month; ++m) {
        days += daysInMonth(year, m);
    }
    days += day - 1;
    return days * 86400 + hour * 3600 + minute * 60 + second;
}

void utcParts(int64_t unix_utc, int& year, int& month, int& day, int& hour, int& minute) {
    int64_t remaining = unix_utc;
    year = 1970;
    while (true) {
        const int days = isLeap(year) ? 366 : 365;
        const int64_t seconds = static_cast<int64_t>(days) * 86400;
        if (remaining < seconds) {
            break;
        }
        remaining -= seconds;
        ++year;
    }
    month = 1;
    while (true) {
        const int days = daysInMonth(year, month);
        const int64_t seconds = static_cast<int64_t>(days) * 86400;
        if (remaining < seconds) {
            break;
        }
        remaining -= seconds;
        ++month;
    }
    day = static_cast<int>(remaining / 86400) + 1;
    remaining %= 86400;
    hour = static_cast<int>(remaining / 3600);
    remaining %= 3600;
    minute = static_cast<int>(remaining / 60);
}

const ZoneRule* findZone(const char* id) {
    const char* canonical = canonicalTimeZoneId(id);
    for (int i = 0; i < kTimeZoneCount; ++i) {
        if (std::strcmp(kZones[i].id, canonical) == 0) {
            return &kZones[i];
        }
    }
    return &kZones[0];
}

int64_t transitionUnix(const ZoneRule& zone, int year, bool start) {
    const uint8_t month = start ? zone.start_month : zone.end_month;
    const uint8_t week = start ? zone.start_week : zone.end_week;
    const uint8_t dow = start ? zone.start_dow : zone.end_dow;
    const uint8_t hour = start ? zone.start_hour : zone.end_hour;
    const int day = nthWeekdayDay(year, month, week, dow);
    const int32_t offset = start ? zone.std_east_sec : zone.dst_east_sec;
    return unixFromUtc(year, month, day, hour, 0, 0) - offset;
}

int32_t offsetAt(const ZoneRule& zone, int64_t unix_utc) {
    if (zone.std_east_sec == zone.dst_east_sec || zone.start_month == 0) {
        return zone.std_east_sec;
    }
    int year = 1970;
    int month = 1;
    int day = 1;
    int hour = 0;
    int minute = 0;
    utcParts(unix_utc, year, month, day, hour, minute);
    const int64_t start = transitionUnix(zone, year, true);
    const int64_t end = transitionUnix(zone, year, false);
    if (start < end) {
        return (unix_utc >= start && unix_utc < end) ? zone.dst_east_sec : zone.std_east_sec;
    }
    return (unix_utc >= start || unix_utc < end) ? zone.dst_east_sec : zone.std_east_sec;
}

}  // namespace

int timeZoneCount() { return kTimeZoneCount; }

const char* timeZoneIdAt(int index) {
    if (index < 0 || index >= kTimeZoneCount) {
        return kDefaultTimeZoneId;
    }
    return kZones[index].id;
}

bool isTimeZoneId(const char* id) {
    if (id == nullptr) {
        return false;
    }
    for (int i = 0; i < kTimeZoneCount; ++i) {
        if (std::strcmp(kZones[i].id, id) == 0) {
            return true;
        }
    }
    return false;
}

const char* canonicalTimeZoneId(const char* id) {
    return isTimeZoneId(id) ? id : kDefaultTimeZoneId;
}

CivilTime civilTimeAt(int64_t unix_utc, const char* zone_id) {
    const ZoneRule* zone = findZone(zone_id);
    const int64_t local = unix_utc + offsetAt(*zone, unix_utc);
    int year = 1970;
    int month = 1;
    int day = 1;
    int hour = 0;
    int minute = 0;
    utcParts(local, year, month, day, hour, minute);
    CivilTime time;
    time.hour = static_cast<uint8_t>(hour);
    time.minute = static_cast<uint8_t>(minute);
    time.valid = true;
    return time;
}

const char* matchTimeZoneId(int32_t offset_east_sec, int64_t unix_utc) {
    for (int i = 0; i < kTimeZoneCount; ++i) {
        if (offsetAt(kZones[i], unix_utc) == offset_east_sec) {
            return kZones[i].id;
        }
    }
    return kDefaultTimeZoneId;
}

}  // namespace luma
