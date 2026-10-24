/****************************************************************************
 * The Java half of the UI seam (ARCHITECTURE.md section 3.2/4.3).
 *
 * Owns the `surv_java_ui` FrameLayout that sits above the axmol GLSurfaceView
 * and switches between the Java screens (menu/loadout/team/pause/settings/help)
 * and the live game. It is also the Java side of `SurvBridge`'s C++ -> Java
 * callbacks (`enterGame`/`returnToMenu`/`showResult`), which always arrive on
 * the UI thread.
 ****************************************************************************/
package dev.axmol.app;

import android.app.Activity;
import android.util.Log;
import android.view.Gravity;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.FrameLayout;
import android.graphics.Color;
import android.util.TypedValue;

public class JavaUiHost {
    private static final String TAG = "JavaUiHost";

    private final Activity mActivity;
    private final FrameLayout mContainer;
    private MenuFragment mMenu;
    private LoadoutFragment mLoadout;
    private TeamFragment mTeam;
    private GamePauseDialog mPause;
    private SettingsModal mSettings;
    private HelpModal mHelp;
    private LeaderboardModal mLeaderboard;
    private PassFragment mPass;
    private boolean mInGame = false;
    private boolean mInTeam = false;

    // Back arrow button: always-on-top, top-left corner. Visible when a sub-screen
    // (loadout, team, settings, help, leaderboard, pause) is open.
    private Button mBackBtn;

    public JavaUiHost(Activity activity, FrameLayout container) {
        mActivity = activity;
        mContainer = container;
    }

    /** Adds the Java UI layer above the axmol surface and shows the menu. */
    public void install() {
        Log.i(TAG, "install() called");
        mContainer.setVisibility(View.VISIBLE);

        // Back arrow button: added directly to the container (which is the Java
        // overlay layer). The container is setClickable(false) so touches pass
        // through to the game canvas, but by default Buttons are clickable
        // regardless of parent clickability.
        createBackBtn();

        showMenu();
    }

    private void createBackBtn() {
        mBackBtn = new Button(mActivity);
        mBackBtn.setText("<"); // simple less-than chevron
        mBackBtn.setTextSize(TypedValue.COMPLEX_UNIT_SP, 32);
        mBackBtn.setTextColor(Color.WHITE);
        mBackBtn.setBackgroundColor(0xBB000000); // more opaque, semi-transparent black
        mBackBtn.setVisibility(View.GONE);
        mBackBtn.setGravity(Gravity.CENTER);
        mBackBtn.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                handleBackPressed();
            }
        });
        int backSize = dp(56);
        FrameLayout.LayoutParams backParams = new FrameLayout.LayoutParams(backSize, backSize);
        backParams.gravity = Gravity.TOP | Gravity.LEFT;
        backParams.setMargins(dp(12), dp(12), 0, 0);
        mContainer.addView(mBackBtn, backParams);
    }

    /** Back to the menu (match ended, quit, or first launch). */
    public void showMenu() {
        Log.i(TAG, "showMenu() called");
        mInGame = false;
        mInTeam = false;
        hideLoadout();
        hideTeam();
        hidePause();
        hideSettings();
        hideHelp();
        hideLeaderboard();
        hidePass();
        hideBackBtn();
        ensureMenu();
        mMenu.show();
        SurvBridge.onMenuReady();
        Log.i(TAG, "showMenu() done, menu shown");
    }

    /** A match started: hide the Java screens so the game canvas is visible. */
    public void enterGame(int activePlayerId) {
        mInGame = true;
        hideLoadout();
        hideTeam();
        hidePause();
        hideSettings();
        hideHelp();
        hideLeaderboard();
        hidePass();
        hideBackBtn();
        if (mMenu != null) {
            mMenu.hide();
        }
        Log.i(TAG, "enterGame activePlayerId=" + activePlayerId);
    }

    /** The match ended or failed: back to the menu, with a localized error. */
    public void returnToMenu(String errorL10nKey) {
        showMenu();
        if (errorL10nKey != null && !errorL10nKey.isEmpty() && mMenu != null) {
            mMenu.showMessage(Json.peekString(SurvBridge.getL10nJson(), errorL10nKey));
        }
        Log.i(TAG, "returnToMenu error=" + errorL10nKey);
    }

    /** The active player died (show result screen / spectate prompt). */
    public void showResult() {
        Log.i(TAG, "showResult");
    }

    /** Every region probe finished (M8 pingTest): hand it to the menu. */
    public void pingResults(String json) {
        if (mMenu != null) {
            mMenu.onPingResults(json);
        } else {
            Log.w(TAG, "ping results before the menu existed");
        }
    }

    // Back arrow button helpers
    private void showBackBtn() {
        if (mBackBtn != null) mBackBtn.setVisibility(View.VISIBLE);
    }

    private void hideBackBtn() {
        if (mBackBtn != null) mBackBtn.setVisibility(View.GONE);
    }

    // Android hardware back button: close any open modal dialog.
    // Returns true if the back was handled (modal was open and was closed).
    public boolean handleBackPressed() {
        if (mPause != null && mPause.isOpen()) {
            hidePause();
            updateBackBtn();
            return true;
        }
        if (mSettings != null && mSettings.isOpen()) {
            hideSettings();
            updateBackBtn();
            return true;
        }
        if (mHelp != null && mHelp.isOpen()) {
            hideHelp();
            updateBackBtn();
            return true;
        }
        if (mLeaderboard != null && mLeaderboard.isOpen()) {
            hideLeaderboard();
            updateBackBtn();
            return true;
        }
        if (mPass != null && mPass.isOpen()) {
            hidePass();
            updateBackBtn();
            return true;
        }
        if (mLoadout != null && mLoadout.isOpen()) {
            hideLoadout();
            updateBackBtn();
            return true;
        }
        if (mTeam != null && mTeam.isOpen()) {
            hideTeam();
            updateBackBtn();
            if (mMenu != null) mMenu.show();
            return true;
        }
        return false;
    }

    // Show/hide the back arrow based on whether any sub-screen is open.
    private void updateBackBtn() {
        boolean anyOpen = (mPause != null && mPause.isOpen())
                       || (mSettings != null && mSettings.isOpen())
                       || (mHelp != null && mHelp.isOpen())
                       || (mLeaderboard != null && mLeaderboard.isOpen())
                       || (mPass != null && mPass.isOpen())
                       || (mLoadout != null && mLoadout.isOpen())
                       || (mTeam != null && mTeam.isOpen());
        if (anyOpen) {
            showBackBtn();
        } else {
            hideBackBtn();
        }
    }

    /** A C++ side config edit; the Java screens re-read their values. */
    public void onConfigChanged(String key) {
        if (mMenu != null) mMenu.refresh();
        if (mLoadout != null && mLoadout.isOpen()) mLoadout.refresh();
        if (mSettings != null && mSettings.isOpen()) mSettings.syncFromConfig();
        if (mPass != null && mPass.isOpen()) mPass.refresh();
        Log.i(TAG, "onConfigChanged " + key);
    }

    public boolean isInGame() { return mInGame; }
    public boolean isPauseOpen() { return mPause != null && mPause.isOpen(); }

    // --- Settings ---

    public void showSettings() {
        ensureSettings();
        mSettings.show();
        updateBackBtn();
    }

    public void hideSettings() {
        if (mSettings != null) mSettings.hide();
        updateBackBtn();
    }

    // --- Help ---

    public void showHelp() {
        ensureHelp();
        mHelp.show();
        updateBackBtn();
    }

    public void hideHelp() {
        if (mHelp != null) mHelp.hide();
        updateBackBtn();
    }

    // --- Leaderboard ---

    public void showLeaderboard() {
        ensureLeaderboard();
        mLeaderboard.show();
        updateBackBtn();
    }

    public void hideLeaderboard() {
        if (mLeaderboard != null) mLeaderboard.hide();
        updateBackBtn();
    }

    // --- Pass (Battle Pass) ---

    public void showPass() {
        ensurePass();
        mPass.show();
        updateBackBtn();
    }

    public void hidePass() {
        if (mPass != null) mPass.hide();
        updateBackBtn();
    }

    // --- Loadout ---

    public void showLoadout() {
        ensureLoadout();
        mLoadout.show();
        updateBackBtn();
    }

    public void hideLoadout() {
        if (mLoadout != null) mLoadout.hide();
        updateBackBtn();
    }

    // --- Team ---

    public void showTeam(boolean create, String roomCode) {
        mInTeam = true;
        ensureTeam();
        mTeam.show();
        updateBackBtn();
        if (create) {
            mTeam.createTeam();
        } else {
            mTeam.setInviteCode(roomCode);
        }
    }

    public void hideTeam() {
        mInTeam = false;
        if (mTeam != null) {
            mTeam.hide();
        }
        updateBackBtn();
    }

    public void leaveTeam() {
        mInTeam = false;
        hideTeam();
        if (mMenu != null) mMenu.show();
    }

    public void updateTeam(float dt) {
        // TeamFragment handles its own updates via WebSocket callbacks
    }

    public void updatePass(float dt) {
        if (mPass != null && mPass.isOpen()) {
            mPass.refresh();
        }
    }

    // --- Pause dialog ---

    public void showPause() {
        ensurePause();
        SurvBridge.onPauseGame();
        mPause.show();
        updateBackBtn();
    }

    public void hidePause() {
        if (mPause != null) {
            mPause.hide();
            updateBackBtn();
            SurvBridge.onResumeGame();
        }
    }

    public void togglePause() {
        ensurePause();
        if (mPause.isOpen()) {
            hidePause();
        } else {
            showPause();
        }
    }

    public void setPauseError(String l10nKey, String fallback) {
        ensurePause();
        mPause.setStatus(fallback);
    }

    // --- internals ---

    private void ensureMenu() {
        if (mMenu == null) {
            Log.i(TAG, "ensureMenu: creating new MenuFragment");
            mMenu = new MenuFragment(mActivity, this);
            FrameLayout.LayoutParams params = new FrameLayout.LayoutParams(
                    ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.MATCH_PARENT);
            mContainer.addView(mMenu.root(), params);
            Log.i(TAG, "ensureMenu: MenuFragment created and added");
        }
    }

    private void ensureSettings() {
        if (mSettings == null) {
            mSettings = new SettingsModal(mActivity);
            FrameLayout.LayoutParams params = new FrameLayout.LayoutParams(
                    ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.MATCH_PARENT);
            mContainer.addView(mSettings.root(), params);
        }
    }

    private void ensureHelp() {
        if (mHelp == null) {
            mHelp = new HelpModal(mActivity);
            FrameLayout.LayoutParams params = new FrameLayout.LayoutParams(
                    ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.MATCH_PARENT);
            mContainer.addView(mHelp.root(), params);
        }
    }

    private void ensureLeaderboard() {
        if (mLeaderboard == null) {
            mLeaderboard = new LeaderboardModal(mActivity);
            FrameLayout.LayoutParams params = new FrameLayout.LayoutParams(
                    ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.MATCH_PARENT);
            mContainer.addView(mLeaderboard.root(), params);
        }
    }

    private void ensurePass() {
        if (mPass == null) {
            mPass = new PassFragment(mActivity);
            mPass.setOnClose(new PassFragment.OnCloseListener() {
                @Override
                public void onClose() {
                    hidePass();
                    if (mMenu != null) mMenu.show();
                }
            });
            FrameLayout.LayoutParams params = new FrameLayout.LayoutParams(
                    ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.MATCH_PARENT);
            mContainer.addView(mPass.root(), params);
        }
    }

    private void ensureLoadout() {
        if (mLoadout == null) {
            mLoadout = new LoadoutFragment(mActivity);
            FrameLayout.LayoutParams params = new FrameLayout.LayoutParams(
                    ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.MATCH_PARENT);
            mContainer.addView(mLoadout.root(), params);
        }
    }

    private void ensureTeam() {
        if (mTeam == null) {
            mTeam = new TeamFragment(mActivity, this, false);
            FrameLayout.LayoutParams params = new FrameLayout.LayoutParams(
                    ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.MATCH_PARENT);
            mContainer.addView(mTeam.root(), params);
        }
    }

    private void ensurePause() {
        if (mPause == null) {
            mPause = new GamePauseDialog(mActivity, this);
            FrameLayout.LayoutParams params = new FrameLayout.LayoutParams(
                    ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.MATCH_PARENT);
            mContainer.addView(mPause.root(), params);
        }
    }

    private int dp(int value) {
        return (int) (value * mActivity.getResources().getDisplayMetrics().density + 0.5f);
    }

    private String lookupPlayerName() {
        String config = SurvBridge.getConfigJson();
        String name = Json.peekString(config, "playerName");
        return name != null ? name : "Player";
    }

    public void quitToMenu() {
        mInGame = false;
        hidePause();
        SurvBridge.onQuitGame();
        showMenu();
    }
}