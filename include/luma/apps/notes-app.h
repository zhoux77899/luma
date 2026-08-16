#pragma once

#include "luma/core/app.h"

namespace luma {

class NotesApp : public App {
public:
    const char* id() const override;
    const char* name() const override;

    void onEnter(AppContext& context) override;
    void update(const InputFrame& input) override;
    void draw() override;

private:
    AppContext* context_ = nullptr;
};

}  // namespace luma
