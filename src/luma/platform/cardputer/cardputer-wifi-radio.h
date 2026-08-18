#pragma once

#include "luma/core/wifi-radio.h"

#include <cstddef>

namespace luma {

class CardputerWifiRadio : public WifiRadio {
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
    void stationIp(char* out, size_t n) const override;

private:
    NetworkState state_ = NetworkState::Disconnected;
    bool scanning_ = false;
    bool scan_done_ = false;
    int scan_count_ = 0;
    char ssid_[33] = {};
};

}  // namespace luma
