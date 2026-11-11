#include "pandora/log.hpp"

#include <cstdio>
#include <string>

namespace pandora {
namespace {

LogLevel g_level = LogLevel::kInfo;

const char* level_name(LogLevel level) {
    switch (level) {
        case LogLevel::kDebug:
            return "DEBUG";
        case LogLevel::kInfo:
            return "INFO";
        case LogLevel::kWarn:
            return "WARN";
        case LogLevel::kError:
            return "ERROR";
    }
    return "INFO";
}

}  // namespace

void set_log_level(LogLevel level) {
    g_level = level;
}

void set_log_level(std::string_view name) {
    if (name == "debug") {
        g_level = LogLevel::kDebug;
    } else if (name == "warn") {
        g_level = LogLevel::kWarn;
    } else if (name == "error") {
        g_level = LogLevel::kError;
    } else {
        g_level = LogLevel::kInfo;
    }
}

void log(LogLevel level, std::string_view message) {
    if (static_cast<int>(level) < static_cast<int>(g_level)) {
        return;
    }
    std::fprintf(level >= LogLevel::kWarn ? stderr : stdout, "[%s] %.*s\n", level_name(level),
                 static_cast<int>(message.size()), message.data());
    std::fflush(level >= LogLevel::kWarn ? stderr : stdout);
}

}  // namespace pandora
