// ps5-pandora — PlayStation contentVersion handling.
//
// A contentVersion is the version the console itself reports for an installed
// title, and it is how a console decides whether an installed copy is outdated.
// Release tags cannot do that job: they have no common format and no defined
// order. The format is fixed at NN.NNN.NNN and is compared as three numbers.

#pragma once

#include <compare>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

namespace pandora {

/// A PlayStation content version, always rendered NN.NNN.NNN.
class ContentVersion {
public:
    /// Largest value a three-digit field can hold.
    static constexpr std::uint16_t kMaxField = 999;
    /// Largest value the two-digit major field can hold.
    static constexpr std::uint16_t kMaxMajor = 99;

    constexpr ContentVersion() = default;
    constexpr ContentVersion(std::uint16_t major, std::uint16_t minor, std::uint16_t patch)
        : major_(major), minor_(minor), patch_(patch) {}

    /// Parses NN.NNN.NNN, rejecting anything else.
    ///
    /// Field widths are enforced exactly: "1.0.0" and "01.0.0" are rejected,
    /// because the console treats a malformed value as unknown.
    static std::optional<ContentVersion> parse(std::string_view text);

    /// Renders NN.NNN.NNN.
    std::string str() const;

    /// Next patch, the usual choice for a release. Clamped at 999.
    ContentVersion bump_patch() const;
    /// Next minor: minor + 1, patch reset to 0.
    ContentVersion bump_minor() const;
    /// Next major: major + 1, the rest reset to 0.
    ContentVersion bump_major() const;

    constexpr std::uint16_t major() const { return major_; }
    constexpr std::uint16_t minor() const { return minor_; }
    constexpr std::uint16_t patch() const { return patch_; }

    friend constexpr bool operator==(const ContentVersion&, const ContentVersion&) = default;
    friend constexpr auto operator<=>(const ContentVersion&, const ContentVersion&) = default;

private:
    std::uint16_t major_ = 1;
    std::uint16_t minor_ = 0;
    std::uint16_t patch_ = 0;
};

}  // namespace pandora
