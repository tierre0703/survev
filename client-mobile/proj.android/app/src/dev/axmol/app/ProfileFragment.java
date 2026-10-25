/****************************************************************************
 * ProfileFragment.java - M8 account screen (client/src/ui/profileUi.ts).
 *
 * The native client logs in by device id (AccountManager), so this screen has
 * no OAuth buttons. It mirrors the rest of the web profile management:
 *   - account name (set on first login, change with the 10 day cooldown)
 *   - device account info (slug / signed-in state)
 *   - Reset Stats (type "RESET STATS"), Delete Account (type "DELETE")
 *   - Log Out / Sign In
 *
 * Threading: AccountManager callbacks arrive on the UI thread; all SurvBridge
 * calls are made from it.
 ****************************************************************************/
package dev.axmol.app;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.DialogInterface;
import android.graphics.Color;
import android.graphics.drawable.GradientDrawable;
import android.text.InputType;
import android.util.Log;
import android.util.TypedValue;
import android.view.Gravity;
import android.view.View;
import android.view.ViewGroup;
import android.view.inputmethod.EditorInfo;
import android.widget.Button;
import android.widget.EditText;
import android.widget.FrameLayout;
import android.widget.LinearLayout;
import android.widget.ScrollView;
import android.widget.TextView;

import java.util.concurrent.TimeUnit;

public class ProfileFragment implements AccountManager.Listener {
    private static final String TAG = "ProfileFragment";

    private static final int PANEL_BG = 0xF01A1A2E;
    private static final int GREEN = 0xFF83AF50;
    private static final int GREY = 0xFF7A7A7A;
    private static final int BLUE = 0xFF3498DB;
    private static final int RED = 0xFFC0392B;
    private static final int GOLD = 0xFFFFD700;

    private final Activity mActivity;
    private final FrameLayout mRoot;
    private final AccountManager mAccount;

    private final TextView mNameLabel;
    private final TextView mSlugLabel;
    private final TextView mStatus;
    private final TextView mNameBtn;
    private final TextView mSignInBtn;
    private final TextView mResetBtn;
    private final TextView mDeleteBtn;
    private final TextView mLogoutBtn;

    private boolean mOpen = false;
    private boolean mNamePromptShown = false;

    public ProfileFragment(Activity activity) {
        mActivity = activity;
        mAccount = AccountManager.get(activity);

        mRoot = new FrameLayout(activity);
        mRoot.setBackgroundColor(0xCC000000);

        ScrollView outerScroll = new ScrollView(activity);
        outerScroll.setLayoutParams(new FrameLayout.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.MATCH_PARENT));

        LinearLayout panel = new LinearLayout(activity);
        panel.setOrientation(LinearLayout.VERTICAL);
        panel.setBackgroundColor(PANEL_BG);
        panel.setPadding(dp(18), dp(14), dp(18), dp(14));
        FrameLayout.LayoutParams panelParams = new FrameLayout.LayoutParams(
                dp(360), ViewGroup.LayoutParams.WRAP_CONTENT);
        panelParams.gravity = Gravity.CENTER;
        panel.setLayoutParams(panelParams);

        // ===== Header =====
        LinearLayout headerRow = new LinearLayout(activity);
        headerRow.setOrientation(LinearLayout.HORIZONTAL);
        headerRow.setGravity(Gravity.CENTER_VERTICAL);

        TextView title = new TextView(activity);
        title.setText(l10n("index-account", "Account"));
        title.setTextColor(GOLD);
        title.setTextSize(TypedValue.COMPLEX_UNIT_SP, 20);
        title.setTypeface(null, android.graphics.Typeface.BOLD);
        headerRow.addView(title, new LinearLayout.LayoutParams(
                0, ViewGroup.LayoutParams.WRAP_CONTENT, 1f));

        TextView close = new TextView(activity);
        close.setText("\u2715");
        close.setTextColor(Color.WHITE);
        close.setTextSize(TypedValue.COMPLEX_UNIT_SP, 20);
        close.setGravity(Gravity.CENTER);
        close.setClickable(true);
        close.setPadding(dp(8), 0, dp(8), 0);
        close.setOnClickListener(new View.OnClickListener() {
            @Override public void onClick(View v) {
                hide();
                if (mOnClose != null) mOnClose.onClose();
            }
        });
        headerRow.addView(close, new LinearLayout.LayoutParams(
                ViewGroup.LayoutParams.WRAP_CONTENT, dp(36)));
        panel.addView(headerRow);

        // ===== Account identity =====
        mNameLabel = new TextView(activity);
        mNameLabel.setTextColor(Color.WHITE);
        mNameLabel.setTextSize(TypedValue.COMPLEX_UNIT_SP, 22);
        mNameLabel.setTypeface(null, android.graphics.Typeface.BOLD);
        mNameLabel.setGravity(Gravity.CENTER);
        mNameLabel.setPadding(0, dp(10), 0, 0);
        panel.addView(mNameLabel);

        mSlugLabel = new TextView(activity);
        mSlugLabel.setTextColor(0xFFAAAAAA);
        mSlugLabel.setTextSize(TypedValue.COMPLEX_UNIT_SP, 12);
        mSlugLabel.setGravity(Gravity.CENTER);
        mSlugLabel.setPadding(0, dp(2), 0, dp(8));
        panel.addView(mSlugLabel);

        mStatus = new TextView(activity);
        mStatus.setTextColor(0xFFFF7878);
        mStatus.setTextSize(TypedValue.COMPLEX_UNIT_SP, 12);
        mStatus.setGravity(Gravity.CENTER);
        mStatus.setVisibility(View.GONE);
        panel.addView(mStatus);

        // ===== Buttons =====
        mSignInBtn = makeButton("Sign In", BLUE, 15);
        mSignInBtn.setOnClickListener(new View.OnClickListener() {
            @Override public void onClick(View v) {
                setStatus(null);
                if (mAccount.isSignedOut()) {
                    mAccount.signIn();
                } else {
                    mAccount.retry();
                }
                refresh();
            }
        });
        panel.addView(mSignInBtn, buttonParams());

        mNameBtn = makeButton(l10n("index-set-account-name", "Set your account name"), GREEN, 15);
        mNameBtn.setOnClickListener(new View.OnClickListener() {
            @Override public void onClick(View v) {
                showNameDialog();
            }
        });
        panel.addView(mNameBtn, buttonParams());

        mResetBtn = makeButton(l10n("index-reset-stats", "Reset Stats"), GREY, 14);
        mResetBtn.setOnClickListener(new View.OnClickListener() {
            @Override public void onClick(View v) {
                showConfirmDialog(
                        l10n("index-reset-stats", "Reset Stats"),
                        l10n("index-reset-stats-desc", "Enter \"RESET STATS\" to reset your stats:"),
                        "RESET STATS",
                        new Runnable() {
                            @Override public void run() {
                                mAccount.resetStats(new AccountManager.SimpleCallback() {
                                    @Override public void onResult(boolean success) {
                                        setStatus(success ? "Stats reset."
                                                : "Failed to reset stats.");
                                    }
                                });
                            }
                        });
            }
        });
        panel.addView(mResetBtn, buttonParams());

        mDeleteBtn = makeButton(l10n("index-delete-account", "Delete Account"), RED, 14);
        mDeleteBtn.setOnClickListener(new View.OnClickListener() {
            @Override public void onClick(View v) {
                showConfirmDialog(
                        l10n("index-delete-account", "Delete Account"),
                        l10n("index-delete-account-desc", "Enter \"DELETE\" to delete your account:"),
                        "DELETE",
                        new Runnable() {
                            @Override public void run() {
                                mAccount.deleteAccount(new AccountManager.SimpleCallback() {
                                    @Override public void onResult(boolean success) {
                                        setStatus(success ? "Account deleted."
                                                : "Failed to delete the account.");
                                    }
                                });
                            }
                        });
            }
        });
        panel.addView(mDeleteBtn, buttonParams());

        mLogoutBtn = makeButton(l10n("index-log-out", "Log Out"), GREY, 14);
        mLogoutBtn.setOnClickListener(new View.OnClickListener() {
            @Override public void onClick(View v) {
                mAccount.logout();
                setStatus("Logged out.");
                refresh();
            }
        });
        panel.addView(mLogoutBtn, buttonParams());

        outerScroll.addView(panel);
        mRoot.addView(outerScroll);
        mRoot.setVisibility(View.GONE);

        mAccount.addListener(this);
        refresh();
    }

    public interface OnCloseListener {
        void onClose();
    }

    private OnCloseListener mOnClose;

    public void setOnClose(OnCloseListener listener) {
        mOnClose = listener;
    }

    public View root() {
        return mRoot;
    }

    public void show() {
        mOpen = true;
        mRoot.setVisibility(View.VISIBLE);
        refresh();
        if (mAccount.isLoggedIn() && !mAccount.getProfile().usernameSet) {
            promptSetUsername();
        }
    }

    public void hide() {
        mOpen = false;
        mRoot.setVisibility(View.GONE);
    }

    public boolean isOpen() {
        return mOpen;
    }

    /** Web `profileUi.onLogin`: ask for a name until the account has one. */
    public void promptSetUsername() {
        if (!mAccount.isLoggedIn() || mAccount.getProfile().usernameSet || mNamePromptShown) {
            return;
        }
        mNamePromptShown = true;
        showNameDialog();
    }

    public void refresh() {
        AccountManager.Profile p = mAccount.getProfile();
        boolean loggedIn = p.loggedIn;

        mNameLabel.setText(loggedIn ? p.username : "Not signed in");
        if (loggedIn) {
            mSlugLabel.setText(p.slug.isEmpty() ? "" : "/stats?slug=" + p.slug);
        } else {
            mSlugLabel.setText("Sign in to sync stats, quests and your loadout.");
        }

        mSignInBtn.setVisibility(loggedIn ? View.GONE : View.VISIBLE);
        mSignInBtn.setText(mAccount.isSignedOut() ? "Sign In" : "Retry sign-in");
        mNameBtn.setVisibility(loggedIn ? View.VISIBLE : View.GONE);
        mResetBtn.setVisibility(loggedIn ? View.VISIBLE : View.GONE);
        mDeleteBtn.setVisibility(loggedIn ? View.VISIBLE : View.GONE);
        mLogoutBtn.setVisibility(loggedIn ? View.VISIBLE : View.GONE);

        if (loggedIn) {
            mNameBtn.setText(p.usernameSet
                    ? l10n("index-change-account-name", "Change Account Name")
                    : l10n("index-set-account-name", "Set your account name"));
            setEnabled(mNameBtn, p.usernameChangeTime <= 0);
            if (p.usernameChangeTime > 0) {
                mNameBtn.setText("Name change available in "
                        + formatRemaining(p.usernameChangeTime));
            }
        }
    }

    // --- AccountManager.Listener -------------------------------------------

    @Override
    public void onAccountChanged(AccountManager.Profile profile) {
        Log.i(TAG, "account changed: loggedIn=" + profile.loggedIn
                + " username=" + profile.username);
        if (mOpen) {
            refresh();
            if (profile.loggedIn && !profile.usernameSet) {
                promptSetUsername();
            }
        }
    }

    @Override
    public void onAccountError(String error) {
        Log.w(TAG, "account error: " + error);
        String message;
        if ("account_offline".equals(error)) {
            message = "Cannot reach the server.";
        } else if ("account_banned".equals(error)) {
            message = "This account is banned.";
        } else {
            message = "Account request failed.";
        }
        setStatus(message);
        if (mOpen) {
            refresh();
        }
    }

    // --- dialogs ------------------------------------------------------------

    private void showNameDialog() {
        final AccountManager.Profile p = mAccount.getProfile();
        if (!p.loggedIn) {
            return;
        }
        if (p.usernameChangeTime > 0 && p.usernameSet) {
            setStatus("Username has already been set recently.");
            return;
        }

        final EditText input = new EditText(mActivity);
        input.setSingleLine(true);
        input.setInputType(InputType.TYPE_CLASS_TEXT);
        input.setImeOptions(EditorInfo.IME_ACTION_DONE);
        input.setHint(l10n("index-enter-name", "Enter name"));
        if (p.usernameSet) {
            input.setText(p.username);
            input.setSelection(input.getText().length());
        }

        final AlertDialog dialog = new AlertDialog.Builder(mActivity)
                .setTitle(p.usernameSet
                        ? l10n("index-change-account-name", "Change Account Name")
                        : l10n("index-set-account-name", "Set your account name"))
                .setView(input)
                .setPositiveButton(l10n("index-finish", "Finish"), null)
                .setNegativeButton(l10n("index-back", "Back"), null)
                .create();
        dialog.show();
        dialog.getButton(DialogInterface.BUTTON_POSITIVE).setOnClickListener(
                new View.OnClickListener() {
                    @Override public void onClick(View v) {
                        String name = input.getText().toString().trim();
                        mAccount.setUsername(name, new AccountManager.UsernameCallback() {
                            @Override public void onResult(String errorCode) {
                                if (errorCode == null) {
                                    dialog.dismiss();
                                    setStatus(null);
                                    refresh();
                                } else {
                                    setStatus(usernameError(errorCode));
                                }
                            }
                        });
                    }
                });
    }

    private void showConfirmDialog(String title, String message, final String expected,
                                   final Runnable onConfirm) {
        final EditText input = new EditText(mActivity);
        input.setSingleLine(true);
        input.setInputType(InputType.TYPE_CLASS_TEXT);

        final AlertDialog dialog = new AlertDialog.Builder(mActivity)
                .setTitle(title)
                .setMessage(message)
                .setView(input)
                .setPositiveButton(l10n("index-confirm", "Confirm"), null)
                .setNegativeButton(l10n("index-back", "Back"), null)
                .create();
        dialog.show();
        dialog.getButton(DialogInterface.BUTTON_POSITIVE).setOnClickListener(
                new View.OnClickListener() {
                    @Override public void onClick(View v) {
                        if (expected.equals(input.getText().toString().trim())) {
                            dialog.dismiss();
                            setStatus(null);
                            onConfirm.run();
                        } else {
                            setStatus("Type \"" + expected + "\" to confirm.");
                        }
                    }
                });
    }

    // --- helpers ------------------------------------------------------------

    private static String usernameError(String code) {
        if ("invalid".equals(code)) return "Invalid username.";
        if ("taken".equals(code)) return "Name already taken!";
        if ("change_time_not_expired".equals(code)) {
            return "Username has already been set recently.";
        }
        return "Failed setting username.";
    }

    private static String formatRemaining(long ms) {
        long days = TimeUnit.MILLISECONDS.toDays(ms);
        long hours = TimeUnit.MILLISECONDS.toHours(ms) % 24;
        long minutes = TimeUnit.MILLISECONDS.toMinutes(ms) % 60;
        if (days > 0) return days + "d " + hours + "h";
        if (hours > 0) return hours + "h " + minutes + "m";
        return Math.max(1, minutes) + "m";
    }

    private void setStatus(String text) {
        mStatus.setText(text != null ? text : "");
        mStatus.setVisibility(text != null && !text.isEmpty() ? View.VISIBLE : View.GONE);
    }

    private void setEnabled(TextView view, boolean enabled) {
        view.setEnabled(enabled);
        view.setAlpha(enabled ? 1f : 0.5f);
        view.setClickable(enabled);
    }

    private String l10n(String key, String fallback) {
        String text = Json.peekString(SurvBridge.getL10nJson(), key);
        return text != null && !text.isEmpty() ? text : fallback;
    }

    private TextView makeButton(String label, final int body, int sp) {
        TextView btn = new TextView(mActivity);
        btn.setText(label);
        btn.setTextColor(Color.WHITE);
        btn.setTextSize(TypedValue.COMPLEX_UNIT_SP, sp);
        btn.setGravity(Gravity.CENTER);
        btn.setClickable(true);
        GradientDrawable bg = new GradientDrawable();
        bg.setColor(body);
        bg.setCornerRadius(dp(6));
        btn.setBackground(bg);
        return btn;
    }

    private LinearLayout.LayoutParams buttonParams() {
        LinearLayout.LayoutParams lp = new LinearLayout.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT, dp(38));
        lp.topMargin = dp(6);
        return lp;
    }

    private int dp(int value) {
        return (int) (value * mActivity.getResources().getDisplayMetrics().density + 0.5f);
    }
}
