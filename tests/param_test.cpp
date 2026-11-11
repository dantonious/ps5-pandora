#include "pandora/param.hpp"
#include "support/check.hpp"

using pandora::ParamJson;

int main() {
    std::string error;

    EXPECT(pandora::validate_title_id("PPSA77010", &error));
    EXPECT(pandora::validate_title_id("ABCD12345", &error));
    EXPECT(!pandora::validate_title_id("PPSA7701", &error));
    EXPECT(!pandora::validate_title_id("PPSA770100", &error));
    EXPECT(!pandora::validate_title_id("ppsa77010", &error));
    EXPECT(!pandora::validate_title_id("PPS077010", &error));  // digit in the letter block
    EXPECT(!pandora::validate_title_id("PPSA7701O", &error));  // letter in the digit block
    EXPECT(!pandora::validate_title_id("", &error));
    EXPECT(!error.empty());

    // The reserved ranges are rejected for reasons the user can read.
    EXPECT(!pandora::validate_title_id("CUSA12345", &error));
    EXPECT(error.find("PS4") != std::string::npos);
    EXPECT(!pandora::validate_title_id("NPXS12345", &error));
    EXPECT(!pandora::validate_title_id("PPSA99999", &error));

    const auto param = ParamJson::parse(
        R"({"titleId": "PPSA77010", "contentVersion": "01.000.070", "category": "utilities"})",
        &error);
    EXPECT(param.has_value());
    EXPECT_STR_EQ("PPSA77010", param->title_id);
    EXPECT_STR_EQ("01.000.070", param->content_version);
    EXPECT(!param->master_version.has_value());
    EXPECT(param->validate(&error));

    // Unknown fields are ignored, not rejected: param.json carries more than we use.
    // Round trip through to_json.
    const std::string rendered = param->to_json();
    const auto reparsed = ParamJson::parse(rendered, &error);
    EXPECT(reparsed.has_value());
    EXPECT_STR_EQ(param->title_id, reparsed->title_id);
    EXPECT_STR_EQ(param->content_version, reparsed->content_version);

    // masterVersion survives when present and null when absent.
    const auto with_master = ParamJson::parse(
        R"({"titleId": "PPSA77010", "contentVersion": "01.000.070", "masterVersion": "01.000.000"})",
        &error);
    EXPECT(with_master.has_value());
    EXPECT(with_master->master_version.has_value());
    EXPECT(with_master->to_json().find("masterVersion") != std::string::npos);

    const auto without_master =
        ParamJson::parse(R"({"titleId": "PPSA77010", "contentVersion": "01.000.070"})", &error);
    EXPECT(without_master->to_json().find("masterVersion") == std::string::npos);

    // Missing required fields.
    EXPECT(!ParamJson::parse(R"({"contentVersion": "01.000.070"})", &error).has_value());
    EXPECT(!ParamJson::parse(R"({"titleId": "PPSA77010"})", &error).has_value());
    EXPECT(!ParamJson::parse(R"({"titleId": null, "contentVersion": "01.000.070"})", &error)
                .has_value());

    // A well-formed document can still be invalid.
    const auto bad_version =
        ParamJson::parse(R"({"titleId": "PPSA77010", "contentVersion": "1.0.0"})", &error);
    EXPECT(bad_version.has_value());
    EXPECT(!bad_version->validate(&error));

    return pandora::test::summary("param");
}
