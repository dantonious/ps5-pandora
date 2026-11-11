#include "pandora/param.hpp"

#include "pandora/json.hpp"

namespace pandora {
namespace {

bool fail(std::string* error, std::string message) {
    if (error != nullptr) {
        *error = std::move(message);
    }
    return false;
}

}  // namespace

bool validate_title_id(std::string_view id, std::string* error) {
    if (id.size() != 9) {
        return fail(error, "title id must be exactly 9 characters");
    }
    for (std::size_t i = 0; i < 4; ++i) {
        if (id[i] < 'A' || id[i] > 'Z') {
            return fail(error, "the first four characters must be uppercase letters");
        }
    }
    for (std::size_t i = 4; i < 9; ++i) {
        if (id[i] < '0' || id[i] > '9') {
            return fail(error, "the last five characters must be digits");
        }
    }
    if (id.substr(0, 4) == "CUSA") {
        return fail(error, "CUSA belongs to PS4 titles");
    }
    if (id.substr(0, 4) == "NPXS") {
        return fail(error, "NPXS belongs to system titles");
    }
    if (id == kReservedTitleId) {
        return fail(error, "PPSA99999 is reserved by the native-app boilerplate");
    }
    return true;
}

std::optional<ParamJson> ParamJson::parse(std::string_view text, std::string* error) {
    const auto document = json::parse_flat_object(text, error);
    if (!document.has_value()) {
        return std::nullopt;
    }

    ParamJson param;
    const auto title_id = document->get("titleId");
    if (!title_id.has_value()) {
        fail(error, "titleId is missing or null");
        return std::nullopt;
    }
    param.title_id = *title_id;

    const auto content_version = document->get("contentVersion");
    if (!content_version.has_value()) {
        fail(error, "contentVersion is missing or null");
        return std::nullopt;
    }
    param.content_version = *content_version;

    // masterVersion is optional. A present-but-null value means absent.
    param.master_version = document->get("masterVersion");

    return param;
}

std::string ParamJson::to_json() const {
    json::FlatObject object;
    object.set("titleId", title_id);
    object.set("contentVersion", content_version);
    if (master_version.has_value()) {
        object.set("masterVersion", *master_version);
    }
    return json::to_pretty_string(object);
}

bool ParamJson::validate(std::string* error) const {
    std::string reason;
    if (!validate_title_id(title_id, &reason)) {
        return fail(error, reason);
    }
    if (!parsed_content_version().has_value()) {
        return fail(error, "contentVersion must look like NN.NNN.NNN, e.g. 01.000.070");
    }
    return true;
}

std::optional<ContentVersion> ParamJson::parsed_content_version() const {
    return ContentVersion::parse(content_version);
}

}  // namespace pandora
