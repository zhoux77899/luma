#include "luma/core/file-storage.h"

#include <cerrno>
#include <cstdio>
#include <cstring>
#include <string>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <direct.h>
#include <io.h>
#include <windows.h>
#else
#include <sys/stat.h>
#include <unistd.h>
#endif

namespace luma {
namespace {

bool isSeparator(char character) { return character == '/' || character == '\\'; }

std::string parentPath(const std::string& path) {
    const auto pos = path.find_last_of("/\\");
    if (pos == std::string::npos) {
        return {};
    }
    return path.substr(0, pos);
}

bool existsPath(const char* path) {
#ifdef _WIN32
    return _access(path, 0) == 0;
#else
    return access(path, F_OK) == 0;
#endif
}

bool isDirectory(const char* path) {
#ifdef _WIN32
    const DWORD attributes = GetFileAttributesA(path);
    return attributes != INVALID_FILE_ATTRIBUTES && (attributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
#else
    struct stat info {};
    return stat(path, &info) == 0 && S_ISDIR(info.st_mode);
#endif
}

bool makeDirectory(const char* path) {
#ifdef _WIN32
    return _mkdir(path) == 0 || errno == EEXIST;
#else
    return mkdir(path, 0755) == 0 || errno == EEXIST;
#endif
}

}  // namespace

FileStorage::FileStorage(const char* root) : root_(root == nullptr ? "" : root) {}

std::string FileStorage::join(const char* relative) const {
    std::string path = root_;
    if (path.empty()) {
        path = ".";
    }
    if (relative == nullptr || relative[0] == '\0') {
        return path;
    }
    if (!path.empty() && !isSeparator(path.back())) {
        path.push_back('/');
    }
    if (isSeparator(relative[0])) {
        ++relative;
    }
    path.append(relative);
    return path;
}

bool FileStorage::ensureDirectory(const char* path) const {
    if (path == nullptr || path[0] == '\0') {
        return false;
    }

    std::string current;
    for (size_t i = 0; path[i] != '\0'; ++i) {
        current.push_back(path[i]);
        if (isSeparator(path[i]) || path[i + 1] == '\0') {
            if (current.empty() || current == "." || current == ".." ||
                (current.size() == 2 && current[1] == ':')) {
                continue;
            }
            if (existsPath(current.c_str()) && !isDirectory(current.c_str())) {
                return false;
            }
            if (!existsPath(current.c_str()) && !makeDirectory(current.c_str())) {
                return false;
            }
        }
    }
    return true;
}

bool FileStorage::writeAll(const char* path, const char* data, size_t length) const {
    FILE* file = std::fopen(path, "wb");
    if (file == nullptr) {
        return false;
    }
    const size_t written = length == 0 ? 0 : std::fwrite(data, 1, length, file);
    const int closed = std::fclose(file);
    return written == length && closed == 0;
}

bool FileStorage::replaceFile(const char* from, const char* to) const {
#ifdef _WIN32
    return MoveFileExA(from, to, MOVEFILE_REPLACE_EXISTING) != 0;
#else
    return std::rename(from, to) == 0;
#endif
}

bool FileStorage::begin() {
    if (!ensureDirectory(root_.c_str())) {
        return false;
    }
    if (!ensureDirectory(join("prefs").c_str())) {
        return false;
    }
    if (!ensureDirectory(join("apps/notes").c_str())) {
        return false;
    }
    return true;
}

bool FileStorage::loadPref(const char* key, void* data, size_t size) {
    if (key == nullptr || data == nullptr) {
        return false;
    }
    const std::string path = join((std::string("prefs/") + key).c_str());
    FILE* file = std::fopen(path.c_str(), "rb");
    if (file == nullptr) {
        return false;
    }
    if (std::fseek(file, 0, SEEK_END) != 0) {
        std::fclose(file);
        return false;
    }
    const long length = std::ftell(file);
    if (length < 0 || static_cast<size_t>(length) != size) {
        std::fclose(file);
        return false;
    }
    std::rewind(file);
    const size_t read = std::fread(data, 1, size, file);
    std::fclose(file);
    return read == size;
}

bool FileStorage::savePref(const char* key, const void* data, size_t size) {
    if (key == nullptr || data == nullptr) {
        return false;
    }
    if (!ensureDirectory(join("prefs").c_str())) {
        return false;
    }
    const std::string path = join((std::string("prefs/") + key).c_str());
    return writeAll(path.c_str(), static_cast<const char*>(data), size);
}

bool FileStorage::readFile(const char* path, char* buffer, size_t capacity, size_t& length) {
    if (path == nullptr || buffer == nullptr) {
        return false;
    }
    const std::string full = join(path);
    FILE* file = std::fopen(full.c_str(), "rb");
    if (file == nullptr) {
        return false;
    }
    if (std::fseek(file, 0, SEEK_END) != 0) {
        std::fclose(file);
        return false;
    }
    const long file_length = std::ftell(file);
    if (file_length < 0 || static_cast<size_t>(file_length) > capacity) {
        std::fclose(file);
        return false;
    }
    std::rewind(file);
    length = static_cast<size_t>(file_length);
    const size_t read = length == 0 ? 0 : std::fread(buffer, 1, length, file);
    std::fclose(file);
    return read == length;
}

bool FileStorage::writeFileAtomic(const char* path, const char* data, size_t length) {
    if (path == nullptr || (length > 0 && data == nullptr)) {
        return false;
    }

    const std::string full = join(path);
    const std::string parent = parentPath(full);
    if (!parent.empty() && !ensureDirectory(parent.c_str())) {
        return false;
    }

    const std::string tmp = full + ".tmp";
    if (existsPath(tmp.c_str()) && isDirectory(tmp.c_str())) {
        return false;
    }
    if (!writeAll(tmp.c_str(), data, length)) {
        return false;
    }
    if (!replaceFile(tmp.c_str(), full.c_str())) {
        std::remove(tmp.c_str());
        return false;
    }
    return true;
}

void FileStorage::processDeferredSaves() {}

}  // namespace luma
