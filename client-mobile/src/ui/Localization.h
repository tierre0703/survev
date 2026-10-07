#pragma once
// Engine-independent port of client/src/ui/localization.ts.
//
// The web client keeps English compiled in and lazily fetches
// `l10n/<locale>.json` on locale change. The native client ships every
// `l10n/*.json` in the APK, so it can register a loader that reads from the
// asset root (`ui::Files` on the app side) or from a plain directory (host
// tests). `translate(key)` mirrors the web client: exact key, `key` with the
// first space turned into a dash, then the English fallback, else "".
#include "Json.h"

#include <functional>
#include <map>
#include <string>
#include <vector>

namespace ui {

// Locale codes the web client accepts (localization.ts Locales).
struct LocaleInfo {
    const char* code;
    const char* name;
};
extern const LocaleInfo kLocales[];
extern const int kLocaleCount;

class Localization {
public:
    // `loader` returns the JSON text for `l10n/<locale>.json` (or "" when
    // missing). English is registered eagerly by `registerEnglish`.
    using Loader = std::function<std::string(const std::string& locale)>;

    Localization() = default;

    void setLoader(Loader loader) { _loader = std::move(loader); }

    // Registers the built-in English table (client/src/en.json is large; the
    // app passes the file contents, tests pass a small table).
    bool registerEnglish(const std::string& json);
    // Optional: snapshot the same English table from the JSON asset.
    bool loadLocale(const std::string& locale);

    // Mirrors Localization.setLocale: falls back to "en" for unknown locales.
    void setLocale(const std::string& locale);
    const std::string& getLocale() const { return _locale; }

    // Mirrors Localization.translate.
    std::string translate(const std::string& key) const;

    // Language list for the settings dropdown.
    const LocaleInfo* locales() const { return kLocales; }
    int localeCount() const { return kLocaleCount; }
    // Display name for a code ("en" -> "English").
    std::string localeName(const std::string& code) const;
    // Best-effort locale from a BCP-47 language tag ("zh-Hans" -> "zh-cn").
    std::string detectLocale(const std::string& languageTag) const;

    bool hasLocale(const std::string& locale) const;

private:
    std::string get(const std::string& locale, const std::string& key) const;

    Loader _loader;
    std::string _locale = "en";
    std::map<std::string, std::map<std::string, std::string>> _tables;
};

} // namespace ui
