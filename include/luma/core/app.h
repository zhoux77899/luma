#pragma once

#include "luma/core/display.h"
#include "luma/core/input-frame.h"

namespace luma {

class AppContext;

class App {
public:
    virtual ~App() = default;

    virtual const char* id() const = 0;
    virtual const char* name() const = 0;
    virtual char shortcut() const { return '\0'; }
    virtual Color accent() const;

    virtual void onEnter(AppContext& context) { (void)context; }
    virtual void onExit() {}
    virtual void update(const InputFrame& input) = 0;
    virtual void draw() = 0;
};

}  // namespace luma
