#include "pandora/json.hpp"
#include "support/check.hpp"

using pandora::json::FlatObject;

int main() {
    std::string error;

    const auto object = pandora::json::parse_flat_object(
        R"({"titleId": "PPSA77010", "contentVersion": "01.000.070"})", &error);
    EXPECT(object.has_value());
    EXPECT_STR_EQ("PPSA77010", object->get("titleId").value_or("<missing>"));
    EXPECT_STR_EQ("01.000.070", object->get("contentVersion").value_or("<missing>"));
    EXPECT(object->has("titleId"));
    EXPECT(!object->has("nope"));
    EXPECT(!object->get("nope").has_value());

    // null is a present key with no value.
    const auto with_null = pandora::json::parse_flat_object(R"({"a": null})", &error);
    EXPECT(with_null.has_value());
    EXPECT(with_null->has("a"));
    EXPECT(!with_null->get("a").has_value());

    // Whitespace and an empty object.
    EXPECT(pandora::json::parse_flat_object("  {  }  ", &error).has_value());
    EXPECT(pandora::json::parse_flat_object("\n{\n \"a\" : \"b\"\n}\n", &error).has_value());

    // Escapes.
    const auto escaped = pandora::json::parse_flat_object(R"({"path": "a\/b\nc"})", &error);
    EXPECT(escaped.has_value());
    EXPECT_STR_EQ("a/b\nc", escaped->get("path").value_or("<missing>"));

    // Malformed input is rejected, not patched up.
    EXPECT(!pandora::json::parse_flat_object("", &error).has_value());
    EXPECT(!pandora::json::parse_flat_object("[]", &error).has_value());
    EXPECT(!pandora::json::parse_flat_object(R"({"a": })", &error).has_value());
    EXPECT(!pandora::json::parse_flat_object(R"({"a" "b"})", &error).has_value());
    EXPECT(!pandora::json::parse_flat_object(R"({"a": "b")", &error).has_value());
    EXPECT(!pandora::json::parse_flat_object(R"({"a": "b"} trailing)", &error).has_value());
    EXPECT(!pandora::json::parse_flat_object(R"({"a": 12})", &error).has_value());
    EXPECT(!pandora::json::parse_flat_object(R"({"a": "\u0041"})", &error).has_value());
    EXPECT(!error.empty());

    // replace keeps the key's position and reports an absent key.
    FlatObject editable;
    editable.set("first", "1");
    editable.set("second", "2");
    EXPECT(editable.replace("second", "22"));
    EXPECT(!editable.replace("third", "3"));
    EXPECT_STR_EQ("22", editable.get("second").value_or("<missing>"));
    EXPECT_EQ(2u, editable.fields().size());
    EXPECT_STR_EQ("first", editable.fields()[0].key);

    // Output is valid JSON that we can read back.
    FlatObject out;
    out.set("titleId", "PPSA77010");
    out.set_null("masterVersion");
    out.set("note", "quote \" and backslash \\");
    const std::string rendered = pandora::json::to_pretty_string(out);
    const auto reparsed = pandora::json::parse_flat_object(rendered, &error);
    EXPECT(reparsed.has_value());
    EXPECT_STR_EQ("PPSA77010", reparsed->get("titleId").value_or("<missing>"));
    EXPECT(reparsed->has("masterVersion"));
    EXPECT_STR_EQ("quote \" and backslash \\", reparsed->get("note").value_or("<missing>"));

    // Control characters are escaped, not emitted raw.
    EXPECT_STR_EQ("a\\nb", pandora::json::escape("a\nb"));

    return pandora::test::summary("json");
}
