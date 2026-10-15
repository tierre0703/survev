#pragma once
// Engine-independent re-encoding of the bundled news entries (ui/News.h) into
// the JSON document the Java menu consumes over the bridge (`SurvBridge.
// getNewsJson()`, ARCHITECTURE.md §5.1 move matrix: "News.{h,cpp} keep as
// data, rendered by Java").
//
// Shape (stable; the Java `MenuFragment` + the host tests both depend on it):
//   [{"date":"2026-10-14","title":"...","href":"","paragraphs":
//      [{"text":"...","style":"normal|highlight|redacted"}]}, ...]
#include "News.h"

#include <string>

namespace ui {

// Newest entry first, exactly like `newsEntries()`.
std::string newsToJson(const std::vector<NewsEntry>& entries);

} // namespace ui