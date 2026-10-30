/****************************************************************************
 * SettingsModal.java — Full settings screen matching web client #modal-settings.
 *
 * Styled after the Java pause menu / result dialog so all native screens share
 * one design language:
 *   - Dark-green translucent card with a tan border and rounded corners
 *   - Teal pill buttons with an inset bottom shadow and press feedback
 *   - Thin teal sliders with a round thumb (label/value above the track)
 *   - Cream section headers with compact vertical spacing
 *   - Fixed title/footer; only the settings body scrolls
 *
 * Functionality is unchanged: language cycle, Master/SFX/Music volume,
 * interpolation/screen shake/anonymous-names toggles, touch move/aim styles,
 * aim line, Key Binds, links and reset-to-defaults.
 ****************************************************************************/
package dev.axmol.app;

import android.app.Activity;
import android.content.Intent;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.graphics.PixelFormat;
import android.graphics.Rect;
import android.graphics.RectF;
import android.graphics.drawable.Drawable;
import android.graphics.drawable.GradientDrawable;
import android.graphics.drawable.LayerDrawable;
import android.net.Uri;
import android.util.Log;
import android.util.TypedValue;
import android.view.Gravity;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewGroup;
import android.widget.FrameLayout;
import android.widget.LinearLayout;
import android.widget.ScrollView;
import android.widget.SeekBar;
import android.widget.TextView;

import java.util.ArrayList;
import java.util.List;

public class SettingsModal {
    private static final String TAG = "SettingsModal";

    // Palette shared with GamePauseDialog (web #ui-game-menu).
    private static final int TEAL          = 0xFF50AFAB;
    private static final int TEAL_SHADOW   = 0xFF387C79;
    private static final int TEAL_OFF      = 0xFF4E6B65;
    private static final int TEAL_OFF_SHD  = 0xFF344B47;
    private static final int PANEL_BG      = 0xE6152411;
    private static final int PANEL_BORDER  = 0xFFB9AD84;
    private static final int TEXT_CREAM    = 0xFFF1EAD2;
    private static final int TEXT_LABEL    = 0xFFE6EFE2;
    private static final int TEXT_VALUE    = 0xFFA8CEC9;
    private static final int TEXT_MUTED    = 0xFFB7C4B4;
    private static final int TRACK_BG      = 0xFF2A4A40;
    // Web-adjacent (attribution/HOF/changelog) links sit lower in the hierarchy.
    private static final int LINK_BODY     = 0xFF3E4F45;
    private static final int LINK_SHADOW   = 0xFF2A372F;
    private static final int SCRIM         = 0x36000000;

    private final Activity mActivity;
    private final FrameLayout mRoot;
    private boolean mOpen = false;
    private Runnable mOnHidden = null;
    private Runnable mOnOpenBinds = null;

    // --- widgets ---
    private TextView mLanguageBtn;
    private SliderRow mMasterRow;
    private SliderRow mSfxRow;
    private SliderRow mMusicRow;
    private TextView mInterpolationBtn;
    private TextView mScreenShakeBtn;
    private TextView mAnonNamesBtn;
    private TextView mMoveStyleBtn;
    private TextView mAimStyleBtn;
    private TextView mAimLineBtn;
    private TextView mStatusText;
    private TextView mCloseBtn;
    private TextView mResetBtn;
    private ScrollView mScroll;

    // --- state ---
    // ui::kLocales, fetched from the C++ bridge (localization.ts Locales).
    private List<Json.LocaleEntry> mLocales = new ArrayList<>();
    private int mLocaleIdx = 0;
    private boolean mMoveLocked = false;
    private boolean mAimLocked = false;
    private boolean mAimLine = true;
    private boolean mInterpolationOn = true;
    private boolean mScreenShakeOn = true;
    private boolean mAnonNamesOn = false;

    public SettingsModal(Activity activity) {
        mActivity = activity;
        mRoot = new FrameLayout(activity);
        buildUi();
    }

    /**
     * Builds the modal views. Called again after a language change/reset so
     * the already-open screen re-localizes in place.
     */
    private void buildUi() {
        mRoot.removeAllViews();
        mRoot.setBackgroundColor(SCRIM);
        // Modal: taps on the dimmed area must not reach the menu behind it.
        mRoot.setClickable(true);
        mRoot.setFocusable(true);

        LinearLayout card = new LinearLayout(mActivity);
        card.setOrientation(LinearLayout.VERTICAL);
        card.setGravity(Gravity.CENTER_HORIZONTAL);
        GradientDrawable cardBg = new GradientDrawable();
        cardBg.setColor(PANEL_BG);
        cardBg.setCornerRadius(dp(8));
        cardBg.setStroke(dp(2), PANEL_BORDER);
        card.setBackground(cardBg);
        card.setPadding(dp(12), dp(10), dp(12), dp(10));
        FrameLayout.LayoutParams cardParams = new FrameLayout.LayoutParams(
                cardWidth(), cardHeight());
        cardParams.gravity = Gravity.CENTER;
        card.setLayoutParams(cardParams);

        TextView title = new TextView(mActivity);
        title.setText("\u2699 " + L10n.t("index-settings", "Settings"));
        title.setTextColor(TEXT_CREAM);
        title.setTextSize(TypedValue.COMPLEX_UNIT_SP, 16);
        title.setTypeface(android.graphics.Typeface.DEFAULT_BOLD);
        title.setGravity(Gravity.CENTER);
        title.setIncludeFontPadding(false);
        title.setShadowLayer(dp(2), 0, dp(2), 0x55000000);
        card.addView(title, fullParams(ViewGroup.LayoutParams.WRAP_CONTENT));

        addGap(card, dp(6));

        // Only the body scrolls: the title and the Reset/Close footer stay put.
        mScroll = new ScrollView(mActivity);
        mScroll.setVerticalScrollBarEnabled(false);
        mScroll.setFillViewport(true);

        LinearLayout body = new LinearLayout(mActivity);
        body.setOrientation(LinearLayout.VERTICAL);
        body.setPadding(0, 0, 0, dp(2));
        mScroll.addView(body, new FrameLayout.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT));
        card.addView(mScroll, new LinearLayout.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT, 0, 1f));

        // === Language ===
        body.addView(makeSectionHeader(L10n.t("settings-language", "Language")),
                fullParams(ViewGroup.LayoutParams.WRAP_CONTENT));
        mLocales = Json.parseLocales(SurvBridge.getLocalesJson());
        if (mLocales.isEmpty()) {
            Json.LocaleEntry en = new Json.LocaleEntry();
            en.code = "en";
            en.name = "English";
            mLocales.add(en);
        }
        mLanguageBtn = makeThemedButton(
                mLocales.get(0).name + "  \u25BE", TEAL, TEAL_SHADOW);
        mLanguageBtn.setOnClickListener(new View.OnClickListener() {
            @Override public void onClick(View v) {
                mLocaleIdx = (mLocaleIdx + 1) % mLocales.size();
                Json.LocaleEntry locale = mLocales.get(mLocaleIdx);
                mLanguageBtn.setText(locale.name + "  \u25BE");
                SurvBridge.setLanguage(locale.code);
                SurvBridge.saveConfig();
                L10n.invalidate();
                // Rebuild the open screen with the new locale's labels.
                rebuildUi();
                mStatusText.setText("");
            }
        });
        body.addView(mLanguageBtn, fullParams(dp(36)));

        // === Audio ===
        body.addView(makeSectionHeader(L10n.t("settings-audio", "Audio")),
                fullParams(ViewGroup.LayoutParams.WRAP_CONTENT));
        mMasterRow = makeSliderRow(L10n.t("index-master-volume", "Master Volume"), 100);
        bindVolumeSlider(mMasterRow, "master");
        body.addView(mMasterRow.root, fullParams(ViewGroup.LayoutParams.WRAP_CONTENT));

        mSfxRow = makeSliderRow(L10n.t("index-sfx-volume", "SFX Volume"), 100);
        bindVolumeSlider(mSfxRow, "sound");
        body.addView(mSfxRow.root, fullParams(ViewGroup.LayoutParams.WRAP_CONTENT));

        mMusicRow = makeSliderRow(L10n.t("index-music-volume", "Music Volume"), 100);
        bindVolumeSlider(mMusicRow, "music");
        body.addView(mMusicRow.root, fullParams(ViewGroup.LayoutParams.WRAP_CONTENT));

        // === Graphics ===
        body.addView(makeSectionHeader(L10n.t("settings-graphics", "Graphics")),
                fullParams(ViewGroup.LayoutParams.WRAP_CONTENT));
        mInterpolationBtn = makeThemedButton("", TEAL, TEAL_SHADOW);
        mInterpolationBtn.setOnClickListener(new View.OnClickListener() {
            @Override public void onClick(View v) {
                mInterpolationOn = !mInterpolationOn;
                applyToggleButtons();
                SurvBridge.setConfigValue("interpolation", mInterpolationOn ? "true" : "false");
                SurvBridge.saveConfig();
            }
        });
        body.addView(mInterpolationBtn, fullParams(dp(36)));

        mScreenShakeBtn = makeThemedButton("", TEAL, TEAL_SHADOW);
        mScreenShakeBtn.setOnClickListener(new View.OnClickListener() {
            @Override public void onClick(View v) {
                mScreenShakeOn = !mScreenShakeOn;
                applyToggleButtons();
                SurvBridge.setConfigValue("screenShake", mScreenShakeOn ? "true" : "false");
                SurvBridge.saveConfig();
            }
        });
        body.addView(mScreenShakeBtn, fullParams(dp(36)));

        // === Controls ===
        body.addView(makeSectionHeader(L10n.t("index-controls", "Controls")),
                fullParams(ViewGroup.LayoutParams.WRAP_CONTENT));
        mMoveStyleBtn = makeThemedButton("", TEAL, TEAL_SHADOW);
        mMoveStyleBtn.setOnClickListener(new View.OnClickListener() {
            @Override public void onClick(View v) {
                mMoveLocked = !mMoveLocked;
                applyTouchStyleButtons();
                SurvBridge.setConfigValue("touchMoveStyle", mMoveLocked ? "locked" : "anywhere");
                SurvBridge.saveConfig();
            }
        });
        body.addView(mMoveStyleBtn, fullParams(dp(36)));

        mAimStyleBtn = makeThemedButton("", TEAL, TEAL_SHADOW);
        mAimStyleBtn.setOnClickListener(new View.OnClickListener() {
            @Override public void onClick(View v) {
                mAimLocked = !mAimLocked;
                applyTouchStyleButtons();
                SurvBridge.setConfigValue("touchAimStyle", mAimLocked ? "locked" : "anywhere");
                SurvBridge.saveConfig();
            }
        });
        body.addView(mAimStyleBtn, fullParams(dp(36)));

        mAimLineBtn = makeThemedButton("", TEAL, TEAL_SHADOW);
        mAimLineBtn.setOnClickListener(new View.OnClickListener() {
            @Override public void onClick(View v) {
                mAimLine = !mAimLine;
                applyTouchStyleButtons();
                SurvBridge.setConfigValue("touchAimLine", mAimLine ? "true" : "false");
                SurvBridge.saveConfig();
            }
        });
        body.addView(mAimLineBtn, fullParams(dp(36)));

        // Key/mouse bind editor (client/src/inputBinds.ts InputBindUi): the
        // table lives in C++ and is applied to the live hardware input.
        TextView bindsBtn = makeThemedButton(
                L10n.t("index-keybinds", "Key Binds") + "...", TEAL, TEAL_SHADOW);
        bindsBtn.setOnClickListener(new View.OnClickListener() {
            @Override public void onClick(View v) {
                if (mOnOpenBinds != null) mOnOpenBinds.run();
            }
        });
        body.addView(bindsBtn, fullParams(dp(36)));

        // === Game ===
        body.addView(makeSectionHeader(L10n.t("settings-game", "Game")),
                fullParams(ViewGroup.LayoutParams.WRAP_CONTENT));
        mAnonNamesBtn = makeThemedButton("", TEAL, TEAL_SHADOW);
        mAnonNamesBtn.setOnClickListener(new View.OnClickListener() {
            @Override public void onClick(View v) {
                mAnonNamesOn = !mAnonNamesOn;
                applyToggleButtons();
                SurvBridge.setConfigValue("anonPlayerNames", mAnonNamesOn ? "true" : "false");
                SurvBridge.saveConfig();
            }
        });
        body.addView(mAnonNamesBtn, fullParams(dp(36)));

        // === Links === (web #settings-links; compact two-column rows)
        addGap(body, dp(6));
        LinearLayout linkRowA = new LinearLayout(mActivity);
        linkRowA.setOrientation(LinearLayout.HORIZONTAL);
        TextView leaderboardBtn = makeLinkButton(L10n.t("index-leaderboards", "Leaderboards"));
        leaderboardBtn.setOnClickListener(new View.OnClickListener() {
            @Override public void onClick(View v) { openWebPage("/stats"); }
        });
        TextView attribBtn = makeLinkButton(L10n.t("index-attributions", "attributions"));
        attribBtn.setOnClickListener(new View.OnClickListener() {
            @Override public void onClick(View v) { openWebPage("/attribution.txt"); }
        });
        linkRowA.addView(leaderboardBtn, halfParams(0, dp(3)));
        linkRowA.addView(attribBtn, halfParams(dp(3), 0));
        body.addView(linkRowA, fullParams(dp(34)));

        addGap(body, dp(4));
        LinearLayout linkRowB = new LinearLayout(mActivity);
        linkRowB.setOrientation(LinearLayout.HORIZONTAL);
        TextView hofBtn = makeLinkButton(L10n.t("index-hof", "Hall of Fame"));
        hofBtn.setOnClickListener(new View.OnClickListener() {
            @Override public void onClick(View v) { openWebPage("/hof.html"); }
        });
        TextView changelogBtn = makeLinkButton(
                L10n.t("index-original-changelog", "Changelog"));
        changelogBtn.setOnClickListener(new View.OnClickListener() {
            @Override public void onClick(View v) { openWebPage("/changelogRec.html"); }
        });
        linkRowB.addView(hofBtn, halfParams(0, dp(3)));
        linkRowB.addView(changelogBtn, halfParams(dp(3), 0));
        body.addView(linkRowB, fullParams(dp(34)));

        // === Fixed footer: status + Reset/Close ===
        mStatusText = new TextView(mActivity);
        mStatusText.setTextColor(TEXT_MUTED);
        mStatusText.setTextSize(TypedValue.COMPLEX_UNIT_SP, 10);
        mStatusText.setGravity(Gravity.CENTER);
        mStatusText.setIncludeFontPadding(false);
        card.addView(mStatusText, fullParams(ViewGroup.LayoutParams.WRAP_CONTENT));

        addGap(card, dp(5));

        LinearLayout actions = new LinearLayout(mActivity);
        actions.setOrientation(LinearLayout.HORIZONTAL);
        mResetBtn = makeThemedButton(L10n.t("settings-reset", "Reset"),
                TEAL_OFF, TEAL_OFF_SHD);
        mResetBtn.setOnClickListener(new View.OnClickListener() {
            @Override public void onClick(View v) { resetToDefaults(); }
        });
        actions.addView(mResetBtn, halfParams(0, dp(3)));

        mCloseBtn = makeThemedButton(L10n.t("index-close", "Close"), TEAL, TEAL_SHADOW);
        mCloseBtn.setOnClickListener(new View.OnClickListener() {
            @Override public void onClick(View v) { hide(); }
        });
        actions.addView(mCloseBtn, halfParams(dp(3), 0));
        card.addView(actions, fullParams(dp(40)));

        mRoot.addView(card);
        applyTouchStyleButtons();
        applyToggleButtons();
    }

    private TextView makeSectionHeader(String text) {
        TextView header = new TextView(mActivity);
        header.setText(text);
        header.setTextColor(TEXT_CREAM);
        header.setTextSize(TypedValue.COMPLEX_UNIT_SP, 12);
        header.setTypeface(android.graphics.Typeface.DEFAULT_BOLD);
        header.setIncludeFontPadding(false);
        header.setPadding(0, dp(7), 0, dp(3));
        return header;
    }

    private void addGap(LinearLayout parent, int height) {
        View gap = new View(mActivity);
        parent.addView(gap, new LinearLayout.LayoutParams(1, height));
    }

    /** Horizontal label/value line above a full-width slider (pause dialog). */
    private SliderRow makeSliderRow(String label, int defaultValue) {
        SliderRow row = new SliderRow();

        LinearLayout root = new LinearLayout(mActivity);
        root.setOrientation(LinearLayout.VERTICAL);
        root.setPadding(0, dp(4), 0, dp(2));

        LinearLayout labelRow = new LinearLayout(mActivity);
        labelRow.setOrientation(LinearLayout.HORIZONTAL);
        labelRow.setGravity(Gravity.CENTER_VERTICAL);

        TextView labelView = new TextView(mActivity);
        labelView.setText(label);
        labelView.setTextColor(TEXT_LABEL);
        labelView.setTextSize(TypedValue.COMPLEX_UNIT_SP, 13);
        labelView.setIncludeFontPadding(false);
        labelView.setLayoutParams(new LinearLayout.LayoutParams(
                0, ViewGroup.LayoutParams.WRAP_CONTENT, 1f));
        labelRow.addView(labelView);

        TextView value = new TextView(mActivity);
        value.setText(defaultValue + "%");
        value.setTextColor(TEXT_VALUE);
        value.setTextSize(TypedValue.COMPLEX_UNIT_SP, 10);
        value.setIncludeFontPadding(false);
        labelRow.addView(value);

        root.addView(labelRow);

        SeekBar slider = new SeekBar(mActivity);
        slider.setMax(100);
        slider.setProgress(defaultValue);
        styleSlider(slider);
        root.addView(slider, new LinearLayout.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT, dp(34)));

        row.root = root;
        row.label = labelView;
        row.value = value;
        row.bar = slider;
        return row;
    }

    private void bindVolumeSlider(final SliderRow row, final String audioKey) {
        row.bar.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {
            @Override public void onProgressChanged(SeekBar sb, int progress, boolean fromUser) {
                float val = progress / 100f;
                row.value.setText(String.format("%.0f%%", val * 100));
                if (fromUser) SurvBridge.setAudio(audioKey, val);
            }
            @Override public void onStartTrackingTouch(SeekBar sb) {}
            @Override public void onStopTrackingTouch(SeekBar sb) { SurvBridge.saveConfig(); }
        });
    }

    /** Teal progress track + round thumb, matching the web range input. */
    private void styleSlider(SeekBar slider) {
        slider.setProgressDrawable(new SliderTrackDrawable());
        GradientDrawable thumb = new GradientDrawable();
        thumb.setShape(GradientDrawable.OVAL);
        thumb.setColor(TEAL);
        thumb.setStroke(dp(2), TEAL_SHADOW);
        thumb.setSize(dp(22), dp(22));
        slider.setThumb(thumb);
        slider.setThumbOffset(dp(1));
        slider.setSplitTrack(false);
        slider.setPadding(dp(10), 0, dp(10), 0);
    }

    /**
     * Thin rounded track with a teal filled portion (same drawable as the pause
     * menu). A plain LayerDrawable would stretch to the SeekBar height.
     */
    private final class SliderTrackDrawable extends Drawable {
        private final Paint mPaint = new Paint(Paint.ANTI_ALIAS_FLAG);
        private int mLevel = 0;
        private int mAlpha = 255;
        private android.graphics.ColorFilter mFilter = null;

        @Override public void draw(Canvas canvas) {
            Rect b = getBounds();
            float h = dp(6);
            float top = b.top + (b.height() - h) / 2f;
            float radius = h / 2f;

            mPaint.setColorFilter(mFilter);
            mPaint.setColor(TRACK_BG);
            mPaint.setAlpha(mAlpha);
            canvas.drawRoundRect(new RectF(b.left, top, b.right, top + h), radius, radius, mPaint);

            float right = b.left + b.width() * (mLevel / 10000f);
            if (right > b.left) {
                mPaint.setColor(TEAL);
                mPaint.setAlpha(mAlpha);
                canvas.drawRoundRect(new RectF(b.left, top, right, top + h), radius, radius, mPaint);
            }
        }

        @Override protected boolean onLevelChange(int level) {
            mLevel = level;
            invalidateSelf();
            return true;
        }

        @Override public void setAlpha(int alpha) { mAlpha = alpha; invalidateSelf(); }
        @Override public void setColorFilter(android.graphics.ColorFilter cf) {
            mFilter = cf;
            invalidateSelf();
        }
        @Override public int getOpacity() { return PixelFormat.TRANSLUCENT; }
    }

    /**
     * Teal pill with a darker inset bottom edge (GamePauseDialog's buttonBg)
     * and press feedback. Text-only: the pause screen's line icons are not
     * needed for the settings labels.
     */
    private TextView makeThemedButton(String label, final int body, final int shadow) {
        TextView btn = new TextView(mActivity);
        btn.setText(label);
        btn.setTextColor(Color.WHITE);
        btn.setTextSize(TypedValue.COMPLEX_UNIT_SP, 13);
        btn.setGravity(Gravity.CENTER);
        btn.setIncludeFontPadding(false);
        btn.setShadowLayer(dp(1), 0, dp(1), 0x55000000);
        btn.setPadding(dp(14), 0, dp(14), 0);
        btn.setClickable(true);
        btn.setBackground(buttonBg(body, shadow));
        btn.setOnTouchListener(new View.OnTouchListener() {
            @Override public boolean onTouch(View v, MotionEvent e) {
                int action = e.getActionMasked();
                if (action == MotionEvent.ACTION_DOWN) {
                    v.setAlpha(0.72f);
                } else if (action == MotionEvent.ACTION_UP || action == MotionEvent.ACTION_CANCEL) {
                    v.setAlpha(1f);
                }
                return false;
            }
        });
        return btn;
    }

    private TextView makeLinkButton(String label) {
        TextView btn = makeThemedButton(label, LINK_BODY, LINK_SHADOW);
        btn.setTextSize(TypedValue.COMPLEX_UNIT_SP, 12);
        btn.setPadding(dp(6), 0, dp(6), 0);
        return btn;
    }

    private void setButtonTone(TextView btn, boolean on) {
        btn.setBackground(buttonBg(on ? TEAL : TEAL_OFF, on ? TEAL_SHADOW : TEAL_OFF_SHD));
    }

    private LayerDrawable buttonBg(int body, int shadow) {
        GradientDrawable bottom = new GradientDrawable();
        bottom.setColor(shadow);
        bottom.setCornerRadius(dp(6));
        GradientDrawable top = new GradientDrawable();
        top.setColor(body);
        top.setCornerRadius(dp(6));
        LayerDrawable ld = new LayerDrawable(new Drawable[] { bottom, top });
        ld.setLayerInset(1, 0, 0, 0, dp(3));
        return ld;
    }

    private LinearLayout.LayoutParams fullParams(int height) {
        return new LinearLayout.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT, height);
    }

    private LinearLayout.LayoutParams halfParams(int leftMargin, int rightMargin) {
        LinearLayout.LayoutParams lp = new LinearLayout.LayoutParams(
                0, ViewGroup.LayoutParams.MATCH_PARENT, 1f);
        lp.leftMargin = leftMargin;
        lp.rightMargin = rightMargin;
        return lp;
    }

    private void applyToggleButtons() {
        setToggle(mInterpolationBtn,
                L10n.t("index-client-side-interp", "Client side interpolation"),
                mInterpolationOn);
        setToggle(mScreenShakeBtn, L10n.t("index-screen-shake", "Screen shake"),
                mScreenShakeOn);
        setToggle(mAnonNamesBtn,
                L10n.t("index-anon-player-names", "Anonymize player names"),
                mAnonNamesOn);
    }

    /** Check/cross pill: the tone alone is not enough for color-blind users. */
    private void setToggle(TextView btn, String label, boolean on) {
        btn.setText((on ? "\u2713  " : "\u2715  ") + label);
        setButtonTone(btn, on);
    }

    private void applyTouchStyleButtons() {
        mMoveStyleBtn.setText(mMoveLocked
                ? L10n.t("settings-move-locked", "Move Style: Locked")
                : L10n.t("settings-move-anywhere", "Move Style: Anywhere"));
        mAimStyleBtn.setText(mAimLocked
                ? L10n.t("settings-aim-locked", "Aim Style: Locked")
                : L10n.t("settings-aim-anywhere", "Aim Style: Anywhere"));
        final String aimLineText = mAimLine
                ? L10n.t("settings-aim-line-on", "Aim Line: On")
                : L10n.t("settings-aim-line-off", "Aim Line: Off");
        mAimLineBtn.setText((mAimLine ? "\u2713  " : "\u2715  ") + aimLineText);
        setButtonTone(mAimLineBtn, mAimLine);
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

    // ===== Public API =====

    public View root() { return mRoot; }

    public void show() {
        mOpen = true;
        mRoot.setVisibility(View.VISIBLE);
        syncFromConfig();
        // Reopen at the top; a previous scroll should hide nothing.
        mScroll.post(new Runnable() {
            @Override public void run() { mScroll.scrollTo(0, 0); }
        });
    }

    public void hide() {
        mOpen = false;
        mRoot.setVisibility(View.GONE);
        if (mOnHidden != null) mOnHidden.run();
    }

    public boolean isOpen() { return mOpen; }

    /** Called after hide() (menu re-localizes when the language changed). */
    public void setOnHidden(Runnable onHidden) { mOnHidden = onHidden; }

    /** Opens the Java Key Binds screen (owned by JavaUiHost). */
    public void setOnOpenBinds(Runnable onOpenBinds) { mOnOpenBinds = onOpenBinds; }

    /** Reset all settings to default values. */
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
            mLanguageBtn.setText(mLocales.get(mLocaleIdx).name + "  \u25BE");
        }
        SurvBridge.setLanguage("en");
        L10n.invalidate();

        // Volumes (web defaults: master 1, sound 1, music 1). setAudio()
        // applies on the game thread, so the visible sliders are pinned after
        // the rebuild below instead of relying on the config read.
        SurvBridge.setAudio("master", 1.0f);
        SurvBridge.setAudio("sound", 1.0f);
        SurvBridge.setAudio("music", 1.0f);

        // Graphics / game toggles.
        mInterpolationOn = true;
        mScreenShakeOn = true;
        mAnonNamesOn = false;
        SurvBridge.setConfigValue("interpolation", "true");
        SurvBridge.setConfigValue("screenShake", "true");
        SurvBridge.setConfigValue("anonPlayerNames", "false");

        // Controls (config defaults are "anywhere" + aim line on).
        mMoveLocked = false;
        mAimLocked = false;
        mAimLine = true;
        SurvBridge.setConfigValue("touchMoveStyle", "anywhere");
        SurvBridge.setConfigValue("touchAimStyle", "anywhere");
        SurvBridge.setConfigValue("touchAimLine", "true");

        SurvBridge.saveConfig();
        // Reset can change the language too; rebuild so every label matches.
        rebuildUi();
        // The audio setters are marshalled to the game thread; show the reset
        // values immediately rather than the config's previous numbers.
        mMasterRow.bar.setProgress(100);
        mSfxRow.bar.setProgress(100);
        mMusicRow.bar.setProgress(100);
        mMasterRow.value.setText("100%");
        mSfxRow.value.setText("100%");
        mMusicRow.value.setText("100%");
        mStatusText.setText(L10n.t("settings-reset-done", "Settings reset to defaults"));

        Log.i(TAG, "Settings reset to defaults");
    }

    /** Rebuilds the view tree and re-applies the saved config. */
    private void rebuildUi() {
        buildUi();
        syncFromConfig();
        applyTouchStyleButtons();
    }

    public void syncFromConfig() {
        String config = SurvBridge.getConfigJson();
        if (config == null || config.isEmpty()) return;

        // Language (names come from ui::kLocales via the bridge).
        String lang = Json.peekString(config, "language");
        if (lang != null) {
            for (int i = 0; i < mLocales.size(); i++) {
                if (mLocales.get(i).code.equals(lang)) {
                    mLocaleIdx = i;
                    mLanguageBtn.setText(mLocales.get(i).name + "  \u25BE");
                    break;
                }
            }
        }

        // Volume sliders: the config stores numbers, not strings.
        float master = Json.peekFloat(config, "masterVolume", 1.0f);
        mMasterRow.bar.setProgress(Math.round(master * 100));
        mMasterRow.value.setText(String.format("%.0f%%", master * 100));
        float sfx = Json.peekFloat(config, "soundVolume", 1.0f);
        mSfxRow.bar.setProgress(Math.round(sfx * 100));
        mSfxRow.value.setText(String.format("%.0f%%", sfx * 100));
        float music = Json.peekFloat(config, "musicVolume", 1.0f);
        mMusicRow.bar.setProgress(Math.round(music * 100));
        mMusicRow.value.setText(String.format("%.0f%%", music * 100));

        // Graphics / game toggles.
        mInterpolationOn = Json.peekBool(config, "interpolation", true);
        mScreenShakeOn = Json.peekBool(config, "screenShake", true);
        mAnonNamesOn = Json.peekBool(config, "anonPlayerNames", false);
        applyToggleButtons();

        // Touch style / aim line.
        mMoveLocked = "locked".equals(Json.peekString(config, "touchMoveStyle"));
        mAimLocked = "locked".equals(Json.peekString(config, "touchAimStyle"));
        mAimLine = Json.peekBool(config, "touchAimLine", true);
        applyTouchStyleButtons();

        mStatusText.setText("");
    }

    private int cardWidth() {
        int screen = mActivity.getResources().getDisplayMetrics().widthPixels;
        return Math.min(dp(360), screen - dp(24));
    }

    /**
     * Cap the card so the border always fits while the settings body scrolls.
     * 480dp keeps tall tablets from looking like a sidebar; short landscape
     * screens fall back to the viewport minus a small margin.
     */
    private int cardHeight() {
        int screen = mActivity.getResources().getDisplayMetrics().heightPixels;
        return Math.max(dp(240), Math.min(dp(480), screen - dp(24)));
    }

    private int dp(int value) {
        return (int) (value * mActivity.getResources().getDisplayMetrics().density + 0.5f);
    }

    private static final class SliderRow {
        LinearLayout root;
        TextView label;
        TextView value;
        SeekBar bar;
    }
}
