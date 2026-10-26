/****************************************************************************
 * SurvevMobile - the single JNI bridge between the Android Java UI and the
 * axmol/C++ game (ARCHITECTURE.md section 4).
 *
 * Two directions, one class:
 *   * `native` methods below are implemented in client-mobile/src/app/UiBridge.cpp
 *     (Java -> C++). Every one of them must be called from the UI thread; the
 *     engine marshals anything thread-sensitive itself.
 *   * the `static void onNative*` handlers are called FROM C++ on the GL thread
 *     (C++ -> Java). They only post a Runnable to the UI thread, because the
 *     Java UI host owns every View.
 *
 * The class is deliberately dumb: no gameplay pointers, no network, no state
 * beyond the UI host reference. See ARCHITECTURE.md section 4.2 for the
 * threading rules - do not add a method that breaks them.
 ****************************************************************************/
package dev.axmol.app;

import android.util.Log;

public final class SurvBridge {
    private static final String TAG = "SurvBridge";

    /** Installed by AppActivity.onCreate once the Java UI container exists. */
    private static JavaUiHost sHost;

    /** The Java UI host (menu/loadout/team/game overlays). May be null early. */
    public static void setHost(JavaUiHost host) {
        sHost = host;
    }

    public static JavaUiHost getHost() {
        return sHost;
    }

    private SurvBridge() {}

    // --- C++ entry points (Java -> C++) ------------------------------------

    /** The Java UI is built and visible. */
    public static native void onMenuReady();

    /** "Play Solo/Duo/Squad": teamMode is 1, 2 or 4. */
    public static native void onQuickStart(int teamMode);

    /** Start a match the Java HTTP layer discovered (SurvApi/TeamSocket). */
    public static native void onJoinUrls(String[] urls, String joinToken);

    /** "Back to main menu" while a matchmaking request is pending. */
    public static native void onCancelPending();

    /** Leave the running match and return to the menu. */
    public static native void onQuitGame();

    /** The pause dialog opened (engine pauses the sim + mutes audio). */
    public static native void onPauseGame();

    /** The pause dialog closed. */
    public static native void onResumeGame();

    /** SpectateMsg action from the pause dialog: 1=Begin, 2=Next, 3=Prev. */
    public static native void spectate(int action);

    /**
     * BT-keyboard key state (Android KeyEvent.KEYCODE_*). axmol's GLSurfaceView
     * only forwards DPAD/BACK to the engine, so AppActivity sends the gameplay
     * letters/digits through here; C++ routes them to HardwareInput on the GL
     * thread (P1.6).
     */
    public static native void hardwareKey(int keyCode, boolean down);

    public static native void setPlayerName(String name);

    public static native void setRegion(String region);

    /** Re-validates the loadout through the C++ validator; false when malformed. */
    public static native boolean setLoadout(String loadoutJson);

    /** key is one of "master", "sound", "music", "mute". */
    public static native void setAudio(String key, float value);

    public static native void setLanguage(String locale);

    /** Engine stats overlay (fps/draw calls); persisted as `showFps`. */
    public static native void setShowFps(boolean show);

    /** GameConfig.protocolVersion (shared/gameConfig.ts). */
    public static native int getProtocolVersion();

    public static native void saveConfig();

    /**
     * M8 error reporting (client/src/errorLogs.ts): a non-fatal report from the
     * Java half. `loc` is "windowOnError", "errorLog" or "storeGeneric";
     * `dataJson` is the payload. The C++ side samples/throttles it. The main
     * thread's crash handler bypasses this and posts directly (the process dies).
     */
    public static native void reportError(String loc, String dataJson);

    // --- read-only queries, answered synchronously from a C++ snapshot ------

    /** The whole config blob ({"playerName": ..., "loadout": {...}, ...}). */
    public static native String getConfigJson();

    /** Flattened {"key": "text"} table for the current locale. */
    public static native String getL10nJson();

    /** Language list for settings: [{"code","name"}, ...] (localization.ts Locales). */
    public static native String getLocalesJson();

    /** The validated loadout selection. */
    public static native String getLoadoutJson();

    /** Owned loadout items with shared-def metadata (JSON array; empty until defs load). */
    public static native String getLoadoutItemsJson();

    /** Persists the account's owned item types (profile `items`) for the validator. */
    public static native boolean setAccountItems(String typesJson);

    /** Cached /api/site_info response ("" until the first fetch responds). */
    public static native String getSiteInfoJson();

    /** The bundled news entries as a JSON array (main menu news column). */
    public static native String getNewsJson();

    public static native boolean isInGame();

    /** The active language code (config `language`, "en" when unset). */
    public static native String getLanguage();

    /** Set a config key-value pair (for sensitivity, HUD scale, etc.). */
    public static native void setConfigValue(String key, String value);

    // --- called from C++ (GL thread) ---------------------------------------
    // Each handler hops to the UI thread before touching a View. The Java UI
    // host is not required to exist yet (an early match start is dropped).

    static void onNativeMatchStarted(final int activePlayerId) {
        post(new Runnable() {
            @Override
            public void run() {
                JavaUiHost host = sHost;
                if (host != null) {
                    host.enterGame(activePlayerId);
                } else {
                    Log.w(TAG, "onNativeMatchStarted before the UI host existed");
                }
            }
        });
    }

    static void onNativeMatchEnded(final String errorL10nKey) {
        post(new Runnable() {
            @Override
            public void run() {
                JavaUiHost host = sHost;
                if (host != null) {
                    host.returnToMenu(errorL10nKey);
                }
            }
        });
    }

    static void onNativePlayerDead() {
        post(new Runnable() {
            @Override
            public void run() {
                JavaUiHost host = sHost;
                if (host != null) {
                    host.showResult();
                }
            }
        });
    }

    static void onNativeShowMenu() {
        post(new Runnable() {
            @Override
            public void run() {
                JavaUiHost host = sHost;
                if (host != null) {
                    host.showMenu();
                }
            }
        });
    }

    /** Cheap round-trip used by the S1 gate (see JavaUiHost). */
    static void onNativeConfigChanged(final String key) {
        post(new Runnable() {
            @Override
            public void run() {
                JavaUiHost host = sHost;
                if (host != null) {
                    host.onConfigChanged(key);
                }
            }
        });
    }

    /** Every region probe finished; `json` is PingTest::resultsJson() (C++). */
    static void onNativePingResults(final String json) {
        post(new Runnable() {
            @Override
            public void run() {
                JavaUiHost host = sHost;
                if (host != null) {
                    host.pingResults(json);
                }
            }
        });
    }

    /** Android BACK in a match: open/close the pause dialog (C++). */
    static void onNativeTogglePause() {
        post(new Runnable() {
            @Override
            public void run() {
                JavaUiHost host = sHost;
                if (host != null) {
                    host.togglePause();
                }
            }
        });
    }

    private static void post(Runnable runnable) {
        AppActivity activity = AppActivity.current();
        if (activity == null) {
            Log.w(TAG, "no AppActivity; dropping a native UI callback");
            return;
        }
        activity.runOnUiThread(runnable);
    }
}
