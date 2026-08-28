#include "luma/apps/dots-app.h"

#include "luma/core/app-context.h"
#include "luma/core/clock.h"
#include "luma/core/diagnostics.h"
#include "luma/core/display.h"
#include "luma/core/settings.h"
#include "luma/core/storage.h"
#include "luma/ui/app-chrome.h"
#include "luma/ui/components.h"
#include "luma/ui/font.h"
#include "luma/ui/layout.h"
#include "luma/ui/renderer.h"
#include "luma/ui/theme.h"

#include <cstdio>
#include <cstring>

namespace luma {
namespace {

constexpr uint32_t kAutosaveDelayMs = 500;
constexpr uint32_t kFooterPageMs = 5000;
constexpr int kPenCount = 13;
constexpr int kRowHeight = 14;
constexpr int kRowGap = 2;
constexpr int kListPad = 3;
constexpr int kMatrixVisible = 4;
constexpr int kScrollbarWidth = 2;
constexpr int kIndexChipPadX = 2;
constexpr int kIndexBytes =
    4 + DotsApp::kMaxMatrices + DotsApp::kMaxMatrices * 4 +
    DotsApp::kMaxMatrices * static_cast<int>(DotsApp::kNameBytes);
const char kIndexMagic[4] = {'D', 'O', 'T', '1'};

const Color kPens[kPenCount] = {
    theme::kBenihi,     theme::kAraisyu, theme::kYamabuki, theme::kNae,      theme::kWakatake,
    theme::kAomidori,   theme::kMizu,    theme::kTsuyukusa, theme::kBenimidori, theme::kFuji,
    theme::kMomo,       theme::kTsutsuji, theme::kGofun,
};

int centeredTextY(int box_y, int box_h) { return box_y + (box_h - font::kGlyphHeight) / 2; }

Color penColor(uint8_t index) {
    if (index == 0 || index > kPenCount) {
        return theme::kKuro;
    }
    return kPens[index - 1];
}

void formatCursorCoord(int x, int y, char* out, size_t out_size) {
    std::snprintf(out, out_size, "%d,%d", x, y);
}

bool isUntitledName(const char* name) { return name != nullptr && std::strcmp(name, "Untitled") == 0; }

}  // namespace

const char* DotsApp::id() const { return "dots"; }
const char* DotsApp::name() const { return "DOTS"; }
Color DotsApp::accent() const { return theme::kYamabuki; }

void DotsApp::onEnter(AppContext& context) {
    context_ = &context;
    screen_ = Screen::List;
    list_selected_ = 0;
    list_scroll_ = 0;
    editing_slot_ = -1;
    delete_slot_ = -1;
    cursor_x_ = 0;
    cursor_y_ = 0;
    pen_index_ = 1;
    picker_index_ = 0;
    dirty_ = false;
    save_failed_ = false;
    created_new_ = false;
    last_edit_ms_ = 0;
    last_footer_page_ = 0;
    name_field_[0] = '\0';
    clearCells();
    loadStore();
    if (matrixCount() == 0) {
        list_selected_ = 0;
    }
}

void DotsApp::onExit() {
    if (screen_ == Screen::Paint || screen_ == Screen::Picker || screen_ == Screen::ClearDialog ||
        screen_ == Screen::NamePrompt) {
        if (dirty_) {
            saveDocument();
        }
        if (created_new_ && !hasLamps() && editing_slot_ >= 0) {
            deleteSlot(editing_slot_);
        }
    }
}

void DotsApp::loadStore() {
    for (int i = 0; i < kMaxMatrices; ++i) {
        slots_[i] = Slot{};
    }
    loadIndex();
}

bool DotsApp::loadIndex() {
    if (context_ == nullptr) {
        return false;
    }
    char buffer[kIndexBytes] = {};
    size_t length = 0;
    if (!context_->storage().readFile(kIndexPath, buffer, sizeof(buffer), length) ||
        length != static_cast<size_t>(kIndexBytes)) {
        return false;
    }
    if (std::memcmp(buffer, kIndexMagic, 4) != 0) {
        return false;
    }
    for (int i = 0; i < kMaxMatrices; ++i) {
        slots_[i].used = buffer[4 + i] != 0;
        std::memcpy(&slots_[i].mtime, buffer + 4 + kMaxMatrices + i * 4, 4);
        std::memcpy(slots_[i].name,
                    buffer + 4 + kMaxMatrices + kMaxMatrices * 4 + i * static_cast<int>(kNameBytes),
                    kNameBytes);
        slots_[i].name[kNameBytes - 1] = '\0';
    }
    return true;
}

bool DotsApp::saveIndex() {
    if (context_ == nullptr) {
        return false;
    }
    char buffer[kIndexBytes] = {};
    std::memcpy(buffer, kIndexMagic, 4);
    for (int i = 0; i < kMaxMatrices; ++i) {
        buffer[4 + i] = slots_[i].used ? 1 : 0;
        std::memcpy(buffer + 4 + kMaxMatrices + i * 4, &slots_[i].mtime, 4);
        std::memcpy(buffer + 4 + kMaxMatrices + kMaxMatrices * 4 + i * static_cast<int>(kNameBytes),
                    slots_[i].name, kNameBytes);
    }
    return context_->storage().writeFileAtomic(kIndexPath, buffer, sizeof(buffer));
}

void DotsApp::slotPath(int slot, char* out, size_t out_size) const {
    std::snprintf(out, out_size, "/apps/dots/%02d.bin", slot);
}

int DotsApp::matrixCount() const {
    int count = 0;
    for (int i = 0; i < kMaxMatrices; ++i) {
        if (slots_[i].used) {
            ++count;
        }
    }
    return count;
}

void DotsApp::collectOrdered(int* ordered) const {
    int count = 0;
    for (int i = 0; i < kMaxMatrices; ++i) {
        if (slots_[i].used) {
            ordered[count++] = i;
        }
    }
    for (int i = 0; i < count; ++i) {
        int best = i;
        for (int j = i + 1; j < count; ++j) {
            if (slots_[ordered[j]].mtime > slots_[ordered[best]].mtime ||
                (slots_[ordered[j]].mtime == slots_[ordered[best]].mtime &&
                 ordered[j] > ordered[best])) {
                best = j;
            }
        }
        const int tmp = ordered[i];
        ordered[i] = ordered[best];
        ordered[best] = tmp;
    }
}

int DotsApp::allocateSlot() const {
    for (int i = 0; i < kMaxMatrices; ++i) {
        if (!slots_[i].used) {
            return i;
        }
    }
    return -1;
}

uint32_t DotsApp::stamp() const {
    if (context_ == nullptr) {
        return next_stamp_;
    }
    const int64_t unix_utc = context_->clock().unixUtc();
    if (unix_utc > 0) {
        return static_cast<uint32_t>(unix_utc);
    }
    const uint32_t ms = context_->clock().millis();
    if (ms > 0) {
        return ms;
    }
    return next_stamp_;
}

bool DotsApp::hasLamps() const {
    for (size_t i = 0; i < kCellCount; ++i) {
        if (cells_[i] != 0) {
            return true;
        }
    }
    return false;
}

void DotsApp::clearCells() { std::memset(cells_, 0, sizeof(cells_)); }

void DotsApp::openNew() {
    if (matrixCount() >= kMaxMatrices) {
        context_->requestRedraw();
        return;
    }
    editing_slot_ = allocateSlot();
    if (editing_slot_ < 0) {
        context_->requestRedraw();
        return;
    }
    clearCells();
    cursor_x_ = 0;
    cursor_y_ = 0;
    slots_[editing_slot_].used = true;
    slots_[editing_slot_].mtime = stamp();
    std::snprintf(slots_[editing_slot_].name, kNameBytes, "Untitled");
    created_new_ = true;
    dirty_ = true;
    save_failed_ = false;
    last_edit_ms_ = context_->clock().millis();
    screen_ = Screen::Paint;
    ++next_stamp_;
    context_->requestRedraw();
}

void DotsApp::openSelected() {
    const int count = matrixCount();
    if (list_selected_ >= count) {
        openNew();
        return;
    }
    int ordered[kMaxMatrices] = {};
    collectOrdered(ordered);
    editing_slot_ = ordered[list_selected_];
    clearCells();
    char path[32] = {};
    slotPath(editing_slot_, path, sizeof(path));
    size_t loaded = 0;
    char buffer[kCellCount] = {};
    if (context_->storage().readFile(path, buffer, kCellCount, loaded)) {
        if (loaded > kCellCount) {
            loaded = kCellCount;
        }
        std::memcpy(cells_, buffer, loaded);
    }
    cursor_x_ = 0;
    cursor_y_ = 0;
    created_new_ = false;
    dirty_ = false;
    save_failed_ = false;
    last_edit_ms_ = 0;
    screen_ = Screen::Paint;
    context_->requestRedraw();
}

void DotsApp::leavePaint() {
    if (!hasLamps() && created_new_) {
        if (editing_slot_ >= 0) {
            deleteSlot(editing_slot_);
        }
        dirty_ = false;
        save_failed_ = false;
        created_new_ = false;
        editing_slot_ = -1;
        screen_ = Screen::List;
        if (list_selected_ > matrixCount()) {
            list_selected_ = matrixCount();
        }
        if (context_ != nullptr) {
            context_->consumeBack();
            context_->requestRedraw();
        }
        return;
    }
    if (dirty_) {
        saveDocument();
    }
    if (editing_slot_ >= 0 && isUntitledName(slots_[editing_slot_].name)) {
        beginNameEditor(Screen::NamePrompt, "");
        if (context_ != nullptr) {
            context_->consumeBack();
        }
        return;
    }
    finishLeavePaint();
    if (context_ != nullptr) {
        context_->consumeBack();
    }
}

void DotsApp::finishLeavePaint() {
    screen_ = Screen::List;
    created_new_ = false;
    if (editing_slot_ >= 0) {
        int ordered[kMaxMatrices] = {};
        collectOrdered(ordered);
        const int count = matrixCount();
        list_selected_ = count;
        for (int i = 0; i < count; ++i) {
            if (ordered[i] == editing_slot_) {
                list_selected_ = i;
                break;
            }
        }
    }
    editing_slot_ = -1;
    if (context_ != nullptr) {
        context_->requestRedraw();
    }
}

void DotsApp::confirmDelete() {
    if (delete_slot_ >= 0) {
        deleteSlot(delete_slot_);
    }
    delete_slot_ = -1;
    screen_ = Screen::List;
    if (list_selected_ > matrixCount()) {
        list_selected_ = matrixCount();
    }
    if (context_ != nullptr) {
        context_->requestRedraw();
    }
}

void DotsApp::deleteSlot(int slot) {
    if (slot < 0 || slot >= kMaxMatrices) {
        return;
    }
    slots_[slot] = Slot{};
    if (context_ != nullptr) {
        char path[32] = {};
        slotPath(slot, path, sizeof(path));
        context_->storage().removeFile(path);
        saveIndex();
    }
}

void DotsApp::markEdited() {
    dirty_ = true;
    save_failed_ = false;
    if (context_ != nullptr) {
        last_edit_ms_ = context_->clock().millis();
        context_->requestRedraw();
    }
}

void DotsApp::saveDocument() {
    if (context_ == nullptr || editing_slot_ < 0) {
        return;
    }
    char path[32] = {};
    slotPath(editing_slot_, path, sizeof(path));
    if (!context_->storage().writeFileAtomic(path, reinterpret_cast<const char*>(cells_),
                                             kCellCount)) {
        save_failed_ = true;
        context_->diagnostics().emit("ERROR", "dots save failed");
        context_->requestRedraw();
        return;
    }
    slots_[editing_slot_].used = true;
    slots_[editing_slot_].mtime = stamp();
    if (slots_[editing_slot_].name[0] == '\0') {
        std::snprintf(slots_[editing_slot_].name, kNameBytes, "Untitled");
    }
    if (!saveIndex()) {
        save_failed_ = true;
        context_->diagnostics().emit("ERROR", "dots save failed");
        context_->requestRedraw();
        return;
    }
    dirty_ = false;
    save_failed_ = false;
    ++next_stamp_;
}

void DotsApp::autosaveIfDue() {
    if (!dirty_ || context_ == nullptr) {
        return;
    }
    if (context_->clock().millis() - last_edit_ms_ < kAutosaveDelayMs) {
        return;
    }
    saveDocument();
}

void DotsApp::tickChrome() {
    if (context_ == nullptr) {
        return;
    }
    const uint32_t page = context_->clock().millis() / kFooterPageMs;
    if (page != last_footer_page_) {
        last_footer_page_ = page;
        context_->requestRedraw();
    }
}

bool DotsApp::nameTaken(const char* candidate, int ignore_slot) const {
    if (candidate == nullptr || candidate[0] == '\0') {
        return false;
    }
    for (int i = 0; i < kMaxMatrices; ++i) {
        if (!slots_[i].used || i == ignore_slot) {
            continue;
        }
        if (std::strcmp(slots_[i].name, candidate) == 0) {
            return true;
        }
    }
    return false;
}

void DotsApp::assignUniqueUntitled(char* out, size_t out_size) const {
    if (out == nullptr || out_size == 0) {
        return;
    }
    if (!nameTaken("Untitled", editing_slot_)) {
        std::snprintf(out, out_size, "Untitled");
        return;
    }
    for (int n = 2; n < 100; ++n) {
        char candidate[kNameBytes] = {};
        std::snprintf(candidate, sizeof(candidate), "Untitled %d", n);
        if (!nameTaken(candidate, editing_slot_)) {
            std::snprintf(out, out_size, "%s", candidate);
            return;
        }
    }
    std::snprintf(out, out_size, "Untitled");
}

bool DotsApp::tryCommitName(const char* typed, int slot) {
    if (slot < 0 || slot >= kMaxMatrices) {
        return false;
    }
    char resolved[kNameBytes] = {};
    if (typed == nullptr || typed[0] == '\0') {
        assignUniqueUntitled(resolved, sizeof(resolved));
    } else if (isUntitledName(typed) && nameTaken(typed, slot)) {
        assignUniqueUntitled(resolved, sizeof(resolved));
    } else if (nameTaken(typed, slot)) {
        return false;
    } else {
        std::snprintf(resolved, sizeof(resolved), "%s", typed);
    }
    std::snprintf(slots_[slot].name, kNameBytes, "%s", resolved);
    slots_[slot].mtime = stamp();
    saveIndex();
    return true;
}

void DotsApp::beginNameEditor(Screen screen, const char* seed) {
    screen_ = screen;
    if (seed != nullptr) {
        std::snprintf(name_field_, sizeof(name_field_), "%s", seed);
    } else {
        name_field_[0] = '\0';
    }
    if (context_ != nullptr) {
        context_->requestRedraw();
    }
}

void DotsApp::appendNameText(const InputFrame& input) {
    if (input.action == InputAction::Delete) {
        const size_t length = std::strlen(name_field_);
        if (length > 0) {
            name_field_[length - 1] = '\0';
            if (context_ != nullptr) {
                context_->requestRedraw();
            }
        }
        return;
    }
    for (uint8_t i = 0; i < input.textLength; ++i) {
        const char ch = input.text[i];
        if (ch < 32 || ch > 126) {
            continue;
        }
        const size_t length = std::strlen(name_field_);
        if (length + 1 >= kNameBytes) {
            break;
        }
        name_field_[length] = ch;
        name_field_[length + 1] = '\0';
        if (context_ != nullptr) {
            context_->requestRedraw();
        }
    }
}

void DotsApp::updateList(const InputFrame& input) {
    const int count = matrixCount();
    const int rows = count + 1;
    if (input.action == InputAction::Up && list_selected_ > 0) {
        --list_selected_;
        if (list_selected_ < list_scroll_) {
            list_scroll_ = list_selected_;
        }
        context_->requestRedraw();
        return;
    }
    if (input.action == InputAction::Down && list_selected_ < rows - 1) {
        ++list_selected_;
        if (list_selected_ >= count) {
            if (list_scroll_ < count - kMatrixVisible) {
                list_scroll_ = count > kMatrixVisible ? count - kMatrixVisible : 0;
            }
        } else if (list_selected_ >= list_scroll_ + kMatrixVisible) {
            list_scroll_ = list_selected_ - kMatrixVisible + 1;
        }
        context_->requestRedraw();
        return;
    }
    if (input.action == InputAction::Confirm) {
        openSelected();
        return;
    }
    if (input.action == InputAction::Delete && list_selected_ < count) {
        int ordered[kMaxMatrices] = {};
        collectOrdered(ordered);
        delete_slot_ = ordered[list_selected_];
        screen_ = Screen::DeleteDialog;
        context_->requestRedraw();
        return;
    }
    if (input.textLength == 1 && (input.text[0] == 'r' || input.text[0] == 'R') &&
        list_selected_ < count) {
        int ordered[kMaxMatrices] = {};
        collectOrdered(ordered);
        editing_slot_ = ordered[list_selected_];
        beginNameEditor(Screen::RenameEditor, slots_[editing_slot_].name);
    }
}

void DotsApp::updatePaint(const InputFrame& input) {
    if (input.action == InputAction::Left && cursor_x_ > 0) {
        --cursor_x_;
        context_->requestRedraw();
        return;
    }
    if (input.action == InputAction::Right && cursor_x_ < kCellsX - 1) {
        ++cursor_x_;
        context_->requestRedraw();
        return;
    }
    if (input.action == InputAction::Up && cursor_y_ > 0) {
        --cursor_y_;
        context_->requestRedraw();
        return;
    }
    if (input.action == InputAction::Down && cursor_y_ < kCellsY - 1) {
        ++cursor_y_;
        context_->requestRedraw();
        return;
    }
    if (input.action == InputAction::Confirm) {
        const size_t index = static_cast<size_t>(cursor_y_ * kCellsX + cursor_x_);
        if (cells_[index] != pen_index_) {
            cells_[index] = pen_index_;
            markEdited();
        }
        return;
    }
    if (input.action == InputAction::Delete) {
        const size_t index = static_cast<size_t>(cursor_y_ * kCellsX + cursor_x_);
        if (cells_[index] != 0) {
            cells_[index] = 0;
            markEdited();
        }
        return;
    }
    if (input.action == InputAction::Back) {
        leavePaint();
        return;
    }
    for (uint8_t i = 0; i < input.textLength; ++i) {
        const char ch = input.text[i];
        if (ch == 'c' || ch == 'C') {
            picker_index_ = pen_index_ > 0 ? static_cast<uint8_t>(pen_index_ - 1) : 0;
            screen_ = Screen::Picker;
            context_->requestRedraw();
            return;
        }
        if (ch == 'x' || ch == 'X') {
            screen_ = Screen::ClearDialog;
            context_->requestRedraw();
            return;
        }
    }
}

void DotsApp::updatePicker(const InputFrame& input) {
    if (input.action == InputAction::Left) {
        picker_index_ = picker_index_ == 0 ? static_cast<uint8_t>(kPenCount - 1)
                                           : static_cast<uint8_t>(picker_index_ - 1);
        context_->requestRedraw();
        return;
    }
    if (input.action == InputAction::Right) {
        picker_index_ = static_cast<uint8_t>((picker_index_ + 1) % kPenCount);
        context_->requestRedraw();
        return;
    }
    if (input.action == InputAction::Confirm) {
        pen_index_ = static_cast<uint8_t>(picker_index_ + 1);
        screen_ = Screen::Paint;
        context_->requestRedraw();
        return;
    }
    if (input.action == InputAction::Back) {
        screen_ = Screen::Paint;
        context_->consumeBack();
        context_->requestRedraw();
    }
}

void DotsApp::updateNameEditor(const InputFrame& input) {
    if (input.action == InputAction::Confirm) {
        if (!tryCommitName(name_field_, editing_slot_)) {
            context_->requestRedraw();
            return;
        }
        if (screen_ == Screen::NamePrompt) {
            finishLeavePaint();
        } else {
            screen_ = Screen::List;
            editing_slot_ = -1;
            context_->requestRedraw();
        }
        return;
    }
    if (input.action == InputAction::Back) {
        if (screen_ == Screen::NamePrompt) {
            finishLeavePaint();
        } else {
            screen_ = Screen::List;
            editing_slot_ = -1;
            if (context_ != nullptr) {
                context_->requestRedraw();
            }
        }
        context_->consumeBack();
        return;
    }
    appendNameText(input);
}

void DotsApp::updateClearDialog(const InputFrame& input) {
    if (input.action == InputAction::Confirm) {
        clearCells();
        markEdited();
        screen_ = Screen::Paint;
        return;
    }
    if (input.action == InputAction::Back) {
        screen_ = Screen::Paint;
        context_->consumeBack();
        context_->requestRedraw();
    }
}

void DotsApp::updateDeleteDialog(const InputFrame& input) {
    if (input.action == InputAction::Confirm) {
        confirmDelete();
        return;
    }
    if (input.action == InputAction::Back) {
        delete_slot_ = -1;
        screen_ = Screen::List;
        context_->consumeBack();
        context_->requestRedraw();
    }
}

void DotsApp::update(const InputFrame& input) {
    if (context_ == nullptr) {
        return;
    }
    tickChrome();
    if (screen_ == Screen::Paint || screen_ == Screen::Picker || screen_ == Screen::ClearDialog) {
        autosaveIfDue();
    }
    if (screen_ == Screen::List) {
        updateList(input);
        return;
    }
    if (screen_ == Screen::Paint) {
        updatePaint(input);
        return;
    }
    if (screen_ == Screen::Picker) {
        updatePicker(input);
        return;
    }
    if (screen_ == Screen::NamePrompt || screen_ == Screen::RenameEditor) {
        updateNameEditor(input);
        return;
    }
    if (screen_ == Screen::ClearDialog) {
        updateClearDialog(input);
        return;
    }
    updateDeleteDialog(input);
}

void DotsApp::drawList() {
    const theme::Palette palette = theme::paletteFor(context_->settings().theme(), accent());
    UiRenderer renderer(context_->display(), palette);
    renderer.beginFrame();
    renderer.clearAppCanvas();
    drawStandardHeader(*context_, renderer, name());

    const int count = matrixCount();
    int ordered[kMaxMatrices] = {};
    collectOrdered(ordered);
    if (list_scroll_ < 0) {
        list_scroll_ = 0;
    }
    if (count > kMatrixVisible && list_scroll_ + kMatrixVisible > count) {
        list_scroll_ = count - kMatrixVisible;
    }
    if (count <= kMatrixVisible) {
        list_scroll_ = 0;
    }

    const int content_y = layout::kContentBoth.y + 2;
    const int content_h = layout::kContentBoth.h - 4;
    const int list_x = layout::kChromeInset;
    const int list_w = layout::kWidth - 2 * layout::kChromeInset;
    const int new_y = content_y + content_h - kRowHeight;
    const bool overflow = count > kMatrixVisible;
    const int visible = count < kMatrixVisible ? count : kMatrixVisible;
    if (count > 0 && visible > 0) {
        const int inner_h = visible * kRowHeight + (visible - 1) * kRowGap;
        const int outer_h = inner_h + 2 * kListPad;
        const Rect outer{list_x, content_y, list_w, outer_h};
        renderer.surface().fillRoundRect(outer, layout::kCardRadius, palette.card);
        const int inner_x = list_x + kListPad;
        const int inner_w = list_w - 2 * kListPad - (overflow ? kScrollbarWidth + 2 : 0);
        if (overflow) {
            drawOverflowScrollbar(renderer.surface(), palette,
                                  {list_x + list_w - kListPad - kScrollbarWidth,
                                   content_y + kListPad, kScrollbarWidth, inner_h},
                                  count, list_scroll_, kMatrixVisible);
        }
        int row_y = content_y + kListPad;
        for (int i = 0; i < visible; ++i) {
            const int index = list_scroll_ + i;
            if (index >= count) {
                break;
            }
            const int slot = ordered[index];
            const bool selected = index == list_selected_;
            char number[4] = {};
            std::snprintf(number, sizeof(number), "%02d", index + 1);
            const int index_w = font::textWidth("00", 1) + 2 * kIndexChipPadX;
            const Rect index_chip{inner_x, row_y, index_w, kRowHeight};
            const Rect title_chip{inner_x + index_w + kRowGap, row_y, inner_w - index_w - kRowGap,
                                  kRowHeight};
            const Color chip_fill = selected ? palette.accent : palette.canvas;
            const Color chip_text = selected ? palette.canvas : palette.primary_text;
            renderer.surface().fillRoundRect(index_chip, layout::kCardRadius, chip_fill);
            renderer.surface().fillRoundRect(title_chip, layout::kCardRadius, chip_fill);
            renderer.surface().drawText(
                {index_chip.x + (index_chip.w - font::textWidth(number, 1)) / 2,
                 centeredTextY(index_chip.y, index_chip.h)},
                {chip_text, 1}, number);
            char label[kNameBytes] = {};
            ellipsizeToWidth(slots_[slot].name[0] != '\0' ? slots_[slot].name : "Untitled", label,
                             sizeof(label), title_chip.w - 8);
            renderer.surface().drawText({title_chip.x + 4, centeredTextY(title_chip.y, title_chip.h)},
                                        {chip_text, 1}, label);
            row_y += kRowHeight + kRowGap;
        }
    }

    const Rect new_row{list_x, new_y, list_w, kRowHeight};
    const bool new_selected = list_selected_ == count;
    renderer.surface().fillRoundRect(new_row, layout::kCardRadius,
                                     new_selected ? palette.accent : palette.card);
    renderer.surface().drawText({new_row.x + 4, centeredTextY(new_row.y, new_row.h)},
                                {new_selected ? palette.canvas : palette.primary_text, 1},
                                "New matrix");

    if (count >= kMaxMatrices && new_selected) {
        const KeyHint status[] = {{nullptr, "FULL"}, {"Esc", "back"}};
        drawStandardFooter(*context_, renderer, status, 2);
    } else if (new_selected) {
        const KeyHint hints[] = {{"Ent", "new"}, {"Esc", "back"}};
        drawStandardFooter(*context_, renderer, hints, 2);
    } else {
        const KeyHint hints[] = {{"Ent", "open"}, {"Del", "delete"}, {"R", "rename"}, {"Esc", "back"}};
        drawStandardFooter(*context_, renderer, hints, 4);
    }
    renderer.endFrame();
}

void DotsApp::drawPaint() {
    const theme::Palette palette = theme::paletteFor(context_->settings().theme(), accent());
    UiRenderer renderer(context_->display(), palette);
    renderer.beginFrame();
    renderer.surface().fillRect(layout::kContentFooterOnly, theme::kKuro);
    for (int y = 0; y < kCellsY; ++y) {
        for (int x = 0; x < kCellsX; ++x) {
            const uint8_t value = cells_[static_cast<size_t>(y * kCellsX + x)];
            if (value == 0) {
                continue;
            }
            const Rect dot{x * kCellPixels + 1, y * kCellPixels + 1, 2, 2};
            renderer.surface().fillRect(dot, penColor(value));
        }
    }
    const Rect cursor{cursor_x_ * kCellPixels, cursor_y_ * kCellPixels, kCellPixels, kCellPixels};
    renderer.surface().drawRect(cursor, theme::kGofun);
    char coord[8] = {};
    formatCursorCoord(cursor_x_, cursor_y_, coord, sizeof(coord));
    if (save_failed_) {
        const KeyHint status[] = {{nullptr, "SAVE FAIL"}};
        drawStandardFooter(*context_, renderer, status, 1, nullptr, coord);
    } else {
        const Color swatch = penColor(pen_index_);
        const KeyHint hints[] = {{"Ent", "paint"},
                                 {"Del", "erase"},
                                 {"Esc", "back"},
                                 kFooterPageBreak,
                                 {"C", "color"},
                                 {"X", "clear"}};
        drawStandardFooter(*context_, renderer, hints, 6, &swatch, coord);
    }
    renderer.endFrame();
}

void DotsApp::drawPicker() {
    const theme::Palette palette = theme::paletteFor(context_->settings().theme(), accent());
    UiRenderer renderer(context_->display(), palette);
    renderer.beginFrame();
    renderer.surface().fillRect(layout::kContentFooterOnly, theme::kKuro);
    constexpr int kChip = 14;
    constexpr int kGap = 2;
    const int row_w = kPenCount * kChip + (kPenCount - 1) * kGap;
    const int row_x = (layout::kWidth - row_w) / 2;
    const int row_y = 48;
    for (int i = 0; i < kPenCount; ++i) {
        const Rect chip{row_x + i * (kChip + kGap), row_y, kChip, kChip};
        renderer.surface().fillRect(chip, kPens[i]);
        if (i == picker_index_) {
            renderer.surface().drawRect(chip, theme::kGofun);
        }
    }
    const KeyHint hints[] = {{"Ent", "ok"}, {"Esc", "back"}};
    const Color swatch = kPens[picker_index_];
    drawStandardFooter(*context_, renderer, hints, 2, &swatch);
    renderer.endFrame();
}

void DotsApp::drawNameEditor() {
    const theme::Palette palette = theme::paletteFor(context_->settings().theme(), accent());
    UiRenderer renderer(context_->display(), palette);
    renderer.beginFrame();
    renderer.clearAppCanvas();
    drawStandardHeader(*context_, renderer, name());
    const char* title = screen_ == Screen::RenameEditor ? "Rename?" : "Name?";
    drawDialog(renderer.surface(), palette, title, name_field_[0] != '\0' ? name_field_ : "_");
    const KeyHint hints[] = {{"Ent", "ok"}, {"Esc", "back"}};
    drawStandardFooter(*context_, renderer, hints, 2);
    renderer.endFrame();
}

void DotsApp::drawClearDialog() {
    const theme::Palette palette = theme::paletteFor(context_->settings().theme(), accent());
    UiRenderer renderer(context_->display(), palette);
    renderer.beginFrame();
    renderer.surface().fillRect(layout::kContentFooterOnly, theme::kKuro);
    drawDialog(renderer.surface(), palette, "Clear?", "All lamps", layout::kContentFooterOnly);
    const KeyHint hints[] = {{"Ent", "clear"}, {"Esc", "cancel"}};
    drawStandardFooter(*context_, renderer, hints, 2);
    renderer.endFrame();
}

void DotsApp::drawDeleteDialog() {
    const theme::Palette palette = theme::paletteFor(context_->settings().theme(), accent());
    UiRenderer renderer(context_->display(), palette);
    renderer.beginFrame();
    renderer.clearAppCanvas();
    drawStandardHeader(*context_, renderer, name());
    const char* title = (delete_slot_ >= 0 && slots_[delete_slot_].name[0] != '\0')
                            ? slots_[delete_slot_].name
                            : "Untitled";
    drawDialog(renderer.surface(), palette, "Delete?", title);
    const KeyHint hints[] = {{"Ent", "delete"}, {"Esc", "cancel"}};
    drawStandardFooter(*context_, renderer, hints, 2);
    renderer.endFrame();
}

void DotsApp::draw() {
    if (context_ == nullptr) {
        return;
    }
    if (screen_ == Screen::List) {
        drawList();
        return;
    }
    if (screen_ == Screen::Picker) {
        drawPicker();
        return;
    }
    if (screen_ == Screen::NamePrompt || screen_ == Screen::RenameEditor) {
        drawNameEditor();
        return;
    }
    if (screen_ == Screen::ClearDialog) {
        drawClearDialog();
        return;
    }
    if (screen_ == Screen::DeleteDialog) {
        drawDeleteDialog();
        return;
    }
    drawPaint();
}

}  // namespace luma
