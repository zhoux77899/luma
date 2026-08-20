#include "luma/apps/notes-app.h"

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
constexpr int kBodyLineHeight = 12;
constexpr int kTitleLineHeight = font::kGlyphHeight * 2;
constexpr int kTextX = layout::kChromeInset;
constexpr int kMaxBodyCols = (layout::kWidth - 2 * layout::kChromeInset) / font::kGlyphWidth;
constexpr int kMaxTitleCols =
    (layout::kWidth - 2 * layout::kChromeInset) / (font::kGlyphWidth * 2);
constexpr int kIndexBytes = 4 + NotesApp::kMaxNotes + NotesApp::kMaxNotes * 4;
constexpr int kRowHeight = 14;
constexpr int kRowGap = 2;
constexpr int kListPad = 3;
constexpr int kNoteVisible = 4;
constexpr int kScrollbarWidth = 2;
constexpr int kIndexChipPadX = 2;
const char kIndexMagic[4] = {'N', 'T', 'E', '1'};

int centeredTextY(int box_y, int box_h) { return box_y + (box_h - font::kGlyphHeight) / 2; }

}  // namespace

const char* NotesApp::id() const { return "notes"; }
const char* NotesApp::name() const { return "NOTES"; }
Color NotesApp::accent() const { return theme::kWakatake; }

void NotesApp::onEnter(AppContext& context) {
    context_ = &context;
    screen_ = Screen::List;
    list_selected_ = 0;
    list_scroll_ = 0;
    editing_slot_ = -1;
    delete_slot_ = -1;
    length_ = 0;
    cursor_ = 0;
    scroll_line_ = 0;
    scroll_col_ = 0;
    dirty_ = false;
    save_failed_ = false;
    last_edit_ms_ = 0;
    std::memset(document_, 0, sizeof(document_));
    loadStore();
    if (noteCount() == 0) {
        list_selected_ = 0;
    }
}

void NotesApp::onExit() {
    if (screen_ == Screen::Editor && dirty_) {
        leaveEditor();
    }
}

void NotesApp::loadStore() {
    for (int i = 0; i < kMaxNotes; ++i) {
        slots_[i] = Slot{};
        titles_[i][0] = '\0';
    }
    if (!loadIndex()) {
        migrateLegacy();
        saveIndex();
    }
    for (int i = 0; i < kMaxNotes; ++i) {
        if (slots_[i].used) {
            refreshTitle(i);
        }
    }
}

bool NotesApp::loadIndex() {
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
    for (int i = 0; i < kMaxNotes; ++i) {
        slots_[i].used = buffer[4 + i] != 0;
        std::memcpy(&slots_[i].mtime, buffer + 4 + kMaxNotes + i * 4, 4);
    }
    return true;
}

bool NotesApp::saveIndex() {
    if (context_ == nullptr) {
        return false;
    }
    char buffer[kIndexBytes] = {};
    std::memcpy(buffer, kIndexMagic, 4);
    for (int i = 0; i < kMaxNotes; ++i) {
        buffer[4 + i] = slots_[i].used ? 1 : 0;
        std::memcpy(buffer + 4 + kMaxNotes + i * 4, &slots_[i].mtime, 4);
    }
    return context_->storage().writeFileAtomic(kIndexPath, buffer, sizeof(buffer));
}

void NotesApp::migrateLegacy() {
    if (context_ == nullptr) {
        return;
    }
    char legacy[kMaxLength] = {};
    size_t loaded = 0;
    if (!context_->storage().readFile(kLegacyPath, legacy, kMaxLength, loaded) || loaded == 0) {
        return;
    }
    slots_[0].used = true;
    slots_[0].mtime = stamp();
    char path[32] = {};
    slotPath(0, path, sizeof(path));
    if (context_->storage().writeFileAtomic(path, legacy, loaded)) {
        refreshTitle(0);
        context_->storage().removeFile(kLegacyPath);
    } else {
        slots_[0] = Slot{};
    }
}

void NotesApp::refreshTitle(int slot) {
    titles_[slot][0] = '\0';
    if (context_ == nullptr || slot < 0 || slot >= kMaxNotes || !slots_[slot].used) {
        return;
    }
    char path[32] = {};
    slotPath(slot, path, sizeof(path));
    char document[kMaxLength] = {};
    size_t loaded = 0;
    if (!context_->storage().readFile(path, document, kMaxLength, loaded)) {
        return;
    }
    fillTitle(document, loaded, titles_[slot], sizeof(titles_[slot]));
}

void NotesApp::fillTitle(const char* document, size_t length, char* out, size_t out_size) const {
    if (out == nullptr || out_size == 0) {
        return;
    }
    out[0] = '\0';
    if (document == nullptr) {
        return;
    }
    size_t end = 0;
    while (end < length && document[end] != '\n') {
        ++end;
    }
    if (end == 0) {
        std::snprintf(out, out_size, "Untitled");
        return;
    }
    if (end + 1 > out_size) {
        end = out_size - 1;
    }
    std::memcpy(out, document, end);
    out[end] = '\0';
}

void NotesApp::slotPath(int slot, char* out, size_t out_size) const {
    std::snprintf(out, out_size, "/apps/notes/%02d.txt", slot);
}

int NotesApp::noteCount() const {
    int count = 0;
    for (int i = 0; i < kMaxNotes; ++i) {
        if (slots_[i].used) {
            ++count;
        }
    }
    return count;
}

void NotesApp::collectOrdered(int* ordered) const {
    int count = 0;
    for (int i = 0; i < kMaxNotes; ++i) {
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

int NotesApp::allocateSlot() const {
    for (int i = 0; i < kMaxNotes; ++i) {
        if (!slots_[i].used) {
            return i;
        }
    }
    return -1;
}

uint32_t NotesApp::stamp() const {
    if (context_ == nullptr) {
        return next_stamp_;
    }
    const int64_t unix = context_->clock().unixUtc();
    if (unix > 0) {
        return static_cast<uint32_t>(unix);
    }
    const uint32_t ms = context_->clock().millis();
    if (ms > 0) {
        return ms;
    }
    return next_stamp_;
}

void NotesApp::openSelected() {
    const int count = noteCount();
    if (list_selected_ >= count) {
        if (count >= kMaxNotes) {
            context_->requestRedraw();
            return;
        }
        editing_slot_ = -1;
        length_ = 0;
        std::memset(document_, 0, sizeof(document_));
    } else {
        int ordered[kMaxNotes] = {};
        collectOrdered(ordered);
        editing_slot_ = ordered[list_selected_];
        char path[32] = {};
        slotPath(editing_slot_, path, sizeof(path));
        length_ = 0;
        std::memset(document_, 0, sizeof(document_));
        size_t loaded = 0;
        if (context_->storage().readFile(path, document_, kMaxLength, loaded)) {
            length_ = loaded;
            if (length_ > kMaxLength) {
                length_ = kMaxLength;
            }
        }
    }
    cursor_ = length_;
    scroll_line_ = 0;
    scroll_col_ = 0;
    dirty_ = false;
    save_failed_ = false;
    last_edit_ms_ = 0;
    screen_ = Screen::Editor;
    ensureCursorVisible();
    context_->requestRedraw();
}

void NotesApp::leaveEditor() {
    if (length_ == 0) {
        if (editing_slot_ >= 0) {
            deleteSlot(editing_slot_);
        }
        dirty_ = false;
        save_failed_ = false;
        screen_ = Screen::List;
        editing_slot_ = -1;
        if (list_selected_ > noteCount()) {
            list_selected_ = noteCount();
        }
        if (context_ != nullptr) {
            context_->requestRedraw();
        }
        return;
    }
    if (dirty_) {
        saveDocument();
    }
    screen_ = Screen::List;
    if (editing_slot_ >= 0) {
        int ordered[kMaxNotes] = {};
        collectOrdered(ordered);
        const int count = noteCount();
        list_selected_ = 0;
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

void NotesApp::confirmDelete() {
    if (delete_slot_ >= 0) {
        deleteSlot(delete_slot_);
    }
    delete_slot_ = -1;
    screen_ = Screen::List;
    if (list_selected_ > noteCount()) {
        list_selected_ = noteCount();
    }
    if (list_scroll_ > list_selected_) {
        list_scroll_ = list_selected_;
    }
    context_->requestRedraw();
}

void NotesApp::deleteSlot(int slot) {
    if (slot < 0 || slot >= kMaxNotes) {
        return;
    }
    char path[32] = {};
    slotPath(slot, path, sizeof(path));
    if (context_ != nullptr) {
        context_->storage().removeFile(path);
    }
    slots_[slot] = Slot{};
    titles_[slot][0] = '\0';
    saveIndex();
}

void NotesApp::markEdited() {
    dirty_ = true;
    if (context_ != nullptr) {
        last_edit_ms_ = context_->clock().millis();
        context_->requestRedraw();
    }
}

void NotesApp::insert(const char* text, size_t count) {
    if (text == nullptr || count == 0) {
        return;
    }
    if (length_ + count > kMaxLength) {
        if (context_ != nullptr) {
            context_->requestRedraw();
        }
        return;
    }
    if (cursor_ < length_) {
        std::memmove(document_ + cursor_ + count, document_ + cursor_, length_ - cursor_);
    }
    std::memcpy(document_ + cursor_, text, count);
    length_ += count;
    cursor_ += count;
    markEdited();
    ensureCursorVisible();
}

void NotesApp::deleteBefore() {
    if (cursor_ == 0) {
        return;
    }
    std::memmove(document_ + cursor_ - 1, document_ + cursor_, length_ - cursor_);
    --cursor_;
    --length_;
    markEdited();
    ensureCursorVisible();
}

void NotesApp::moveHorizontal(int delta) {
    if (delta < 0 && cursor_ > 0) {
        --cursor_;
    } else if (delta > 0 && cursor_ < length_) {
        ++cursor_;
    } else {
        return;
    }
    if (context_ != nullptr) {
        context_->requestRedraw();
    }
    ensureCursorVisible();
}

void NotesApp::moveVertical(int delta) {
    const size_t col = columnAt(cursor_);
    size_t target = cursor_;
    if (delta < 0) {
        const size_t start = lineStart(cursor_);
        if (start == 0) {
            return;
        }
        target = lineStart(start - 1);
    } else if (delta > 0) {
        const size_t end = lineEnd(cursor_);
        if (end >= length_) {
            return;
        }
        target = end + 1;
    } else {
        return;
    }

    const size_t target_end = lineEnd(target);
    const size_t target_start = lineStart(target);
    cursor_ = target_start + col;
    if (cursor_ > target_end) {
        cursor_ = target_end;
    }
    if (context_ != nullptr) {
        context_->requestRedraw();
    }
    ensureCursorVisible();
}

void NotesApp::saveDocument() {
    if (context_ == nullptr) {
        return;
    }
    int slot = editing_slot_;
    if (slot < 0) {
        slot = allocateSlot();
        if (slot < 0) {
            save_failed_ = true;
            context_->diagnostics().emit("ERROR", "notes save failed");
            context_->requestRedraw();
            return;
        }
    }
    char path[32] = {};
    slotPath(slot, path, sizeof(path));
    if (!context_->storage().writeFileAtomic(path, document_, length_)) {
        save_failed_ = true;
        context_->diagnostics().emit("ERROR", "notes save failed");
        context_->requestRedraw();
        return;
    }
    slots_[slot].used = true;
    slots_[slot].mtime = stamp();
    if (slots_[slot].mtime < next_stamp_) {
        slots_[slot].mtime = next_stamp_;
    }
    ++next_stamp_;
    if (slots_[slot].mtime >= next_stamp_) {
        next_stamp_ = slots_[slot].mtime + 1;
    }
    if (!saveIndex()) {
        save_failed_ = true;
        context_->diagnostics().emit("ERROR", "notes save failed");
        context_->requestRedraw();
        return;
    }
    editing_slot_ = slot;
    refreshTitle(slot);
    save_failed_ = false;
    dirty_ = false;
    context_->requestRedraw();
}

void NotesApp::autosaveIfDue() {
    if (!dirty_ || context_ == nullptr || length_ == 0) {
        return;
    }
    if (context_->clock().millis() - last_edit_ms_ < kAutosaveDelayMs) {
        return;
    }
    saveDocument();
}

void NotesApp::ensureCursorVisible() {
    const int line = lineIndex(cursor_);
    const int col = static_cast<int>(columnAt(cursor_));
    const int view_h = layout::kContentBoth.h - 4;
    if (line < scroll_line_) {
        scroll_line_ = line;
    }
    while (scroll_line_ < line) {
        int height = 0;
        for (int i = scroll_line_; i <= line; ++i) {
            height += lineHeight(i);
        }
        if (height <= view_h) {
            break;
        }
        ++scroll_line_;
    }
    const int max_cols = maxColsForLine(line);
    if (col < scroll_col_) {
        scroll_col_ = col;
    } else if (col >= scroll_col_ + max_cols) {
        scroll_col_ = col - max_cols + 1;
    }
    if (scroll_line_ < 0) {
        scroll_line_ = 0;
    }
    if (scroll_col_ < 0) {
        scroll_col_ = 0;
    }
}

int NotesApp::lineHeight(int line) const { return line == 0 ? kTitleLineHeight : kBodyLineHeight; }

int NotesApp::maxColsForLine(int line) const { return line == 0 ? kMaxTitleCols : kMaxBodyCols; }

int NotesApp::glyphWidthForLine(int line) const {
    return line == 0 ? font::kGlyphWidth * 2 : font::kGlyphWidth;
}

size_t NotesApp::lineStart(size_t index) const {
    if (index > length_) {
        index = length_;
    }
    while (index > 0 && document_[index - 1] != '\n') {
        --index;
    }
    return index;
}

size_t NotesApp::lineEnd(size_t index) const {
    if (index > length_) {
        index = length_;
    }
    while (index < length_ && document_[index] != '\n') {
        ++index;
    }
    return index;
}

size_t NotesApp::columnAt(size_t index) const { return index - lineStart(index); }

int NotesApp::lineIndex(size_t index) const {
    int line = 0;
    const size_t limit = index > length_ ? length_ : index;
    for (size_t i = 0; i < limit; ++i) {
        if (document_[i] == '\n') {
            ++line;
        }
    }
    return line;
}

void NotesApp::updateList(const InputFrame& input) {
    const int count = noteCount();
    const int rows = count + 1;
    if (input.action == InputAction::Up && list_selected_ > 0) {
        --list_selected_;
        if (list_selected_ < count && list_selected_ < list_scroll_) {
            list_scroll_ = list_selected_;
        } else if (list_selected_ < count && list_selected_ >= list_scroll_ + kNoteVisible) {
            list_scroll_ = list_selected_ - kNoteVisible + 1;
        }
        context_->requestRedraw();
        return;
    }
    if (input.action == InputAction::Down && list_selected_ + 1 < rows) {
        ++list_selected_;
        if (list_selected_ < count && list_selected_ >= list_scroll_ + kNoteVisible) {
            list_scroll_ = list_selected_ - kNoteVisible + 1;
        }
        context_->requestRedraw();
        return;
    }
    if (input.action == InputAction::Confirm) {
        openSelected();
        return;
    }
    if (input.action == InputAction::Delete && list_selected_ < count) {
        int ordered[kMaxNotes] = {};
        collectOrdered(ordered);
        delete_slot_ = ordered[list_selected_];
        screen_ = Screen::DeleteDialog;
        context_->requestRedraw();
    }
}

void NotesApp::updateEditor(const InputFrame& input) {
    if (input.action == InputAction::Back) {
        leaveEditor();
        context_->consumeBack();
        return;
    }
    if (input.action == InputAction::Left) {
        moveHorizontal(-1);
    } else if (input.action == InputAction::Right) {
        moveHorizontal(1);
    } else if (input.action == InputAction::Up) {
        moveVertical(-1);
    } else if (input.action == InputAction::Down) {
        moveVertical(1);
    } else if (input.action == InputAction::Delete) {
        deleteBefore();
    } else if (input.action == InputAction::Confirm) {
        const char newline = '\n';
        insert(&newline, 1);
    }

    if (input.textLength > 0) {
        insert(input.text, input.textLength);
    }

    autosaveIfDue();
}

void NotesApp::updateDeleteDialog(const InputFrame& input) {
    if (input.action == InputAction::Back) {
        screen_ = Screen::List;
        delete_slot_ = -1;
        context_->consumeBack();
        context_->requestRedraw();
        return;
    }
    if (input.action == InputAction::Confirm) {
        confirmDelete();
    }
}

void NotesApp::update(const InputFrame& input) {
    if (context_ == nullptr) {
        return;
    }
    if (screen_ == Screen::List) {
        updateList(input);
        return;
    }
    if (screen_ == Screen::DeleteDialog) {
        updateDeleteDialog(input);
        return;
    }
    updateEditor(input);
}

void NotesApp::drawList() {
    const theme::Palette palette = theme::paletteFor(context_->settings().theme(), accent());
    UiRenderer renderer(context_->display(), palette);
    renderer.beginFrame();
    renderer.clearAppCanvas();
    drawStandardHeader(*context_, renderer, name());

    const int count = noteCount();
    int ordered[kMaxNotes] = {};
    collectOrdered(ordered);
    if (list_scroll_ < 0) {
        list_scroll_ = 0;
    }
    if (count > kNoteVisible && list_scroll_ + kNoteVisible > count) {
        list_scroll_ = count - kNoteVisible;
    }
    if (count <= kNoteVisible) {
        list_scroll_ = 0;
    }

    const int content_y = layout::kContentBoth.y + 2;
    const int content_h = layout::kContentBoth.h - 4;
    const int list_x = layout::kChromeInset;
    const int list_w = layout::kWidth - 2 * layout::kChromeInset;
    const int new_y = content_y + content_h - kRowHeight;
    const bool overflow = count > kNoteVisible;
    const int visible = count < kNoteVisible ? count : kNoteVisible;
    if (count > 0 && visible > 0) {
        const int inner_h = visible * kRowHeight + (visible - 1) * kRowGap;
        const int outer_h = inner_h + 2 * kListPad;
        const Rect outer{list_x, content_y, list_w, outer_h};
        renderer.surface().fillRoundRect(outer, layout::kCardRadius, palette.card);
        const int inner_x = list_x + kListPad;
        const int inner_w =
            list_w - 2 * kListPad - (overflow ? kScrollbarWidth + 2 : 0);
        if (overflow) {
            drawOverflowScrollbar(renderer.surface(), palette,
                                  {list_x + list_w - kListPad - kScrollbarWidth,
                                   content_y + kListPad, kScrollbarWidth, inner_h},
                                  count, list_scroll_, kNoteVisible);
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
            const Rect title_chip{inner_x + index_w + kRowGap, row_y,
                                  inner_w - index_w - kRowGap, kRowHeight};
            const Color chip_fill = selected ? palette.accent : palette.canvas;
            const Color chip_text = selected ? palette.canvas : palette.primary_text;
            renderer.surface().fillRoundRect(index_chip, layout::kCardRadius, chip_fill);
            renderer.surface().fillRoundRect(title_chip, layout::kCardRadius, chip_fill);
            renderer.surface().drawText(
                {index_chip.x + (index_chip.w - font::textWidth(number, 1)) / 2,
                 centeredTextY(index_chip.y, index_chip.h)},
                {chip_text, 1}, number);
            char label[48] = {};
            ellipsizeToWidth(titles_[slot][0] != '\0' ? titles_[slot] : "Untitled", label,
                             sizeof(label), title_chip.w - 8);
            renderer.surface().drawText({title_chip.x + 4, centeredTextY(title_chip.y, title_chip.h)},
                                        {chip_text, 1}, label);
            row_y += kRowHeight + kRowGap;
        }
    }

    const Rect new_row{list_x, new_y, list_w, kRowHeight};
    const bool new_selected = list_selected_ == count;
    if (new_selected) {
        renderer.surface().fillRoundRect(new_row, layout::kCardRadius, palette.accent);
    } else {
        renderer.surface().fillRoundRect(new_row, layout::kCardRadius, palette.card);
    }
    renderer.surface().drawText({new_row.x + 4, centeredTextY(new_row.y, new_row.h)},
                                {new_selected ? palette.canvas : palette.primary_text, 1},
                                "New note");

    if (count >= kMaxNotes && new_selected) {
        const KeyHint status[] = {{nullptr, "FULL"}, {"Esc", "back"}};
        drawStandardFooter(renderer, status, 2);
    } else if (new_selected) {
        const KeyHint hints[] = {{"Ent", "new"}, {"Esc", "back"}};
        drawStandardFooter(renderer, hints, 2);
    } else {
        const KeyHint hints[] = {{"Ent", "open"}, {"Del", "delete"}, {"Esc", "back"}};
        drawStandardFooter(renderer, hints, 3);
    }
    renderer.endFrame();
}

void NotesApp::drawEditor() {
    const theme::Palette palette = theme::paletteFor(context_->settings().theme(), accent());
    UiRenderer renderer(context_->display(), palette);
    renderer.beginFrame();
    renderer.clearAppCanvas();
    drawStandardHeader(*context_, renderer, name());

    const int text_y0 = layout::kContentBoth.y + 2;
    const int view_h = layout::kContentBoth.h - 4;
    int line = 0;
    size_t index = 0;
    int y = text_y0;
    while (index <= length_) {
        const size_t end = lineEnd(index);
        if (line >= scroll_line_) {
            if (y + lineHeight(line) > text_y0 + view_h) {
                break;
            }
            const int scale = line == 0 ? 2 : 1;
            const int max_cols = maxColsForLine(line);
            const int glyph_w = glyphWidthForLine(line);
            const size_t line_len = end - index;
            const size_t start_col = static_cast<size_t>(scroll_col_);
            char visible[48] = {};
            size_t out = 0;
            if (start_col < line_len) {
                out = line_len - start_col;
                if (out > static_cast<size_t>(max_cols)) {
                    out = static_cast<size_t>(max_cols);
                }
                if (out > sizeof(visible) - 1) {
                    out = sizeof(visible) - 1;
                }
                std::memcpy(visible, document_ + index + start_col, out);
            }
            renderer.surface().drawText({kTextX, y}, {palette.primary_text, scale}, visible);

            if (cursor_ >= index && cursor_ <= end) {
                const int cursor_col = static_cast<int>(cursor_ - index) - scroll_col_;
                if (cursor_col >= 0 && cursor_col <= max_cols) {
                    const int cx = kTextX + cursor_col * glyph_w;
                    renderer.surface().fillRect({cx, y, 1, font::kGlyphHeight * scale},
                                                palette.primary_text);
                }
            }
            y += lineHeight(line);
        }
        if (end >= length_) {
            break;
        }
        index = end + 1;
        ++line;
    }

    if (save_failed_) {
        const KeyHint status[] = {{nullptr, "SAVE FAIL"}};
        drawStandardFooter(renderer, status, 1);
    } else if (length_ >= kMaxLength) {
        const KeyHint status[] = {{nullptr, "FULL"}};
        drawStandardFooter(renderer, status, 1);
    } else {
        const KeyHint hints[] = {{"Ent", "line"}, {"Del", "bk"}, {"Esc", "back"}};
        drawStandardFooter(renderer, hints, 3);
    }
    renderer.endFrame();
}

void NotesApp::drawDeleteDialog() {
    const theme::Palette palette = theme::paletteFor(context_->settings().theme(), accent());
    UiRenderer renderer(context_->display(), palette);
    renderer.beginFrame();
    renderer.clearAppCanvas();
    drawStandardHeader(*context_, renderer, name());
    const char* title =
        (delete_slot_ >= 0 && titles_[delete_slot_][0] != '\0') ? titles_[delete_slot_] : "Untitled";
    drawDialog(renderer.surface(), palette, "Delete?", title);
    const KeyHint hints[] = {{"Ent", "delete"}, {"Esc", "cancel"}};
    drawStandardFooter(renderer, hints, 2);
    renderer.endFrame();
}

void NotesApp::draw() {
    if (context_ == nullptr) {
        return;
    }
    if (screen_ == Screen::List) {
        drawList();
        return;
    }
    if (screen_ == Screen::DeleteDialog) {
        drawDeleteDialog();
        return;
    }
    drawEditor();
}

}  // namespace luma
