#include "cardputer-wifi-radio.h"

#include <WiFi.h>
#include <cstdio>
#include <cstring>

namespace luma {

void CardputerWifiRadio::begin() {
    WiFi.mode(WIFI_STA);
    WiFi.setAutoReconnect(false);
    state_ = NetworkState::Disconnected;
}

void CardputerWifiRadio::update() {
    if (scanning_) {
        const int16_t found = WiFi.scanComplete();
        if (found >= 0) {
            scan_count_ = found;
            scanning_ = false;
            scan_done_ = true;
        }
    }

    if (state_ == NetworkState::Connecting) {
        const wl_status_t status = WiFi.status();
        if (status == WL_CONNECTED) {
            state_ = NetworkState::Connected;
            std::snprintf(ssid_, sizeof(ssid_), "%s", WiFi.SSID().c_str());
        } else if (status == WL_CONNECT_FAILED || status == WL_NO_SSID_AVAIL) {
            state_ = NetworkState::Failed;
        }
        return;
    }

    if (state_ == NetworkState::Connected && WiFi.status() != WL_CONNECTED) {
        state_ = NetworkState::Disconnected;
        ssid_[0] = '\0';
    }
}

void CardputerWifiRadio::startScan() {
    WiFi.scanDelete();
    WiFi.scanNetworks(true, false);
    scanning_ = true;
    scan_done_ = false;
    scan_count_ = 0;
}

bool CardputerWifiRadio::scanComplete() const { return scan_done_; }

int CardputerWifiRadio::scanCount() const { return scan_count_; }

bool CardputerWifiRadio::scanAt(int index, WifiScanHit& out) const {
    if (index < 0 || index >= scan_count_) {
        return false;
    }
    std::snprintf(out.ssid, sizeof(out.ssid), "%s", WiFi.SSID(index).c_str());
    out.encrypted = WiFi.encryptionType(index) != WIFI_AUTH_OPEN;
    out.rssi = static_cast<int8_t>(WiFi.RSSI(index));
    return true;
}

void CardputerWifiRadio::connect(const char* ssid, const char* password) {
    if (ssid == nullptr || ssid[0] == '\0') {
        return;
    }
    std::snprintf(ssid_, sizeof(ssid_), "%s", ssid);
    state_ = NetworkState::Connecting;
    if (password == nullptr || password[0] == '\0') {
        WiFi.begin(ssid);
        return;
    }
    WiFi.begin(ssid, password);
}

void CardputerWifiRadio::disconnect() {
    WiFi.disconnect(false);
    state_ = NetworkState::Disconnected;
    ssid_[0] = '\0';
}

NetworkState CardputerWifiRadio::radioState() const { return state_; }

const char* CardputerWifiRadio::connectedSsid() const { return ssid_; }

int8_t CardputerWifiRadio::rssi() const {
    if (state_ != NetworkState::Connected) {
        return -127;
    }
    return static_cast<int8_t>(WiFi.RSSI());
}

void CardputerWifiRadio::stationIp(char* out, size_t n) const {
    if (out == nullptr || n == 0) {
        return;
    }
    out[0] = '\0';
    if (state_ != NetworkState::Connected) {
        return;
    }
    const IPAddress ip = WiFi.localIP();
    std::snprintf(out, n, "%u.%u.%u.%u", static_cast<unsigned>(ip[0]),
                  static_cast<unsigned>(ip[1]), static_cast<unsigned>(ip[2]),
                  static_cast<unsigned>(ip[3]));
}

}  // namespace luma
