#pragma once

#include <string_view>

namespace pandora {

enum class LogLevel { kDebug = 0, kInfo = 1, kWarn = 2, kError = 3 };

/// Writes one line to stdout, or stderr for warnings and errors.
void log(LogLevel level, std::string_view message);

void set_log_level(LogLevel level);

/// Accepts "debug", "info", "warn", "error"; anything else means info.
void set_log_level(std::string_view name);

}  // namespace pandora
