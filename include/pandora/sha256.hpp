// SHA-256, implemented here rather than linked in.
//
// The catalog ties a listing to the artifact hash, so the exporter has to
// compute one, and the console build should not grow a crypto dependency purely
// for that. This is the standard algorithm, verified against the published test
// vectors in tests/sha256_test.cpp.

#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

namespace pandora {

class Sha256 {
public:
    Sha256();

    void update(const void* data, std::size_t length);
    void update(std::string_view data);

    /// Finalises and returns the digest as 64 lowercase hex characters.
    /// The object must not be used again afterwards.
    std::string hex_digest();

private:
    void process_block(const std::uint8_t* block);

    std::uint32_t state_[8];
    std::uint64_t bit_count_;
    std::uint8_t buffer_[64];
    std::size_t buffer_length_;
};

std::string sha256_hex(std::string_view data);

std::optional<std::string> sha256_file(const std::string& path, std::string* error);

}  // namespace pandora
