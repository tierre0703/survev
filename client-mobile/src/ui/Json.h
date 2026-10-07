#pragma once
// Minimal, engine-independent JSON reader for the native UI layer.
//
// The web client loads configuration and localization over `fetch`. The native
// client reads the same files out of the APK assets, so we need a tiny JSON
// parser that does not depend on axmol (host tests) and returns plain C++
// values with the keys the UI cares about.
//
// Values are parsed into a `JsonValue` tree (objects, arrays, numbers, strings,
// booleans, null). Object lookup is linear, which is fine for the small
// `l10n/*.json` and `site_info` documents the UI reads.
#include <cctype>
#include <cmath>
#include <cstddef>
#include <cstdlib>
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace ui {

class JsonValue {
public:
    enum class Kind { Null, Bool, Number, String, Array, Object };

    Kind kind = Kind::Null;
    bool boolean = false;
    double number = 0.0;
    std::string string;
    std::vector<JsonValue> array;
    std::map<std::string, JsonValue> object;

    bool isNull() const { return kind == Kind::Null; }
    bool isObject() const { return kind == Kind::Object; }
    bool isArray() const { return kind == Kind::Array; }
    bool isString() const { return kind == Kind::String; }
    bool isNumber() const { return kind == Kind::Number; }

    double asNumber(double fallback = 0.0) const {
        if (kind == Kind::Number) {
            return number;
        }
        if (kind == Kind::Bool) {
            return boolean ? 1.0 : 0.0;
        }
        if (kind == Kind::String) {
            try {
                return std::stod(string);
            } catch (...) {
                return fallback;
            }
        }
        return fallback;
    }
    float asFloat(float fallback = 0.0f) const {
        return static_cast<float>(asNumber(static_cast<double>(fallback)));
    }
    int asInt(int fallback = 0) const {
        return static_cast<int>(std::lround(asNumber(static_cast<double>(fallback))));
    }
    bool asBool(bool fallback = false) const {
        if (kind == Kind::Bool) {
            return boolean;
        }
        if (kind == Kind::Number) {
            return number != 0.0;
        }
        if (kind == Kind::Null) {
            return fallback;
        }
        return true;
    }
    const std::string& asString() const { return string; }

    // Object/array access. `get` returns nullptr when the key is missing.
    const JsonValue* get(const std::string& key) const {
        if (kind != Kind::Object) {
            return nullptr;
        }
        auto it = object.find(key);
        return it == object.end() ? nullptr : &it->second;
    }
    const JsonValue* at(std::size_t i) const {
        if (kind != Kind::Array || i >= array.size()) {
            return nullptr;
        }
        return &array[i];
    }
    std::size_t size() const {
        if (kind == Kind::Array) {
            return array.size();
        }
        if (kind == Kind::Object) {
            return object.size();
        }
        return 0;
    }

    std::string getString(const std::string& key, const std::string& fallback = "") const {
        const JsonValue* v = get(key);
        return (v && v->isString()) ? v->string : fallback;
    }
    float getFloat(const std::string& key, float fallback = 0.0f) const {
        const JsonValue* v = get(key);
        return v ? v->asFloat(fallback) : fallback;
    }
    int getInt(const std::string& key, int fallback = 0) const {
        const JsonValue* v = get(key);
        return v ? v->asInt(fallback) : fallback;
    }
    bool getBool(const std::string& key, bool fallback = false) const {
        const JsonValue* v = get(key);
        return v ? v->asBool(fallback) : fallback;
    }

private:
    friend class JsonParser;
};

// Recursive-descent parser for standard JSON (RFC 8259 subset: no comments,
// no trailing commas, `\uXXXX` escapes decoded to UTF-8).
class JsonParser {
public:
    static bool parse(const std::string& text, JsonValue& out) {
        JsonParser p(text);
        p.skipWs();
        if (!p.parseValue(out)) {
            return false;
        }
        p.skipWs();
        return p.pos >= p.text.size();
    }

private:
    explicit JsonParser(const std::string& t) : text(t) {}

    const std::string& text;
    std::size_t pos = 0;

    void skipWs() {
        while (pos < text.size()) {
            const char c = text[pos];
            if (c == ' ' || c == '\t' || c == '\n' || c == '\r') {
                pos++;
            } else {
                break;
            }
        }
    }

    bool parseValue(JsonValue& out) {
        if (pos >= text.size()) {
            return false;
        }
        const char c = text[pos];
        if (c == '{') {
            return parseObject(out);
        }
        if (c == '[') {
            return parseArray(out);
        }
        if (c == '"') {
            out.kind = JsonValue::Kind::String;
            return parseString(out.string);
        }
        if (c == 't' || c == 'f') {
            return parseBool(out);
        }
        if (c == 'n') {
            return parseNull(out);
        }
        return parseNumber(out);
    }

    bool parseObject(JsonValue& out) {
        out.kind = JsonValue::Kind::Object;
        pos++; // {
        skipWs();
        if (pos < text.size() && text[pos] == '}') {
            pos++;
            return true;
        }
        while (pos < text.size()) {
            skipWs();
            std::string key;
            if (!parseString(key)) {
                return false;
            }
            skipWs();
            if (pos >= text.size() || text[pos] != ':') {
                return false;
            }
            pos++;
            skipWs();
            JsonValue value;
            if (!parseValue(value)) {
                return false;
            }
            out.object[key] = std::move(value);
            skipWs();
            if (pos < text.size() && text[pos] == ',') {
                pos++;
                continue;
            }
            if (pos < text.size() && text[pos] == '}') {
                pos++;
                return true;
            }
            return false;
        }
        return false;
    }

    bool parseArray(JsonValue& out) {
        out.kind = JsonValue::Kind::Array;
        pos++; // [
        skipWs();
        if (pos < text.size() && text[pos] == ']') {
            pos++;
            return true;
        }
        while (pos < text.size()) {
            skipWs();
            JsonValue value;
            if (!parseValue(value)) {
                return false;
            }
            out.array.push_back(std::move(value));
            skipWs();
            if (pos < text.size() && text[pos] == ',') {
                pos++;
                continue;
            }
            if (pos < text.size() && text[pos] == ']') {
                pos++;
                return true;
            }
            return false;
        }
        return false;
    }

    bool parseString(std::string& out) {
        out.clear();
        if (pos >= text.size() || text[pos] != '"') {
            return false;
        }
        pos++;
        while (pos < text.size()) {
            const unsigned char c = static_cast<unsigned char>(text[pos]);
            if (c == '"') {
                pos++;
                return true;
            }
            if (c == '\\') {
                pos++;
                if (pos >= text.size()) {
                    return false;
                }
                const char e = text[pos++];
                switch (e) {
                    case '"': out.push_back('"'); break;
                    case '\\': out.push_back('\\'); break;
                    case '/': out.push_back('/'); break;
                    case 'b': out.push_back('\b'); break;
                    case 'f': out.push_back('\f'); break;
                    case 'n': out.push_back('\n'); break;
                    case 'r': out.push_back('\r'); break;
                    case 't': out.push_back('\t'); break;
                    case 'u': {
                        if (pos + 4 > text.size()) {
                            return false;
                        }
                        unsigned cp = 0;
                        for (int i = 0; i < 4; i++) {
                            cp = (cp << 4) | hexDigit(text[pos++]);
                        }
                        // Surrogate pair -> UTF-8.
                        if (cp >= 0xD800 && cp <= 0xDBFF && pos + 6 <= text.size()
                            && text[pos] == '\\' && text[pos + 1] == 'u') {
                            pos += 2;
                            unsigned lo = 0;
                            for (int i = 0; i < 4; i++) {
                                lo = (lo << 4) | hexDigit(text[pos++]);
                            }
                            cp = 0x10000 + ((cp - 0xD800) << 10) + (lo - 0xDC00);
                        }
                        appendUtf8(out, cp);
                        break;
                    }
                    default:
                        out.push_back(e);
                        break;
                }
                continue;
            }
            out.push_back(static_cast<char>(c));
            pos++;
        }
        return false;
    }

    static unsigned hexDigit(char c) {
        if (c >= '0' && c <= '9') return static_cast<unsigned>(c - '0');
        if (c >= 'a' && c <= 'f') return static_cast<unsigned>(c - 'a' + 10);
        if (c >= 'A' && c <= 'F') return static_cast<unsigned>(c - 'A' + 10);
        return 0;
    }

    static void appendUtf8(std::string& out, unsigned cp) {
        if (cp < 0x80) {
            out.push_back(static_cast<char>(cp));
        } else if (cp < 0x800) {
            out.push_back(static_cast<char>(0xC0 | (cp >> 6)));
            out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
        } else if (cp < 0x10000) {
            out.push_back(static_cast<char>(0xE0 | (cp >> 12)));
            out.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
            out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
        } else {
            out.push_back(static_cast<char>(0xF0 | (cp >> 18)));
            out.push_back(static_cast<char>(0x80 | ((cp >> 12) & 0x3F)));
            out.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
            out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
        }
    }

    bool parseNumber(JsonValue& out) {
        const std::size_t start = pos;
        if (pos < text.size() && (text[pos] == '-' || text[pos] == '+')) {
            pos++;
        }
        bool any = false;
        while (pos < text.size()) {
            const char c = text[pos];
            if (std::isdigit(static_cast<unsigned char>(c)) || c == '.' || c == 'e' || c == 'E'
                || c == '+' || c == '-') {
                pos++;
                any = true;
            } else {
                break;
            }
        }
        if (!any) {
            return false;
        }
        out.kind = JsonValue::Kind::Number;
        out.number = std::strtod(text.substr(start, pos - start).c_str(), nullptr);
        return true;
    }

    bool parseBool(JsonValue& out) {
        if (text.compare(pos, 4, "true") == 0) {
            pos += 4;
            out.kind = JsonValue::Kind::Bool;
            out.boolean = true;
            return true;
        }
        if (text.compare(pos, 5, "false") == 0) {
            pos += 5;
            out.kind = JsonValue::Kind::Bool;
            out.boolean = false;
            return true;
        }
        return false;
    }

    bool parseNull(JsonValue& out) {
        if (text.compare(pos, 4, "null") == 0) {
            pos += 4;
            out.kind = JsonValue::Kind::Null;
            return true;
        }
        return false;
    }
};

} // namespace ui
