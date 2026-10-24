/****************************************************************************
 * MenuFragment.java — main-menu overlay (ARCHITECTURE.md §7, step S3).
 *
 * Enhanced main menu matching web client's start menu card with:
 *   +--------------------------------------------+
 *   |  [☢]              [player name    ] [⚙] [♪]|  
 *   |  [ Local [0 players]                  v ]  |
 *   |  [            Play Solo               ]    |
 *   |  [ Play Duo ] [ Play Squad ]               |
 *   |  [ Join Team ] [ Create Team ]             |
 *   |  [       📖 How to Play            ]       |
 *   |  [ 🏆 Survev Pass  ]  [ 📰 News  ]        |
 *   +--------------------------------------------+
 *
 * RESPONSIVE: wraps content in a full-height ScrollView for landscape fit.
 *
 * Threading rules (ARCHITECTURE.md §4.2):
 *   * SurvApi runs network on a background thread and delivers on the UI thread.
 *   * SurvBridge native calls are made from the UI thread.
 ****************************************************************************/
package dev.axmol.app;

import android.app.Activity;
import android.content.Context;
import android.graphics.Color;
import android.graphics.Typeface;
import android.graphics.drawable.GradientDrawable;
import android.graphics.drawable.LayerDrawable;
import android.os.Handler;
import android.os.Looper;
import android.text.TextUtils;
import android.util.Log;
import android.util.TypedValue;
import android.view.Gravity;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewGroup;
import android.view.animation.AlphaAnimation;
import android.view.animation.ScaleAnimation;
import android.widget.Button;
import android.widget.EditText;
import android.widget.FrameLayout;
import android.widget.LinearLayout;
import android.widget.ScrollView;
import android.widget.TextView;
import android.widget.Toast;

import java.util.ArrayList;
import java.util.List;
import java.util.Map;

public class MenuFragment {
    private static final String TAG = "MenuFragment";

    // ===================== Web Client Color Palette ==========================
    private static final int GREEN         = 0xFF83AF50; // .btn-green
    private static final int GREEN_SHADOW  = 0xFF5B7A38;
    private static final int TEAL          = 0xFF4FADA9;
    private static final int TEAL_SHADOW   = 0xFF37817E;
    private static final int GREY          = 0xFF7A7A7A;
    private static final int GREY_SHADOW   = 0xFF3E3E3E;
    private static final int BLUE          = 0xFF1E90FF;
    private static final int BLUE_SHADOW   = 0xFF1871C8;
    private static final int PURPLE        = 0xFF9B59B6;
    private static final int PURPLE_SHADOW = 0xFF6E3D7F;
    private static final int LIME          = 0xFF00FF00;
    private static final int CARD_BG       = 0xCC1B2A12;
    private static final int SELECT_BG     = 0x66000000;
    private static final int SCRIM         = 0x4C000000;
    private static final int GOLD          = 0xFFFFD700;

    private final Activity mActivity;
    private final FrameLayout mRoot;

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
    private final TextView mNewsText;
    private final TextView mSettingsBtn;
    private final TextView mMuteBtn;
    private final TextView mPassBtn;
    private final TextView mNewsBtn;
    private final ScrollView mNewsScrollView;
    private final LinearLayout mTopBarRow;
    private final LinearLayout mBottomRow;
    private final TextView mPlayerCountLbl;
    private final JavaUiHost mHost;

    // --- state ---
    private List<PopEntry> mPops = new ArrayList<PopEntry>();
    // M8 region latency test results (client/src/pingTest.ts port), one entry per
    // site_info pop; empty until the probes finish.
    private List<Json.PingEntry> mPings = new ArrayList<Json.PingEntry>();
    private int mSelectedRegionIdx = 0;
    // The web's config `regionSelected`: once the user picked a region, the ping
    // test must not override it.
    private boolean mRegionChosen = false;
    private boolean mMuted = false;
    private boolean mFindingGame = false;
    private String mApiBaseUrl = "http://127.0.0.1:8000";
    private boolean mNewsExpanded = false;
    private boolean mRegionDropdownOpen = false;
    private final LinearLayout mRegionDropdown;
    private final FrameLayout mRegionRowContainer;

    public MenuFragment(Activity activity, JavaUiHost host) {
        mActivity = activity;
        mHost = host;

        mRoot = new FrameLayout(activity);
        mRoot.setBackgroundColor(SCRIM);

        // Full-height ScrollView for landscape responsiveness
        ScrollView scrollView = new ScrollView(activity);
        scrollView.setLayoutParams(new FrameLayout.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.MATCH_PARENT));

        // ===== Card (wrapped inside scroll) =====
        LinearLayout card = new LinearLayout(activity);
        card.setOrientation(LinearLayout.VERTICAL);
        card.setPadding(dp(14), dp(14), dp(14), dp(12));
        GradientDrawable cardBg = new GradientDrawable();
        cardBg.setColor(CARD_BG);
        cardBg.setCornerRadius(dp(10));
        card.setBackground(cardBg);
        FrameLayout.LayoutParams cardParams = new FrameLayout.LayoutParams(
                dp(280), ViewGroup.LayoutParams.WRAP_CONTENT);
        cardParams.gravity = Gravity.CENTER;
        card.setLayoutParams(cardParams);

        // ===== Row 0: Top bar =====
        mTopBarRow = row(activity);
        
        mCustomizeBtn = makeIconButton("\u2622", BLUE, BLUE_SHADOW, 22);
        mCustomizeBtn.setOnClickListener(new View.OnClickListener() {
            @Override public void onClick(View v) {
                animatePress(v);
                if (mHost != null) mHost.showLoadout();
            }
        });
        mTopBarRow.addView(mCustomizeBtn, new LinearLayout.LayoutParams(dp(36), dp(36)));

        View spacer = new View(activity);
        spacer.setLayoutParams(new LinearLayout.LayoutParams(0, dp(36), 1f));
        mTopBarRow.addView(spacer);

        mPlayerCountLbl = new TextView(activity);
        mPlayerCountLbl.setText("\uD83D\uDC65 0");
        mPlayerCountLbl.setTextColor(0xFFAAAAAA);
        mPlayerCountLbl.setTextSize(TypedValue.COMPLEX_UNIT_SP, 11);
        mPlayerCountLbl.setGravity(Gravity.CENTER);
        mTopBarRow.addView(mPlayerCountLbl, new LinearLayout.LayoutParams(
                ViewGroup.LayoutParams.WRAP_CONTENT, ViewGroup.LayoutParams.WRAP_CONTENT));

        mSettingsBtn = makeIconButton("\u2699", 0xFF666666, 0xFF444444, 20);
        mSettingsBtn.setOnClickListener(new View.OnClickListener() {
            @Override public void onClick(View v) {
                animatePress(v);
                if (mHost != null) mHost.showSettings();
            }
        });
        LinearLayout.LayoutParams setLp = new LinearLayout.LayoutParams(dp(32), dp(32));
        setLp.leftMargin = dp(8);
        mTopBarRow.addView(mSettingsBtn, setLp);

        mMuteBtn = makeIconButton("\u266A", 0xFF666666, 0xFF444444, 18);
        mMuteBtn.setOnClickListener(new View.OnClickListener() {
            @Override public void onClick(View v) {
                animatePress(v);
                toggleMute();
            }
        });
        LinearLayout.LayoutParams muteLp = new LinearLayout.LayoutParams(dp(32), dp(32));
        muteLp.leftMargin = dp(4);
        mTopBarRow.addView(mMuteBtn, muteLp);
        
        card.addView(mTopBarRow, rowParams(0));

        // ===== Row 1: name input =====
        LinearLayout nameRow = row(activity);
        mName = new EditText(activity);
        mName.setSingleLine(true);
        mName.setHint("Enter your name");
        mName.setTextColor(Color.BLACK);
        mName.setHintTextColor(0xFF808080);
        mName.setTextSize(TypedValue.COMPLEX_UNIT_SP, 15);
        mName.setGravity(Gravity.CENTER);
        mName.setPadding(dp(10), 0, dp(10), 0);
        GradientDrawable nameBg = new GradientDrawable();
        nameBg.setColor(Color.WHITE);
        nameBg.setCornerRadius(dp(6));
        mName.setBackground(nameBg);
        mName.setOnFocusChangeListener(new View.OnFocusChangeListener() {
            @Override public void onFocusChange(View v, boolean hasFocus) {
                if (!hasFocus) {
                    SurvBridge.setPlayerName(mName.getText().toString().trim());
                }
            }
        });
        nameRow.addView(mName, new LinearLayout.LayoutParams(0, dp(38), 1f));
        card.addView(nameRow, rowParams(0));

        // ===== Row 2: region select (click opens dropdown) =====
        mRegionLabel = new TextView(activity);
        mRegionLabel.setText("Local [0 players]");
        mRegionLabel.setTextColor(Color.WHITE);
        mRegionLabel.setTextSize(TypedValue.COMPLEX_UNIT_SP, 15);
        mRegionLabel.setGravity(Gravity.CENTER);
        GradientDrawable selBg = new GradientDrawable();
        selBg.setColor(SELECT_BG);
        selBg.setCornerRadius(dp(5));
        selBg.setStroke(dp(2), LIME);
        mRegionLabel.setBackground(selBg);
        mRegionLabel.setClickable(true);
        mRegionLabel.setOnClickListener(new View.OnClickListener() {
            @Override public void onClick(View v) {
                toggleRegionDropdown();
                animatePress(v);
            }
        });
        mRegionRowContainer = new FrameLayout(activity);
        mRegionRowContainer.addView(mRegionLabel, new FrameLayout.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.MATCH_PARENT));
        TextView chevron = new TextView(activity);
        chevron.setText("\u25BC");
        chevron.setTextColor(LIME);
        chevron.setTextSize(TypedValue.COMPLEX_UNIT_SP, 12);
        FrameLayout.LayoutParams chevLp = new FrameLayout.LayoutParams(
                ViewGroup.LayoutParams.WRAP_CONTENT, ViewGroup.LayoutParams.WRAP_CONTENT);
        chevLp.gravity = Gravity.CENTER_VERTICAL | Gravity.RIGHT;
        chevLp.rightMargin = dp(10);
        mRegionRowContainer.addView(chevron, chevLp);
        card.addView(mRegionRowContainer, rowParams(dp(40)));

        // ===== Region dropdown (scrollable list, hidden by default) =====
        mRegionDropdown = new LinearLayout(activity);
        mRegionDropdown.setOrientation(LinearLayout.VERTICAL);
        mRegionDropdown.setBackgroundColor(0xDD333333);
        mRegionDropdown.setVisibility(View.GONE);
        card.addView(mRegionDropdown, new LinearLayout.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT));

        // ===== Row 3: Play Solo =====
        mPlaySolo = makeButton("\u25B6 Play Solo", GREEN, GREEN_SHADOW, 18);
        mPlaySolo.setOnClickListener(new View.OnClickListener() {
            @Override public void onClick(View v) { 
                animatePress(v);
                onPlay(1); 
            }
        });
        card.addView(mPlaySolo, rowParams(dp(42)));

        // ===== Row 4: Duo | Squad =====
        LinearLayout modeRow = row(activity);
        mPlayDuo = makeButton("Duo", GREEN, GREEN_SHADOW, 16);
        mPlayDuo.setOnClickListener(new View.OnClickListener() {
            @Override public void onClick(View v) { 
                animatePress(v);
                onPlay(2); 
            }
        });
        mPlaySquad = makeButton("Squad", GREEN, GREEN_SHADOW, 16);
        mPlaySquad.setOnClickListener(new View.OnClickListener() {
            @Override public void onClick(View v) { 
                animatePress(v);
                onPlay(4); 
            }
        });
        addHalfPair(modeRow, mPlayDuo, mPlaySquad);
        card.addView(modeRow, rowParams(dp(38)));

        // ===== Row 5: Join Team | Create Team =====
        LinearLayout teamRow = row(activity);
        mJoinTeamBtn = makeButton("Join Team", TEAL, TEAL_SHADOW, 15);
        mJoinTeamBtn.setOnClickListener(new View.OnClickListener() {
            @Override public void onClick(View v) {
                animatePress(v);
                if (mHost != null) mHost.showTeam(false, "");
            }
        });
        mCreateTeamBtn = makeButton("Create Team", TEAL, TEAL_SHADOW, 15);
        mCreateTeamBtn.setOnClickListener(new View.OnClickListener() {
            @Override public void onClick(View v) {
                animatePress(v);
                if (mHost != null) mHost.showTeam(true, "");
            }
        });
        addHalfPair(teamRow, mJoinTeamBtn, mCreateTeamBtn);
        card.addView(teamRow, rowParams(dp(38)));

        // ===== Row 6: How to Play =====
        mHelpBtn = makeButton("\uD83D\uDCD6 How to Play", GREY, GREY_SHADOW, 15);
        mHelpBtn.setOnClickListener(new View.OnClickListener() {
            @Override public void onClick(View v) {
                animatePress(v);
                if (mHost != null) mHost.showHelp();
            }
        });
        card.addView(mHelpBtn, rowParams(dp(36)));

        // ===== Separator =====
        View sep = new View(activity);
        sep.setLayoutParams(new LinearLayout.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT, dp(1)));
        sep.setBackgroundColor(0x33FFFFFF);
        card.addView(sep, new LinearLayout.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT, dp(1)));

        // ===== Row 7: Pass + News =====
        mBottomRow = row(activity);
        
        mPassBtn = makeButton("\uD83C\uDFC6 Pass", PURPLE, PURPLE_SHADOW, 14);
        mPassBtn.setOnClickListener(new View.OnClickListener() {
            @Override public void onClick(View v) {
                animatePress(v);
                if (mHost != null) mHost.showPass();
            }
        });
        LinearLayout.LayoutParams passLp = new LinearLayout.LayoutParams(0, dp(32), 1f);
        passLp.rightMargin = dp(4);
        mBottomRow.addView(mPassBtn, passLp);

        mNewsBtn = makeButton("\uD83D\uDCF0 News", 0xFF555555, 0xFF333333, 14);
        mNewsBtn.setOnClickListener(new View.OnClickListener() {
            @Override public void onClick(View v) {
                animatePress(v);
                toggleNews();
            }
        });
        LinearLayout.LayoutParams newsLp = new LinearLayout.LayoutParams(0, dp(32), 1f);
        newsLp.leftMargin = dp(4);
        mBottomRow.addView(mNewsBtn, newsLp);
        
        card.addView(mBottomRow, rowParams(dp(34)));

        // ===== News Panel =====
        mNewsScrollView = new ScrollView(activity);
        mNewsScrollView.setLayoutParams(new LinearLayout.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT, dp(120)));
        mNewsScrollView.setVisibility(View.GONE);
        
        mNewsText = new TextView(activity);
        mNewsText.setTextColor(0xFFCCCCCC);
        mNewsText.setTextSize(TypedValue.COMPLEX_UNIT_SP, 13);
        mNewsText.setPadding(dp(8), dp(8), dp(8), dp(8));
        mNewsScrollView.addView(mNewsText);
        card.addView(mNewsScrollView);

        // Status / error line
        mStatus = new TextView(activity);
        mStatus.setTextColor(0xFFFF7878);
        mStatus.setTextSize(TypedValue.COMPLEX_UNIT_SP, 13);
        mStatus.setGravity(Gravity.CENTER);
        mStatus.setVisibility(View.GONE);
        card.addView(mStatus, new LinearLayout.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT));

        scrollView.addView(card);
        mRoot.addView(scrollView);

        seedFromConfig();
        fetchSiteInfo();
    }

    // ===================== Layout helpers ===================================

    private LinearLayout row(Activity a) {
        LinearLayout l = new LinearLayout(a);
        l.setOrientation(LinearLayout.HORIZONTAL);
        return l;
    }

    private LinearLayout.LayoutParams rowParams(int heightPx) {
        LinearLayout.LayoutParams lp = new LinearLayout.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT,
                heightPx > 0 ? heightPx : ViewGroup.LayoutParams.WRAP_CONTENT);
        lp.topMargin = dp(5);
        return lp;
    }

    private void addHalfPair(LinearLayout row, View left, View right) {
        LinearLayout.LayoutParams l = new LinearLayout.LayoutParams(0, ViewGroup.LayoutParams.MATCH_PARENT, 1f);
        l.rightMargin = dp(4);
        LinearLayout.LayoutParams r = new LinearLayout.LayoutParams(0, ViewGroup.LayoutParams.MATCH_PARENT, 1f);
        r.leftMargin = dp(4);
        row.addView(left, l);
        row.addView(right, r);
    }

    private TextView makeButton(String label, final int body, final int shadow, int sp) {
        TextView btn = new TextView(mActivity);
        btn.setText(label);
        btn.setTextColor(Color.WHITE);
        btn.setTextSize(TypedValue.COMPLEX_UNIT_SP, sp);
        btn.setGravity(Gravity.CENTER);
        btn.setShadowLayer(dp(1), 0, dp(1), 0x44000000);
        btn.setClickable(true);
        btn.setBackground(buttonBg(body, shadow));
        btn.setOnTouchListener(new View.OnTouchListener() {
            @Override public boolean onTouch(View v, MotionEvent e) {
                int a = e.getActionMasked();
                if (a == MotionEvent.ACTION_DOWN) {
                    v.setBackground(buttonBg(dim(body), dim(shadow)));
                } else if (a == MotionEvent.ACTION_UP || a == MotionEvent.ACTION_CANCEL) {
                    v.setBackground(buttonBg(body, shadow));
                }
                return false;
            }
        });
        return btn;
    }

    private TextView makeIconButton(String label, final int body, final int shadow, int sp) {
        TextView btn = new TextView(mActivity);
        btn.setText(label);
        btn.setTextColor(Color.WHITE);
        btn.setTextSize(TypedValue.COMPLEX_UNIT_SP, sp);
        btn.setGravity(Gravity.CENTER);
        btn.setClickable(true);
        GradientDrawable bg = new GradientDrawable();
        bg.setColor(body);
        bg.setCornerRadius(dp(6));
        btn.setBackground(bg);
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
        ld.setLayerInset(1, 0, 0, 0, dp(3));
        return ld;
    }

    private int dim(int c) {
        float f = 0.80f;
        return Color.rgb((int) (Color.red(c) * f), (int) (Color.green(c) * f), (int) (Color.blue(c) * f));
    }

    private void animatePress(View v) {
        ScaleAnimation scaleDown = new ScaleAnimation(1.0f, 0.95f, 1.0f, 0.95f, 0.5f, 100);
        v.startAnimation(scaleDown);
        new Handler(Looper.getMainLooper()).postDelayed(new Runnable() {
            @Override public void run() {
                ScaleAnimation scaleUp = new ScaleAnimation(0.95f, 1.0f, 0.95f, 1.0f, 0.5f, 100);
                v.startAnimation(scaleUp);
            }
        }, 100);
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
        mMuted = Json.peekBool(config, "muteAudio", false);
        updateMuteButton();
    }

    private String regionText(String region, int players) {
        String name = region.length() > 0
                ? Character.toUpperCase(region.charAt(0)) + region.substring(1)
                : region;
        String text = name + " [" + players + (players == 1 ? " player]" : " players]") + "";
        int ping = pingFor(region);
        if (ping >= 0) {
            text += " \u00B7 " + ping + " ms";
        }
        return text;
    }

    /** Latency of `region` from the last ping test, or -1 when it did not answer. */
    private int pingFor(String region) {
        // A region may have several zones; rank it by its fastest one, like the
        // web sorting every test and taking the best (pingTest.getRegion()).
        int best = -1;
        for (Json.PingEntry p : mPings) {
            if (!p.region.equals(region) || p.pingMs < 0) {
                continue;
            }
            if (best < 0 || p.pingMs < best) {
                best = p.pingMs;
            }
        }
        return best;
    }

    private int indexOfRegion(String region) {
        for (int i = 0; i < mPops.size(); i++) {
            if (mPops.get(i).region.equals(region)) {
                return i;
            }
        }
        return -1;
    }

    private void showRegion() {
        if (mSelectedRegionIdx >= 0 && mSelectedRegionIdx < mPops.size()) {
            PopEntry p = mPops.get(mSelectedRegionIdx);
            mRegionLabel.setText(regionText(p.region, p.playerCount));
            mPlayerCountLbl.setText("\uD83D\uDC65 " + p.playerCount);
        }
    }

    private void setStatus(String text) {
        mStatus.setText(text != null ? text : "");
        mStatus.setVisibility(text != null && !text.isEmpty() ? View.VISIBLE : View.GONE);
        if (text != null && !text.isEmpty()) {
            AlphaAnimation flash = new AlphaAnimation(1.0f, 0.5f);
            flash.setDuration(300);
            mStatus.startAnimation(flash);
        }
    }

    private void toggleMute() {
        mMuted = !mMuted;
        SurvBridge.setAudio("mute", mMuted ? 1.0f : 0.0f);
        SurvBridge.saveConfig();
        updateMuteButton();
    }
    
    private void updateMuteButton() {
        mMuteBtn.setText(mMuted ? "\uD83D\uDD07" : "\u266A");
        mMuteBtn.setTextColor(mMuted ? 0xFFFF6666 : Color.WHITE);
    }

    private void toggleNews() {
        mNewsExpanded = !mNewsExpanded;
        mNewsScrollView.setVisibility(mNewsExpanded ? View.VISIBLE : View.GONE);
        mNewsBtn.setText(mNewsExpanded ? "\u25B2 Hide News" : "\uD83D\uDCF0 News");
    }

    private void fetchSiteInfo() {
        setStatus("Connecting to server...");
        SurvApi.siteInfo(mApiBaseUrl, new SurvApi.Callback() {
            @Override public void onResult(String json) {
                if (json == null) {
                    setStatus("Cannot reach server");
                    return;
                }
                Log.i(TAG, "site_info response: " + json.substring(0, Math.min(json.length(), 200)));
                List<Json.PopEntry> parsed = Json.parsePops(json);
                mPops.clear();
                for (Json.PopEntry p : parsed) {
                    PopEntry myP = new PopEntry();
                    myP.region = p.region;
                    myP.playerCount = p.playerCount;
                    myP.l10n = p.l10n;
                    mPops.add(myP);
                }
                if (mPops.size() > 0) {
                    // Keep the persisted choice (web `config.regionSelected`) so
                    // the latency test only auto-selects for new players.
                    mRegionChosen = Json.peekBool(SurvBridge.getConfigJson(), "regionSelected", false);
                    mSelectedRegionIdx = 0;
                    String saved = Json.peekString(SurvBridge.getConfigJson(), "region");
                    if (mRegionChosen && saved != null) {
                        int idx = indexOfRegion(saved);
                        if (idx >= 0) mSelectedRegionIdx = idx;
                    }
                    showRegion();
                    buildRegionDropdown();
                    setStatus("");
                } else {
                    Log.w(TAG, "No regions parsed from site_info.");
                    setStatus("No regions available from server");
                }
                String newsJson = SurvBridge.getNewsJson();
                String newsText = parseNewsText(newsJson);
                if (newsText != null) {
                    mNewsText.setText(newsText);
                } else {
                    mNewsText.setText("\uD83D\uDCE4 No news available.\nCheck back later for updates!");
                }
            }
        });
    }

    // ===================== Region latency (M8) =============================

    /** C++ finished every probe: show the latencies and pick the closest region. */
    public void onPingResults(String json) {
        List<Json.PingEntry> pings = Json.parsePings(json);
        if (pings.isEmpty()) {
            return;
        }
        mPings = pings;
        Log.i(TAG, "pingTest results " + json);

        if (!mRegionChosen) {
            // pingTest.ts getRegion(): the lowest latency wins, ties keep the
            // earlier region (the C++ side already ordered bestRegion, but the
            // menu needs the index).
            int bestIdx = -1;
            int bestMs = Integer.MAX_VALUE;
            for (int i = 0; i < mPops.size(); i++) {
                int ms = pingFor(mPops.get(i).region);
                if (ms >= 0 && ms < bestMs) {
                    bestMs = ms;
                    bestIdx = i;
                }
            }
            if (bestIdx >= 0) {
                mSelectedRegionIdx = bestIdx;
                SurvBridge.setRegion(mPops.get(bestIdx).region);
            }
        }
        showRegion();
        if (mRegionDropdownOpen) {
            buildRegionDropdown();
        }
    }

    /** Zones of `region`, fastest first (pingTest.getZones); "[]" when unknown. */
    private String zonesJson(String region) {
        List<Json.PingEntry> matches = new ArrayList<Json.PingEntry>();
        for (Json.PingEntry p : mPings) {
            if (p.region.equals(region)) {
                matches.add(p);
            }
        }
        // Insertion sort by measured latency; unmeasured (-1) goes last.
        for (int i = 1; i < matches.size(); i++) {
            Json.PingEntry cur = matches.get(i);
            int key = cur.pingMs < 0 ? Integer.MAX_VALUE : cur.pingMs;
            int j = i - 1;
            while (j >= 0) {
                int other = matches.get(j).pingMs < 0 ? Integer.MAX_VALUE : matches.get(j).pingMs;
                if (other <= key) break;
                matches.set(j + 1, matches.get(j));
                j--;
            }
            matches.set(j + 1, cur);
        }
        StringBuilder sb = new StringBuilder("[");
        int count = 0;
        for (Json.PingEntry p : matches) {
            if (p.zone == null || p.zone.isEmpty()) {
                continue;
            }
            if (count++ > 0) {
                sb.append(',');
            }
            sb.append('"').append(escapeJson(p.zone)).append('"');
        }
        sb.append(']');
        return sb.toString();
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

        int gameModeIdx = teamMode == 1 ? 0 : teamMode == 2 ? 1 : 2;
        final String bodyStr = "{\"region\":\"" + escapeJson(region) + "\""
                + ",\"zones\":" + zonesJson(region)
                + ",\"version\":1028"
                + ",\"playerCount\":" + teamMode
                + ",\"autoFill\":true"
                + ",\"gameModeIdx\":" + gameModeIdx
                + "}";
        Log.i(TAG, "onPlay find_game body=" + bodyStr);

        SurvApi.findGame(mApiBaseUrl, bodyStr, new SurvApi.Callback() {
            @Override public void onResult(String json) {
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

    private void toggleRegionDropdown() {
        if (mPops.isEmpty()) {
            // Ping the server again
            fetchSiteInfo();
            return;
        }
        mRegionDropdownOpen = !mRegionDropdownOpen;
        mRegionDropdown.setVisibility(mRegionDropdownOpen ? View.VISIBLE : View.GONE);
    }

    private void buildRegionDropdown() {
        mRegionDropdown.removeAllViews();
        for (int i = 0; i < mPops.size(); i++) {
            final int idx = i;
            PopEntry p = mPops.get(i);
            TextView item = new TextView(mActivity);
            item.setText(regionText(p.region, p.playerCount));
            item.setTextColor(Color.WHITE);
            item.setTextSize(TypedValue.COMPLEX_UNIT_SP, 14);
            item.setGravity(Gravity.CENTER);
            item.setPadding(dp(8), dp(6), dp(8), dp(6));
            GradientDrawable itemBg = new GradientDrawable();
            itemBg.setColor(0xFF444444);
            itemBg.setCornerRadius(dp(4));
            item.setBackground(itemBg);
            item.setClickable(true);
            final int fi = i;
            item.setOnClickListener(new View.OnClickListener() {
                @Override public void onClick(View v) {
                    selectRegion(fi);
                }
            });
            mRegionDropdown.addView(item, new LinearLayout.LayoutParams(
                    ViewGroup.LayoutParams.MATCH_PARENT, dp(36)));
        }
    }

    private void selectRegion(int idx) {
        if (idx < 0 || idx >= mPops.size()) return;
        mSelectedRegionIdx = idx;
        mRegionChosen = true;
        showRegion();
        SurvBridge.setRegion(mPops.get(idx).region);
        // Close dropdown
        mRegionDropdownOpen = false;
        mRegionDropdown.setVisibility(View.GONE);
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
        mMuted = Json.peekBool(config, "muteAudio", false);
        updateMuteButton();
    }

    public void showMessage(String text) {
        setStatus(text);
    }

    // ===================== Types ===========================================

    static class PopEntry {
        String region; int playerCount; String l10n;
    }

    // ===================== JSON Parsing ====================================

    private String parseNewsText(String json) {
        if (json == null) return null;
        StringBuilder out = new StringBuilder();
        int at = json.indexOf('[');
        if (at < 0) return null;
        int depth = 0, objStart = -1;
        for (int i = at; i < json.length(); i++) {
            char c = json.charAt(i);
            if (c == '{') { if (depth == 0) objStart = i; depth++; }
            else if (c == '}') {
                depth--;
                if (depth == 0 && objStart >= 0) {
                    String obj = json.substring(objStart, i + 1);
                    String title = Json.peekString(obj, "title");
                    if (title != null) {
                        if (out.length() > 0) out.append('\n');
                        out.append("\u2726 ").append(title);
                        String desc = Json.peekString(obj, "body");
                        if (desc != null && !desc.isEmpty()) {
                            out.append("\n   ").append(desc.length() > 100 ? desc.substring(0, 100) + "..." : desc);
                        }
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
        try {
            com.google.gson.JsonObject root = com.google.gson.JsonParser.parseString(json).getAsJsonObject();
            com.google.gson.JsonElement resEl = root.get("res");
            if (resEl != null && resEl.isJsonObject()) {
                com.google.gson.JsonObject resObj = resEl.getAsJsonObject();
                com.google.gson.JsonElement urlsEl = resObj.get("urls");
                if (urlsEl != null && urlsEl.isJsonArray()) {
                    com.google.gson.JsonArray arr = urlsEl.getAsJsonArray();
                    for (int i = 0; i < arr.size(); i++) {
                        urls.add(arr.get(i).getAsString());
                    }
                }
            }
        } catch (Exception e) {
            Log.w(TAG, "parseUrls failed: " + e.getMessage());
        }
        return urls;
    }

    private String parseJoinToken(String json) {
        if (json == null) return null;
        try {
            com.google.gson.JsonObject root = com.google.gson.JsonParser.parseString(json).getAsJsonObject();
            com.google.gson.JsonElement resEl = root.get("res");
            if (resEl != null && resEl.isJsonObject()) {
                com.google.gson.JsonObject resObj = resEl.getAsJsonObject();
                com.google.gson.JsonElement tokenEl = resObj.get("joinToken");
                if (tokenEl != null && tokenEl.isJsonPrimitive()) {
                    return tokenEl.getAsString();
                }
            }
        } catch (Exception e) {
            Log.w(TAG, "parseJoinToken failed: " + e.getMessage());
        }
        return null;
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