#include "luma/platform/host/host-diagnostics.h"

#include <cstdio>

namespace luma {

void HostDiagnostics::emit(const char* prefix, const char* message) {
    std::printf("[%s] %s\n", prefix, message);
    std::fflush(stdout);
}

}  // namespace luma
