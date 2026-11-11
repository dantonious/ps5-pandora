// Console implementation of Platform.
//
// UNVALIDATED. This file has never been executed on hardware. It is the first
// milestone in docs/ROADMAP.md, and the FFI below is written against the
// libkernel symbols exposed by the PS5 Payload SDK. Do not assume any of it
// works until a hardware test report says so.
//
// It is compiled only when PANDORA_PLATFORM_PS5 is defined, which the console
// toolchain file sets. Every unsafe construct is confined to this file.

#include "pandora/platform.hpp"

#if defined(PANDORA_PLATFORM_PS5)

#include <fcntl.h>
#include <unistd.h>

#include <sys/stat.h>
#include <algorithm>
#include <cerrno>
#include <cstddef>
#include <cstring>
#include <filesystem>

extern "C" {
int sceKernelOpen(const char* path, int flags, int mode);
int sceKernelClose(int fd);
long sceKernelRead(int fd, void* buffer, std::size_t length);
long sceKernelWrite(int fd, const void* buffer, std::size_t length);
int sceKernelGetdents(int fd, char* buffer, std::size_t length);
}

namespace pandora {
namespace {

bool fail(std::string* error, std::string message) {
    if (error != nullptr) {
        *error = std::move(message);
    }
    return false;
}

class Ps5Platform final : public Platform {
public:
    Ps5Platform() : root_("/data/pandora") {}

    std::string name() const override { return "ps5"; }

    std::string data_root() const override { return root_; }

    std::vector<DirEntry> list_dir(const std::string& path, std::string* error) const override {
        std::vector<DirEntry> entries;

        const int fd = sceKernelOpen(path.c_str(), O_RDONLY, 0);
        if (fd < 0) {
            fail(error, "cannot open " + path + ": " + std::strerror(errno));
            return entries;
        }

        char buffer[4096];
        for (;;) {
            const int bytes = sceKernelGetdents(fd, buffer, sizeof(buffer));
            if (bytes <= 0) {
                break;
            }

            int offset = 0;
            while (offset < bytes) {
                const char* name = buffer + offset;
                const std::size_t max = static_cast<std::size_t>(bytes - offset);
                std::size_t length = 0;
                while (length < max && name[length] != '\0') {
                    ++length;
                }
                if (length >= max) {
                    break;
                }

                const std::string entry_name(name, length);
                // Records are padded to a four-byte boundary.
                offset += static_cast<int>((length + 1 + 3) & ~3U);

                if (entry_name.empty() || entry_name == "." || entry_name == "..") {
                    continue;
                }

                DirEntry entry;
                entry.name = entry_name;
                entry.path = path + "/" + entry_name;

                struct stat info{};
                if (stat(entry.path.c_str(), &info) == 0) {
                    entry.is_dir = S_ISDIR(info.st_mode) != 0;
                    entry.size = entry.is_dir ? 0 : static_cast<std::uint64_t>(info.st_size);
                }
                entries.push_back(std::move(entry));
            }
        }

        sceKernelClose(fd);

        std::sort(entries.begin(), entries.end(), [](const DirEntry& a, const DirEntry& b) {
            if (a.is_dir != b.is_dir) {
                return a.is_dir;
            }
            return a.name < b.name;
        });
        return entries;
    }

    std::optional<std::string> read_file(const std::string& path,
                                         std::string* error) const override {
        const int fd = sceKernelOpen(path.c_str(), O_RDONLY, 0);
        if (fd < 0) {
            fail(error, "cannot open " + path + ": " + std::strerror(errno));
            return std::nullopt;
        }

        std::string data;
        char chunk[64 * 1024];
        for (;;) {
            const long bytes = sceKernelRead(fd, chunk, sizeof(chunk));
            if (bytes < 0) {
                sceKernelClose(fd);
                fail(error, "read failed for " + path + ": " + std::strerror(errno));
                return std::nullopt;
            }
            if (bytes == 0) {
                break;
            }
            data.append(chunk, static_cast<std::size_t>(bytes));
        }

        sceKernelClose(fd);
        return data;
    }

    bool write_file(const std::string& path, std::string_view data,
                    std::string* error) const override {
        const std::filesystem::path target(path);
        if (target.has_parent_path()) {
            std::error_code fs_error;
            std::filesystem::create_directories(target.parent_path(), fs_error);
        }

        const int fd = sceKernelOpen(path.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd < 0) {
            return fail(error, "cannot create " + path + ": " + std::strerror(errno));
        }

        std::size_t written = 0;
        while (written < data.size()) {
            const long bytes = sceKernelWrite(fd, data.data() + written, data.size() - written);
            if (bytes <= 0) {
                sceKernelClose(fd);
                return fail(error, "write failed for " + path);
            }
            written += static_cast<std::size_t>(bytes);
        }

        sceKernelClose(fd);
        return true;
    }

    bool ensure_dir(const std::string& path, std::string* error) const override {
        std::error_code fs_error;
        std::filesystem::create_directories(path, fs_error);
        if (fs_error) {
            return fail(error, "cannot create " + path + ": " + fs_error.message());
        }
        return true;
    }

private:
    std::string root_;
};

}  // namespace

std::unique_ptr<Platform> make_platform() {
    return std::make_unique<Ps5Platform>();
}

}  // namespace pandora

#endif  // PANDORA_PLATFORM_PS5
