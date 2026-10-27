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

        // Starts the M8 device login on first use (AccountManager.ensureLoaded)
        // and keeps the menu's account chrome in sync.
        AccountManager.get(mActivity).addListener(mAccountListener);

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
        hideProfile();
        hideBackBtn();
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
        if (mPause != null) mPause.setPlayerDead(false);
        hideSettings();
        hideHelp();
        hideLeaderboard();
        hidePass();
        hideProfile();
        hideBackBtn();
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
            updateBackBtn();
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
        if (mProfile != null && mProfile.isOpen()) {
            hideProfile();
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
                       || (mProfile != null && mProfile.isOpen())
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
        showLeaderboard(null);
    }

    /** Live snapshot from Game::leaderboardJson ({"activePlayerId","players":[...]}). */
    public void showLeaderboard(String json) {
        ensureLeaderboard();
        if (json != null && !json.isEmpty()) {
            mLeaderboard.updateFromJson(json);
        }
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

    // --- Account / Profile (M8) ---

    public void showProfile() {
        ensureProfile();
        mProfile.show();
        updateBackBtn();
    }

    public void hideProfile() {
        if (mProfile != null) mProfile.hide();
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
        mTeam.connect(create, roomCode != null ? roomCode : "");
        updateBackBtn();
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
            mSettings.setOnHidden(new Runnable() {
                @Override public void run() {
                    if (mMenu != null) mMenu.refresh();
                }
            });
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