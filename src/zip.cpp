// Write a ZIP archive whose top level is the title folder, TITLEID/.
//
// Entries are stored uncompressed. That keeps this file free of any compression
// dependency, which matters because the same writer has to run on the console.
// The archive is dominated by the payload binary either way.

#include "pandora/zip.hpp"

#include <cstddef>
#include <fstream>

namespace pandora::zip {
namespace {

constexpr std::uint32_t kLocalHeaderSignature = 0x04034b50U;
constexpr std::uint32_t kCentralHeaderSignature = 0x02014b50U;
constexpr std::uint32_t kEndOfCentralDirectorySignature = 0x06054b50U;
constexpr std::uint16_t kVersionNeeded = 20;  // 2.0: the baseline ZIP feature set
constexpr std::uint16_t kMethodStored = 0;

void put_u16(std::string& out, std::uint16_t value) {
    out.push_back(static_cast<char>(value & 0xFFU));
    out.push_back(static_cast<char>((value >> 8U) & 0xFFU));
}

void put_u32(std::string& out, std::uint32_t value) {
    out.push_back(static_cast<char>(value & 0xFFU));
    out.push_back(static_cast<char>((value >> 8U) & 0xFFU));
    out.push_back(static_cast<char>((value >> 16U) & 0xFFU));
    out.push_back(static_cast<char>((value >> 24U) & 0xFFU));
}

/// A fixed timestamp keeps archives byte-for-byte reproducible across machines,
/// which matters because a catalog record pins the artifact's hash.
constexpr std::uint16_t kDosTime = 0x0000;
constexpr std::uint16_t kDosDate = 0x2821;  // 2000-01-01

}  // namespace

std::uint32_t crc32(std::string_view data) {
    static std::uint32_t table[256];
    static bool initialised = false;
    if (!initialised) {
        for (std::uint32_t i = 0; i < 256; ++i) {
            std::uint32_t value = i;
            for (int bit = 0; bit < 8; ++bit) {
                value = (value & 1U) != 0U ? (0xEDB88320U ^ (value >> 1U)) : (value >> 1U);
            }
            table[i] = value;
        }
        initialised = true;
    }

    std::uint32_t crc = 0xFFFFFFFFU;
    for (const char c : data) {
        crc = table[(crc ^ static_cast<std::uint8_t>(c)) & 0xFFU] ^ (crc >> 8U);
    }
    return crc ^ 0xFFFFFFFFU;
}

bool write_archive(const std::string& path, const std::vector<Entry>& entries, std::string* error) {
    std::string out;
    std::string central;

    for (const Entry& entry : entries) {
        const std::uint32_t offset = static_cast<std::uint32_t>(out.size());
        const std::uint32_t checksum = crc32(entry.data);
        const std::uint32_t size = static_cast<std::uint32_t>(entry.data.size());
        const std::uint16_t name_length = static_cast<std::uint16_t>(entry.name.size());

        put_u32(out, kLocalHeaderSignature);
        put_u16(out, kVersionNeeded);
        put_u16(out, 0);  // general purpose flags
        put_u16(out, kMethodStored);
        put_u16(out, kDosTime);
        put_u16(out, kDosDate);
        put_u32(out, checksum);
        put_u32(out, size);
        put_u32(out, size);
        put_u16(out, name_length);
        put_u16(out, 0);  // extra field length
        out += entry.name;
        out += entry.data;

        put_u32(central, kCentralHeaderSignature);
        put_u16(central, kVersionNeeded);  // version made by
        put_u16(central, kVersionNeeded);  // version needed
        put_u16(central, 0);
        put_u16(central, kMethodStored);
        put_u16(central, kDosTime);
        put_u16(central, kDosDate);
        put_u32(central, checksum);
        put_u32(central, size);
        put_u32(central, size);
        put_u16(central, name_length);
        put_u16(central, 0);  // extra
        put_u16(central, 0);  // comment
        put_u16(central, 0);  // disk number
        put_u16(central, 0);  // internal attributes
        put_u32(central, 0);  // external attributes
        put_u32(central, offset);
        central += entry.name;
    }

    const std::uint32_t central_offset = static_cast<std::uint32_t>(out.size());
    const std::uint32_t central_size = static_cast<std::uint32_t>(central.size());
    out += central;

    put_u32(out, kEndOfCentralDirectorySignature);
    put_u16(out, 0);
    put_u16(out, 0);
    put_u16(out, static_cast<std::uint16_t>(entries.size()));
    put_u16(out, static_cast<std::uint16_t>(entries.size()));
    put_u32(out, central_size);
    put_u32(out, central_offset);
    put_u16(out, 0);  // comment length

    std::ofstream output(path, std::ios::binary | std::ios::trunc);
    if (!output) {
        if (error != nullptr) {
            *error = "cannot create " + path;
        }
        return false;
    }
    output.write(out.data(), static_cast<std::streamsize>(out.size()));
    if (!output) {
        if (error != nullptr) {
            *error = "write failed for " + path;
        }
        return false;
    }
    return true;
}

}  // namespace pandora::zip
