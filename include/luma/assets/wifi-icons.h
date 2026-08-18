#pragma once

#include <cstdint>

namespace luma {
namespace assets {

constexpr int kWifiIconSize = 16;
constexpr int kWifiListIconSize = 10;

extern const uint16_t kWifiDisconnected[kWifiIconSize];
extern const uint16_t kWifiArc1[kWifiIconSize];
extern const uint16_t kWifiArc2[kWifiIconSize];
extern const uint16_t kWifiArc3[kWifiIconSize];
extern const uint16_t kWifiDot[kWifiIconSize];

extern const uint16_t kWifiListDot[kWifiListIconSize];
extern const uint16_t kWifiListArc2[kWifiListIconSize];
extern const uint16_t kWifiListArc3[kWifiListIconSize];
extern const uint16_t kLockClosed[kWifiListIconSize];
extern const uint16_t kLockOpen[kWifiListIconSize];

}  // namespace assets
}  // namespace luma
