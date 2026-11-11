// Host implementation of Platform, backed by std::filesystem.
//
// This is not a stub: it is what runs when you build and run on your laptop, and
// it is what the tests exercise. Behaviour here and on the console must match,
// which is why the differences are kept to path roots only.

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <system_error>

#include "pandora/platform.hpp"

namespace pandora {
namespace {

bool fail(std::string* error, std::string message) {
    if (error != nullptr) {
        *error = std::move(message);
    }
    return false;
}

class HostPlatform final : public Platform {
public:
    HostPlatform() : root_("./data") {}

    std::string name() const override { return "host"; }

    std::string data_root() const override { return root_; }

    std::vector<DirEntry> list_dir(const std::string& path, std::string* error) const override {
        std::vector<DirEntry> entries;

        std::error_code iteration_error;
        std::filesystem::directory_iterator iterator(path, iteration_error);
        if (iteration_error) {
            fail(error, "cannot list " + path + ": " + iteration_error.message());
            return entries;
        }

        for (const auto& item : iterator) {
            std::error_code status_error;
            const bool is_dir = item.is_directory(status_error);
            const bool is_file = item.is_regular_file(status_error);
            if (status_error || (!is_dir && !is_file)) {
                continue;
            }

            DirEntry entry;
            entry.name = item.path().filename().string();
            entry.path = item.path().string();
            entry.is_dir = is_dir;
            if (!is_dir) {
                std::error_code size_error;
                const auto size = item.file_size(size_error);
                entry.size = size_error ? 0 : static_cast<std::uint64_t>(size);
            }
            entries.push_back(std::move(entry));
        }

        std::sort(entries.begin(), entries.end(), [](const DirEntry& a, const DirEntry& b) {
            if (a.is_dir != b.is_dir) {
                return a.is_dir;  // directories first
            }
            return a.name < b.name;
        });
        return entries;
    }

    std::optional<std::string> read_file(const std::string& path,
                                         std::string* error) const override {
        std::ifstream input(path, std::ios::binary);
        if (!input) {
            fail(error, "cannot open " + path);
            return std::nullopt;
        }
        std::string data((std::istreambuf_iterator<char>(input)), std::istreambuf_iterator<char>());
        if (input.bad()) {
            fail(error, "read failed for " + path);
            return std::nullopt;
        }
        return data;
    }

    bool write_file(const std::string& path, std::string_view data,
                    std::string* error) const override {
        const std::filesystem::path target(path);
        if (target.has_parent_path()) {
            std::error_code fs_error;
            std::filesystem::create_directories(target.parent_path(), fs_error);
            if (fs_error) {
                return fail(error, "cannot create " + target.parent_path().string() + ": " +
                                       fs_error.message());
            }
        }

        std::ofstream output(path, std::ios::binary | std::ios::trunc);
        if (!output) {
            return fail(error, "cannot write " + path);
        }
        output.write(data.data(), static_cast<std::streamsize>(data.size()));
        if (!output) {
            return fail(error, "write failed for " + path);
        }
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
    return std::make_unique<HostPlatform>();
}

}  // namespace pandora
