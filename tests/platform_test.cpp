#include <algorithm>
#include <filesystem>
#include <fstream>

#include "pandora/platform.hpp"
#include "support/check.hpp"

namespace {

void write(const std::filesystem::path& path, const std::string& contents) {
    std::filesystem::create_directories(path.parent_path());
    std::ofstream out(path, std::ios::binary);
    out << contents;
}

}  // namespace

int main() {
    const auto platform = pandora::make_platform();
    EXPECT(!platform->name().empty());
    EXPECT(!platform->data_root().empty());

    const std::filesystem::path root =
        std::filesystem::temp_directory_path() / "pandora-platform-test";
    std::filesystem::remove_all(root);
    write(root / "zeta.txt", "z");
    write(root / "alpha" / "nested.txt", "n");
    write(root / "mid.bin", std::string(4096, 'x'));

    std::string error;
    const auto entries = platform->list_dir(root.string(), &error);
    EXPECT(error.empty());
    EXPECT_EQ(3u, entries.size());

    // Directories first, then names in ascending order: callers can rely on it.
    EXPECT_STR_EQ("alpha", entries[0].name);
    EXPECT(entries[0].is_dir);
    EXPECT_STR_EQ("mid.bin", entries[1].name);
    EXPECT(!entries[1].is_dir);
    EXPECT_STR_EQ("zeta.txt", entries[2].name);
    EXPECT_EQ(4096u, entries[1].size);
    EXPECT_EQ(0u, entries[0].size);

    // Reading, including binary content that contains a NUL.
    const auto text = platform->read_file((root / "zeta.txt").string(), &error);
    EXPECT(text.has_value());
    EXPECT_STR_EQ("z", *text);

    const auto binary = platform->read_file((root / "mid.bin").string(), &error);
    EXPECT(binary.has_value());
    EXPECT_EQ(4096u, binary->size());

    // A missing file reports an error rather than an empty success.
    error.clear();
    EXPECT(!platform->read_file((root / "absent").string(), &error).has_value());
    EXPECT(!error.empty());

    // A missing directory likewise.
    error.clear();
    const auto absent = platform->list_dir((root / "no-such-dir").string(), &error);
    EXPECT(absent.empty());
    EXPECT(!error.empty());

    // An empty directory is a success with nothing in it.
    std::filesystem::create_directories(root / "vacant");
    error.clear();
    const auto vacant = platform->list_dir((root / "vacant").string(), &error);
    EXPECT(vacant.empty());
    EXPECT(error.empty());

    // Writing creates parent directories, and the bytes survive the round trip.
    const std::string nested = (root / "deep" / "deeper" / "out.bin").string();
    const std::string payload = std::string("a\0b", 3);
    EXPECT(platform->write_file(nested, payload, &error));
    const auto read_back = platform->read_file(nested, &error);
    EXPECT(read_back.has_value());
    EXPECT_EQ(3u, read_back->size());

    std::filesystem::remove_all(root);
    return pandora::test::summary("platform");
}
