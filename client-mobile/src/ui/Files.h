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

// Bundled GUI font families -> the file base names staged in Content/fonts.
// tools/fetch-fonts.ps1 writes `RobotoCondensed-Normal.ttf` and
// `RobotoCondensed-Bold.ttf` (the web client only ships woff2, which axmol
// cannot load).
struct GuiFontFile {
    const char* file;   // Content/fonts/<file>
    const char* alias;  // family the UI asks for
};
inline constexpr int kGuiFontCount = 2;
inline constexpr GuiFontFile kGuiFontFiles[kGuiFontCount] = {
    {"RobotoCondensed-Normal.ttf", "RobotoCondensed"},
    {"RobotoCondensed-Bold.ttf", "RobotoCondensed-Bold"},
};

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
    // Warms up the bundled GUI TTF faces (see `kGuiFontFiles`) up front so the
    // first Label does not pay the parse cost. Safe to call when the fonts are
    // absent (the UI then falls back to the system font).
    static void registerFonts(const std::string& dir);
    // Resolves a requested font family + weight to a staged TTF path ("" when
    // the TTF build is not present, so callers fall back to the system font).
    static std::string fontFace(const std::string& family, bool bold = false);
};

} // namespace ui

// App-facing alias so the entry point can register the GUI font without pulling
// in `ui::Files` from a translation unit that also has `using namespace ax`
// (where `surv::ui` and `ax::ui` are ambiguous).
namespace surv { namespace ui {
void registerGuiFonts(const char* dir);
} } // namespace surv::ui
