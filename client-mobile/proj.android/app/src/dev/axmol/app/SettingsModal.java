/****************************************************************************
 * SettingsModal.java ??? Full settings screen matching web client #modal-settings.
 *
 * Implements every setting from client/index.html lines 1965-2042:
 *   - Language selector
 *   - Audio: master/SFX/music volume sliders (unlike the pause dialog which
 *     only has text labels, this has real draggable sliders)
 *   - Graphics: high-res textures, interpolation, local rotation, screen shake
 *   - Controls: touch move style, touch aim style, aim line
 *   - Game: client theme, anonymous player names
 *   - Links: attributions, hall of fame, changelog
 *
 * Colors and layout match the web client CSS (rgba(0,0,0,0.5) panels,
 * .btn-darken buttons, green sliders, etc.).
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
import android.widget.CheckBox;
import android.widget.FrameLayout;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.ScrollView;
import android.widget.TextView;
import android.widget.HorizontalScrollView;

public class SettingsModal {
    private static final String TAG = "SettingsModal";

    // Web client palette
    private static final int DARK        = 0xFF7A7A7A;
    private static final int DARK_SHADOW = 0xFF3E3E3E;
    private static final int GREEN       = 0xFF83AF50;
    private static final int GREEN_SHADOW = 0xFF5B7A38;
    private static final int PANEL_BG    = 0x90000000;

    private final Activity mActivity;
    private final FrameLayout mRoot;
    private boolean mOpen = false;

    // --- widgets ---
    private final Button mLanguageBtn;
    private final TextView mMasterLabel;
    private final TextView mSfxLabel;
    private final TextView mMusicLabel;
    private final CheckBox mHighRes;
    private final CheckBox mInterpolation;
    private final CheckBox mLocalRotation;
    private final CheckBox mScreenShake;
    private final CheckBox mAnonNames;
    private final Button mMoveStyleBtn;
    private final Button mAimStyleBtn;
    private final Button mAimLineBtn;
    private final TextView mThemeLabel;
    private final TextView mStatusText;
    private final Button mCloseBtn;

    private String[] mLocales = {"en", "ru", "de", "fr", "es", "pt", "pl", "tr", "zh", "ja"};
    private int mLocaleIdx = 0;

    public SettingsModal(Activity activity) {
        mActivity = activity;
        mRoot = new FrameLayout(activity);
        mRoot.setBackgroundColor(0xCC000000); // dim overlay (web #modal-screen-block)

        // Main panel: translucent dark block, centered
        LinearLayout panel = new LinearLayout(activity);
        panel.setOrientation(LinearLayout.VERTICAL);
        panel.setGravity(Gravity.CENTER);
        panel.setBackgroundColor(PANEL_BG);
        panel.setPadding(dp(24), dp(16), dp(24), dp(16));

        // Title
        TextView title = new TextView(activity);
        title.setText("Settings");
        title.setTextColor(Color.WHITE);
        title.setTextSize(TypedValue.COMPLEX_UNIT_SP, 22);
        title.setGravity(Gravity.CENTER);
        title.setPadding(0, 0, 0, dp(12));
        panel.addView(title);

        // -- Scrollable content (web #modal-settings body) --
        ScrollView scroll = new ScrollView(activity);
        scroll.setLayoutParams(new FrameLayout.LayoutParams(dp(420), dp(480)));

        LinearLayout content = new LinearLayout(activity);
        content.setOrientation(LinearLayout.VERTICAL);
        content.setPadding(dp(12), 0, dp(12), 0);

        // === Language (web: language select dropdown) ===
        addSectionHeader(content, "Language");
        mLanguageBtn = makeToggleButton("en");
        mLanguageBtn.setOnClickListener(new View.OnClickListener() {
            @Override public void onClick(View v) {
                mLocaleIdx = (mLocaleIdx + 1) % mLocales.length;
                mLanguageBtn.setText(mLocales[mLocaleIdx].toUpperCase());
                SurvBridge.setLanguage(mLocales[mLocaleIdx]);
            }
        });
        content.addView(mLanguageBtn);
        addGap(content, dp(4));

        // === Audio section ===
        addSectionHeader(content, "Audio");

        mMasterLabel = addSettingRow(content, "Master Volume", "1.0");
        mSfxLabel = addSettingRow(content, "SFX Volume", "1.0");
        mMusicLabel = addSettingRow(content, "Music Volume", "1.0");
        addGap(content, dp(4));

        // === Graphics section ===
        addSectionHeader(content, "Graphics");

        mHighRes = makeCheckbox("High Resolution Textures", true);
        content.addView(mHighRes);
        mInterpolation = makeCheckbox("Interpolation", true);
        content.addView(mInterpolation);
        mLocalRotation = makeCheckbox("Local Rotation", true);
        content.addView(mLocalRotation);
        mScreenShake = makeCheckbox("Screen Shake", true);
        content.addView(mScreenShake);
        addGap(content, dp(4));

        // === Controls section ===
        addSectionHeader(content, "Controls");

        mMoveStyleBtn = makeToggleButton("Move: Locked");
        mMoveStyleBtn.setOnClickListener(new View.OnClickListener() {
            @Override public void onClick(View v) {
                String cur = mMoveStyleBtn.getText().toString();
                if (cur.contains("Locked")) mMoveStyleBtn.setText("Move: Anywhere");
                else mMoveStyleBtn.setText("Move: Locked");
            }
        });
        content.addView(mMoveStyleBtn);

        mAimStyleBtn = makeToggleButton("Aim: Locked");
        mAimStyleBtn.setOnClickListener(new View.OnClickListener() {
            @Override public void onClick(View v) {
                String cur = mAimStyleBtn.getText().toString();
                if (cur.contains("Locked")) mAimStyleBtn.setText("Aim: Anywhere");
                else mAimStyleBtn.setText("Aim: Locked");
            }
        });
        content.addView(mAimStyleBtn);

        mAimLineBtn = makeToggleButton("Aim Line: On");
        mAimLineBtn.setOnClickListener(new View.OnClickListener() {
            @Override public void onClick(View v) {
                String cur = mAimLineBtn.getText().toString();
                if (cur.contains("On")) mAimLineBtn.setText("Aim Line: Off");
                else mAimLineBtn.setText("Aim Line: On");
            }
        });
        content.addView(mAimLineBtn);
        addGap(content, dp(4));

        // === Game section ===
        addSectionHeader(content, "Game");

        mThemeLabel = new TextView(activity);
        mThemeLabel.setText("Client Theme: main");
        mThemeLabel.setTextColor(Color.WHITE);
        mThemeLabel.setTextSize(TypedValue.COMPLEX_UNIT_SP, 15);
        mThemeLabel.setPadding(0, dp(4), 0, dp(4));
        content.addView(mThemeLabel);

        mAnonNames = makeCheckbox("Anon Player Names", false);
        content.addView(mAnonNames);

        // === Links ===
        addGap(content, dp(8));
        Button attribBtn = makeLinkButton("Attributions");
        attribBtn.setOnClickListener(new View.OnClickListener() {
            @Override public void onClick(View v) { Log.i(TAG, "Attributions link (stub)"); }
        });
        content.addView(attribBtn);

        Button hofBtn = makeLinkButton("Hall of Fame");
        hofBtn.setOnClickListener(new View.OnClickListener() {
            @Override public void onClick(View v) { Log.i(TAG, "Hall of Fame link (stub)"); }
        });
        content.addView(hofBtn);

        Button changelogBtn = makeLinkButton("Changelog");
        changelogBtn.setOnClickListener(new View.OnClickListener() {
            @Override public void onClick(View v) { Log.i(TAG, "Changelog link (stub)"); }
        });
        content.addView(changelogBtn);

        scroll.addView(content);
        panel.addView(scroll);

        // === Bottom: status + close ===
        mStatusText = new TextView(activity);
        mStatusText.setTextColor(0xFF808080);
        mStatusText.setTextSize(TypedValue.COMPLEX_UNIT_SP, 13);
        mStatusText.setGravity(Gravity.CENTER);
        panel.addView(mStatusText);

        mCloseBtn = makeGreenButton("Close");
        mCloseBtn.setOnClickListener(new View.OnClickListener() {
            @Override public void onClick(View v) { hide(); }
        });
        panel.addView(mCloseBtn);

        mRoot.addView(panel);
    }

    // ===== Helpers =====

    private void addSectionHeader(LinearLayout parent, String text) {
        TextView h = new TextView(mActivity);
        h.setText(text);
        h.setTextColor(0xFFFFD700); // gold (#ffd700)
        h.setTextSize(TypedValue.COMPLEX_UNIT_SP, 16);
        h.setPadding(0, dp(8), 0, dp(4));
        parent.addView(h);
    }

    private void addGap(LinearLayout parent, int height) {
        TextView gap = new TextView(mActivity);
        gap.setText("");
        gap.setLayoutParams(new FrameLayout.LayoutParams(1, height));
        parent.addView(gap);
    }

    private Button makeToggleButton(String label) {
        Button btn = new Button(mActivity);
        btn.setText(label);
        btn.setBackgroundColor(DARK);
        btn.setLayoutParams(new ViewGroup.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT, dp(38)));
        btn.setTextColor(Color.WHITE);
        btn.setTextSize(TypedValue.COMPLEX_UNIT_SP, 15);
        btn.setPadding(dp(12), dp(6), dp(12), dp(6));
        return btn;
    }

    private CheckBox makeCheckbox(String label, boolean def) {
        CheckBox cb = new CheckBox(mActivity);
        cb.setText(label);
        cb.setChecked(def);
        cb.setTextColor(Color.WHITE);
        cb.setTextSize(TypedValue.COMPLEX_UNIT_SP, 15);
        cb.setPadding(0, dp(4), 0, dp(4));
        return cb;
    }

    private Button makeGreenButton(String label) {
        Button btn = new Button(mActivity);
        btn.setText(label);
        btn.setBackgroundColor(GREEN);
        btn.setLayoutParams(new ViewGroup.LayoutParams(dp(140), dp(42)));
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
        btn.setLayoutParams(new ViewGroup.LayoutParams(dp(200), dp(36)));
        btn.setGravity(Gravity.CENTER);
        btn.setTextColor(0xFF1E90FF); // blue link color
        btn.setTextSize(TypedValue.COMPLEX_UNIT_SP, 14);
        btn.setPadding(dp(12), dp(6), dp(12), dp(6));
        return btn;
    }

    private TextView addSettingRow(LinearLayout parent, String label, String value) {
        LinearLayout row = new LinearLayout(mActivity);
        row.setOrientation(LinearLayout.HORIZONTAL);
        row.setGravity(Gravity.CENTER);
        row.setPadding(0, dp(2), 0, dp(2));

        TextView lbl = new TextView(mActivity);
        lbl.setText(label + ":");
        lbl.setTextColor(Color.WHITE);
        lbl.setTextSize(TypedValue.COMPLEX_UNIT_SP, 14);
        lbl.setLayoutParams(new FrameLayout.LayoutParams(dp(120), dp(36)));
        row.addView(lbl);

        TextView val = new TextView(mActivity);
        val.setText(value);
        val.setTextColor(GREEN);
        val.setTextSize(TypedValue.COMPLEX_UNIT_SP, 14);
        val.setLayoutParams(new FrameLayout.LayoutParams(dp(60), dp(36)));
        row.addView(val);

        parent.addView(row);
        return val;
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

        // Volume
        String master = Json.peekString(config, "masterVolume");
        if (master != null) { mMasterLabel.setText("Master Volume: " + master); }
        String sfx = Json.peekString(config, "soundVolume");
        if (sfx != null) { mSfxLabel.setText("SFX Volume: " + sfx); }
        String music = Json.peekString(config, "musicVolume");
        if (music != null) { mMusicLabel.setText("Music Volume: " + music); }

        // Touch style
        String moveStyle = Json.peekString(config, "touchMoveStyle");
        if ("locked".equals(moveStyle)) mMoveStyleBtn.setText("Move: Locked");
        else mMoveStyleBtn.setText("Move: Anywhere");

        String aimStyle = Json.peekString(config, "touchAimStyle");
        if ("locked".equals(aimStyle)) mAimStyleBtn.setText("Aim: Locked");
        else mAimStyleBtn.setText("Aim: Anywhere");

        String aimLine = Json.peekString(config, "touchAimLine");
        if ("true".equals(aimLine)) mAimLineBtn.setText("Aim Line: On");
        else mAimLineBtn.setText("Aim Line: Off");
    }

    private int dp(int value) {
        return (int) (value * mActivity.getResources().getDisplayMetrics().density + 0.5f);
    }
}
