/****************************************************************************
 * L10n.java — cached access to the C++ localization table.
 *
 * `SurvBridge.getL10nJson()` returns the flattened English + active-locale map
 * (ui::Localization::flattened). The Java UI needs it for every label, so keep
 * one parsed copy and reload it when `SurvBridge.getLanguage()` changes.
 *
 * The locale check matters on a cold start: the Java menu can be constructed
 * before the C++ bridge has loaded the saved config, so caching the first
 * (English) table forever left every label in the wrong language. The language
 * code from the C++ config is the cheapest reliable invalidation signal.
 ****************************************************************************/
package dev.axmol.app;

import java.util.Collections;
import java.util.Map;

public final class L10n {
    private static Map<String, String> sTable = Collections.emptyMap();
    private static String sLocale = null;

    private L10n() {}

    /** Force the next lookup to re-read the table (settings language change). */
    public static void invalidate() {
        sTable = Collections.emptyMap();
        sLocale = null;
    }

    /** Localized text for `key`, or `fallback` when the locale has no entry. */
    public static String t(String key, String fallback) {
        Map<String, String> table = table();
        String text = table.get(key);
        return text != null && !text.isEmpty() ? text : fallback;
    }

    private static Map<String, String> table() {
        String locale = SurvBridge.getLanguage();
        if (sTable.isEmpty() || sLocale == null || !sLocale.equals(locale)) {
            sLocale = locale;
            sTable = Json.parseStringMap(SurvBridge.getL10nJson());
        }
        return sTable;
    }
}
