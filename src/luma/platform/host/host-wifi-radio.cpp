#include "luma/platform/host/host-wifi-radio.h"

#include <cstdio>
#include <cstring>

namespace luma {
namespace {

const WifiScanHit kCanned[] = {
    {"Luma-Lab", true, -48},
    {"Luma-Lab", true, -70},
    {"Open-Cafe", false, -72},
    {"Office-5G", true, -55},
    {"Guest", false, -80},
    {"Printer", true, -62},
    {"IoT-Mesh", true, -75},
    {"Library", false, -68},
};

constexpr int kCannedCount = static_cast<int>(sizeof(kCanned) / sizeof(kCanned[0]));

}  // namespace

void HostWifiRadio::begin() { state_ = NetworkState::Disconnected; }

void HostWifiRadio::update() {
    if (state_ != NetworkState::Connecting) {
        return;
    }
    ++connect_ticks_;
    if (connect_ticks_ >= kConnectTicks) {
        state_ = NetworkState::Connected;
    }
}

void HostWifiRadio::startScan() { scan_done_ = true; }

bool HostWifiRadio::scanComplete() const { return scan_done_; }

int HostWifiRadio::scanCount() const { return scan_done_ ? kCannedCount : 0; }

bool HostWifiRadio::scanAt(int index, WifiScanHit& out) const {
    if (!scan_done_ || index < 0 || index >= kCannedCount) {
        return false;
    }
    out = kCanned[index];
    return true;
}

void HostWifiRadio::connect(const char* ssid, const char* password) {
    (void)password;
    if (ssid == nullptr || ssid[0] == '\0') {
        return;
    }
    std::snprintf(ssid_, sizeof(ssid_), "%s", ssid);
    state_ = NetworkState::Connecting;
    connect_ticks_ = 0;
    rssi_ = -55;
}

void HostWifiRadio::disconnect() {
    state_ = NetworkState::Disconnected;
    ssid_[0] = '\0';
}

NetworkState HostWifiRadio::radioState() const { return state_; }

const char* HostWifiRadio::connectedSsid() const { return ssid_; }

int8_t HostWifiRadio::rssi() const { return rssi_; }

void HostWifiRadio::stationIp(char* out, size_t n) const {
    if (out == nullptr || n == 0) {
        return;
    }
    if (state_ != NetworkState::Connected) {
        out[0] = '\0';
        return;
    }
    std::snprintf(out, n, "192.168.1.10");
}

}  // namespace luma
