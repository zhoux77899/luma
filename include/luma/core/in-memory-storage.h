#pragma once

#include "luma/core/storage.h"

#include <map>
#include <string>

namespace luma {

class InMemoryStorage : public Storage {
public:
    bool begin() override;
    bool loadPref(const char* key, void* data, size_t size) override;
    bool savePref(const char* key, const void* data, size_t size) override;
    bool readFile(const char* path, char* buffer, size_t capacity, size_t& length) override;
    bool writeFileAtomic(const char* path, const char* data, size_t length) override;
    void processDeferredSaves() override;

private:
    std::map<std::string, std::string> prefs_;
    std::map<std::string, std::string> files_;
};

}  // namespace luma
