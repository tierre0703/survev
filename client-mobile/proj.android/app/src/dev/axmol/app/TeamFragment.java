/****************************************************************************
 * TeamFragment.java - team room screen (client/src/ui/teamMenu.ts, /team_v2).
 *
 * One JSON WebSocket drives the screen (shared/types/team.ts):
 *   client -> server: create, join, setRoomProps, playGame, kick, keepAlive,
 *                     changeName, gameComplete
 *   server -> client: state, joinGame, keepAlive, kicked, error
 *
 * The invite link is a normal web link (`https://<webBaseUrl>/#CODE`) so browser
 * players can join; the join field accepts that link, `survev://team/CODE` or a
 * bare code (TeamLinks.extractCode). The socket sends the device account's
 * Bearer token, so the server records team stats like the web cookie does.
 *
 * Threading: TeamSocket callbacks arrive on its reader thread; everything here
 * posts to the UI handler before touching Views or SurvBridge.
 ****************************************************************************/
package dev.axmol.app;

import android.app.Activity;
import android.content.ClipData;
import android.content.ClipboardManager;
import android.content.Context;
import android.content.Intent;
import android.graphics.Color;
import android.graphics.drawable.GradientDrawable;
import android.os.Handler;
import android.os.Looper;
import android.util.Log;
import android.util.TypedValue;
import android.view.Gravity;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.EditText;
import android.widget.FrameLayout;
import android.widget.LinearLayout;
import android.widget.ScrollView;
import android.widget.TextView;
import android.widget.Toast;

import com.google.gson.JsonArray;
import com.google.gson.JsonElement;
import com.google.gson.JsonObject;
import com.google.gson.JsonParser;

import java.util.ArrayList;
import java.util.List;

public class TeamFragment {
    private static final String TAG = "TeamFragment";

    private static final int TEAL = 0xFF4FADA9;
    private static final int GREEN = 0xFF83AF50;
    private static final int GREEN_SHADOW = 0xFF5B7A38;
    private static final int RED = 0xFFE57373;
    private static final int GREY = 0xFF666666;
    private static final int GOLD = 0xFFFFD700;
    private static final int PANEL_BG = 0xF01A1A2E;

    private final Activity mActivity;
    private final JavaUiHost mHost;
    private final FrameLayout mRoot;
    private final Handler mHandler = new Handler(Looper.getMainLooper());

    // --- widgets ---
    private final TextView mTitleLabel;
    private final TextView mCodeLabel;
    private final TextView mLinkLabel;
    private final Button mCopyBtn;
    private final Button mShareBtn;
    private final EditText mJoinInput;
    private final Button mJoinBtn;
    private final LinearLayout mJoinRow;
    private final TextView mRosterHeader;
    private final LinearLayout mRosterList;
    private final Button mRegionBtn;
    private final Button mModeBtn;
    private final Button mAutoFillBtn;
    private final TextView mStatusText;
    private final Button mPlayBtn;
    private final Button mLeaveBtn;

    // --- state ---
    private boolean mOpen = false;
    private boolean mCreate = true;
    private boolean mJoined = false;
    private boolean mInGame = false;
    private boolean mFindingGame = false;
    private boolean mCaptchaEnabled = false;
    private boolean mAutoFill = true;
    private boolean mUserClosing = false;
    private String mCode = "";
    private String mRoomUrl = "";
    private String mRegion = "na";
    private String mPlayerName = "Player";
    private int mLocalPlayerId = -1;
    private int mGameModeIdx = 1;
    private int mMaxPlayers = 4;
    private TeamSocket mWs = null;

    private final List<TeamPlayer> mPlayers = new ArrayList<TeamPlayer>();
    private final List<Integer> mEnabledModes = new ArrayList<Integer>();
    private final List<String> mRegions = new ArrayList<String>();
    private final List<Integer> mSiteTeamModes = new ArrayList<Integer>();

    private final Runnable mKeepAlive = new Runnable() {
        @Override
        public void run() {
            if (!mOpen && !mInGame) {
                return;
            }
            if (mJoined && mWs != null && mWs.isRunning()) {
                mWs.send("{\"type\":\"keepAlive\"}");
            }
            mHandler.postDelayed(this, 10000);
        }
    };

    public TeamFragment(Activity activity, JavaUiHost host) {
        mActivity = activity;
        mHost = host;

        mRoot = new FrameLayout(activity);
        mRoot.setBackgroundColor(0xCC000000);
        mRoot.setVisibility(View.GONE);

        ScrollView outerScroll = new ScrollView(activity);
        outerScroll.setLayoutParams(new FrameLayout.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.MATCH_PARENT));

        LinearLayout panel = new LinearLayout(activity);
        panel.setOrientation(LinearLayout.VERTICAL);
        panel.setGravity(Gravity.CENTER);
        panel.setBackgroundColor(PANEL_BG);
        panel.setPadding(dp(16), dp(14), dp(16), dp(14));
        FrameLayout.LayoutParams panelParams = new FrameLayout.LayoutParams(
                dp(390), ViewGroup.LayoutParams.WRAP_CONTENT);
        panelParams.gravity = Gravity.CENTER;
        panel.setLayoutParams(panelParams);

        // ===== Header =====
        mTitleLabel = new TextView(activity);
        mTitleLabel.setText("\uD83D\uDC65 Team");
        mTitleLabel.setTextColor(Color.WHITE);
        mTitleLabel.setTextSize(TypedValue.COMPLEX_UNIT_SP, 18);
        mTitleLabel.setGravity(Gravity.CENTER);
        panel.addView(mTitleLabel);
        addGap(panel, dp(8));

        // ===== Invite code + link =====
        LinearLayout inviteRow = new LinearLayout(activity);
        inviteRow.setOrientation(LinearLayout.HORIZONTAL);
        inviteRow.setGravity(Gravity.CENTER_VERTICAL);

        mCodeLabel = new TextView(activity);
        mCodeLabel.setText("Invite code: ----");
        mCodeLabel.setTextColor(GOLD);
        mCodeLabel.setTextSize(TypedValue.COMPLEX_UNIT_SP, 14);
        mCodeLabel.setTypeface(null, android.graphics.Typeface.BOLD);
        mCodeLabel.setLayoutParams(new LinearLayout.LayoutParams(
                0, ViewGroup.LayoutParams.WRAP_CONTENT, 1f));
        inviteRow.addView(mCodeLabel);

        mCopyBtn = new Button(activity);
        mCopyBtn.setText("\uD83D\uDCCB Copy");
        mCopyBtn.setBackgroundColor(TEAL);
        mCopyBtn.setTextColor(Color.WHITE);
        mCopyBtn.setTextSize(TypedValue.COMPLEX_UNIT_SP, 11);
        mCopyBtn.setOnClickListener(new View.OnClickListener() {
            @Override public void onClick(View v) { copyInviteLink(); }
        });
        inviteRow.addView(mCopyBtn, new LinearLayout.LayoutParams(dp(72), dp(34)));

        mShareBtn = new Button(activity);
        mShareBtn.setText("\u2797 Share");
        mShareBtn.setBackgroundColor(0xFF4A90D9);
        mShareBtn.setTextColor(Color.WHITE);
        mShareBtn.setTextSize(TypedValue.COMPLEX_UNIT_SP, 11);
        LinearLayout.LayoutParams shareLp = new LinearLayout.LayoutParams(dp(76), dp(34));
        shareLp.leftMargin = dp(4);
        mShareBtn.setOnClickListener(new View.OnClickListener() {
            @Override public void onClick(View v) { shareInviteLink(); }
        });
        inviteRow.addView(mShareBtn, shareLp);
        panel.addView(inviteRow);

        mLinkLabel = new TextView(activity);
        mLinkLabel.setTextColor(0xFFAAAAAA);
        mLinkLabel.setTextSize(TypedValue.COMPLEX_UNIT_SP, 10);
        mLinkLabel.setTextIsSelectable(true);
        mLinkLabel.setPadding(0, dp(2), 0, dp(4));
        panel.addView(mLinkLabel);

        // ===== Join input (only until connected) =====
        mJoinRow = new LinearLayout(activity);
        mJoinRow.setOrientation(LinearLayout.HORIZONTAL);
        mJoinRow.setGravity(Gravity.CENTER_VERTICAL);

        mJoinInput = new EditText(activity);
        mJoinInput.setSingleLine(true);
        mJoinInput.setHint("Paste invite link or code");
        mJoinInput.setTextColor(Color.WHITE);
        mJoinInput.setHintTextColor(0xFF888888);
        mJoinInput.setTextSize(TypedValue.COMPLEX_UNIT_SP, 13);
        mJoinInput.setBackgroundColor(0xFF333333);
        mJoinInput.setPadding(dp(8), dp(4), dp(8), dp(4));
        mJoinInput.setLayoutParams(new LinearLayout.LayoutParams(0, dp(38), 1f));
        mJoinRow.addView(mJoinInput);

        mJoinBtn = new Button(activity);
        mJoinBtn.setText("Join");
        mJoinBtn.setBackgroundColor(TEAL);
        mJoinBtn.setTextColor(Color.WHITE);
        mJoinBtn.setTextSize(TypedValue.COMPLEX_UNIT_SP, 13);
        mJoinBtn.setOnClickListener(new View.OnClickListener() {
            @Override public void onClick(View v) { joinFromInput(); }
        });
        LinearLayout.LayoutParams joinLp = new LinearLayout.LayoutParams(dp(76), dp(38));
        joinLp.leftMargin = dp(4);
        mJoinRow.addView(mJoinBtn, joinLp);
        panel.addView(mJoinRow);

        addGap(panel, dp(8));

        // ===== Roster =====
        mRosterHeader = new TextView(activity);
        mRosterHeader.setText("Team Members (0/4)");
        mRosterHeader.setTextColor(0xFFAAAAAA);
        mRosterHeader.setTextSize(TypedValue.COMPLEX_UNIT_SP, 12);
        panel.addView(mRosterHeader);

        ScrollView rosterScroll = new ScrollView(activity);
        rosterScroll.setLayoutParams(new FrameLayout.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT, dp(150)));
        mRosterList = new LinearLayout(activity);
        mRosterList.setOrientation(LinearLayout.VERTICAL);
        rosterScroll.addView(mRosterList);
        panel.addView(rosterScroll);

        addGap(panel, dp(6));

        // ===== Room properties (leader) =====
        LinearLayout propsRow = new LinearLayout(activity);
        propsRow.setOrientation(LinearLayout.HORIZONTAL);
        mRegionBtn = makeSmallButton("Region: Local");
        mRegionBtn.setOnClickListener(new View.OnClickListener() {
            @Override public void onClick(View v) { cycleRegion(); }
        });
        propsRow.addView(mRegionBtn, smallParams(true));
        mModeBtn = makeSmallButton("Mode: Squad");
        mModeBtn.setOnClickListener(new View.OnClickListener() {
            @Override public void onClick(View v) { cycleMode(); }
        });
        propsRow.addView(mModeBtn, smallParams(false));
        mAutoFillBtn = makeSmallButton("Auto-fill: ON");
        mAutoFillBtn.setOnClickListener(new View.OnClickListener() {
            @Override public void onClick(View v) { toggleAutoFill(); }
        });
        propsRow.addView(mAutoFillBtn, smallParams(false));
        panel.addView(propsRow);

        // ===== Status =====
        mStatusText = new TextView(activity);
        mStatusText.setTextColor(0xFFAAAAAA);
        mStatusText.setTextSize(TypedValue.COMPLEX_UNIT_SP, 11);
        mStatusText.setGravity(Gravity.CENTER);
        mStatusText.setPadding(0, dp(4), 0, dp(4));
        panel.addView(mStatusText);

        // ===== Actions =====
        LinearLayout btnRow = new LinearLayout(activity);
        btnRow.setOrientation(LinearLayout.HORIZONTAL);
        mPlayBtn = new Button(activity);
        mPlayBtn.setText("\u25B6 Play");
        mPlayBtn.setBackgroundColor(GREEN);
        mPlayBtn.setShadowLayer(dp(2), 0, dp(2), GREEN_SHADOW);
        mPlayBtn.setTextColor(Color.WHITE);
        mPlayBtn.setTextSize(TypedValue.COMPLEX_UNIT_SP, 16);
        mPlayBtn.setEnabled(false);
        LinearLayout.LayoutParams playLp = new LinearLayout.LayoutParams(0, dp(42), 1f);
        playLp.rightMargin = dp(4);
        mPlayBtn.setOnClickListener(new View.OnClickListener() {
            @Override public void onClick(View v) { startGame(); }
        });
        btnRow.addView(mPlayBtn, playLp);

        mLeaveBtn = new Button(activity);
        mLeaveBtn.setText("Leave \u2716");
        mLeaveBtn.setBackgroundColor(RED);
        mLeaveBtn.setTextColor(Color.WHITE);
        mLeaveBtn.setTextSize(TypedValue.COMPLEX_UNIT_SP, 16);
        LinearLayout.LayoutParams leaveLp = new LinearLayout.LayoutParams(0, dp(42), 1f);
        leaveLp.leftMargin = dp(4);
        mLeaveBtn.setOnClickListener(new View.OnClickListener() {
            @Override public void onClick(View v) { leave(); }
        });
        btnRow.addView(mLeaveBtn, leaveLp);
        panel.addView(btnRow);

        outerScroll.addView(panel);
        mRoot.addView(outerScroll);

        loadSiteInfo("");
        render();
    }

    public interface OnCloseListener {
        void onClose();
    }

    private OnCloseListener mOnClose;

    public void setOnClose(OnCloseListener listener) {
        mOnClose = listener;
    }

    // --- public API ---------------------------------------------------------

    public View root() {
        return mRoot;
    }

    public boolean isOpen() {
        return mOpen;
    }

    public boolean isInGame() {
        return mInGame;
    }

    /** Entry point from the menu / deep link. */
    public void connect(boolean create, String rawLink) {
        closeSocket();
        loadSiteInfo(mHost.siteInfoJson());
        mCreate = create;
        mCode = TeamLinks.extractCode(rawLink);
        mRoomUrl = mCode.isEmpty() ? "" : "#" + mCode;
        mPlayers.clear();
        mJoined = false;
        mInGame = false;
        mFindingGame = false;
        mCaptchaEnabled = false;
        mAutoFill = true;
        mUserClosing = false;
        mLocalPlayerId = -1;
        mMaxPlayers = 4;
        mPlayerName = getPlayerName();
        mRegion = defaultRegion();
        mGameModeIdx = defaultGameMode();
        mJoinInput.setText(create ? "" : mCode);
        mJoinRow.setVisibility(create ? View.GONE : View.VISIBLE);
        mStatusText.setText("");

        show();
        if (create) {
            mTitleLabel.setText("\uD83D\uDC65 Create Team");
            mJoinRow.setVisibility(View.GONE);
            openSocket();
        } else if (mCode.isEmpty()) {
            mTitleLabel.setText("\uD83D\uDC65 Join Team");
            setStatus(l10n("index-enter-invite", "Enter an invite link or team code."));
        } else {
            mTitleLabel.setText("\uD83D\uDC65 Join Team");
            mJoinRow.setVisibility(View.GONE);
            openSocket();
        }
        render();
    }

    public void showMessage(String text) {
        setStatus(text != null ? text : "");
    }

    /** A match started from this team: hide the UI but keep the socket. */
    public void hideForGame() {
        if (mInGame) {
            return;
        }
        mInGame = true;
        mOpen = false;
        mRoot.setVisibility(View.GONE);
        Log.i(TAG, "team hidden for game");
    }

    /** Returning from a team match (web `onGameComplete`). */
    public void onGameComplete() {
        if (!mInGame) {
            return;
        }
        mInGame = false;
        if (mWs != null && mWs.isRunning()) {
            mWs.send("{\"type\":\"gameComplete\"}");
        }
        mOpen = true;
        mRoot.setVisibility(View.VISIBLE);
        render();
        Log.i(TAG, "team game complete");
    }

    public void refresh() {
        render();
    }

    // --- UI construction helpers -------------------------------------------

    private Button makeSmallButton(String label) {
        Button b = new Button(mActivity);
        b.setText(label);
        b.setTextColor(Color.WHITE);
        b.setTextSize(TypedValue.COMPLEX_UNIT_SP, 10);
        b.setBackgroundColor(0xFF3A3A3A);
        b.setAllCaps(false);
        return b;
    }

    private LinearLayout.LayoutParams smallParams(boolean first) {
        LinearLayout.LayoutParams lp = new LinearLayout.LayoutParams(0, dp(32), 1f);
        lp.rightMargin = first ? dp(4) : 0;
        if (!first) {
            lp.leftMargin = dp(2);
        }
        return lp;
    }

    // --- connection ---------------------------------------------------------

    private void openSocket() {
        setStatus(mCreate ? "Creating team..." : "Joining team...");
        if (mCreate && mCode.isEmpty()) {
            mRoomUrl = "";
        }
        String token = "";
        AccountManager account = AccountManager.peek();
        if (account != null) {
            token = account.getSessionToken();
        }
        final TeamSocket ws = new TeamSocket();
        mWs = ws;
        boolean started = ws.connect(SurvApi.teamWsUrl(), token,
                new TeamSocket.OnOpen() {
                    @Override public void onOpen() {
                        mHandler.post(new Runnable() {
                            @Override public void run() {
                                if (mWs != ws) return;
                                sendHello(ws);
                            }
                        });
                    }
                },
                new TeamSocket.OnMessage() {
                    @Override public void onMessage(final String text) {
                        mHandler.post(new Runnable() {
                            @Override public void run() {
                                if (mWs == ws) handleMessage(text);
                            }
                        });
                    }
                },
                new TeamSocket.OnError() {
                    @Override public void onError(final String reason) {
                        mHandler.post(new Runnable() {
                            @Override public void run() {
                                if (mWs == ws) handleSocketError(reason);
                            }
                        });
                    }
                });
        if (!started) {
            setStatus("Invalid team server URL.");
        }
    }

    private void sendHello(TeamSocket ws) {
        String name = SurvApi.escapeJson(mPlayerName);
        if (mCreate) {
            String msg = "{\"type\":\"create\",\"data\":{\"roomData\":{"
                    + "\"roomUrl\":\"\",\"findingGame\":false,"
                    + "\"region\":\"" + SurvApi.escapeJson(mRegion) + "\","
                    + "\"autoFill\":" + (mAutoFill ? "true" : "false") + ","
                    + "\"gameModeIdx\":" + mGameModeIdx + "},"
                    + "\"playerData\":{\"name\":\"" + name + "\"}}}";
            ws.send(msg);
        } else {
            String msg = "{\"type\":\"join\",\"data\":{\"roomUrl\":\""
                    + SurvApi.escapeJson(mCode) + "\","
                    + "\"playerData\":{\"name\":\"" + name + "\"}}}";
            ws.send(msg);
        }
    }

    private void handleSocketError(String reason) {
        if (mUserClosing) {
            return;
        }
        Log.w(TAG, "team socket error: " + reason);
        if (mJoined) {
            mHost.returnToMenu("index-lost-connection");
        } else {
            setStatus(l10n("index-lost-connection", "Lost connection to team."));
            mJoinBtn.setEnabled(true);
            if (!mCreate) {
                mJoinRow.setVisibility(View.VISIBLE);
            }
        }
    }

    // --- protocol -----------------------------------------------------------

    private void handleMessage(String json) {
        try {
            JsonObject root = JsonParser.parseString(json).getAsJsonObject();
            String type = root.has("type") ? root.get("type").getAsString() : "";
            JsonObject data = root.has("data") && root.get("data").isJsonObject()
                    ? root.getAsJsonObject("data")
                    : new JsonObject();

            if ("state".equals(type)) {
                applyState(data);
            } else if ("joinGame".equals(type)) {
                applyJoinGame(data);
            } else if ("error".equals(type)) {
                String err = data.has("type") ? data.get("type").getAsString() : "lost_conn";
                Log.w(TAG, "team error: " + err);
                closeSocket();
                mHost.returnToMenu(errorL10nKey(err));
            } else if ("kicked".equals(type)) {
                closeSocket();
                mHost.returnToMenu("index-team-kicked");
            }
            // keepAlive: no-op
        } catch (Exception e) {
            Log.w(TAG, "team message parse failed: " + e.getMessage());
        }
    }

    private void applyState(JsonObject data) {
        mJoined = true;
        mHandler.removeCallbacks(mKeepAlive);
        mHandler.postDelayed(mKeepAlive, 10000);

        mLocalPlayerId = data.has("localPlayerId") ? data.get("localPlayerId").getAsInt() : -1;

        JsonObject room = data.has("room") && data.get("room").isJsonObject()
                ? data.getAsJsonObject("room")
                : null;
        if (room != null) {
            String roomUrl = str(room, "roomUrl");
            mRoomUrl = roomUrl;
            mCode = roomUrl.startsWith("#") ? roomUrl.substring(1) : roomUrl;
            mRegion = str(room, "region");
            mAutoFill = bool(room, "autoFill", true);
            mGameModeIdx = intVal(room, "gameModeIdx", 1);
            mMaxPlayers = intVal(room, "maxPlayers", 4);
            mFindingGame = bool(room, "findingGame", false);
            mCaptchaEnabled = bool(room, "captchaEnabled", false);
            if (room.has("enabledGameModeIdxs") && room.get("enabledGameModeIdxs").isJsonArray()) {
                mEnabledModes.clear();
                JsonArray modes = room.getAsJsonArray("enabledGameModeIdxs");
                for (int i = 0; i < modes.size(); i++) {
                    mEnabledModes.add(modes.get(i).getAsInt());
                }
            }
            String lastError = str(room, "lastError");
            if (!lastError.isEmpty()) {
                setStatus(errorText(lastError));
            } else {
                setStatus(mFindingGame ? "Finding game..." : "");
            }
        }

        mPlayers.clear();
        if (data.has("players") && data.get("players").isJsonArray()) {
            JsonArray players = data.getAsJsonArray("players");
            for (int i = 0; i < players.size(); i++) {
                JsonElement el = players.get(i);
                if (el == null || !el.isJsonObject()) continue;
                JsonObject p = el.getAsJsonObject();
                TeamPlayer player = new TeamPlayer();
                player.name = str(p, "name");
                player.playerId = intVal(p, "playerId", -1);
                player.isLeader = bool(p, "isLeader", false);
                player.inGame = bool(p, "inGame", false);
                mPlayers.add(player);
            }
        }

        mJoinRow.setVisibility(View.GONE);
        render();
    }

    private void applyJoinGame(JsonObject data) {
        List<String> urls = new ArrayList<String>();
        if (data.has("urls") && data.get("urls").isJsonArray()) {
            JsonArray arr = data.getAsJsonArray("urls");
            for (int i = 0; i < arr.size(); i++) {
                urls.add(arr.get(i).getAsString());
            }
        }
        String token = str(data, "joinToken");
        if (urls.isEmpty() || token.isEmpty()) {
            setStatus("Invalid join data from team server.");
            return;
        }
        setStatus("Joining game...");
        hideForGame();
        SurvBridge.onJoinUrls(urls.toArray(new String[0]), token);
    }

    // --- rendering ----------------------------------------------------------

    private void render() {
        mTitleLabel.setText(mCreate ? "\uD83D\uDC65 Create Team" : "\uD83D\uDC65 Join Team");
        mCodeLabel.setText(mCode.isEmpty() ? "Invite code: ----" : "Invite code: " + mCode);
        String link = TeamLinks.inviteUrl(mCode);
        mLinkLabel.setText(link);
        boolean hasLink = !link.isEmpty();
        mCopyBtn.setEnabled(hasLink);
        mShareBtn.setEnabled(hasLink);
        if (!mJoined) {
            mJoinRow.setVisibility(mCreate ? View.GONE : View.VISIBLE);
        }

        // Roster
        mRosterHeader.setText("Team Members (" + mPlayers.size() + "/" + mMaxPlayers + ")");
        mRosterList.removeAllViews();
        for (int i = 0; i < mPlayers.size(); i++) {
            renderPlayer(mPlayers.get(i));
        }
        if (mPlayers.isEmpty()) {
            TextView hint = new TextView(mActivity);
            hint.setText(mCreate ? "Waiting for players..." : "Connecting...");
            hint.setTextColor(0xFF777777);
            hint.setTextSize(TypedValue.COMPLEX_UNIT_SP, 11);
            mRosterList.addView(hint);
        }

        // Room props
        mRegionBtn.setText("Region: " + regionName(mRegion));
        mModeBtn.setText("Mode: " + modeName(mGameModeIdx));
        mAutoFillBtn.setText("Auto-fill: " + (mAutoFill ? "ON" : "OFF"));
        boolean leader = isLeader();
        setPropEnabled(mRegionBtn, leader && mJoined);
        setPropEnabled(mModeBtn, leader && mJoined);
        setPropEnabled(mAutoFillBtn, leader && mJoined);

        // Play
        boolean anyInGame = false;
        for (int i = 0; i < mPlayers.size(); i++) {
            anyInGame |= mPlayers.get(i).inGame;
        }
        if (leader) {
            AccountManager account = AccountManager.peek();
            boolean signedIn = account != null && account.isLoggedIn();
            boolean captchaBlocks = mCaptchaEnabled && !signedIn;
            boolean canPlay = !mFindingGame && !anyInGame && !captchaBlocks;
            mPlayBtn.setEnabled(canPlay);
            mPlayBtn.setText(mFindingGame ? "Finding game..."
                    : "Play (" + mPlayers.size() + ")");
            if (captchaBlocks) {
                mPlayBtn.setText("Captcha required");
            }
        } else {
            mPlayBtn.setEnabled(false);
            mPlayBtn.setText("Waiting for leader");
        }
    }

    private void renderPlayer(final TeamPlayer player) {
        LinearLayout card = new LinearLayout(mActivity);
        card.setOrientation(LinearLayout.HORIZONTAL);
        card.setGravity(Gravity.CENTER_VERTICAL);
        card.setPadding(dp(8), dp(6), dp(8), dp(6));

        GradientDrawable bg = new GradientDrawable();
        bg.setColor(player.isLeader ? 0x33FFD700 : 0x22FFFFFF);
        bg.setCornerRadius(dp(6));
        bg.setStroke(player.playerId == mLocalPlayerId ? dp(2) : dp(1),
                player.isLeader ? GOLD : 0x33FFFFFF);
        card.setBackground(bg);

        TextView icon = new TextView(mActivity);
        icon.setText(player.isLeader ? "\uD83D\uDC51" : (player.inGame ? "\uD83C\uDFAE" : "\u2705"));
        icon.setTextSize(TypedValue.COMPLEX_UNIT_SP, 13);
        card.addView(icon);

        TextView name = new TextView(mActivity);
        name.setText(player.name
                + (player.playerId == mLocalPlayerId ? " (YOU)" : "")
                + (player.inGame ? "  IN GAME" : ""));
        name.setTextColor(player.isLeader ? GOLD : Color.WHITE);
        name.setTextSize(TypedValue.COMPLEX_UNIT_SP, 13);
        name.setPadding(dp(6), 0, 0, 0);
        name.setLayoutParams(new LinearLayout.LayoutParams(
                0, ViewGroup.LayoutParams.WRAP_CONTENT, 1f));
        card.addView(name);

        if (isLeader() && player.playerId != mLocalPlayerId) {
            Button kick = new Button(mActivity);
            kick.setText("Kick");
            kick.setBackgroundColor(0xFF8B3A3A);
            kick.setTextColor(Color.WHITE);
            kick.setTextSize(TypedValue.COMPLEX_UNIT_SP, 9);
            kick.setOnClickListener(new View.OnClickListener() {
                @Override public void onClick(View v) {
                    send("{\"type\":\"kick\",\"data\":{\"playerId\":" + player.playerId + "}}");
                }
            });
            card.addView(kick, new LinearLayout.LayoutParams(dp(56), dp(30)));
        }

        mRosterList.addView(card);
    }

    // --- actions ------------------------------------------------------------

    private void joinFromInput() {
        String code = TeamLinks.extractCode(mJoinInput.getText().toString());
        if (code.isEmpty()) {
            setStatus(l10n("index-enter-invite", "Enter an invite link or team code."));
            return;
        }
        mCode = code;
        mRoomUrl = "#" + code;
        mCreate = false;
        mJoinBtn.setEnabled(false);
        mJoinInput.setText(code);
        closeSocket();
        openSocket();
        render();
    }

    private void cycleRegion() {
        if (!isLeader() || mRegions.isEmpty()) return;
        int idx = mRegions.indexOf(mRegion);
        mRegion = mRegions.get((idx + 1) % mRegions.size());
        sendRoomProps();
        render();
    }

    private void cycleMode() {
        if (!isLeader() || mEnabledModes.isEmpty()) return;
        int idx = mEnabledModes.indexOf(mGameModeIdx);
        mGameModeIdx = mEnabledModes.get((idx + 1) % mEnabledModes.size());
        sendRoomProps();
        render();
    }

    private void toggleAutoFill() {
        if (!isLeader()) return;
        mAutoFill = !mAutoFill;
        sendRoomProps();
        render();
    }

    private void sendRoomProps() {
        String msg = "{\"type\":\"setRoomProps\",\"data\":{"
                + "\"roomUrl\":\"" + SurvApi.escapeJson(mRoomUrl) + "\","
                + "\"findingGame\":" + (mFindingGame ? "true" : "false") + ","
                + "\"region\":\"" + SurvApi.escapeJson(mRegion) + "\","
                + "\"autoFill\":" + (mAutoFill ? "true" : "false") + ","
                + "\"gameModeIdx\":" + mGameModeIdx + "}}";
        send(msg);
    }

    private void startGame() {
        if (!isLeader() || mFindingGame) return;
        mFindingGame = true;
        render();
        String msg = "{\"type\":\"playGame\",\"data\":{"
                + "\"version\":" + SurvBridge.getProtocolVersion() + ","
                + "\"region\":\"" + SurvApi.escapeJson(mRegion) + "\","
                + "\"zones\":" + mHost.zonesJson(mRegion) + "}}";
        send(msg);
    }

    private void copyInviteLink() {
        String link = TeamLinks.inviteUrl(mCode);
        if (link.isEmpty()) {
            toast("No invite link yet");
            return;
        }
        ClipboardManager clipboard = (ClipboardManager) mActivity.getSystemService(
                Context.CLIPBOARD_SERVICE);
        clipboard.setPrimaryClip(ClipData.newPlainText("team invite link", link));
        toast("Copied: " + link);
    }

    private void shareInviteLink() {
        String link = TeamLinks.inviteUrl(mCode);
        if (link.isEmpty()) {
            toast("No invite link yet");
            return;
        }
        Intent share = new Intent(Intent.ACTION_SEND);
        share.setType("text/plain");
        share.putExtra(Intent.EXTRA_TEXT, "Join my Survev squad! " + link);
        mActivity.startActivity(Intent.createChooser(share, "Share Invite Link"));
    }

    private void leave() {
        if (mOnClose != null) {
            mOnClose.onClose();
        } else {
            hide();
        }
        mHost.leaveTeam();
    }

    public void hide() {
        mUserClosing = true;
        closeSocket();
        mHandler.removeCallbacks(mKeepAlive);
        mOpen = false;
        mInGame = false;
        mJoined = false;
        mRoot.setVisibility(View.GONE);
        Log.i(TAG, "team hide");
    }

    private void closeSocket() {
        if (mWs != null) {
            mWs.close();
            mWs = null;
        }
        mHandler.removeCallbacks(mKeepAlive);
    }

    private void send(String json) {
        if (mWs != null && mWs.isRunning()) {
            mWs.send(json);
        }
    }

    // --- helpers ------------------------------------------------------------

    private boolean isLeader() {
        for (int i = 0; i < mPlayers.size(); i++) {
            TeamPlayer p = mPlayers.get(i);
            if (p.playerId == mLocalPlayerId) {
                return p.isLeader;
            }
        }
        return mCreate && mJoined;
    }

    private void setStatus(String text) {
        mStatusText.setText(text != null ? text : "");
    }

    private void setPropEnabled(Button b, boolean enabled) {
        b.setEnabled(enabled);
        b.setAlpha(enabled ? 1f : 0.55f);
    }

    private String getPlayerName() {
        String name = Json.peekString(SurvBridge.getConfigJson(), "playerName");
        if (name != null && !name.isEmpty()) {
            return name;
        }
        return "Player";
    }

    private void loadSiteInfo(String json) {
        mRegions.clear();
        mSiteTeamModes.clear();
        mEnabledModes.clear();
        try {
            if (json == null || json.isEmpty()) {
                throw new Exception("no site info");
            }
            JsonObject root = JsonParser.parseString(json).getAsJsonObject();
            if (root.has("pops") && root.get("pops").isJsonObject()) {
                JsonObject pops = root.getAsJsonObject("pops");
                for (String key : pops.keySet()) {
                    mRegions.add(key);
                }
            }
            if (root.has("modes") && root.get("modes").isJsonArray()) {
                JsonArray modes = root.getAsJsonArray("modes");
                for (int i = 0; i < modes.size(); i++) {
                    JsonObject mode = modes.get(i).getAsJsonObject();
                    mSiteTeamModes.add(intVal(mode, "teamMode", 1));
                    if (bool(mode, "enabled", false) && intVal(mode, "teamMode", 1) > 1) {
                        mEnabledModes.add(i);
                    }
                }
            }
        } catch (Exception e) {
            Log.w(TAG, "site info unavailable (" + e.getMessage() + "), using defaults");
        }

        // The native menu owns the HTTP fetch, and a deep link can arrive before
        // it completes. The default deployment mode table is Solo/Duo/Squad, so
        // keep the mode controls usable in that case.
        if (mSiteTeamModes.isEmpty()) {
            mSiteTeamModes.add(1);
            mSiteTeamModes.add(2);
            mSiteTeamModes.add(4);
        }
        if (mEnabledModes.isEmpty()) {
            mEnabledModes.add(1);
            mEnabledModes.add(2);
        }
    }

    private String defaultRegion() {
        String configured = Json.peekString(SurvBridge.getConfigJson(), "region");
        if (configured != null && mRegions.contains(configured)) {
            return configured;
        }
        if (!mRegions.isEmpty()) {
            return mRegions.get(0);
        }
        return configured != null && !configured.isEmpty() ? configured : "na";
    }

    private int defaultGameMode() {
        int configured = Json.peekInt(SurvBridge.getConfigJson(), "gameModeIdx");
        if (mEnabledModes.contains(configured)) {
            return configured;
        }
        if (!mEnabledModes.isEmpty()) {
            return mEnabledModes.get(0);
        }
        return 1;
    }

    private String regionName(String region) {
        String key = "index-" + region;
        String text = l10n(key, null);
        if (text != null && !text.isEmpty()) {
            return text;
        }
        return region.isEmpty() ? "Local" : region;
    }

    private String modeName(int modeIdx) {
        int teamMode = modeIdx >= 0 && modeIdx < mSiteTeamModes.size()
                ? mSiteTeamModes.get(modeIdx)
                : 4;
        if (teamMode <= 1) return "Solo";
        if (teamMode == 2) return "Duo";
        return "Squad";
    }

    private String errorL10nKey(String error) {
        if ("join_full".equals(error)) return "index-team-is-full";
        if ("join_not_found".equals(error)) return "index-failed-joining-team";
        if ("create_failed".equals(error)) return "index-failed-creating-team";
        if ("join_failed".equals(error)) return "index-failed-joining-team";
        if ("join_game_failed".equals(error)) return "index-failed-joining-game";
        if ("find_game_error".equals(error) || "find_game_full".equals(error)) {
            return "index-failed-finding-game";
        }
        if ("find_game_invalid_protocol".equals(error)) return "index-invalid-protocol";
        if ("find_game_invalid_captcha".equals(error)) return "index-invalid-captcha";
        if ("banned".equals(error)) return "index-ip-banned";
        if ("behind_proxy".equals(error)) return "index-behind-proxy";
        if ("rate_limited".equals(error)) return "index-rate-limited";
        if ("kicked".equals(error)) return "index-team-kicked";
        return "index-lost-connection";
    }

    private String errorText(String error) {
        String key = errorL10nKey(error);
        String text = l10n(key, null);
        return text != null && !text.isEmpty() ? text : "Team error: " + error;
    }

    private String l10n(String key, String fallback) {
        String text = Json.peekString(SurvBridge.getL10nJson(), key);
        return text != null && !text.isEmpty() ? text : fallback;
    }

    private void show() {
        mOpen = true;
        mRoot.setVisibility(View.VISIBLE);
        Log.i(TAG, "team show create=" + mCreate + " code=" + mCode);
    }

    private void toast(String text) {
        Toast.makeText(mActivity, text, Toast.LENGTH_SHORT).show();
    }

    private void addGap(LinearLayout parent, int height) {
        View gap = new View(mActivity);
        gap.setLayoutParams(new ViewGroup.LayoutParams(1, height));
        parent.addView(gap);
    }

    private static String str(JsonObject obj, String key) {
        JsonElement el = obj.get(key);
        return el != null && !el.isJsonNull() ? el.getAsString() : "";
    }

    private static boolean bool(JsonObject obj, String key, boolean fallback) {
        JsonElement el = obj.get(key);
        return el != null && !el.isJsonNull() ? el.getAsBoolean() : fallback;
    }

    private static int intVal(JsonObject obj, String key, int fallback) {
        JsonElement el = obj.get(key);
        return el != null && !el.isJsonNull() ? el.getAsInt() : fallback;
    }

    private int dp(int value) {
        return (int) (value * mActivity.getResources().getDisplayMetrics().density + 0.5f);
    }

    static class TeamPlayer {
        String name;
        int playerId;
        boolean isLeader;
        boolean inGame;
    }
}
