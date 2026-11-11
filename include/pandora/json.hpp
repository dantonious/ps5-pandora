// A deliberately small JSON reader for the flat string maps this project uses.
//
// sce_sys/param.json and the local config are both flat objects whose values are
// strings or null. That is the whole requirement, so the reader handles exactly
// that and rejects everything else instead of guessing. A full JSON library
// would be a dependency to ship, patch and audit for no benefit here.

#pragma once

#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace pandora::json {

struct Field {
    std::string key;
    std::optional<std::string> value;  // nullopt represents JSON null
};

class FlatObject {
public:
    void set(std::string key, std::string value);

    /// Stores a key whose value is JSON null.
    void set_null(std::string key);

    /// Returns the value, or nullopt when the key is absent or null.
    std::optional<std::string> get(std::string_view key) const;

    /// True when the key is present, even if its value is null.
    bool has(std::string_view key) const;

    /// Overwrites the value of a key that already exists, leaving its position
    /// unchanged. Returns false when the key is absent.
    bool replace(std::string_view key, std::string value);

    const std::vector<Field>& fields() const { return fields_; }

private:
    std::vector<Field> fields_;
};

/// Parses a flat object. On failure returns nullopt and fills \p error.
std::optional<FlatObject> parse_flat_object(std::string_view text, std::string* error);

/// Escapes a string for embedding in JSON output.
std::string escape(std::string_view text);

/// Renders a flat object with two-space indentation and a trailing newline.
std::string to_pretty_string(const FlatObject& object);

}  // namespace pandora::json
