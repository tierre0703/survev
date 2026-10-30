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
import android.widget.FrameLayout;

public class JavaUiHost {
    private static final String TAG = "JavaUiHost";

    private final Activity mActivity;
    private final FrameLayout mContainer;
    private MenuFragment mMenu;
    private LoadoutFragment mLoadout;
    private TeamFragment mTeam;
    private GamePauseDialog mPause;
    private GameOverDialog mGameOver;
    private SettingsModal mSettings;
    private BindEditorModal mBinds;
    private HelpModal mHelp;
    private LeaderboardModal mLeaderboard;
    private PassFragment mPass;
    private ProfileFragment mProfile;
    private boolean mInGame = false;
    private boolean mInTeam = false;
    // Web `profileUi.onLogin`: the first login opens the set-name prompt once.
    private boolean mAutoPromptedName = false;
    // The profile screen was opened only for that prompt: close it once set.
    private boolean mProfileOpenedForName = false;
    // Detect a different account (sign out/in, delete) so it can prompt again.
    private String mLastSlug = "";

    /** Device account state -> menu chrome + the one-time name prompt. */
    private final AccountManager.Listener mAccountListener = new AccountManager.Listener() {
        @Override
        public void onAccountChanged(AccountManager.Profile profile) {
            if (mMenu != null) {
                mMenu.onAccountChanged(profile);
            }
            if (mPass != null && mPass.isOpen()) {
                mPass.refresh();
            }
            if (!profile.slug.isEmpty() && !profile.slug.equals(mLastSlug)) {
                mLastSlug = profile.slug;
                mAutoPromptedName = false;
            }
            if (mProfileOpenedForName && profile.loggedIn && profile.usernameSet) {
                mProfileOpenedForName = false;
                hideProfile();
                if (mMenu != null) {
                    mMenu.show();
                }
            }
            if (!mAutoPromptedName && profile.loggedIn && !profile.usernameSet
                    && !mInGame && !mInTeam) {
                mAutoPromptedName = true;
                // Post so a showMenu() later in the same call stack (install)
                // cannot immediately hide the prompt again.
                mContainer.post(new Runnable() {
                    @Override public void run() {
                        if (!mInGame) {
                            mProfileOpenedForName = true;
                            showProfile();
                        }
                    }
                });
            }
        }

        @Override
        public void onAccountError(String error) {
            if (mMenu != null) {
                mMenu.onAccountError(error);
            }
        }
    };

    public JavaUiHost(Activity activity, FrameLayout container) {
        mActivity = activity;
        mContainer = container;
    }

    /** Adds the Java UI layer above the axmol surface and shows the menu. */
    public void install() {
        Log.i(TAG, "install() called");
        mContainer.setVisibility(View.VISIBLE);

        // Starts the M8 device login on first use (AccountManager.ensureLoaded)
        // and keeps the menu's account chrome in sync.
        AccountManager.get(mActivity).addListener(mAccountListener);

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
        hideGameOver();
        hideBinds();
        hideSettings();
        hideHelp();
        hideLeaderboard();
        hidePass();
        hideProfile();
        ensureMenu();
        mMenu.show();
        SurvBridge.onMenuReady();
        Log.i(TAG, "showMenu() done, menu shown");
    }

    /** Fastest-first zones of `region`, for the team playGame message. */
    public String zonesJson(String region) {
        return mMenu != null ? mMenu.zonesJson(region) : "[\"" + region + "\"]";
    }

    /** A match started: hide the Java screens so the game canvas is visible. */
    public void enterGame(int activePlayerId) {
        mInGame = true;
        hideLoadout();
        hidePause();
        hideGameOver();
        if (mPause != null) mPause.setPlayerDead(false);
        hideBinds();
        hideSettings();
        hideHelp();
        hideLeaderboard();
        hidePass();
        hideProfile();
        if (mInTeam && mTeam != null) {
            // Keep the team socket (and the room) alive for the post-match
            // return; `returnToMenu` re-shows the team screen.
            mTeam.hideForGame();
        } else {
            hideTeam();
        }
        if (mMenu != null) {
            mMenu.hide();
        }
        Log.i(TAG, "enterGame activePlayerId=" + activePlayerId);
    }

    /** The match ended or failed: back to the menu, with a localized error. */
    public void returnToMenu(String errorL10nKey) {
        // A team match returns to its room (web `teamMenu.onGameComplete`), not
        // to the main menu: the team socket stayed alive while in game.
        if (mInTeam && mTeam != null && mTeam.isInGame()) {
            mInGame = false;
            mTeam.onGameComplete();
            if (errorL10nKey != null && !errorL10nKey.isEmpty()) {
                mTeam.showMessage(Json.peekString(SurvBridge.getL10nJson(), errorL10nKey));
            }
            Log.i(TAG, "returnToMenu (team room) error=" + errorL10nKey);
            return;
        }
        showMenu();
        if (errorL10nKey != null && !errorL10nKey.isEmpty() && mMenu != null) {
            String text = Json.peekString(SurvBridge.getL10nJson(), errorL10nKey);
            if ("index-invalid-protocol".equals(errorL10nKey)
                    || "index-ip-banned".equals(errorL10nKey)
                    || "index-behind-proxy".equals(errorL10nKey)) {
                mMenu.showErrorKey(errorL10nKey);
            } else {
                mMenu.showMessage(text);
            }
        }
        Log.i(TAG, "returnToMenu error=" + errorL10nKey);
    }

    /** The active player died (show result screen / spectate prompt). */
    public void showResult() {
        ensurePause();
        if (mPause != null) {
            mPause.setPlayerDead(true);
        }
        Log.i(TAG, "showResult (pause spectate enabled)");
    }

    /**
     * Final result screen (ui.ts showStats via Game::gameOverJson). The Java
     * overlay sits above the GL surface, so the C++ HUD only keeps the modal
     * state; PLAY NEW GAME calls quitToMenu(), SPECTATE calls SurvBridge.
     */
    public void showGameOver(String json) {
        if (mPause != null && mPause.isOpen()) {
            hidePause();
        }
        ensureGameOver();
        mGameOver.show(json);
        Log.i(TAG, "showGameOver (Java result dialog)");
    }

    public void hideGameOver() {
        if (mGameOver != null) {
            mGameOver.hide();
        }
    }

    /** Every region probe finished (M8 pingTest): hand it to the menu. */
    public void pingResults(String json) {
        if (mMenu != null) {
            mMenu.onPingResults(json);
        } else {
            Log.w(TAG, "ping results before the menu existed");
        }
    }

    // Android hardware back button: close any open modal dialog.
    // Returns true if the back was handled (modal was open and was closed).
    public boolean handleBackPressed() {
        // The result screen is modal (web showStats): BACK must not quit the
        // match by accident; the dialog's own button does that.
        if (mGameOver != null && mGameOver.isOpen()) {
            return true;
        }
        if (mPause != null && mPause.isOpen()) {
            hidePause();
            return true;
        }
        if (mBinds != null && mBinds.isOpen()) {
            hideBinds();
            return true;
        }
        if (mSettings != null && mSettings.isOpen()) {
            hideSettings();
            return true;
        }
        if (mHelp != null && mHelp.isOpen()) {
            hideHelp();
            return true;
        }
        if (mLeaderboard != null && mLeaderboard.isOpen()) {
            hideLeaderboard();
            return true;
        }
        if (mPass != null && mPass.isOpen()) {
            hidePass();
            return true;
        }
        if (mProfile != null && mProfile.isOpen()) {
            hideProfile();
            return true;
        }
        if (mLoadout != null && mLoadout.isOpen()) {
            hideLoadout();
            return true;
        }
        if (mTeam != null && mTeam.isOpen()) {
            hideTeam();
            if (mMenu != null) mMenu.show();
            return true;
        }
        return false;
    }

    /** A C++ side config edit; the Java screens re-read their values. */
    public void onConfigChanged(String key) {
        if (mMenu != null) mMenu.refresh();
        if (mLoadout != null && mLoadout.isOpen()) mLoadout.refresh();
        if (mSettings != null && mSettings.isOpen()) mSettings.syncFromConfig();
        if (mPass != null && mPass.isOpen()) mPass.refresh();
        if (mProfile != null && mProfile.isOpen()) mProfile.refresh();
        if (mTeam != null && mTeam.isOpen()) mTeam.refresh();
        Log.i(TAG, "onConfigChanged " + key);
    }

    public boolean isInGame() { return mInGame; }
    public boolean isPauseOpen() { return mPause != null && mPause.isOpen(); }

    /** Raw site_info for the Java team screen (menu owns the HTTP fetch). */
    public String siteInfoJson() {
        return mMenu != null ? mMenu.siteInfoJson() : "";
    }

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
        showLeaderboard(null);
    }

    /** Live snapshot from Game::leaderboardJson ({"activePlayerId","players":[...]}). */
    public void showLeaderboard(String json) {
        ensureLeaderboard();
        if (json != null && !json.isEmpty()) {
            mLeaderboard.updateFromJson(json);
        }
        mLeaderboard.show();
    }

    public void hideLeaderboard() {
        if (mLeaderboard != null) mLeaderboard.hide();
    }

    // --- Pass (Battle Pass) ---

    public void showPass() {
        ensurePass();
        mPass.show();
    }

    public void hidePass() {
        if (mPass != null) mPass.hide();
    }

    // --- Account / Profile (M8) ---

    public void showProfile() {
        ensureProfile();
        mProfile.show();
    }

    public void hideProfile() {
        if (mProfile != null) mProfile.hide();
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
        mTeam.connect(create, roomCode != null ? roomCode : "");
    }

    /** `survev://team/<code>` (or a pasted link) from AppActivity. */
    public void openTeamInvite(String rawLink) {
        String code = TeamLinks.extractCode(rawLink);
        if (code.isEmpty()) {
            Log.w(TAG, "ignoring team invite without a code: " + rawLink);
            return;
        }
        if (mInGame) {
            Log.w(TAG, "ignoring team invite while in a match: " + code);
            return;
        }
        showMenu();
        showTeam(false, code);
        Log.i(TAG, "opening team invite " + code);
    }

    /**
     * Web `tryJoinGameFromParam`: `?u=<base64 comma-separated urls>&jt=<token>`
     * direct match link from AppActivity. Leaves any team room first.
     */
    public void openJoinLink(String[] urls, String joinToken) {
        if (urls == null || urls.length == 0 || joinToken == null || joinToken.isEmpty()) {
            return;
        }
        mInGame = false;
        showMenu();
        SurvBridge.onJoinUrls(urls, joinToken);
        Log.i(TAG, "opening join link urls=" + urls.length);
    }

    public void hideTeam() {
        mInTeam = false;
        if (mTeam != null) {
            mTeam.hide();
        }
    }

    public void leaveTeam() {
        mInTeam = false;
        hideTeam();
        if (mMenu != null) mMenu.show();
    }

    /**
     * The team room is gone (its socket closed) but the match it started is
     * still running: forget the team without changing the current screen, so
     * the match-end callback opens the main menu instead of the dead room.
     */
    public void dropTeam() {
        mInTeam = false;
        if (mTeam != null) mTeam.hide();
        Log.i(TAG, "dropTeam (room socket lost)");
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
            mSettings.setOnHidden(new Runnable() {
                @Override public void run() {
                    if (mMenu != null) mMenu.refresh();
                }
            });
            mSettings.setOnOpenBinds(new Runnable() {
                @Override public void run() { showBinds(); }
            });
            FrameLayout.LayoutParams params = new FrameLayout.LayoutParams(
                    ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.MATCH_PARENT);
            mContainer.addView(mSettings.root(), params);
        }
    }

    // --- Key Binds (client/src/inputBinds.ts) ---

    private void ensureBinds() {
        if (mBinds == null) {
            mBinds = new BindEditorModal(mActivity);
            FrameLayout.LayoutParams params = new FrameLayout.LayoutParams(
                    ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.MATCH_PARENT);
            mContainer.addView(mBinds.root(), params);
        }
    }

    public void showBinds() {
        ensureBinds();
        mBinds.show();
    }

    public void hideBinds() {
        if (mBinds != null) mBinds.hide();
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

    private void ensureProfile() {
        if (mProfile == null) {
            mProfile = new ProfileFragment(mActivity);
            mProfile.setOnClose(new ProfileFragment.OnCloseListener() {
                @Override
                public void onClose() {
                    hideProfile();
                    if (mMenu != null) mMenu.show();
                }
            });
            FrameLayout.LayoutParams params = new FrameLayout.LayoutParams(
                    ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.MATCH_PARENT);
            mContainer.addView(mProfile.root(), params);
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
            mTeam = new TeamFragment(mActivity, this);
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

    private void ensureGameOver() {
        if (mGameOver == null) {
            mGameOver = new GameOverDialog(mActivity, this);
            FrameLayout.LayoutParams params = new FrameLayout.LayoutParams(
                    ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.MATCH_PARENT);
            mContainer.addView(mGameOver.root(), params);
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
        hideGameOver();
        SurvBridge.onQuitGame();
        // A team match returns to its room through returnToMenu(), which sends
        // `gameComplete` and re-shows the team screen. Do not call showMenu()
        // here: it clears mInTeam before that callback arrives (web
        // main.ts:351-358 keeps the team socket/room alive).
        if (mInTeam && mTeam != null) {
            return;
        }
        showMenu();
    }
}