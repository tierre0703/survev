package dev.axmol.app;

import android.util.Log;
import com.google.gson.JsonElement;
import com.google.gson.JsonObject;
import com.google.gson.JsonParser;
import com.google.gson.JsonArray;
import com.google.gson.JsonPrimitive;
import java.util.ArrayList;
import java.util.List;
import java.util.Map;

public final class Json {
    private static final String TAG = "Json";
    private Json() {}

    /**
     * Returns the string value of the top-level `key` in a flat JSON object, or
     * null when the document, the key or the value type does not match.
     */
    public static String peekString(String json, String key) {
        if (json == null || key == null) return null;
        try {
            JsonObject obj = JsonParser.parseString(json).getAsJsonObject();
            JsonElement el = obj.get(key);
            if (el != null && el.isJsonPrimitive()) {
                JsonPrimitive p = el.getAsJsonPrimitive();
                if (p.isString()) return p.getAsString();
            }
        } catch (Exception e) {
            Log.w(TAG, "peekString failed for key=" + key + ": " + e.getMessage());
        }
        return null;
    }

    /** Returns the boolean value of a top-level key, or `fallback` on error. */
    public static boolean peekBool(String json, String key, boolean fallback) {
        if (json == null || key == null) return fallback;
        try {
            JsonObject obj = JsonParser.parseString(json).getAsJsonObject();
            JsonElement el = obj.get(key);
            if (el != null && el.isJsonPrimitive()) {
                JsonPrimitive p = el.getAsJsonPrimitive();
                if (p.isBoolean()) return p.getAsBoolean();
                if (p.isString()) return "true".equals(p.getAsString());
            }
        } catch (Exception e) {
            Log.w(TAG, "peekBool failed for key=" + key + ": " + e.getMessage());
        }
        return fallback;
    }

    /** Returns the number value of a top-level key, or `fallback` on error. */
    public static float peekFloat(String json, String key, float fallback) {
        if (json == null || key == null) return fallback;
        try {
            JsonObject obj = JsonParser.parseString(json).getAsJsonObject();
            JsonElement el = obj.get(key);
            if (el != null && el.isJsonPrimitive()) {
                JsonPrimitive p = el.getAsJsonPrimitive();
                if (p.isNumber()) return p.getAsFloat();
                if (p.isString()) return Float.parseFloat(p.getAsString());
            }
        } catch (Exception e) {
            Log.w(TAG, "peekFloat failed for key=" + key + ": " + e.getMessage());
        }
        return fallback;
    }

    /** Returns the integer value of a top-level key, or 0 on error. */
    public static int peekInt(String json, String key) {        if (json == null || key == null) return 0;
        try {
            JsonObject obj = JsonParser.parseString(json).getAsJsonObject();
            JsonElement el = obj.get(key);
            if (el != null && el.isJsonPrimitive()) {
                return el.getAsInt();
            }
        } catch (Exception e) {
            Log.w(TAG, "peekInt failed for key=" + key + ": " + e.getMessage());
        }
        return 0;
    }

    /** Returns the double value of a top-level key, or 0.0 on error. */
    public static double peekDouble(String json, String key) {
        if (json == null || key == null) return 0.0;
        try {
            JsonObject obj = JsonParser.parseString(json).getAsJsonObject();
            JsonElement el = obj.get(key);
            if (el != null && el.isJsonPrimitive()) {
                return el.getAsDouble();
            }
        } catch (Exception e) {
            Log.w(TAG, "peekDouble failed for key=" + key + ": " + e.getMessage());
        }
        return 0.0;
    }

    /** String value of a field inside a nested object, or null. */
    public static String peekNestedString(String json, String parent, String key) {
        if (json == null || parent == null || key == null) return null;
        try {
            JsonObject root = JsonParser.parseString(json).getAsJsonObject();
            JsonObject obj = root.getAsJsonObject(parent);
            if (obj == null) return null;
            JsonElement el = obj.get(key);
            if (el != null && el.isJsonPrimitive()) {
                JsonPrimitive p = el.getAsJsonPrimitive();
                if (p.isString()) return p.getAsString();
                if (p.isNumber()) return p.getAsString();
            }
        } catch (Exception e) {
            Log.w(TAG, "peekNestedString failed for " + parent + "." + key + ": " + e.getMessage());
        }
        return null;
    }

    /** Number value of a field inside a nested object, or fallback. */
    public static double peekNestedDouble(String json, String parent, String key, double fallback) {
        if (json == null || parent == null || key == null) return fallback;
        try {
            JsonObject root = JsonParser.parseString(json).getAsJsonObject();
            JsonObject obj = root.getAsJsonObject(parent);
            if (obj == null) return fallback;
            JsonElement el = obj.get(key);
            if (el != null && el.isJsonPrimitive()) {
                return el.getAsDouble();
            }
        } catch (Exception e) {
            Log.w(TAG, "peekNestedDouble failed for " + parent + "." + key + ": " + e.getMessage());
        }
        return fallback;
    }

    /** Number of keys in a flat JSON object (0 when the text is not an object). */
    public static int countKeys(String json) {
        if (json == null) return 0;
        try {
            JsonObject obj = JsonParser.parseString(json).getAsJsonObject();
            return obj.size();
        } catch (Exception e) {
            return 0;
        }
    }

    /**
     * Parses a JSON object value by key and returns a list of string values from
     * a nested array, e.g. {"key": ["a", "b"]}
     */
    public static List<String> parseStringArray(String json, String key) {
        List<String> result = new ArrayList<>();
        if (json == null || key == null) return result;
        try {
            JsonObject obj = JsonParser.parseString(json).getAsJsonObject();
            JsonElement el = obj.get(key);
            if (el != null && el.isJsonArray()) {
                JsonArray arr = el.getAsJsonArray();
                for (int i = 0; i < arr.size(); i++) {
                    JsonElement item = arr.get(i);
                    if (item.isJsonPrimitive()) {
                        result.add(item.getAsString());
                    }
                }
            }
        } catch (Exception e) {
            Log.w(TAG, "parseStringArray failed for key=" + key + ": " + e.getMessage());
        }
        return result;
    }

    /**
     * Parses the "pops" object: {"na": {"playerCount": 42, "l10n": "..."}, ...}
     * Returns a list of PopEntry for each region key.
     */
    public static List<PopEntry> parsePops(String json) {
        List<PopEntry> pops = new ArrayList<>();
        if (json == null) return pops;
        try {
            JsonObject root = JsonParser.parseString(json).getAsJsonObject();
            JsonObject popsObj = root.getAsJsonObject("pops");
            if (popsObj == null) {
                Log.w(TAG, "parsePops: no pops object found");
                return pops;
            }
            for (Map.Entry<String, JsonElement> entry : popsObj.entrySet()) {
                String region = entry.getKey();
                JsonElement val = entry.getValue();
                if (val != null && val.isJsonObject()) {
                    JsonObject regionObj = val.getAsJsonObject();
                    PopEntry p = new PopEntry();
                    p.region = region;
                    p.l10n = regionObj.has("l10n") && !regionObj.get("l10n").isJsonNull()
                            ? regionObj.get("l10n").getAsString() : region;
                    p.playerCount = regionObj.has("playerCount") && !regionObj.get("playerCount").isJsonNull()
                            ? regionObj.get("playerCount").getAsInt() : 0;
                    pops.add(p);
                }
            }
        } catch (Exception e) {
            Log.w(TAG, "parsePops failed: " + e.getMessage());
        }
        return pops;
    }

    public static class PopEntry {
        public String region;
        public int playerCount;
        public String l10n;
    }

    /**
     * Parses the C++ PingTest snapshot ({"complete","bestRegion","tests":[...]})
     * into the entries the menu shows next to each region. `pingMs` is -1 when
     * the region never answered.
     */
    public static List<PingEntry> parsePings(String json) {
        List<PingEntry> pings = new ArrayList<>();
        if (json == null) return pings;
        try {
            JsonObject root = JsonParser.parseString(json).getAsJsonObject();
            JsonElement testsEl = root.get("tests");
            if (testsEl == null || !testsEl.isJsonArray()) return pings;
            JsonArray tests = testsEl.getAsJsonArray();
            for (int i = 0; i < tests.size(); i++) {
                JsonElement el = tests.get(i);
                if (el == null || !el.isJsonObject()) continue;
                JsonObject obj = el.getAsJsonObject();
                PingEntry p = new PingEntry();
                p.region = obj.has("region") ? obj.get("region").getAsString() : "";
                p.zone = obj.has("zone") ? obj.get("zone").getAsString() : p.region;
                p.pingMs = obj.has("pingMs") ? obj.get("pingMs").getAsInt() : -1;
                p.complete = obj.has("complete") && obj.get("complete").getAsBoolean();
                if (!p.region.isEmpty()) pings.add(p);
            }
        } catch (Exception e) {
            Log.w(TAG, "parsePings failed: " + e.getMessage());
        }
        return pings;
    }

    public static class PingEntry {
        public String region;
        public String zone;
        /** Round-trip in ms, -1 when the region did not answer. */
        public int pingMs = -1;
        public boolean complete = false;
    }

    /** Language dropdown entries (ui::kLocales): [{"code","name"}, ...]. */
    public static List<LocaleEntry> parseLocales(String json) {        List<LocaleEntry> out = new ArrayList<>();
        if (json == null) return out;
        try {
            JsonArray arr = JsonParser.parseString(json).getAsJsonArray();
            for (int i = 0; i < arr.size(); i++) {
                JsonElement el = arr.get(i);
                if (el == null || !el.isJsonObject()) continue;
                JsonObject obj = el.getAsJsonObject();
                LocaleEntry e = new LocaleEntry();
                e.code = obj.has("code") ? obj.get("code").getAsString() : "";
                e.name = obj.has("name") ? obj.get("name").getAsString() : e.code;
                if (!e.code.isEmpty()) out.add(e);
            }
        } catch (Exception e) {
            Log.w(TAG, "parseLocales failed: " + e.getMessage());
        }
        return out;
    }

    public static class LocaleEntry {
        public String code = "";
        public String name = "";
    }

    /**
     * Parses `site_info.modes`: [{mapName, teamMode, enabled}, ...]. This is the
     * single source of truth for the quick-play buttons (siteInfo.ts
     * getGameModeStyles); the native menu must not hard-code Solo/Duo/Squad.
     */
    public static List<ModeEntry> parseModes(String json) {
        List<ModeEntry> modes = new ArrayList<>();
        if (json == null) return modes;
        try {
            JsonObject root = JsonParser.parseString(json).getAsJsonObject();
            JsonElement modesEl = root.get("modes");
            if (modesEl == null || !modesEl.isJsonArray()) return modes;
            JsonArray arr = modesEl.getAsJsonArray();
            for (int i = 0; i < arr.size(); i++) {
                JsonElement el = arr.get(i);
                if (el == null || !el.isJsonObject()) continue;
                JsonObject obj = el.getAsJsonObject();
                ModeEntry e = new ModeEntry();
                e.mapName = obj.has("mapName") && !obj.get("mapName").isJsonNull()
                        ? obj.get("mapName").getAsString() : "main";
                e.teamMode = obj.has("teamMode") ? obj.get("teamMode").getAsInt() : 1;
                e.enabled = !obj.has("enabled") || obj.get("enabled").getAsBoolean();
                modes.add(e);
            }
        } catch (Exception e) {
            Log.w(TAG, "parseModes failed: " + e.getMessage());
        }
        return modes;
    }

    public static class ModeEntry {
        public String mapName = "main";
        public int teamMode = 1;
        public boolean enabled = true;
    }

    /** Flattens a flat string map (the l10n table) into a lookup map. */
    public static Map<String, String> parseStringMap(String json) {
        Map<String, String> out = new java.util.HashMap<>();
        if (json == null) return out;
        try {
            JsonObject obj = JsonParser.parseString(json).getAsJsonObject();
            for (Map.Entry<String, JsonElement> entry : obj.entrySet()) {
                JsonElement el = entry.getValue();
                if (el != null && el.isJsonPrimitive() && el.getAsJsonPrimitive().isString()) {
                    out.put(entry.getKey(), el.getAsString());
                }
            }
        } catch (Exception e) {
            Log.w(TAG, "parseStringMap failed: " + e.getMessage());
        }
        return out;
    }
}
