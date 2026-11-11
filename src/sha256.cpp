#include "pandora/sha256.hpp"

#include <cstdio>
#include <fstream>
#include <vector>

namespace pandora {
namespace {

constexpr std::uint32_t kRoundConstants[64] = {
    0x428a2f98U, 0x71374491U, 0xb5c0fbcfU, 0xe9b5dba5U, 0x3956c25bU, 0x59f111f1U, 0x923f82a4U,
    0xab1c5ed5U, 0xd807aa98U, 0x12835b01U, 0x243185beU, 0x550c7dc3U, 0x72be5d74U, 0x80deb1feU,
    0x9bdc06a7U, 0xc19bf174U, 0xe49b69c1U, 0xefbe4786U, 0x0fc19dc6U, 0x240ca1ccU, 0x2de92c6fU,
    0x4a7484aaU, 0x5cb0a9dcU, 0x76f988daU, 0x983e5152U, 0xa831c66dU, 0xb00327c8U, 0xbf597fc7U,
    0xc6e00bf3U, 0xd5a79147U, 0x06ca6351U, 0x14292967U, 0x27b70a85U, 0x2e1b2138U, 0x4d2c6dfcU,
    0x53380d13U, 0x650a7354U, 0x766a0abbU, 0x81c2c92eU, 0x92722c85U, 0xa2bfe8a1U, 0xa81a664bU,
    0xc24b8b70U, 0xc76c51a3U, 0xd192e819U, 0xd6990624U, 0xf40e3585U, 0x106aa070U, 0x19a4c116U,
    0x1e376c08U, 0x2748774cU, 0x34b0bcb5U, 0x391c0cb3U, 0x4ed8aa4aU, 0x5b9cca4fU, 0x682e6ff3U,
    0x748f82eeU, 0x78a5636fU, 0x84c87814U, 0x8cc70208U, 0x90befffaU, 0xa4506cebU, 0xbef9a3f7U,
    0xc67178f2U,
};

constexpr std::uint32_t rotr(std::uint32_t value, unsigned count) {
    return (value >> count) | (value << (32U - count));
}

}  // namespace

Sha256::Sha256() {
    state_[0] = 0x6a09e667U;
    state_[1] = 0xbb67ae85U;
    state_[2] = 0x3c6ef372U;
    state_[3] = 0xa54ff53aU;
    state_[4] = 0x510e527fU;
    state_[5] = 0x9b05688cU;
    state_[6] = 0x1f83d9abU;
    state_[7] = 0x5be0cd19U;
    buffer_length_ = 0;
    bit_count_ = 0;
}

void Sha256::process_block(const std::uint8_t* block) {
    std::uint32_t schedule[64];

    for (int i = 0; i < 16; ++i) {
        schedule[i] = (static_cast<std::uint32_t>(block[i * 4]) << 24U) |
                      (static_cast<std::uint32_t>(block[i * 4 + 1]) << 16U) |
                      (static_cast<std::uint32_t>(block[i * 4 + 2]) << 8U) |
                      static_cast<std::uint32_t>(block[i * 4 + 3]);
    }
    for (int i = 16; i < 64; ++i) {
        const std::uint32_t s0 =
            rotr(schedule[i - 15], 7) ^ rotr(schedule[i - 15], 18) ^ (schedule[i - 15] >> 3U);
        const std::uint32_t s1 =
            rotr(schedule[i - 2], 17) ^ rotr(schedule[i - 2], 19) ^ (schedule[i - 2] >> 10U);
        schedule[i] = schedule[i - 16] + s0 + schedule[i - 7] + s1;
    }

    std::uint32_t a = state_[0];
    std::uint32_t b = state_[1];
    std::uint32_t c = state_[2];
    std::uint32_t d = state_[3];
    std::uint32_t e = state_[4];
    std::uint32_t f = state_[5];
    std::uint32_t g = state_[6];
    std::uint32_t h = state_[7];

    for (int i = 0; i < 64; ++i) {
        const std::uint32_t s1 = rotr(e, 6) ^ rotr(e, 11) ^ rotr(e, 25);
        const std::uint32_t choice = (e & f) ^ (~e & g);
        const std::uint32_t temp1 = h + s1 + choice + kRoundConstants[i] + schedule[i];
        const std::uint32_t s0 = rotr(a, 2) ^ rotr(a, 13) ^ rotr(a, 22);
        const std::uint32_t majority = (a & b) ^ (a & c) ^ (b & c);
        const std::uint32_t temp2 = s0 + majority;

        h = g;
        g = f;
        f = e;
        e = d + temp1;
        d = c;
        c = b;
        b = a;
        a = temp1 + temp2;
    }

    state_[0] += a;
    state_[1] += b;
    state_[2] += c;
    state_[3] += d;
    state_[4] += e;
    state_[5] += f;
    state_[6] += g;
    state_[7] += h;
}

void Sha256::update(const void* data, std::size_t length) {
    const auto* bytes = static_cast<const std::uint8_t*>(data);
    bit_count_ += static_cast<std::uint64_t>(length) * 8U;

    for (std::size_t i = 0; i < length; ++i) {
        buffer_[buffer_length_++] = bytes[i];
        if (buffer_length_ == 64) {
            process_block(buffer_);
            buffer_length_ = 0;
        }
    }
}

void Sha256::update(std::string_view data) {
    update(data.data(), data.size());
}

std::string Sha256::hex_digest() {
    // Padding: 0x80, zeros, then the length in bits as a big-endian 64-bit value.
    const std::uint64_t bit_count = bit_count_;

    const std::uint8_t padding = 0x80U;
    update(&padding, 1);
    bit_count_ = bit_count;

    const std::uint8_t zero = 0x00U;
    while (buffer_length_ != 56) {
        update(&zero, 1);
        bit_count_ = bit_count;
    }

    std::uint8_t length_bytes[8];
    for (int i = 0; i < 8; ++i) {
        length_bytes[i] =
            static_cast<std::uint8_t>((bit_count >> (56U - 8U * static_cast<unsigned>(i))) & 0xFFU);
    }
    update(length_bytes, 8);

    char buffer[65];
    for (int i = 0; i < 8; ++i) {
        std::snprintf(buffer + i * 8, 9, "%08x", state_[i]);
    }
    return std::string(buffer, 64);
}

std::string sha256_hex(std::string_view data) {
    Sha256 hasher;
    hasher.update(data);
    return hasher.hex_digest();
}

std::optional<std::string> sha256_file(const std::string& path, std::string* error) {
    std::ifstream input(path, std::ios::binary);
    if (!input) {
        if (error != nullptr) {
            *error = "cannot open " + path;
        }
        return std::nullopt;
    }

    Sha256 hasher;
    std::vector<char> chunk(64 * 1024);
    while (input) {
        input.read(chunk.data(), static_cast<std::streamsize>(chunk.size()));
        const std::streamsize got = input.gcount();
        if (got > 0) {
            hasher.update(chunk.data(), static_cast<std::size_t>(got));
        }
    }
    if (input.bad()) {
        if (error != nullptr) {
            *error = "read failed for " + path;
        }
        return std::nullopt;
    }
    return hasher.hex_digest();
}

}  // namespace pandora
