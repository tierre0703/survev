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
}
