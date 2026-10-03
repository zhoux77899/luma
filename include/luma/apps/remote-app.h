#pragma once

#include "luma/apps/remote-encode.h"
#include "luma/core/app.h"

namespace luma {

class RemoteApp : public App {
public:
    const char* id() const override;
    const char* name() const override;
    Color accent() const override;

    void onEnter(AppContext& context) override;
    void onExit() override;
    void update(const InputFrame& input) override;
    void draw() override;

private:
    enum class Screen : uint8_t { Face, Editor, Picker, DeleteDialog };

    void loadStore();
    void saveStore();
    int userCount() const;
    int cycleCount() const;
    bool currentIsBrand() const;
    int currentBrand() const;
    int currentUserSlot() const;
    void collectUserOrder(int* order) const;
    bool nameTaken(const char* candidate, int ignore) const;
    void assignUnique(const char* base, char* out) const;
    bool createEmpty();
    bool createCopy(int brand);
    void deleteCurrentUser();
    void fire(RemoteKey key);
    void updateFace(const InputFrame& input);
    void updateEditor(const InputFrame& input);
    void updatePicker(const InputFrame& input);
    void updateDelete(const InputFrame& input);
    void drawFace();
    void drawEditor();
    void drawPicker();
    void drawDelete();

    AppContext* context_ = nullptr;
    Screen screen_ = Screen::Face;
    RemoteDocument users_[kMaxUserRemotes]{};
    bool user_used_[kMaxUserRemotes]{};
    int cursor_ = 0;
    int last_key_ = -1;
    bool full_ = false;
    int editor_field_ = 0;
    int editor_scroll_ = 0;
    int picker_ = 0;
    uint32_t next_stamp_ = 1;
};

}  // namespace luma
