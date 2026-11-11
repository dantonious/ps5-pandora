#include "pandora/version.hpp"
#include "support/check.hpp"

using pandora::ContentVersion;

int main() {
    // Round trip and field widths.
    const auto parsed = ContentVersion::parse("01.000.070");
    EXPECT(parsed.has_value());
    EXPECT_STR_EQ("01.000.070", parsed->str());
    EXPECT_EQ(1, parsed->major());
    EXPECT_EQ(0, parsed->minor());
    EXPECT_EQ(70, parsed->patch());

    // Rejections: the console treats a malformed value as unknown, so being
    // strict here is the correct behaviour rather than pedantry.
    EXPECT(!ContentVersion::parse("1.000.070").has_value());
    EXPECT(!ContentVersion::parse("01.00.070").has_value());
    EXPECT(!ContentVersion::parse("01.000.07").has_value());
    EXPECT(!ContentVersion::parse("01.000").has_value());
    EXPECT(!ContentVersion::parse("01.000.070.1").has_value());
    EXPECT(!ContentVersion::parse("aa.000.070").has_value());
    EXPECT(!ContentVersion::parse("").has_value());
    EXPECT(!ContentVersion::parse("01.000.07x").has_value());

    // Ordering is numeric, not lexicographic on the raw text.
    EXPECT(ContentVersion::parse("01.002.000").value() <
           ContentVersion::parse("01.010.000").value());
    EXPECT(ContentVersion::parse("02.000.000").value() >
           ContentVersion::parse("01.999.999").value());
    EXPECT(ContentVersion::parse("01.000.010").value() >
           ContentVersion::parse("01.000.009").value());

    // Bumps.
    EXPECT_STR_EQ("01.000.001", ContentVersion::parse("01.000.000")->bump_patch().str());
    EXPECT_STR_EQ("01.001.000", ContentVersion::parse("01.000.070")->bump_minor().str());
    EXPECT_STR_EQ("02.000.000", ContentVersion::parse("01.500.700")->bump_major().str());

    // The clamp follows the field width, not the integer type. Before this was
    // fixed, bumping 999 produced 01.000.1000, which is not a valid version.
    EXPECT_STR_EQ("01.000.999", ContentVersion::parse("01.000.999")->bump_patch().str());
    EXPECT_STR_EQ("01.999.000", ContentVersion::parse("01.999.010")->bump_minor().str());
    EXPECT_STR_EQ("99.000.000", ContentVersion::parse("99.010.000")->bump_major().str());

    // Whatever a bump produces must parse again.
    const ContentVersion corner{99, 999, 999};
    EXPECT(ContentVersion::parse(corner.bump_patch().str()).has_value());
    EXPECT(ContentVersion::parse(corner.bump_minor().str()).has_value());
    EXPECT(ContentVersion::parse(corner.bump_major().str()).has_value());

    return pandora::test::summary("version");
}
