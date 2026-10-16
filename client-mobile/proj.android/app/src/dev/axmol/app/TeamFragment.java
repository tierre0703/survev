/****************************************************************************
 * TeamFragment.java — S5 team room screen (ARCHITECTURE.md §7, step S5).
 *
 * The Java team room: roster display, invite code, region/mode/auto-fill props
 * (leader-editable), Play/Leave buttons. Uses TeamSocket for the /team_v2
 * JSON control channel.
 *
 * Threading:
 *   * TeamSocket runs its own reader thread; callbacks arrive on that thread.
 *   * SurvBridge native calls (onJoinUrls, config setters) are marshalled to
 *     the UI thread via AppActivity.runOnUiThread.
 ****************************************************************************/
package dev.axmol.app;

import android.app.Activity;
import android.graphics.Color;
import android.util.Log;
import android.util.TypedValue;
import android.view.Gravity;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.FrameLayout;
import android.widget.LinearLayout;
import android.widget.ScrollView;
import android.widget.TextView;

import java.util.ArrayList;
import java.util.List;

public class TeamFragment {
    private static final String TAG = "TeamFragment";

    private final Activity mActivity;
    private final FrameLayout mRoot;

    // --- widgets ---
    private final TextView mTitle;
    private final TextView mInviteCode;
    private final TextView mStatus;
    private final LinearLayout mRoster;
    private final Button mRegionBtn;
    private final Button mModeBtn;
    private final Button mAutoFillBtn;
    private final Button mPlayBtn;
    private final Button mLeaveBtn;
    private final TextView mErrorText;

    // --- team state ---
    private TeamSocket mSocket;
    private boolean mIsLeader = false;
    private boolean mJoined = false;
    private boolean mFindingGame = false;
    private boolean mActive = false;
    private int mLocalPlayerId = 0;
    private String mRoomUrl = "";
    private String mRegion = "na";
    private int mGameModeIdx = 0;
    private boolean mAutoFill = true;
    private List<TeamPlayerInfo> mPlayers = new ArrayList<TeamPlayerInfo>();
    private List<Integer> mEnabledModes = new ArrayList<Integer>();
    private String mApiBaseUrl = "http://127.0.0.1:8000";
    private String mPlayerName = "Player";

    // Timer for keepAlive (called from UI thread every ~10s)
    private float mKeepAliveTicker = 0.0f;

    // Pending operations queued from callbacks
    private volatile boolean mPendingRefresh = false;
    private volatile String mPendingError = null;
    private volatile String mPendingJoinUrls = null;
    private volatile String mPendingJoinToken = null;

    public interface OnPlayListener {
        void onPlay(String[] urls, String joinToken);
    }
    private OnPlayListener mOnPlay;

    static class TeamPlayerInfo {
        int playerId;
        boolean isLeader;
        boolean inGame;
        String name;
    }

    public TeamFragment(Activity activity) {
        mActivity = activity;

        mRoot = new FrameLayout(activity);
        mRoot.setBackgroundColor(0xE00A0A0A);

        LinearLayout panel = new LinearLayout(activity);
        panel.setOrientation(LinearLayout.VERTICAL);
        panel.setGravity(Gravity.CENTER);
        panel.setLayoutParams(new FrameLayout.LayoutParams(
                ViewGroup.LayoutParams.WRAP_CONTENT, ViewGroup.LayoutParams.WRAP_CONTENT));
        panel.setBackgroundColor(0xFF1A1A1A);
        panel.setPadding(dp(20), dp(20), dp(20), dp(20));

        // Title
        mTitle = new TextView(activity);
        mTitle.setText("Team");
        mTitle.setTextColor(Color.WHITE);
        mTitle.setTextSize(TypedValue.COMPLEX_UNIT_SP, 24);
        mTitle.setGravity(Gravity.CENTER);
        panel.addView(mTitle);

        // Invite code
        mInviteCode = new TextView(activity);
        mInviteCode.setTextColor(0xFF8CDC8C);
        mInviteCode.setTextSize(TypedValue.COMPLEX_UNIT_SP, 18);
        mInviteCode.setGravity(Gravity.CENTER);
        panel.addView(mInviteCode);

        // Status
        mStatus = new TextView(activity);
        mStatus.setTextColor(0xFFC0C0C0);
        mStatus.setTextSize(TypedValue.COMPLEX_UNIT_SP, 14);
        mStatus.setGravity(Gravity.CENTER);
        panel.addView(mStatus);

        // Roster header
        TextView rosterHeader = new TextView(activity);
        rosterHeader.setText("Players");
        rosterHeader.setTextColor(Color.WHITE);
        rosterHeader.setTextSize(TypedValue.COMPLEX_UNIT_SP, 16);
        rosterHeader.setPadding(0, dp(8), 0, dp(4));
        panel.addView(rosterHeader);

        // Roster list
        ScrollView scroll = new ScrollView(activity);
        scroll.setLayoutParams(new FrameLayout.LayoutParams(dp(400), dp(250)));
        mRoster = new LinearLayout(activity);
        mRoster.setOrientation(LinearLayout.VERTICAL);
        scroll.addView(mRoster);
        panel.addView(scroll);

        // Props row
        LinearLayout propsRow = new LinearLayout(activity);
        propsRow.setOrientation(LinearLayout.HORIZONTAL);
        propsRow.setGravity(Gravity.CENTER);

        mRegionBtn = new Button(activity);
        mRegionBtn.setText("Region");
        mRegionBtn.setBackgroundColor(0xFF4A4A4A);
        mRegionBtn.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                if (mIsLeader) cycleRegion();
            }
        });
        propsRow.addView(mRegionBtn);

        mModeBtn = new Button(activity);
        mModeBtn.setText("Mode");
        mModeBtn.setBackgroundColor(0xFF4A4A4A);
        mModeBtn.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                if (mIsLeader && mEnabledModes.size() > 0) cycleMode();
            }
        });
        propsRow.addView(mModeBtn);

        mAutoFillBtn = new Button(activity);
        mAutoFillBtn.setText("Auto Fill");
        mAutoFillBtn.setBackgroundColor(0xFF4A4A4A);
        mAutoFillBtn.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                if (mIsLeader) toggleAutoFill();
            }
        });
        propsRow.addView(mAutoFillBtn);

        panel.addView(propsRow);

        // Error text
        mErrorText = new TextView(activity);
        mErrorText.setTextColor(0xFFFF7878);
        mErrorText.setTextSize(TypedValue.COMPLEX_UNIT_SP, 14);
        panel.addView(mErrorText);

        // Play + Leave
        LinearLayout buttonRow = new LinearLayout(activity);
        buttonRow.setOrientation(LinearLayout.HORIZONTAL);
        buttonRow.setGravity(Gravity.CENTER);

        mPlayBtn = new Button(activity);
        mPlayBtn.setText("Play");
        mPlayBtn.setBackgroundColor(0xFF83AF50);
        mPlayBtn.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                onPlay();
            }
        });
        buttonRow.addView(mPlayBtn);

        mLeaveBtn = new Button(activity);
        mLeaveBtn.setText("Leave Team");
        mLeaveBtn.setBackgroundColor(0xFF5A5A5A);
        mLeaveBtn.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                leaveTeam();
            }
        });
        buttonRow.addView(mLeaveBtn);

        panel.addView(buttonRow);

        mRoot.addView(panel);
    }

    public void setOnPlay(OnPlayListener listener) { mOnPlay = listener; }

    public View root() { return mRoot; }

    public void setApiBaseUrl(String url) { mApiBaseUrl = url; }

    public void setPlayerName(String name) { mPlayerName = name; }

    // --- Public API for JavaUiHost ---

    public void show() {
        mActive = true;
        mRoot.setVisibility(View.VISIBLE);
    }

    public void hide() {
        mActive = false;
        mRoot.setVisibility(View.GONE);
    }

    public boolean isActive() { return mActive; }

    public boolean isJoined() { return mJoined; }

    /** Create a new team room. */
    public void createTeam() {
        leaveTeam();
        connectTeamSocket(true, "");
    }

    /** Join an existing team by invite code. */
    public void joinTeam(String roomCode) {
        leaveTeam();
        connectTeamSocket(false, roomCode);
    }

    /** Called from the menu when a match ends (re-enters the room). */
    public void onGameComplete(String errorMessage) {
        sendJson("{\"type\":\"gameComplete\"}");
        refresh();
    }

    /** Called from the menu to leave and go back. */
    public void leaveTeam() {
        mActive = false;
        mJoined = false;
        mIsLeader = false;
        mFindingGame = false;
        mPlayers.clear();
        if (mSocket != null) {
            mSocket.close();
            mSocket = null;
        }
        updateRoster();
    }

    /** Per-frame keep-alive ticker (call from UI thread every frame). */
    public void update(float dt) {
        if (!mActive || !mJoined) return;

        mKeepAliveTicker += dt;
        if (mKeepAliveTicker >= 10.0f) {
            mKeepAliveTicker = 0.0f;
            sendJson("{\"type\":\"keepAlive\"}");
        }

        // Process pending callbacks on UI thread.
        checkPending();
    }

    /** Reads the C++ config for seed values. */
    public void seedFromConfig() {
        String config = SurvBridge.getConfigJson();
        String region = Json.peekString(config, "region");
        if (region != null && !region.isEmpty()) {
            mRegion = region;
        }
    }

    // --- TeamSocket callbacks ---

    private void onSocketMessage(String json) {
        // Parse message (simple approach — find the type field)
        String type = Json.peekString(json, "type");
        if (type == null) return;

        if ("state".equals(type)) {
            onState(json);
        } else if ("joinGame".equals(type)) {
            onJoinGame(json);
        } else if ("kicked".equals(type)) {
            mPendingError = "Kicked from team";
        } else if ("error".equals(type)) {
            String err = Json.peekString(json, "error");
            mPendingError = err != null ? err : "Team error";
        }
    }

    private void onSocketError(String reason) {
        mPendingError = reason;
    }

    private void onState(String json) {
        // Find "data" object
        int dataStart = json.indexOf("\"data\"");
        if (dataStart < 0) return;
        int colon = json.indexOf(':', dataStart);
        if (colon < 0) return;
        int objStart = json.indexOf('{', colon);
        if (objStart < 0) return;

        // Use our simple parser for the full state
        // For now keep the existing state parsing simple
        mJoined = true;

        // Read room data
        String room = extractNested(json, objStart);
        if (room != null) {
            String ru = Json.peekString(room, "roomUrl");
            if (ru != null) mRoomUrl = ru;
            String r = Json.peekString(room, "region");
            if (r != null) mRegion = r;
            String mi = Json.peekString(room, "gameModeIdx");
            if (mi != null) mGameModeIdx = Integer.parseInt(mi);
            String af = Json.peekString(room, "autoFill");
            if (af != null) mAutoFill = "true".equals(af);
            String fg = Json.peekString(room, "findingGame");
            if (fg != null) mFindingGame = "true".equals(fg);
            // enabledGameModeIdxs
            mEnabledModes = parseIntArray(room, "enabledGameModeIdxs");
        }

        // Read players
        mPlayers.clear();
        String playersStr = extractArray(json, objStart, "players");
        if (playersStr != null) {
            List<String> playerJsons = parseArrayValues(playersStr);
            for (String pj : playerJsons) {
                TeamPlayerInfo p = new TeamPlayerInfo();
                String pid = Json.peekString(pj, "playerId");
                if (pid != null) p.playerId = Integer.parseInt(pid);
                String il = Json.peekString(pj, "isLeader");
                p.isLeader = "true".equals(il);
                String ig = Json.peekString(pj, "inGame");
                p.inGame = "true".equals(ig);
                String nm = Json.peekString(pj, "name");
                if (nm != null) p.name = nm;
                mPlayers.add(p);
                if (p.playerId == mLocalPlayerId) {
                    mIsLeader = p.isLeader;
                }
            }
        }

        // Read localPlayerId
        String lpid = Json.peekString(json, "localPlayerId");
        if (lpid != null) mLocalPlayerId = Integer.parseInt(lpid);

        mPendingRefresh = true;
    }

    private void onJoinGame(String json) {
        // Parse the join match data
        String dataStr = extractNested(json, json.indexOf("\"data\""));
        if (dataStr == null) return;

        List<String> urls = parseArrayFromNested(dataStr, "urls");
        String token = Json.peekString(dataStr, "joinToken");

        if (urls.size() > 0 && token != null) {
            // Hand to C++ via bridge on UI thread.
            final String[] urlArray = urls.toArray(new String[urls.size()]);
            final String joinToken = token;
            AppActivity activity = AppActivity.current();
            if (activity != null) {
                activity.runOnUiThread(new Runnable() {
                    @Override
                    public void run() {
                        SurvBridge.onJoinUrls(urlArray, joinToken);
                        if (mOnPlay != null) {
                            mOnPlay.onPlay(urlArray, joinToken);
                        }
                    }
                });
            }
        }
    }

    // --- UI actions ---

    private void onPlay() {
        if (!mIsLeader || mFindingGame) return;
        mFindingGame = true;
        mErrorText.setText("Finding game...");
        sendJson("{\"type\":\"playGame\",\"data\":{\"region\":\"" + escapeJson(mRegion)
                + "\",\"version\":1028}}");
        refresh();
    }

    private void cycleRegion() {
        // Simple cycle between na, eu, as, sa, oce
        String[] regions = {"na", "eu", "as", "sa", "oce"};
        int idx = 0;
        for (int i = 0; i < regions.length; i++) {
            if (regions[i].equals(mRegion)) { idx = (i + 1) % regions.length; break; }
        }
        mRegion = regions[idx];
        sendRoomProps();
    }

    private void cycleMode() {
        if (mEnabledModes.size() <= 1) return;
        int idx = 0;
        for (int i = 0; i < mEnabledModes.size(); i++) {
            if (mEnabledModes.get(i) == mGameModeIdx) {
                idx = (i + 1) % mEnabledModes.size();
                break;
            }
        }
        mGameModeIdx = mEnabledModes.get(idx);
        sendRoomProps();
    }

    private void toggleAutoFill() {
        mAutoFill = !mAutoFill;
        sendRoomProps();
    }

    private void sendRoomProps() {
        sendJson("{\"type\":\"setRoomProps\",\"data\":{"
                + "\"roomUrl\":\"" + escapeJson(mRoomUrl) + "\","
                + "\"region\":\"" + escapeJson(mRegion) + "\","
                + "\"gameModeIdx\":" + mGameModeIdx + ","
                + "\"autoFill\":" + mAutoFill + ","
                + "\"findingGame\":" + mFindingGame
                + "}}");
        refresh();
    }

    // --- Socket connection ---

    private void connectTeamSocket(final boolean create, final String roomCode) {
        String wsUrl = mApiBaseUrl.replace("http://", "ws://")
                .replace("https://", "wss://") + "/team_v2";
        // Ensure at least scheme is present
        if (!wsUrl.startsWith("ws://") && !wsUrl.startsWith("wss://")) {
            wsUrl = "ws://" + wsUrl;
        }

        mSocket = new TeamSocket();
        final boolean isCreate = create;
        final String code = roomCode;
        final String name = mPlayerName;

        boolean ok = mSocket.connect(wsUrl,
            new TeamSocket.OnMessage() {
                @Override
                public void onMessage(String msg) {
                    onSocketMessage(msg);
                }
            },
            new TeamSocket.OnError() {
                @Override
                public void onError(String reason) {
                    onSocketError(reason);
                }
            });

        if (!ok) {
            mPendingError = "Cannot connect to team server";
            return;
        }

        // Send create/join
        if (isCreate) {
            String msg = "{\"type\":\"create\",\"data\":{"
                    + "\"roomData\":{\"roomUrl\":\"\",\"region\":\"" + escapeJson(mRegion)
                    + "\",\"gameModeIdx\":0,\"autoFill\":true,\"findingGame\":false},"
                    + "\"playerData\":{\"name\":\"" + escapeJson(name) + "\"}}}";
            sendJson(msg);
        } else {
            String msg = "{\"type\":\"join\",\"data\":{"
                    + "\"roomUrl\":\"" + escapeJson(code) + "\","
                    + "\"playerData\":{\"name\":\"" + escapeJson(name) + "\"}}}";
            sendJson(msg);
        }
    }

    private void sendJson(String json) {
        if (mSocket != null && mSocket.isRunning()) {
            mSocket.send(json);
        }
    }

    // --- UI refresh ---

    private void checkPending() {
        if (mPendingRefresh) {
            mPendingRefresh = false;
            refresh();
        }
        if (mPendingError != null) {
            String err = mPendingError;
            mPendingError = null;
            mErrorText.setText(err);
            if (mActive) {
                leaveTeam();
            }
        }
    }

    public void refresh() {
        mInviteCode.setText(mJoined ? "#" + mRoomUrl : "");
        mStatus.setText(mJoined ? "Invite code" : "Joining Team...");

        mRegionBtn.setText(mRegion.toUpperCase());
        mRegionBtn.setEnabled(mIsLeader && !mFindingGame);

        String modeLabel = "Mode " + (mGameModeIdx + 1);
        String[] modeNames = {"Solo", "Duo", "Squad"};
        if (mGameModeIdx >= 0 && mGameModeIdx < modeNames.length) {
            modeLabel = modeNames[mGameModeIdx];
        }
        mModeBtn.setText(modeLabel);
        mModeBtn.setEnabled(mIsLeader && !mFindingGame);

        mAutoFillBtn.setText(mAutoFill ? "Auto Fill" : "No Fill");
        mAutoFillBtn.setEnabled(mIsLeader && !mFindingGame);

        mPlayBtn.setVisibility(mIsLeader ? View.VISIBLE : View.GONE);
        mPlayBtn.setEnabled(mIsLeader && !mFindingGame);
        mPlayBtn.setText(mFindingGame ? "Finding game..." : "Play");

        updateRoster();
    }

    private void updateRoster() {
        mRoster.removeAllViews();
        for (TeamPlayerInfo p : mPlayers) {
            String text = p.name;
            if (p.isLeader) text += " *";
            if (p.inGame) text += " (in game)";
            TextView label = new TextView(mActivity);
            label.setText(text);
            label.setTextColor(p.inGame ? 0xFFB0B0B0 : Color.WHITE);
            label.setTextSize(TypedValue.COMPLEX_UNIT_SP, 15);
            label.setPadding(dp(8), 0, 0, 0);
            mRoster.addView(label);
        }
    }

    // --- JSON helpers ---

    private String extractNested(String json, int startAt) {
        int brace = json.indexOf('{', startAt);
        if (brace < 0) return null;
        int depth = 1;
        for (int i = brace + 1; i < json.length(); i++) {
            char c = json.charAt(i);
            if (c == '{') depth++;
            else if (c == '}') {
                depth--;
                if (depth == 0) return json.substring(brace, i + 1);
            }
        }
        return null;
    }

    private String extractArray(String json, int startAt, String key) {
        int at = json.indexOf("\"" + key + "\"", startAt);
        if (at < 0) return null;
        int colon = json.indexOf(':', at);
        if (colon < 0) return null;
        return extractNested(json, colon);
    }

    private List<String> parseArrayValues(String arrayJson) {
        List<String> values = new ArrayList<String>();
        if (arrayJson == null) return values;
        int brace = 0, depth = 0;
        int objStart = -1;
        for (int i = 0; i < arrayJson.length(); i++) {
            char c = arrayJson.charAt(i);
            if (c == '{') {
                if (depth == 0) objStart = i;
                depth++;
            } else if (c == '}') {
                depth--;
                if (depth == 0 && objStart >= 0) {
                    values.add(arrayJson.substring(objStart, i + 1));
                    objStart = -1;
                }
            }
        }
        return values;
    }

    private List<String> parseArrayFromNested(String json, String key) {
        List<String> items = new ArrayList<String>();
        if (json == null) return items;
        int at = json.indexOf("\"" + key + "\"");
        if (at < 0) return items;
        int colon = json.indexOf(':', at);
        if (colon < 0) return items;
        int arrayStart = json.indexOf('[', colon);
        if (arrayStart < 0) return items;
        StringBuilder seg = new StringBuilder();
        boolean inString = false;
        for (int i = arrayStart; i < json.length(); i++) {
            char c = json.charAt(i);
            if (inString) {
                if (c == '\\') { seg.append(c); i++; if (i < json.length()) seg.append(json.charAt(i)); }
                else if (c == '"') {
                    items.add(seg.toString());
                    seg = new StringBuilder();
                    inString = false;
                } else { seg.append(c); }
            } else {
                if (c == '"') inString = true;
                else if (c == '[') { /* skip */ }
                else if (c == ']') break;
            }
        }
        return items;
    }

    private List<Integer> parseIntArray(String json, String key) {
        List<Integer> items = new ArrayList<Integer>();
        if (json == null) return items;
        int at = json.indexOf("\"" + key + "\"");
        if (at < 0) return items;
        int colon = json.indexOf(':', at);
        if (colon < 0) return items;
        int arrayStart = json.indexOf('[', colon);
        if (arrayStart < 0) return items;
        StringBuilder seg = new StringBuilder();
        for (int i = arrayStart; i < json.length(); i++) {
            char c = json.charAt(i);
            if (c >= '0' && c <= '9') seg.append(c);
            else if (c == ',' || c == ']') {
                if (seg.length() > 0) {
                    items.add(Integer.parseInt(seg.toString()));
                    seg = new StringBuilder();
                }
                if (c == ']') break;
            }
        }
        return items;
    }

    private String escapeJson(String s) {
        if (s == null) return "";
        StringBuilder out = new StringBuilder();
        for (int i = 0; i < s.length(); i++) {
            char c = s.charAt(i);
            switch (c) {
                case '"': out.append("\\\""); break;
                case '\\': out.append("\\\\"); break;
                case '\n': out.append("\\n"); break;
                case '\r': out.append("\\r"); break;
                default: out.append(c); break;
            }
        }
        return out.toString();
    }

    private int dp(int value) {
        return (int) (value * mActivity.getResources().getDisplayMetrics().density + 0.5f);
    }
}