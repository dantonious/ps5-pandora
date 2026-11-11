#include "pandora/config.hpp"

#include <filesystem>
#include <fstream>

#include "support/check.hpp"

int main() {
    const std::filesystem::path directory =
        std::filesystem::temp_directory_path() / "pandora-config-test";
    std::filesystem::remove_all(directory);
    std::filesystem::create_directories(directory);

    // Defaults bind to loopback: reaching the console UI from the network should
    // be something you asked for.
    const pandora::Config defaults;
    EXPECT_STR_EQ("127.0.0.1", defaults.bind_address);
    EXPECT_EQ(8080, defaults.port);

    // A missing file is not an error.
    const pandora::Config missing = pandora::Config::load((directory / "absent.json").string());
    EXPECT_STR_EQ(defaults.bind_address, missing.bind_address);
    EXPECT_EQ(defaults.port, missing.port);

    const std::string path = (directory / "pandora.json").string();
    pandora::Config written;
    written.bind_address = "0.0.0.0";
    written.port = 9411;
    written.data_root = "/data/pandora";
    written.log_level = "debug";

    std::string error;
    EXPECT(written.save(path, &error));

    const pandora::Config loaded = pandora::Config::load(path);
    EXPECT_STR_EQ("0.0.0.0", loaded.bind_address);
    EXPECT_EQ(9411, loaded.port);
    EXPECT_STR_EQ("/data/pandora", loaded.data_root);
    EXPECT_STR_EQ("debug", loaded.log_level);

    // Out-of-range and non-numeric ports fall back rather than binding somewhere
    // unexpected.
    {
        std::ofstream out((directory / "bad_port.json"));
        out << "{\"port\": \"70000\"}";
    }
    EXPECT_EQ(defaults.port, pandora::Config::load((directory / "bad_port.json").string()).port);
    {
        std::ofstream out((directory / "nan_port.json"));
        out << "{\"port\": \"http\"}";
    }
    EXPECT_EQ(defaults.port, pandora::Config::load((directory / "nan_port.json").string()).port);

    // A corrupt file falls back to defaults instead of failing to start.
    {
        std::ofstream out((directory / "corrupt.json"));
        out << "{ this is not json";
    }
    const pandora::Config corrupt = pandora::Config::load((directory / "corrupt.json").string());
    EXPECT_EQ(defaults.port, corrupt.port);

    // save creates the parent directories it needs.
    const std::string nested = (directory / "a" / "b" / "pandora.json").string();
    EXPECT(written.save(nested, &error));
    EXPECT(std::filesystem::is_regular_file(nested));

    std::filesystem::remove_all(directory);
    return pandora::test::summary("config");
}
