/****************************************************************************
 * SettingsModal.java — Full settings screen matching web client #modal-settings.
 *
 * Enhanced settings with:
 *   - Language selector (cycle button)
 *   - Audio: real SeekBar sliders for Master/SFX/Music volume
 *   - Graphics: high-res textures, interpolation, local rotation, screen shake
 *   - Controls: touch move/aim style, aim line
 *   - Game: client theme, anonymous player names, HUD scale
 *   - Links: attributions, hall of fame, changelog
 *   - Reset to defaults functionality
 *   - Immediate config save on change
 *
 * Colors and layout match the web client CSS.
 ****************************************************************************/
package dev.axmol.app;

import android.app.Activity;
import android.content.Intent;
import android.graphics.Color;
import android.graphics.drawable.GradientDrawable;
import android.net.Uri;
import android.util.Log;
import android.util.TypedValue;
import android.view.Gravity;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.CheckBox;
import android.widget.CompoundButton;
import android.widget.FrameLayout;
import android.widget.LinearLayout;
import android.widget.SeekBar;
import android.widget.ScrollView;
import android.widget.TextView;

import java.util.ArrayList;
import java.util.List;

public class SettingsModal {
    private static final String TAG = "SettingsModal";

    // Web client palette
    private static final int DARK        = 0xFF7A7A7A;
    private static final int DARK_SHADOW = 0xFF3E3E3E;
    private static final int GREEN       = 0xFF83AF50;
    private static final int GREEN_SHADOW = 0xFF5B7A38;
    private static final int PANEL_BG    = 0xF0000000; // more opaque

    private final Activity mActivity;
    private final FrameLayout mRoot;
    private boolean mOpen = false;
    private Runnable mOnHidden = null;

    // --- widgets ---
    private final Button mLanguageBtn;
    private final SeekBar mMasterSlider;
    private final TextView mMasterLabel;
    private final SeekBar mSfxSlider;
    private final TextView mSfxLabel;
    private final SeekBar mMusicSlider;
    private final TextView mMusicLabel;
    private final CheckBox mInterpolation;
    private final CheckBox mScreenShake;
    private final CheckBox mAnonNames;
    private final Button mMoveStyleBtn;
    private final Button mAimStyleBtn;
    private final Button mAimLineBtn;
    private final TextView mStatusText;
    private final Button mCloseBtn;
    private final Button mResetBtn;

    // ui::kLocales, fetched from the C++ bridge (localization.ts Locales).
    private List<Json.LocaleEntry> mLocales = new ArrayList<>();
    private int mLocaleIdx = 0;
    private boolean mMoveLocked = false;
    private boolean mAimLocked = false;
    private boolean mAimLine = true;

    public SettingsModal(Activity activity) {
        mActivity = activity;
        mRoot = new FrameLayout(activity);
        mRoot.setBackgroundColor(0xCC000000); // dim overlay

        // Full-height ScrollView for landscape responsiveness
        ScrollView outerScroll = new ScrollView(activity);
        outerScroll.setLayoutParams(new FrameLayout.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.MATCH_PARENT));

        // Main panel: translucent dark block, centered
        LinearLayout panel = new LinearLayout(activity);
        panel.setOrientation(LinearLayout.VERTICAL);
        panel.setGravity(Gravity.CENTER);
        panel.setBackgroundColor(PANEL_BG);
        panel.setPadding(dp(20), dp(14), dp(20), dp(14));

        // Title row with reset button
        LinearLayout titleRow = new LinearLayout(activity);
        titleRow.setOrientation(LinearLayout.HORIZONTAL);
        
        TextView title = new TextView(activity);
        title.setText("\u2699 " + L10n.t("index-settings", "Settings"));
        title.setTextColor(Color.WHITE);
        title.setTextSize(TypedValue.COMPLEX_UNIT_SP, 20);
        title.setGravity(Gravity.CENTER);
        title.setLayoutParams(new LinearLayout.LayoutParams(0, ViewGroup.LayoutParams.WRAP_CONTENT, 1f));
        titleRow.addView(title);

        mResetBtn = makeSmallButton("Reset");
        mResetBtn.setOnClickListener(new View.OnClickListener() {
            @Override public void onClick(View v) { resetToDefaults(); }
        });
        titleRow.addView(mResetBtn);
        panel.addView(titleRow);

        // -- Scrollable content (web #modal-settings body) --
        ScrollView scroll = new ScrollView(activity);
        scroll.setLayoutParams(new FrameLayout.LayoutParams(dp(380), dp(520)));

        LinearLayout content = new LinearLayout(activity);
        content.setOrientation(LinearLayout.VERTICAL);
        content.setPadding(dp(10), 0, dp(10), 0);

        // === Language (web: language select dropdown) ===
        addSectionHeader(content, "Language");
        mLocales = Json.parseLocales(SurvBridge.getLocalesJson());
        if (mLocales.isEmpty()) {
            Json.LocaleEntry en = new Json.LocaleEntry();
            en.code = "en";
            en.name = "English";
            mLocales.add(en);
        }
        mLanguageBtn = makeToggleButton("English");
        mLanguageBtn.setOnClickListener(new View.OnClickListener() {
            @Override public void onClick(View v) {
                mLocaleIdx = (mLocaleIdx + 1) % mLocales.size();
                Json.LocaleEntry locale = mLocales.get(mLocaleIdx);
                mLanguageBtn.setText(locale.name);
                SurvBridge.setLanguage(locale.code);
                SurvBridge.saveConfig();
                L10n.invalidate();
                mStatusText.setText("");
            }
        });
        content.addView(mLanguageBtn);
        addGap(content, dp(6));

        // === Audio section with real sliders ===
        addSectionHeader(content, "\uD83C\uDFB5 Audio");

        // Master Volume slider
        LinearLayout masterRow = makeSliderRow(L10n.t("index-master-volume", "Master Volume"), 100);
        mMasterLabel = (TextView) masterRow.getChildAt(1);
        mMasterSlider = (SeekBar) masterRow.getChildAt(2);
        mMasterSlider.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {
            @Override public void onProgressChanged(SeekBar sb, int progress, boolean fromUser) {
                float val = progress / 100f;
                mMasterLabel.setText(String.format("%.0f%%", val * 100));
                if (fromUser) {
                    SurvBridge.setAudio("master", val);
                    SurvBridge.saveConfig();
                }
            }
            @Override public void onStartTrackingTouch(SeekBar sb) {}
            @Override public void onStopTrackingTouch(SeekBar sb) {}
        });
        content.addView(masterRow);

        // SFX Volume slider
        LinearLayout sfxRow = makeSliderRow(L10n.t("index-sfx-volume", "SFX Volume"), 100);
        mSfxLabel = (TextView) sfxRow.getChildAt(1);
        mSfxSlider = (SeekBar) sfxRow.getChildAt(2);
        mSfxSlider.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {
            @Override public void onProgressChanged(SeekBar sb, int progress, boolean fromUser) {
                float val = progress / 100f;
                mSfxLabel.setText(String.format("%.0f%%", val * 100));
                if (fromUser) {
                    SurvBridge.setAudio("sound", val);
                    SurvBridge.saveConfig();
                }
            }
            @Override public void onStartTrackingTouch(SeekBar sb) {}
            @Override public void onStopTrackingTouch(SeekBar sb) {}
        });
        content.addView(sfxRow);

        // Music Volume slider
        LinearLayout musicRow = makeSliderRow(L10n.t("index-music-volume", "Music Volume"), 100);
        mMusicLabel = (TextView) musicRow.getChildAt(1);
        mMusicSlider = (SeekBar) musicRow.getChildAt(2);
        mMusicSlider.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {
            @Override public void onProgressChanged(SeekBar sb, int progress, boolean fromUser) {
                float val = progress / 100f;
                mMusicLabel.setText(String.format("%.0f%%", val * 100));
                if (fromUser) {
                    SurvBridge.setAudio("music", val);
                    SurvBridge.saveConfig();
                }
            }
            @Override public void onStartTrackingTouch(SeekBar sb) {}
            @Override public void onStopTrackingTouch(SeekBar sb) {}
        });
        content.addView(musicRow);

        addGap(content, dp(6));

        // === Graphics section ===
        // `highResTex` has no native equivalent: the C++ client ships only the
        // low-res atlases and quality is controlled by the GFX surface tier in
        // the pause dialog (RenderScale), so the control is absent.
        addSectionHeader(content, "\uD83D\uDCF8 Graphics");

        mInterpolation = makeCheckbox(L10n.t("index-client-side-interp", "Client side interpolation"), true);
        mInterpolation.setOnCheckedChangeListener(new CompoundButton.OnCheckedChangeListener() {
            @Override public void onCheckedChanged(CompoundButton b, boolean checked) {
                SurvBridge.setConfigValue("interpolation", checked ? "true" : "false");
                SurvBridge.saveConfig();
            }
        });
        content.addView(mInterpolation);

        mScreenShake = makeCheckbox(L10n.t("index-screen-shake", "Screen shake"), true);
        mScreenShake.setOnCheckedChangeListener(new CompoundButton.OnCheckedChangeListener() {
            @Override public void onCheckedChanged(CompoundButton b, boolean checked) {
                SurvBridge.setConfigValue("screenShake", checked ? "true" : "false");
                SurvBridge.saveConfig();
            }
        });
        content.addView(mScreenShake);
        
        addGap(content, dp(6));

        // === Controls section ===
        addSectionHeader(content, "\u270B Controls");

        mMoveStyleBtn = makeToggleButton("Move Style: Anywhere");
        mMoveStyleBtn.setOnClickListener(new View.OnClickListener() {
            @Override public void onClick(View v) {
                mMoveLocked = !mMoveLocked;
                applyTouchStyleButtons();
                SurvBridge.setConfigValue("touchMoveStyle", mMoveLocked ? "locked" : "anywhere");
                SurvBridge.saveConfig();
            }
        });
        content.addView(mMoveStyleBtn);

        mAimStyleBtn = makeToggleButton("Aim Style: Anywhere");
        mAimStyleBtn.setOnClickListener(new View.OnClickListener() {
            @Override public void onClick(View v) {
                mAimLocked = !mAimLocked;
                applyTouchStyleButtons();
                SurvBridge.setConfigValue("touchAimStyle", mAimLocked ? "locked" : "anywhere");
                SurvBridge.saveConfig();
            }
        });
        content.addView(mAimStyleBtn);

        mAimLineBtn = makeToggleButton("Aim Line: On");
        mAimLineBtn.setOnClickListener(new View.OnClickListener() {
            @Override public void onClick(View v) {
                mAimLine = !mAimLine;
                applyTouchStyleButtons();
                SurvBridge.setConfigValue("touchAimLine", mAimLine ? "true" : "false");
                SurvBridge.saveConfig();
            }
        });
        content.addView(mAimLineBtn);
        
        addGap(content, dp(6));

        // === Game section ===
        addSectionHeader(content, "\uD83C\uDFAE Game");

        mAnonNames = makeCheckbox(L10n.t("index-anon-player-names", "Anonymize player names"), false);
        mAnonNames.setOnCheckedChangeListener(new CompoundButton.OnCheckedChangeListener() {
            @Override public void onCheckedChanged(CompoundButton b, boolean checked) {
                SurvBridge.setConfigValue("anonPlayerNames", checked ? "true" : "false");
                SurvBridge.saveConfig();
            }
        });
        content.addView(mAnonNames);

        // === Links ===
        addGap(content, dp(10));
        Button leaderboardBtn = makeLinkButton("\uD83C\uDFC6 " + L10n.t("index-leaderboards", "Leaderboards"));
        leaderboardBtn.setOnClickListener(new View.OnClickListener() {
            @Override public void onClick(View v) { openWebPage("/stats"); }
        });
        content.addView(leaderboardBtn);

        Button attribBtn = makeLinkButton("\u00A9 " + L10n.t("index-attributions", "attributions"));
        attribBtn.setOnClickListener(new View.OnClickListener() {
            @Override public void onClick(View v) { openWebPage("/attribution.txt"); }
        });
        content.addView(attribBtn);

        Button hofBtn = makeLinkButton("\uD83C\uDFC6 " + L10n.t("index-hof", "Hall of Fame"));
        hofBtn.setOnClickListener(new View.OnClickListener() {
            @Override public void onClick(View v) { openWebPage("/hof.html"); }
        });
        content.addView(hofBtn);

        Button changelogBtn = makeLinkButton("\uD83D\uDCDD " + L10n.t("index-original-changelog", "Changelog"));
        changelogBtn.setOnClickListener(new View.OnClickListener() {
            @Override public void onClick(View v) { openWebPage("/changelogRec.html"); }
        });
        content.addView(changelogBtn);

        scroll.addView(content);
        panel.addView(scroll);

        // === Bottom: status + close ===
        mStatusText = new TextView(activity);
        mStatusText.setTextColor(0xFF808080);
        mStatusText.setTextSize(TypedValue.COMPLEX_UNIT_SP, 12);
        mStatusText.setGravity(Gravity.CENTER);
        panel.addView(mStatusText);

        mCloseBtn = makeGreenButton("Close \u2715");
        mCloseBtn.setOnClickListener(new View.OnClickListener() {
            @Override public void onClick(View v) { hide(); }
        });
        panel.addView(mCloseBtn);

        outerScroll.addView(panel);

        mRoot.addView(outerScroll);
    }

    private void addSectionHeader(LinearLayout parent, String text) {
        TextView h = new TextView(mActivity);
        h.setText(text);
        h.setTextColor(0xFFFFD700); // gold (#ffd700)
        h.setTextSize(TypedValue.COMPLEX_UNIT_SP, 15);
        h.setTypeface(android.graphics.Typeface.DEFAULT_BOLD);
        h.setPadding(0, dp(10), 0, dp(4));
        parent.addView(h);
    }

    private void addGap(LinearLayout parent, int height) {
        View gap = new View(mActivity);
        gap.setLayoutParams(new ViewGroup.LayoutParams(1, height));
        parent.addView(gap);
    }

    /**
     * Creates a slider row: Label | Percentage value | SeekBar
     */
    private LinearLayout makeSliderRow(String label, int defaultValue) {
        LinearLayout row = new LinearLayout(mActivity);
        row.setOrientation(LinearLayout.HORIZONTAL);
        row.setGravity(Gravity.CENTER_VERTICAL);
        row.setPadding(0, dp(3), 0, dp(3));

        TextView lbl = new TextView(mActivity);
        lbl.setText(label);
        lbl.setTextColor(Color.WHITE);
        lbl.setTextSize(TypedValue.COMPLEX_UNIT_SP, 13);
        lbl.setLayoutParams(new LinearLayout.LayoutParams(dp(70), ViewGroup.LayoutParams.WRAP_CONTENT));
        row.addView(lbl);

        TextView val = new TextView(mActivity);
        val.setText(defaultValue + "%");
        val.setTextColor(GREEN);
        val.setTextSize(TypedValue.COMPLEX_UNIT_SP, 12);
        val.setLayoutParams(new LinearLayout.LayoutParams(dp(50), ViewGroup.LayoutParams.WRAP_CONTENT));
        row.addView(val);

        SeekBar slider = new SeekBar(mActivity);
        slider.setMax(100);
        slider.setProgress(defaultValue);
        slider.setLayoutParams(new LinearLayout.LayoutParams(
                0, ViewGroup.LayoutParams.WRAP_CONTENT, 1f));
        
        // Removed SeekBar styling that uses unavailable API
        
        row.addView(slider);

        return row;
    }

    private Button makeToggleButton(String label) {
        Button btn = new Button(mActivity);
        btn.setText(label);
        btn.setBackgroundColor(DARK);
        btn.setLayoutParams(new ViewGroup.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT, dp(36)));
        btn.setTextColor(Color.WHITE);
        btn.setTextSize(TypedValue.COMPLEX_UNIT_SP, 13);
        btn.setGravity(Gravity.CENTER);
        btn.setPadding(dp(12), dp(6), dp(12), dp(6));
        return btn;
    }

    private CheckBox makeCheckbox(String label, boolean def) {
        CheckBox cb = new CheckBox(mActivity);
        cb.setText(label);
        cb.setChecked(def);
        cb.setTextColor(Color.WHITE);
        cb.setTextSize(TypedValue.COMPLEX_UNIT_SP, 13);
        cb.setPadding(dp(4), dp(4), dp(4), dp(4));
        return cb;
    }

    private Button makeGreenButton(String label) {
        Button btn = new Button(mActivity);
        btn.setText(label);
        btn.setBackgroundColor(GREEN);
        btn.setLayoutParams(new ViewGroup.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT, dp(44)));
        btn.setGravity(Gravity.CENTER);
        btn.setTextColor(Color.WHITE);
        btn.setTextSize(TypedValue.COMPLEX_UNIT_SP, 16);
        btn.setPadding(dp(12), dp(8), dp(12), dp(8));
        return btn;
    }

    private Button makeLinkButton(String label) {
        Button btn = new Button(mActivity);
        btn.setText(label);
        btn.setBackgroundColor(DARK);
        btn.setLayoutParams(new ViewGroup.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT, dp(34)));
        btn.setGravity(Gravity.CENTER);
        btn.setTextColor(0xFF1E90FF); // blue link color
        btn.setTextSize(TypedValue.COMPLEX_UNIT_SP, 13);
        btn.setPadding(dp(12), dp(6), dp(12), dp(6));
        return btn;
    }

    /**
     * Opens a web-only page (attribution/HOF/changelog). P1.8 will route this
     * through the config `webBaseUrl`; the store origin is the same for now.
     */
    private void openWebPage(String path) {
        try {
            Intent intent = new Intent(Intent.ACTION_VIEW,
                    Uri.parse(TeamLinks.webBaseUrl() + path));
            mActivity.startActivity(intent);
        } catch (Exception e) {
            Log.w(TAG, "No browser for " + path + ": " + e.getMessage());
            mStatusText.setText(L10n.t("index-error", "Error") + ": " + path);
        }
    }
    
    private Button makeSmallButton(String label) {
        Button btn = new Button(mActivity);
        btn.setText(label);
        btn.setBackgroundColor(0xFF666666);
        btn.setTextSize(TypedValue.COMPLEX_UNIT_SP, 11);
        btn.setPadding(dp(8), dp(2), dp(8), dp(2));
        return btn;
    }

    // ===== Public API =====

    public View root() { return mRoot; }

    public void show() {
        mOpen = true;
        mRoot.setVisibility(View.VISIBLE);
        syncFromConfig();
    }

    public void hide() {
        mOpen = false;
        mRoot.setVisibility(View.GONE);
        if (mOnHidden != null) mOnHidden.run();
    }

    public boolean isOpen() { return mOpen; }

    /** Called after hide() (menu re-localizes when the language changed). */
    public void setOnHidden(Runnable onHidden) { mOnHidden = onHidden; }

    private void applyTouchStyleButtons() {
        mMoveStyleBtn.setText(mMoveLocked ? "Move Style: Locked" : "Move Style: Anywhere");
        mAimStyleBtn.setText(mAimLocked ? "Aim Style: Locked" : "Aim Style: Anywhere");
        mAimLineBtn.setText(mAimLine ? "Aim Line: On" : "Aim Line: Off");
    }

    /** Reset all settings to default values */
    private void resetToDefaults() {
        // Language (web default is English, which is not the first Locales entry)
        mLocaleIdx = 0;
        for (int i = 0; i < mLocales.size(); i++) {
            if ("en".equals(mLocales.get(i).code)) {
                mLocaleIdx = i;
                break;
            }
        }
        if (!mLocales.isEmpty()) {
            mLanguageBtn.setText(mLocales.get(mLocaleIdx).name);
        }
        SurvBridge.setLanguage("en");
        L10n.invalidate();

        // Volumes (web defaults: master 1, sound 1, music 1).
        mMasterSlider.setProgress(100);
        mSfxSlider.setProgress(100);
        mMusicSlider.setProgress(100);
        mMasterLabel.setText("100%");
        mSfxLabel.setText("100%");
        mMusicLabel.setText("100%");
        SurvBridge.setAudio("master", 1.0f);
        SurvBridge.setAudio("sound", 1.0f);
        SurvBridge.setAudio("music", 1.0f);

        // Graphics
        mInterpolation.setChecked(true);
        mScreenShake.setChecked(true);
        mAnonNames.setChecked(false);
        SurvBridge.setConfigValue("interpolation", "true");
        SurvBridge.setConfigValue("screenShake", "true");
        SurvBridge.setConfigValue("anonPlayerNames", "false");

        // Controls (config defaults are "anywhere" + aim line on)
        mMoveLocked = false;
        mAimLocked = false;
        mAimLine = true;
        applyTouchStyleButtons();
        SurvBridge.setConfigValue("touchMoveStyle", "anywhere");
        SurvBridge.setConfigValue("touchAimStyle", "anywhere");
        SurvBridge.setConfigValue("touchAimLine", "true");

        SurvBridge.saveConfig();
        mStatusText.setText("Settings reset to defaults");

        Log.i(TAG, "Settings reset to defaults");
    }

    public void syncFromConfig() {
        String config = SurvBridge.getConfigJson();
        if (config == null || config.isEmpty()) return;

        // Language (names come from ui::kLocales via the bridge)
        String lang = Json.peekString(config, "language");
        if (lang != null) {
            for (int i = 0; i < mLocales.size(); i++) {
                if (mLocales.get(i).code.equals(lang)) {
                    mLocaleIdx = i;
                    mLanguageBtn.setText(mLocales.get(i).name);
                    break;
                }
            }
        }

        // Volume sliders: the config stores numbers, not strings.
        float master = Json.peekFloat(config, "masterVolume", 1.0f);
        mMasterSlider.setProgress(Math.round(master * 100));
        mMasterLabel.setText(String.format("%.0f%%", master * 100));
        float sfx = Json.peekFloat(config, "soundVolume", 1.0f);
        mSfxSlider.setProgress(Math.round(sfx * 100));
        mSfxLabel.setText(String.format("%.0f%%", sfx * 100));
        float music = Json.peekFloat(config, "musicVolume", 1.0f);
        mMusicSlider.setProgress(Math.round(music * 100));
        mMusicLabel.setText(String.format("%.0f%%", music * 100));

        // Touch style / aim line (booleans, not strings)
        mMoveLocked = "locked".equals(Json.peekString(config, "touchMoveStyle"));
        mAimLocked = "locked".equals(Json.peekString(config, "touchAimStyle"));
        mAimLine = Json.peekBool(config, "touchAimLine", true);
        applyTouchStyleButtons();

        // Graphics checkboxes
        mInterpolation.setChecked(Json.peekBool(config, "interpolation", true));
        mScreenShake.setChecked(Json.peekBool(config, "screenShake", true));
        mAnonNames.setChecked(Json.peekBool(config, "anonPlayerNames", false));

        mStatusText.setText("");
    }

    private int dp(int value) {
        return (int) (value * mActivity.getResources().getDisplayMetrics().density + 0.5f);
    }
}
