// sce_sys/param.json: the metadata the console reads to install a title.

#pragma once

#include <optional>
#include <string>
#include <string_view>

#include "pandora/version.hpp"

namespace pandora {

/// The boilerplate's placeholder. Submissions must pick a real, free title id.
inline constexpr std::string_view kReservedTitleId = "PPSA99999";

/// The subset of param.json this project reads and writes.
struct ParamJson {
    std::string title_id;
    std::string content_version;
    std::optional<std::string> master_version;

    /// Parses the fields above, ignoring any others.
    static std::optional<ParamJson> parse(std::string_view text, std::string* error);

    std::string to_json() const;

    /// Checks the title id and the version format.
    bool validate(std::string* error) const;

    std::optional<ContentVersion> parsed_content_version() const;
};

/// Checks a title id: four uppercase letters then five digits.
///
/// Also rejects the ranges that belong to other things: CUSA is PS4 (a PS5
/// title with a CUSA id will not install), and NPXS is reserved for system
/// software.
bool validate_title_id(std::string_view id, std::string* error);

}  // namespace pandora
