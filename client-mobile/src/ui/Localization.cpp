#include "Localization.h"

#include <algorithm>
#include <cctype>

namespace ui {

const LocaleInfo kLocales[] = {
    {"en", "English"},
    {"fr", "Français"},
    {"jp", "日本語"},
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

std::map<std::string, std::string> Localization::flattened() const {
    // English first, then the active locale on top: exactly the lookup order
    // `translate` uses, so a key that only exists in en.json is still visible
    // to the Java UI (it would otherwise render its raw key).
    std::map<std::string, std::string> table;
    auto english = _tables.find("en");
    if (english != _tables.end()) {
        table = english->second;
    }
    if (_locale != "en") {
        auto current = _tables.find(_locale);
        if (current != _tables.end()) {
            for (const auto& entry : current->second) {
                table[entry.first] = entry.second;
            }
        }
    }
    return table;
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
    // Prefer the shipped codes first (en/fr/jp), then their BCP-47 aliases.
    for (int i = 0; i < kLocaleCount; i++) {
        if (tag.find(kLocales[i].code) != std::string::npos) {
            return kLocales[i].code;
        }
    }
    if (tag.rfind("ja", 0) == 0) {
        return "jp"; // BCP-47 uses "ja"; the web locale file is "jp".
    }
    if (tag.rfind("fr", 0) == 0) {
        return "fr";
    }
    return "en";
}

} // namespace ui
