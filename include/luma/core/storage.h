#pragma once

#include <cstddef>

namespace luma {

class Storage {
public:
    virtual ~Storage() = default;
    virtual bool begin() = 0;
    virtual bool loadPref(const char* key, void* data, size_t size) = 0;
    virtual bool savePref(const char* key, const void* data, size_t size) = 0;
    virtual bool readFile(const char* path, char* buffer, size_t capacity, size_t& length) = 0;
    virtual bool writeFileAtomic(const char* path, const char* data, size_t length) = 0;
    virtual void processDeferredSaves() = 0;
};

}  // namespace luma
