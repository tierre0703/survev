#include "Localization.h"

#include <algorithm>
#include <cctype>

namespace ui {

const LocaleInfo kLocales[] = {
    {"da", "Dansk"},
    {"de", "Deutsch"},
    {"en", "English"},
    {"es", "Español"},
    {"fr", "Français"},
    {"it", "Italiano"},
    {"nl", "Nederlands"},
    {"pl", "Polski"},
    {"pt", "Português"},
    {"ru", "Русский"},
    {"sv", "Svenska"},
    {"vn", "Tiếng Việt"},
    {"tr", "Türkçe"},
    {"jp", "日本語"},
    {"ko", "한국어"},
    {"th", "ภาษาไทย"},
    {"zh-cn", "中文简体"},
    {"zh-tw", "中文繁體"},
};
const int kLocaleCount = static_cast<int>(sizeof(kLocales) / sizeof(kLocales[0]));

namespace {

std::map<std::string, std::string> flatten(const JsonValue& root) {
    std::map<std::string, std::string> table;
    if (!root.isObject()) {
        return table;
    }
    for (const auto& entry : root.object) {
        if (entry.second.isString()) {
            table[entry.first] = entry.second.string;
        } else if (entry.second.isNumber()) {
            // Localization files are strings; numbers are stringified for
            // robustness against hand-edited translations.
            const double d = entry.second.number;
            if (d == static_cast<long long>(d)) {
                table[entry.first] = std::to_string(static_cast<long long>(d));
            } else {
                table[entry.first] = std::to_string(d);
            }
        }
    }
    return table;
}

} // namespace

bool Localization::registerEnglish(const std::string& json) {
    JsonValue root;
    if (!JsonParser::parse(json, root) || !root.isObject()) {
        return false;
    }
    _tables["en"] = flatten(root);
    return true;
}

bool Localization::hasLocale(const std::string& locale) const {
    return _tables.find(locale) != _tables.end();
}

bool Localization::loadLocale(const std::string& locale) {
    if (locale.empty() || hasLocale(locale)) {
        return hasLocale(locale);
    }
    if (!_loader) {
        return false;
    }
    const std::string json = _loader(locale);
    if (json.empty()) {
        return false;
    }
    JsonValue root;
    if (!JsonParser::parse(json, root) || !root.isObject()) {
        return false;
    }
    _tables[locale] = flatten(root);
    return true;
}

void Localization::setLocale(const std::string& locale) {
    std::string next = locale;
    bool known = false;
    for (int i = 0; i < kLocaleCount; i++) {
        if (next == kLocales[i].code) {
            known = true;
            break;
        }
    }
    if (!known) {
        next = "en";
    }
    loadLocale(next);
    _locale = next;
}

std::string Localization::get(const std::string& locale, const std::string& key) const {
    auto it = _tables.find(locale);
    if (it == _tables.end()) {
        return {};
    }
    auto e = it->second.find(key);
    if (e == it->second.end()) {
        return {};
    }
    return e->second;
}

std::string Localization::translate(const std::string& key) const {
    // Also try spaces as dashes (mirrors localization.ts).
    std::string spaced = key;
    const std::size_t space = spaced.find(' ');
    if (space != std::string::npos) {
        spaced[space] = '-';
    }
    std::string out = get(_locale, key);
    if (out.empty() && spaced != key) {
        out = get(_locale, spaced);
    }
    if (out.empty()) {
        out = get("en", key);
    }
    if (out.empty() && spaced != key) {
        out = get("en", spaced);
    }
    return out;
}

std::string Localization::localeName(const std::string& code) const {
    for (int i = 0; i < kLocaleCount; i++) {
        if (code == kLocales[i].code) {
            return kLocales[i].name;
        }
    }
    return code;
}

std::string Localization::detectLocale(const std::string& languageTag) const {
    std::string tag = languageTag;
    std::transform(tag.begin(), tag.end(), tag.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    // Prefer full codes first (zh-cn/zh-tw) before the 2-letter wildcards.
    for (int i = 0; i < kLocaleCount; i++) {
        if (tag.find(kLocales[i].code) != std::string::npos) {
            return kLocales[i].code;
        }
    }
    static const char* wildcards[] = { "pt", "de", "es", "fr", "ko", "ru", "en" };
    for (const char* w : wildcards) {
        if (tag.find(w) != std::string::npos) {
            return w;
        }
    }
    return "en";
}

} // namespace ui
