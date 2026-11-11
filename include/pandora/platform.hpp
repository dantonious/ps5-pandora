// The one place where host and console differ.
//
// Everything above this interface is portable code that runs unchanged in both
// builds and is covered by the tests. Keeping the boundary this narrow is what
// makes the console port a build task rather than a rewrite.

#pragma once

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace pandora {

struct DirEntry {
    std::string name;
    std::string path;
    bool is_dir = false;
    std::uint64_t size = 0;
};

class Platform {
public:
    virtual ~Platform() = default;

    virtual std::string name() const = 0;
    virtual std::string data_root() const = 0;

    /// Lists \p path, directories first, then names in ascending order.
    /// On failure returns an empty vector and fills \p error.
    virtual std::vector<DirEntry> list_dir(const std::string& path, std::string* error) const = 0;

    virtual std::optional<std::string> read_file(const std::string& path,
                                                 std::string* error) const = 0;

    virtual bool write_file(const std::string& path, std::string_view data,
                            std::string* error) const = 0;

    /// Creates \p path and any missing parents. Succeeds when it already exists.
    virtual bool ensure_dir(const std::string& path, std::string* error) const = 0;
};

/// The implementation for the build in progress: host or console.
std::unique_ptr<Platform> make_platform();

}  // namespace pandora
