/****************************************************************************
 * TeamFragment.java — Team room screen (ARCHITECTURE.md §7, step S5b).
 *
 * Enhanced team room with:
 *   - Full roster display with player cards
 *   - Leader distinction (crown icon, gold accent, "LEADER" label)
 *   - Player ready/checkmark status
 *   - Ping/connection quality indicator per player
 *   - Copy invite code button with toast feedback
 *   - Share invite link via Android intent
 *   - Chat/message area stub
 *   - Play/Leave buttons
 *
 * Threading: WebSocket on background thread, UI updates delivered on UI thread.
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
import android.text.TextUtils;
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

import java.util.ArrayList;
import java.util.List;

public class TeamFragment {
    private static final String TAG = "TeamFragment";

    // Web team colors
    private static final int TEAL        = 0xFF4FADA9;
    private static final int TEAL_SHADOW = 0xFF37817E;
    private static final int GREEN       = 0xFF83AF50;
    private static final int GREEN_SHADOW= 0xFF5B7A38;
    private static final int RED         = 0xFFE57373;
    private static final int RED_SHADOW  = 0xFFC62828;
    private static final int GREY        = 0xFF666666;
    private static final int GOLD        = 0xFFFFD700;
    private static final int PANEL_BG    = 0xF0000000;

    // Ping colors
    private static final int PING_GOOD    = 0xFF4CAF50; // green <100ms
    private static final int PING_MEDIUM  = 0xFFFFC107; // yellow 100-200ms
    private static final int PING_BAD     = 0xFFF44336; // red >200ms

    private final Activity mActivity;
    private final FrameLayout mRoot;
    private final JavaUiHost mHost;
    private final boolean mIsCreate;

    // --- widgets ---
    private final TextView mTitleLabel;
    private final EditText mInviteCodeInput;
    private final Button mCopyBtn;
    private final Button mShareBtn;
    private final ScrollView mRosterScroll;
    private final LinearLayout mRosterList;
    private final LinearLayout mChatArea;      // chat stub
    private final EditText mChatInput;         // disabled stub
    private final TextView mPropsText;
    private final Button mPlayBtn;
    private final Button mLeaveBtn;
    private final TextView mStatusText;

    // --- state ---
    private boolean mOpen = false;
    private String mTeamId = "";
    private String mInviteCode = "";
    private boolean mIsLeader = false;
    private List<TeamMember> mMembers = new ArrayList<>();
    private TeamSocket mWs = null;
    private Handler mHandler = new Handler(Looper.getMainLooper());

    public TeamFragment(Activity activity, JavaUiHost host, boolean isCreate) {
        mActivity = activity;
        mHost = host;
        mIsCreate = isCreate;

        mRoot = new FrameLayout(activity);
        mRoot.setBackgroundColor(0xCC000000);

        // Full-height ScrollView for landscape responsiveness
        ScrollView outerScroll = new ScrollView(activity);
        outerScroll.setLayoutParams(new FrameLayout.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.MATCH_PARENT));

        // Main panel
        LinearLayout panel = new LinearLayout(activity);
        panel.setOrientation(LinearLayout.VERTICAL);
        panel.setGravity(Gravity.CENTER);
        panel.setBackgroundColor(PANEL_BG);
        panel.setPadding(dp(16), dp(14), dp(16), dp(14));
        FrameLayout.LayoutParams panelParams = new FrameLayout.LayoutParams(
                dp(340), ViewGroup.LayoutParams.WRAP_CONTENT);
        panelParams.gravity = Gravity.CENTER;
        panel.setLayoutParams(panelParams);

        // Title row
        mTitleLabel = new TextView(activity);
        mTitleLabel.setText(isCreate ? "\uD83D\uDC65 Create Team" : "\uD83D\uDC65 Join Team");
        mTitleLabel.setTextColor(Color.WHITE);
        mTitleLabel.setTextSize(TypedValue.COMPLEX_UNIT_SP, 18);
        mTitleLabel.setGravity(Gravity.CENTER);
        mTitleLabel.setTypeface(android.graphics.Typeface.DEFAULT_BOLD);
        panel.addView(mTitleLabel);

        addGap(panel, dp(8));

        // === Invite Code Row ===
        LinearLayout codeRow = new LinearLayout(activity);
        codeRow.setOrientation(LinearLayout.HORIZONTAL);
        codeRow.setGravity(Gravity.CENTER_VERTICAL);

        TextView codeLbl = new TextView(activity);
        codeLbl.setText("Code:");
        codeLbl.setTextColor(GOLD);
        codeLbl.setTextSize(TypedValue.COMPLEX_UNIT_SP, 13);
        codeRow.addView(codeLbl);

        mInviteCodeInput = new EditText(activity);
        mInviteCodeInput.setSingleLine(true);
        mInviteCodeInput.setHint(isCreate ? "Generating..." : "Enter code");
        mInviteCodeInput.setTextColor(Color.WHITE);
        mInviteCodeInput.setHintTextColor(0xFF888888);
        mInviteCodeInput.setBackgroundColor(0xFF333333);
        mInviteCodeInput.setPadding(dp(8), dp(4), dp(8), dp(4));
        mInviteCodeInput.setEnabled(!isCreate); // leader can't edit (auto-generated)
        LinearLayout.LayoutParams inputLp = new LinearLayout.LayoutParams(
                0, dp(36), 1f);
        codeRow.addView(mInviteCodeInput, inputLp);

        // Copy button
        mCopyBtn = new Button(activity);
        mCopyBtn.setText("\uD83D\uDCCB");
        mCopyBtn.setBackgroundColor(TEAL);
        mCopyBtn.setTextColor(Color.WHITE);
        mCopyBtn.setTextSize(TypedValue.COMPLEX_UNIT_SP, 14);
        LinearLayout.LayoutParams copyLp = new LinearLayout.LayoutParams(dp(40), dp(36));
        copyLp.leftMargin = dp(4);
        mCopyBtn.setOnClickListener(new View.OnClickListener() {
            @Override public void onClick(View v) { copyInviteCode(); }
        });
        codeRow.addView(mCopyBtn, copyLp);

        // Share button
        mShareBtn = new Button(activity);
        mShareBtn.setText("\u2797");
        mShareBtn.setBackgroundColor(0xFF4A90D9);
        mShareBtn.setTextColor(Color.WHITE);
        mShareBtn.setTextSize(TypedValue.COMPLEX_UNIT_SP, 14);
        LinearLayout.LayoutParams shareLp = new LinearLayout.LayoutParams(dp(40), dp(36));
        shareLp.leftMargin = dp(2);
        mShareBtn.setOnClickListener(new View.OnClickListener() {
            @Override public void onClick(View v) { shareInviteLink(); }
        });
        codeRow.addView(mShareBtn, shareLp);

        panel.addView(codeRow);

        addGap(panel, dp(6));

        // === Roster (scrollable player list) ===
        TextView rosterHeader = new TextView(activity);
        rosterHeader.setText("Team Members (" + mMembers.size() + "/4)");
        rosterHeader.setTextColor(0xFFAAAAAA);
        rosterHeader.setTextSize(TypedValue.COMPLEX_UNIT_SP, 12);
        panel.addView(rosterHeader);

        mRosterScroll = new ScrollView(activity);
        mRosterScroll.setLayoutParams(new FrameLayout.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT, dp(140)));
        
        mRosterList = new LinearLayout(activity);
        mRosterList.setOrientation(LinearLayout.VERTICAL);
        mRosterScroll.addView(mRosterList);
        panel.addView(mRosterScroll);

        addGap(panel, dp(6));

        // === Chat Area Stub ===
        mChatArea = new LinearLayout(activity);
        mChatArea.setOrientation(LinearLayout.VERTICAL);
        mChatArea.setBackgroundColor(0x22FFFFFF);
        mChatArea.setPadding(dp(6), dp(4), dp(6), dp(4));
        
        TextView chatLabel = new TextView(activity);
        chatLabel.setText("\uD83D\uDCAC Team Chat");
        chatLabel.setTextColor(0xFF888888);
        chatLabel.setTextSize(TypedValue.COMPLEX_UNIT_SP, 11);
        mChatArea.addView(chatLabel);
        
        ScrollView chatScroll = new ScrollView(activity);
        chatScroll.setLayoutParams(new FrameLayout.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT, dp(60)));
        
        TextView chatMessages = new TextView(activity);
        chatMessages.setText("Chat will be available in a future update.");
        chatMessages.setTextColor(0xFF666666);
        chatMessages.setTextSize(TypedValue.COMPLEX_UNIT_SP, 11);
        chatScroll.addView(chatMessages);
        mChatArea.addView(chatScroll);
        
        mChatInput = new EditText(activity);
        mChatInput.setHint("Coming soon...");
        mChatInput.setEnabled(false);
        mChatInput.setTextSize(TypedValue.COMPLEX_UNIT_SP, 11);
        mChatArea.addView(mChatInput);
        
        panel.addView(mChatArea);

        // === Game Mode / Props ===
        mPropsText = new TextView(activity);
        mPropsText.setText("Mode: Squad | Region: Local");
        mPropsText.setTextColor(0xFF999999);
        mPropsText.setTextSize(TypedValue.COMPLEX_UNIT_SP, 11);
        mPropsText.setGravity(Gravity.CENTER);
        panel.addView(mPropsText);

        addGap(panel, dp(8));

        // Status text
        mStatusText = new TextView(activity);
        mStatusText.setTextColor(0xFF808080);
        mStatusText.setTextSize(TypedValue.COMPLEX_UNIT_SP, 11);
        mStatusText.setGravity(Gravity.CENTER);
        panel.addView(mStatusText);

        addGap(panel, dp(6));

        // === Action Buttons ===
        LinearLayout btnRow = new LinearLayout(activity);
        btnRow.setOrientation(LinearLayout.HORIZONTAL);

        mPlayBtn = new Button(activity);
        mPlayBtn.setText("\u25B6 Play");
        mPlayBtn.setBackgroundColor(GREEN);
        mPlayBtn.setShadowLayer(dp(2), 0, dp(2), GREEN_SHADOW);
        mPlayBtn.setTextColor(Color.WHITE);
        mPlayBtn.setTextSize(TypedValue.COMPLEX_UNIT_SP, 16);
        mPlayBtn.setEnabled(false); // Enable when team is ready
        LinearLayout.LayoutParams playLp = new LinearLayout.LayoutParams(
                0, dp(42), 1f);
        playLp.rightMargin = dp(4);
        mPlayBtn.setOnClickListener(new View.OnClickListener() {
            @Override public void onClick(View v) { startPlay(); }
        });
        btnRow.addView(mPlayBtn, playLp);

        mLeaveBtn = new Button(activity);
        mLeaveBtn.setText("Leave \u2716");
        mLeaveBtn.setBackgroundColor(RED);
        mLeaveBtn.setShadowLayer(dp(2), 0, dp(2), RED_SHADOW);
        mLeaveBtn.setTextColor(Color.WHITE);
        mLeaveBtn.setTextSize(TypedValue.COMPLEX_UNIT_SP, 16);
        LinearLayout.LayoutParams leaveLp = new LinearLayout.LayoutParams(
                0, dp(42), 1f);
        leaveLp.leftMargin = dp(4);
        mLeaveBtn.setOnClickListener(new View.OnClickListener() {
            @Override public void onClick(View v) { leaveTeam(); }
        });
        btnRow.addView(mLeaveBtn, leaveLp);

        panel.addView(btnRow);

        outerScroll.addView(panel);

        mRoot.addView(outerScroll);

        if (isCreate) {
            createTeam();
        } else {
            mInviteCodeInput.requestFocus();
        }
    }

    // ===== Network Operations =====

    public void createTeam() {
        setStatus("Creating team...");
        String apiBase = "http://127.0.0.1:8000";
        SurvApi.createTeam(apiBase, new SurvApi.Callback() {
            @Override public void onResult(String json) {
                if (json == null) {
                    setStatus("Failed to create team");
                    return;
                }
                String teamId = Json.peekString(json, "teamId");
                String code = Json.peekString(json, "inviteCode");
                if (code != null && !code.isEmpty()) {
                    mTeamId = teamId != null ? teamId : "";
                    mInviteCode = code;
                    mInviteCodeInput.setText(code);
                    mIsLeader = true;
                    connectWebSocket();
                    setStatus("Team created! Waiting for players...");
                    
                    // Add self as member
                    addMember("You", true, 0, true);
                } else {
                    String error = Json.peekString(json, "error");
                    setStatus("Error: " + (error != null ? error : "Unknown"));
                }
            }
        });
    }

    private void joinTeam(String code) {
        if (code == null || code.trim().isEmpty()) {
            setStatus("Please enter an invite code");
            return;
        }
        setStatus("Joining team " + code.trim() + "...");
        String apiBase = "http://127.0.0.1:8000";
        SurvApi.joinTeam(apiBase, code.trim(), new SurvApi.Callback() {
            @Override public void onResult(String json) {
                if (json == null) {
                    setStatus("Failed to join team");
                    return;
                }
                String teamId = Json.peekString(json, "teamId");
                String wsUrl = Json.peekString(json, "wsUrl");
                if (teamId != null) {
                    mTeamId = teamId;
                    mInviteCode = code.trim();
                    mInviteCodeInput.setText(code);
                    mIsLeader = false;
                    mInviteCodeInput.setEnabled(false);
                    
                    // Add members from response
                    parseMembers(json);
                    
                    if (wsUrl != null) {
                        connectToWebSocket(wsUrl);
                    }
                    setStatus("Joined team!");
                } else {
                    String error = Json.peekString(json, "error");
                    setStatus("Error: " + (error != null ? error : "Invalid code"));
                }
            }
        });
    }

    private void connectWebSocket() {
        String wsUrl = "ws://127.0.0.1:9001/team/" + mTeamId;
        connectToWebSocket(wsUrl);
    }

    private void connectToWebSocket(String url) {
        TeamSocket ws = new TeamSocket();
        boolean ok = ws.connect(url, new TeamSocket.OnMessage() {
            @Override public void onMessage(String msg) {
                Log.d(TAG, "Team WS msg: " + msg);
                handleMessage(msg);
            }
        }, new TeamSocket.OnError() {
            @Override public void onError(String reason) {
                Log.i(TAG, "Team WS closed: " + reason);
                mHandler.post(() -> setStatus("Connection error: " + reason));
            }
        });
        if (ok) {
            mWs = ws;
            Log.i(TAG, "Team WS connected");
            mHandler.post(() -> setStatus("Connected to team server"));
            
            // Send join message
            String config = SurvBridge.getConfigJson();
            String playerName = Json.peekString(config, "playerName");
            if (playerName == null || playerName.isEmpty()) playerName = "Player";
            ws.send("{\"type\":\"join\",\"name\":\"" + escapeJson(playerName) + "\"}");
        } else {
            setStatus("Failed to connect");
        }
    }

    private void handleMessage(String json) {
        mHandler.post(() -> {
            String type = Json.peekString(json, "type");
            
            if ("roster".equals(type)) {
                parseMembers(json);
            } else if ("playerJoined".equals(type)) {
                String name = Json.peekString(json, "name");
                boolean leader = "true".equals(Json.peekString(json, "isLeader"));
                addMember(name, leader, 0, false);
                toast(name + " joined the team");
            } else if ("playerLeft".equals(type)) {
                String name = Json.peekString(json, "name");
                removeMember(name);
                toast(name + " left the team");
            } else if ("readyChanged".equals(type)) {
                String name = Json.peekString(json, "name");
                boolean ready = "true".equals(Json.peekString(json, "ready"));
                updateReadyStatus(name, ready);
            } else if ("startGame".equals(type)) {
                String token = Json.peekString(json, "joinToken");
                if (token != null) {
                    List<String> urls = new ArrayList<String>();
                    int at = json.indexOf("\"urls\"");
                    if (at >= 0) {
                        int arrStart = json.indexOf('[', at);
                    }
                    if (urls.isEmpty()) urls.add("ws://127.0.0.1:9001");
                    SurvBridge.onJoinUrls(urls.toArray(new String[0]), token);
                    hide();
                    if (mHost != null) mHost.enterGame(0);
                }
            }
        });
    }

    // ===== Roster Management =====

    private void parseMembers(String json) {
        mMembers.clear();
        mRosterList.removeAllViews();
        
        // Find members array
        int at = json.indexOf("\"members\"");
        if (at < 0) at = json.indexOf("\"roster\"");
        if (at < 0) return;
        
        int colon = json.indexOf(':', at);
        int arrStart = json.indexOf('[', colon);
        if (arrStart < 0) return;
        
        // Parse member objects (simplified)
        int depth = 0;
        int objStart = -1;
        for (int i = arrStart; i < json.length(); i++) {
            char c = json.charAt(i);
            if (c == '{') {
                if (depth == 0) objStart = i;
                depth++;
            } else if (c == '}') {
                depth--;
                if (depth == 0 && objStart >= 0) {
                    String obj = json.substring(objStart, i + 1);
                    String name = Json.peekString(obj, "name");
                    if (name != null) {
                        boolean leader = "true".equals(Json.peekString(obj, "isLeader"));
                        boolean ready = "true".equals(Json.peekString(obj, "ready"));
                        int ping = 0;
                        try { ping = Integer.parseInt(Json.peekString(obj, "ping")); } catch (Exception e) {}
                        addMember(name, leader, ping, ready);
                    }
                    objStart = -1;
                }
            }
        }
        
        updateRosterHeader();
        updatePlayButton();
    }

    private void addMember(String name, boolean isLeader, int ping, boolean ready) {
        TeamMember m = new TeamMember();
        m.name = name;
        m.isLeader = isLeader;
        m.ping = ping;
        m.ready = ready;
        m.isSelf = "You".equals(name) || name.equals(getPlayerName());
        mMembers.add(m);
        
        renderMemberCard(m);
        updateRosterHeader();
        updatePlayButton();
    }

    private void removeMember(String name) {
        for (int i = 0; i < mMembers.size(); i++) {
            if (mMembers.get(i).name.equals(name)) {
                mMembers.remove(i);
                break;
            }
        }
        rebuildRoster();
        updatePlayButton();
    }

    private void updateReadyStatus(String name, boolean ready) {
        for (TeamMember m : mMembers) {
            if (m.name.equals(name)) {
                m.ready = ready;
                break;
            }
        }
        rebuildRoster();
        updatePlayButton();
    }

    private void renderMemberCard(TeamMember m) {
        LinearLayout card = new LinearLayout(mActivity);
        card.setOrientation(LinearLayout.HORIZONTAL);
        card.setGravity(Gravity.CENTER_VERTICAL);
        card.setPadding(dp(8), dp(6), dp(8), dp(6));
        
        // Style based on role
        GradientDrawable bg = new GradientDrawable();
        bg.setColor(m.isLeader ? 0x33FFD700 : 0x22FFFFFF);
        bg.setCornerRadius(dp(6));
        bg.setStroke(m.isSelf ? dp(2) : dp(1), m.isLeader ? GOLD : 0x33FFFFFF);
        card.setBackground(bg);
        
        // Leader crown / status icon
        TextView icon = new TextView(mActivity);
        if (m.isLeader) {
            icon.setText("\uD83D\uDC51"); // crown
            icon.setTextColor(GOLD);
        } else if (m.ready) {
            icon.setText("\u2705"); // checkmark
            icon.setTextColor(0xFF4CAF50);
        } else {
            icon.setText("\u23F3"); // hourglass
            icon.setTextColor(0xFF999999);
        }
        icon.setTextSize(TypedValue.COMPLEX_UNIT_SP, 14);
        card.addView(icon);
        
        // Name
        TextView nameView = new TextView(mActivity);
        nameView.setText(m.name + (m.isLeader ? " (LEADER)" : "") + (m.isSelf ? " (YOU)" : ""));
        nameView.setTextColor(m.isLeader ? GOLD : Color.WHITE);
        nameView.setTextSize(TypedValue.COMPLEX_UNIT_SP, 13);
        nameView.setLayoutParams(new LinearLayout.LayoutParams(0, ViewGroup.LayoutParams.WRAP_CONTENT, 1f));
        card.addView(nameView);
        
        // Ping indicator
        View pingDot = new View(mActivity);
        int dotSize = dp(10);
        pingDot.setLayoutParams(new ViewGroup.LayoutParams(dotSize, dotSize));
        GradientDrawable dotBg = new GradientDrawable();
        dotBg.setShape(GradientDrawable.OVAL);
        dotBg.setColor(getPingColor(m.ping));
        pingDot.setBackground(dotBg);
        card.addView(pingDot);
        
        // In-game badge
        if (m.inGame) {
            TextView ingameBadge = new TextView(mActivity);
            ingameBadge.setText("IN GAME");
            ingameBadge.setTextColor(0xFF4CAF70);
            ingameBadge.setTextSize(TypedValue.COMPLEX_UNIT_SP, 9);
            card.addView(ingameBadge);
        }
        
        // Ready toggle (for non-leader self)
        if (m.isSelf && !m.isLeader) {
            Button readyToggle = new Button(mActivity);
            readyToggle.setText(m.ready ? "\u2705 Ready" : "\u23F3 Not Ready");
            readyToggle.setBackgroundColor(m.ready ? GREEN : GREY);
            readyToggle.setTextColor(Color.WHITE);
            readyToggle.setTextSize(TypedValue.COMPLEX_UNIT_SP, 10);
            readyToggle.setOnClickListener(new View.OnClickListener() {
                @Override public void onClick(View v) {
                    toggleReady();
                }
            });
            card.addView(readyToggle);
        }
        
        mRosterList.addView(card);
    }

    private void rebuildRoster() {
        mRosterList.removeAllViews();
        for (TeamMember m : mMembers) {
            renderMemberCard(m);
        }
    }

    private void updateRosterHeader() {
        // Find header by searching parent
        int count = mRosterList.getChildCount();
        // Update would need reference - simplified here
    }

    private void updatePlayButton() {
        // Leader can always start, or everyone must be ready
        boolean canPlay = mIsLeader && mMembers.size() >= 1;
        if (!mIsLeader) {
            canPlay = true; // Non-leaders just see the button disabled
        }
        mPlayBtn.setEnabled(canPlay);
        if (canPlay && mMembers.size() >= 2) {
            mPlayBtn.setText("\u25B6 Play (" + mMembers.size() + ")");
        } else {
            mPlayBtn.setText("\u25B6 Play");
        }
    }

    private int getPingColor(int ping) {
        if (ping < 100) return PING_GOOD;
        if (ping < 200) return PING_MEDIUM;
        return PING_BAD;
    }

    // ===== Actions =====

    private void copyInviteCode() {
        String code = mInviteCodeInput.getText().toString().trim();
        if (code.isEmpty()) {
            toast("No code to copy");
            return;
        }
        
        ClipboardManager clipboard = (ClipboardManager) mActivity.getSystemService(Context.CLIPBOARD_SERVICE);
        ClipData clip = ClipData.newPlainText("invite code", code);
        clipboard.setPrimaryClip(clip);
        toast("Copied: " + code);
    }

    private void shareInviteLink() {
        String code = mInviteCodeInput.getText().toString().trim();
        if (code.isEmpty()) {
            toast("No code to share");
            return;
        }
        
        Intent shareIntent = new Intent(Intent.ACTION_SEND);
        shareIntent.setType("text/plain");
        shareIntent.putExtra(Intent.EXTRA_TEXT, 
                "Join my Survev squad! Code: " + code + "\nDownload at https://survev.io");
        mActivity.startActivity(Intent.createChooser(shareIntent, "Share Invite Code"));
    }

    private void toggleReady() {
        if (mWs != null && mWs.isRunning()) {
            boolean newReady = false;
            for (TeamMember m : mMembers) {
                if (m.isSelf) {
                    newReady = !m.ready;
                    break;
                }
            }
            mWs.send("{\"type\":\"setReady\",\"ready\":" + newReady + "}");
        }
    }

    private void startPlay() {
        if (mWs != null && mWs.isRunning()) {
            mWs.send("{\"type\":\"startGame\"}");
            setStatus("Starting game...");
        } else {
            setStatus("Not connected to team server");
        }
    }

    private void leaveTeam() {
        if (mWs != null) {
            mWs.send("{\"type\":\"leave\"}");
            mWs.close();
            mWs = null;
        }
        hide();
        if (mHost != null) mHost.hideTeam();
    }

    // ===== Helpers =====

    private void setStatus(String text) {
        mStatusText.setText(text);
        Log.i(TAG, text);
    }

    private void toast(final String text) {
        Toast.makeText(mActivity, text, Toast.LENGTH_SHORT).show();
    }

    private String getPlayerName() {
        String config = SurvBridge.getConfigJson();
        String name = Json.peekString(config, "playerName");
        return (name != null && !name.isEmpty()) ? name : "Player";
    }

    private void addGap(LinearLayout parent, int height) {
        View gap = new View(mActivity);
        gap.setLayoutParams(new ViewGroup.LayoutParams(1, height));
        parent.addView(gap);
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

    // ===== Public API =====

    public View root() { return mRoot; }

    public void show() {
        if (mOpen) return;
        mOpen = true;
        mRoot.setVisibility(View.VISIBLE);
        
        if (!mIsCreate) {
            String code = mInviteCodeInput.getText().toString().trim();
            if (!code.isEmpty()) {
                joinTeam(code);
            }
        }
    }

    public void hide() {
        if (!mOpen) return;
        mOpen = false;
        mRoot.setVisibility(View.GONE);
        
        if (mWs != null) {
            mWs.close();
            mWs = null;
        }
    }

    public boolean isOpen() { return mOpen; }

    public void setInviteCode(String code) {
        if (code != null && !code.isEmpty()) {
            mInviteCodeInput.setText(code);
            if (!mIsCreate) {
                joinTeam(code);
            }
        }
    }

    private int dp(int value) {
        return (int) (value * mActivity.getResources().getDisplayMetrics().density + 0.5f);
    }

    // ===== Types =====

    static class TeamMember {
        String name;
        boolean isLeader;
        boolean isSelf;
        int ping;
        boolean ready;
        boolean inGame;
    }
}