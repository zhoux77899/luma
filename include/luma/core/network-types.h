#pragma once

#include <cstdint>

namespace luma {

enum class NetworkState : uint8_t {
    Disconnected = 0,
    Connecting = 1,
    Connected = 2,
    Failed = 3,
    Unknown = 4
};

enum class SignalStrength : uint8_t {
    None = 0,
    Weakest = 1,
    Weak = 2,
    Mid = 3,
    Strong = 4
};

struct WifiScanHit {
    char ssid[33] = {};
    bool encrypted = false;
    int8_t rssi = 0;
};

}  // namespace luma
