#pragma once

#include "luma/core/clock.h"

#include <cstdint>

namespace luma {

constexpr int kTimeZoneCount = 12;
constexpr const char* kDefaultTimeZoneId = "UTC";

int timeZoneCount();
const char* timeZoneIdAt(int index);
bool isTimeZoneId(const char* id);
const char* canonicalTimeZoneId(const char* id);
CivilTime civilTimeAt(int64_t unix_utc, const char* zone_id);
const char* matchTimeZoneId(int32_t offset_east_sec, int64_t unix_utc);

}  // namespace luma
