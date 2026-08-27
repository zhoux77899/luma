#pragma once

#include "luma/core/app.h"

#include <cstddef>
#include <cstdint>

namespace luma {

class DotsApp : public App {
public:
    static constexpr int kCellsX = 60;
    static constexpr int kCellsY = 30;
    static constexpr int kCellPixels = 4;
    static constexpr int kMaxMatrices = 16;
    static constexpr size_t kNameBytes = 32;
    static constexpr size_t kCellCount = static_cast<size_t>(kCellsX * kCellsY);
    static constexpr const char* kIndexPath = "/apps/dots/index";

    const char* id() const override;
    const char* name() const override;
    Color accent() const override;

    void onEnter(AppContext& context) override;
    void onExit() override;
    void update(const InputFrame& input) override;
    void draw() override;

private:
    enum class Screen : int {
        List,
        Paint,
        Picker,
        NamePrompt,
        ClearDialog,
        RenameEditor,
        DeleteDialog
    };

    struct Slot {
        bool used = false;
        uint32_t mtime = 0;
        char name[kNameBytes] = {};
    };

    void loadStore();
    bool loadIndex();
    bool saveIndex();
    void slotPath(int slot, char* out, size_t out_size) const;
    int matrixCount() const;
    void collectOrdered(int* ordered) const;
    int allocateSlot() const;
    uint32_t stamp() const;
    bool hasLamps() const;
    void clearCells();
    void openNew();
    void openSelected();
    void leavePaint();
    void finishLeavePaint();
    void confirmDelete();
    void deleteSlot(int slot);
    void markEdited();
    void saveDocument();
    void autosaveIfDue();
    void tickChrome();
    bool nameTaken(const char* candidate, int ignore_slot) const;
    void assignUniqueUntitled(char* out, size_t out_size) const;
    bool tryCommitName(const char* typed, int slot);
    void beginNameEditor(Screen screen, const char* seed);
    void appendNameText(const InputFrame& input);
    void updateList(const InputFrame& input);
    void updatePaint(const InputFrame& input);
    void updatePicker(const InputFrame& input);
    void updateNameEditor(const InputFrame& input);
    void updateClearDialog(const InputFrame& input);
    void updateDeleteDialog(const InputFrame& input);
    void drawList();
    void drawPaint();
    void drawPicker();
    void drawNameEditor();
    void drawClearDialog();
    void drawDeleteDialog();

    AppContext* context_ = nullptr;
    Screen screen_ = Screen::List;
    Slot slots_[kMaxMatrices]{};
    int list_selected_ = 0;
    int list_scroll_ = 0;
    int editing_slot_ = -1;
    int delete_slot_ = -1;
    uint32_t next_stamp_ = 1;
    uint8_t cells_[kCellCount] = {};
    int cursor_x_ = 0;
    int cursor_y_ = 0;
    uint8_t pen_index_ = 1;
    uint8_t picker_index_ = 0;
    bool dirty_ = false;
    bool save_failed_ = false;
    bool created_new_ = false;
    uint32_t last_edit_ms_ = 0;
    uint32_t last_footer_page_ = 0;
    char name_field_[kNameBytes] = {};
};

}  // namespace luma
