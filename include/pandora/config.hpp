#pragma once

#include <cstdint>
#include <string>

namespace pandora {

/// Runtime configuration.
///
/// The default bind address is loopback. The console UI is opened locally, so
/// exposing the server to the network is an explicit opt-in rather than
/// something you discover you have been doing.
struct Config {
    std::string bind_address = "127.0.0.1";
    std::uint16_t port = 8080;
    std::string data_root = "./data";
    std::string payload_dir = "./data/payloads";
    std::string log_level = "info";

    /// Reads \p path, or returns defaults when it does not exist or is invalid.
    static Config load(const std::string& path);

    bool save(const std::string& path, std::string* error) const;
};

}  // namespace pandora
