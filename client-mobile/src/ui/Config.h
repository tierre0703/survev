#pragma once
// Engine-independent port of client/src/config.ts.
//
// The web client keeps the settings blob in `localStorage["surviv_config"]`.
// The native client keeps the same JSON shape, persisted through a
// `ConfigStorage` implementation (the app wires it to `ax::UserDefault`,
// i.e. SharedPreferences on Android). Values are typed accessors over a tiny
// JSON tree so new keys can be read/written without recompiling the schema;
// the defaults below mirror the web client exactly so shared behaviour
// (volumes, touch styles, regions) stays identical.
#include "Json.h"

#include <functional>
#include <string>
#include <vector>

namespace ui {

// Persistence hook. `read` returns the stored JSON (or empty), `write` stores
// the merged blob.
class ConfigStorage {
public:
    virtual ~ConfigStorage() = default;
    virtual std::string read() = 0;
    virtual void write(const std::string& json) = 0;
};

// In-memory storage (host tests / first run fallback).
class MemoryConfigStorage : public ConfigStorage {
public:
    std::string read() override { return value; }
    void write(const std::string& json) override { value = json; }
    std::string value;
};

// Persistence through the engine's UserDefault (SharedPreferences on Android),
// matching the web client's `localStorage["surviv_config"]`. Defined in
// ConfigStorage.cpp so this header stays engine-free for the host tests.
class UserDefaultConfigStorage : public ConfigStorage {
public:
    explicit UserDefaultConfigStorage(const std::string& key = "surviv_config");
    std::string read() override;
    void write(const std::string& json) override;

private:
    std::string _key;
};

// Serializes a JsonValue. Numbers that are whole are emitted without a decimal
// point so the blob stays readable and stable.
std::string jsonToString(const JsonValue& value);

class Config {
public:
    explicit Config(ConfigStorage* storage = nullptr) : _storage(storage) {}

    void setStorage(ConfigStorage* storage) { _storage = storage; }

    // Loads the persisted blob on top of the defaults; safe to call once.
    void load();

    // Typed accessors (missing keys fall back to the provided default, which
    // callers usually pass as the web client's default value).
    std::string getString(const std::string& key, const std::string& fallback = "") const;
    float getFloat(const std::string& key, float fallback = 0.0f) const;
    int getInt(const std::string& key, int fallback = 0) const;
    bool getBool(const std::string& key, bool fallback = false) const;

    void setString(const std::string& key, const std::string& value);
    void setFloat(const std::string& key, float value);
    void setInt(const std::string& key, int value);
    void setBool(const std::string& key, bool value);

    // Typed reads that fall back to the web default for `key` rather than to
    // the caller's value (the menu mostly wants the defaults).
    std::string stringOrDefault(const std::string& key) const { return getString(key, defaultString(key)); }
    float floatOrDefault(const std::string& key) const { return getFloat(key, defaultFloat(key)); }
    int intOrDefault(const std::string& key) const { return getInt(key, defaultInt(key)); }
    bool boolOrDefault(const std::string& key) const { return getBool(key, defaultBool(key)); }

    // Stores the raw blob (used by the loadout menu to persist the loadout).
    void setJson(const std::string& key, const JsonValue& value);
    const JsonValue* getJson(const std::string& key) const;

    void addListener(std::function<void(const std::string&)> listener) {
        _listeners.push_back(std::move(listener));
    }

    // Persists the current blob without firing listeners (used after the
    // upgrade path mutates defaults).
    void save() { persist(); }

    const JsonValue& root() const { return _root; }

    // Web defaults (config.ts defaultConfig). Also used as the persisted
    // fallback values when a key is absent.
    static const Config& defaults();
    static std::string defaultString(const std::string& key);
    static float defaultFloat(const std::string& key);
    static int defaultInt(const std::string& key);
    static bool defaultBool(const std::string& key);

    // The loadout JSON the web client stores under `loadout` (validated).
    static JsonValue defaultLoadout() { return buildDefaultLoadout(); }

private:
    void notify(const std::string& key);
    void persist();

    ConfigStorage* _storage = nullptr;
    JsonValue _root;
    std::vector<std::function<void(const std::string&)>> _listeners;

    static JsonValue buildDefaults();
    static JsonValue buildDefaultLoadout();
};

} // namespace ui
