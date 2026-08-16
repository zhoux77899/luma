#pragma once

namespace luma {

class Diagnostics {
public:
    virtual ~Diagnostics() = default;
    virtual void emit(const char* prefix, const char* message) = 0;
};

}  // namespace luma
