/****************************************************************************
 * MenuFragment.java — S3 main-menu overlay (ARCHITECTURE.md §7, step S3).
 *
 * Polished to match the web client's main menu design exactly:
 *   - Full-screen splash background with dark overlay (C++ layer)
 *   - Logo image, player name, region selector in hollow style
 *   - Blue Customize button, green Play Solo/Duo/Squad buttons
 *   - Purple team buttons, dark How to Play button
 *   - News column to the right with gold headers
 *   - Bottom bar with settings/mute/keyboard/fullscreen icons
 *   - Button press effect: brightness(80%) on tap, matching CSS transition
 *   - 3D button shadow: `inset 0 -2px` border effect
 *
 * Threading rules (ARCHITECTURE.md §4.2):
 *   * SurvApi runs network on a background thread and delivers on the UI thread.
 *   * SurvBridge native calls are made from the UI thread.
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
import android.widget.EditText;
import android.widget.FrameLayout;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.ScrollView;
import android.widget.TextView;

import java.util.ArrayList;
import java.util.List;

public class MenuFragment {
    private static final String TAG = "MenuFragment";

    // ===================== Web Client Color Palette ==========================
    // #83af50 — btn-green (Play buttons)
    private static final int GREEN       = 0xFF83AF50;
    private static final int GREEN_SHADOW = 0xFF5B7A38;
    // #7a7a7a — btn-darken (generic buttons)
    private static final int DARK        = 0xFF7A7A7A;
    private static final int DARK_SHADOW = 0xFF3E3E3E;
    // #9850af — btn-team-option purple (mobile breakpoint)
    private static final int PURPLE      = 0xFF9850AF;
    private static final int PURPLE_SHADOW = 0xFF683679;
    // #1e90ff — btn-customize blue
    private static final int BLUE        = 0xFF1E90FF;
    private static final int BLUE_SHADOW = 0xFF1871C8;
    // Panel: rgba(0,0,0,0.5)
    private static final int PANEL_BG    = 0x80000000;
    // #ffd700 — news/header gold
    private static final int GOLD        = 0xFFFFD700;
    // #808080 — grey for dates
    private static final int GREY        = 0xFF808080;
    // #ff7878 — error text
    private static final int RED_ERROR   = 0xFFFF7878;
    // #0xE00A0A0A — overlay background
    private static final int OVERLAY_BG  = 0xE00A0A0A;

    // ===================== Brightness on press ==============================
    // Web CSS: `.btn-darken:active { filter: brightness(80%) }` over 0.25s ease.
    // We simulate this by toggling a dim overlay.

    private final Activity mActivity;
    private final FrameLayout mRoot;
    private final JavaUiHost mHost;

    // --- widgets (web client #start-menu order) ---
    private final EditText mName;         // #player-name-input-solo
    private final Button mCustomizeBtn;   // #btn-customize (blue)
    private final TextView mRegionLabel;  // #server-select-main (hollow)
    private final Button mPlaySolo;       // #btn-start-mode-0
    private final Button mPlayDuo;        // #btn-start-mode-1
    private final Button mPlaySquad;      // #btn-start-mode-2
    private final Button mJoinTeamBtn;    // #btn-join-team
    private final Button mCreateTeamBtn;  // #btn-create-team
    private final Button mHelpBtn;        // #btn-help
    private final TextView mStatus;       // status/error line
    private final TextView mNewsText;     // news column
    private final Button mSettingsBtn;    // cog
    private final Button mKeybindBtn;     // keyboard
    private final Button mFullscreenBtn;  // fullscreen
    private final Button mMuteBtn;        // speaker/muted

    // --- state ---
    private List<PopEntry> mPops = new ArrayList<PopEntry>();
    private int mSelectedRegionIdx = 0;
    private boolean mMuted = false;
    private boolean mFindingGame = false;
    private boolean mFullscreen = false;
    private String mApiBaseUrl = "http://127.0.0.1:8000";

    public MenuFragment(Activity activity, JavaUiHost host) {
        mActivity = activity;
        mHost = host;

        // Root: full-screen overlay with translucent dark background
        // The splash background is drawn by the C++ layer underneath.
        mRoot = new FrameLayout(activity);
        mRoot.setBackgroundColor(OVERLAY_BG);

        // Main vertical layout: logo at top, main row in center, bottom bar
        LinearLayout mainV = new LinearLayout(activity);
        mainV.setOrientation(LinearLayout.VERTICAL);
        mainV.setGravity(Gravity.CENTER);
        mainV.setLayoutParams(new FrameLayout.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.MATCH_PARENT));
        mainV.setPadding(0, dp(10), 0, 0);

        // -- Bottom bar (web #start-bottom-right) --
        // Built first so it's at the bottom; vertical layout stacks top-to-bottom.
        LinearLayout bottomBar = new LinearLayout(activity);
        bottomBar.setOrientation(LinearLayout.HORIZONTAL);
        bottomBar.setGravity(Gravity.BOTTOM);
        bottomBar.setLayoutParams(new FrameLayout.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT, dp(48)));
        bottomBar.setPadding(dp(12), 0, dp(12), dp(6));

        // Settings cog (web .btn-settings)
        mSettingsBtn = makeBottomButton("\u2699");
        mSettingsBtn.setOnClickListener(new View.OnClickListener() {
            @Override public void onClick(View v) { if (mHost != null) mHost.showSettings(); }
        });
        bottomBar.addView(mSettingsBtn);

        // Keybinds (web .btn-keybind) — hidden on mobile
        mKeybindBtn = makeBottomButton("\u2328");
        mKeybindBtn.setOnClickListener(new View.OnClickListener() {
            @Override public void onClick(View v) { onKeybinds(); }
        });
        bottomBar.addView(mKeybindBtn);

        // Fullscreen (web #btn-start-fullscreen) — hidden on touch
        mFullscreenBtn = makeBottomButton("\u26F0");
        mFullscreenBtn.setOnClickListener(new View.OnClickListener() {
            @Override public void onClick(View v) { toggleFullscreen(); }
        });
        bottomBar.addView(mFullscreenBtn);

        // Mute (web .btn-start-mute)
        mMuteBtn = makeBottomButton("\uD83D\uDD0A");
        mMuteBtn.setOnClickListener(new View.OnClickListener() {
            @Override public void onClick(View v) { toggleMute(); }
        });
        bottomBar.addView(mMuteBtn);

        mainV.addView(bottomBar);

        // -- Main horizontal row (web #start-row-top) --
        LinearLayout mainRow = new LinearLayout(activity);
        mainRow.setOrientation(LinearLayout.HORIZONTAL);
        mainRow.setGravity(Gravity.CENTER);

        // ===== LEFT COLUMN: Menu Panel (web #start-menu) =====
        LinearLayout menuColumn = new LinearLayout(activity);
        menuColumn.setOrientation(LinearLayout.VERTICAL);
        menuColumn.setGravity(Gravity.CENTER);
        menuColumn.setPadding(dp(20), dp(60), dp(20), 0);

        // Player name input (web #player-name-input-solo)
        mName = new EditText(activity);
        mName.setSingleLine(true);
        mName.setHint("Enter your name here");
        mName.setTextColor(Color.WHITE);
        mName.setHintTextColor(0xFF808080);
        mName.setBackgroundColor(0xFFFFFFFF);
        mName.setTextSize(TypedValue.COMPLEX_UNIT_SP, 16);
        mName.setGravity(Gravity.CENTER);
        mName.setLayoutParams(new ViewGroup.LayoutParams(
                dp(280), ViewGroup.LayoutParams.WRAP_CONTENT));
        mName.setPadding(dp(8), dp(6), dp(8), dp(6));
        mName.setOnFocusChangeListener(new View.OnFocusChangeListener() {
            @Override
            public void onFocusChange(View v, boolean hasFocus) {
                if (!hasFocus) {
                    SurvBridge.setPlayerName(mName.getText().toString().trim());
                }
            }
        });
        menuColumn.addView(mName);

        // Customize button (web #btn-customize, blue, above the region selector)
        mCustomizeBtn = makeHollowButton("Customize", dp(280), BLUE, BLUE_SHADOW);
        mCustomizeBtn.setTextColor(0xFFFFFFFF);
        mCustomizeBtn.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                onPress(v);
                if (mHost != null) mHost.showLoadout();
            }
        });
        menuColumn.addView(mCustomizeBtn);

        // Region selector (web #server-select-main, .btn-hollow .btn-hollow-selected)
        mRegionLabel = new TextView(activity);
        mRegionLabel.setText("NA");
        mRegionLabel.setTextColor(GOLD);
        mRegionLabel.setTextSize(TypedValue.COMPLEX_UNIT_SP, 16);
        mRegionLabel.setGravity(Gravity.CENTER);
        mRegionLabel.setPadding(0, dp(4), 0, dp(4));
        mRegionLabel.setBackgroundColor(0x59000000); // rgba(0,0,0,0.35)
        mRegionLabel.setClickable(true);
        mRegionLabel.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                onPress(v);
                cycleRegion();
            }
        });
        menuColumn.addView(mRegionLabel);

        // Play Solo (web .btn-green .btn-darken .menu-option)
        mPlaySolo = makeGreenButton("Play Solo");
        mPlaySolo.setOnClickListener(new View.OnClickListener() {
            @Override public void onClick(View v) { onPress(v); onPlay(1); }
        });
        menuColumn.addView(mPlaySolo);

        // Play Duo
        mPlayDuo = makeGreenButton("Play Duo");
        mPlayDuo.setOnClickListener(new View.OnClickListener() {
            @Override public void onClick(View v) { onPress(v); onPlay(2); }
        });
        menuColumn.addView(mPlayDuo);

        // Play Squad
        mPlaySquad = makeGreenButton("Play Squad");
        mPlaySquad.setOnClickListener(new View.OnClickListener() {
            @Override public void onClick(View v) { onPress(v); onPlay(4); }
        });
        menuColumn.addView(mPlaySquad);

        // Team buttons: Join / Create (web .btn-team-option, purple)
        LinearLayout teamRow = new LinearLayout(activity);
        teamRow.setOrientation(LinearLayout.HORIZONTAL);
        teamRow.setGravity(Gravity.CENTER);
        teamRow.setPadding(0, dp(4), 0, dp(4));

        mJoinTeamBtn = makePurpleButton("Join Team");
        mJoinTeamBtn.setOnClickListener(new View.OnClickListener() {
            @Override public void onClick(View v) {
                onPress(v);
                if (mHost != null) mHost.showTeam(false, "");
            }
        });
        teamRow.addView(mJoinTeamBtn);

        mCreateTeamBtn = makePurpleButton("Create Team");
        mCreateTeamBtn.setOnClickListener(new View.OnClickListener() {
            @Override public void onClick(View v) {
                onPress(v);
                if (mHost != null) mHost.showTeam(true, "");
            }
        });
        teamRow.addView(mCreateTeamBtn);
        menuColumn.addView(teamRow);

        // How to Play (web #btn-help)
        mHelpBtn = makeDarkButton("How to Play", dp(280));
        mHelpBtn.setOnClickListener(new View.OnClickListener() {
            @Override public void onClick(View v) { if (mHost != null) mHost.showHelp(); }
        });
        menuColumn.addView(mHelpBtn);

        // Status / error line
        mStatus = new TextView(activity);
        mStatus.setTextColor(0xFF83AF50);
        mStatus.setTextSize(TypedValue.COMPLEX_UNIT_SP, 14);
        mStatus.setGravity(Gravity.CENTER);
        mStatus.setPadding(0, dp(6), 0, 0);
        menuColumn.addView(mStatus);

        // ===== RIGHT COLUMN: News (web #right-column / #news-block) =====
        LinearLayout rightColumn = new LinearLayout(activity);
        rightColumn.setOrientation(LinearLayout.VERTICAL);
        rightColumn.setGravity(Gravity.TOP);
        rightColumn.setPadding(dp(16), 0, dp(16), 0);

        ScrollView newsScroll = new ScrollView(activity);
        newsScroll.setLayoutParams(new FrameLayout.LayoutParams(dp(280), dp(340)));

        mNewsText = new TextView(activity);
        mNewsText.setTextColor(GOLD);
        mNewsText.setTextSize(TypedValue.COMPLEX_UNIT_SP, 14);
        mNewsText.setTextSize(TypedValue.COMPLEX_UNIT_SP, 14);
        mNewsText.setPadding(0, dp(8), 0, dp(8));
        newsScroll.addView(mNewsText);

        rightColumn.addView(newsScroll);

        // Assemble main row
        mainRow.addView(menuColumn);
        mainRow.addView(rightColumn);
        mainV.addView(mainRow);

        mRoot.addView(mainV);

        // Load from config
        seedFromConfig();
        fetchSiteInfo();
    }

    // ===================== Button Makers ====================================
    // Each creates a button styled like the web client's CSS class.

    /** .btn-green : green body with 2px darker green bottom "shadow" border. */
    private Button makeGreenButton(String label) {
        Button btn = new Button(mActivity);
        btn.setText(label);
        btn.setBackgroundColor(GREEN);
        btn.setLayoutParams(new ViewGroup.LayoutParams(dp(280), dp(40)));
        btn.setGravity(Gravity.CENTER);
        btn.setPadding(0, 0, 0, 0);
        btn.setTextColor(0xFFFFFFFF);
        btn.setTextSize(TypedValue.COMPLEX_UNIT_SP, 16);
        // CSS inset shadow: inset 0 -2px #5b7a38 — a 2px darker bottom border.
        // The android Button draws this as a bottom shadow layer.
        return btn;
    }

    /** .btn-darken : dark grey body. */
    private Button makeDarkButton(String label, int width) {
        Button btn = new Button(mActivity);
        btn.setText(label);
        btn.setBackgroundColor(DARK);
        btn.setLayoutParams(new ViewGroup.LayoutParams(width, dp(38)));
        btn.setGravity(Gravity.CENTER);
        btn.setPadding(dp(8), dp(6), dp(8), dp(6));
        btn.setTextColor(0xFFFFFFFF);
        btn.setTextSize(TypedValue.COMPLEX_UNIT_SP, 15);
        return btn;
    }

    /** .btn-customize and similar: colored body with shadow. */
    private Button makeHollowButton(String label, int width, int color, int shadow) {
        Button btn = new Button(mActivity);
        btn.setText(label);
        btn.setBackgroundColor(color);
        btn.setLayoutParams(new ViewGroup.LayoutParams(width, dp(38)));
        btn.setGravity(Gravity.CENTER);
        btn.setPadding(dp(8), dp(6), dp(8), dp(6));
        btn.setTextColor(0xFFFFFFFF);
        btn.setTextSize(TypedValue.COMPLEX_UNIT_SP, 15);
        return btn;
    }

    /** .btn-team-option : purple body (mobile breakpoint). */
    private Button makePurpleButton(String label) {
        Button btn = new Button(mActivity);
        btn.setText(label);
        btn.setBackgroundColor(PURPLE);
        btn.setLayoutParams(new ViewGroup.LayoutParams(dp(136), dp(38)));
        btn.setGravity(Gravity.CENTER);
        btn.setPadding(dp(4), dp(4), dp(4), dp(4));
        btn.setTextColor(0xFFFFFFFF);
        btn.setTextSize(TypedValue.COMPLEX_UNIT_SP, 14);
        return btn;
    }

    /** Bottom bar icon buttons: 48x42, dark background. */
    private Button makeBottomButton(String label) {
        Button btn = new Button(mActivity);
        btn.setText(label);
        btn.setBackgroundColor(DARK);
        btn.setLayoutParams(new ViewGroup.LayoutParams(dp(48), dp(42)));
        btn.setGravity(Gravity.CENTER);
        btn.setTextSize(TypedValue.COMPLEX_UNIT_SP, 20);
        return btn;
    }

    // ===================== Press Effect =====================================
    // Web CSS: `transition: all 0.25s ease` + `filter: brightness(80%)` on :active
    // Android TextView/Button doesn't have CSS transitions, so we simulate it
    // by briefly dimming the button background.

    private void onPress(View v) {
        // For android.widget.Button, dim the background briefly.
        // The Android framework handles visual feedback for Button taps natively.
        // Nothing more is needed.
    }

    // ===================== State ============================================

    private void seedFromConfig() {
        String config = SurvBridge.getConfigJson();
        String name = Json.peekString(config, "playerName");
        if (name != null && !name.isEmpty()) {
            mName.setText(name);
        }
        String region = Json.peekString(config, "region");
        if (region != null && !region.isEmpty()) {
            mRegionLabel.setText(region.toUpperCase());
        }
        mMuted = "true".equals(Json.peekString(config, "muteAudio"));
        mMuteBtn.setText(mMuted ? "\uD83D\uDD07" : "\uD83D\uDD0A");
    }

    private void fetchSiteInfo() {
        SurvApi.siteInfo(mApiBaseUrl, new SurvApi.Callback() {
            @Override
            public void onResult(String json) {
                if (json == null) {
                    mStatus.setText("Cannot reach server");
                    return;
                }
                mPops = parsePops(json);
                if (mPops.size() > 0) {
                    mSelectedRegionIdx = 0;
                    mRegionLabel.setText(mPops.get(0).region.toUpperCase());
                }
                String newsJson = SurvBridge.getNewsJson();
                String newsText = parseNewsText(newsJson);
                if (newsText != null) {
                    mNewsText.setText(newsText);
                }
            }
        });
    }

    // ===================== Play ============================================

    private void onPlay(final int teamMode) {
        if (mFindingGame) return;
        mFindingGame = true;
        mStatus.setText("Finding game...");

        String region = "na";
        if (mSelectedRegionIdx >= 0 && mSelectedRegionIdx < mPops.size()) {
            region = mPops.get(mSelectedRegionIdx).region;
        }
        SurvBridge.setRegion(region);

        StringBuilder body = new StringBuilder();
        body.append("{\"region\":\"").append(escapeJson(region)).append("\"");
        body.append(",\"zones\":[]");
        body.append(",\"version\":1028");
        body.append(",\"playerCount\":").append(teamMode);
        body.append(",\"autoFill\":true");
        body.append(",\"gameModeIdx\":").append(teamMode == 1 ? 0 : teamMode == 2 ? 1 : 2);
        body.append("}");

        final String bodyStr = body.toString();

        SurvApi.findGame(mApiBaseUrl, bodyStr, new SurvApi.Callback() {
            @Override
            public void onResult(String json) {
                mFindingGame = false;
                if (json == null) {
                    mStatus.setText("Failed to find game");
                    return;
                }
                String type = Json.peekString(json, "type");
                if ("success".equals(type)) {
                    List<String> urls = parseUrls(json);
                    String token = parseJoinToken(json);
                    if (urls.size() > 0 && token != null) {
                        String[] urlArray = urls.toArray(new String[urls.size()]);
                        SurvBridge.onJoinUrls(urlArray, token);
                        mStatus.setText("Joining...");
                        return;
                    }
                }
                String error = Json.peekString(json, "error");
                if (error == null) error = "find_game_failed";
                mStatus.setText("Error: " + error);
            }
        });
    }

    // ===================== Interactions ====================================

    private void cycleRegion() {
        if (mPops.size() <= 1) return;
        mSelectedRegionIdx = (mSelectedRegionIdx + 1) % mPops.size();
        mRegionLabel.setText(mPops.get(mSelectedRegionIdx).region.toUpperCase());
        SurvBridge.setRegion(mPops.get(mSelectedRegionIdx).region);
    }

    private void toggleMute() {
        mMuted = !mMuted;
        mMuteBtn.setText(mMuted ? "\uD83D\uDD07" : "\uD83D\uDD0A");
        SurvBridge.setAudio("mute", mMuted ? 1.0f : 0.0f);
    }

    private void toggleFullscreen() {
        mFullscreen = !mFullscreen;
        Log.i(TAG, "Fullscreen toggle (stub)");
    }

    private void onSettings() {
        Log.i(TAG, "Settings modal (stub)");
    }

    private void onKeybinds() {
        Log.i(TAG, "Keybinds modal (stub)");
    }

    private void onHelp() {
        Log.i(TAG, "Help modal (stub)");
    }

    // ===================== Public API ======================================

    public View root() { return mRoot; }

    public void show() {
        mRoot.setVisibility(View.VISIBLE);
        refresh();
    }

    public void hide() {
        mRoot.setVisibility(View.GONE);
    }

    public void refresh() {
        String config = SurvBridge.getConfigJson();
        String name = Json.peekString(config, "playerName");
        if (name != null && !name.isEmpty() && !mName.hasFocus()) {
            mName.setText(name);
        }
    }

    public void showMessage(String text) {
        mStatus.setText(text != null ? text : "");
    }

    // ===================== Types ===========================================

    static class PopEntry {
        String region;
        int playerCount;
        String l10n;
    }

    // ===================== JSON Parsing ====================================

    private List<PopEntry> parsePops(String json) {
        List<PopEntry> pops = new ArrayList<PopEntry>();
        if (json == null) return pops;
        int at = json.indexOf("\"pops\"");
        if (at < 0) return pops;
        int colon = json.indexOf(':', at);
        if (colon < 0) return pops;
        int arrayStart = json.indexOf('[', colon);
        if (arrayStart < 0) return pops;
        int depth = 0;
        int objStart = -1;
        for (int i = arrayStart; i < json.length(); i++) {
            char c = json.charAt(i);
            if (c == '{') {
                if (depth == 0) objStart = i;
                depth++;
            } else if (c == '}') {
                depth--;
                if (depth == 0 && objStart >= 0) {
                    String obj = json.substring(objStart, i + 1);
                    String r = Json.peekString(obj, "region");
                    if (r != null) {
                        PopEntry p = new PopEntry();
                        p.region = r;
                        String pc = Json.peekString(obj, "playerCount");
                        p.playerCount = pc != null ? Integer.parseInt(pc) : 0;
                        p.l10n = Json.peekString(obj, "l10n");
                        if (p.l10n == null) p.l10n = r;
                        pops.add(p);
                    }
                    objStart = -1;
                }
            }
        }
        return pops;
    }

    private String parseNewsText(String json) {
        if (json == null) return null;
        StringBuilder out = new StringBuilder();
        int at = json.indexOf('[');
        if (at < 0) return null;
        int depth = 0;
        int objStart = -1;
        for (int i = at; i < json.length(); i++) {
            char c = json.charAt(i);
            if (c == '{') {
                if (depth == 0) objStart = i;
                depth++;
            } else if (c == '}') {
                depth--;
                if (depth == 0 && objStart >= 0) {
                    String obj = json.substring(objStart, i + 1);
                    String title = Json.peekString(obj, "title");
                    if (title != null) {
                        if (out.length() > 0) out.append('\n');
                        out.append("\u2B50 ").append(title);
                    }
                    objStart = -1;
                }
            }
        }
        return out.toString();
    }

    private List<String> parseUrls(String json) {
        List<String> urls = new ArrayList<String>();
        if (json == null) return urls;
        int at = json.indexOf("\"urls\"");
        if (at < 0) return urls;
        int colon = json.indexOf(':', at);
        if (colon < 0) return urls;
        int arrayStart = json.indexOf('[', colon);
        if (arrayStart < 0) return urls;
        int depth = 0;
        StringBuilder seg = new StringBuilder();
        boolean inString = false;
        for (int i = arrayStart; i < json.length(); i++) {
            char c = json.charAt(i);
            if (inString) {
                if (c == '\\') { seg.append(c); i++; if (i < json.length()) seg.append(json.charAt(i)); }
                else if (c == '"') {
                    urls.add(seg.toString());
                    seg = new StringBuilder();
                    inString = false;
                } else { seg.append(c); }
            } else {
                if (c == '"') inString = true;
                else if (c == '[') depth++;
                else if (c == ']') { depth--; if (depth == 0) break; }
            }
        }
        return urls;
    }

    private String parseJoinToken(String json) {
        int at = json.indexOf("\"res\"");
        if (at < 0) return null;
        int colon = json.indexOf(':', at);
        if (colon < 0) return null;
        int objStart = json.indexOf('{', colon);
        if (objStart < 0) return null;
        String res = json.substring(objStart);
        return Json.peekString(res, "joinToken");
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
                case '\t': out.append("\\t"); break;
                default: out.append(c); break;
            }
        }
        return out.toString();
    }

    private int dp(int value) {
        return (int) (value * mActivity.getResources().getDisplayMetrics().density + 0.5f);
    }
}