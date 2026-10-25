/****************************************************************************
 * AccountManager.java - M8 device account (no OAuth).
 *
 * The native client has no browser login flow. On first launch it derives a
 * stable device id (Settings.Secure.ANDROID_ID, or a random UUID when that is
 * unavailable) and calls POST /api/auth/device, which creates the user on the
 * first run and returns a session token afterwards. The token is kept in the
 * C++ config (the source of truth, like the web `sessionCookie`) and sent as
 * `Authorization: Bearer` by SurvApi.
 *
 * The rest of the account surface mirrors client/src/account.ts:
 *   profile  -> username/slug/usernameSet/usernameChangeTime + items/loadout
 *   username -> set/change the account name (10 day cooldown on the server)
 *   logout / reset_stats / delete
 *
 * Threading: every SurvApi callback already arrives on the UI thread, and all
 * SurvBridge calls below happen on it too. No locks are needed.
 ****************************************************************************/
package dev.axmol.app;

import android.app.Activity;
import android.content.SharedPreferences;
import android.provider.Settings;
import android.util.Log;

import com.google.gson.JsonElement;
import com.google.gson.JsonObject;
import com.google.gson.JsonParser;

import java.util.ArrayList;
import java.util.List;
import java.util.UUID;

public final class AccountManager {
    private static final String TAG = "AccountManager";

    /**
     * Account credentials live in their own SharedPreferences file: unlike
     * gameplay config they are not read by C++/the game loop, and keeping them
     * out of `SurvBridge.setConfigValue` avoids its number/bool JSON coercion
     * (an all-digit ANDROID_ID would otherwise be truncated through a float).
     */
    private static final String PREFS_NAME = "survev_account";
    private static final String PREF_DEVICE_ID = "deviceId";
    private static final String PREF_SESSION_TOKEN = "sessionToken";
    private static final String PREF_SIGNED_OUT = "signedOut";
    /**
     * The account username currently mirrored into the menu's `playerName`
     * field. Empty means the field is user-owned; a match means a later account
     * rename should follow it.
     */
    private static final String PREF_PLAYER_NAME_ACCOUNT = "playerNameAccount";

    private static AccountManager sInstance;

    /** Snapshot of the logged-in user; `loggedIn` is false before login. */
    public static final class Profile {
        public boolean loggedIn = false;
        public String username = "";
        public String slug = "";
        public boolean usernameSet = false;
        public boolean linked = false;
        /** Milliseconds until the next allowed username change (0 = allowed). */
        public long usernameChangeTime = 0;
    }

    public interface Listener {
        /** Called with the current (possibly logged-out) profile after each change. */
        void onAccountChanged(Profile profile);

        /** A localized-ish error code: account_offline/account_error/account_banned. */
        void onAccountError(String error);
    }

    public interface UsernameCallback {
        /** null on success, otherwise failed/invalid/taken/change_time_not_expired. */
        void onResult(String errorCode);
    }

    public interface SimpleCallback {
        void onResult(boolean success);
    }

    private final Activity mActivity;
    private final SharedPreferences mPrefs;
    private final List<Listener> mListeners = new ArrayList<Listener>();
    private final String mApiBaseUrl;

    private final Profile mProfile = new Profile();
    private String mDeviceId = "";
    private String mToken = "";
    private boolean mStarted = false;
    private boolean mBusy = false;

    private AccountManager(Activity activity) {
        mActivity = activity;
        mPrefs = activity.getSharedPreferences(PREFS_NAME, Activity.MODE_PRIVATE);
        mApiBaseUrl = SurvApi.DEFAULT_API_BASE_URL;
    }

    /** UI-thread entry point; the Activity is only used to read ANDROID_ID. */
    public static AccountManager get(Activity activity) {
        if (sInstance == null) {
            sInstance = new AccountManager(activity);
        }
        return sInstance;
    }

    public static AccountManager peek() {
        return sInstance;
    }

    public void addListener(Listener listener) {
        if (listener == null || mListeners.contains(listener)) {
            return;
        }
        mListeners.add(listener);
        listener.onAccountChanged(mProfile);
        ensureLoaded();
    }

    public void removeListener(Listener listener) {
        mListeners.remove(listener);
    }

    public Profile getProfile() {
        return mProfile;
    }

    public boolean isLoggedIn() {
        return mProfile.loggedIn;
    }

    /** True after an explicit Log Out (the device login is not re-run). */
    public boolean isSignedOut() {
        return "1".equals(readPref(PREF_SIGNED_OUT));
    }

    /** The account username the menu's player-name field currently follows. */
    public String getBoundPlayerName() {
        return readPref(PREF_PLAYER_NAME_ACCOUNT);
    }

    public void bindPlayerName(String accountName) {
        writePref(PREF_PLAYER_NAME_ACCOUNT, accountName != null ? accountName : "");
    }

    public void unbindPlayerName() {
        writePref(PREF_PLAYER_NAME_ACCOUNT, "");
    }

    /** Starts the device login on first use unless the user signed out. */
    public void ensureLoaded() {
        if (mStarted || mBusy) {
            return;
        }
        mStarted = true;

        mDeviceId = ensureDeviceId();
        mToken = readPref(PREF_SESSION_TOKEN);

        if (isSignedOut()) {
            Log.i(TAG, "device login skipped (signed out)");
            notifyChanged();
            return;
        }
        if (!mToken.isEmpty()) {
            fetchProfile(true);
        } else {
            deviceLogin();
        }
    }

    /** Retry after a failed login/profile request. */
    public void retry() {
        if (mBusy) {
            return;
        }
        mStarted = false;
        mProfile.loggedIn = false;
        ensureLoaded();
    }

    /** Profile-screen "Sign in": clears the signed-out latch and logs in. */
    public void signIn() {
        if (mBusy) {
            return;
        }
        writePref(PREF_SIGNED_OUT, "0");
        mStarted = true;
        mDeviceId = ensureDeviceId();
        deviceLogin();
    }

    public void setUsername(String username, final UsernameCallback callback) {
        if (!mProfile.loggedIn || mToken.isEmpty()) {
            callback.onResult("failed");
            return;
        }
        String body = "{\"username\":\"" + SurvApi.escapeJson(username) + "\"}";
        SurvApi.userPost(mApiBaseUrl, "username", mToken, body, new SurvApi.StatusCallback() {
            @Override
            public void onResult(int code, String json) {
                if (code != 200 || json == null) {
                    callback.onResult("failed");
                    return;
                }
                String result = "failed";
                try {
                    JsonObject root = JsonParser.parseString(json).getAsJsonObject();
                    if (root.has("result")) {
                        result = root.get("result").getAsString();
                    }
                } catch (Exception e) {
                    Log.w(TAG, "username response parse failed: " + e.getMessage());
                }
                if ("success".equals(result)) {
                    fetchProfile(false);
                    callback.onResult(null);
                } else {
                    callback.onResult(result);
                }
            }
        });
    }

    public void logout() {
        final String token = mToken;
        clearSession();
        writePref(PREF_SIGNED_OUT, "1");
        mProfile.loggedIn = false;
        mProfile.username = "";
        mProfile.slug = "";
        mProfile.usernameSet = false;
        mProfile.usernameChangeTime = 0;
        notifyChanged();
        if (!token.isEmpty()) {
            SurvApi.userPost(mApiBaseUrl, "logout", token, "{}", new SurvApi.StatusCallback() {
                @Override
                public void onResult(int code, String json) {
                    Log.i(TAG, "logout -> " + code);
                }
            });
        }
    }

    public void resetStats(final SimpleCallback callback) {
        if (!mProfile.loggedIn || mToken.isEmpty()) {
            callback.onResult(false);
            return;
        }
        SurvApi.userPost(mApiBaseUrl, "reset_stats", mToken, "{}", new SurvApi.StatusCallback() {
            @Override
            public void onResult(int code, String json) {
                callback.onResult(code == 200);
            }
        });
    }

    public void deleteAccount(final SimpleCallback callback) {
        if (!mProfile.loggedIn || mToken.isEmpty()) {
            callback.onResult(false);
            return;
        }
        final String token = mToken;
        SurvApi.userPost(mApiBaseUrl, "delete", token, "{}", new SurvApi.StatusCallback() {
            @Override
            public void onResult(int code, String json) {
                if (code != 200) {
                    callback.onResult(false);
                    return;
                }
                // The device identity outlives the account: start a fresh one so
                // the next launch is not tied to the deleted user. This mirrors
                // the web reload after `delete` (which lands on the login menu).
                clearSession();
                writePref(PREF_SIGNED_OUT, "0");
                mStarted = true;
                mProfile.loggedIn = false;
                notifyChanged();
                callback.onResult(true);
                deviceLogin();
            }
        });
    }

    /**
     * Pushes the local loadout to the account (web `account.setLoadout`). The
     * server validates it against the owned items and returns the corrected
     * selection, which `fetchProfile` then applies back through SurvBridge.
     */
    public void pushLoadout(String loadoutJson) {
        if (!mProfile.loggedIn || mToken.isEmpty() || loadoutJson == null) {
            return;
        }
        String body = "{\"loadout\":" + loadoutJson + "}";
        SurvApi.userPost(mApiBaseUrl, "loadout", mToken, body, new SurvApi.StatusCallback() {
            @Override
            public void onResult(int code, String json) {
                Log.i(TAG, "push loadout -> " + code);
            }
        });
    }

    // --- internals ----------------------------------------------------------

    private void deviceLogin() {
        if (mBusy) {
            return;
        }
        mBusy = true;

        SurvApi.deviceLogin(mApiBaseUrl, mDeviceId, new SurvApi.StatusCallback() {
            @Override
            public void onResult(int code, String json) {
                mBusy = false;
                if (code != 200 || json == null) {
                    Log.w(TAG, "device login failed: " + code);
                    notifyError("account_offline");
                    return;
                }
                try {
                    JsonObject root = JsonParser.parseString(json).getAsJsonObject();
                    if (!root.has("success") || !root.get("success").getAsBoolean()) {
                        notifyError("account_error");
                        return;
                    }
                    String token = root.get("token").getAsString();
                    if (token == null || token.isEmpty()) {
                        notifyError("account_error");
                        return;
                    }
                    mToken = token;
                    writePref(PREF_SESSION_TOKEN, token);
                    fetchProfile(false);
                } catch (Exception e) {
                    Log.w(TAG, "device login parse failed: " + e.getMessage());
                    notifyError("account_error");
                }
            }
        });
    }

    private void fetchProfile(final boolean hadToken) {
        SurvApi.userPost(mApiBaseUrl, "profile", mToken, "{}", new SurvApi.StatusCallback() {
            @Override
            public void onResult(int code, String json) {
                if (code == 200 && json != null) {
                    try {
                        JsonObject root = JsonParser.parseString(json).getAsJsonObject();
                        if (root.has("banned") && root.get("banned").getAsBoolean()) {
                            clearSession();
                            mProfile.loggedIn = false;
                            notifyChanged();
                            notifyError("account_banned");
                            return;
                        }
                        if (root.has("success") && root.get("success").getAsBoolean()) {
                            applyProfile(root);
                            return;
                        }
                    } catch (Exception e) {
                        Log.w(TAG, "profile parse failed: " + e.getMessage());
                        notifyError("account_error");
                        return;
                    }
                }

                if (code == 401 && hadToken) {
                    // Session expired/removed: silently fall back to device login.
                    Log.i(TAG, "stored session rejected, logging in again");
                    clearSession();
                    deviceLogin();
                    return;
                }
                notifyError(code == -1 ? "account_offline" : "account_error");
            }
        });
    }

    private void applyProfile(JsonObject root) {
        JsonObject profile = root.getAsJsonObject("profile");
        if (profile == null) {
            notifyError("account_error");
            return;
        }

        mProfile.loggedIn = true;
        mProfile.username = jsonString(profile, "username");
        mProfile.slug = jsonString(profile, "slug");
        mProfile.usernameSet = jsonBool(profile, "usernameSet");
        mProfile.linked = jsonBool(profile, "linked");
        mProfile.usernameChangeTime = jsonLong(profile, "usernameChangeTime");

        // The account loadout is authoritative (web `account.loadout`); the
        // C++ validator still normalizes it before the JoinMsg sees it.
        JsonElement loadout = root.get("loadout");
        if (loadout != null && loadout.isJsonObject()) {
            SurvBridge.setLoadout(loadout.toString());
        }

        Log.i(TAG, "profile loaded: username=" + mProfile.username
                + " usernameSet=" + mProfile.usernameSet
                + " slug=" + mProfile.slug);
        notifyChanged();
    }

    private String ensureDeviceId() {
        String deviceId = readPref(PREF_DEVICE_ID);
        if (!deviceId.isEmpty()) {
            return deviceId;
        }

        try {
            deviceId = Settings.Secure.getString(
                    mActivity.getContentResolver(), Settings.Secure.ANDROID_ID);
        } catch (Exception e) {
            deviceId = null;
        }
        if (deviceId != null) {
            deviceId = deviceId.replaceAll("[^A-Za-z0-9_-]", "");
        }
        if (deviceId == null || deviceId.length() < 6) {
            deviceId = UUID.randomUUID().toString();
        }

        writePref(PREF_DEVICE_ID, deviceId);
        Log.i(TAG, "device id created");
        return deviceId;
    }

    private void clearSession() {
        mToken = "";
        writePref(PREF_SESSION_TOKEN, "");
    }

    private void notifyChanged() {
        List<Listener> copy = new ArrayList<Listener>(mListeners);
        for (int i = 0; i < copy.size(); i++) {
            copy.get(i).onAccountChanged(mProfile);
        }
    }

    private void notifyError(String error) {
        List<Listener> copy = new ArrayList<Listener>(mListeners);
        for (int i = 0; i < copy.size(); i++) {
            copy.get(i).onAccountError(error);
        }
    }

    private String readPref(String key) {
        return mPrefs.getString(key, "");
    }

    private void writePref(String key, String value) {
        mPrefs.edit().putString(key, value != null ? value : "").apply();
    }

    private static String jsonString(JsonObject obj, String key) {
        JsonElement el = obj.get(key);
        return el != null && !el.isJsonNull() ? el.getAsString() : "";
    }

    private static boolean jsonBool(JsonObject obj, String key) {
        JsonElement el = obj.get(key);
        return el != null && !el.isJsonNull() && el.getAsBoolean();
    }

    private static long jsonLong(JsonObject obj, String key) {
        JsonElement el = obj.get(key);
        if (el == null || el.isJsonNull()) {
            return 0;
        }
        try {
            return Math.max(0, el.getAsLong());
        } catch (Exception e) {
            return 0;
        }
    }
}
