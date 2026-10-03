#include "luma/apps/remote-app.h"

#include "luma/assets/brand-marks.h"
#include "luma/core/app-context.h"
#include "luma/core/clock.h"
#include "luma/core/infrared.h"
#include "luma/core/settings.h"
#include "luma/core/storage.h"
#include "luma/ui/app-chrome.h"
#include "luma/ui/font.h"
#include "luma/ui/layout.h"
#include "luma/ui/renderer.h"
#include "luma/ui/theme.h"

#include <cstdio>
#include <cstring>

namespace luma {
namespace {

constexpr const char* kStorePath = "/apps/remote/index";
constexpr char kMagic[4] = {'R', 'M', '0', '1'};

const char* keyLabel(int key) {
    static const char* kLabels[] = {"P  Power", "M  Mute", ";  Vol+", ".  Vol-", ",  Ch-", "/  Ch+"};
    return kLabels[key];
}

const char* protocolName(BuiltinProtocol protocol) {
    switch (protocol) {
        case BuiltinProtocol::Nec:
            return "NEC";
        case BuiltinProtocol::Samsung:
            return "Samsung";
        case BuiltinProtocol::Sony:
            return "Sony";
        case BuiltinProtocol::Rc6:
            return "RC6";
        case BuiltinProtocol::Panasonic:
            return "Panasonic";
        case BuiltinProtocol::Rca:
            return "RCA";
        case BuiltinProtocol::NecExt:
            return "NEC+";
        case BuiltinProtocol::Custom:
            return "Custom";
    }
    return "";
}

const char* familyName(CustomFamily family) {
    switch (family) {
        case CustomFamily::PulseDistance:
            return "Distance";
        case CustomFamily::PulseWidth:
            return "Width";
        case CustomFamily::Manchester:
            return "Manchester";
    }
    return "";
}

enum class Field : uint8_t {
    Name,
    Protocol,
    Family,
    Carrier,
    HeaderMark,
    HeaderSpace,
    BitMark,
    OneValue,
    ZeroValue,
    BitSpace,
    AddressBits,
    CommandBits,
    BitOrder,
    Invert,
    Toggle,
    Address,
    Command
};

int keyFromFace(const InputFrame& input) {
    if (input.action == InputAction::Up) {
        return static_cast<int>(RemoteKey::VolUp);
    }
    if (input.action == InputAction::Down) {
        return static_cast<int>(RemoteKey::VolDown);
    }
    if (input.action == InputAction::Left) {
        return static_cast<int>(RemoteKey::ChDown);
    }
    if (input.action == InputAction::Right) {
        return static_cast<int>(RemoteKey::ChUp);
    }
    if (input.action != InputAction::None || input.textLength != 1) {
        return -1;
    }
    switch (input.text[0]) {
        case 'p':
        case 'P':
            return static_cast<int>(RemoteKey::Power);
        case 'm':
        case 'M':
            return static_cast<int>(RemoteKey::Mute);
        case ';':
            return static_cast<int>(RemoteKey::VolUp);
        case '.':
            return static_cast<int>(RemoteKey::VolDown);
        case ',':
            return static_cast<int>(RemoteKey::ChDown);
        case '/':
            return static_cast<int>(RemoteKey::ChUp);
        default:
            return -1;
    }
}

char faceLetter(const InputFrame& input) {
    if (input.action != InputAction::None || input.textLength != 1) {
        return '\0';
    }
    char letter = input.text[0];
    if (letter >= 'A' && letter <= 'Z') {
        letter = static_cast<char>(letter - 'A' + 'a');
    }
    return letter;
}

bool nearBlack(uint8_t r, uint8_t g, uint8_t b) {
    const int max_c = r > g ? (r > b ? r : b) : (g > b ? g : b);
    const int min_c = r < g ? (r < b ? r : b) : (g < b ? g : b);
    return (max_c - min_c) < 24 && max_c < 48;
}

bool nearWhite(uint8_t r, uint8_t g, uint8_t b) {
    const int max_c = r > g ? (r > b ? r : b) : (g > b ? g : b);
    const int min_c = r < g ? (r < b ? r : b) : (g < b ? g : b);
    return (max_c - min_c) < 24 && min_c > 220;
}

void appendHex(uint32_t& value, char digit) {
    int nibble = -1;
    if (digit >= '0' && digit <= '9') {
        nibble = digit - '0';
    } else if (digit >= 'a' && digit <= 'f') {
        nibble = digit - 'a' + 10;
    } else if (digit >= 'A' && digit <= 'F') {
        nibble = digit - 'A' + 10;
    }
    if (nibble < 0) {
        return;
    }
    const uint32_t next = (value << 4) | static_cast<uint32_t>(nibble);
    if (next <= 0xFFFF) {
        value = next;
    }
}

void appendDecimal(uint16_t& value, char digit) {
    if (digit < '0' || digit > '9') {
        return;
    }
    const unsigned next = static_cast<unsigned>(value) * 10U + static_cast<unsigned>(digit - '0');
    if (next <= 65535U) {
        value = static_cast<uint16_t>(next);
    }
}

void popHex(uint32_t& value) { value >>= 4; }

void popDecimal(uint16_t& value) { value = static_cast<uint16_t>(value / 10); }

void drawMark(DisplaySurface& display, const theme::Palette& palette, const assets::RgbaMark* drawn, int x,
              int y) {
    if (drawn == nullptr || drawn->rgba == nullptr) {
        return;
    }
    for (int py = 0; py < drawn->height; ++py) {
        for (int px = 0; px < drawn->width; ++px) {
            const uint8_t* pixel = drawn->rgba + (py * drawn->width + px) * 4;
            if (pixel[3] < 16 || nearWhite(pixel[0], pixel[1], pixel[2])) {
                continue;
            }
            Color color{pixel[0], pixel[1], pixel[2]};
            if (nearBlack(pixel[0], pixel[1], pixel[2])) {
                color = palette.primary_text;
            }
            display.fillRect({x + px, y + py, 1, 1}, color);
        }
    }
}

}  // namespace

const char* RemoteApp::id() const { return "remote"; }
const char* RemoteApp::name() const { return "REMOTE"; }
Color RemoteApp::accent() const { return theme::kBenimidori; }

void RemoteApp::onEnter(AppContext& context) {
    context_ = &context;
    screen_ = Screen::Face;
    cursor_ = 0;
    last_key_ = -1;
    full_ = false;
    loadStore();
}

void RemoteApp::onExit() {
    if (context_ != nullptr && screen_ == Screen::Editor) {
        saveStore();
    }
    context_ = nullptr;
}

int RemoteApp::userCount() const {
    int count = 0;
    for (int slot = 0; slot < kMaxUserRemotes; ++slot) {
        if (user_used_[slot]) {
            ++count;
        }
    }
    return count;
}

int RemoteApp::cycleCount() const { return kBrandRemoteCount + userCount(); }

void RemoteApp::collectUserOrder(int* order) const {
    int count = 0;
    for (int slot = 0; slot < kMaxUserRemotes; ++slot) {
        if (user_used_[slot]) {
            order[count++] = slot;
        }
    }
    for (int i = 0; i < count; ++i) {
        for (int j = i + 1; j < count; ++j) {
            if (users_[order[j]].mtime > users_[order[i]].mtime) {
                const int swap = order[i];
                order[i] = order[j];
                order[j] = swap;
            }
        }
    }
}

bool RemoteApp::currentIsBrand() const { return cursor_ >= 0 && cursor_ < kBrandRemoteCount; }

int RemoteApp::currentBrand() const { return currentIsBrand() ? cursor_ : -1; }

int RemoteApp::currentUserSlot() const {
    if (currentIsBrand()) {
        return -1;
    }
    int order[kMaxUserRemotes] = {};
    collectUserOrder(order);
    const int index = cursor_ - kBrandRemoteCount;
    if (index < 0 || index >= userCount()) {
        return -1;
    }
    return order[index];
}

bool RemoteApp::nameTaken(const char* candidate, int ignore) const {
    for (int slot = 0; slot < kMaxUserRemotes; ++slot) {
        if (!user_used_[slot] || slot == ignore) {
            continue;
        }
        if (std::strcmp(users_[slot].name, candidate) == 0) {
            return true;
        }
    }
    return false;
}

void RemoteApp::assignUnique(const char* base, char* out) const {
    if (!nameTaken(base, -1)) {
        std::snprintf(out, 16, "%s", base);
        return;
    }
    for (int suffix = 2; suffix < 100; ++suffix) {
        char candidate[16] = {};
        std::snprintf(candidate, sizeof(candidate), "%s %d", base, suffix);
        if (!nameTaken(candidate, -1)) {
            std::snprintf(out, 16, "%s", candidate);
            return;
        }
    }
    std::snprintf(out, 16, "%s", base);
}

void RemoteApp::loadStore() {
    std::memset(users_, 0, sizeof(users_));
    std::memset(user_used_, 0, sizeof(user_used_));
    next_stamp_ = 1;
    if (context_ == nullptr) {
        return;
    }
    char buffer[8 + sizeof(users_)] = {};
    size_t length = 0;
    if (!context_->storage().readFile(kStorePath, buffer, sizeof(buffer), length) || length < 5) {
        return;
    }
    if (std::memcmp(buffer, kMagic, 4) != 0) {
        return;
    }
    const int count = static_cast<unsigned char>(buffer[4]);
    const size_t record = sizeof(RemoteDocument);
    if (count < 0 || count > kMaxUserRemotes || length < 5 + static_cast<size_t>(count) * record) {
        return;
    }
    for (int slot = 0; slot < count; ++slot) {
        std::memcpy(&users_[slot], buffer + 5 + static_cast<size_t>(slot) * record, record);
        users_[slot].name[15] = '\0';
        user_used_[slot] = true;
        if (users_[slot].mtime >= next_stamp_) {
            next_stamp_ = users_[slot].mtime + 1;
        }
    }
}

void RemoteApp::saveStore() {
    if (context_ == nullptr) {
        return;
    }
    int order[kMaxUserRemotes] = {};
    collectUserOrder(order);
    const int count = userCount();
    char buffer[8 + sizeof(users_)] = {};
    std::memcpy(buffer, kMagic, 4);
    buffer[4] = static_cast<char>(count);
    const size_t record = sizeof(RemoteDocument);
    for (int index = 0; index < count; ++index) {
        std::memcpy(buffer + 5 + static_cast<size_t>(index) * record, &users_[order[index]], record);
    }
    context_->storage().writeFileAtomic(kStorePath, buffer, 5 + static_cast<size_t>(count) * record);
}

bool RemoteApp::createEmpty() {
    if (userCount() >= kMaxUserRemotes) {
        return false;
    }
    int slot = -1;
    for (int index = 0; index < kMaxUserRemotes; ++index) {
        if (!user_used_[index]) {
            slot = index;
            break;
        }
    }
    if (slot < 0) {
        return false;
    }
    users_[slot] = {};
    assignUnique("Untitled", users_[slot].name);
    users_[slot].protocol = BuiltinProtocol::Nec;
    users_[slot].address = 0;
    users_[slot].mtime = next_stamp_++;
    user_used_[slot] = true;
    cursor_ = kBrandRemoteCount;
    saveStore();
    return true;
}

bool RemoteApp::createCopy(int brand) {
    if (brand < 0 || brand >= kBrandRemoteCount || userCount() >= kMaxUserRemotes) {
        return false;
    }
    int slot = -1;
    for (int index = 0; index < kMaxUserRemotes; ++index) {
        if (!user_used_[index]) {
            slot = index;
            break;
        }
    }
    if (slot < 0) {
        return false;
    }
    users_[slot] = {};
    assignUnique(brandName(brand), users_[slot].name);
    users_[slot].protocol = brandProtocol(brand);
    users_[slot].address = brandAddress(brand);
    users_[slot].logo = static_cast<int8_t>(brand);
    seedCustomFromBuiltin(users_[slot].protocol, users_[slot].custom);
    for (int key = 0; key < 6; ++key) {
        uint32_t command = 0;
        if (brandCommand(brand, static_cast<RemoteKey>(key), command)) {
            users_[slot].commands[key] = command;
            users_[slot].command_set[key] = true;
        }
    }
    users_[slot].mtime = next_stamp_++;
    user_used_[slot] = true;
    cursor_ = kBrandRemoteCount;
    saveStore();
    return true;
}

void RemoteApp::deleteCurrentUser() {
    const int slot = currentUserSlot();
    if (slot < 0) {
        return;
    }
    user_used_[slot] = false;
    users_[slot] = {};
    if (cursor_ >= cycleCount()) {
        cursor_ = cycleCount() - 1;
    }
    if (cursor_ < 0) {
        cursor_ = 0;
    }
    saveStore();
}

void RemoteApp::fire(RemoteKey key) {
    if (context_ == nullptr) {
        return;
    }
    Infrared::Frame frame;
    bool encoded = false;
    if (currentIsBrand()) {
        encoded = encodeBrand(currentBrand(), key, frame);
    } else {
        const int slot = currentUserSlot();
        if (slot >= 0) {
            encoded = encodeDocument(users_[slot], key, frame);
        }
    }
    last_key_ = static_cast<int>(key);
    if (encoded) {
        context_->infrared().transmit(frame);
    }
    context_->requestRedraw();
}

void RemoteApp::updateFace(const InputFrame& input) {
    const char letter = faceLetter(input);
    if (letter == '[') {
        cursor_ = (cursor_ + cycleCount() - 1) % cycleCount();
        last_key_ = -1;
        full_ = false;
        context_->requestRedraw();
        return;
    }
    if (letter == ']') {
        cursor_ = (cursor_ + 1) % cycleCount();
        last_key_ = -1;
        full_ = false;
        context_->requestRedraw();
        return;
    }
    if (letter == 'n') {
        if (userCount() >= kMaxUserRemotes) {
            full_ = true;
            context_->requestRedraw();
            return;
        }
        full_ = false;
        picker_ = 0;
        screen_ = Screen::Picker;
        context_->requestRedraw();
        return;
    }
    if (letter == 'e' && !currentIsBrand()) {
        editor_field_ = 0;
        editor_scroll_ = 0;
        screen_ = Screen::Editor;
        context_->requestRedraw();
        return;
    }
    if (input.action == InputAction::Delete && !currentIsBrand()) {
        screen_ = Screen::DeleteDialog;
        context_->requestRedraw();
        return;
    }
    const int key = keyFromFace(input);
    if (key >= 0) {
        full_ = false;
        fire(static_cast<RemoteKey>(key));
    }
}

int fieldCountFor(const RemoteDocument& document) {
    if (document.protocol != BuiltinProtocol::Custom) {
        return 9;
    }
    if (document.custom.family == CustomFamily::PulseWidth) {
        return 18;
    }
    if (document.custom.family == CustomFamily::Manchester) {
        return 17;
    }
    return 20;
}

Field fieldAt(const RemoteDocument& document, int index) {
    static const Field kCommands[] = {Field::Command, Field::Command, Field::Command,
                                      Field::Command, Field::Command, Field::Command};
    if (index >= fieldCountFor(document) - 6) {
        return kCommands[index - (fieldCountFor(document) - 6)];
    }
    if (document.protocol != BuiltinProtocol::Custom) {
        static const Field kBuiltin[] = {Field::Name, Field::Protocol, Field::Address};
        return kBuiltin[index];
    }
    if (document.custom.family == CustomFamily::PulseWidth) {
        static const Field kWidth[] = {Field::Name,        Field::Protocol, Field::Family,      Field::Carrier,
                                       Field::HeaderMark,  Field::OneValue, Field::ZeroValue,  Field::BitSpace,
                                       Field::CommandBits, Field::AddressBits, Field::BitOrder, Field::Address};
        return kWidth[index];
    }
    if (document.custom.family == CustomFamily::Manchester) {
        static const Field kMan[] = {Field::Name,       Field::Protocol,    Field::Family,     Field::Carrier,
                                     Field::HeaderMark, Field::HeaderSpace, Field::BitSpace,   Field::AddressBits,
                                     Field::CommandBits, Field::Toggle,     Field::Address};
        return kMan[index];
    }
    static const Field kDistance[] = {Field::Name,       Field::Protocol,    Field::Family,     Field::Carrier,
                                      Field::HeaderMark, Field::HeaderSpace, Field::BitMark,    Field::OneValue,
                                      Field::ZeroValue,  Field::AddressBits, Field::CommandBits, Field::BitOrder,
                                      Field::Invert,     Field::Address};
    return kDistance[index];
}

int commandOrdinal(const RemoteDocument& document, int field_index) {
    int seen = 0;
    const int count = fieldCountFor(document);
    for (int index = 0; index <= field_index && index < count; ++index) {
        if (fieldAt(document, index) == Field::Command) {
            if (index == field_index) {
                return seen;
            }
            ++seen;
        }
    }
    return 0;
}

void RemoteApp::updateEditor(const InputFrame& input) {
    const int slot = currentUserSlot();
    if (slot < 0) {
        screen_ = Screen::Face;
        context_->consumeBack();
        return;
    }
    RemoteDocument& document = users_[slot];
    const int count = fieldCountFor(document);
    if (editor_field_ >= count) {
        editor_field_ = count - 1;
    }
    if (input.action == InputAction::Back) {
        document.mtime = next_stamp_++;
        saveStore();
        screen_ = Screen::Face;
        context_->consumeBack();
        context_->requestRedraw();
        return;
    }
    if (input.action == InputAction::Up && editor_field_ > 0) {
        --editor_field_;
        context_->requestRedraw();
        return;
    }
    if (input.action == InputAction::Down && editor_field_ + 1 < count) {
        ++editor_field_;
        context_->requestRedraw();
        return;
    }
    const Field field = fieldAt(document, editor_field_);
    const bool left = input.action == InputAction::Left;
    const bool right = input.action == InputAction::Right || input.action == InputAction::Confirm;
    if (field == Field::Protocol && (left || right)) {
        int value = static_cast<int>(document.protocol);
        value = right ? (value + 1) % 8 : (value + 7) % 8;
        const BuiltinProtocol next = static_cast<BuiltinProtocol>(value);
        if (next == BuiltinProtocol::Custom && document.protocol != BuiltinProtocol::Custom) {
            seedCustomFromBuiltin(document.protocol, document.custom);
        }
        document.protocol = next;
        context_->requestRedraw();
        return;
    }
    if (field == Field::Family && (left || right)) {
        int value = static_cast<int>(document.custom.family);
        value = right ? (value + 1) % 3 : (value + 2) % 3;
        document.custom.family = static_cast<CustomFamily>(value);
        context_->requestRedraw();
        return;
    }
    if ((field == Field::AddressBits || field == Field::CommandBits) && (left || right)) {
        uint8_t& bits = field == Field::AddressBits ? document.custom.address_bits : document.custom.command_bits;
        if (right && bits < 16) {
            ++bits;
        }
        if (left && bits > 1) {
            --bits;
        }
        context_->requestRedraw();
        return;
    }
    if ((field == Field::BitOrder || field == Field::Invert || field == Field::Toggle) && (left || right)) {
        if (field == Field::BitOrder) {
            document.custom.lsb_first = !document.custom.lsb_first;
        } else if (field == Field::Invert) {
            document.custom.invert = !document.custom.invert;
        } else {
            document.custom.leading_toggle = !document.custom.leading_toggle;
        }
        context_->requestRedraw();
        return;
    }
    if (input.action == InputAction::Delete) {
        if (field == Field::Name && document.name[0] != '\0') {
            document.name[std::strlen(document.name) - 1] = '\0';
        } else if (field == Field::Address) {
            popHex(document.address);
        } else if (field == Field::Command) {
            popHex(document.commands[commandOrdinal(document, editor_field_)]);
        } else if (field == Field::Carrier) {
            popDecimal(document.custom.carrier_khz);
        } else if (field == Field::HeaderMark) {
            popDecimal(document.custom.header_mark);
        } else if (field == Field::HeaderSpace) {
            popDecimal(document.custom.header_space);
        } else if (field == Field::BitMark) {
            popDecimal(document.custom.bit_mark);
        } else if (field == Field::OneValue) {
            popDecimal(document.custom.one_space);
        } else if (field == Field::ZeroValue) {
            popDecimal(document.custom.zero_space);
        } else if (field == Field::BitSpace) {
            popDecimal(document.custom.bit_space);
        } else if (field == Field::AddressBits && document.custom.address_bits > 1) {
            --document.custom.address_bits;
        } else if (field == Field::CommandBits && document.custom.command_bits > 1) {
            --document.custom.command_bits;
        }
        context_->requestRedraw();
        return;
    }
    if (input.textLength != 1) {
        return;
    }
    const char digit = input.text[0];
    if (field == Field::Name && std::strlen(document.name) < 15 && digit >= 32 && digit < 127) {
        const size_t length = std::strlen(document.name);
        document.name[length] = digit;
        document.name[length + 1] = '\0';
    } else if (field == Field::Address) {
        appendHex(document.address, digit);
    } else if (field == Field::Command) {
        const int ordinal = commandOrdinal(document, editor_field_);
        appendHex(document.commands[ordinal], digit);
        document.command_set[ordinal] = true;
    } else if (field == Field::Carrier) {
        appendDecimal(document.custom.carrier_khz, digit);
    } else if (field == Field::HeaderMark) {
        appendDecimal(document.custom.header_mark, digit);
    } else if (field == Field::HeaderSpace) {
        appendDecimal(document.custom.header_space, digit);
    } else if (field == Field::BitMark) {
        appendDecimal(document.custom.bit_mark, digit);
    } else if (field == Field::OneValue) {
        appendDecimal(document.custom.one_space, digit);
    } else if (field == Field::ZeroValue) {
        appendDecimal(document.custom.zero_space, digit);
    } else if (field == Field::BitSpace) {
        appendDecimal(document.custom.bit_space, digit);
    } else if (field == Field::AddressBits && digit >= '0' && digit <= '9') {
        const int next = digit - '0';
        if (next >= 1 && next <= 9) {
            document.custom.address_bits = static_cast<uint8_t>(next);
        }
    } else if (field == Field::CommandBits && digit >= '0' && digit <= '9') {
        const int next = digit - '0';
        if (next >= 1 && next <= 9) {
            document.custom.command_bits = static_cast<uint8_t>(next);
        }
    }
    context_->requestRedraw();
}

void RemoteApp::updatePicker(const InputFrame& input) {
    const int rows = 1 + kBrandRemoteCount;
    if (input.action == InputAction::Back) {
        screen_ = Screen::Face;
        context_->consumeBack();
        context_->requestRedraw();
        return;
    }
    if (input.action == InputAction::Up && picker_ > 0) {
        --picker_;
        context_->requestRedraw();
        return;
    }
    if (input.action == InputAction::Down && picker_ + 1 < rows) {
        ++picker_;
        context_->requestRedraw();
        return;
    }
    if (input.action == InputAction::Confirm) {
        const bool created = picker_ == 0 ? createEmpty() : createCopy(picker_ - 1);
        screen_ = Screen::Face;
        if (!created) {
            full_ = true;
        }
        context_->requestRedraw();
    }
}

void RemoteApp::updateDelete(const InputFrame& input) {
    if (input.action == InputAction::Back) {
        screen_ = Screen::Face;
        context_->consumeBack();
        context_->requestRedraw();
        return;
    }
    if (input.action == InputAction::Confirm) {
        deleteCurrentUser();
        screen_ = Screen::Face;
        context_->requestRedraw();
    }
}

void RemoteApp::update(const InputFrame& input) {
    if (context_ == nullptr) {
        return;
    }
    switch (screen_) {
        case Screen::Face:
            updateFace(input);
            break;
        case Screen::Editor:
            updateEditor(input);
            break;
        case Screen::Picker:
            updatePicker(input);
            break;
        case Screen::DeleteDialog:
            updateDelete(input);
            break;
    }
}

void RemoteApp::drawFace() {
    UiRenderer renderer(context_->display(), theme::paletteFor(context_->settings().theme(), accent()));
    renderer.beginFrame();
    renderer.clearAppCanvas();
    drawStandardHeader(*context_, renderer, name());
    const theme::Palette palette = renderer.palette();
    const char* title = "";
    if (full_) {
        title = "FULL";
    } else if (currentIsBrand()) {
        title = brandName(currentBrand());
    } else {
        const int slot = currentUserSlot();
        title = slot >= 0 ? users_[slot].name : "";
    }
    const assets::RgbaMark* mark = nullptr;
    if (currentIsBrand()) {
        mark = assets::brandMark(currentBrand());
    } else {
        const int slot = currentUserSlot();
        if (slot >= 0 && users_[slot].logo >= 0) {
            mark = assets::brandMark(users_[slot].logo);
        }
    }
    if (mark == nullptr || mark->rgba == nullptr || full_) {
        const int title_w = font::textWidth(title, 1, true);
        context_->display().drawText({(layout::kWidth - title_w) / 2, 40}, {palette.primary_text, 1, true},
                                     title);
    } else {
        drawMark(context_->display(), palette, mark, (layout::kWidth - mark->width) / 2, 34);
    }
    for (int key = 0; key < 6; ++key) {
        const int column = key % 2;
        const int row = key / 2;
        Rect bounds{column == 0 ? 6 : 123, 64 + row * 23, 111, 20};
        const bool hot = key == last_key_;
        context_->display().fillRoundRect(bounds, layout::kCardRadius, hot ? palette.accent : palette.card);
        context_->display().drawText({bounds.x + 6, bounds.y + 5},
                                     {hot ? palette.canvas : palette.primary_text, 1, false}, keyLabel(key));
    }
    renderer.endFrame();
}

const char* fieldCaption(const RemoteDocument& document, int field_index, char* value, size_t value_size) {
    const Field field = fieldAt(document, field_index);
    switch (field) {
        case Field::Name:
            std::snprintf(value, value_size, "%s", document.name);
            return "Name";
        case Field::Protocol:
            std::snprintf(value, value_size, "%s", protocolName(document.protocol));
            return "Protocol";
        case Field::Family:
            std::snprintf(value, value_size, "%s", familyName(document.custom.family));
            return "Family";
        case Field::Carrier:
            std::snprintf(value, value_size, "%u", document.custom.carrier_khz);
            return "kHz";
        case Field::HeaderMark:
            std::snprintf(value, value_size, "%u", document.custom.header_mark);
            return "Hdr mark";
        case Field::HeaderSpace:
            std::snprintf(value, value_size, "%u", document.custom.header_space);
            return "Hdr space";
        case Field::BitMark:
            std::snprintf(value, value_size, "%u", document.custom.bit_mark);
            return "Bit mark";
        case Field::OneValue:
            std::snprintf(value, value_size, "%u", document.custom.one_space);
            return "One";
        case Field::ZeroValue:
            std::snprintf(value, value_size, "%u", document.custom.zero_space);
            return "Zero";
        case Field::BitSpace:
            std::snprintf(value, value_size, "%u", document.custom.bit_space);
            return "Space";
        case Field::AddressBits:
            std::snprintf(value, value_size, "%u", document.custom.address_bits);
            return "Addr bits";
        case Field::CommandBits:
            std::snprintf(value, value_size, "%u", document.custom.command_bits);
            return "Cmd bits";
        case Field::BitOrder:
            std::snprintf(value, value_size, "%s", document.custom.lsb_first ? "LSB" : "MSB");
            return "Order";
        case Field::Invert:
            std::snprintf(value, value_size, "%s", document.custom.invert ? "On" : "Off");
            return "Invert";
        case Field::Toggle:
            std::snprintf(value, value_size, "%s", document.custom.leading_toggle ? "On" : "Off");
            return "Toggle";
        case Field::Address:
            std::snprintf(value, value_size, "%X", static_cast<unsigned>(document.address));
            return "Address";
        case Field::Command: {
            static const char* kNames[] = {"Power", "Mute", "Vol+", "Vol-", "Ch-", "Ch+"};
            const int ordinal = commandOrdinal(document, field_index);
            if (document.command_set[ordinal]) {
                std::snprintf(value, value_size, "%X", static_cast<unsigned>(document.commands[ordinal]));
            } else {
                std::snprintf(value, value_size, "-");
            }
            return kNames[ordinal];
        }
    }
    return "";
}

void RemoteApp::drawEditor() {
    const int slot = currentUserSlot();
    UiRenderer renderer(context_->display(), theme::paletteFor(context_->settings().theme(), accent()));
    renderer.beginFrame();
    renderer.clearAppCanvas();
    drawStandardHeader(*context_, renderer, name());
    const theme::Palette palette = renderer.palette();
    if (slot >= 0) {
        const RemoteDocument& document = users_[slot];
        const int count = fieldCountFor(document);
        constexpr int kVisible = 5;
        if (editor_field_ < editor_scroll_) {
            editor_scroll_ = editor_field_;
        }
        if (editor_field_ >= editor_scroll_ + kVisible) {
            editor_scroll_ = editor_field_ - kVisible + 1;
        }
        for (int row = 0; row < kVisible; ++row) {
            const int index = editor_scroll_ + row;
            if (index >= count) {
                break;
            }
            char value[20] = {};
            const char* caption = fieldCaption(document, index, value, sizeof(value));
            Rect bounds{6, layout::kContentBoth.y + 2 + row * 16, 228, 15};
            const bool selected = index == editor_field_;
            context_->display().fillRoundRect(bounds, 3, selected ? palette.accent : palette.card);
            char line[40] = {};
            std::snprintf(line, sizeof(line), "%s %s", caption, value);
            context_->display().drawText({10, bounds.y + 2},
                                         {selected ? palette.canvas : palette.primary_text, 1, false}, line);
        }
    }
    const KeyHint hints[] = {{"Esc", "done"}};
    drawStandardFooter(*context_, renderer, hints, 1);
    renderer.endFrame();
}

void RemoteApp::drawPicker() {
    UiRenderer renderer(context_->display(), theme::paletteFor(context_->settings().theme(), accent()));
    renderer.beginFrame();
    renderer.clearAppCanvas();
    drawStandardHeader(*context_, renderer, name());
    const theme::Palette palette = renderer.palette();
    constexpr int kVisible = 5;
    int scroll = 0;
    if (picker_ >= kVisible) {
        scroll = picker_ - kVisible + 1;
    }
    for (int row = 0; row < kVisible; ++row) {
        const int index = scroll + row;
        if (index > kBrandRemoteCount) {
            break;
        }
        const char* label = index == 0 ? "Empty" : brandName(index - 1);
        Rect bounds{6, layout::kContentBoth.y + 2 + row * 16, 228, 15};
        const bool selected = index == picker_;
        context_->display().fillRoundRect(bounds, 3, selected ? palette.accent : palette.card);
        context_->display().drawText({10, bounds.y + 2},
                                     {selected ? palette.canvas : palette.primary_text, 1, false}, label);
    }
    const KeyHint hints[] = {{"Ent", "new"}, {"Esc", "back"}};
    drawStandardFooter(*context_, renderer, hints, 2);
    renderer.endFrame();
}

void RemoteApp::drawDelete() {
    UiRenderer renderer(context_->display(), theme::paletteFor(context_->settings().theme(), accent()));
    renderer.beginFrame();
    renderer.clearAppCanvas();
    drawStandardHeader(*context_, renderer, name());
    const theme::Palette palette = renderer.palette();
    Rect card{40, 58, 160, 28};
    context_->display().fillRoundRect(card, layout::kCardRadius, palette.card);
    context_->display().drawText({88, 66}, {palette.primary_text, 1, true}, "Delete?");
    const KeyHint hints[] = {{"Ent", "ok"}, {"Esc", "back"}};
    drawStandardFooter(*context_, renderer, hints, 2);
    renderer.endFrame();
}

void RemoteApp::draw() {
    if (context_ == nullptr) {
        return;
    }
    switch (screen_) {
        case Screen::Face:
            drawFace();
            break;
        case Screen::Editor:
            drawEditor();
            break;
        case Screen::Picker:
            drawPicker();
            break;
        case Screen::DeleteDialog:
            drawDelete();
            break;
    }
}

}  // namespace luma
