#include "pandora/config.hpp"

#include <cstdint>
#include <exception>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <system_error>

#include "pandora/json.hpp"
#include "pandora/log.hpp"

namespace pandora {
namespace {

bool fail(std::string* error, std::string message) {
    if (error != nullptr) {
        *error = std::move(message);
    }
    return false;
}

std::optional<std::string> read_text_file(const std::string& path, std::string* error) {
    std::ifstream input(path, std::ios::binary);
    if (!input) {
        fail(error, "cannot open " + path);
        return std::nullopt;
    }
    std::ostringstream buffer;
    buffer << input.rdbuf();
    return buffer.str();
}

}  // namespace

Config Config::load(const std::string& path) {
    Config config;

    std::error_code exists_error;
    if (!std::filesystem::exists(path, exists_error)) {
        // Absent configuration is not an error: defaults target the console.
        return config;
    }

    std::string error;
    const auto text = read_text_file(path, &error);
    if (!text.has_value()) {
        log(LogLevel::kWarn, "config: " + error + "; using defaults");
        return config;
    }

    const auto document = json::parse_flat_object(*text, &error);
    if (!document.has_value()) {
        log(LogLevel::kWarn, "config: " + error + "; using defaults");
        return config;
    }

    if (const auto value = document->get("bind_address"); value.has_value()) {
        config.bind_address = *value;
    }
    if (const auto value = document->get("port"); value.has_value()) {
        try {
            const unsigned long parsed = std::stoul(*value);
            if (parsed > 0 && parsed <= 65535) {
                config.port = static_cast<std::uint16_t>(parsed);
            } else {
                log(LogLevel::kWarn,
                    "config: port out of range; keeping " + std::to_string(config.port));
            }
        } catch (const std::exception&) {
            log(LogLevel::kWarn,
                "config: port is not a number; keeping " + std::to_string(config.port));
        }
    }
    if (const auto value = document->get("data_root"); value.has_value()) {
        config.data_root = *value;
    }
    if (const auto value = document->get("payload_dir"); value.has_value()) {
        config.payload_dir = *value;
    }
    if (const auto value = document->get("log_level"); value.has_value()) {
        config.log_level = *value;
    }

    return config;
}

bool Config::save(const std::string& path, std::string* error) const {
    const std::filesystem::path target(path);
    if (target.has_parent_path()) {
        std::error_code fs_error;
        std::filesystem::create_directories(target.parent_path(), fs_error);
        if (fs_error) {
            return fail(error, "cannot create " + target.parent_path().string() + ": " +
                                   fs_error.message());
        }
    }

    json::FlatObject object;
    object.set("bind_address", bind_address);
    object.set("port", std::to_string(port));
    object.set("data_root", data_root);
    object.set("payload_dir", payload_dir);
    object.set("log_level", log_level);

    std::ofstream output(path, std::ios::binary | std::ios::trunc);
    if (!output) {
        return fail(error, "cannot write " + path);
    }
    output << json::to_pretty_string(object);
    if (!output) {
        return fail(error, "write failed for " + path);
    }
    return true;
}

}  // namespace pandora
