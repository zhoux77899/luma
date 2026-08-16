#pragma once

namespace luma {

class Audio {
public:
    virtual ~Audio() = default;
    virtual void begin() {}
    virtual void play(const char* event) = 0;
};

}  // namespace luma
