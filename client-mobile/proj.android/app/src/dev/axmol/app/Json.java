/****************************************************************************
 * Minimal JSON reader for the Java UI (ARCHITECTURE.md section 4.1: the bridge
 * only marshals JSON strings, no structs).
 *
 * The Java screens need to read a *few* keys out of the documents C++ produces
 * (the config blob, the flattened l10n table, the news array). A full parser
 * would be dead weight here, so this class only knows how to find the top-level
 * value of a key and how to count the keys of a flat object - enough for
 * "playerName", "index-play-solo" and the news column. When the real screens
 * land (S3+) and need nested documents, replace this with one shared parser
 * rather than growing it method by method.
 ****************************************************************************/
package dev.axmol.app;

public final class Json {
    private Json() {}

    /**
     * Returns the string value of the top-level `key` in a flat JSON object, or
     * null when the document, the key or the value type does not match.
     */
    public static String peekString(String json, String key) {
        if (json == null || key == null) {
            return null;
        }
        int at = findKey(json, key);
        if (at < 0) {
            return null;
        }
        at = skipWhitespace(json, at);
        if (at >= json.length() || json.charAt(at) != '"') {
            return null;
        }
        StringBuilder out = new StringBuilder();
        for (int i = at + 1; i < json.length(); i++) {
            char c = json.charAt(i);
            if (c == '\\' && i + 1 < json.length()) {
                char escaped = json.charAt(++i);
                switch (escaped) {
                    case 'n': out.append('\n'); break;
                    case 'r': out.append('\r'); break;
                    case 't': out.append('\t'); break;
                    case 'b': out.append('\b'); break;
                    case 'f': out.append('\f'); break;
                    default: out.append(escaped); break;
                }
                continue;
            }
            if (c == '"') {
                return out.toString();
            }
            out.append(c);
        }
        return null;
    }

    /** Number of keys in a flat JSON object (0 when the text is not an object). */
    public static int countKeys(String json) {
        if (json == null) {
            return 0;
        }
        int at = skipWhitespace(json, 0);
        if (at >= json.length() || json.charAt(at) != '{') {
            return 0;
        }
        int depth = 0;
        int keys = 0;
        boolean inString = false;
        for (int i = at; i < json.length(); i++) {
            char c = json.charAt(i);
            if (inString) {
                if (c == '\\') {
                    i++;
                } else if (c == '"') {
                    inString = false;
                }
                continue;
            }
            switch (c) {
                case '"':
                    inString = true;
                    if (depth == 1) {
                        keys++;
                    }
                    break;
                case '{':
                case '[':
                    depth++;
                    break;
                case '}':
                    if (depth == 1) {
                        return keys;
                    }
                    depth--;
                    break;
                case ']':
                    depth--;
                    break;
                default:
                    break;
            }
        }
        return keys;
    }

    private static int findKey(String json, String key) {
        int at = skipWhitespace(json, 0);
        if (at >= json.length() || json.charAt(at) != '{') {
            return -1;
        }
        int depth = 0;
        boolean inString = false;
        for (int i = at; i < json.length(); i++) {
            char c = json.charAt(i);
            if (inString) {
                if (c == '\\') {
                    i++;
                } else if (c == '"') {
                    inString = false;
                    if (depth == 1) {
                        int end = i + 1;
                        int colon = skipWhitespace(json, end);
                        if (colon < json.length() && json.charAt(colon) == ':') {
                            String found = unescape(json, i);
                            if (key.equals(found)) {
                                return colon + 1;
                            }
                        }
                    }
                }
                continue;
            }
            if (c == '"') {
                inString = true;
                continue;
            }
            if (c == '{' || c == '[') {
                depth++;
            } else if (c == '}' || c == ']') {
                if (depth == 0) {
                    return -1;
                }
                depth--;
            }
        }
        return -1;
    }

    /** Re-reads the string literal that ends at `closeQuote` (exclusive). */
    private static String unescape(String json, int closeQuote) {
        int start = closeQuote;
        for (int i = closeQuote - 1; i >= 0; i--) {
            if (json.charAt(i) == '"' && (i == 0 || json.charAt(i - 1) != '\\')) {
                start = i + 1;
                break;
            }
        }
        return json.substring(start, closeQuote);
    }

    private static int skipWhitespace(String json, int at) {
        while (at < json.length()) {
            char c = json.charAt(at);
            if (c == ' ' || c == '\t' || c == '\n' || c == '\r') {
                at++;
            } else {
                break;
            }
        }
        return at;
    }
}
