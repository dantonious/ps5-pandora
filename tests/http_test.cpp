#include "pandora/http.hpp"

#include "support/check.hpp"

namespace {

/// A platform with canned answers, so routing is tested without touching disk.
class FakePlatform final : public pandora::Platform {
public:
    std::string name() const override { return "fake"; }
    std::string data_root() const override { return "/fake"; }

    std::vector<pandora::DirEntry> list_dir(const std::string& path,
                                            std::string* error) const override {
        if (path == "/missing") {
            if (error != nullptr) {
                *error = "cannot list /missing";
            }
            return {};
        }
        if (path == "/empty") {
            return {};
        }
        return {pandora::DirEntry{"beta.txt", path + "/beta.txt", false, 2048},
                pandora::DirEntry{"alpha", path + "/alpha", true, 0}};
    }

    std::optional<std::string> read_file(const std::string&, std::string*) const override {
        return std::nullopt;
    }
    bool write_file(const std::string&, std::string_view, std::string*) const override {
        return false;
    }
    bool ensure_dir(const std::string&, std::string*) const override { return true; }
};

pandora::Response call(const std::string& target, const pandora::Platform& platform,
                       const std::string& method = "GET") {
    pandora::Request request;
    request.method = method;
    request.target = target;
    const std::size_t question = target.find('?');
    if (question == std::string::npos) {
        request.path = target;
    } else {
        request.path = target.substr(0, question);
        request.query = target.substr(question + 1);
    }
    return pandora::route_request(request, platform);
}

}  // namespace

int main() {
    const FakePlatform platform;

    // Health.
    const auto health = call("/api/health", platform);
    EXPECT_EQ(200, health.status);
    EXPECT(health.content_type.find("application/json") != std::string::npos);
    EXPECT(health.body.find("\"status\": \"ok\"") != std::string::npos);
    EXPECT(health.body.find("\"platform\": \"fake\"") != std::string::npos);

    // Version.
    const auto version = call("/api/version", platform);
    EXPECT_EQ(200, version.status);
    EXPECT(version.body.find("\"data_root\": \"/fake\"") != std::string::npos);

    // The UI is served from the embedded document.
    const auto index = call("/", platform);
    EXPECT_EQ(200, index.status);
    EXPECT(index.content_type.find("text/html") != std::string::npos);
    EXPECT(index.body.find("ps5-pandora") != std::string::npos);

    // A listing, including a path that needs percent-decoding.
    const auto listing = call("/api/fs/list?path=%2Fdata", platform);
    EXPECT_EQ(200, listing.status);
    EXPECT(listing.body.find("\"path\": \"/data\"") != std::string::npos);
    EXPECT(listing.body.find("\"name\": \"alpha\"") != std::string::npos);
    EXPECT(listing.body.find("\"is_dir\": true") != std::string::npos);
    EXPECT(listing.body.find("\"size\": 2048") != std::string::npos);

    // An empty directory is a success with no entries, not an error.
    const auto empty = call("/api/fs/list?path=/empty", platform);
    EXPECT_EQ(200, empty.status);
    EXPECT(empty.body.find("\"entries\": [\n  ]") != std::string::npos);

    // A missing path parameter is the client's fault.
    EXPECT_EQ(400, call("/api/fs/list", platform).status);
    EXPECT_EQ(400, call("/api/fs/list?path=", platform).status);

    // A platform error becomes a 404 carrying the message.
    const auto missing = call("/api/fs/list?path=/missing", platform);
    EXPECT_EQ(404, missing.status);
    EXPECT(missing.body.find("cannot list /missing") != std::string::npos);

    // Unknown routes, unsupported methods.
    EXPECT_EQ(404, call("/nope", platform).status);
    EXPECT_EQ(405, call("/api/health", platform, "POST").status);
    EXPECT_EQ(200, call("/api/health", platform, "HEAD").status);

    // Query parsing: absent, empty, duplicated (first wins), and + as space.
    {
        pandora::Request request;
        request.query = "a=1&b=&a=2&c=x+y&d";
        EXPECT_STR_EQ("1", request.param("a").value_or("<missing>"));
        EXPECT_STR_EQ("", request.param("b").value_or("<missing>"));
        EXPECT_STR_EQ("x y", request.param("c").value_or("<missing>"));
        EXPECT_STR_EQ("", request.param("d").value_or("<missing>"));
        EXPECT(!request.param("z").has_value());
    }

    // Percent-decoding, including the cases it must leave alone.
    EXPECT_STR_EQ("/data/pandora", pandora::percent_decode("%2Fdata%2Fpandora"));
    EXPECT_STR_EQ("a b", pandora::percent_decode("a+b"));
    EXPECT_STR_EQ("%zz", pandora::percent_decode("%zz"));
    EXPECT_STR_EQ("trailing%", pandora::percent_decode("trailing%"));
    EXPECT_STR_EQ("100%", pandora::percent_decode("100%25"));

    // Serialisation is a well-formed response.
    const std::string wire = pandora::Response::text("hi").serialize();
    EXPECT(wire.find("HTTP/1.1 200 OK\r\n") == 0);
    EXPECT(wire.find("Content-Length: 2\r\n") != std::string::npos);
    EXPECT(wire.find("\r\n\r\nhi") != std::string::npos);

    return pandora::test::summary("http");
}
