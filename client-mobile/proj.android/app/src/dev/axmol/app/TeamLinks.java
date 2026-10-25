/****************************************************************************
 * TeamLinks.java - invite link helpers for the team room (web teamMenu.ts).
 *
 * The web server identifies a room by `#<code>`; the web team screen shows
 * `https://<site>/#<code>` and the mobile flow accepts a pasted link. Native
 * shares the same web link (config `webBaseUrl`, default https://survev.io) so
 * browser players can join, and also accepts `survev://team/<code>` deep links.
 ****************************************************************************/
package dev.axmol.app;

import java.util.regex.Pattern;

public final class TeamLinks {
    private TeamLinks() {}

    public static final String DEFAULT_WEB_BASE_URL = "https://survev.io";

    /** Team codes are 4 chars today (server generateTeamCode); stay lenient. */
    private static final Pattern CODE = Pattern.compile("[A-Za-z0-9]{2,16}");

    /**
     * Extracts a room code from a bare code, `#CODE`, a web invite URL
     * (`https://survev.io/#CODE`), `survev://team/CODE` or
     * `survev://team?code=CODE`. Returns "" when nothing usable is found.
     */
    public static String extractCode(String raw) {
        if (raw == null) return "";
        String s = raw.trim();
        if (s.isEmpty()) return "";

        if (s.startsWith("survev://")) {
            int q = s.indexOf("code=");
            if (q >= 0) {
                s = s.substring(q + "code=".length());
            } else {
                int slash = s.indexOf('/', "survev://".length());
                s = slash >= 0 ? s.substring(slash + 1) : s.substring("survev://".length());
            }
        }

        int hash = s.indexOf('#');
        if (hash >= 0) {
            s = s.substring(hash + 1);
        }

        int query = s.indexOf('?');
        if (query >= 0) {
            s = s.substring(0, query);
        }
        int slash = s.lastIndexOf('/');
        if (slash >= 0) {
            s = s.substring(slash + 1);
        }

        s = s.trim();
        return CODE.matcher(s).matches() ? s : "";
    }

    /** The browser-joinable invite URL shown/copied by the team screen. */
    public static String inviteUrl(String code) {
        if (code == null || code.isEmpty()) return "";
        return webBaseUrl() + "/#" + code;
    }

    /** App-to-app deep link handled by AppActivity's VIEW intent filter. */
    public static String appLink(String code) {
        return code == null || code.isEmpty() ? "" : "survev://team/" + code;
    }

    private static String webBaseUrl() {
        String base = Json.peekString(SurvBridge.getConfigJson(), "webBaseUrl");
        if (base == null || base.isEmpty()) {
            base = DEFAULT_WEB_BASE_URL;
        }
        if (!base.startsWith("http://") && !base.startsWith("https://")) {
            base = DEFAULT_WEB_BASE_URL;
        }
        while (base.endsWith("/")) {
            base = base.substring(0, base.length() - 1);
        }
        return base;
    }
}
