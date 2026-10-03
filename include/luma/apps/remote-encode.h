#pragma once

#include "luma/core/infrared.h"

#include <cstdint>

namespace luma {

enum class RemoteKey : uint8_t { Power = 0, Mute, VolUp, VolDown, ChDown, ChUp, Count };

enum class BuiltinProtocol : uint8_t { Nec = 0, Samsung, Sony, Rc6, Panasonic, Rca, NecExt, Custom };

enum class CustomFamily : uint8_t { PulseDistance = 0, PulseWidth, Manchester };

struct CustomRules {
    CustomFamily family = CustomFamily::PulseDistance;
    uint16_t carrier_khz = 38;
    uint16_t header_mark = 9000;
    uint16_t header_space = 4500;
    uint16_t bit_mark = 560;
    uint16_t one_space = 1690;
    uint16_t zero_space = 560;
    uint16_t bit_space = 600;
    uint8_t address_bits = 8;
    uint8_t command_bits = 8;
    bool lsb_first = true;
    bool invert = true;
    bool leading_toggle = false;
};

struct RemoteDocument {
    char name[16] = {};
    BuiltinProtocol protocol = BuiltinProtocol::Nec;
    CustomRules custom = {};
    uint32_t address = 0;
    uint32_t commands[6] = {};
    bool command_set[6] = {};
    uint32_t mtime = 0;
    int8_t logo = -1;
};

constexpr int kBrandRemoteCount = 7;
constexpr int kMaxUserRemotes = 9;
constexpr int kMaxRemotes = kBrandRemoteCount + kMaxUserRemotes;

const char* brandName(int index);
BuiltinProtocol brandProtocol(int index);
uint32_t brandAddress(int index);
bool brandCommand(int index, RemoteKey key, uint32_t& command);

void seedCustomFromBuiltin(BuiltinProtocol protocol, CustomRules& rules);

bool encodeBuiltin(BuiltinProtocol protocol, uint32_t address, uint32_t command, Infrared::Frame& out);
bool encodeCustom(const CustomRules& rules, uint32_t address, uint32_t command, Infrared::Frame& out);
bool encodeBrand(int brand, RemoteKey key, Infrared::Frame& out);
bool encodeDocument(const RemoteDocument& document, RemoteKey key, Infrared::Frame& out);

}  // namespace luma
