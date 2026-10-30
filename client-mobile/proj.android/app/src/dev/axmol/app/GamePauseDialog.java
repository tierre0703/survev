/****************************************************************************
 * GamePauseDialog.java — In-game pause menu (ARCHITECTURE.md §7, step S6).
 *
 * Styled after the web client's #ui-game-menu (client/css/game.css):
 *   - Translucent dark-green card with a tan border
 *   - Teal "pill" buttons with an inset bottom shadow and left icon
 *   - Thin teal sliders with round thumb for all volumes/sensitivities
 *   - Aim Line / Sound toggles (same config keys as SettingsModal)
 *   - Real SeekBar sliders for Master/SFX/Music volume adjustment
 *   - Sensitivity sliders (move sensitivity, aim sensitivity)
 *   - Style toggles (low graphics, show FPS, debug info)
 *   - Mute toggle with visual feedback
 *   - Return to game / Quit buttons
 *   - Quit confirmation dialog
 *   - Spectate options stub (for when dead)
 ****************************************************************************/
package dev.axmol.app;

import android.app.Activity;
import android.app.Dialog;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.graphics.Path;
import android.graphics.PixelFormat;
import android.graphics.Rect;
import android.graphics.RectF;
import android.graphics.drawable.ColorDrawable;
import android.graphics.drawable.Drawable;
import android.graphics.drawable.GradientDrawable;
import android.graphics.drawable.LayerDrawable;
import android.util.TypedValue;
import android.view.Gravity;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewGroup;
import android.view.Window;
import android.view.WindowManager;
import android.widget.FrameLayout;
import android.widget.LinearLayout;
import android.widget.ScrollView;
import android.widget.SeekBar;
import android.widget.TextView;

public class GamePauseDialog {
    // Palette lifted from the web client menu (teal pills on a dark green card).
    private static final int TEAL         = 0xFF50AFAB;
    private static final int TEAL_SHADOW  = 0xFF387C79;
    private static final int TEAL_OFF     = 0xFF4E6B65;
    private static final int TEAL_OFF_SHD = 0xFF344B47;
    private static final int PANEL_BG     = 0xE6152411;
    private static final int PANEL_BORDER = 0xFFB9AD84;
    private static final int TEXT_CREAM   = 0xFFF1EAD2;
    private static final int TEXT_LABEL   = 0xFFE6EFE2;
    private static final int TEXT_VALUE   = 0xFFA8CEC9;
    private static final int TEXT_MUTED   = 0xFFB7C4B4;
    private static final int TRACK_BG     = 0xFF2A4A40;

    private final Activity mActivity;
    private final FrameLayout mRoot;
    private final ScrollView mScroll;
    private final JavaUiHost mHost;
    private boolean mOpen = false;

    // --- widgets ---
    private final TextView mTitle;
    private final TextView mSensHeader;
    private final TextView mMasterLbl;
    private final TextView mSfxLbl;
    private final TextView mMusicLbl;
    private final TextView mMoveSensLbl;
    private final TextView mAimSensLbl;
    private final SeekBar mMasterSlider;
    private final TextView mMasterLabel;
    private final SeekBar mSfxSlider;
    private final TextView mSfxLabel;
    private final SeekBar mMusicSlider;
    private final TextView mMusicLabel;
    private final SeekBar mMoveSensSlider;
    private final TextView mMoveSensLabel;
    private final SeekBar mAimSensSlider;
    private final TextView mAimSensLabel;
    private final TextView mRenderTierBtn;
    private final TextView mShowFpsBtn;
    private final TextView mAimLineBtn;
    private final TextView mMuteBtn;
    private final TextView mReturnBtn;
    private final TextView mQuitBtn;
    private final TextView mSpectateBtn;
    private final TextView mLeaderboardBtn;
    private final TextView mStatusText;

    // RenderScale tier l10n keys (Full/Medium/Low/Potato).
    private static final String[] TIER_KEYS = {
            "pause-tier-full", "pause-tier-medium", "pause-tier-low", "pause-tier-potato"
    };

    // --- state ---
    private boolean mMuted = false;
    private boolean mShowFps = false;
    private boolean mAimLine = true;
    private float mRenderScale = RenderScale.FULL;
    private boolean mIsDead = false; // Set externally when player dies

    public GamePauseDialog(Activity activity, JavaUiHost host) {
        mActivity = activity;
        mHost = host;

        mRoot = new FrameLayout(activity);
        // Light scrim (the web menu lets the match stay readable behind it).
        mRoot.setBackgroundColor(0x36000000);

        // Fixed-height card with an internal scroll area. The title and the
        // Return/Quit footer stay visible while the settings list scrolls, so
        // the panel border is never clipped by the screen edge.
        LinearLayout card = new LinearLayout(activity);
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

        // Title
        mTitle = new TextView(activity);
        mTitle.setTextColor(TEXT_CREAM);
        mTitle.setTextSize(TypedValue.COMPLEX_UNIT_SP, 16);
        mTitle.setGravity(Gravity.CENTER);
        mTitle.setShadowLayer(dp(2), 0, dp(2), 0x55000000);
        mTitle.setIncludeFontPadding(false);
        card.addView(mTitle, fullParams(ViewGroup.LayoutParams.WRAP_CONTENT));

        addGap(card, dp(6));

        // Scrollable middle section (everything except the title and footer).
        ScrollView contentScroll = new ScrollView(activity);
        contentScroll.setVerticalScrollBarEnabled(false);
        contentScroll.setFillViewport(true);
        mScroll = contentScroll;

        LinearLayout body = new LinearLayout(activity);
        body.setOrientation(LinearLayout.VERTICAL);
        body.setPadding(0, 0, 0, dp(2));
        contentScroll.addView(body, new FrameLayout.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT));
        card.addView(contentScroll, new LinearLayout.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT, 0, 1f));

        // === Quick toggles (web menu's double-row layout) ===
        mRenderTierBtn = makeThemedButton("", PauseIcon.DISPLAY);
        compactButton(mRenderTierBtn);
        mRenderTierBtn.setOnClickListener(new View.OnClickListener() {
            @Override public void onClick(View v) { cycleRenderTier(); }
        });

        mShowFpsBtn = makeThemedButton("", PauseIcon.GAUGE);
        compactButton(mShowFpsBtn);
        mShowFpsBtn.setOnClickListener(new View.OnClickListener() {
            @Override public void onClick(View v) { toggleShowFps(); }
        });

        LinearLayout toggleRowA = new LinearLayout(activity);
        toggleRowA.setOrientation(LinearLayout.HORIZONTAL);
        toggleRowA.addView(mRenderTierBtn, halfParams(0, dp(3)));
        toggleRowA.addView(mShowFpsBtn, halfParams(dp(3), 0));
        body.addView(toggleRowA, fullParams(dp(36)));

        addGap(body, dp(6));

        mAimLineBtn = makeThemedButton("", PauseIcon.CHECK);
        compactButton(mAimLineBtn);
        mAimLineBtn.setOnClickListener(new View.OnClickListener() {
            @Override public void onClick(View v) { toggleAimLine(); }
        });

        mMuteBtn = makeThemedButton("", PauseIcon.SPEAKER);
        compactButton(mMuteBtn);
        mMuteBtn.setOnClickListener(new View.OnClickListener() {
            @Override public void onClick(View v) { toggleMute(); }
        });

        LinearLayout toggleRowB = new LinearLayout(activity);
        toggleRowB.setOrientation(LinearLayout.HORIZONTAL);
        toggleRowB.addView(mAimLineBtn, halfParams(0, dp(3)));
        toggleRowB.addView(mMuteBtn, halfParams(dp(3), 0));
        body.addView(toggleRowB, fullParams(dp(36)));

        addGap(body, dp(8));

        // === Volume Sliders ===
        SliderRow masterRow = makeSliderRow(100);
        mMasterLbl = masterRow.label;
        mMasterLabel = masterRow.value;
        mMasterSlider = masterRow.bar;
        mMasterSlider.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {
            @Override public void onProgressChanged(SeekBar sb, int progress, boolean fromUser) {
                float val = progress / 100f;
                mMasterLabel.setText(String.format("%.0f%%", val * 100));
                if (fromUser) SurvBridge.setAudio("master", val);
            }
            @Override public void onStartTrackingTouch(SeekBar sb) {}
            @Override public void onStopTrackingTouch(SeekBar sb) { SurvBridge.saveConfig(); }
        });
        body.addView(masterRow.root, fullParams(ViewGroup.LayoutParams.WRAP_CONTENT));

        SliderRow sfxRow = makeSliderRow(100);
        mSfxLbl = sfxRow.label;
        mSfxLabel = sfxRow.value;
        mSfxSlider = sfxRow.bar;
        mSfxSlider.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {
            @Override public void onProgressChanged(SeekBar sb, int progress, boolean fromUser) {
                float val = progress / 100f;
                mSfxLabel.setText(String.format("%.0f%%", val * 100));
                if (fromUser) SurvBridge.setAudio("sound", val);
            }
            @Override public void onStartTrackingTouch(SeekBar sb) {}
            @Override public void onStopTrackingTouch(SeekBar sb) { SurvBridge.saveConfig(); }
        });
        body.addView(sfxRow.root, fullParams(ViewGroup.LayoutParams.WRAP_CONTENT));

        SliderRow musicRow = makeSliderRow(60);
        mMusicLbl = musicRow.label;
        mMusicLabel = musicRow.value;
        mMusicSlider = musicRow.bar;
        mMusicSlider.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {
            @Override public void onProgressChanged(SeekBar sb, int progress, boolean fromUser) {
                float val = progress / 100f;
                mMusicLabel.setText(String.format("%.0f%%", val * 100));
                if (fromUser) SurvBridge.setAudio("music", val);
            }
            @Override public void onStartTrackingTouch(SeekBar sb) {}
            @Override public void onStopTrackingTouch(SeekBar sb) { SurvBridge.saveConfig(); }
        });
        body.addView(musicRow.root, fullParams(ViewGroup.LayoutParams.WRAP_CONTENT));

        addGap(body, dp(4));

        // === Sensitivity Sliders ===
        mSensHeader = makeSectionHeader("");
        body.addView(mSensHeader, fullParams(ViewGroup.LayoutParams.WRAP_CONTENT));

        SliderRow moveSensRow = makeSliderRow(50);
        mMoveSensLbl = moveSensRow.label;
        mMoveSensLabel = moveSensRow.value;
        mMoveSensSlider = moveSensRow.bar;
        mMoveSensSlider.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {
            @Override public void onProgressChanged(SeekBar sb, int progress, boolean fromUser) {
                float val = progress / 100f;
                mMoveSensLabel.setText(String.format("%.0f%%", val * 100));
                if (fromUser) {
                    SurvBridge.setConfigValue("touchMoveSensitivity", String.valueOf(val));
                    SurvBridge.saveConfig();
                }
            }
            @Override public void onStartTrackingTouch(SeekBar sb) {}
            @Override public void onStopTrackingTouch(SeekBar sb) {}
        });
        body.addView(moveSensRow.root, fullParams(ViewGroup.LayoutParams.WRAP_CONTENT));

        SliderRow aimSensRow = makeSliderRow(50);
        mAimSensLbl = aimSensRow.label;
        mAimSensLabel = aimSensRow.value;
        mAimSensSlider = aimSensRow.bar;
        mAimSensSlider.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {
            @Override public void onProgressChanged(SeekBar sb, int progress, boolean fromUser) {
                float val = progress / 100f;
                mAimSensLabel.setText(String.format("%.0f%%", val * 100));
                if (fromUser) {
                    SurvBridge.setConfigValue("touchAimSensitivity", String.valueOf(val));
                    SurvBridge.saveConfig();
                }
            }
            @Override public void onStartTrackingTouch(SeekBar sb) {}
            @Override public void onStopTrackingTouch(SeekBar sb) {}
        });
        body.addView(aimSensRow.root, fullParams(ViewGroup.LayoutParams.WRAP_CONTENT));

        addGap(body, dp(8));

        // Spectate button (visible only when dead)
        mSpectateBtn = makeThemedButton("", PauseIcon.EYE);
        mSpectateBtn.setVisibility(View.GONE); // Hidden until player dies
        mSpectateBtn.setOnClickListener(new View.OnClickListener() {
            @Override public void onClick(View v) {
                // spectateMsg.ts Begin; the C++ HUD takes over the spectating
                // overlay once the server moves activePlayerId.
                hide();
                SurvBridge.spectate(1);
            }
        });
        body.addView(mSpectateBtn, fullParams(dp(36)));

        // Live in-game leaderboard (Game::leaderboardJson -> Java modal).
        mLeaderboardBtn = makeThemedButton("", PauseIcon.TROPHY);
        mLeaderboardBtn.setOnClickListener(new View.OnClickListener() {
            @Override public void onClick(View v) {
                SurvBridge.onRequestLeaderboard();
            }
        });
        body.addView(mLeaderboardBtn, fullParams(dp(36)));

        // Status text
        mStatusText = new TextView(activity);
        mStatusText.setTextColor(TEXT_MUTED);
        mStatusText.setTextSize(TypedValue.COMPLEX_UNIT_SP, 10);
        mStatusText.setGravity(Gravity.CENTER);
        mStatusText.setIncludeFontPadding(false);
        body.addView(mStatusText, fullParams(ViewGroup.LayoutParams.WRAP_CONTENT));

        addGap(card, dp(8));

        // === Action Buttons (fixed footer) ===
        mReturnBtn = makeThemedButton("", PauseIcon.PLAY);
        mReturnBtn.setOnClickListener(new View.OnClickListener() {
            @Override public void onClick(View v) { hide(); }
        });
        card.addView(mReturnBtn, fullParams(dp(40)));

        addGap(card, dp(6));

        mQuitBtn = makeThemedButton("", PauseIcon.POWER);
        mQuitBtn.setOnClickListener(new View.OnClickListener() {
            @Override public void onClick(View v) { confirmQuit(); }
        });
        card.addView(mQuitBtn, fullParams(dp(40)));

        mRoot.addView(card);

        // A newly constructed dialog is hidden until show(); ensurePause() adds
        // it to the overlay container before the user opens it.
        mRoot.setVisibility(View.GONE);

        // Strings are (re)applied on show() so a language change in the menu
        // reaches the dialog that was built at app start.
        applyLocalizedStrings();
        seedFromConfig();
    }

    // ===== Helpers =====

    /** Re-applies every static label for the active locale (see show()). */
    private void applyLocalizedStrings() {
        mTitle.setText(L10n.t("pause-paused", "PAUSED"));
        mMasterLbl.setText(L10n.t("index-master-volume", "Master Volume"));
        mSfxLbl.setText(L10n.t("index-sfx-volume", "SFX Volume"));
        mMusicLbl.setText(L10n.t("index-music-volume", "Music Volume"));
        mSensHeader.setText(L10n.t("pause-sensitivity", "Sensitivity"));
        mMoveSensLbl.setText(L10n.t("pause-move", "Move"));
        mAimSensLbl.setText(L10n.t("pause-aim", "Aim"));
        mSpectateBtn.setText(L10n.t("game-spectate", "Spectate"));
        mLeaderboardBtn.setText(L10n.t("index-leaderboards", "Leaderboards"));
        mReturnBtn.setText(L10n.t("game-return-to-game", "Return to Game"));
        mQuitBtn.setText(L10n.t("game-quit-game", "Quit Game"));
        mAimLineBtn.setText(L10n.t("game-aim-line", "Aim Line"));
        updateRenderTierButton();
        applyShowFps();
        updateAimLineButton();
        updateMuteButton();
    }

    /** Web menu card is ~190px on a phone; keep it narrow like the reference. */
    private int cardWidth() {
        int screen = mActivity.getResources().getDisplayMetrics().widthPixels;
        return Math.min(dp(250), screen - dp(24));
    }

    /**
     * Cap the card so the border always fits, while the middle list scrolls.
     * The reference menu is ~78% of the viewport height; 440dp keeps shorter
     * landscape screens usable and taller tablets from looking like a sidebar.
     */
    private int cardHeight() {
        int screen = mActivity.getResources().getDisplayMetrics().heightPixels;
        return Math.max(dp(240), Math.min(dp(440), screen - dp(24)));
    }

    private LinearLayout.LayoutParams fullParams(int height) {
        return new LinearLayout.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT, height);
    }

    private void addGap(LinearLayout parent, int height) {
        View gap = new View(mActivity);
        gap.setLayoutParams(new ViewGroup.LayoutParams(1, height));
        parent.addView(gap);
    }

    /** Horizontal label/value line above a full-width slider (web slider row). */
    private SliderRow makeSliderRow(int defaultValue) {
        SliderRow row = new SliderRow();

        LinearLayout root = new LinearLayout(mActivity);
        root.setOrientation(LinearLayout.VERTICAL);
        root.setPadding(0, dp(4), 0, dp(2));

        LinearLayout labelRow = new LinearLayout(mActivity);
        labelRow.setOrientation(LinearLayout.HORIZONTAL);
        labelRow.setGravity(Gravity.CENTER_VERTICAL);

        TextView label = new TextView(mActivity);
        label.setTextColor(TEXT_LABEL);
        label.setTextSize(TypedValue.COMPLEX_UNIT_SP, 13);
        label.setIncludeFontPadding(false);
        label.setLayoutParams(new LinearLayout.LayoutParams(
                0, ViewGroup.LayoutParams.WRAP_CONTENT, 1f));
        labelRow.addView(label);

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
        row.label = label;
        row.value = value;
        row.bar = slider;
        return row;
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
     * Thin rounded track with a teal filled portion. A plain LayerDrawable was
     * stretched to the SeekBar height, so the track read as a solid block (and
     * the web look needs a thin line); drawing it directly keeps a 6dp track
     * centred under the 22dp thumb.
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
        @Override public void setColorFilter(android.graphics.ColorFilter cf) { mFilter = cf; invalidateSelf(); }
        @Override public int getOpacity() { return PixelFormat.TRANSLUCENT; }
    }

    private TextView makeSectionHeader(String text) {
        TextView header = new TextView(mActivity);
        header.setText(text);
        header.setTextColor(TEXT_CREAM);
        header.setTextSize(TypedValue.COMPLEX_UNIT_SP, 12);
        header.setTypeface(android.graphics.Typeface.DEFAULT_BOLD);
        header.setIncludeFontPadding(false);
        header.setPadding(0, dp(6), 0, dp(2));
        return header;
    }

    private TextView makeThemedButton(String label, int icon) {
        return makeThemedButton(label, icon, TEAL, TEAL_SHADOW);
    }

    /**
     * Teal pill with a darker inset bottom edge (MenuFragment's buttonBg) and
     * a white left icon, mirroring the web `.btn-game-menu` rows.
     */
    private TextView makeThemedButton(String label, int icon, final int body, final int shadow) {
        TextView btn = new TextView(mActivity);
        btn.setText(label);
        btn.setTextColor(Color.WHITE);
        btn.setTextSize(TypedValue.COMPLEX_UNIT_SP, 13);
        btn.setGravity(Gravity.CENTER);
        btn.setIncludeFontPadding(false);
        btn.setShadowLayer(dp(1), 0, dp(1), 0x55000000);
        btn.setPadding(dp(14), 0, dp(14), 0);
        btn.setCompoundDrawablePadding(dp(8));
        btn.setClickable(true);
        setButtonIcon(btn, icon);
        btn.setBackground(buttonBg(body, shadow));
        btn.setOnTouchListener(new View.OnTouchListener() {
            @Override public boolean onTouch(View v, MotionEvent e) {
                int a = e.getActionMasked();
                if (a == MotionEvent.ACTION_DOWN) {
                    v.setAlpha(0.72f);
                } else if (a == MotionEvent.ACTION_UP || a == MotionEvent.ACTION_CANCEL) {
                    v.setAlpha(1f);
                }
                return false;
            }
        });
        return btn;
    }

    private void setButtonIcon(TextView btn, int icon) {
        Drawable drawable = new PauseIcon(icon, dp(18), Color.WHITE);
        btn.setCompoundDrawablesWithIntrinsicBounds(drawable, null, null, null);
    }

    /** Tighter padding for a half-width button in the web's double row. */
    private void compactButton(TextView btn) {
        btn.setTextSize(TypedValue.COMPLEX_UNIT_SP, 11);
        btn.setPadding(dp(4), 0, dp(4), 0);
        btn.setCompoundDrawablePadding(dp(3));
    }

    /** Themed pill without a left icon (dialog Cancel/Quit actions). */
    private TextView makePlainButton(String label, int body, int shadow) {
        TextView btn = makeThemedButton(label, PauseIcon.CROSSHAIR, body, shadow);
        btn.setCompoundDrawablesWithIntrinsicBounds(null, null, null, null);
        btn.setPadding(dp(8), 0, dp(8), 0);
        return btn;
    }

    private LinearLayout.LayoutParams halfParams(int leftMargin, int rightMargin) {
        LinearLayout.LayoutParams lp = new LinearLayout.LayoutParams(
                0, ViewGroup.LayoutParams.MATCH_PARENT, 1f);
        lp.leftMargin = leftMargin;
        lp.rightMargin = rightMargin;
        return lp;
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

    /** "Show FPS": drives the engine stats overlay (persisted as `showFps`). */
    private void toggleShowFps() {
        mShowFps = !mShowFps;
        applyShowFps();
        SurvBridge.setShowFps(mShowFps);
    }

    private void applyShowFps() {
        mShowFpsBtn.setText(L10n.t("pause-show-fps", "Show FPS"));
        setButtonTone(mShowFpsBtn, mShowFps);
    }

    /** "Aim Line" toggle (same `touchAimLine` key as SettingsModal). */
    private void toggleAimLine() {
        mAimLine = !mAimLine;
        SurvBridge.setConfigValue("touchAimLine", mAimLine ? "true" : "false");
        SurvBridge.saveConfig();
        updateAimLineButton();
    }

    private void updateAimLineButton() {
        setButtonTone(mAimLineBtn, mAimLine);
        setButtonIcon(mAimLineBtn, mAimLine ? PauseIcon.CHECK : PauseIcon.CROSSHAIR);
    }

    /**
     * "GFX" cycles the render-resolution tiers (Full/Medium/Low/Potato) and
     * persists the tier as `renderScale`; the C++ config applies it again on the
     * next launch. Lower tiers shrink the EGL surface, so layout/HUD sizes are
     * unchanged but fill-rate (and battery use) drop.
     */
    private void cycleRenderTier() {
        mRenderScale = RenderScale.next(mRenderScale);
        RenderScale.apply(mRenderScale);
        SurvBridge.setConfigValue("renderScale", String.valueOf(mRenderScale));
        SurvBridge.saveConfig();
        updateRenderTierButton();
    }

    private void updateRenderTierButton() {
        final String tier = localizedTierName(mRenderScale);
        mRenderTierBtn.setText(L10n.t("pause-gfx", "GFX") + ": " + tier);
        setButtonTone(mRenderTierBtn, mRenderScale < 0.999f);
    }

    /** Localized RenderScale.tierName(); custom persisted scales keep a %. */
    private String localizedTierName(float value) {
        final float scale = RenderScale.normalize(value);
        for (int i = 0; i < RenderScale.TIERS.length; i++) {
            if (Math.abs(scale - RenderScale.TIERS[i]) < 0.005f) {
                return L10n.t(TIER_KEYS[i], RenderScale.TIER_NAMES[i]);
            }
        }
        return L10n.t("pause-tier-custom", "Custom") + " "
                + Math.round(scale * 100) + "%";
    }

    private void toggleMute() {
        mMuted = !mMuted;
        // UiBridge reads non-zero as mute (MenuFragment uses the same mapping).
        SurvBridge.setAudio("mute", mMuted ? 1.0f : 0.0f);
        SurvBridge.saveConfig();
        updateMuteButton();
    }

    private void updateMuteButton() {
        if (mMuted) {
            mMuteBtn.setText(L10n.t("pause-muted", "Muted"));
            setButtonTone(mMuteBtn, false);
            setButtonIcon(mMuteBtn, PauseIcon.MUTED);
        } else {
            mMuteBtn.setText(L10n.t("game-sound", "Sound"));
            setButtonTone(mMuteBtn, true);
            setButtonIcon(mMuteBtn, PauseIcon.SPEAKER);
        }
    }

    private void confirmQuit() {
        final Dialog dialog = new Dialog(mActivity);
        dialog.requestWindowFeature(Window.FEATURE_NO_TITLE);

        LinearLayout panel = new LinearLayout(mActivity);
        panel.setOrientation(LinearLayout.VERTICAL);
        GradientDrawable bg = new GradientDrawable();
        bg.setColor(PANEL_BG);
        bg.setCornerRadius(dp(8));
        bg.setStroke(dp(2), PANEL_BORDER);
        panel.setBackground(bg);
        panel.setPadding(dp(16), dp(14), dp(16), dp(14));

        TextView title = new TextView(mActivity);
        title.setText(L10n.t("pause-quit-title", "Quit Game?"));
        title.setTextColor(TEXT_CREAM);
        title.setTextSize(TypedValue.COMPLEX_UNIT_SP, 16);
        title.setTypeface(android.graphics.Typeface.DEFAULT_BOLD);
        title.setIncludeFontPadding(false);
        panel.addView(title, fullParams(ViewGroup.LayoutParams.WRAP_CONTENT));

        addGap(panel, dp(8));

        TextView message = new TextView(mActivity);
        message.setText(L10n.t("pause-quit-message",
                "Are you sure you want to quit? Any unsaved progress will be lost."));
        message.setTextColor(TEXT_LABEL);
        message.setTextSize(TypedValue.COMPLEX_UNIT_SP, 13);
        message.setIncludeFontPadding(false);
        panel.addView(message, fullParams(ViewGroup.LayoutParams.WRAP_CONTENT));

        addGap(panel, dp(12));

        LinearLayout actions = new LinearLayout(mActivity);
        actions.setOrientation(LinearLayout.HORIZONTAL);

        TextView cancel = makePlainButton(L10n.t("game-cancel", "Cancel"), TEAL_OFF, TEAL_OFF_SHD);
        cancel.setOnClickListener(new View.OnClickListener() {
            @Override public void onClick(View v) { dialog.dismiss(); }
        });
        actions.addView(cancel, halfParams(0, dp(3)));

        TextView quit = makePlainButton(L10n.t("pause-quit-confirm", "Quit"), TEAL, TEAL_SHADOW);
        quit.setOnClickListener(new View.OnClickListener() {
            @Override public void onClick(View v) {
                dialog.dismiss();
                if (mHost != null) mHost.quitToMenu();
            }
        });
        actions.addView(quit, halfParams(dp(3), 0));

        panel.addView(actions, fullParams(dp(40)));

        dialog.setContentView(panel);
        Window window = dialog.getWindow();
        if (window != null) {
            window.setBackgroundDrawable(new ColorDrawable(Color.TRANSPARENT));
            window.addFlags(WindowManager.LayoutParams.FLAG_DIM_BEHIND);
            window.setDimAmount(0.45f);
        }
        dialog.show();
        if (window != null) {
            int width = Math.min(dp(300), mActivity.getResources().getDisplayMetrics().widthPixels - dp(24));
            window.setLayout(width, ViewGroup.LayoutParams.WRAP_CONTENT);
        }
    }

    private void seedFromConfig() {
        String config = SurvBridge.getConfigJson();
        if (config == null || config.isEmpty()) return;

        mMuted = Json.peekBool(config, "muteAudio", false);
        updateMuteButton();

        mShowFps = Json.peekBool(config, "showFps", false);
        applyShowFps();

        mAimLine = Json.peekBool(config, "touchAimLine", true);
        updateAimLineButton();

        mRenderScale = RenderScale.persisted();
        updateRenderTierButton();

        float master = Json.peekFloat(config, "masterVolume", 1.0f);
        mMasterSlider.setProgress(Math.round(master * 100));
        mMasterLabel.setText(String.format("%.0f%%", master * 100));
        float sfx = Json.peekFloat(config, "soundVolume", 1.0f);
        mSfxSlider.setProgress(Math.round(sfx * 100));
        mSfxLabel.setText(String.format("%.0f%%", sfx * 100));
        float music = Json.peekFloat(config, "musicVolume", 1.0f);
        mMusicSlider.setProgress(Math.round(music * 100));
        mMusicLabel.setText(String.format("%.0f%%", music * 100));
    }

    // ===== Public API =====

    public View root() { return mRoot; }

    public void show() {
        if (mOpen) return;
        mOpen = true;
        mRoot.setVisibility(View.VISIBLE);
        applyLocalizedStrings();
        seedFromConfig();
        // Reopen at the title; an earlier scroll should not hide PAUSED/volume.
        mScroll.post(new Runnable() {
            @Override public void run() { mScroll.scrollTo(0, 0); }
        });
    }

    public void hide() {
        if (!mOpen) return;
        mOpen = false;
        mRoot.setVisibility(View.GONE);
    }

    public boolean isOpen() { return mOpen; }

    /** Call when player dies to show spectate option */
    public void setPlayerDead(boolean dead) {
        mIsDead = dead;
        mSpectateBtn.setVisibility(dead ? View.VISIBLE : View.GONE);
        mReturnBtn.setText(L10n.t("game-return-to-game", "Return to Game"));
    }

    public void setStatus(String text) {
        mStatusText.setText(text != null ? text : "");
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

    /**
     * Tiny line-art icon set drawn in code (the Java UI ships no drawable
     * assets). All shapes use a 0..1 box around the drawable bounds so they
     * stay crisp at any density.
     */
    private static final class PauseIcon extends Drawable {
        static final int SETTINGS = 0;
        static final int DISPLAY = 1;
        static final int GAUGE = 2;
        static final int CROSSHAIR = 3;
        static final int CHECK = 4;
        static final int SPEAKER = 5;
        static final int MUTED = 6;
        static final int POWER = 7;
        static final int PLAY = 8;
        static final int TROPHY = 9;
        static final int EYE = 10;

        private final int mKind;
        private final int mSize;
        private final Paint mStroke = new Paint(Paint.ANTI_ALIAS_FLAG);
        private final Paint mFill = new Paint(Paint.ANTI_ALIAS_FLAG);

        PauseIcon(int kind, int size, int color) {
            mKind = kind;
            mSize = size;
            mStroke.setColor(color);
            mStroke.setStyle(Paint.Style.STROKE);
            mStroke.setStrokeCap(Paint.Cap.ROUND);
            mStroke.setStrokeJoin(Paint.Join.ROUND);
            mFill.setColor(color);
            mFill.setStyle(Paint.Style.FILL);
        }

        @Override public void draw(Canvas canvas) {
            Rect b = getBounds();
            float w = b.width(), h = b.height();
            float s = Math.min(w, h);
            float cx = b.left + w / 2f, cy = b.top + h / 2f;
            mStroke.setStrokeWidth(Math.max(1.5f, s * 0.085f));

            switch (mKind) {
                case SETTINGS: {
                    // Six teeth around a ring (web cog.svg stand-in).
                    for (int i = 0; i < 6; i++) {
                        canvas.save();
                        canvas.rotate(i * 60f, cx, cy);
                        canvas.drawRoundRect(new RectF(
                                cx - s * 0.055f, cy - s * 0.42f,
                                cx + s * 0.055f, cy - s * 0.28f),
                                s * 0.03f, s * 0.03f, mFill);
                        canvas.restore();
                    }
                    canvas.drawCircle(cx, cy, s * 0.17f, mStroke);
                    break;
                }
                case DISPLAY: {
                    canvas.drawRoundRect(new RectF(
                            cx - s * 0.36f, cy - s * 0.31f,
                            cx + s * 0.36f, cy + s * 0.17f),
                            s * 0.07f, s * 0.07f, mStroke);
                    canvas.drawLine(cx, cy + s * 0.17f, cx, cy + s * 0.30f, mStroke);
                    canvas.drawLine(cx - s * 0.17f, cy + s * 0.30f,
                            cx + s * 0.17f, cy + s * 0.30f, mStroke);
                    break;
                }
                case GAUGE: {
                    canvas.drawArc(new RectF(
                            cx - s * 0.32f, cy - s * 0.22f,
                            cx + s * 0.32f, cy + s * 0.42f),
                            180f, 180f, false, mStroke);
                    canvas.drawLine(cx, cy + s * 0.12f,
                            cx + s * 0.16f, cy - s * 0.14f, mStroke);
                    canvas.drawCircle(cx, cy + s * 0.12f, s * 0.055f, mFill);
                    break;
                }
                case CROSSHAIR: {
                    canvas.drawCircle(cx, cy, s * 0.25f, mStroke);
                    canvas.drawLine(cx, cy - s * 0.42f, cx, cy - s * 0.30f, mStroke);
                    canvas.drawLine(cx, cy + s * 0.30f, cx, cy + s * 0.42f, mStroke);
                    canvas.drawLine(cx - s * 0.42f, cy, cx - s * 0.30f, cy, mStroke);
                    canvas.drawLine(cx + s * 0.30f, cy, cx + s * 0.42f, cy, mStroke);
                    canvas.drawCircle(cx, cy, s * 0.05f, mFill);
                    break;
                }
                case CHECK: {
                    Path p = new Path();
                    p.moveTo(cx - s * 0.28f, cy + s * 0.01f);
                    p.lineTo(cx - s * 0.07f, cy + s * 0.22f);
                    p.lineTo(cx + s * 0.30f, cy - s * 0.20f);
                    canvas.drawPath(p, mStroke);
                    break;
                }
                case SPEAKER:
                case MUTED: {
                    Path p = new Path();
                    p.moveTo(cx - s * 0.37f, cy - s * 0.11f);
                    p.lineTo(cx - s * 0.22f, cy - s * 0.11f);
                    p.lineTo(cx - s * 0.02f, cy - s * 0.30f);
                    p.lineTo(cx - s * 0.02f, cy + s * 0.30f);
                    p.lineTo(cx - s * 0.22f, cy + s * 0.11f);
                    p.lineTo(cx - s * 0.37f, cy + s * 0.11f);
                    p.close();
                    canvas.drawPath(p, mFill);
                    if (mKind == SPEAKER) {
                        canvas.drawArc(new RectF(
                                cx + s * 0.02f, cy - s * 0.18f,
                                cx + s * 0.32f, cy + s * 0.18f),
                                -55f, 110f, false, mStroke);
                        canvas.drawArc(new RectF(
                                cx - s * 0.06f, cy - s * 0.34f,
                                cx + s * 0.42f, cy + s * 0.34f),
                                -55f, 110f, false, mStroke);
                    } else {
                        canvas.drawLine(cx + s * 0.10f, cy - s * 0.14f,
                                cx + s * 0.34f, cy + s * 0.16f, mStroke);
                        canvas.drawLine(cx + s * 0.34f, cy - s * 0.14f,
                                cx + s * 0.10f, cy + s * 0.16f, mStroke);
                    }
                    break;
                }
                case POWER: {
                    canvas.drawArc(new RectF(
                            cx - s * 0.28f, cy - s * 0.28f,
                            cx + s * 0.28f, cy + s * 0.28f),
                            -55f, 290f, false, mStroke);
                    canvas.drawLine(cx, cy - s * 0.40f, cx, cy, mStroke);
                    break;
                }
                case PLAY: {
                    Path p = new Path();
                    p.moveTo(cx - s * 0.16f, cy - s * 0.28f);
                    p.lineTo(cx + s * 0.28f, cy);
                    p.lineTo(cx - s * 0.16f, cy + s * 0.28f);
                    p.close();
                    canvas.drawPath(p, mFill);
                    break;
                }
                case TROPHY: {
                    Path cup = new Path();
                    cup.moveTo(cx - s * 0.26f, cy - s * 0.30f);
                    cup.lineTo(cx + s * 0.26f, cy - s * 0.30f);
                    cup.lineTo(cx + s * 0.20f, cy + s * 0.02f);
                    cup.quadTo(cx, cy + s * 0.30f, cx - s * 0.20f, cy + s * 0.02f);
                    cup.close();
                    canvas.drawPath(cup, mStroke);
                    canvas.drawArc(new RectF(
                            cx - s * 0.46f, cy - s * 0.26f,
                            cx - s * 0.16f, cy + s * 0.08f),
                            90f, 180f, false, mStroke);
                    canvas.drawArc(new RectF(
                            cx + s * 0.16f, cy - s * 0.26f,
                            cx + s * 0.46f, cy + s * 0.08f),
                            270f, 180f, false, mStroke);
                    canvas.drawLine(cx, cy + s * 0.30f, cx, cy + s * 0.39f, mStroke);
                    canvas.drawLine(cx - s * 0.17f, cy + s * 0.41f,
                            cx + s * 0.17f, cy + s * 0.41f, mStroke);
                    break;
                }
                case EYE: {
                    Path eye = new Path();
                    eye.moveTo(cx - s * 0.40f, cy);
                    eye.quadTo(cx, cy - s * 0.32f, cx + s * 0.40f, cy);
                    eye.quadTo(cx, cy + s * 0.32f, cx - s * 0.40f, cy);
                    canvas.drawPath(eye, mStroke);
                    canvas.drawCircle(cx, cy, s * 0.11f, mFill);
                    break;
                }
                default:
                    break;
            }
        }

        @Override public void setAlpha(int alpha) { mStroke.setAlpha(alpha); mFill.setAlpha(alpha); }
        @Override public void setColorFilter(android.graphics.ColorFilter cf) {
            mStroke.setColorFilter(cf);
            mFill.setColorFilter(cf);
        }
        @Override public int getOpacity() { return PixelFormat.TRANSLUCENT; }
        @Override public int getIntrinsicWidth() { return mSize; }
        @Override public int getIntrinsicHeight() { return mSize; }
    }
}
