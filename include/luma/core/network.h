#pragma once

#include "luma/core/network-types.h"

#include <cstdint>

namespace luma {

class Clock;
class Diagnostics;
class Storage;
class WifiRadio;

class Network {
public:
    static constexpr int kMaxProfiles = 5;
    static constexpr uint8_t kSchema = 1;
    static constexpr uint32_t kManualTimeoutMs = 15000;
    static constexpr uint32_t kBackgroundRetryMs = 5000;
    static constexpr uint32_t kBackgroundIdleMs = 30000;
    static constexpr int kBackgroundAttempts = 3;
    static constexpr const char* kPrefKey = "wifi";

    void attach(WifiRadio& radio, Storage& storage, Diagnostics& diagnostics, Clock& clock);
    void load();
    void begin();
    void update();

    NetworkState state() const;
    SignalStrength signalStrength() const;
    const char* connectedSsid() const;
    bool takeConnectedEdge();

    void startScan();
    bool scanInProgress() const;
    int publicScanCount() const;
    bool publicScanAt(int index, WifiScanHit& out) const;

    void connect(const char* ssid, const char* password);
    void connectProfile(int index);
    void deleteProfile(int index);
    int profileCount() const;
    const char* profileSsid(int index) const;
    bool profileHasPassword(int index) const;

private:
    enum class Mode : uint8_t { Idle, Manual, Background };

    struct Profile {
        char ssid[33] = {};
        char password[64] = {};
    };

    struct Blob {
        uint8_t schema = kSchema;
        uint8_t count = 0;
        Profile profiles[kMaxProfiles] = {};
    };

    void emit(const char* message);
    void saveProfiles();
    void rememberSuccess(const char* ssid, const char* password);
    int findProfile(const char* ssid) const;
    void promote(int index);
    SignalStrength strengthFromRssi(int8_t rssi) const;
    void startBackgroundRound();
    void tryNextBackgroundProfile();
    bool scanContains(const char* ssid) const;
    bool isSavedSsid(const char* ssid) const;
    void copySsid(char* dst, const char* src) const;
    void copyPassword(char* dst, const char* src) const;

    WifiRadio* radio_ = nullptr;
    Storage* storage_ = nullptr;
    Diagnostics* diagnostics_ = nullptr;
    Clock* clock_ = nullptr;
    Blob blob_{};
    NetworkState state_ = NetworkState::Disconnected;
    Mode mode_ = Mode::Idle;
    bool connected_edge_ = false;
    bool scan_pending_ = false;
    char pending_ssid_[33] = {};
    char pending_password_[64] = {};
    uint32_t action_started_ms_ = 0;
    uint32_t next_background_ms_ = 0;
    int background_attempts_ = 0;
    int background_profile_ = 0;
    bool waiting_for_scan_ = false;
};

}  // namespace luma
