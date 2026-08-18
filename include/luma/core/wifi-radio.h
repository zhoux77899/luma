#pragma once

#include "luma/core/network-types.h"

#include <cstddef>

namespace luma {

class WifiRadio {
public:
    virtual ~WifiRadio() = default;

    virtual void begin() {}
    virtual void update() {}
    virtual void startScan() = 0;
    virtual bool scanComplete() const = 0;
    virtual int scanCount() const = 0;
    virtual bool scanAt(int index, WifiScanHit& out) const = 0;
    virtual void connect(const char* ssid, const char* password) = 0;
    virtual void disconnect() = 0;
    virtual NetworkState radioState() const = 0;
    virtual const char* connectedSsid() const = 0;
    virtual int8_t rssi() const = 0;
    virtual void stationIp(char* out, size_t n) const = 0;
};

}  // namespace luma
