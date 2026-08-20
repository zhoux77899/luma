#pragma once

#include "luma/core/app.h"

#include <cstddef>
#include <cstdint>

namespace luma {

class NotesApp : public App {
public:
    static constexpr size_t kMaxLength = 1024;
    static constexpr int kMaxNotes = 16;
    static constexpr const char* kIndexPath = "/apps/notes/index";
    static constexpr const char* kLegacyPath = "/apps/notes/notes.txt";

    const char* id() const override;
    const char* name() const override;
    Color accent() const override;

    void onEnter(AppContext& context) override;
    void onExit() override;
    void update(const InputFrame& input) override;
    void draw() override;

private:
    enum class Screen : int { List, Editor, DeleteDialog };

    struct Slot {
        bool used = false;
        uint32_t mtime = 0;
    };

    void loadStore();
    bool loadIndex();
    bool saveIndex();
    void migrateLegacy();
    void refreshTitle(int slot);
    void fillTitle(const char* document, size_t length, char* out, size_t out_size) const;
    void slotPath(int slot, char* out, size_t out_size) const;
    int noteCount() const;
    void collectOrdered(int* ordered) const;
    int allocateSlot() const;
    uint32_t stamp() const;
    void openSelected();
    void leaveEditor();
    void confirmDelete();
    void deleteSlot(int slot);
    void markEdited();
    void insert(const char* text, size_t count);
    void deleteBefore();
    void moveHorizontal(int delta);
    void moveVertical(int delta);
    void saveDocument();
    void autosaveIfDue();
    void ensureCursorVisible();
    int lineHeight(int line) const;
    int maxColsForLine(int line) const;
    int glyphWidthForLine(int line) const;
    size_t lineStart(size_t index) const;
    size_t lineEnd(size_t index) const;
    size_t columnAt(size_t index) const;
    int lineIndex(size_t index) const;
    void updateList(const InputFrame& input);
    void updateEditor(const InputFrame& input);
    void updateDeleteDialog(const InputFrame& input);
    void drawList();
    void drawEditor();
    void drawDeleteDialog();

    AppContext* context_ = nullptr;
    Screen screen_ = Screen::List;
    Slot slots_[kMaxNotes]{};
    char titles_[kMaxNotes][48]{};
    int list_selected_ = 0;
    int list_scroll_ = 0;
    int editing_slot_ = -1;
    int delete_slot_ = -1;
    uint32_t next_stamp_ = 1;
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
