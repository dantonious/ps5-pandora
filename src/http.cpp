#include "pandora/http.hpp"

#include <string>

#include "pandora/json.hpp"
#include "pandora/log.hpp"
#include "pandora/ui_index_html.hpp"

#ifndef PANDORA_VERSION
#define PANDORA_VERSION "0.0.0-dev"
#endif
#ifndef PANDORA_NAME
#define PANDORA_NAME "ps5-pandora"
#endif

#if !defined(_WIN32)
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

namespace pandora {
namespace {

constexpr std::string_view kReasonOk = "OK";
constexpr std::string_view kReasonBadRequest = "Bad Request";
constexpr std::string_view kReasonNotFound = "Not Found";
constexpr std::string_view kReasonMethodNotAllowed = "Method Not Allowed";
constexpr std::string_view kReasonInternalError = "Internal Server Error";

std::string_view reason_phrase(int status) {
    switch (status) {
        case 200:
            return kReasonOk;
        case 400:
            return kReasonBadRequest;
        case 404:
            return kReasonNotFound;
        case 405:
            return kReasonMethodNotAllowed;
        case 500:
            return kReasonInternalError;
        default:
            return "Unknown";
    }
}

std::string entries_to_json(std::string_view path, const std::vector<DirEntry>& entries) {
    std::string body = "{\n  \"path\": \"";
    body += json::escape(path);
    body += "\",\n  \"entries\": [\n";
    for (std::size_t i = 0; i < entries.size(); ++i) {
        const DirEntry& entry = entries[i];
        body += "    {\"name\": \"";
        body += json::escape(entry.name);
        body += "\", \"is_dir\": ";
        body += entry.is_dir ? "true" : "false";
        body += ", \"size\": ";
        body += std::to_string(entry.size);
        body += "}";
        if (i + 1 < entries.size()) {
            body += ',';
        }
        body += '\n';
    }
    body += "  ]\n}\n";
    return body;
}

}  // namespace

std::string percent_decode(std::string_view text) {
    std::string out;
    out.reserve(text.size());

    for (std::size_t i = 0; i < text.size(); ++i) {
        const char c = text[i];
        if (c == '+') {
            out.push_back(' ');
            continue;
        }
        if (c != '%' || i + 2 >= text.size()) {
            out.push_back(c);
            continue;
        }

        auto digit = [](char value) -> int {
            if (value >= '0' && value <= '9')
                return value - '0';
            if (value >= 'a' && value <= 'f')
                return value - 'a' + 10;
            if (value >= 'A' && value <= 'F')
                return value - 'A' + 10;
            return -1;
        };

        const int high = digit(text[i + 1]);
        const int low = digit(text[i + 2]);
        if (high < 0 || low < 0) {
            // Not a valid escape: keep the text as the client wrote it.
            out.push_back(c);
            continue;
        }
        out.push_back(static_cast<char>((high << 4) | low));
        i += 2;
    }
    return out;
}

std::optional<std::string> Request::param(std::string_view key) const {
    std::size_t start = 0;
    while (start <= query.size()) {
        std::size_t end = query.find('&', start);
        if (end == std::string::npos) {
            end = query.size();
        }

        const std::string_view pair = std::string_view(query).substr(start, end - start);
        const std::size_t equals = pair.find('=');
        const std::string_view name =
            equals == std::string_view::npos ? pair : pair.substr(0, equals);
        if (name == key) {
            const std::string_view raw =
                equals == std::string_view::npos ? std::string_view{} : pair.substr(equals + 1);
            return percent_decode(raw);
        }

        if (end == query.size()) {
            break;
        }
        start = end + 1;
    }
    return std::nullopt;
}

std::string Response::serialize() const {
    std::string head = "HTTP/1.1 ";
    head += std::to_string(status);
    head += ' ';
    head += reason_phrase(status);
    head += "\r\nContent-Type: ";
    head += content_type;
    head += "\r\nContent-Length: ";
    head += std::to_string(body.size());
    head += "\r\nConnection: close\r\n\r\n";
    return head + body;
}

Response Response::json(std::string body, int status) {
    Response response;
    response.status = status;
    response.content_type = "application/json; charset=utf-8";
    response.body = std::move(body);
    return response;
}

Response Response::html(std::string body) {
    Response response;
    response.content_type = "text/html; charset=utf-8";
    response.body = std::move(body);
    return response;
}

Response Response::text(std::string body, int status) {
    Response response;
    response.status = status;
    response.content_type = "text/plain; charset=utf-8";
    response.body = std::move(body);
    return response;
}

Response route_request(const Request& request, const Platform& platform) {
    if (request.method != "GET" && request.method != "HEAD") {
        return Response::text("method not allowed\n", 405);
    }

    if (request.path == "/" || request.path == "/index.html") {
        return Response::html(generated::kUiIndexHtml);
    }

    if (request.path == "/api/health") {
        std::string body = "{\"status\": \"ok\", \"name\": \"";
        body += PANDORA_NAME;
        body += "\", \"version\": \"";
        body += PANDORA_VERSION;
        body += "\", \"platform\": \"";
        body += platform.name();
        body += "\"}\n";
        return Response::json(std::move(body));
    }

    if (request.path == "/api/version") {
        json::FlatObject object;
        object.set("name", PANDORA_NAME);
        object.set("version", PANDORA_VERSION);
        object.set("platform", platform.name());
        object.set("data_root", platform.data_root());
        return Response::json(json::to_pretty_string(object));
    }

    if (request.path == "/api/fs/list") {
        const auto path = request.param("path");
        if (!path.has_value() || path->empty()) {
            return Response::json("{\"error\": \"the path parameter is required\"}\n", 400);
        }

        std::string error;
        const auto entries = platform.list_dir(*path, &error);
        if (!error.empty()) {
            return Response::json("{\"error\": \"" + json::escape(error) + "\"}\n", 404);
        }
        return Response::json(entries_to_json(*path, entries));
    }

    return Response::text("not found\n", 404);
}

#if defined(_WIN32)

bool HttpServer::run(std::string* error) {
    if (error != nullptr) {
        *error =
            "the HTTP server is not implemented for Windows; build for the host on "
            "Linux or macOS, or for the console";
    }
    return false;
}

#else

bool HttpServer::run(std::string* error) {
    const int listener = ::socket(AF_INET, SOCK_STREAM, 0);
    if (listener < 0) {
        if (error != nullptr) {
            *error = "cannot create the listening socket";
        }
        return false;
    }

    const int enable = 1;
    ::setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(enable));

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_port = htons(config_.port);
    if (::inet_pton(AF_INET, config_.bind_address.c_str(), &address.sin_addr) != 1) {
        ::close(listener);
        if (error != nullptr) {
            *error = "bind_address is not a valid IPv4 address: " + config_.bind_address;
        }
        return false;
    }

    if (::bind(listener, reinterpret_cast<sockaddr*>(&address), sizeof(address)) < 0) {
        ::close(listener);
        if (error != nullptr) {
            *error = "cannot bind " + config_.bind_address + ":" + std::to_string(config_.port);
        }
        return false;
    }

    if (::listen(listener, 16) < 0) {
        ::close(listener);
        if (error != nullptr) {
            *error = "cannot listen";
        }
        return false;
    }

    log(LogLevel::kInfo, "serving http://" + config_.bind_address + ":" +
                             std::to_string(config_.port) + " (" + platform_.name() + ")");

    for (;;) {
        const int client = ::accept(listener, nullptr, nullptr);
        if (client < 0) {
            continue;
        }

        char buffer[8192];
        const ssize_t bytes = ::recv(client, buffer, sizeof(buffer) - 1, 0);
        if (bytes > 0) {
            buffer[bytes] = '\0';

            Request request;
            const std::string head(buffer, static_cast<std::size_t>(bytes));
            const std::size_t first_space = head.find(' ');
            const std::size_t second_space = first_space == std::string::npos
                                                 ? std::string::npos
                                                 : head.find(' ', first_space + 1);

            if (first_space != std::string::npos && second_space != std::string::npos) {
                request.method = head.substr(0, first_space);
                request.target = head.substr(first_space + 1, second_space - first_space - 1);

                const std::size_t question = request.target.find('?');
                if (question == std::string::npos) {
                    request.path = request.target;
                } else {
                    request.path = request.target.substr(0, question);
                    request.query = request.target.substr(question + 1);
                }
            }

            const Response response = route_request(request, platform_);
            const std::string payload = response.serialize();

            std::size_t sent = 0;
            while (sent < payload.size()) {
                const ssize_t written =
                    ::send(client, payload.data() + sent, payload.size() - sent, 0);
                if (written <= 0) {
                    break;
                }
                sent += static_cast<std::size_t>(written);
            }
        }

        ::close(client);
    }

    return true;
}

#endif  // _WIN32

}  // namespace pandora
