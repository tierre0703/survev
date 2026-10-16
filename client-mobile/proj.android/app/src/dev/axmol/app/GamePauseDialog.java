/****************************************************************************
 * GamePauseDialog.java — S6 in-game pause dialog (ARCHITECTURE.md §7, step S6).
 *
 * The Java pause overlay that appears when the player taps the pause/menu
 * button during a match. Shows touch style toggles, aim line toggle, mute
 * toggle, volume labels, error/status line, and Return/Quit buttons.
 *
 * Uses only the axmol Android View SDK (android.widget, not android.slider).
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
import android.widget.FrameLayout;
import android.widget.LinearLayout;
import android.widget.TextView;

public class GamePauseDialog {
    private static final String TAG = "GamePauseDialog";

    private final Activity mActivity;
    private final FrameLayout mRoot;

    // --- widgets ---
    private final Button mMoveStyleBtn;
    private final Button mAimStyleBtn;
    private final Button mAimLineBtn;
    private final Button mMuteBtn;
    private final TextView mMasterLabel;
    private final TextView mSoundLabel;
    private final TextView mMusicLabel;
    private final TextView mErrorText;
    private final Button mResumeBtn;
    private final Button mQuitBtn;

    // --- callbacks ---
    private OnQuit mOnQuit;
    private OnResume mOnResume;

    public interface OnQuit {
        void onQuit();
    }

    public interface OnResume {
        void onResume();
    }

    // --- state ---
    private boolean mVisible = false;
    private boolean mMuted = false;
    private float mMasterVol = 1.0f;
    private float mSoundVol = 1.0f;
    private float mMusicVol = 1.0f;
    private int mMoveStyle = 1;
    private int mAimStyle = 1;
    private boolean mAimLineOn = true;

    public GamePauseDialog(Activity activity) {
        mActivity = activity;

        mRoot = new FrameLayout(activity);
        mRoot.setBackgroundColor(0xCC000000); // dark transparent overlay

        LinearLayout panel = new LinearLayout(activity);
        panel.setOrientation(LinearLayout.VERTICAL);
        panel.setGravity(Gravity.CENTER);
        panel.setBackgroundColor(0xFF343434);
        panel.setPadding(dp(16), dp(16), dp(16), dp(16));

        // --- Touch move style ---
        mMoveStyleBtn = new Button(activity);
        mMoveStyleBtn.setText("Move: Anywhere");
        mMoveStyleBtn.setBackgroundColor(0xFF5A5A5A);
        mMoveStyleBtn.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                mMoveStyle = mMoveStyle == 0 ? 1 : 0;
                mMoveStyleBtn.setText(mMoveStyle == 0 ? "Move: Locked" : "Move: Anywhere");
                saveConfig();
            }
        });
        panel.addView(mMoveStyleBtn);

        mAimStyleBtn = new Button(activity);
        mAimStyleBtn.setText("Aim: Anywhere");
        mAimStyleBtn.setBackgroundColor(0xFF5A5A5A);
        mAimStyleBtn.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                mAimStyle = mAimStyle == 0 ? 1 : 0;
                mAimStyleBtn.setText(mAimStyle == 0 ? "Aim: Locked" : "Aim: Anywhere");
                saveConfig();
            }
        });
        panel.addView(mAimStyleBtn);

        mAimLineBtn = new Button(activity);
        mAimLineBtn.setText("Aim Line: On");
        mAimLineBtn.setBackgroundColor(0xFF5A5A5A);
        mAimLineBtn.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                mAimLineOn = !mAimLineOn;
                mAimLineBtn.setText(mAimLineOn ? "Aim Line: On" : "Aim Line: Off");
                saveConfig();
            }
        });
        panel.addView(mAimLineBtn);

        mMuteBtn = new Button(activity);
        mMuteBtn.setText("Sound");
        mMuteBtn.setBackgroundColor(0xFF5A5A5A);
        mMuteBtn.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                mMuted = !mMuted;
                mMuteBtn.setText(mMuted ? "Sound: Off" : "Sound");
                SurvBridge.setAudio("mute", mMuted ? 1.0f : 0.0f);
            }
        });
        panel.addView(mMuteBtn);

        mMasterLabel = makeLabel("Master Volume: 1.0", 15);
        panel.addView(mMasterLabel);
        mSoundLabel = makeLabel("SFX Volume: 1.0", 15);
        panel.addView(mSoundLabel);
        mMusicLabel = makeLabel("Music Volume: 1.0", 15);
        panel.addView(mMusicLabel);

        mErrorText = new TextView(activity);
        mErrorText.setTextColor(0xFFFF7878);
        mErrorText.setTextSize(TypedValue.COMPLEX_UNIT_SP, 14);
        mErrorText.setGravity(Gravity.CENTER);
        panel.addView(mErrorText);

        // Buttons row
        LinearLayout buttonRow = new LinearLayout(activity);
        buttonRow.setOrientation(LinearLayout.HORIZONTAL);
        buttonRow.setGravity(Gravity.CENTER);
        buttonRow.setPadding(0, dp(16), 0, 0);

        mResumeBtn = new Button(activity);
        mResumeBtn.setText("Return to Game");
        mResumeBtn.setBackgroundColor(0xFF5A7A38);
        mResumeBtn.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                hide();
                if (mOnResume != null) mOnResume.onResume();
            }
        });
        buttonRow.addView(mResumeBtn);

        mQuitBtn = new Button(activity);
        mQuitBtn.setText("Quit Game");
        mQuitBtn.setBackgroundColor(0xFF7A3A3A);
        mQuitBtn.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                hide();
                if (mOnQuit != null) mOnQuit.onQuit();
            }
        });
        buttonRow.addView(mQuitBtn);

        panel.addView(buttonRow);
        mRoot.addView(panel);
    }

    // --- callbacks ---

    public void setOnQuit(OnQuit listener) { mOnQuit = listener; }
    public void setOnResume(OnResume listener) { mOnResume = listener; }

    // --- show/hide ---

    public void show() {
        if (mVisible) return;
        mVisible = true;
        syncFromConfig();
        mRoot.setVisibility(View.VISIBLE);
    }

    public void hide() {
        if (!mVisible) return;
        mVisible = false;
        mRoot.setVisibility(View.GONE);
    }

    public boolean isOpen() { return mVisible; }

    // --- Public: called from outside ---

    public void setError(String l10nKey, String fallback) {
        if (l10nKey == null || l10nKey.isEmpty()) {
            mErrorText.setText("");
            return;
        }
        String localized = Json.peekString(SurvBridge.getL10nJson(), l10nKey);
        mErrorText.setText(localized != null && !localized.isEmpty() ? localized : (fallback != null ? fallback : l10nKey));
    }

    public View root() { return mRoot; }

    // --- config sync ---

    private void syncFromConfig() {
        String config = SurvBridge.getConfigJson();
        if (config == null) return;

        String moveStyle = Json.peekString(config, "touchMoveStyle");
        mMoveStyle = "locked".equals(moveStyle) ? 0 : 1;
        mMoveStyleBtn.setText(mMoveStyle == 0 ? "Move: Locked" : "Move: Anywhere");

        String aimStyle = Json.peekString(config, "touchAimStyle");
        mAimStyle = "locked".equals(aimStyle) ? 0 : 1;
        mAimStyleBtn.setText(mAimStyle == 0 ? "Aim: Locked" : "Aim: Anywhere");

        String aimLine = Json.peekString(config, "touchAimLine");
        mAimLineOn = "true".equals(aimLine);
        mAimLineBtn.setText(mAimLineOn ? "Aim Line: On" : "Aim Line: Off");

        String muted = Json.peekString(config, "muteAudio");
        mMuted = "true".equals(muted);
        mMuteBtn.setText(mMuted ? "Sound: Off" : "Sound");

        String master = Json.peekString(config, "masterVolume");
        if (master != null) {
            try { mMasterVol = Float.parseFloat(master); } catch (Exception e) {}
        }
        mMasterLabel.setText("Master Volume: " + mMasterVol);

        String sound = Json.peekString(config, "soundVolume");
        if (sound != null) {
            try { mSoundVol = Float.parseFloat(sound); } catch (Exception e) {}
        }
        mSoundLabel.setText("SFX Volume: " + mSoundVol);

        String music = Json.peekString(config, "musicVolume");
        if (music != null) {
            try { mMusicVol = Float.parseFloat(music); } catch (Exception e) {}
        }
        mMusicLabel.setText("Music Volume: " + mMusicVol);
    }

    private void saveConfig() {
        SurvBridge.setAudio("mute", mMuted ? 1.0f : 0.0f);
        SurvBridge.setAudio("master", mMasterVol);
        SurvBridge.setAudio("sound", mSoundVol);
        SurvBridge.setAudio("music", mMusicVol);
        SurvBridge.saveConfig();
    }

    // --- helpers ---

    private TextView makeLabel(String text, float size) {
        TextView label = new TextView(mActivity);
        label.setText(text);
        label.setTextColor(Color.WHITE);
        label.setTextSize(TypedValue.COMPLEX_UNIT_SP, size);
        label.setPadding(0, dp(4), 0, dp(2));
        return label;
    }

    private int dp(int value) {
        return (int) (value * mActivity.getResources().getDisplayMetrics().density + 0.5f);
    }
}