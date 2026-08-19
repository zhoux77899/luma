#pragma once

#include "luma/core/battery-types.h"

#include <cstdint>

namespace luma {

class BatterySource;
class Clock;
class Diagnostics;
class Storage;

class Battery {
public:
    static constexpr int kMaxSamples = 60;
    static constexpr uint8_t kSchema = 1;
    static constexpr uint32_t kSampleMs = 60000;
    static constexpr int kCheckpointSamples = 5;
    static constexpr const char* kPrefKey = "battery";

    void attach(BatterySource& source, Storage& storage, Diagnostics& diagnostics, Clock& clock);
    void load();
    void begin();
    void update();

    BatteryReading current() const;
    uint8_t runId() const;
    int sampleCount() const;
    bool sampleAt(int index, BatterySample& out) const;

private:
    struct PackedSample {
        uint8_t percent = 0;
        uint8_t flags = 0;
        uint16_t voltage_mv = 0;
        uint32_t millis = 0;
        uint32_t unix_utc = 0;
        uint8_t run_id = 0;
        uint8_t reserved[3] = {};
    };

    struct Blob {
        uint8_t schema = kSchema;
        uint8_t count = 0;
        uint8_t reserved[2] = {};
        PackedSample samples[kMaxSamples] = {};
    };

    void emitError(const char* message);
    void sampleNow();
    bool checkpoint();
    PackedSample pack(const BatterySample& sample) const;
    BatterySample unpack(const PackedSample& packed) const;

    BatterySource* source_ = nullptr;
    Storage* storage_ = nullptr;
    Diagnostics* diagnostics_ = nullptr;
    Clock* clock_ = nullptr;
    BatteryReading current_{};
    BatterySample samples_[kMaxSamples] = {};
    int count_ = 0;
    int head_ = 0;
    uint8_t run_id_ = 1;
    uint32_t last_sample_ms_ = 0;
    int samples_since_checkpoint_ = 0;
    bool sampled_ = false;
};

}  // namespace luma
