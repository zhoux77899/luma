#pragma once

#include "luma/core/clock.h"

#include <cstddef>
#include <cstdint>

namespace luma {

constexpr int kTimeZoneCount = 12;
constexpr int kTimeZoneSectionCount = 5;
constexpr const char* kDefaultTimeZoneId = "UTC";

int timeZoneCount();
const char* timeZoneIdAt(int index);
bool isTimeZoneId(const char* id);
const char* canonicalTimeZoneId(const char* id);
CivilTime civilTimeAt(int64_t unix_utc, const char* zone_id);
const char* matchTimeZoneId(int32_t offset_east_sec, int64_t unix_utc);

int timeZoneSectionCount();
const char* timeZoneSectionLabelAt(int section);
int timeZoneCountInSection(int section);
const char* timeZoneIdInSection(int section, int index);
int timeZoneSectionOf(const char* id);
void formatTimeZoneUtcLabel(const char* id, char* buf, size_t n);

}  // namespace luma
