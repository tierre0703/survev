/****************************************************************************
 * MenuFragment.java â€” main-menu overlay (ARCHITECTURE.md Â§7, step S3).
 *
 * Layout matches the web client's start menu card:
 *   +--------------------------------------------+
 *   |  [ player name           ] [ customize ]   |
 *   |  [ Local [0 players]                  v ]  |  green outline select
 *   |  [            Play Solo               ]    |
 *   |  [ Play Duo ] [ Play Squad ]               |
 *   |  [ Join Team ] [ Create Team ]             |
 *   |  [          How to Play           ]        |
 *   +--------------------------------------------+
 *
 * A single centered rounded dark card over the splash background. Every
 * button has the web CSS "inset 0 -Npx" darker bottom edge (3D look).
 *
 * Threading rules (ARCHITECTURE.md Â§4.2):
 *   * SurvApi runs network on a background thread and delivers on the UI thread.
 *   * SurvBridge native calls are made from the UI thread.
 ****************************************************************************/
package dev.axmol.app;

import android.app.Activity;
import android.graphics.Color;
import android.graphics.Typeface;
import android.graphics.drawable.GradientDrawable;
import android.graphics.drawable.LayerDrawable;
import android.util.Log;
import android.util.TypedValue;
import android.view.Gravity;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.EditText;
import android.widget.FrameLayout;
import android.widget.LinearLayout;
import android.widget.TextView;

import java.util.ArrayList;
import java.util.List;

public class MenuFragment {
    private static final String TAG = "MenuFragment";

    // ===================== Web Client Color Palette ==========================
    private static final int GREEN         = 0xFF83AF50; // .btn-green
    private static final int GREEN_SHADOW  = 0xFF5B7A38;
    private static final int TEAL          = 0xFF4FADA9; // team buttons
    private static final int TEAL_SHADOW   = 0xFF37817E;
    private static final int GREY          = 0xFF7A7A7A; // how to play
    private static final int GREY_SHADOW   = 0xFF3E3E3E;
    private static final int BLUE          = 0xFF1E90FF; // customize
    private static final int BLUE_SHADOW   = 0xFF1871C8;
    private static final int LIME          = 0xFF00FF00; // region select outline
    private static final int CARD_BG       = 0xCC1B2A12; // dark translucent card
    private static final int SELECT_BG     = 0x66000000;
    private static final int SCRIM         = 0x4C000000; // 0.3 alpha

    private final Activity mActivity;
    private final FrameLayout mRoot;
    private final JavaUiHost mHost;

    // --- widgets ---
    private final EditText mName;
    private final TextView mCustomizeBtn;
    private final TextView mRegionLabel;
    private final TextView mPlaySolo;
    private final TextView mPlayDuo;
    private final TextView mPlaySquad;
    private final TextView mJoinTeamBtn;
    private final TextView mCreateTeamBtn;
    private final TextView mHelpBtn;
    private final TextView mStatus;
    private final TextView mNewsText;       // kept for fetch/update logic; not shown

    // --- state ---
    private List<PopEntry> mPops = new ArrayList<PopEntry>();
    private int mSelectedRegionIdx = 0;
    private boolean mMuted = false;
    private boolean mFindingGame = false;
    private String mApiBaseUrl = "http://127.0.0.1:8000";

    public MenuFragment(Activity activity, JavaUiHost host) {
        mActivity = activity;
        mHost = host;

        // Root: light scrim over the C++ splash; the card is centered on it.
        mRoot = new FrameLayout(activity);
        mRoot.setBackgroundColor(SCRIM);

        // ===== Card =====
        LinearLayout card = new LinearLayout(activity);
        card.setOrientation(LinearLayout.VERTICAL);
        card.setPadding(dp(16), dp(16), dp(16), dp(12));
        GradientDrawable cardBg = new GradientDrawable();
        cardBg.setColor(CARD_BG);
        cardBg.setCornerRadius(dp(8));
        card.setBackground(cardBg);
        FrameLayout.LayoutParams cardParams = new FrameLayout.LayoutParams(
                dp(260), ViewGroup.LayoutParams.WRAP_CONTENT);
        cardParams.gravity = Gravity.CENTER;
        card.setLayoutParams(cardParams);

        // ===== Row 1: name input + customize =====
        LinearLayout nameRow = row(activity);

        mName = new EditText(activity);
        mName.setSingleLine(true);
        mName.setHint("Enter your name here");
        mName.setTextColor(Color.BLACK);
        mName.setHintTextColor(0xFF808080);
        mName.setTypeface(condensed(true));
        mName.setTextSize(TypedValue.COMPLEX_UNIT_SP, 16);
        mName.setGravity(Gravity.CENTER);
        mName.setPadding(dp(6), 0, dp(6), 0);
        GradientDrawable nameBg = new GradientDrawable();
        nameBg.setColor(Color.WHITE);
        nameBg.setCornerRadius(dp(6));
        mName.setBackground(nameBg);
        mName.setOnFocusChangeListener(new View.OnFocusChangeListener() {
            @Override
            public void onFocusChange(View v, boolean hasFocus) {
                if (!hasFocus) {
                    SurvBridge.setPlayerName(mName.getText().toString().trim());
                }
            }
        });
        LinearLayout.LayoutParams nameLp = new LinearLayout.LayoutParams(0, dp(36), 1f);
        nameRow.addView(mName, nameLp);

        // Customize: blue square with a radiation-style icon glyph.
        mCustomizeBtn = makeButton("\u2622", BLUE, BLUE_SHADOW, 20);
        mCustomizeBtn.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                if (mHost != null) mHost.showLoadout();
            }
        });
        LinearLayout.LayoutParams custLp = new LinearLayout.LayoutParams(dp(36), dp(36));
        custLp.leftMargin = dp(4);
        nameRow.addView(mCustomizeBtn, custLp);
        card.addView(nameRow, rowParams(0));

        // ===== Row 2: region select (lime outline + chevron) =====
        mRegionLabel = new TextView(activity);
        mRegionLabel.setText("Local [0 players]");
        mRegionLabel.setTextColor(Color.WHITE);
        mRegionLabel.setTypeface(condensed(false));
        mRegionLabel.setTextSize(TypedValue.COMPLEX_UNIT_SP, 16);
        mRegionLabel.setGravity(Gravity.CENTER);
        mRegionLabel.setCompoundDrawablePadding(0);
        GradientDrawable selBg = new GradientDrawable();
        selBg.setColor(SELECT_BG);
        selBg.setCornerRadius(dp(5));
        selBg.setStroke(dp(2), LIME);
        mRegionLabel.setBackground(selBg);
        mRegionLabel.setClickable(true);
        mRegionLabel.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                cycleRegion();
            }
        });
        FrameLayout selectBox = new FrameLayout(activity);
        selectBox.addView(mRegionLabel, new FrameLayout.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.MATCH_PARENT));
        TextView chevron = new TextView(activity);
        chevron.setText("\u2228"); // reads as a down chevron
        chevron.setTextColor(Color.WHITE);
        chevron.setTextSize(TypedValue.COMPLEX_UNIT_SP, 14);
        FrameLayout.LayoutParams chevLp = new FrameLayout.LayoutParams(
                ViewGroup.LayoutParams.WRAP_CONTENT, ViewGroup.LayoutParams.WRAP_CONTENT);
        chevLp.gravity = Gravity.CENTER_VERTICAL | Gravity.RIGHT;
        chevLp.rightMargin = dp(8);
        selectBox.addView(chevron, chevLp);
        card.addView(selectBox, rowParams(dp(38)));

        // ===== Row 3: Play Solo =====
        mPlaySolo = makeButton("Play Solo", GREEN, GREEN_SHADOW, 17);
        mPlaySolo.setOnClickListener(new View.OnClickListener() {
            @Override public void onClick(View v) { onPlay(1); }
        });
        card.addView(mPlaySolo, rowParams(dp(36)));

        // ===== Row 4: Duo | Squad =====
        LinearLayout modeRow = row(activity);
        mPlayDuo = makeButton("Play Duo", GREEN, GREEN_SHADOW, 17);
        mPlayDuo.setOnClickListener(new View.OnClickListener() {
            @Override public void onClick(View v) { onPlay(2); }
        });
        mPlaySquad = makeButton("Play Squad", GREEN, GREEN_SHADOW, 17);
        mPlaySquad.setOnClickListener(new View.OnClickListener() {
            @Override public void onClick(View v) { onPlay(4); }
        });
        addHalfPair(modeRow, mPlayDuo, mPlaySquad);
        card.addView(modeRow, rowParams(dp(36)));

        // ===== Row 5: Join Team | Create Team =====
        LinearLayout teamRow = row(activity);
        mJoinTeamBtn = makeButton("Join Team", TEAL, TEAL_SHADOW, 17);
        mJoinTeamBtn.setOnClickListener(new View.OnClickListener() {
            @Override public void onClick(View v) {
                if (mHost != null) mHost.showTeam(false, "");
            }
        });
        mCreateTeamBtn = makeButton("Create Team", TEAL, TEAL_SHADOW, 17);
        mCreateTeamBtn.setOnClickListener(new View.OnClickListener() {
            @Override public void onClick(View v) {
                if (mHost != null) mHost.showTeam(true, "");
            }
        });
        addHalfPair(teamRow, mJoinTeamBtn, mCreateTeamBtn);
        card.addView(teamRow, rowParams(dp(36)));

        // ===== Row 6: How to Play =====
        mHelpBtn = makeButton("How to Play", GREY, GREY_SHADOW, 17);
        mHelpBtn.setOnClickListener(new View.OnClickListener() {
            @Override public void onClick(View v) {
                if (mHost != null) mHost.showHelp();
            }
        });
        card.addView(mHelpBtn, rowParams(dp(36)));

        // Status / error line (hidden until there is something to say)
        mStatus = new TextView(activity);
        mStatus.setTextColor(0xFFFF7878);
        mStatus.setTypeface(condensed(false));
        mStatus.setTextSize(TypedValue.COMPLEX_UNIT_SP, 14);
        mStatus.setGravity(Gravity.CENTER);
        mStatus.setVisibility(View.GONE);
        card.addView(mStatus, new LinearLayout.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT));

        // News is not part of this design; keep an off-screen holder so the
        // existing fetch logic keeps working.
        mNewsText = new TextView(activity);

        mRoot.addView(card);

        seedFromConfig();
        fetchSiteInfo();
    }

    // ===================== Layout helpers ===================================

    private LinearLayout row(Activity a) {
        LinearLayout l = new LinearLayout(a);
        l.setOrientation(LinearLayout.HORIZONTAL);
        return l;
    }

    /** Vertical spacing between rows: 6dp, with explicit height (0 = wrap). */
    private LinearLayout.LayoutParams rowParams(int heightPx) {
        LinearLayout.LayoutParams lp = new LinearLayout.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT,
                heightPx > 0 ? heightPx : ViewGroup.LayoutParams.WRAP_CONTENT);
        lp.topMargin = dp(6);
        return lp;
    }

    private void addHalfPair(LinearLayout row, View left, View right) {
        LinearLayout.LayoutParams l = new LinearLayout.LayoutParams(0, ViewGroup.LayoutParams.MATCH_PARENT, 1f);
        l.rightMargin = dp(3);
        LinearLayout.LayoutParams r = new LinearLayout.LayoutParams(0, ViewGroup.LayoutParams.MATCH_PARENT, 1f);
        r.leftMargin = dp(3);
        row.addView(left, l);
        row.addView(right, r);
    }

    private Typeface condensed(boolean bold) {
        return Typeface.create("sans-serif-condensed", bold ? Typeface.BOLD : Typeface.NORMAL);
    }

    /**
     * Web `.btn-*`: rounded rectangle with a darker 4px bottom edge
     * (`box-shadow: inset 0 -4px`) and `filter: brightness(80%)` while pressed.
     */
    private TextView makeButton(String label, final int body, final int shadow, int sp) {
        TextView btn = new TextView(mActivity);
        btn.setText(label);
        btn.setTextColor(Color.WHITE);
        btn.setTypeface(condensed(false));
        btn.setTextSize(TypedValue.COMPLEX_UNIT_SP, sp);
        btn.setGravity(Gravity.CENTER);
        btn.setShadowLayer(dp(1), 0, dp(1), 0x44000000);
        btn.setClickable(true);
        btn.setBackground(buttonBg(body, shadow));
        btn.setOnTouchListener(new View.OnTouchListener() {
            @Override
            public boolean onTouch(View v, MotionEvent e) {
                int a = e.getActionMasked();
                if (a == MotionEvent.ACTION_DOWN) {
                    v.setBackground(buttonBg(dim(body), dim(shadow)));
                } else if (a == MotionEvent.ACTION_UP || a == MotionEvent.ACTION_CANCEL) {
                    v.setBackground(buttonBg(body, shadow));
                }
                return false; // let the click listener fire
            }
        });
        return btn;
    }

    private LayerDrawable buttonBg(int body, int shadow) {
        GradientDrawable bottom = new GradientDrawable();
        bottom.setColor(shadow);
        bottom.setCornerRadius(dp(6));
        GradientDrawable top = new GradientDrawable();
        top.setColor(body);
        top.setCornerRadius(dp(6));
        LayerDrawable ld = new LayerDrawable(new android.graphics.drawable.Drawable[] { bottom, top });
        ld.setLayerInset(1, 0, 0, 0, dp(3)); // body sits above a 3dp shadow edge
        return ld;
    }

    private int dim(int c) {
        float f = 0.8f;
        return Color.rgb((int) (Color.red(c) * f), (int) (Color.green(c) * f), (int) (Color.blue(c) * f));
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
            mRegionLabel.setText(regionText(region, 0));
        }
        mMuted = "true".equals(Json.peekString(config, "muteAudio"));
    }

    /** "Local [0 players]" — the web region select's option text. */
    private String regionText(String region, int players) {
        String name = region.length() > 0
                ? Character.toUpperCase(region.charAt(0)) + region.substring(1)
                : region;
        return name + " [" + players + (players == 1 ? " player]" : " players]");
    }

    private void showRegion() {
        if (mSelectedRegionIdx >= 0 && mSelectedRegionIdx < mPops.size()) {
            PopEntry p = mPops.get(mSelectedRegionIdx);
            mRegionLabel.setText(regionText(p.region, p.playerCount));
        }
    }

    private void setStatus(String text) {
        mStatus.setText(text != null ? text : "");
        mStatus.setVisibility(text != null && !text.isEmpty() ? View.VISIBLE : View.GONE);
    }

    private void fetchSiteInfo() {
        SurvApi.siteInfo(mApiBaseUrl, new SurvApi.Callback() {
            @Override
            public void onResult(String json) {
                if (json == null) {
                    setStatus("Cannot reach server");
                    return;
                }
                mPops = parsePops(json);
                if (mPops.size() > 0) {
                    mSelectedRegionIdx = 0;
                    showRegion();
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
        setStatus("Finding game...");

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
                    setStatus("Failed to find game");
                    return;
                }
                String type = Json.peekString(json, "type");
                if ("success".equals(type)) {
                    List<String> urls = parseUrls(json);
                    String token = parseJoinToken(json);
                    if (urls.size() > 0 && token != null) {
                        String[] urlArray = urls.toArray(new String[urls.size()]);
                        SurvBridge.onJoinUrls(urlArray, token);
                        setStatus("Joining...");
                        return;
                    }
                }
                String error = Json.peekString(json, "error");
                if (error == null) error = "find_game_failed";
                setStatus("Error: " + error);
            }
        });
    }

    // ===================== Interactions ====================================

    private void cycleRegion() {
        if (mPops.size() <= 1) return;
        mSelectedRegionIdx = (mSelectedRegionIdx + 1) % mPops.size();
        showRegion();
        SurvBridge.setRegion(mPops.get(mSelectedRegionIdx).region);
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
        setStatus(text);
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