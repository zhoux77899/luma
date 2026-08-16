#include "luma/core/in-memory-storage.h"

#include <cstring>

namespace luma {

bool InMemoryStorage::begin() { return true; }

bool InMemoryStorage::loadPref(const char* key, void* data, size_t size) {
    const auto it = prefs_.find(key);
    if (it == prefs_.end() || it->second.size() != size) {
        return false;
    }
    std::memcpy(data, it->second.data(), size);
    return true;
}

bool InMemoryStorage::savePref(const char* key, const void* data, size_t size) {
    prefs_[key] = std::string(static_cast<const char*>(data), size);
    return true;
}

bool InMemoryStorage::readFile(const char* path, char* buffer, size_t capacity, size_t& length) {
    const auto it = files_.find(path);
    if (it == files_.end()) {
        return false;
    }
    length = it->second.size();
    if (length > capacity) {
        return false;
    }
    std::memcpy(buffer, it->second.data(), length);
    return true;
}

bool InMemoryStorage::writeFileAtomic(const char* path, const char* data, size_t length) {
    files_[path] = std::string(data, length);
    return true;
}

void InMemoryStorage::processDeferredSaves() {}

}  // namespace luma
