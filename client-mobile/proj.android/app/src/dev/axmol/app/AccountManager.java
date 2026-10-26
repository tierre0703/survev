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

import com.google.gson.JsonArray;
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

    /** One owned account item and its confirmation status (loadout.ItemStatus). */
    public static final class ItemInfo {
        public String type = "";
        public int status = 0; // 0=New, 1=Confirmed, 2=Ackd
        /** Account source type (web `item.source`, e.g. "pass"/"halloween"). */
        public String source = "";
        /** Milliseconds since epoch when the account acquired it (0 = unknown). */
        public long timeAcquired = 0;
    }

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

    /** Parsed `/api/user/get_pass` (shared/types/user.ts GetPassResponse). */
    public static final class PassInfo {
        public String type = "";
        public int level = 1;
        public int xp = 0;
        public boolean newItems = false;
        public final List<QuestInfo> quests = new ArrayList<QuestInfo>();
    }

    public static final class QuestInfo {
        public int idx;
        public String type = "";
        public int progress;
        public int target;
        public boolean complete;
        public boolean rerolled;
        public long timeToRefresh;
    }

    public interface PassCallback {
        void onResult(boolean success, PassInfo pass);
    }

    private final Activity mActivity;
    private final SharedPreferences mPrefs;
    private final List<Listener> mListeners = new ArrayList<Listener>();
    private final String mApiBaseUrl;

    private final Profile mProfile = new Profile();
    private final List<ItemInfo> mItems = new ArrayList<ItemInfo>();
    private String mDeviceId = "";
    private String mToken = "";
    private boolean mStarted = false;
    private boolean mBusy = false;

    private AccountManager(Activity activity) {
        mActivity = activity;
        mPrefs = activity.getSharedPreferences(PREFS_NAME, Activity.MODE_PRIVATE);
        mApiBaseUrl = SurvApi.apiBaseUrl();
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

    public List<ItemInfo> getItems() {
        return mItems;
    }

    /** The owned item types as a JSON array (SurvBridge.setAccountItems). */
    public String getItemTypesJson() {
        JsonArray types = new JsonArray();
        for (ItemInfo item : mItems) {
            types.add(item.type);
        }
        return types.toString();
    }

    public boolean isLoggedIn() {
        return mProfile.loggedIn;
    }

    /** The raw device-session token, for the `/team_v2` Bearer handshake. */
    public String getSessionToken() {
        return mProfile.loggedIn ? mToken : "";
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

    /**
     * Loads the battle pass + quests (`/api/user/get_pass`, web
     * `account.getPass`). `tryRefreshQuests` lets the server reroll expired
     * slots; callers pass true when opening the pass screen.
     */
    public void getPass(final boolean tryRefreshQuests, final PassCallback callback) {
        if (!mProfile.loggedIn || mToken.isEmpty()) {
            callback.onResult(false, null);
            return;
        }
        String body = "{\"tryRefreshQuests\":" + (tryRefreshQuests ? "true" : "false") + "}";
        SurvApi.userPost(mApiBaseUrl, "get_pass", mToken, body, new SurvApi.StatusCallback() {
            @Override
            public void onResult(int code, String json) {
                if (code != 200 || json == null) {
                    callback.onResult(false, null);
                    return;
                }
                try {
                    JsonObject root = JsonParser.parseString(json).getAsJsonObject();
                    if (!root.has("success") || !root.get("success").getAsBoolean()) {
                        callback.onResult(false, null);
                        return;
                    }

                    PassInfo pass = new PassInfo();
                    JsonObject p = root.getAsJsonObject("pass");
                    if (p != null) {
                        pass.type = jsonString(p, "type");
                        pass.level = jsonInt(p, "level", 1);
                        pass.xp = jsonInt(p, "xp", 0);
                        pass.newItems = jsonBool(p, "newItems");
                    }

                    JsonArray quests = root.getAsJsonArray("quests");
                    if (quests != null) {
                        for (int i = 0; i < quests.size(); i++) {
                            JsonElement el = quests.get(i);
                            if (el == null || !el.isJsonObject()) {
                                continue;
                            }
                            JsonObject q = el.getAsJsonObject();
                            QuestInfo quest = new QuestInfo();
                            quest.idx = jsonInt(q, "idx", 0);
                            quest.type = jsonString(q, "type");
                            quest.progress = jsonInt(q, "progress", 0);
                            quest.target = Math.max(1, jsonInt(q, "target", 1));
                            quest.complete = jsonBool(q, "complete");
                            quest.rerolled = jsonBool(q, "rerolled");
                            quest.timeToRefresh = Math.max(0, jsonLong(q, "timeToRefresh"));
                            pass.quests.add(quest);
                        }
                    }

                    Log.i(TAG, "pass loaded: type=" + pass.type + " level=" + pass.level
                            + " xp=" + pass.xp + " quests=" + pass.quests.size());
                    // account.ts:getPass: newly unlocked pass items mean the
                    // profile's item list is stale -> reload it (new-item badges).
                    if (pass.newItems) {
                        fetchProfile(false);
                    }
                    callback.onResult(true, pass);
                } catch (Exception e) {
                    Log.w(TAG, "get_pass parse failed: " + e.getMessage());
                    callback.onResult(false, null);
                }
            }
        });
    }

    /** Rerolls one quest slot (`/api/user/refresh_quest`). */
    public void refreshQuest(int idx, final SimpleCallback callback) {
        if (!mProfile.loggedIn || mToken.isEmpty()) {
            callback.onResult(false);
            return;
        }
        String body = "{\"idx\":" + idx + "}";
        SurvApi.userPost(mApiBaseUrl, "refresh_quest", mToken, body, new SurvApi.StatusCallback() {
            @Override
            public void onResult(int code, String json) {
                boolean success = false;
                if (code == 200 && json != null) {
                    try {
                        JsonObject root = JsonParser.parseString(json).getAsJsonObject();
                        success = root.has("success") && root.get("success").getAsBoolean();
                    } catch (Exception e) {
                        Log.w(TAG, "refresh_quest parse failed: " + e.getMessage());
                    }
                }
                callback.onResult(success);
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
    /**
     * Marks items Confirmed (1) or Ackd (2) on the account (web
     * `account.setItemStatus`). The local copy is updated first so the
     * customize screen does not fire duplicate requests on every refresh.
     */
    public void setItemStatus(final int status, List<String> itemTypes) {
        if (!mProfile.loggedIn || mToken.isEmpty() || itemTypes == null || itemTypes.isEmpty()) {
            return;
        }
        for (ItemInfo item : mItems) {
            for (String type : itemTypes) {
                if (item.type.equals(type)) {
                    item.status = Math.max(item.status, status);
                }
            }
        }
        JsonArray types = new JsonArray();
        for (String type : itemTypes) {
            types.add(type);
        }
        JsonObject body = new JsonObject();
        body.addProperty("status", status);
        body.add("itemTypes", types);
        SurvApi.userPost(mApiBaseUrl, "set_item_status", mToken, body.toString(),
                new SurvApi.StatusCallback() {
                    @Override
                    public void onResult(int code, String json) {
                        Log.i(TAG, "set_item_status " + status + " -> " + code);
                    }
                });
    }

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

        // Owned items first: the account loadout below is validated against the
        // owned set, so an item unlocked after the default loadout must be known
        // before we hand the server's selection to the C++ validator.
        JsonElement items = root.get("items");
        mItems.clear();
        if (items != null && items.isJsonArray()) {
            for (JsonElement item : items.getAsJsonArray()) {
                if (item == null || !item.isJsonObject()) continue;
                JsonObject obj = item.getAsJsonObject();
                JsonElement type = obj.get("type");
                if (type == null || !type.isJsonPrimitive()) continue;
                ItemInfo info = new ItemInfo();
                info.type = type.getAsString();
                try {
                    info.status = obj.has("status") ? obj.get("status").getAsInt() : 0;
                    info.source = obj.has("source") && !obj.get("source").isJsonNull()
                            ? obj.get("source").getAsString() : "";
                    info.timeAcquired = obj.has("timeAcquired") && !obj.get("timeAcquired").isJsonNull()
                            ? obj.get("timeAcquired").getAsLong() : 0L;
                } catch (Exception ignored) {
                    info.status = 0;
                }
                mItems.add(info);
            }
        }
        SurvBridge.setAccountItems(getItemTypesJson());

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
        // Do not leave the previous account's unlocks in the C++ owned set.
        mItems.clear();
        SurvBridge.setAccountItems("[]");
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

    private static int jsonInt(JsonObject obj, String key, int fallback) {
        JsonElement el = obj.get(key);
        if (el == null || el.isJsonNull()) {
            return fallback;
        }
        try {
            return el.getAsInt();
        } catch (Exception e) {
            return fallback;
        }
    }
}
