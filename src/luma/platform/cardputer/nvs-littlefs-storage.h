#pragma once

#include "luma/core/storage.h"

#include <Preferences.h>

namespace luma {

class Diagnostics;

class NvsLittleFsStorage : public Storage {
public:
    explicit NvsLittleFsStorage(Diagnostics& diagnostics);

    bool begin() override;
    bool loadPref(const char* key, void* data, size_t size) override;
    bool savePref(const char* key, const void* data, size_t size) override;
    bool readFile(const char* path, char* buffer, size_t capacity, size_t& length) override;
    bool writeFileAtomic(const char* path, const char* data, size_t length) override;
    void processDeferredSaves() override;

private:
    bool ensureParentDirectories(const char* path);
    void emitError(const char* message);

    Diagnostics& diagnostics_;
    Preferences prefs_;
    bool prefs_ready_ = false;
    bool fs_ready_ = false;
};

}  // namespace luma
