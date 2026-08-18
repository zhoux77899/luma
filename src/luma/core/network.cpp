#include "luma/core/network.h"

#include "luma/core/clock.h"
#include "luma/core/diagnostics.h"
#include "luma/core/storage.h"
#include "luma/core/wifi-radio.h"

#include <cstdio>
#include <cstring>

namespace luma {
namespace {

void clearText(char* dst, size_t size) {
    if (dst != nullptr && size > 0) {
        std::memset(dst, 0, size);
    }
}

}  // namespace

void Network::copySsid(char* dst, const char* src) const {
    clearText(dst, 33);
    if (src == nullptr) {
        return;
    }
    std::snprintf(dst, 33, "%s", src);
}

void Network::copyPassword(char* dst, const char* src) const {
    clearText(dst, 64);
    if (src == nullptr) {
        return;
    }
    std::snprintf(dst, 64, "%s", src);
}

void Network::attach(WifiRadio& radio, Storage& storage, Diagnostics& diagnostics, Clock& clock) {
    radio_ = &radio;
    storage_ = &storage;
    diagnostics_ = &diagnostics;
    clock_ = &clock;
}

void Network::emit(const char* message) {
    if (diagnostics_ != nullptr && message != nullptr) {
        diagnostics_->emit("NET", message);
    }
}

void Network::load() {
    if (storage_ == nullptr) {
        return;
    }
    Blob loaded{};
    if (!storage_->loadPref(kPrefKey, &loaded, sizeof(loaded)) || loaded.schema != kSchema ||
        loaded.count > kMaxProfiles) {
        blob_ = Blob{};
        return;
    }
    blob_ = loaded;
}

void Network::saveProfiles() {
    if (storage_ == nullptr) {
        return;
    }
    blob_.schema = kSchema;
    storage_->savePref(kPrefKey, &blob_, sizeof(blob_));
}

int Network::findProfile(const char* ssid) const {
    if (ssid == nullptr) {
        return -1;
    }
    for (int i = 0; i < blob_.count; ++i) {
        if (std::strcmp(blob_.profiles[i].ssid, ssid) == 0) {
            return i;
        }
    }
    return -1;
}

void Network::promote(int index) {
    if (index <= 0 || index >= blob_.count) {
        return;
    }
    const Profile selected = blob_.profiles[index];
    for (int i = index; i > 0; --i) {
        blob_.profiles[i] = blob_.profiles[i - 1];
    }
    blob_.profiles[0] = selected;
}

void Network::rememberSuccess(const char* ssid, const char* password) {
    const int existing = findProfile(ssid);
    if (existing >= 0) {
        copyPassword(blob_.profiles[existing].password, password);
        promote(existing);
        saveProfiles();
        return;
    }
    if (blob_.count == kMaxProfiles) {
        --blob_.count;
    }
    for (int i = blob_.count; i > 0; --i) {
        blob_.profiles[i] = blob_.profiles[i - 1];
    }
    copySsid(blob_.profiles[0].ssid, ssid);
    copyPassword(blob_.profiles[0].password, password);
    ++blob_.count;
    saveProfiles();
}

void Network::begin() {
    if (radio_ != nullptr) {
        radio_->begin();
    }
    startBackgroundRound();
}

bool Network::scanContains(const char* ssid) const {
    if (radio_ == nullptr || ssid == nullptr) {
        return false;
    }
    WifiScanHit hit;
    for (int i = 0; i < radio_->scanCount(); ++i) {
        if (radio_->scanAt(i, hit) && std::strcmp(hit.ssid, ssid) == 0) {
            return true;
        }
    }
    return false;
}

bool Network::isSavedSsid(const char* ssid) const { return findProfile(ssid) >= 0; }

bool Network::isFirstPublicSsid(int radio_index, const char* ssid) const {
    if (radio_ == nullptr || ssid == nullptr) {
        return false;
    }
    WifiScanHit earlier;
    for (int i = 0; i < radio_index; ++i) {
        if (radio_->scanAt(i, earlier) && !isSavedSsid(earlier.ssid) &&
            std::strcmp(earlier.ssid, ssid) == 0) {
            return false;
        }
    }
    return true;
}

void Network::bestPublicHit(const char* ssid, WifiScanHit& out) const {
    WifiScanHit hit;
    bool found = false;
    for (int i = 0; i < radio_->scanCount(); ++i) {
        if (!radio_->scanAt(i, hit) || isSavedSsid(hit.ssid) || std::strcmp(hit.ssid, ssid) != 0) {
            continue;
        }
        if (!found || hit.rssi > out.rssi) {
            out = hit;
            found = true;
        }
    }
}

void Network::startBackgroundRound() {
    if (reconnect_held_ || radio_ == nullptr || blob_.count == 0) {
        state_ = NetworkState::Disconnected;
        mode_ = Mode::Idle;
        return;
    }
    mode_ = Mode::Background;
    state_ = NetworkState::Connecting;
    waiting_for_scan_ = true;
    background_profile_ = 0;
    radio_->startScan();
    if (clock_ != nullptr) {
        action_started_ms_ = clock_->millis();
    }
}

void Network::tryNextBackgroundProfile() {
    if (radio_ == nullptr) {
        return;
    }
    while (background_profile_ < blob_.count) {
        const int index = background_profile_;
        ++background_profile_;
        if (!scanContains(blob_.profiles[index].ssid)) {
            continue;
        }
        copySsid(pending_ssid_, blob_.profiles[index].ssid);
        copyPassword(pending_password_, blob_.profiles[index].password);
        radio_->connect(pending_ssid_, pending_password_);
        if (clock_ != nullptr) {
            action_started_ms_ = clock_->millis();
        }
        return;
    }

    ++background_attempts_;
    waiting_for_scan_ = false;
    if (background_attempts_ < kBackgroundAttempts) {
        state_ = NetworkState::Disconnected;
        next_background_ms_ = clock_ != nullptr ? clock_->millis() + kBackgroundRetryMs : 0;
        return;
    }
    state_ = NetworkState::Disconnected;
    mode_ = Mode::Idle;
    next_background_ms_ = clock_ != nullptr ? clock_->millis() + kBackgroundIdleMs : 0;
    background_attempts_ = 0;
}

void Network::connect(const char* ssid, const char* password) {
    if (radio_ == nullptr || ssid == nullptr || ssid[0] == '\0') {
        return;
    }
    reconnect_held_ = false;
    copySsid(pending_ssid_, ssid);
    copyPassword(pending_password_, password);
    mode_ = Mode::Manual;
    state_ = NetworkState::Connecting;
    waiting_for_scan_ = false;
    if (clock_ != nullptr) {
        action_started_ms_ = clock_->millis();
    }
    char message[48] = {};
    std::snprintf(message, sizeof(message), "connecting %s", pending_ssid_);
    emit(message);
    radio_->connect(pending_ssid_, pending_password_);
}

void Network::connectProfile(int index) {
    if (index < 0 || index >= blob_.count) {
        return;
    }
    connect(blob_.profiles[index].ssid, blob_.profiles[index].password);
}

void Network::deleteProfile(int index) {
    if (index < 0 || index >= blob_.count) {
        return;
    }
    for (int i = index; i + 1 < blob_.count; ++i) {
        blob_.profiles[i] = blob_.profiles[i + 1];
    }
    blob_.profiles[blob_.count - 1] = Profile{};
    --blob_.count;
    saveProfiles();
    emit("profile deleted");
}

void Network::disconnect() {
    reconnect_held_ = true;
    next_background_ms_ = 0;
    mode_ = Mode::Idle;
    state_ = NetworkState::Disconnected;
    waiting_for_scan_ = false;
    if (radio_ != nullptr) {
        radio_->disconnect();
    }
    emit("disconnected");
}

void Network::startScan() {
    if (radio_ == nullptr) {
        return;
    }
    scan_pending_ = true;
    radio_->startScan();
    emit("scan");
}

bool Network::scanInProgress() const {
    return radio_ != nullptr && scan_pending_ && !radio_->scanComplete();
}

int Network::publicScanCount() const {
    if (radio_ == nullptr || !radio_->scanComplete()) {
        return 0;
    }
    int count = 0;
    WifiScanHit hit;
    for (int i = 0; i < radio_->scanCount(); ++i) {
        if (radio_->scanAt(i, hit) && !isSavedSsid(hit.ssid) && hit.ssid[0] != '\0' &&
            isFirstPublicSsid(i, hit.ssid)) {
            ++count;
        }
    }
    return count;
}

bool Network::publicScanAt(int index, WifiScanHit& out) const {
    if (radio_ == nullptr || index < 0 || !radio_->scanComplete()) {
        return false;
    }
    int visible = 0;
    WifiScanHit hit;
    for (int i = 0; i < radio_->scanCount(); ++i) {
        if (!radio_->scanAt(i, hit) || isSavedSsid(hit.ssid) || hit.ssid[0] == '\0' ||
            !isFirstPublicSsid(i, hit.ssid)) {
            continue;
        }
        if (visible == index) {
            bestPublicHit(hit.ssid, out);
            return true;
        }
        ++visible;
    }
    return false;
}

NetworkState Network::state() const { return state_; }

const char* Network::connectedSsid() const {
    if (state_ != NetworkState::Connected || radio_ == nullptr) {
        return "";
    }
    return radio_->connectedSsid();
}

bool Network::takeConnectedEdge() {
    const bool edge = connected_edge_;
    connected_edge_ = false;
    return edge;
}

int Network::profileCount() const { return blob_.count; }

const char* Network::profileSsid(int index) const {
    if (index < 0 || index >= blob_.count) {
        return "";
    }
    return blob_.profiles[index].ssid;
}

bool Network::profileHasPassword(int index) const {
    if (index < 0 || index >= blob_.count) {
        return false;
    }
    return blob_.profiles[index].password[0] != '\0';
}

SignalStrength Network::strengthFromRssi(int8_t rssi) const {
    if (rssi >= -60) {
        return SignalStrength::Strong;
    }
    if (rssi >= -70) {
        return SignalStrength::Mid;
    }
    if (rssi >= -80) {
        return SignalStrength::Weak;
    }
    return SignalStrength::Weakest;
}

int8_t Network::rssi() const {
    if (state_ != NetworkState::Connected || radio_ == nullptr) {
        return -127;
    }
    return radio_->rssi();
}

void Network::stationIp(char* out, size_t out_size) const {
    if (out == nullptr || out_size == 0) {
        return;
    }
    out[0] = '\0';
    if (state_ != NetworkState::Connected || radio_ == nullptr) {
        return;
    }
    radio_->stationIp(out, out_size);
}

SignalStrength Network::signalStrength() const {
    if (state_ != NetworkState::Connected || radio_ == nullptr) {
        return SignalStrength::None;
    }
    return strengthFromRssi(radio_->rssi());
}

void Network::update() {
    if (radio_ == nullptr || clock_ == nullptr) {
        return;
    }
    radio_->update();
    const uint32_t now = clock_->millis();
    const NetworkState radio = radio_->radioState();

    if (scan_pending_ && radio_->scanComplete()) {
        scan_pending_ = false;
    }

    if (state_ == NetworkState::Connected && radio != NetworkState::Connected) {
        if (reconnect_held_) {
            state_ = NetworkState::Disconnected;
            mode_ = Mode::Idle;
            return;
        }
        startBackgroundRound();
        return;
    }

    if (mode_ == Mode::Manual && state_ == NetworkState::Connecting) {
        if (radio == NetworkState::Connected) {
            rememberSuccess(pending_ssid_, pending_password_);
            state_ = NetworkState::Connected;
            mode_ = Mode::Idle;
            connected_edge_ = true;
            emit("connected");
            return;
        }
        if (radio == NetworkState::Failed || now - action_started_ms_ >= kManualTimeoutMs) {
            state_ = NetworkState::Failed;
            mode_ = Mode::Idle;
            emit("connect failed");
            return;
        }
        return;
    }

    if (mode_ == Mode::Background && state_ == NetworkState::Connecting) {
        if (waiting_for_scan_ && radio_->scanComplete()) {
            waiting_for_scan_ = false;
            tryNextBackgroundProfile();
            return;
        }
        if (!waiting_for_scan_ && radio == NetworkState::Connected) {
            rememberSuccess(pending_ssid_, pending_password_);
            state_ = NetworkState::Connected;
            mode_ = Mode::Idle;
            connected_edge_ = true;
            background_attempts_ = 0;
            emit("connected");
            return;
        }
        if (!waiting_for_scan_ && radio == NetworkState::Failed) {
            tryNextBackgroundProfile();
            return;
        }
        return;
    }

    if (!reconnect_held_ && state_ != NetworkState::Connected &&
        state_ != NetworkState::Failed && blob_.count > 0 && now >= next_background_ms_ &&
        next_background_ms_ != 0 && mode_ != Mode::Manual) {
        startBackgroundRound();
    }
}

}  // namespace luma
