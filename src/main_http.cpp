// Entry point for the server binary.
//
//     pandora-http [--config PATH]
//
// The console build uses the same entry point; only the platform implementation
// behind make_platform() changes.

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

#include "pandora/config.hpp"
#include "pandora/http.hpp"
#include "pandora/log.hpp"
#include "pandora/platform.hpp"

namespace {

#ifdef PANDORA_VERSION
constexpr const char* kVersion = PANDORA_VERSION;
#else
constexpr const char* kVersion = "0.0.0-dev";
#endif

void print_usage() {
    std::printf(
        "pandora-http "
        "%s"
        "\n"
        "\n"
        "Usage:\n"
        "  pandora-http [--config PATH] [--bind ADDRESS] [--port PORT] [--log LEVEL]\n"
        "\n"
        "Options:\n"
        "  --config PATH   read settings from PATH (default: pandora.json)\n"
        "  --bind ADDRESS  address to bind (default: 127.0.0.1)\n"
        "  --port PORT     port to bind (default: 8080)\n"
        "  --log LEVEL     debug, info, warn or error (default: info)\n"
        "  --help, -h      show this message\n"
        "  --version, -V   show the version\n",
        kVersion);
}

}  // namespace

int main(int argc, char** argv) {
    std::vector<std::string> args(argv + 1, argv + argc);

    pandora::Config config = pandora::Config::load("pandora.json");

    for (std::size_t i = 0; i < args.size(); ++i) {
        const std::string& arg = args[i];
        auto next = [&](const char* name) -> std::string {
            if (i + 1 >= args.size()) {
                std::fprintf(stderr, "error: %s needs a value\n", name);
                std::exit(2);
            }
            return args[++i];
        };

        if (arg == "--help" || arg == "-h") {
            print_usage();
            return 0;
        }
        if (arg == "--version" || arg == "-V") {
            std::printf("%s\n", kVersion);
            return 0;
        }
        if (arg == "--config") {
            config = pandora::Config::load(next("--config"));
        } else if (arg == "--bind") {
            config.bind_address = next("--bind");
        } else if (arg == "--port") {
            config.port = static_cast<std::uint16_t>(std::stoul(next("--port")));
        } else if (arg == "--log") {
            config.log_level = next("--log");
        } else {
            std::fprintf(stderr, "error: unknown argument '%s'\n", arg.c_str());
            print_usage();
            return 2;
        }
    }

    pandora::set_log_level(config.log_level);

    const auto platform = pandora::make_platform();
    pandora::log(pandora::LogLevel::kInfo, std::string("platform: ") + platform->name() +
                                               ", data root: " + platform->data_root());

    // The data root is the one directory the server expects to own, so it
    // creates it rather than failing later on the first write.
    std::string root_error;
    if (!platform->ensure_dir(platform->data_root(), &root_error)) {
        pandora::log(pandora::LogLevel::kWarn, root_error);
    }

    pandora::HttpServer server(config, *platform);
    std::string error;
    if (!server.run(&error)) {
        pandora::log(pandora::LogLevel::kError, error);
        return 1;
    }
    return 0;
}
