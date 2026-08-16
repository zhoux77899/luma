#include "luma/apps/notes-app.h"

#include "luma/core/app-context.h"
#include "luma/core/clock.h"
#include "luma/core/diagnostics.h"
#include "luma/core/display.h"
#include "luma/core/settings.h"
#include "luma/core/storage.h"
#include "luma/ui/app-chrome.h"
#include "luma/ui/font.h"
#include "luma/ui/layout.h"
#include "luma/ui/renderer.h"
#include "luma/ui/theme.h"

#include <cstring>

namespace luma {
namespace {

constexpr uint32_t kAutosaveDelayMs = 500;
constexpr int kLineHeight = 12;
constexpr int kVisibleLines = layout::kContentBoth.h / kLineHeight;
constexpr int kTextX = layout::kChromeInset;
constexpr int kMaxCols = (layout::kWidth - 2 * layout::kChromeInset) / font::kGlyphWidth;

}  // namespace

const char* NotesApp::id() const { return "notes"; }
const char* NotesApp::name() const { return "NOTES"; }

void NotesApp::onEnter(AppContext& context) {
    context_ = &context;
    length_ = 0;
    cursor_ = 0;
    scroll_line_ = 0;
    scroll_col_ = 0;
    dirty_ = false;
    save_failed_ = false;
    last_edit_ms_ = 0;
    std::memset(document_, 0, sizeof(document_));

    size_t loaded = 0;
    if (context.storage().readFile(kDocumentPath, document_, kMaxLength, loaded)) {
        length_ = loaded;
        if (length_ > kMaxLength) {
            length_ = kMaxLength;
        }
    }
    cursor_ = length_;
    ensureCursorVisible();
}

void NotesApp::onExit() {
    if (dirty_) {
        saveDocument();
    }
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
    if (!context_->storage().writeFileAtomic(kDocumentPath, document_, length_)) {
        save_failed_ = true;
        context_->diagnostics().emit("ERROR", "notes save failed");
        context_->requestRedraw();
        return;
    }
    save_failed_ = false;
    dirty_ = false;
    context_->requestRedraw();
}

void NotesApp::autosaveIfDue() {
    if (!dirty_ || context_ == nullptr) {
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
    if (line < scroll_line_) {
        scroll_line_ = line;
    } else if (line >= scroll_line_ + kVisibleLines) {
        scroll_line_ = line - kVisibleLines + 1;
    }
    if (col < scroll_col_) {
        scroll_col_ = col;
    } else if (col >= scroll_col_ + kMaxCols) {
        scroll_col_ = col - kMaxCols + 1;
    }
    if (scroll_line_ < 0) {
        scroll_line_ = 0;
    }
    if (scroll_col_ < 0) {
        scroll_col_ = 0;
    }
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

void NotesApp::update(const InputFrame& input) {
    if (context_ == nullptr) {
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

void NotesApp::draw() {
    if (context_ == nullptr) {
        return;
    }

    const theme::Palette palette = theme::paletteFor(context_->settings().theme());
    UiRenderer renderer(context_->display(), palette);
    renderer.beginFrame();
    renderer.clearAppCanvas();
    drawStandardHeader(*context_, renderer, name());

    const int text_y0 = layout::kContentBoth.y + 2;
    int line = 0;
    size_t index = 0;
    while (index <= length_ && line < scroll_line_ + kVisibleLines) {
        const size_t end = lineEnd(index);
        if (line >= scroll_line_) {
            const int row = line - scroll_line_;
            const int y = text_y0 + row * kLineHeight;
            const size_t line_len = end - index;
            const size_t start_col = static_cast<size_t>(scroll_col_);
            char visible[48] = {};
            size_t out = 0;
            if (start_col < line_len) {
                out = line_len - start_col;
                if (out > static_cast<size_t>(kMaxCols)) {
                    out = static_cast<size_t>(kMaxCols);
                }
                if (out > sizeof(visible) - 1) {
                    out = sizeof(visible) - 1;
                }
                std::memcpy(visible, document_ + index + start_col, out);
            }
            renderer.surface().drawText({kTextX, y}, {palette.primary_text, 1}, visible);

            if (cursor_ >= index && cursor_ <= end) {
                const int cursor_col = static_cast<int>(cursor_ - index) - scroll_col_;
                if (cursor_col >= 0 && cursor_col <= kMaxCols) {
                    const int cx = kTextX + cursor_col * font::kGlyphWidth;
                    renderer.surface().fillRect({cx, y, 1, font::kGlyphHeight}, palette.primary_text);
                }
            }
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

}  // namespace luma
