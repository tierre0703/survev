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
import android.app.AlertDialog;
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
import android.widget.ProgressBar;
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
    private final TextView mJoinTeamBtn;
    private final TextView mCreateTeamBtn;
    private final TextView mHelpBtn;
    private final TextView mStatus;
    private final TextView mNewsText;
    private final TextView mSettingsBtn;
    private final TextView mMuteBtn;
    private final TextView mProfileBtn;
    private final TextView mPassBtn;
    private final TextView mNewsBtn;
    private final ScrollView mNewsScrollView;
    private final LinearLayout mTopBarRow;
    private final LinearLayout mBottomRow;
    private final LinearLayout mModeArea;
    private final LinearLayout mTeamRow;
    private final ProgressBar mPlaySpinner;
    private final TextView mPlayerCountLbl;
    private final JavaUiHost mHost;

    // --- state ---
    private List<PopEntry> mPops = new ArrayList<PopEntry>();
    // site_info.modes (siteInfo.ts getGameModeStyles): the quick-play buttons
    // are built from this list, never hard-coded.
    private List<Json.ModeEntry> mModes = new ArrayList<Json.ModeEntry>();
    private final List<TextView> mModeButtons = new ArrayList<TextView>();
    private boolean mSiteInfoLoaded = false;
    // M8 region latency test results (client/src/pingTest.ts port), one entry per
    // site_info pop; empty until the probes finish.
    private List<Json.PingEntry> mPings = new ArrayList<Json.PingEntry>();
    private int mSelectedRegionIdx = 0;
    // The web's config `regionSelected`: once the user picked a region, the ping
    // test must not override it.
    private boolean mRegionChosen = false;
    private boolean mMuted = false;
    private boolean mFindingGame = false;
    private String mApiBaseUrl = SurvApi.apiBaseUrl();
    // Raw /api/site_info response, shared with TeamFragment (which needs the
    // region list and mode→teamMode table). C++ has no MenuScene on Android, so
    // SurvBridge.getSiteInfoJson() is empty.
    private String mSiteInfoJson = "";
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

        mProfileBtn = makeIconButton("\uD83D\uDC64", 0xFF444444, 0xFF2B2B2B, 16);
        mProfileBtn.setContentDescription("Account");
        mProfileBtn.setOnClickListener(new View.OnClickListener() {
            @Override public void onClick(View v) {
                animatePress(v);
                if (mHost != null) mHost.showProfile();
            }
        });
        LinearLayout.LayoutParams profileLp = new LinearLayout.LayoutParams(dp(32), dp(32));
        profileLp.leftMargin = dp(8);
        mTopBarRow.addView(mProfileBtn, profileLp);

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
        mName.setHint(L10n.t("index-enter-name-here", "Enter your name here"));
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
                    String name = mName.getText().toString().trim();
                    SurvBridge.setPlayerName(name);
                    // A hand-edited name stops following future account renames.
                    AccountManager account = AccountManager.peek();
                    if (account != null && !name.equals(account.getBoundPlayerName())) {
                        account.unbindPlayerName();
                    }
                }
            }
        });
        nameRow.addView(mName, new LinearLayout.LayoutParams(0, dp(38), 1f));
        card.addView(nameRow, rowParams(0));

        // ===== Row 2: region select (click opens dropdown) =====
        mRegionLabel = new TextView(activity);
        mRegionLabel.setText(L10n.t("index-region", "Region"));
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

        // ===== Rows 3/4: quick-play modes (site_info.modes) =====
        // Buttons are created by buildModeButtons() once /api/site_info answers;
        // until then the spinner is shown and Play is unavailable (P1.7).
        mModeArea = new LinearLayout(activity);
        mModeArea.setOrientation(LinearLayout.VERTICAL);
        card.addView(mModeArea, new LinearLayout.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT));

        mPlaySpinner = new ProgressBar(activity, null, android.R.attr.progressBarStyleSmall);
        mPlaySpinner.setVisibility(View.VISIBLE);
        LinearLayout spinnerRow = row(activity);
        spinnerRow.setGravity(Gravity.CENTER);
        spinnerRow.addView(mPlaySpinner, new LinearLayout.LayoutParams(dp(28), dp(28)));
        card.addView(spinnerRow, rowParams(dp(30)));

        // ===== Row 5: Join Team | Create Team =====
        mTeamRow = row(activity);
        mJoinTeamBtn = makeButton(L10n.t("index-join-team", "Join Team"), TEAL, TEAL_SHADOW, 15);
        mJoinTeamBtn.setOnClickListener(new View.OnClickListener() {
            @Override public void onClick(View v) {
                animatePress(v);
                if (mHost != null) mHost.showTeam(false, "");
            }
        });
        mCreateTeamBtn = makeButton(L10n.t("index-create-team", "Create Team"), TEAL, TEAL_SHADOW, 15);
        mCreateTeamBtn.setOnClickListener(new View.OnClickListener() {
            @Override public void onClick(View v) {
                animatePress(v);
                if (mHost != null) mHost.showTeam(true, "");
            }
        });
        addHalfPair(mTeamRow, mJoinTeamBtn, mCreateTeamBtn);
        // siteInfo.ts: team buttons only when an enabled mode has teamMode > 1.
        mTeamRow.setVisibility(View.GONE);
        card.addView(mTeamRow, rowParams(dp(38)));

        // ===== Row 6: How to Play =====
        mHelpBtn = makeButton("\uD83D\uDCD6 " + L10n.t("index-how-to-play", "How to Play"),
                GREY, GREY_SHADOW, 15);
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
        mStatus.setOnClickListener(new View.OnClickListener() {
            @Override public void onClick(View v) {
                // Retry the metadata fetch when it failed; Play stays hidden
                // until site_info answers (P1.7).
                if (!mSiteInfoLoaded && !mFindingGame) {
                    fetchSiteInfo();
                }
            }
        });
        card.addView(mStatus, new LinearLayout.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT));

        scrollView.addView(card);
        mRoot.addView(scrollView);

        seedFromConfig();
        fetchSiteInfo();
        // The Java UI is constructed before the C++ config/locale handshake on
        // a cold start; re-localize once the bridge has applied the saved
        // language (and retry the metadata fetch if it ran too early).
        mRoot.postDelayed(new Runnable() {
            @Override public void run() {
                refresh();
                if (mSiteInfoJson.isEmpty() && !mFindingGame) {
                    fetchSiteInfo();
                }
            }
        }, 1000);
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
        mMuted = Json.peekBool(config, "muteAudio", false);
        updateMuteButton();
    }

    private String regionText(String region, int players) {
        String name = null;
        for (PopEntry p : mPops) {
            if (p.region.equals(region)) {
                name = L10n.t(p.l10n, capitalizeRegion(region));
                break;
            }
        }
        if (name == null) {
            name = capitalizeRegion(region);
        }
        String playersWord = L10n.t("index-players", "players");
        String text = name + " [" + players + " " + playersWord + "]";
        int ping = pingFor(region);
        if (ping >= 0) {
            text += " \u00B7 " + ping + " ms";
        }
        return text;
    }

    private static String capitalizeRegion(String region) {
        return region != null && region.length() > 0
                ? Character.toUpperCase(region.charAt(0)) + region.substring(1)
                : "";
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
        mNewsBtn.setText((mNewsExpanded ? "\u25B2 " : "\uD83D\uDCF0 ")
                + L10n.t("index-news", "News"));
    }

    private void fetchSiteInfo() {
        mSiteInfoLoaded = false;
        mPlaySpinner.setVisibility(View.VISIBLE);
        setStatus(L10n.t("index-connecting", "Connecting to server..."));
        SurvApi.siteInfo(mApiBaseUrl, new SurvApi.Callback() {
            @Override public void onResult(String json) {
                if (json == null) {
                    mPlaySpinner.setVisibility(View.GONE);
                    setStatus(L10n.t("index-cannot-reach-server", "Cannot reach server"));
                    return;
                }
                Log.i(TAG, "site_info response: " + json.substring(0, Math.min(json.length(), 200)));
                mSiteInfoJson = json;
                mModes = Json.parseModes(json);
                buildModeButtons();
                // siteInfo.ts: Join/Create Team only when an enabled team mode exists.
                boolean supportsTeam = false;
                for (Json.ModeEntry mode : mModes) {
                    if (mode.enabled && mode.teamMode > 1) {
                        supportsTeam = true;
                        break;
                    }
                }
                mTeamRow.setVisibility(supportsTeam ? View.VISIBLE : View.GONE);

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
                } else {
                    Log.w(TAG, "No regions parsed from site_info.");
                    setStatus(L10n.t("index-no-regions", "No regions available from server"));
                }
                // Only reveal Play once site_info has supplied a usable mode and
                // region list (P1.7: no hard-coded "na" fallback).
                mSiteInfoLoaded = !mModes.isEmpty() && !mPops.isEmpty();
                mPlaySpinner.setVisibility(View.GONE);
                // The Java UI is built before the C++ locale/config handshake on
                // a cold start; site_info lands afterwards, so re-read the l10n
                // table here and re-render the menu chrome in the saved locale.
                L10n.invalidate();
                applyLocalization();
                if (mSiteInfoLoaded && mPops.size() > 0) {
                    setStatus("");
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

    /** site_info.modes -> the quick-play button rows (siteInfo.ts getGameModeStyles). */
    private void buildModeButtons() {
        mModeArea.removeAllViews();
        mModeButtons.clear();
        LinearLayout halfRow = null;
        for (int i = 0; i < mModes.size(); i++) {
            final Json.ModeEntry mode = mModes.get(i);
            final int modeIdx = i;
            String buttonText = modeButtonText(mode);
            String label = L10n.t("index-play-" + buttonText,
                    buttonText.length() > 0
                            ? "Play " + Character.toUpperCase(buttonText.charAt(0)) + buttonText.substring(1)
                            : "Play");
            TextView btn = makeButton(label, GREEN, GREEN_SHADOW, i == 0 ? 18 : 16);
            btn.setEnabled(mode.enabled);
            btn.setAlpha(mode.enabled ? 1.0f : 0.45f);
            btn.setOnClickListener(new View.OnClickListener() {
                @Override public void onClick(View v) {
                    if (!mode.enabled || !mSiteInfoLoaded) return;
                    animatePress(v);
                    onPlay(modeIdx);
                }
            });
            mModeButtons.add(btn);
            if (i == 0) {
                mModeArea.addView(btn, rowParams(dp(42)));
            } else {
                if (halfRow == null || halfRow.getChildCount() >= 2) {
                    halfRow = row(mActivity);
                    mModeArea.addView(halfRow, rowParams(dp(38)));
                }
                LinearLayout.LayoutParams lp = new LinearLayout.LayoutParams(
                        0, ViewGroup.LayoutParams.MATCH_PARENT, 1f);
                if (halfRow.getChildCount() == 0) {
                    lp.rightMargin = dp(4);
                } else {
                    lp.leftMargin = dp(4);
                }
                halfRow.addView(btn, lp);
            }
        }
    }

    /** MapDef.desc.buttonText, else GameConfig.TeamModeToString[teamMode]. */
    private static String modeButtonText(Json.ModeEntry mode) {
        if (mode.mapName != null && mode.mapName.startsWith("faction")) {
            return "50v50";
        }
        if (mode.teamMode == 1) return "solo";
        if (mode.teamMode == 2) return "duo";
        return "squad";
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
        // The first ping snapshot arrives after the C++ bridge/locale install;
        // use it as the reliable cold-start point to re-read l10n and, if the
        // early site_info request lost the startup race, retry it.
        L10n.invalidate();
        applyLocalization();
        if (mSiteInfoJson.isEmpty() && !mFindingGame) {
            fetchSiteInfo();
        }

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
    public String zonesJson(String region) {
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

    private void onPlay(final int gameModeIdx) {
        if (mFindingGame || !mSiteInfoLoaded) return;
        if (gameModeIdx < 0 || gameModeIdx >= mModes.size() || !mModes.get(gameModeIdx).enabled) {
            return;
        }
        mFindingGame = true;
        setPlayLockout(true);
        setStatus(L10n.t("index-finding-game", "Finding game..."));
        String region = "na";
        if (mSelectedRegionIdx >= 0 && mSelectedRegionIdx < mPops.size()) {
            region = mPops.get(mSelectedRegionIdx).region;
        }
        SurvBridge.setRegion(region);

        final String bodyStr = "{\"region\":\"" + escapeJson(region) + "\""
                + ",\"zones\":" + zonesJson(region)
                + ",\"version\":" + SurvBridge.getProtocolVersion()
                + ",\"playerCount\":1"
                + ",\"autoFill\":true"
                + ",\"gameModeIdx\":" + gameModeIdx
                + "}";
        Log.i(TAG, "onPlay find_game body=" + bodyStr);

        AccountManager account = AccountManager.peek();
        final String token = account != null ? account.getSessionToken() : "";
        findGameAttempt(bodyStr, token, 0);
    }

    /** Play buttons dim/disable and the loading spinner follows the request. */
    private void setPlayLockout(boolean lock) {
        for (TextView btn : mModeButtons) {
            int i = mModeButtons.indexOf(btn);
            boolean enabled = i >= 0 && i < mModes.size() && mModes.get(i).enabled;
            btn.setEnabled(!lock && enabled);
            btn.setAlpha(lock ? 0.5f : (enabled ? 1.0f : 0.45f));
        }
        mPlaySpinner.setVisibility(lock ? View.VISIBLE : View.GONE);
    }

    /** main.ts findGame(): 2 attempts, 500 ms apart, transport failures only. */
    private static final int FIND_GAME_MAX_ATTEMPTS = 2;
    private static final long FIND_GAME_RETRY_DELAY_MS = 500;

    private void findGameAttempt(final String bodyStr, final String authToken, final int attempt) {
        SurvApi.findGame(mApiBaseUrl, bodyStr, authToken, new SurvApi.Callback() {
            @Override public void onResult(String json) {
                if (json == null) {
                    // Transport failure: retry like `findGameImpl`'s catch()
                    // before surfacing the error to the player.
                    if (attempt + 1 < FIND_GAME_MAX_ATTEMPTS) {
                        Log.i(TAG, "find_game transport failure, retrying after "
                                + FIND_GAME_RETRY_DELAY_MS + "ms (attempt " + (attempt + 2) + "/"
                                + FIND_GAME_MAX_ATTEMPTS + ")");
                        new Handler(Looper.getMainLooper()).postDelayed(new Runnable() {
                            @Override public void run() {
                                findGameAttempt(bodyStr, authToken, attempt + 1);
                            }
                        }, FIND_GAME_RETRY_DELAY_MS);
                        return;
                    }
                    mFindingGame = false;
                    setPlayLockout(false);
                    setStatus(l10n("index-failed-finding-game", "Failed finding game."));
                    return;
                }
                mFindingGame = false;
                String type = Json.peekString(json, "type");
                if ("success".equals(type)) {
                    List<String> urls = parseUrls(json);
                    String joinToken = parseJoinToken(json);
                    if (urls.size() > 0 && joinToken != null) {
                        String[] urlArray = urls.toArray(new String[urls.size()]);
                        SurvBridge.onJoinUrls(urlArray, joinToken);
                        setStatus(l10n("index-joining-game", "Joining game"));
                        return;
                    }
                }
                setPlayLockout(false);
                if ("banned".equals(type)) {
                    showIpBan(json);
                    return;
                }
                String error = Json.peekString(json, "error");
                if (error == null) error = "find_game_failed";
                handleFindGameError(error);
            }
        });
    }

    /** main.ts getErrorString(): find_game error -> l10n key. */
    private static String findGameErrorKey(String error) {
        if ("behind_proxy".equals(error)) return "index-behind-proxy";
        if ("find_game_failed".equals(error) || "full".equals(error)) {
            return "index-failed-finding-game";
        }
        if ("invalid_captcha".equals(error)) return "index-invalid-captcha";
        if ("invalid_protocol".equals(error)) return "index-invalid-protocol";
        if ("invalid_region".equals(error)) return "index-invalid-region";
        if ("join_game_failed".equals(error)) return "index-failed-joining-game";
        if ("mode_disabled".equals(error)) return "index-mode-disabled";
        if ("rate_limited".equals(error)) return "index-rate-limited";
        return "index-failed-finding-game";
    }

    private void handleFindGameError(String error) {
        String text = l10n(findGameErrorKey(error), error);
        if ("behind_proxy".equals(error) || "invalid_protocol".equals(error)) {
            showErrorModal(l10n("index-error", "Error"), text);
        } else if ("invalid_captcha".equals(error)) {
            AccountManager account = AccountManager.peek();
            boolean signedIn = account != null && account.isLoggedIn();
            String extra = signedIn ? ""
                    : "\n\nSign in from the account button to play on this server.";
            showErrorModal(l10n("index-captcha", "Captcha required"), text + extra);
        } else {
            setStatus(text);
        }
    }

    private void showIpBan(String json) {
        String reason = Json.peekString(json, "reason");
        boolean permanent = Json.peekBool(json, "permanent", false);
        String expiresIn = Json.peekString(json, "expiresIn");
        StringBuilder message = new StringBuilder(l10n("index-ip-banned", "Your IP has been banned."));
        if (reason != null && !reason.isEmpty()) {
            message.append("\n\nReason: ").append(reason);
        }
        message.append("\nDuration: ")
                .append(permanent ? "indefinite"
                        : (expiresIn == null || expiresIn.isEmpty() ? "temporary" : expiresIn));
        showErrorModal(l10n("index-banned", "Banned"), message.toString());
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

    /** Last /api/site_info payload ("" until the first successful fetch). */
    public String siteInfoJson() { return mSiteInfoJson; }

    public void show() {
        mRoot.setVisibility(View.VISIBLE);
        mFindingGame = false;
        setPlayLockout(false);
        refresh();
    }

    public void hide() {
        mRoot.setVisibility(View.GONE);
    }

    public void refresh() {
        // The settings screen changes the language behind this menu: drop the
        // cached table and re-render every label from the active locale.
        L10n.invalidate();
        applyLocalization();
        String config = SurvBridge.getConfigJson();
        String name = Json.peekString(config, "playerName");
        if (name != null && !name.isEmpty() && !mName.hasFocus()) {
            mName.setText(name);
        }
        mMuted = Json.peekBool(config, "muteAudio", false);
        updateMuteButton();
        AccountManager account = AccountManager.peek();
        if (account != null) {
            updateAccountButton(account.getProfile());
        }
    }

    /** Re-applies the l10n table to the always-visible menu chrome. */
    private void applyLocalization() {
        mName.setHint(L10n.t("index-enter-name-here", "Enter your name here"));
        mJoinTeamBtn.setText(L10n.t("index-join-team", "Join Team"));
        mCreateTeamBtn.setText(L10n.t("index-create-team", "Create Team"));
        mHelpBtn.setText("\uD83D\uDCD6 " + L10n.t("index-how-to-play", "How to Play"));
        mPassBtn.setText("\uD83C\uDFC6 " + L10n.t("index-pass", "Pass"));
        mNewsBtn.setText((mNewsExpanded ? "\u25B2 " : "\uD83D\uDCF0 ")
                + L10n.t("index-news", "News"));
        mSettingsBtn.setContentDescription(L10n.t("index-settings", "Settings"));
        mProfileBtn.setContentDescription(L10n.t("index-account", "Account"));
        if (mPops.isEmpty()) {
            mRegionLabel.setText(L10n.t("index-region", "Region"));
        } else {
            showRegion();
            buildRegionDropdown();
        }
        if (mSiteInfoLoaded) {
            buildModeButtons();
        }
    }

    /** AccountManager -> menu chrome (called on the UI thread). */
    public void onAccountChanged(AccountManager.Profile profile) {
        updateAccountButton(profile);
    }

    public void onAccountError(String error) {
        Log.w(TAG, "account error " + error);
        if ("account_offline".equals(error)) {
            setStatus("Account: cannot reach the server");
        } else if ("account_banned".equals(error)) {
            showErrorModal(l10n("index-account", "Account"),
                    l10n("index-account-banned", "Your account has been banned."));
        }
    }

    /**
     * The account name doubles as the default in-game name (web `playerName`
     * stays an independent local setting, so only fill it when it is empty).
     */
    private void updateAccountButton(AccountManager.Profile profile) {
        if (mProfileBtn == null || profile == null) {
            return;
        }
        mProfileBtn.setTextColor(profile.loggedIn ? 0xFF7FD858 : 0xFFAAAAAA);
        mProfileBtn.setContentDescription(profile.loggedIn
                ? ("Account: " + profile.username)
                : "Account: not signed in");

        AccountManager account = AccountManager.peek();
        if (account == null) {
            return;
        }
        if (profile.loggedIn && profile.usernameSet) {
            String current = mName.getText().toString().trim();
            String bound = account.getBoundPlayerName();
            // The field follows the account until the player edits it by hand
            // (web keeps `playerName` separate, but an auto-created account has
            // no other name to use).
            boolean follows = current.isEmpty() ? bound.isEmpty() : current.equals(bound);
            if (!mName.hasFocus() && follows && !current.equals(profile.username)) {
                mName.setText(profile.username);
                SurvBridge.setPlayerName(profile.username);
            }
            if (follows) {
                account.bindPlayerName(profile.username);
            }
        }
    }

    public void showMessage(String text) {
        setStatus(text);
    }

    /** Localized text for an l10n key, with a fallback for missing entries. */
    public String l10n(String key, String fallback) {
        String text = Json.peekString(SurvBridge.getL10nJson(), key);
        return text != null && !text.isEmpty() ? text : fallback;
    }

    /** A blocking error dialog (P1.2: invalid protocol/IP ban/behind proxy). */
    public void showErrorModal(String title, String message) {
        new AlertDialog.Builder(mActivity)
                .setTitle(title)
                .setMessage(message)
                .setPositiveButton("OK", null)
                .show();
    }

    /** Error from a C++ l10n key (invalid protocol/IP ban/behind proxy). */
    public void showErrorKey(String key) {
        showErrorModal(l10n("index-error", "Error"), l10n(key, key));
    }

    // ===================== Types ===========================================

    static class PopEntry {
        String region; int playerCount; String l10n;
    }

    // ===================== JSON Parsing ====================================

    private String parseNewsText(String json) {
        if (json == null) return null;
        StringBuilder out = new StringBuilder();
        try {
            com.google.gson.JsonArray arr = com.google.gson.JsonParser.parseString(json).getAsJsonArray();
            for (int i = 0; i < arr.size(); i++) {
                com.google.gson.JsonElement el = arr.get(i);
                if (el == null || !el.isJsonObject()) continue;
                com.google.gson.JsonObject entry = el.getAsJsonObject();
                String title = entry.has("title") ? entry.get("title").getAsString() : "";
                String date = entry.has("date") ? entry.get("date").getAsString() : "";
                if (out.length() > 0) out.append("\n\n");
                out.append("\u2726 ");
                if (!date.isEmpty()) out.append(date).append("  ");
                out.append(title);
                if (entry.has("paragraphs") && entry.get("paragraphs").isJsonArray()) {
                    com.google.gson.JsonArray paragraphs = entry.getAsJsonArray("paragraphs");
                    for (int j = 0; j < paragraphs.size(); j++) {
                        com.google.gson.JsonElement pEl = paragraphs.get(j);
                        if (pEl == null || !pEl.isJsonObject()) continue;
                        com.google.gson.JsonObject paragraph = pEl.getAsJsonObject();
                        if (!paragraph.has("text")) continue;
                        String text = paragraph.get("text").getAsString();
                        if (text.isEmpty()) continue;
                        out.append("\n   ");
                        out.append(text.length() > 140 ? text.substring(0, 140) + "..." : text);
                    }
                }
                // The menu box is short; the two newest entries are enough.
                if (i >= 1) break;
            }
        } catch (Exception e) {
            Log.w(TAG, "parseNewsText failed: " + e.getMessage());
            return null;
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