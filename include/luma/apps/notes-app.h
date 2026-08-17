#pragma once

#include "luma/core/app.h"

#include <cstddef>
#include <cstdint>

namespace luma {

class NotesApp : public App {
public:
    static constexpr size_t kMaxLength = 1024;
    static constexpr const char* kDocumentPath = "/apps/notes/notes.txt";

    const char* id() const override;
    const char* name() const override;
    Color accent() const override;

    void onEnter(AppContext& context) override;
    void onExit() override;
    void update(const InputFrame& input) override;
    void draw() override;

private:
    void markEdited();
    void insert(const char* text, size_t count);
    void deleteBefore();
    void moveHorizontal(int delta);
    void moveVertical(int delta);
    void saveDocument();
    void autosaveIfDue();
    void ensureCursorVisible();
    size_t lineStart(size_t index) const;
    size_t lineEnd(size_t index) const;
    size_t columnAt(size_t index) const;
    int lineIndex(size_t index) const;

    AppContext* context_ = nullptr;
    char document_[kMaxLength]{};
    size_t length_ = 0;
    size_t cursor_ = 0;
    int scroll_line_ = 0;
    int scroll_col_ = 0;
    bool dirty_ = false;
    bool save_failed_ = false;
    uint32_t last_edit_ms_ = 0;
};

}  // namespace luma
