#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <utility>

#include "pandora/config.hpp"
#include "pandora/platform.hpp"

namespace pandora {

struct Request {
    std::string method;
    std::string target;  // the raw request target, e.g. /api/fs/list?path=/data
    std::string path;    // target without the query string
    std::string query;   // target after '?', or empty

    /// Returns the decoded value of a query parameter, or nullopt.
    std::optional<std::string> param(std::string_view key) const;
};

struct Response {
    int status = 200;
    std::string content_type = "text/plain; charset=utf-8";
    std::string body;

    std::string serialize() const;

    static Response json(std::string body, int status = 200);
    static Response html(std::string body);
    static Response text(std::string body, int status = 200);
};

/// Maps a request to a response. Pure: no I/O, no sockets, fully unit-testable.
Response route_request(const Request& request, const Platform& platform);

/// Percent-decodes a query component. Invalid escapes are left as written.
std::string percent_decode(std::string_view text);

/// A single-threaded HTTP/1.1 server. Sufficient for a local UI, and deliberately
/// the smallest thing that can serve one: a console has one user.
class HttpServer {
public:
    HttpServer(Config config, const Platform& platform)
        : config_(std::move(config)), platform_(platform) {}

    /// Serves until interrupted. Returns false and fills \p error if it cannot
    /// start or if the platform has no implementation.
    bool run(std::string* error);

private:
    Config config_;
    const Platform& platform_;
};

}  // namespace pandora
