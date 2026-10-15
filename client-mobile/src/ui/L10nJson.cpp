#include "L10nJson.h"
#include "Config.h" // jsonToString (the shared encoder)

namespace ui {

std::string l10nTableToJson(const std::map<std::string, std::string>& table) {
    // Reuse the config blob's encoder so both documents escape strings the same
    // way (quotes/backslashes/control characters).
    JsonValue root;
    root.kind = JsonValue::Kind::Object;
    for (const auto& entry : table) {
        JsonValue value;
        value.kind = JsonValue::Kind::String;
        value.string = entry.second;
        root.object[entry.first] = std::move(value);
    }
    return jsonToString(root);
}

} // namespace ui
