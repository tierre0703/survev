/****************************************************************************
 * GamePauseDialog.java — In-game pause menu (ARCHITECTURE.md §7, step S6).
 *
 * Enhanced pause dialog with:
 *   - Real SeekBar sliders for Master/SFX/Music volume adjustment
 *   - Sensitivity sliders (move sensitivity, aim sensitivity)
 *   - Style toggles (low graphics, show FPS, debug info)
 *   - Mute toggle with visual feedback
 *   - Return to game / Quit buttons
 *   - Quit confirmation dialog
 *   - Spectate options stub (for when dead)
 *
 * Colors match web client pause overlay.
 ****************************************************************************/
package dev.axmol.app;

import android.app.Activity;
import android.app.AlertDialog;
import android.graphics.Color;
import android.graphics.drawable.GradientDrawable;
import android.util.Log;
import android.util.TypedValue;
import android.view.Gravity;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.FrameLayout;
import android.widget.LinearLayout;
import android.widget.ScrollView;
import android.widget.SeekBar;
import android.widget.TextView;

public class GamePauseDialog {
    private static final String TAG = "GamePauseDialog";

    private static final int GREEN       = 0xFF83AF50;
    private static final int GREEN_SHADOW = 0xFF5B7A38;
    private static final int RED         = 0xFFE57373;
    private static final int RED_SHADOW  = 0xFFC62828;
    private static final int GREY        = 0xFF7A7A7A;
    private static final int PANEL_BG    = 0xEE1B2A12;

    private final Activity mActivity;
    private final FrameLayout mRoot;
    private final JavaUiHost mHost;
    private boolean mOpen = false;

    // --- widgets ---
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
    private final Button mRenderTierBtn;
    private final Button mShowFpsBtn;
    private final Button mMuteBtn;
    private final Button mReturnBtn;
    private final Button mQuitBtn;
    private final Button mSpectateBtn;
    private final TextView mStatusText;

    // --- state ---
    private boolean mMuted = false;
    private boolean mShowFps = false;
    private float mRenderScale = RenderScale.FULL;
    private boolean mIsDead = false; // Set externally when player dies

    public GamePauseDialog(Activity activity, JavaUiHost host) {
        mActivity = activity;
        mHost = host;

        mRoot = new FrameLayout(activity);
        mRoot.setBackgroundColor(0xCC000000);

        // Full-height ScrollView for landscape responsiveness
        ScrollView outerScroll = new ScrollView(activity);
        outerScroll.setLayoutParams(new FrameLayout.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.MATCH_PARENT));

        // Centered card
        LinearLayout card = new LinearLayout(activity);
        card.setOrientation(LinearLayout.VERTICAL);
        card.setGravity(Gravity.CENTER);
        card.setBackgroundColor(PANEL_BG);
        card.setPadding(dp(20), dp(16), dp(20), dp(16));
        FrameLayout.LayoutParams cardParams = new FrameLayout.LayoutParams(
                dp(320), ViewGroup.LayoutParams.WRAP_CONTENT);
        cardParams.gravity = Gravity.CENTER;
        card.setLayoutParams(cardParams);

        // Title
        TextView title = new TextView(activity);
        title.setText("\u23F8 PAUSED");
        title.setTextColor(Color.WHITE);
        title.setTextSize(TypedValue.COMPLEX_UNIT_SP, 22);
        title.setGravity(Gravity.CENTER);
        title.setShadowLayer(dp(2), 0, dp(2), 0x44000000);
        card.addView(title);

        addGap(card, dp(12));

        // === Volume Sliders ===
        TextView volHeader = new TextView(activity);
        volHeader.setText("\uD83C\uDFB5 Volume");
        volHeader.setTextColor(0xFFFFD700);
        volHeader.setTextSize(TypedValue.COMPLEX_UNIT_SP, 14);
        volHeader.setTypeface(android.graphics.Typeface.DEFAULT_BOLD);
        card.addView(volHeader);

        // Master volume
        LinearLayout masterRow = makeSliderRow("Master", 100);
        mMasterLabel = (TextView) masterRow.getChildAt(1);
        mMasterSlider = (SeekBar) masterRow.getChildAt(2);
        mMasterSlider.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {
            @Override public void onProgressChanged(SeekBar sb, int progress, boolean fromUser) {
                float val = progress / 100f;
                mMasterLabel.setText(String.format("%.0f%%", val * 100));
                if (fromUser) SurvBridge.setAudio("master", val);
            }
            @Override public void onStartTrackingTouch(SeekBar sb) {}
            @Override public void onStopTrackingTouch(SeekBar sb) { SurvBridge.saveConfig(); }
        });
        card.addView(masterRow);

        // SFX volume
        LinearLayout sfxRow = makeSliderRow("SFX", 100);
        mSfxLabel = (TextView) sfxRow.getChildAt(1);
        mSfxSlider = (SeekBar) sfxRow.getChildAt(2);
        mSfxSlider.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {
            @Override public void onProgressChanged(SeekBar sb, int progress, boolean fromUser) {
                float val = progress / 100f;
                mSfxLabel.setText(String.format("%.0f%%", val * 100));
                if (fromUser) SurvBridge.setAudio("sound", val);
            }
            @Override public void onStartTrackingTouch(SeekBar sb) {}
            @Override public void onStopTrackingTouch(SeekBar sb) { SurvBridge.saveConfig(); }
        });
        card.addView(sfxRow);

        // Music volume
        LinearLayout musicRow = makeSliderRow("Music", 60);
        mMusicLabel = (TextView) musicRow.getChildAt(1);
        mMusicSlider = (SeekBar) musicRow.getChildAt(2);
        mMusicSlider.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {
            @Override public void onProgressChanged(SeekBar sb, int progress, boolean fromUser) {
                float val = progress / 100f;
                mMusicLabel.setText(String.format("%.0f%%", val * 100));
                if (fromUser) SurvBridge.setAudio("music", val);
            }
            @Override public void onStartTrackingTouch(SeekBar sb) {}
            @Override public void onStopTrackingTouch(SeekBar sb) { SurvBridge.saveConfig(); }
        });
        card.addView(musicRow);

        addGap(card, dp(10));

        // === Sensitivity Sliders ===
        TextView sensHeader = new TextView(activity);
        sensHeader.setText("\uD83C\uDFAF Sensitivity");
        sensHeader.setTextColor(0xFFFFD700);
        sensHeader.setTextSize(TypedValue.COMPLEX_UNIT_SP, 14);
        sensHeader.setTypeface(android.graphics.Typeface.DEFAULT_BOLD);
        card.addView(sensHeader);

        // Move sensitivity
        LinearLayout moveSensRow = makeSliderRow("Move", 50);
        mMoveSensLabel = (TextView) moveSensRow.getChildAt(1);
        mMoveSensSlider = (SeekBar) moveSensRow.getChildAt(2);
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
        card.addView(moveSensRow);

        // Aim sensitivity
        LinearLayout aimSensRow = makeSliderRow("Aim", 50);
        mAimSensLabel = (TextView) aimSensRow.getChildAt(1);
        mAimSensSlider = (SeekBar) aimSensRow.getChildAt(2);
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
        card.addView(aimSensRow);

        addGap(card, dp(10));

        // === Toggles Row ===
        LinearLayout togglesRow = new LinearLayout(activity);
        togglesRow.setOrientation(LinearLayout.HORIZONTAL);

        mRenderTierBtn = makeToggleButton("GFX: Full");
        mRenderTierBtn.setOnClickListener(new View.OnClickListener() {
            @Override public void onClick(View v) { cycleRenderTier(); }
        });
        togglesRow.addView(mRenderTierBtn, halfParams());

        mShowFpsBtn = makeToggleButton("Show FPS");
        mShowFpsBtn.setOnClickListener(new View.OnClickListener() {
            @Override public void onClick(View v) { toggleShowFps(); }
        });
        togglesRow.addView(mShowFpsBtn, halfParams());

        mMuteBtn = makeToggleButton("\uD83D\uDD07 Mute");
        mMuteBtn.setOnClickListener(new View.OnClickListener() {
            @Override public void onClick(View v) { toggleMute(); }
        });
        card.addView(mMuteBtn);
        
        card.addView(togglesRow);

        addGap(card, dp(8));

        // Spectate button (visible only when dead)
        mSpectateBtn = new Button(activity);
        mSpectateBtn.setText("\uD83D\uDCF1 " + L10n.t("game-spectate", "Spectate"));
        mSpectateBtn.setBackgroundColor(0xFF4A4A4A);
        mSpectateBtn.setTextColor(Color.WHITE);
        mSpectateBtn.setVisibility(View.GONE); // Hidden until player dies
        mSpectateBtn.setOnClickListener(new View.OnClickListener() {
            @Override public void onClick(View v) {
                // spectateMsg.ts Begin; the C++ HUD takes over the spectating
                // overlay once the server moves activePlayerId.
                hide();
                SurvBridge.spectate(1);
            }
        });
        card.addView(mSpectateBtn);

        // Status text
        mStatusText = new TextView(activity);
        mStatusText.setTextColor(0xFF808080);
        mStatusText.setTextSize(TypedValue.COMPLEX_UNIT_SP, 11);
        mStatusText.setGravity(Gravity.CENTER);
        card.addView(mStatusText);

        addGap(card, dp(8));

        // === Action Buttons ===
        mReturnBtn = new Button(activity);
        mReturnBtn.setText("\u25B6 " + L10n.t("game-return-to-game", "Return to Game"));
        mReturnBtn.setBackgroundColor(GREEN);
        mReturnBtn.setShadowLayer(dp(2), 0, dp(2), GREEN_SHADOW);
        mReturnBtn.setLayoutParams(new ViewGroup.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT, dp(44)));
        mReturnBtn.setGravity(Gravity.CENTER);
        mReturnBtn.setTextColor(Color.WHITE);
        mReturnBtn.setTextSize(TypedValue.COMPLEX_UNIT_SP, 16);
        mReturnBtn.setOnClickListener(new View.OnClickListener() {
            @Override public void onClick(View v) { hide(); }
        });
        card.addView(mReturnBtn);

        addGap(card, dp(6));

        mQuitBtn = new Button(activity);
        mQuitBtn.setText("\u2716 " + L10n.t("game-quit-game", "Quit Game"));
        mQuitBtn.setBackgroundColor(RED);
        mQuitBtn.setShadowLayer(dp(2), 0, dp(2), RED_SHADOW);
        mQuitBtn.setLayoutParams(new ViewGroup.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT, dp(44)));
        mQuitBtn.setGravity(Gravity.CENTER);
        mQuitBtn.setTextColor(Color.WHITE);
        mQuitBtn.setTextSize(TypedValue.COMPLEX_UNIT_SP, 16);
        mQuitBtn.setOnClickListener(new View.OnClickListener() {
            @Override public void onClick(View v) { confirmQuit(); }
        });
        card.addView(mQuitBtn);

        outerScroll.addView(card);

        mRoot.addView(outerScroll);

        // A newly constructed dialog is hidden until show(); ensurePause() adds
        // it to the overlay container before the user opens it.
        mRoot.setVisibility(View.GONE);

        seedFromConfig();
    }

    // ===== Helpers =====

    private void addGap(LinearLayout parent, int height) {
        View gap = new View(mActivity);
        gap.setLayoutParams(new ViewGroup.LayoutParams(1, height));
        parent.addView(gap);
    }

    private LinearLayout makeSliderRow(String label, int defaultValue) {
        LinearLayout row = new LinearLayout(mActivity);
        row.setOrientation(LinearLayout.HORIZONTAL);
        row.setGravity(Gravity.CENTER_VERTICAL);
        row.setPadding(0, dp(3), 0, dp(3));

        TextView lbl = new TextView(mActivity);
        lbl.setText(label);
        lbl.setTextColor(Color.WHITE);
        lbl.setTextSize(TypedValue.COMPLEX_UNIT_SP, 12);
        lbl.setLayoutParams(new LinearLayout.LayoutParams(dp(60), ViewGroup.LayoutParams.WRAP_CONTENT));
        row.addView(lbl);

        TextView val = new TextView(mActivity);
        val.setText(defaultValue + "%");
        val.setTextColor(GREEN);
        val.setTextSize(TypedValue.COMPLEX_UNIT_SP, 11);
        val.setLayoutParams(new LinearLayout.LayoutParams(dp(45), ViewGroup.LayoutParams.WRAP_CONTENT));
        row.addView(val);

        SeekBar slider = new SeekBar(mActivity);
        slider.setMax(100);
        slider.setProgress(defaultValue);
        slider.setLayoutParams(new LinearLayout.LayoutParams(
                0, ViewGroup.LayoutParams.WRAP_CONTENT, 1f));
        row.addView(slider);

        return row;
    }

    private LinearLayout.LayoutParams halfParams() {
        LinearLayout.LayoutParams lp = new LinearLayout.LayoutParams(
                0, dp(36), 1f);
        lp.rightMargin = dp(4);
        return lp;
    }

    private Button makeToggleButton(String label) {
        Button btn = new Button(mActivity);
        btn.setText(label);
        btn.setBackgroundColor(GREY);
        btn.setLayoutParams(new ViewGroup.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT, dp(34)));
        btn.setGravity(Gravity.CENTER);
        btn.setTextColor(Color.WHITE);
        btn.setTextSize(TypedValue.COMPLEX_UNIT_SP, 12);
        return btn;
    }

    /** "Show FPS": drives the engine stats overlay (persisted as `showFps`). */
    private void toggleShowFps() {
        mShowFps = !mShowFps;
        applyShowFps();
        SurvBridge.setShowFps(mShowFps);
    }

    private void applyShowFps() {
        mShowFpsBtn.setText(mShowFps ? "Show FPS \u2713" : "Show FPS");
        mShowFpsBtn.setBackgroundColor(mShowFps ? GREEN : GREY);
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
        final String tier = RenderScale.tierName(mRenderScale);
        mRenderTierBtn.setText("GFX: " + tier);
        mRenderTierBtn.setBackgroundColor(mRenderScale < 0.999f ? GREEN : GREY);
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
            mMuteBtn.setText("\uD83D\uDD07 Muted");
            mMuteBtn.setBackgroundColor(0xFFCC3333);
        } else {
            mMuteBtn.setText("\uD83D\uDD07 Mute");
            mMuteBtn.setBackgroundColor(GREY);
        }
    }

    private void confirmQuit() {
        new AlertDialog.Builder(mActivity)
                .setTitle("Quit Game?")
                .setMessage("Are you sure you want to quit? Any unsaved progress will be lost.")
                .setPositiveButton("Quit", (dialog, which) -> {
                    if (mHost != null) mHost.quitToMenu();
                })
                .setNegativeButton("Cancel", null)
                .show();
    }

    private void seedFromConfig() {
        String config = SurvBridge.getConfigJson();
        if (config == null || config.isEmpty()) return;

        mMuted = Json.peekBool(config, "muteAudio", false);
        updateMuteButton();

        mShowFps = Json.peekBool(config, "showFps", false);
        applyShowFps();

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
        seedFromConfig();
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
        mReturnBtn.setText("\u25B6 " + L10n.t("game-return-to-game", "Return to Game"));
    }
    
    public void setStatus(String text) {
        mStatusText.setText(text != null ? text : "");
    }

    private int dp(int value) {
        return (int) (value * mActivity.getResources().getDisplayMetrics().density + 0.5f);
    }
}
