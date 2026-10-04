#include "luma/apps/remote-encode.h"

namespace luma {
namespace {

bool timingOk(uint16_t value) { return value >= 1; }

bool bitsOk(uint8_t bits) { return bits >= 1 && bits <= 16; }

bool carrierOk(uint16_t khz) {
    return khz >= Infrared::kMinCarrierKhz && khz <= Infrared::kMaxCarrierKhz;
}

bool push(Infrared::Frame& frame, uint16_t duration) {
    if (!timingOk(duration) || frame.count >= Infrared::kMaxDurations) {
        return false;
    }
    frame.durations[frame.count++] = duration;
    return true;
}

bool pushBitPulseDistance(Infrared::Frame& frame, bool one, uint16_t mark, uint16_t one_space,
                          uint16_t zero_space) {
    return push(frame, mark) && push(frame, one ? one_space : zero_space);
}

bool pushBits(Infrared::Frame& frame, uint32_t value, uint8_t bits, bool lsb_first, uint16_t mark,
              uint16_t one_space, uint16_t zero_space) {
    for (uint8_t index = 0; index < bits; ++index) {
        const uint8_t shift = lsb_first ? index : static_cast<uint8_t>(bits - 1 - index);
        const bool one = ((value >> shift) & 1U) != 0;
        if (!pushBitPulseDistance(frame, one, mark, one_space, zero_space)) {
            return false;
        }
    }
    return true;
}

bool encodeRca(uint32_t address, uint32_t command, Infrared::Frame& out) {
    out = {};
    out.frequency_khz = 38;
    if (!push(out, 4000) || !push(out, 4000)) {
        return false;
    }
    const uint8_t addr = static_cast<uint8_t>(address & 0x0F);
    const uint8_t cmd = static_cast<uint8_t>(command & 0xFF);
    uint32_t data = addr;
    data |= static_cast<uint32_t>(cmd) << 4;
    data |= static_cast<uint32_t>(static_cast<uint8_t>(~addr) & 0x0F) << 12;
    data |= static_cast<uint32_t>(static_cast<uint8_t>(~cmd)) << 16;
    return pushBits(out, data, 24, true, 500, 2000, 1000) && push(out, 500);
}

bool encodeNec(uint32_t address, uint32_t command, Infrared::Frame& out) {
    out = {};
    out.frequency_khz = 38;
    if (!push(out, 9000) || !push(out, 4500)) {
        return false;
    }
    const uint8_t addr = static_cast<uint8_t>(address & 0xFF);
    const uint8_t cmd = static_cast<uint8_t>(command & 0xFF);
    if (!pushBits(out, addr, 8, true, 560, 1690, 560) ||
        !pushBits(out, static_cast<uint8_t>(~addr), 8, true, 560, 1690, 560) ||
        !pushBits(out, cmd, 8, true, 560, 1690, 560) ||
        !pushBits(out, static_cast<uint8_t>(~cmd), 8, true, 560, 1690, 560)) {
        return false;
    }
    return push(out, 560);
}

bool appendNecExt(Infrared::Frame& out, uint32_t address, uint8_t command) {
    if (!push(out, 9000) || !push(out, 4500)) {
        return false;
    }
    if (!pushBits(out, address & 0xFFFF, 16, true, 560, 1690, 560) ||
        !pushBits(out, command, 8, true, 560, 1690, 560) ||
        !pushBits(out, static_cast<uint8_t>(~command), 8, true, 560, 1690, 560)) {
        return false;
    }
    return push(out, 560);
}

bool encodeNecExt(uint32_t address, uint32_t command, Infrared::Frame& out) {
    out = {};
    out.frequency_khz = 38;
    return appendNecExt(out, address, static_cast<uint8_t>(command & 0xFF));
}

bool encodeSamsung(uint32_t address, uint32_t command, Infrared::Frame& out) {
    out = {};
    out.frequency_khz = 38;
    if (!push(out, 4500) || !push(out, 4500)) {
        return false;
    }
    const uint8_t addr = static_cast<uint8_t>(address & 0xFF);
    const uint8_t cmd = static_cast<uint8_t>(command & 0xFF);
    if (!pushBits(out, addr, 8, true, 560, 1690, 560) ||
        !pushBits(out, addr, 8, true, 560, 1690, 560) ||
        !pushBits(out, cmd, 8, true, 560, 1690, 560) ||
        !pushBits(out, static_cast<uint8_t>(~cmd), 8, true, 560, 1690, 560)) {
        return false;
    }
    return push(out, 560);
}

bool encodeSony(uint32_t address, uint32_t command, Infrared::Frame& out) {
    out = {};
    out.frequency_khz = 40;
    if (!push(out, 2400) || !push(out, 600)) {
        return false;
    }
    auto sonyBits = [&](uint32_t value, uint8_t bits) {
        for (uint8_t index = 0; index < bits; ++index) {
            const bool one = ((value >> index) & 1U) != 0;
            if (!push(out, one ? 1200 : 600) || !push(out, 600)) {
                return false;
            }
        }
        return true;
    };
    return sonyBits(command & 0x7F, 7) && sonyBits(address & 0x1F, 5);
}

struct Level {
    bool mark;
    uint32_t us;
};

class LevelList {
public:
    bool add(bool mark, uint32_t us) {
        if (us == 0) {
            return false;
        }
        if (count_ > 0 && items_[count_ - 1].mark == mark) {
            const uint32_t sum = items_[count_ - 1].us + us;
            if (sum > 65535) {
                return false;
            }
            items_[count_ - 1].us = sum;
            return true;
        }
        if (count_ >= kMax) {
            return false;
        }
        items_[count_++] = Level{mark, us};
        return true;
    }

    bool toFrame(Infrared::Frame& out, uint16_t khz) const {
        out = {};
        out.frequency_khz = khz;
        if (count_ == 0 || !items_[0].mark) {
            return false;
        }
        for (int index = 0; index < count_; ++index) {
            if (items_[index].us == 0 || items_[index].us > 65535) {
                return false;
            }
        if (!push(out, static_cast<uint16_t>(items_[index].us))) {
            return false;
        }
    }
    if ((out.count % 2) != 0 && !push(out, 1000)) {
        return false;
    }
    return out.count > 0;
}

private:
    static constexpr int kMax = 220;
    Level items_[kMax] = {};
    int count_ = 0;
};

bool encodeRc6(uint32_t address, uint32_t command, Infrared::Frame& out) {
    constexpr uint16_t kHalf = 444;
    LevelList levels;
    if (!levels.add(true, 6 * kHalf) || !levels.add(false, 2 * kHalf)) {
        return false;
    }
    auto bit = [&](bool one, uint16_t half) {
        if (one) {
            return levels.add(true, half) && levels.add(false, half);
        }
        return levels.add(false, half) && levels.add(true, half);
    };
    if (!bit(true, kHalf)) {
        return false;
    }
    for (int index = 0; index < 3; ++index) {
        if (!bit(false, kHalf)) {
            return false;
        }
    }
    if (!bit(false, static_cast<uint16_t>(2 * kHalf))) {
        return false;
    }
    auto msb = [&](uint32_t value, int bits) {
        for (int index = bits - 1; index >= 0; --index) {
            if (!bit(((value >> index) & 1U) != 0, kHalf)) {
                return false;
            }
        }
        return true;
    };
    if (!msb(address & 0xFF, 8) || !msb(command & 0xFF, 8)) {
        return false;
    }
    return levels.toFrame(out, 36);
}

bool encodePanasonic(uint32_t command, Infrared::Frame& out) {
    uint8_t bytes[6] = {0x80, 0x02, 0x20, 0x00, static_cast<uint8_t>(command & 0xFF),
                        static_cast<uint8_t>((command >> 8) & 0xFF)};
    out = {};
    out.frequency_khz = 37;
    if (!push(out, 3456) || !push(out, 1728)) {
        return false;
    }
    for (int byte_index = 0; byte_index < 6; ++byte_index) {
        if (!pushBits(out, bytes[byte_index], 8, true, 432, 1296, 432)) {
            return false;
        }
    }
    return push(out, 432);
}

bool encodePulseDistance(const CustomRules& rules, uint32_t address, uint32_t command,
                         Infrared::Frame& out) {
    out = {};
    out.frequency_khz = rules.carrier_khz;
    if (!carrierOk(rules.carrier_khz) || !bitsOk(rules.address_bits) || !bitsOk(rules.command_bits) ||
        !timingOk(rules.header_mark) || !timingOk(rules.header_space) || !timingOk(rules.bit_mark) ||
        !timingOk(rules.one_space) || !timingOk(rules.zero_space)) {
        return false;
    }
    if (!push(out, rules.header_mark) || !push(out, rules.header_space)) {
        return false;
    }
    const uint32_t address_mask = (rules.address_bits == 16) ? 0xFFFFU : ((1U << rules.address_bits) - 1U);
    const uint32_t command_mask = (rules.command_bits == 16) ? 0xFFFFU : ((1U << rules.command_bits) - 1U);
    const uint32_t addr = address & address_mask;
    const uint32_t cmd = command & command_mask;
    if (!pushBits(out, addr, rules.address_bits, rules.lsb_first, rules.bit_mark, rules.one_space,
                  rules.zero_space) ||
        !pushBits(out, cmd, rules.command_bits, rules.lsb_first, rules.bit_mark, rules.one_space,
                  rules.zero_space)) {
        return false;
    }
    if (rules.invert) {
        const uint32_t inv_addr = (~addr) & address_mask;
        const uint32_t inv_cmd = (~cmd) & command_mask;
        if (!pushBits(out, inv_addr, rules.address_bits, rules.lsb_first, rules.bit_mark, rules.one_space,
                      rules.zero_space) ||
            !pushBits(out, inv_cmd, rules.command_bits, rules.lsb_first, rules.bit_mark, rules.one_space,
                      rules.zero_space)) {
            return false;
        }
    }
    return push(out, rules.bit_mark);
}

bool encodePulseWidth(const CustomRules& rules, uint32_t address, uint32_t command, Infrared::Frame& out) {
    out = {};
    out.frequency_khz = rules.carrier_khz;
    if (!carrierOk(rules.carrier_khz) || !bitsOk(rules.address_bits) || !bitsOk(rules.command_bits) ||
        !timingOk(rules.header_mark) || !timingOk(rules.one_space) || !timingOk(rules.zero_space) ||
        !timingOk(rules.bit_space)) {
        return false;
    }
    if (!push(out, rules.header_mark) || !push(out, rules.bit_space)) {
        return false;
    }
    auto bits = [&](uint32_t value, uint8_t count) {
        const uint32_t mask = (count == 16) ? 0xFFFFU : ((1U << count) - 1U);
        value &= mask;
        for (uint8_t index = 0; index < count; ++index) {
            const uint8_t shift = rules.lsb_first ? index : static_cast<uint8_t>(count - 1 - index);
            const bool one = ((value >> shift) & 1U) != 0;
            if (!push(out, one ? rules.one_space : rules.zero_space) || !push(out, rules.bit_space)) {
                return false;
            }
        }
        return true;
    };
    return bits(command, rules.command_bits) && bits(address, rules.address_bits);
}

bool encodeManchester(const CustomRules& rules, uint32_t address, uint32_t command, Infrared::Frame& out) {
    if (!carrierOk(rules.carrier_khz) || !bitsOk(rules.address_bits) || !bitsOk(rules.command_bits) ||
        !timingOk(rules.header_mark) || !timingOk(rules.header_space) || !timingOk(rules.bit_space)) {
        return false;
    }
    LevelList levels;
    if (!levels.add(true, rules.header_mark) || !levels.add(false, rules.header_space)) {
        return false;
    }
    auto bit = [&](bool one) {
        if (one) {
            return levels.add(true, rules.bit_space) && levels.add(false, rules.bit_space);
        }
        return levels.add(false, rules.bit_space) && levels.add(true, rules.bit_space);
    };
    if (rules.leading_toggle && !bit(false)) {
        return false;
    }
    auto field = [&](uint32_t value, uint8_t count) {
        const uint32_t mask = (count == 16) ? 0xFFFFU : ((1U << count) - 1U);
        value &= mask;
        for (int index = count - 1; index >= 0; --index) {
            if (!bit(((value >> index) & 1U) != 0)) {
                return false;
            }
        }
        return true;
    };
    if (!field(address, rules.address_bits) || !field(command, rules.command_bits)) {
        return false;
    }
    return levels.toFrame(out, rules.carrier_khz);
}

}  // namespace

const char* brandName(int index) {
    static const char* kNames[] = {"LG", "TCL", "Hisense", "Samsung", "Sony", "Panasonic", "Philips"};
    if (index < 0 || index >= kBrandRemoteCount) {
        return "";
    }
    return kNames[index];
}

BuiltinProtocol brandProtocol(int index) {
    switch (index) {
        case 1:
            return BuiltinProtocol::NecExt;
        case 3:
            return BuiltinProtocol::Samsung;
        case 4:
            return BuiltinProtocol::Sony;
        case 5:
            return BuiltinProtocol::Panasonic;
        case 6:
            return BuiltinProtocol::Rc6;
        default:
            return BuiltinProtocol::Nec;
    }
}

uint32_t brandAddress(int index) {
    switch (index) {
        case 2:
            return 0x00;
        case 3:
            return 0x07;
        case 4:
            return 0x01;
        case 6:
            return 0x00;
        case 0:
            return 0x04;
        case 1:
            return 0xC7EA;
        default:
            return 0;
    }
}

bool brandCommand(int index, RemoteKey key, uint32_t& command) {
    const int key_index = static_cast<int>(key);
    if (index < 0 || index >= kBrandRemoteCount || key_index < 0 || key_index >= 6) {
        return false;
    }
    // Power, Mute, Vol+, Vol-, Ch-, Ch+
    static const uint32_t kLg[] = {0x08, 0x09, 0x02, 0x03, 0x01, 0x00};
    static const uint32_t kHisense[] = {0x12, 0x09, 0x02, 0x03, 0x01, 0x00};
    static const uint32_t kSamsung[] = {0x02, 0x0F, 0x07, 0x0B, 0x10, 0x12};
    static const uint32_t kSony[] = {0x15, 0x14, 0x12, 0x13, 0x11, 0x10};
    static const uint32_t kPhilips[] = {0x0C, 0x0D, 0x10, 0x11, 0x21, 0x20};
    static const uint32_t kPanasonic[] = {0x03D0, 0x0320, 0x0200, 0x0210, 0x0350, 0x0340};
    const uint32_t* row = kLg;
    if (index == 1) {
        // Chat/gist 0x57E3 codes as NECext 0xC7EA. Channel keys reuse Up 0x19 and Down 0x33.
        switch (key) {
            case RemoteKey::Power:
                command = 0x17;
                return true;
            case RemoteKey::Mute:
                command = 0x20;
                return true;
            case RemoteKey::VolUp:
                command = 0x0F;
                return true;
            case RemoteKey::VolDown:
                command = 0x10;
                return true;
            case RemoteKey::ChUp:
                command = 0x19;
                return true;
            case RemoteKey::ChDown:
                command = 0x33;
                return true;
            default:
                return false;
        }
    }
    if (index == 2) {
        row = kHisense;
    } else if (index == 3) {
        row = kSamsung;
    } else if (index == 4) {
        row = kSony;
    } else if (index == 5) {
        row = kPanasonic;
    } else if (index == 6) {
        row = kPhilips;
    }
    command = row[key_index];
    return true;
}

void seedCustomFromBuiltin(BuiltinProtocol protocol, CustomRules& rules) {
    rules = {};
    switch (protocol) {
        case BuiltinProtocol::Samsung:
            rules.carrier_khz = 38;
            rules.header_mark = 4500;
            rules.header_space = 4500;
            rules.bit_mark = 560;
            rules.one_space = 1690;
            rules.zero_space = 560;
            rules.invert = false;
            break;
        case BuiltinProtocol::Sony:
            rules.family = CustomFamily::PulseWidth;
            rules.carrier_khz = 40;
            rules.header_mark = 2400;
            rules.one_space = 1200;
            rules.zero_space = 600;
            rules.bit_space = 600;
            rules.address_bits = 5;
            rules.command_bits = 7;
            rules.lsb_first = true;
            rules.invert = false;
            break;
        case BuiltinProtocol::Rc6:
            rules.family = CustomFamily::Manchester;
            rules.carrier_khz = 36;
            rules.header_mark = 2664;
            rules.header_space = 888;
            rules.bit_space = 444;
            rules.leading_toggle = true;
            rules.invert = false;
            break;
        case BuiltinProtocol::Panasonic:
            rules.carrier_khz = 37;
            rules.header_mark = 3456;
            rules.header_space = 1728;
            rules.bit_mark = 432;
            rules.one_space = 1296;
            rules.zero_space = 432;
            rules.address_bits = 16;
            rules.invert = false;
            break;
        case BuiltinProtocol::Rca:
            rules.carrier_khz = 38;
            rules.header_mark = 4000;
            rules.header_space = 4000;
            rules.bit_mark = 500;
            rules.one_space = 2000;
            rules.zero_space = 1000;
            rules.address_bits = 4;
            rules.command_bits = 8;
            rules.lsb_first = true;
            rules.invert = true;
            break;
        case BuiltinProtocol::NecExt:
            rules.carrier_khz = 38;
            rules.header_mark = 9000;
            rules.header_space = 4500;
            rules.bit_mark = 560;
            rules.one_space = 1690;
            rules.zero_space = 560;
            rules.address_bits = 16;
            rules.command_bits = 8;
            rules.lsb_first = true;
            rules.invert = false;
            break;
        case BuiltinProtocol::Nec:
        case BuiltinProtocol::Custom:
            rules.carrier_khz = 38;
            rules.header_mark = 9000;
            rules.header_space = 4500;
            rules.bit_mark = 560;
            rules.one_space = 1690;
            rules.zero_space = 560;
            rules.invert = true;
            break;
    }
}

bool encodeBuiltin(BuiltinProtocol protocol, uint32_t address, uint32_t command, Infrared::Frame& out) {
    switch (protocol) {
        case BuiltinProtocol::Nec:
            return encodeNec(address, command, out);
        case BuiltinProtocol::Samsung:
            return encodeSamsung(address, command, out);
        case BuiltinProtocol::Sony:
            return encodeSony(address, command, out);
        case BuiltinProtocol::Rc6:
            return encodeRc6(address, command, out);
        case BuiltinProtocol::Panasonic:
            return encodePanasonic(command, out);
        case BuiltinProtocol::Rca:
            return encodeRca(address, command, out);
        case BuiltinProtocol::NecExt:
            return encodeNecExt(address, command, out);
        case BuiltinProtocol::Custom:
            return false;
    }
    return false;
}

bool encodeCustom(const CustomRules& rules, uint32_t address, uint32_t command, Infrared::Frame& out) {
    switch (rules.family) {
        case CustomFamily::PulseDistance:
            return encodePulseDistance(rules, address, command, out);
        case CustomFamily::PulseWidth:
            return encodePulseWidth(rules, address, command, out);
        case CustomFamily::Manchester:
            return encodeManchester(rules, address, command, out);
    }
    return false;
}

bool encodeBrand(int brand, RemoteKey key, Infrared::Frame& out) {
    uint32_t command = 0;
    if (!brandCommand(brand, key, command)) {
        return false;
    }
    return encodeBuiltin(brandProtocol(brand), brandAddress(brand), command, out);
}

bool encodeDocument(const RemoteDocument& document, RemoteKey key, Infrared::Frame& out) {
    const int index = static_cast<int>(key);
    if (index < 0 || index >= 6 || !document.command_set[index]) {
        return false;
    }
    if (document.protocol == BuiltinProtocol::Custom) {
        return encodeCustom(document.custom, document.address, document.commands[index], out);
    }
    return encodeBuiltin(document.protocol, document.address, document.commands[index], out);
}

}  // namespace luma
