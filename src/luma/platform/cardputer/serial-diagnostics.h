#pragma once

#include "luma/core/diagnostics.h"

namespace luma {

class SerialDiagnostics : public Diagnostics {
public:
    void emit(const char* prefix, const char* message) override;
};

}  // namespace luma
