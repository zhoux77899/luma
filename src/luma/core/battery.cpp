#include "luma/core/battery.h"

#include "luma/core/battery-source.h"
#include "luma/core/clock.h"
#include "luma/core/diagnostics.h"
#include "luma/core/storage.h"

namespace luma {
namespace {

constexpr uint8_t kFlagPercentValid = 1 << 0;
constexpr uint8_t kFlagVoltageValid = 1 << 1;
constexpr uint8_t kFlagChargingValid = 1 << 2;
constexpr uint8_t kFlagCharging = 1 << 3;

}  // namespace

void Battery::attach(BatterySource& source, Storage& storage, Diagnostics& diagnostics,
                     Clock& clock) {
    source_ = &source;
    storage_ = &storage;
    diagnostics_ = &diagnostics;
    clock_ = &clock;
}

void Battery::emitError(const char* message) {
    if (diagnostics_ != nullptr && message != nullptr) {
        diagnostics_->emit("ERROR", message);
    }
}

Battery::PackedSample Battery::pack(const BatterySample& sample) const {
    PackedSample packed;
    packed.percent = sample.reading.percent;
    packed.voltage_mv = sample.reading.voltage_mv;
    packed.millis = sample.millis;
    packed.unix_utc = sample.unix_utc;
    packed.run_id = sample.run_id;
    if (sample.reading.percent_valid) {
        packed.flags = static_cast<uint8_t>(packed.flags | kFlagPercentValid);
    }
    if (sample.reading.voltage_valid) {
        packed.flags = static_cast<uint8_t>(packed.flags | kFlagVoltageValid);
    }
    if (sample.reading.charging_valid) {
        packed.flags = static_cast<uint8_t>(packed.flags | kFlagChargingValid);
    }
    if (sample.reading.charging) {
        packed.flags = static_cast<uint8_t>(packed.flags | kFlagCharging);
    }
    return packed;
}

BatterySample Battery::unpack(const PackedSample& packed) const {
    BatterySample sample;
    sample.reading.percent = packed.percent;
    sample.reading.voltage_mv = packed.voltage_mv;
    sample.reading.percent_valid = (packed.flags & kFlagPercentValid) != 0;
    sample.reading.voltage_valid = (packed.flags & kFlagVoltageValid) != 0;
    sample.reading.charging_valid = (packed.flags & kFlagChargingValid) != 0;
    sample.reading.charging = (packed.flags & kFlagCharging) != 0;
    sample.millis = packed.millis;
    sample.unix_utc = packed.unix_utc;
    sample.run_id = packed.run_id;
    return sample;
}

void Battery::load() {
    count_ = 0;
    head_ = 0;
    run_id_ = 1;
    if (storage_ == nullptr) {
        return;
    }
    Blob loaded{};
    if (!storage_->loadPref(kPrefKey, &loaded, sizeof(loaded)) || loaded.schema != kSchema ||
        loaded.count > kMaxSamples) {
        return;
    }
    uint8_t max_run = 0;
    for (uint8_t i = 0; i < loaded.count; ++i) {
        samples_[i] = unpack(loaded.samples[i]);
        if (samples_[i].run_id > max_run) {
            max_run = samples_[i].run_id;
        }
    }
    count_ = loaded.count;
    head_ = 0;
    run_id_ = static_cast<uint8_t>(max_run + 1);
    if (run_id_ == 0) {
        run_id_ = 1;
    }
}

bool Battery::checkpoint() {
    if (storage_ == nullptr) {
        return false;
    }
    Blob blob{};
    blob.schema = kSchema;
    blob.count = static_cast<uint8_t>(count_);
    for (int i = 0; i < count_; ++i) {
        BatterySample sample;
        sampleAt(i, sample);
        blob.samples[i] = pack(sample);
    }
    if (!storage_->savePref(kPrefKey, &blob, sizeof(blob))) {
        emitError("battery checkpoint failed");
        return false;
    }
    samples_since_checkpoint_ = 0;
    return true;
}

void Battery::sampleNow() {
    if (source_ == nullptr || clock_ == nullptr) {
        return;
    }
    current_ = source_->read();
    BatterySample sample;
    sample.reading = current_;
    sample.millis = clock_->millis();
    const int64_t unix = clock_->unixUtc();
    sample.unix_utc = unix > 0 ? static_cast<uint32_t>(unix) : 0;
    sample.run_id = run_id_;

    if (count_ == kMaxSamples) {
        head_ = (head_ + 1) % kMaxSamples;
    } else {
        ++count_;
    }
    const int index = (head_ + count_ - 1) % kMaxSamples;
    samples_[index] = sample;
    last_sample_ms_ = sample.millis;
    sampled_ = true;
    ++samples_since_checkpoint_;
}

void Battery::begin() {
    if (source_ != nullptr) {
        current_ = source_->read();
    }
    sampleNow();
}

void Battery::update() {
    if (clock_ == nullptr) {
        return;
    }
    if (source_ != nullptr) {
        current_ = source_->read();
    }
    if (!sampled_ || (clock_->millis() - last_sample_ms_) >= kSampleMs) {
        sampleNow();
        if (samples_since_checkpoint_ >= kCheckpointSamples) {
            checkpoint();
        }
    }
}

BatteryReading Battery::current() const { return current_; }

uint8_t Battery::runId() const { return run_id_; }

int Battery::sampleCount() const { return count_; }

bool Battery::sampleAt(int index, BatterySample& out) const {
    if (index < 0 || index >= count_) {
        return false;
    }
    out = samples_[(head_ + index) % kMaxSamples];
    return true;
}

}  // namespace luma
