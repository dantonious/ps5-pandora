#include "pandora/zip.hpp"

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <string>

#include "pandora/sha256.hpp"
#include "support/check.hpp"

namespace {

std::string read_binary(const std::string& path) {
    std::ifstream input(path, std::ios::binary);
    return std::string((std::istreambuf_iterator<char>(input)), std::istreambuf_iterator<char>());
}

std::uint32_t read_u32(const std::string& data, std::size_t offset) {
    return static_cast<std::uint32_t>(static_cast<unsigned char>(data[offset])) |
           (static_cast<std::uint32_t>(static_cast<unsigned char>(data[offset + 1])) << 8U) |
           (static_cast<std::uint32_t>(static_cast<unsigned char>(data[offset + 2])) << 16U) |
           (static_cast<std::uint32_t>(static_cast<unsigned char>(data[offset + 3])) << 24U);
}

std::uint16_t read_u16(const std::string& data, std::size_t offset) {
    return static_cast<std::uint16_t>(static_cast<unsigned char>(data[offset]) |
                                      (static_cast<unsigned char>(data[offset + 1]) << 8U));
}

}  // namespace

int main() {
    // The published CRC-32 check value for "123456789".
    char crc[16];
    std::snprintf(crc, sizeof(crc), "%08x", pandora::zip::crc32("123456789"));
    EXPECT_STR_EQ("cbf43926", crc);

    char empty_crc[16];
    std::snprintf(empty_crc, sizeof(empty_crc), "%08x", pandora::zip::crc32(""));
    EXPECT_STR_EQ("00000000", empty_crc);

    const std::filesystem::path directory =
        std::filesystem::temp_directory_path() / "pandora-zip-test";
    std::filesystem::remove_all(directory);
    std::filesystem::create_directories(directory);
    const std::string archive_path = (directory / "PPSA77010.zip").string();

    const std::vector<pandora::zip::Entry> entries = {
        {"PPSA77010/", ""},
        {"PPSA77010/sce_sys/", ""},
        {"PPSA77010/README.md", "hello\n"},
        {"PPSA77010/sce_sys/param.json", "{\"titleId\": \"PPSA77010\"}\n"},
    };

    std::string error;
    EXPECT(pandora::zip::write_archive(archive_path, entries, &error));

    const std::string archive = read_binary(archive_path);
    EXPECT(!archive.empty());

    // Local file header signature, and the first stored name.
    EXPECT_EQ(0x04034b50u, read_u32(archive, 0));
    EXPECT_EQ(static_cast<std::uint16_t>(0), read_u16(archive, 8));    // stored, not deflated
    EXPECT_EQ(static_cast<std::uint16_t>(10), read_u16(archive, 26));  // "PPSA77010/"
    EXPECT_STR_EQ("PPSA77010/", archive.substr(30, 10));

    // End of central directory, and the counts in it.
    const std::size_t eocd = archive.rfind("PK\x05\x06");
    EXPECT(eocd != std::string::npos);
    EXPECT_EQ(static_cast<std::uint16_t>(entries.size()), read_u16(archive, eocd + 10));
    EXPECT_EQ(static_cast<std::uint16_t>(entries.size()), read_u16(archive, eocd + 8));

    const std::uint32_t central_size = read_u32(archive, eocd + 12);
    const std::uint32_t central_offset = read_u32(archive, eocd + 16);
    EXPECT_EQ(static_cast<std::uint32_t>(eocd), central_offset + central_size);
    EXPECT_EQ(0x02014b50u, read_u32(archive, central_offset));

    // Every payload byte is in the archive, uncompressed.
    EXPECT(archive.find("hello\n") != std::string::npos);
    EXPECT(archive.find("{\"titleId\": \"PPSA77010\"}\n") != std::string::npos);

    // The writer is deterministic: the same input hashes the same.
    const std::string first_digest = pandora::sha256_file(archive_path, &error).value_or("-");
    EXPECT(pandora::zip::write_archive(archive_path, entries, &error));
    EXPECT_STR_EQ(first_digest, pandora::sha256_file(archive_path, &error).value_or("-"));

    std::filesystem::remove_all(directory);
    return pandora::test::summary("zip");
}
