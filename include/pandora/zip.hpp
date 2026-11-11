// Minimal ZIP writer.
//
// The catalog expects a release asset that is a .zip whose top level is the
// title folder. That is a fixed shape, so this writer produces exactly it.

#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace pandora::zip {

struct Entry {
    std::string name;  // must use '/' separators
    std::string data;  // empty for a directory entry, whose name ends in '/'
};

/// CRC-32 as defined by the ZIP specification.
std::uint32_t crc32(std::string_view data);

/// Writes a stored (uncompressed) archive. Returns false and fills \p error on
/// failure.
bool write_archive(const std::string& path, const std::vector<Entry>& entries, std::string* error);

}  // namespace pandora::zip
