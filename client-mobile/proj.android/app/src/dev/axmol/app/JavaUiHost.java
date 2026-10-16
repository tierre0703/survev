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
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.FrameLayout;

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
    private boolean mInGame = false;
    private boolean mInTeam = false;

    public JavaUiHost(Activity activity, FrameLayout container) {
        mActivity = activity;
        mContainer = container;
    }

    /** Adds the Java UI layer above the axmol surface and shows the menu. */
    public void install() {
        Log.i(TAG, "install() called");
        mContainer.setVisibility(View.VISIBLE);
        showMenu();
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

    /** A C++ side config edit; the Java screens re-read their values. */
    public void onConfigChanged(String key) {
        if (mMenu != null) mMenu.refresh();
        if (mLoadout != null && mLoadout.isOpen()) mLoadout.refresh();
        if (mSettings != null && mSettings.isOpen()) mSettings.syncFromConfig();
        Log.i(TAG, "onConfigChanged " + key);
    }

    public boolean isInGame() { return mInGame; }
    public boolean isPauseOpen() { return mPause != null && mPause.isOpen(); }

    // --- Settings ---

    public void showSettings() {
        ensureSettings();
        mSettings.show();
    }

    public void hideSettings() {
        if (mSettings != null) mSettings.hide();
    }

    // --- Help ---

    public void showHelp() {
        ensureHelp();
        mHelp.show();
    }

    public void hideHelp() {
        if (mHelp != null) mHelp.hide();
    }

    // --- Leaderboard ---

    public void showLeaderboard() {
        ensureLeaderboard();
        mLeaderboard.show();
    }

    public void hideLeaderboard() {
        if (mLeaderboard != null) mLeaderboard.hide();
    }

    // --- Loadout ---

    public void showLoadout() {
        ensureLoadout();
        mLoadout.show();
    }

    public void hideLoadout() {
        if (mLoadout != null) mLoadout.hide();
    }

    // --- Team ---

    public void showTeam(boolean create, String roomCode) {
        mInTeam = true;
        ensureTeam();
        mTeam.setPlayerName(lookupPlayerName());
        mTeam.setApiBaseUrl(lookupApiBaseUrl());
        mTeam.seedFromConfig();
        mTeam.show();
        if (create) {
            mTeam.createTeam();
        } else {
            mTeam.joinTeam(roomCode);
        }
    }

    public void hideTeam() {
        mInTeam = false;
        if (mTeam != null) {
            mTeam.leaveTeam();
            mTeam.hide();
        }
    }

    public void leaveTeam() {
        mInTeam = false;
        hideTeam();
        if (mMenu != null) mMenu.show();
    }

    public void updateTeam(float dt) {
        if (mTeam != null && mTeam.isActive()) {
            mTeam.update(dt);
        }
    }

    // --- Pause dialog ---

    public void showPause() {
        ensurePause();
        SurvBridge.onPauseGame();
        mPause.show();
    }

    public void hidePause() {
        if (mPause != null) {
            mPause.hide();
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
        mPause.setError(l10nKey, fallback);
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
            mTeam = new TeamFragment(mActivity);
            mTeam.setOnPlay(new TeamFragment.OnPlayListener() {
                @Override
                public void onPlay(String[] urls, String joinToken) {
                    mInGame = true;
                    hideTeam();
                }
            });
            FrameLayout.LayoutParams params = new FrameLayout.LayoutParams(
                    ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.MATCH_PARENT);
            mContainer.addView(mTeam.root(), params);
        }
    }

    private void ensurePause() {
        if (mPause == null) {
            mPause = new GamePauseDialog(mActivity);
            mPause.setOnQuit(new GamePauseDialog.OnQuit() {
                @Override
                public void onQuit() {
                    SurvBridge.onQuitGame();
                }
            });
            mPause.setOnResume(new GamePauseDialog.OnResume() {
                @Override
                public void onResume() {
                    SurvBridge.onResumeGame();
                }
            });
            FrameLayout.LayoutParams params = new FrameLayout.LayoutParams(
                    ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.MATCH_PARENT);
            mContainer.addView(mPause.root(), params);
        }
    }

    private String lookupPlayerName() {
        String config = SurvBridge.getConfigJson();
        String name = Json.peekString(config, "playerName");
        return name != null ? name : "Player";
    }

    private String lookupApiBaseUrl() {
        return "http://127.0.0.1:8000";
    }
}