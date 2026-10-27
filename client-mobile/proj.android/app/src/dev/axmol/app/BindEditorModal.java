/****************************************************************************
 * BindEditorModal.java - the Java "Key Binds" screen (client/src/inputBinds.ts
 * InputBindUi).
 *
 * The bind table itself lives in C++ (`input::InputBinds`, persisted in the
 * `binds` config key exactly like the web's toArray()/fromBase64() code). This
 * screen only renders `SurvBridge.getBindsJson()` and edits it:
 *
 *   - tap a row, press a key -> SurvBridge.setBind(action, Key, KEYCODE_*)
 *   - Backspace/Delete clears the bind, Back cancels the capture
 *   - a mouse wheel/button can be captured too (mouse sources only, so a touch
 *     does not bind "Left Mouse" by accident)
 *   - Restore Defaults -> SurvBridge.resetBinds()
 *   - Share Code -> SurvBridge.getBindsCode() (copy to the clipboard)
 *   - Load Code  -> SurvBridge.setBindsCode(code) (CRC-validated by C++)
 *
 * C++ maps Android key codes to input.ts Key codes, so the shared code is
 * web-compatible and a bad paste is rejected without touching the live table.
 ****************************************************************************/
package dev.axmol.app;

import android.app.Activity;
import android.content.ClipData;
import android.content.ClipboardManager;
import android.content.Context;
import android.graphics.Color;
import android.text.InputType;
import android.util.TypedValue;
import android.view.Gravity;
import android.view.InputDevice;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.EditText;
import android.widget.FrameLayout;
import android.widget.LinearLayout;
import android.widget.ScrollView;
import android.widget.TextView;

import java.util.List;

public class BindEditorModal {
    private static final String TAG = "BindEditorModal";

    private static final int DARK = 0xFF7A7A7A;
    private static final int GREEN = 0xFF83AF50;
    private static final int PANEL_BG = 0xF0000000;

    private final Activity mActivity;
    private final FrameLayout mRoot;
    private boolean mOpen = false;

    private LinearLayout mList;
    private TextView mStatus;
    private TextView mCodeView;
    private EditText mCodeInput;
    private View mCaptureView;
    private int mCaptureAction = -1;

    public BindEditorModal(Activity activity) {
        mActivity = activity;
        mRoot = new FrameLayout(activity);
        mRoot.setBackgroundColor(0xCC000000); // dim overlay
        buildUi();
    }

    private void buildUi() {
        ScrollView outerScroll = new ScrollView(mActivity);
        outerScroll.setLayoutParams(new FrameLayout.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.MATCH_PARENT));

        LinearLayout panel = new LinearLayout(mActivity);
        panel.setOrientation(LinearLayout.VERTICAL);
        panel.setGravity(Gravity.CENTER);
        panel.setBackgroundColor(PANEL_BG);
        panel.setPadding(dp(14), dp(8), dp(14), dp(8));

        TextView title = new TextView(mActivity);
        title.setText(L10n.t("index-keybinds", "Key Binds"));
        title.setTextColor(Color.WHITE);
        title.setTextSize(TypedValue.COMPLEX_UNIT_SP, 18);
        title.setGravity(Gravity.CENTER);
        panel.addView(title);

        TextView hint = new TextView(mActivity);
        hint.setText(L10n.t("index-keybinds-hint",
                "Tap an action, then press a key. Backspace clears it, Back cancels."));
        hint.setTextColor(0xFFB0B0B0);
        hint.setTextSize(TypedValue.COMPLEX_UNIT_SP, 10);
        hint.setGravity(Gravity.CENTER);
        hint.setPadding(0, 0, 0, dp(4));
        panel.addView(hint);

        // Scrollable bind rows (the panel must fit a landscape phone screen).
        ScrollView scroll = new ScrollView(mActivity);
        scroll.setLayoutParams(new LinearLayout.LayoutParams(dp(360), dp(168)));
        mList = new LinearLayout(mActivity);
        mList.setOrientation(LinearLayout.VERTICAL);
        scroll.addView(mList);
        panel.addView(scroll);

        mStatus = new TextView(mActivity);
        mStatus.setTextColor(0xFFFFD700);
        mStatus.setTextSize(TypedValue.COMPLEX_UNIT_SP, 11);
        mStatus.setGravity(Gravity.CENTER);
        mStatus.setPadding(0, dp(2), 0, 0);
        panel.addView(mStatus);

        // Share/import row (web .js-btn-keybind-share + #keybind-code-input).
        mCodeView = new TextView(mActivity);
        mCodeView.setTextColor(0xFF80C0FF);
        mCodeView.setTextSize(TypedValue.COMPLEX_UNIT_SP, 10);
        mCodeView.setTextIsSelectable(true);
        mCodeView.setSingleLine(true);
        panel.addView(mCodeView);

        LinearLayout shareRow = new LinearLayout(mActivity);
        shareRow.setOrientation(LinearLayout.HORIZONTAL);
        shareRow.setGravity(Gravity.CENTER_VERTICAL);

        Button copyBtn = makeSmallButton(L10n.t("index-copy", "Copy"));
        copyBtn.setOnClickListener(new View.OnClickListener() {
            @Override public void onClick(View v) { copyCode(); }
        });
        shareRow.addView(copyBtn);

        mCodeInput = new EditText(mActivity);
        mCodeInput.setHint(L10n.t("index-keybind-code", "Paste a bind code"));
        mCodeInput.setInputType(InputType.TYPE_CLASS_TEXT);
        mCodeInput.setTextColor(Color.WHITE);
        mCodeInput.setHintTextColor(0xFF808080);
        mCodeInput.setTextSize(TypedValue.COMPLEX_UNIT_SP, 11);
        mCodeInput.setSingleLine(true);
        LinearLayout.LayoutParams inputParams = new LinearLayout.LayoutParams(
                0, ViewGroup.LayoutParams.WRAP_CONTENT, 1f);
        inputParams.setMargins(dp(6), 0, dp(6), 0);
        shareRow.addView(mCodeInput, inputParams);

        Button loadBtn = makeSmallButton(L10n.t("index-load", "Load"));
        loadBtn.setOnClickListener(new View.OnClickListener() {
            @Override public void onClick(View v) { loadCode(); }
        });
        shareRow.addView(loadBtn);
        panel.addView(shareRow);

        // Footer: restore + close.
        LinearLayout footer = new LinearLayout(mActivity);
        footer.setOrientation(LinearLayout.HORIZONTAL);
        footer.setPadding(0, dp(4), 0, 0);

        Button restoreBtn = makeButton(L10n.t("index-keybinds-restore", "Restore Defaults"), DARK);
        restoreBtn.setOnClickListener(new View.OnClickListener() {
            @Override public void onClick(View v) {
                SurvBridge.resetBinds();
                mStatus.setText("");
                refreshList();
            }
        });
        LinearLayout.LayoutParams restoreParams = new LinearLayout.LayoutParams(
                0, dp(34), 1f);
        restoreParams.setMargins(0, 0, dp(6), 0);
        footer.addView(restoreBtn, restoreParams);

        Button closeBtn = makeButton(L10n.t("index-close", "Close"), GREEN);
        closeBtn.setOnClickListener(new View.OnClickListener() {
            @Override public void onClick(View v) { hide(); }
        });
        footer.addView(closeBtn, new LinearLayout.LayoutParams(0, dp(34), 1f));
        panel.addView(footer);

        outerScroll.addView(panel);
        mRoot.addView(outerScroll);
    }

    // ===== Rows =====

    private void refreshList() {
        if (mList == null) return;
        mList.removeAllViews();
        List<Json.BindEntry> binds = Json.parseBinds(SurvBridge.getBindsJson());
        for (final Json.BindEntry entry : binds) {
            LinearLayout row = new LinearLayout(mActivity);
            row.setOrientation(LinearLayout.HORIZONTAL);
            row.setGravity(Gravity.CENTER_VERTICAL);
            row.setPadding(0, dp(1), 0, dp(1));

            Button name = makeButton(entry.name, DARK);
            name.setGravity(Gravity.LEFT | Gravity.CENTER_VERTICAL);
            name.setOnClickListener(new View.OnClickListener() {
                @Override public void onClick(View v) { beginCapture(entry); }
            });
            row.addView(name, new LinearLayout.LayoutParams(0, dp(34), 1f));

            TextView value = new TextView(mActivity);
            value.setText(entry.label.isEmpty() ? "-" : entry.label);
            value.setTextColor(entry.label.isEmpty() ? 0xFF808080 : 0xFFFFD700);
            value.setTextSize(TypedValue.COMPLEX_UNIT_SP, 12);
            value.setGravity(Gravity.CENTER_VERTICAL | Gravity.RIGHT);
            value.setPadding(dp(8), 0, dp(4), 0);
            row.addView(value, new LinearLayout.LayoutParams(dp(150), dp(34)));

            mList.addView(row);
        }
        mCodeView.setText(SurvBridge.getBindsCode());
    }

    // ===== Capture =====

    private void beginCapture(final Json.BindEntry entry) {
        if (mCaptureView != null) return;
        mCaptureAction = entry.action;
        mStatus.setText(L10n.t("index-keybind-press", "Press a key...")
                + " (" + entry.name + ")");

        mCaptureView = new View(mActivity) {
            @Override public boolean dispatchKeyEvent(KeyEvent event) {
                if (event.getAction() == KeyEvent.ACTION_DOWN) {
                    if (event.getRepeatCount() == 0) {
                        onCapturedKey(event.getKeyCode());
                    }
                    return true;
                }
                return true;
            }

            @Override public boolean onGenericMotionEvent(MotionEvent event) {
                if (event.getAction() == MotionEvent.ACTION_SCROLL) {
                    float value = event.getAxisValue(MotionEvent.AXIS_VSCROLL);
                    if (value != 0.0f) {
                        // input.ts MouseWheel: 1 = up, 2 = down.
                        applyBind(mCaptureAction, 3, value > 0.0f ? 1 : 2);
                    }
                    return true;
                }
                return super.onGenericMotionEvent(event);
            }

            @Override public boolean onTouchEvent(MotionEvent event) {
                // Only real mouse sources: a touch pad must not bind Left Mouse.
                final boolean mouse = (event.getSource() & InputDevice.SOURCE_MOUSE)
                        == InputDevice.SOURCE_MOUSE;
                if (mouse && event.getAction() == MotionEvent.ACTION_DOWN) {
                    final int buttons = event.getButtonState();
                    final int code;
                    if ((buttons & MotionEvent.BUTTON_SECONDARY) != 0) {
                        code = 2; // input.ts MouseButton.Right
                    } else if ((buttons & MotionEvent.BUTTON_TERTIARY) != 0) {
                        code = 1; // Middle
                    } else {
                        code = 0; // Left
                    }
                    applyBind(mCaptureAction, 2, code);
                }
                return true;
            }
        };
        mCaptureView.setFocusable(true);
        mCaptureView.setFocusableInTouchMode(true);
        mCaptureView.setClickable(true);
        mCaptureView.setBackgroundColor(0x66000000);
        mRoot.addView(mCaptureView, new FrameLayout.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.MATCH_PARENT));
        mCaptureView.requestFocus();
    }

    private void onCapturedKey(int keyCode) {
        if (keyCode == KeyEvent.KEYCODE_BACK) {
            finishCapture("");
            return;
        }
        if (keyCode == KeyEvent.KEYCODE_DEL || keyCode == KeyEvent.KEYCODE_FORWARD_DEL) {
            applyBind(mCaptureAction, 0, 0); // clear (web Key.Backspace)
            return;
        }
        // Modifier/OS keys cannot be bound (inputBinds.ts disallowKeys).
        if (keyCode == KeyEvent.KEYCODE_SHIFT_LEFT || keyCode == KeyEvent.KEYCODE_SHIFT_RIGHT
                || keyCode == KeyEvent.KEYCODE_ALT_LEFT || keyCode == KeyEvent.KEYCODE_ALT_RIGHT
                || keyCode == KeyEvent.KEYCODE_CTRL_LEFT || keyCode == KeyEvent.KEYCODE_CTRL_RIGHT
                || keyCode == KeyEvent.KEYCODE_META_LEFT || keyCode == KeyEvent.KEYCODE_META_RIGHT
                || keyCode == KeyEvent.KEYCODE_CAPS_LOCK
                || keyCode == KeyEvent.KEYCODE_VOLUME_UP
                || keyCode == KeyEvent.KEYCODE_VOLUME_DOWN) {
            finishCapture(L10n.t("index-keybind-invalid", "That key cannot be bound"));
            return;
        }
        applyBind(mCaptureAction, 1, keyCode);
    }

    /** type: 0 none, 1 key (Android code), 2 mouse button, 3 wheel. */
    private void applyBind(int action, int type, int code) {
        final boolean ok = SurvBridge.setBind(action, type, code);
        finishCapture(ok ? "" : L10n.t("index-keybind-invalid", "That key cannot be bound"));
    }

    private void finishCapture(String status) {
        if (mCaptureView != null) {
            mRoot.removeView(mCaptureView);
            mCaptureView = null;
        }
        mCaptureAction = -1;
        mStatus.setText(status);
        refreshList();
    }

    // ===== Share code =====

    private void copyCode() {
        try {
            ClipboardManager clipboard =
                    (ClipboardManager) mActivity.getSystemService(Context.CLIPBOARD_SERVICE);
            clipboard.setPrimaryClip(ClipData.newPlainText("Survev key binds",
                    SurvBridge.getBindsCode()));
            mStatus.setText(L10n.t("index-copied", "Copied!"));
        } catch (Exception e) {
            mStatus.setText(L10n.t("index-error", "Error"));
        }
    }

    private void loadCode() {
        String code = mCodeInput.getText() != null ? mCodeInput.getText().toString().trim() : "";
        if (code.isEmpty()) {
            // Convenience: paste from the clipboard when the field is empty.
            try {
                ClipboardManager clipboard =
                        (ClipboardManager) mActivity.getSystemService(Context.CLIPBOARD_SERVICE);
                if (clipboard.hasPrimaryClip() && clipboard.getPrimaryClip().getItemCount() > 0) {
                    code = clipboard.getPrimaryClip().getItemAt(0).coerceToText(mActivity)
                            .toString().trim();
                }
            } catch (Exception ignored) {
            }
        }
        if (SurvBridge.setBindsCode(code)) {
            mCodeInput.setText("");
            mStatus.setText(L10n.t("index-keybind-loaded", "Loaded!"));
        } else {
            mStatus.setText(L10n.t("index-keybind-invalid-code", "Invalid bind code"));
        }
        refreshList();
    }

    // ===== Widgets =====

    private Button makeButton(String label, int color) {
        Button btn = new Button(mActivity);
        btn.setText(label);
        btn.setBackgroundColor(color);
        btn.setTextColor(Color.WHITE);
        btn.setTextSize(TypedValue.COMPLEX_UNIT_SP, 13);
        btn.setGravity(Gravity.CENTER);
        btn.setPadding(dp(10), dp(4), dp(10), dp(4));
        btn.setAllCaps(false);
        return btn;
    }

    private Button makeSmallButton(String label) {
        Button btn = makeButton(label, 0xFF666666);
        btn.setTextSize(TypedValue.COMPLEX_UNIT_SP, 11);
        btn.setLayoutParams(new LinearLayout.LayoutParams(dp(56), dp(30)));
        return btn;
    }

    private int dp(int value) {
        return (int) (value * mActivity.getResources().getDisplayMetrics().density + 0.5f);
    }

    // ===== Public API =====

    public View root() { return mRoot; }

    public boolean isOpen() { return mOpen; }

    public void show() {
        mOpen = true;
        mRoot.setVisibility(View.VISIBLE);
        refreshList();
        mStatus.setText("");
    }

    public void hide() {
        if (mCaptureView != null) {
            finishCapture("");
        }
        mOpen = false;
        mRoot.setVisibility(View.GONE);
    }
}
