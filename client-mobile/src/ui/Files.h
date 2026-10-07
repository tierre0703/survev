#pragma once
// Engine-facing file access for the native UI layer.
//
// Everything the UI reads lives under `Content/` (staged into the APK assets):
// `l10n/*.json`, `img/**`, `audio/**`, `fonts/**`. `ui::Files` wraps
// `ax::FileUtils` so the rest of the UI code never has to think about the
// platform asset root, and adds the relative-path resolution the UI needs when
// a JSON/localization document references another asset (e.g. `url(../img/x)`
// or a sibling file next to the document).
//
// This header is the only UI header that needs the engine; the pure logic
// (Json.h, Config.h, Localization.h) stays engine-free so the host tests link
// without axmol.
#include "Json.h"

#include <string>

namespace ax {
class FileUtils;
}

namespace ui {

class Files {
public:
    // Reads `path` as text (empty when missing), resolving relative to the
    // Content root.
    static std::string readText(const std::string& path);
    // Reads `path` as bytes.
    static bool readBytes(const std::string& path, std::string& out);
    // Reads + parses JSON. Returns false when the file is missing or malformed.
    static bool readJson(const std::string& path, JsonValue& out);
    // True when the asset exists (Content-root relative, or an absolute path).
    static bool exists(const std::string& path);
    // Resolves `ref` relative to the directory of `documentPath` (a path that
    // was itself resolved against the Content root).
    static std::string resolveRelative(const std::string& documentPath, const std::string& ref);
    // Absolute (platform) path for a Content-root-relative name; empty if
    // unresolvable.
    static std::string fullPath(const std::string& path);
};

} // namespace ui
