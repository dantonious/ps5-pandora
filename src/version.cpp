#include "pandora/version.hpp"

namespace pandora {
namespace {

bool all_digits(std::string_view text) {
    if (text.empty()) {
        return false;
    }
    for (const char c : text) {
        if (c < '0' || c > '9') {
            return false;
        }
    }
    return true;
}

/// Adds one, clamping at the field's maximum.
///
/// The clamp is on the field width (999 for three digits, 99 for two), not on
/// the integer type: 01.000.1000 would render as an unparseable version.
std::uint16_t clamp_add(std::uint16_t value, std::uint16_t maximum) {
    return value >= maximum ? maximum : static_cast<std::uint16_t>(value + 1);
}

std::uint16_t to_u16(std::string_view digits) {
    unsigned value = 0;
    for (const char c : digits) {
        value = value * 10U + static_cast<unsigned>(c - '0');
    }
    return static_cast<std::uint16_t>(value);
}

}  // namespace

std::optional<ContentVersion> ContentVersion::parse(std::string_view text) {
    const std::size_t first = text.find('.');
    if (first == std::string_view::npos) {
        return std::nullopt;
    }
    const std::size_t second = text.find('.', first + 1);
    if (second == std::string_view::npos) {
        return std::nullopt;
    }
    if (text.find('.', second + 1) != std::string_view::npos) {
        return std::nullopt;
    }

    const std::string_view major = text.substr(0, first);
    const std::string_view minor = text.substr(first + 1, second - first - 1);
    const std::string_view patch = text.substr(second + 1);

    if (major.size() != 2 || minor.size() != 3 || patch.size() != 3) {
        return std::nullopt;
    }
    if (!all_digits(major) || !all_digits(minor) || !all_digits(patch)) {
        return std::nullopt;
    }

    return ContentVersion{to_u16(major), to_u16(minor), to_u16(patch)};
}

std::string ContentVersion::str() const {
    // Built field by field rather than with snprintf. A single "%.2u.%03u.%03u"
    // cannot be proven to fit, because the fields are wider than the format
    // says, and "it cannot happen" is not a guarantee. Here each field is
    // padded to its width and a value that somehow carries more digits renders
    // them all, so an out-of-range version is visible instead of truncated.
    auto field = [](std::uint16_t value, std::size_t width) {
        std::string digits = std::to_string(value);
        if (digits.size() < width) {
            digits.insert(0, width - digits.size(), '0');
        }
        return digits;
    };

    return field(major_, 2) + "." + field(minor_, 3) + "." + field(patch_, 3);
}

ContentVersion ContentVersion::bump_patch() const {
    return ContentVersion{major_, minor_, clamp_add(patch_, kMaxField)};
}

ContentVersion ContentVersion::bump_minor() const {
    return ContentVersion{major_, clamp_add(minor_, kMaxField), 0};
}

ContentVersion ContentVersion::bump_major() const {
    return ContentVersion{clamp_add(major_, kMaxMajor), 0, 0};
}

}  // namespace pandora
