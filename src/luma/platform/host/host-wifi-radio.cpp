#include "luma/platform/host/host-wifi-radio.h"

#include <cstdio>
#include <cstring>

namespace luma {
namespace {

const WifiScanHit kCanned[] = {
    {"Luma-Lab", true, -48},
    {"Open-Cafe", false, -72},
};

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

}  // namespace luma
