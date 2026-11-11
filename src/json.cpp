#include "pandora/json.hpp"

#include <cctype>
#include <cstdio>

namespace pandora::json {
namespace {

void skip_whitespace(std::string_view text, std::size_t& index) {
    while (index < text.size() && std::isspace(static_cast<unsigned char>(text[index])) != 0) {
        ++index;
    }
}

bool fail(std::string* error, std::string message) {
    if (error != nullptr) {
        *error = std::move(message);
    }
    return false;
}

bool parse_string(std::string_view text, std::size_t& index, std::string& out, std::string* error) {
    if (index >= text.size() || text[index] != '"') {
        return fail(error, "expected a string");
    }
    ++index;
    out.clear();

    while (index < text.size()) {
        const char c = text[index];
        if (c == '"') {
            ++index;
            return true;
        }
        if (c != '\\') {
            out.push_back(c);
            ++index;
            continue;
        }

        ++index;
        if (index >= text.size()) {
            return fail(error, "unterminated escape sequence");
        }
        switch (text[index]) {
            case '"':
                out.push_back('"');
                break;
            case '\\':
                out.push_back('\\');
                break;
            case '/':
                out.push_back('/');
                break;
            case 'b':
                out.push_back('\b');
                break;
            case 'f':
                out.push_back('\f');
                break;
            case 'n':
                out.push_back('\n');
                break;
            case 'r':
                out.push_back('\r');
                break;
            case 't':
                out.push_back('\t');
                break;
            case 'u':
                // Only ASCII is needed by this project. Rejecting the escape is
                // better than silently mangling non-ASCII text.
                return fail(error, "\\u escapes are not supported");
            default:
                return fail(error, "unknown escape sequence");
        }
        ++index;
    }

    return fail(error, "unterminated string");
}

}  // namespace

void FlatObject::set(std::string key, std::string value) {
    fields_.push_back(Field{std::move(key), std::optional<std::string>(std::move(value))});
}

void FlatObject::set_null(std::string key) {
    fields_.push_back(Field{std::move(key), std::nullopt});
}

std::optional<std::string> FlatObject::get(std::string_view key) const {
    for (const Field& field : fields_) {
        if (field.key == key) {
            return field.value;
        }
    }
    return std::nullopt;
}

bool FlatObject::has(std::string_view key) const {
    for (const Field& field : fields_) {
        if (field.key == key) {
            return true;
        }
    }
    return false;
}

bool FlatObject::replace(std::string_view key, std::string value) {
    for (Field& field : fields_) {
        if (field.key == key) {
            field.value = std::move(value);
            return true;
        }
    }
    return false;
}

std::optional<FlatObject> parse_flat_object(std::string_view text, std::string* error) {
    std::size_t index = 0;
    skip_whitespace(text, index);

    if (index >= text.size() || text[index] != '{') {
        fail(error, "expected '{' at the start of the document");
        return std::nullopt;
    }
    ++index;
    skip_whitespace(text, index);

    FlatObject object;
    if (index < text.size() && text[index] == '}') {
        ++index;
        skip_whitespace(text, index);
        if (index != text.size()) {
            fail(error, "trailing data after the object");
            return std::nullopt;
        }
        return object;
    }

    while (true) {
        skip_whitespace(text, index);

        std::string key;
        if (!parse_string(text, index, key, error)) {
            return std::nullopt;
        }

        skip_whitespace(text, index);
        if (index >= text.size() || text[index] != ':') {
            fail(error, "expected ':' after a key");
            return std::nullopt;
        }
        ++index;
        skip_whitespace(text, index);

        if (index >= text.size()) {
            fail(error, "unexpected end of input");
            return std::nullopt;
        }

        if (text.compare(index, 4, "null") == 0) {
            index += 4;
            object.set_null(std::move(key));
        } else {
            std::string value;
            if (!parse_string(text, index, value, error)) {
                return std::nullopt;
            }
            object.set(std::move(key), std::move(value));
        }

        skip_whitespace(text, index);
        if (index >= text.size()) {
            fail(error, "unexpected end of the object");
            return std::nullopt;
        }
        if (text[index] == ',') {
            ++index;
            continue;
        }
        if (text[index] == '}') {
            ++index;
            break;
        }
        fail(error, "expected ',' or '}'");
        return std::nullopt;
    }

    skip_whitespace(text, index);
    if (index != text.size()) {
        fail(error, "trailing data after the object");
        return std::nullopt;
    }
    return object;
}

std::string escape(std::string_view text) {
    std::string out;
    out.reserve(text.size() + 2);
    for (const char c : text) {
        switch (c) {
            case '"':
                out += "\\\"";
                break;
            case '\\':
                out += "\\\\";
                break;
            case '\n':
                out += "\\n";
                break;
            case '\r':
                out += "\\r";
                break;
            case '\t':
                out += "\\t";
                break;
            default:
                if (static_cast<unsigned char>(c) < 0x20U) {
                    char buffer[8];
                    std::snprintf(buffer, sizeof(buffer), "\\u%04X",
                                  static_cast<unsigned>(static_cast<unsigned char>(c)));
                    out += buffer;
                } else {
                    out.push_back(c);
                }
        }
    }
    return out;
}

std::string to_pretty_string(const FlatObject& object) {
    std::string out = "{\n";
    for (std::size_t i = 0; i < object.fields().size(); ++i) {
        const Field& field = object.fields()[i];
        out += "  \"";
        out += escape(field.key);
        out += "\": ";
        if (field.value.has_value()) {
            out += '"';
            out += escape(*field.value);
            out += '"';
        } else {
            out += "null";
        }
        if (i + 1 < object.fields().size()) {
            out += ',';
        }
        out += '\n';
    }
    out += "}\n";
    return out;
}

}  // namespace pandora::json
