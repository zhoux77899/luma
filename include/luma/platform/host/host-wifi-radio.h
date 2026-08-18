#pragma once

#include "luma/core/wifi-radio.h"

namespace luma {

class HostWifiRadio : public WifiRadio {
public:
    void begin() override;
    void update() override;
    void startScan() override;
    bool scanComplete() const override;
    int scanCount() const override;
    bool scanAt(int index, WifiScanHit& out) const override;
    void connect(const char* ssid, const char* password) override;
    void disconnect() override;
    NetworkState radioState() const override;
    const char* connectedSsid() const override;
    int8_t rssi() const override;

private:
    static constexpr int kCannedCount = 2;
    static constexpr int kConnectTicks = 3;

    NetworkState state_ = NetworkState::Disconnected;
    bool scan_done_ = false;
    char ssid_[33] = {};
    int8_t rssi_ = -55;
    int connect_ticks_ = 0;
};

}  // namespace luma
