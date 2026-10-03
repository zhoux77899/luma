#include "cardputer-infrared.h"

#include <driver/rmt.h>

namespace luma {
namespace {

constexpr gpio_num_t kIrTxPin = GPIO_NUM_44;
constexpr rmt_channel_t kIrChannel = RMT_CHANNEL_0;

bool carrierOk(uint16_t khz) {
    return khz >= Infrared::kMinCarrierKhz && khz <= Infrared::kMaxCarrierKhz;
}

rmt_config_t txConfig(uint16_t frequency_khz) {
    rmt_config_t config = {};
    config.rmt_mode = RMT_MODE_TX;
    config.channel = kIrChannel;
    config.gpio_num = kIrTxPin;
    config.clk_div = 160;
    config.mem_block_num = 1;
    config.tx_config.loop_en = false;
    config.tx_config.carrier_en = true;
    config.tx_config.carrier_freq_hz = static_cast<uint32_t>(frequency_khz) * 1000U;
    config.tx_config.carrier_duty_percent = 33;
    config.tx_config.carrier_level = RMT_CARRIER_LEVEL_HIGH;
    config.tx_config.idle_output_en = true;
    config.tx_config.idle_level = RMT_IDLE_LEVEL_LOW;
    return config;
}

}  // namespace

CardputerInfrared::CardputerInfrared(Diagnostics& diagnostics) : diagnostics_(diagnostics) {}

void CardputerInfrared::begin() {
    const rmt_config_t config = txConfig(38);
    if (rmt_config(&config) != ESP_OK || rmt_driver_install(kIrChannel, 0, 0) != ESP_OK) {
        diagnostics_.emit("ERROR", "IR TX begin failed");
        begun_ = false;
        return;
    }
    begun_ = true;
}

bool CardputerInfrared::transmit(const Frame& frame) {
    if (!begun_ || !carrierOk(frame.frequency_khz) || frame.count == 0 ||
        frame.count > Infrared::kMaxDurations) {
        return false;
    }
    rmt_driver_uninstall(kIrChannel);
    const rmt_config_t config = txConfig(frame.frequency_khz);
    if (rmt_config(&config) != ESP_OK || rmt_driver_install(kIrChannel, 0, 0) != ESP_OK) {
        diagnostics_.emit("ERROR", "IR carrier failed");
        begun_ = false;
        return false;
    }

    auto ticks = [](uint16_t microseconds) {
        uint32_t value = (static_cast<uint32_t>(microseconds) + 1U) / 2U;
        if (value == 0) {
            value = 1;
        }
        if (value > 32767U) {
            value = 32767U;
        }
        return static_cast<uint32_t>(value);
    };

    rmt_item32_t items[Infrared::kMaxDurations / 2 + 1] = {};
    int item_count = 0;
    for (uint16_t index = 0; index < frame.count; index += 2) {
        const uint16_t mark = frame.durations[index];
        const uint16_t space = (index + 1 < frame.count) ? frame.durations[index + 1] : 2;
        if (mark == 0) {
            return false;
        }
        items[item_count].duration0 = ticks(mark);
        items[item_count].level0 = 1;
        items[item_count].duration1 = ticks(space);
        items[item_count].level1 = 0;
        ++item_count;
    }
    return rmt_write_items(kIrChannel, items, item_count, true) == ESP_OK;
}

}  // namespace luma
