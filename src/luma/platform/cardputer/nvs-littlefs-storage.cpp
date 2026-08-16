#include "nvs-littlefs-storage.h"

#include "luma/core/diagnostics.h"

#include <LittleFS.h>

#include <cstring>

namespace luma {
namespace {

String siblingTmp(const char* path) {
    String tmp(path);
    tmp += ".tmp";
    return tmp;
}

String siblingBak(const char* path) {
    String bak(path);
    bak += ".bak";
    return bak;
}

}  // namespace

NvsLittleFsStorage::NvsLittleFsStorage(Diagnostics& diagnostics) : diagnostics_(diagnostics) {}

void NvsLittleFsStorage::emitError(const char* message) { diagnostics_.emit("ERROR", message); }

bool NvsLittleFsStorage::ensureParentDirectories(const char* path) {
    if (path == nullptr) {
        return false;
    }

    String current;
    for (size_t i = 0; path[i] != '\0'; ++i) {
        current += path[i];
        if (path[i] == '/' && current.length() > 1) {
            String directory = current.substring(0, current.length() - 1);
            if (directory.length() > 0 && !LittleFS.exists(directory)) {
                if (!LittleFS.mkdir(directory)) {
                    return false;
                }
            }
        }
    }
    return true;
}

bool NvsLittleFsStorage::begin() {
    prefs_ready_ = prefs_.begin("luma", false);
    if (!prefs_ready_) {
        emitError("nvs begin failed");
    }

    fs_ready_ = LittleFS.begin(true);
    if (!fs_ready_) {
        emitError("littlefs mount failed");
        return false;
    }

    if (!LittleFS.exists("/apps") && !LittleFS.mkdir("/apps")) {
        emitError("mkdir /apps failed");
        return false;
    }
    if (!LittleFS.exists("/apps/notes") && !LittleFS.mkdir("/apps/notes")) {
        emitError("mkdir /apps/notes failed");
        return false;
    }

    diagnostics_.emit("STORAGE", "ready");
    return true;
}

bool NvsLittleFsStorage::loadPref(const char* key, void* data, size_t size) {
    if (!prefs_ready_ || key == nullptr || data == nullptr) {
        return false;
    }
    if (!prefs_.isKey(key) || prefs_.getBytesLength(key) != size) {
        return false;
    }
    return prefs_.getBytes(key, data, size) == size;
}

bool NvsLittleFsStorage::savePref(const char* key, const void* data, size_t size) {
    if (!prefs_ready_ || key == nullptr || data == nullptr) {
        return false;
    }
    return prefs_.putBytes(key, data, size) == size;
}

bool NvsLittleFsStorage::readFile(const char* path, char* buffer, size_t capacity, size_t& length) {
    if (!fs_ready_ || path == nullptr || buffer == nullptr) {
        return false;
    }

    File file = LittleFS.open(path, "r");
    if (!file) {
        return false;
    }

    const size_t file_size = file.size();
    if (file_size > capacity) {
        file.close();
        return false;
    }

    length = file.read(reinterpret_cast<uint8_t*>(buffer), file_size);
    file.close();
    return length == file_size;
}

bool NvsLittleFsStorage::writeFileAtomic(const char* path, const char* data, size_t length) {
    if (!fs_ready_ || path == nullptr || (length > 0 && data == nullptr)) {
        return false;
    }
    if (!ensureParentDirectories(path)) {
        emitError("storage mkdir failed");
        return false;
    }

    const String tmp = siblingTmp(path);
    const String bak = siblingBak(path);
    LittleFS.remove(tmp);

    File file = LittleFS.open(tmp, "w");
    if (!file) {
        emitError("storage tmp open failed");
        return false;
    }
    const size_t written = length == 0 ? 0 : file.write(reinterpret_cast<const uint8_t*>(data), length);
    file.close();
    if (written != length) {
        LittleFS.remove(tmp);
        emitError("storage tmp write failed");
        return false;
    }

    if (LittleFS.exists(path)) {
        LittleFS.remove(bak);
        if (!LittleFS.rename(path, bak)) {
            LittleFS.remove(tmp);
            emitError("storage backup failed");
            return false;
        }
    }

    if (!LittleFS.rename(tmp, path)) {
        if (LittleFS.exists(bak)) {
            LittleFS.rename(bak, path);
        }
        LittleFS.remove(tmp);
        emitError("storage replace failed");
        return false;
    }

    LittleFS.remove(bak);
    diagnostics_.emit("STORAGE", "saved");
    return true;
}

void NvsLittleFsStorage::processDeferredSaves() {}

}  // namespace luma
