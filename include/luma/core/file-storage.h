#pragma once

#include "luma/core/storage.h"

#include <string>

namespace luma {

class FileStorage : public Storage {
public:
    explicit FileStorage(const char* root);

    bool begin() override;
    bool loadPref(const char* key, void* data, size_t size) override;
    bool savePref(const char* key, const void* data, size_t size) override;
    bool readFile(const char* path, char* buffer, size_t capacity, size_t& length) override;
    bool writeFileAtomic(const char* path, const char* data, size_t length) override;
    bool removeFile(const char* path) override;
    void processDeferredSaves() override;

private:
    std::string join(const char* relative) const;
    bool ensureDirectory(const char* path) const;
    bool writeAll(const char* path, const char* data, size_t length) const;
    bool replaceFile(const char* from, const char* to) const;

    std::string root_;
};

class HostStorageAdapter : public FileStorage {
public:
    explicit HostStorageAdapter(const char* root) : FileStorage(root) {}
};

}  // namespace luma
