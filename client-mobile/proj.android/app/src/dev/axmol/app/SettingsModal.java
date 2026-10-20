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
import android.graphics.Color;
import android.graphics.drawable.GradientDrawable;
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

    // --- widgets ---
    private final Button mLanguageBtn;
    private final SeekBar mMasterSlider;
    private final TextView mMasterLabel;
    private final SeekBar mSfxSlider;
    private final TextView mSfxLabel;
    private final SeekBar mMusicSlider;
    private final TextView mMusicLabel;
    private final CheckBox mHighRes;
    private final CheckBox mInterpolation;
    private final CheckBox mLocalRotation;
    private final CheckBox mScreenShake;
    private final CheckBox mAnonNames;
    private final Button mMoveStyleBtn;
    private final Button mAimStyleBtn;
    private final Button mAimLineBtn;
    private final Button mHudScaleBtn;
    private final TextView mThemeLabel;
    private final TextView mStatusText;
    private final Button mCloseBtn;
    private final Button mResetBtn;

    private String[] mLocales = {"en", "ru", "de", "fr", "es", "pt", "pl", "tr", "zh", "ja"};
    private int mLocaleIdx = 0;
    
    // HUD scale options
    private String[] mHudScales = {"Small", "Normal", "Large"};
    private int mHudScaleIdx = 1;

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
        title.setText("\u2699 Settings");
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
        mLanguageBtn = makeToggleButton("EN");
        mLanguageBtn.setOnClickListener(new View.OnClickListener() {
            @Override public void onClick(View v) {
                mLocaleIdx = (mLocaleIdx + 1) % mLocales.length;
                mLanguageBtn.setText(mLocales[mLocaleIdx].toUpperCase());
                SurvBridge.setLanguage(mLocales[mLocaleIdx]);
                SurvBridge.saveConfig();
            }
        });
        content.addView(mLanguageBtn);
        addGap(content, dp(6));

        // === Audio section with real sliders ===
        addSectionHeader(content, "\uD83C\uDFB5 Audio");

        // Master Volume slider
        LinearLayout masterRow = makeSliderRow("Master", 100);
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
        LinearLayout sfxRow = makeSliderRow("SFX", 100);
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
        LinearLayout musicRow = makeSliderRow("Music", 100);
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
        addSectionHeader(content, "\uD83D\uDCF8 Graphics");

        mHighRes = makeCheckbox("High Resolution Textures", true);
        mHighRes.setOnCheckedChangeListener(new CompoundButton.OnCheckedChangeListener() {
            @Override public void onCheckedChanged(CompoundButton b, boolean checked) {
                SurvBridge.saveConfig();
            }
        });
        content.addView(mHighRes);
        
        mInterpolation = makeCheckbox("Client-side Interpolation", true);
        mInterpolation.setOnCheckedChangeListener(new CompoundButton.OnCheckedChangeListener() {
            @Override public void onCheckedChanged(CompoundButton b, boolean checked) {
                SurvBridge.saveConfig();
            }
        });
        content.addView(mInterpolation);
        
        mLocalRotation = makeCheckbox("Local Player Rotation", true);
        mLocalRotation.setOnCheckedChangeListener(new CompoundButton.OnCheckedChangeListener() {
            @Override public void onCheckedChanged(CompoundButton b, boolean checked) {
                SurvBridge.saveConfig();
            }
        });
        content.addView(mLocalRotation);
        
        mScreenShake = makeCheckbox("Screen Shake Effects", true);
        mScreenShake.setOnCheckedChangeListener(new CompoundButton.OnCheckedChangeListener() {
            @Override public void onCheckedChanged(CompoundButton b, boolean checked) {
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
                String cur = mMoveStyleBtn.getText().toString();
                if (cur.contains("Locked")) mMoveStyleBtn.setText("Move Style: Anywhere");
                else mMoveStyleBtn.setText("Move Style: Locked");
                SurvBridge.saveConfig();
            }
        });
        content.addView(mMoveStyleBtn);

        mAimStyleBtn = makeToggleButton("Aim Style: Anywhere");
        mAimStyleBtn.setOnClickListener(new View.OnClickListener() {
            @Override public void onClick(View v) {
                String cur = mAimStyleBtn.getText().toString();
                if (cur.contains("Locked")) mAimStyleBtn.setText("Aim Style: Anywhere");
                else mAimStyleBtn.setText("Aim Style: Locked");
                SurvBridge.saveConfig();
            }
        });
        content.addView(mAimStyleBtn);

        mAimLineBtn = makeToggleButton("Aim Line: On");
        mAimLineBtn.setOnClickListener(new View.OnClickListener() {
            @Override public void onClick(View v) {
                String cur = mAimLineBtn.getText().toString();
                if (cur.contains("On")) mAimLineBtn.setText("Aim Line: Off");
                else mAimLineBtn.setText("Aim Line: On");
                SurvBridge.saveConfig();
            }
        });
        content.addView(mAimLineBtn);
        
        mHudScaleBtn = makeToggleButton("HUD Scale: Normal");
        mHudScaleBtn.setOnClickListener(new View.OnClickListener() {
            @Override public void onClick(View v) {
                mHudScaleIdx = (mHudScaleIdx + 1) % mHudScales.length;
                mHudScaleBtn.setText("HUD Scale: " + mHudScales[mHudScaleIdx]);
                SurvBridge.saveConfig();
            }
        });
        content.addView(mHudScaleBtn);
        
        addGap(content, dp(6));

        // === Game section ===
        addSectionHeader(content, "\uD83C\uDFAE Game");

        mThemeLabel = new TextView(activity);
        mThemeLabel.setText("Theme: Default");
        mThemeLabel.setTextColor(Color.WHITE);
        mThemeLabel.setTextSize(TypedValue.COMPLEX_UNIT_SP, 14);
        mThemeLabel.setPadding(dp(4), dp(4), dp(4), dp(4));
        content.addView(mThemeLabel);

        mAnonNames = makeCheckbox("Anonymize Player Names", false);
        mAnonNames.setOnCheckedChangeListener(new CompoundButton.OnCheckedChangeListener() {
            @Override public void onCheckedChanged(CompoundButton b, boolean checked) {
                SurvBridge.saveConfig();
            }
        });
        content.addView(mAnonNames);

        // === Links ===
        addGap(content, dp(10));
        Button attribBtn = makeLinkButton("\u00A9 Attributions");
        attribBtn.setOnClickListener(new View.OnClickListener() {
            @Override public void onClick(View v) { Log.i(TAG, "Attributions link (stub)"); }
        });
        content.addView(attribBtn);

        Button hofBtn = makeLinkButton("\uD83C\uDFC6 Hall of Fame");
        hofBtn.setOnClickListener(new View.OnClickListener() {
            @Override public void onClick(View v) { Log.i(TAG, "Hall of Fame link (stub)"); }
        });
        content.addView(hofBtn);

        Button changelogBtn = makeLinkButton("\uD83D\uDCDD Changelog");
        changelogBtn.setOnClickListener(new View.OnClickListener() {
            @Override public void onClick(View v) { Log.i(TAG, "Changelog link (stub)"); }
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
    }

    public boolean isOpen() { return mOpen; }

    /** Reset all settings to default values */
    private void resetToDefaults() {
        // Reset language
        mLocaleIdx = 0;
        mLanguageBtn.setText("EN");
        SurvBridge.setLanguage("en");
        
        // Reset volumes
        mMasterSlider.setProgress(100);
        mSfxSlider.setProgress(100);
        mMusicSlider.setProgress(60);
        mMasterLabel.setText("100%");
        mSfxLabel.setText("100%");
        mMusicLabel.setText("60%");
        SurvBridge.setAudio("master", 1.0f);
        SurvBridge.setAudio("sound", 1.0f);
        SurvBridge.setAudio("music", 0.6f);
        
        // Reset checkboxes
        mHighRes.setChecked(true);
        mInterpolation.setChecked(true);
        mLocalRotation.setChecked(true);
        mScreenShake.setChecked(true);
        mAnonNames.setChecked(false);
        
        // Reset toggles
        mMoveStyleBtn.setText("Move Style: Anywhere");
        mAimStyleBtn.setText("Aim Style: Anywhere");
        mAimLineBtn.setText("Aim Line: On");
        mHudScaleIdx = 1;
        mHudScaleBtn.setText("HUD Scale: Normal");
        
        SurvBridge.saveConfig();
        mStatusText.setText("Settings reset to defaults");
        
        Log.i(TAG, "Settings reset to defaults");
    }

    public void syncFromConfig() {
        String config = SurvBridge.getConfigJson();
        if (config == null || config.isEmpty()) return;

        // Language
        String lang = Json.peekString(config, "language");
        if (lang != null) {
            mLanguageBtn.setText(lang.toUpperCase());
            for (int i = 0; i < mLocales.length; i++) {
                if (mLocales[i].equals(lang)) { mLocaleIdx = i; break; }
            }
        }

        // Volume sliders
        String master = Json.peekString(config, "masterVolume");
        if (master != null) { 
            try { 
                float mv = Float.parseFloat(master);
                mMasterSlider.setProgress((int)(mv * 100)); 
                mMasterLabel.setText(String.format("%.0f%%", mv * 100));
            } catch (Exception e) {} 
        }
        String sfx = Json.peekString(config, "soundVolume");
        if (sfx != null) { 
            try { 
                float sv = Float.parseFloat(sfx);
                mSfxSlider.setProgress((int)(sv * 100)); 
                mSfxLabel.setText(String.format("%.0f%%", sv * 100));
            } catch (Exception e) {} 
        }
        String music = Json.peekString(config, "musicVolume");
        if (music != null) { 
            try { 
                float mv = Float.parseFloat(music);
                mMusicSlider.setProgress((int)(mv * 100)); 
                mMusicLabel.setText(String.format("%.0f%%", mv * 100));
            } catch (Exception e) {} 
        }

        // Touch style
        String moveStyle = Json.peekString(config, "touchMoveStyle");
        if ("locked".equals(moveStyle)) mMoveStyleBtn.setText("Move Style: Locked");
        else mMoveStyleBtn.setText("Move Style: Anywhere");

        String aimStyle = Json.peekString(config, "touchAimStyle");
        if ("locked".equals(aimStyle)) mAimStyleBtn.setText("Aim Style: Locked");
        else mAimStyleBtn.setText("Aim Style: Anywhere");

        String aimLine = Json.peekString(config, "touchAimLine");
        if ("true".equals(aimLine)) mAimLineBtn.setText("Aim Line: On");
        else mAimLineBtn.setText("Aim Line: Off");
        
        // Graphics checkboxes
        String hiRes = Json.peekString(config, "highResTextures");
        mHighRes.setChecked(hiRes == null || "true".equals(hiRes));
        
        String interp = Json.peekString(config, "interpolation");
        mInterpolation.setChecked(interp == null || "true".equals(interp));
        
        String localRot = Json.peekString(config, "localRotation");
        mLocalRotation.setChecked(localRot == null || "true".equals(localRot));
        
        String shake = Json.peekString(config, "screenShake");
        mScreenShake.setChecked(shake == null || "true".equals(shake));
        
        String anon = Json.peekString(config, "anonPlayerNames");
        mAnonNames.setChecked("true".equals(anon));
        
        mStatusText.setText("");
    }

    private int dp(int value) {
        return (int) (value * mActivity.getResources().getDisplayMetrics().density + 0.5f);
    }
}
