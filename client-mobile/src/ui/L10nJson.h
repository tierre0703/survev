#pragma once
// Engine-independent helper that flattens a localization table into the JSON
// document the Java UI consumes over the JNI bridge (ARCHITECTURE.md §5.3).
//
// The web client keeps `l10n/*.json` as a nested document (sections -> keys);
// `ui::Localization` flattens it once per locale. Java has no access to that
// map, so `SurvBridge.getL10nJson()` hands it the same table re-encoded as a
// flat `{"key": "value"}` object. Keeping the encoder here (rather than in
// `Localization.cpp`) means the host tests can assert the exact bytes without
// the engine, and Java can be sure of one canonical shape.
#include "Json.h"

#include <map>
#include <string>

namespace ui {

// Serializes `table` as a flat JSON object with deterministic key order (the
// `std::map` order, so the same table always produces the same text). Values
// are escaped with the shared `jsonToString` encoder via a synthetic JsonValue
// tree, so the two stay byte-compatible.
std::string l10nTableToJson(const std::map<std::string, std::string>& table);

} // namespace ui