/****************************************************************************
 * HelpModal.java — "How to Play" screen (ARCHITECTURE.md §7, step S5).
 *
 * Enhanced help screen with:
 *   - Control list with touch/glyph indicators
 *   - Movement controls section
 *   - Combat controls section  
 *   - Interaction controls section
 *   - Advanced tips (building, weapons, team, gas)
 *   - Video/tutorial link stub
 *   - Better formatting with icons per section
 *
 * Threading: all UI on the UI thread.
 ****************************************************************************/
package dev.axmol.app;

import android.app.Activity;
import android.graphics.Color;
import android.graphics.Typeface;
import android.util.TypedValue;
import android.view.Gravity;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.FrameLayout;
import android.widget.LinearLayout;
import android.widget.ScrollView;
import android.widget.TextView;

public class HelpModal {
    private static final String TAG = "HelpModal";

    private static final int PANEL_BG    = 0xF0000000;
    private static final int GREEN       = 0xFF83AF50;
    private static final int GREEN_SHADOW = 0xFF5B7A38;
    private static final int GREY        = 0xFF7A7A7A;
    private static final int GOLD        = 0xFFFFD700;
    private static final int BLUE        = 0xFF1E90FF;

    private final Activity mActivity;
    private final FrameLayout mRoot;
    private Button mCloseBtn;
    private boolean mOpen = false;
    private String mBuiltLocale = "";

    public HelpModal(Activity activity) {
        mActivity = activity;

        mRoot = new FrameLayout(activity);
        mRoot.setBackgroundColor(0xCC000000);
        buildUi();
    }

    /** Builds the modal views; re-run on show() after a language change. */
    private void buildUi() {
        mRoot.removeAllViews();
        mBuiltLocale = SurvBridge.getLanguage();
        final Activity activity = mActivity;

        // Full-height ScrollView for landscape responsiveness
        ScrollView outerScroll = new ScrollView(activity);
        outerScroll.setLayoutParams(new FrameLayout.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.MATCH_PARENT));

        // Main panel
        LinearLayout panel = new LinearLayout(activity);
        panel.setOrientation(LinearLayout.VERTICAL);
        panel.setGravity(Gravity.CENTER);
        panel.setBackgroundColor(PANEL_BG);
        panel.setPadding(dp(20), dp(14), dp(20), dp(14));

        // Title
        TextView title = new TextView(activity);
        title.setText("\uD83D\uDCD6 " + L10n.t("index-how-to-play", "How to Play"));
        title.setTextColor(Color.WHITE);
        title.setTextSize(TypedValue.COMPLEX_UNIT_SP, 20);
        title.setGravity(Gravity.CENTER);
        title.setTypeface(Typeface.DEFAULT_BOLD);
        panel.addView(title);

        // Help body (web help modal): the content goes straight into the
        // panel; the full-height outer ScrollView does the scrolling. (A nested
        // inner scroll never flings on device — see SettingsModal.)
        LinearLayout content = new LinearLayout(activity);
        content.setOrientation(LinearLayout.VERTICAL);
        content.setPadding(dp(10), dp(8), dp(10), dp(8));

        // === Touch controls (native HUD) ===
        addSectionHeader(content, "\uD83D\uDCF1 " + L10n.t("help-touch", "Touch"), 0xFF4CAF50);
        addControlRow(content, "\uD83D\uDD79",
                L10n.t("help-left-pad", "Left pad"),
                L10n.t("help-left-pad-desc", "Drag to move"));
        addControlRow(content, "\uD83C\uDFAF",
                L10n.t("help-right-pad", "Right pad"),
                L10n.t("help-right-pad-desc", "Drag to aim; release past the edge to fire"));
        addControlRow(content, "\u270B",
                L10n.t("help-pickup-prompt", "PICK UP prompt"),
                L10n.t("help-pickup-desc", "Tap the prompt (or USE) to loot"));
        addControlRow(content, "\uD83D\uDD27",
                L10n.t("help-use-reload", "USE / RELOAD"),
                L10n.t("help-use-reload-desc", "Action pills above the pads"));
        addControlRow(content, "\uD83D\uDC8A",
                L10n.t("help-heal-boost", "HEAL / BOOST"),
                L10n.t("help-heal-boost-desc", "Quick-use the best item in the bag"));
        addControlRow(content, "\uD83D\uDD0D",
                L10n.t("help-scope-swap", "SCOPE / SWAP"),
                L10n.t("help-scope-swap-desc", "Cycle scopes or switch gun slots"));
        addControlRow(content, "\uD83D\uDD2B",
                L10n.t("help-weapon-cards", "Weapon cards"),
                L10n.t("help-weapon-cards-desc", "Tap to equip; hold to drop"));
        addControlRow(content, "\uD83D\uDCAC",
                L10n.t("help-emote-ping", "Emote / ping"),
                L10n.t("help-emote-ping-desc", "Right-side wheel buttons"));
        addControlRow(content, "\uD83D\uDDFA",
                L10n.t("help-minimap", "Minimap"),
                L10n.t("help-minimap-desc", "Tap to open the big map"));

        addGap(content, dp(8));

        // === Keyboard / mouse (BT keyboard or desktop/emulator) ===
        addSectionHeader(content, "\u2328 " + L10n.t("index-controls", "Controls"), 0xFF2196F3);
        addControlRow(content, "\uD83D\uDEAA", L10n.t("index-movement", "Movement"),
                L10n.t("help-movement-keys", "W, A, S, D / arrows"));
        addControlRow(content, "\uD83C\uDFAF", L10n.t("index-aim", "Aim"),
                L10n.t("index-aim-ctrl", "Mouse"));
        addControlRow(content, "\uD83D\uDD1E", L10n.t("index-shoot", "Shoot"),
                L10n.t("index-shoot-ctrl", "Left-Click"));
        addControlRow(content, "\uD83D\uDD27", L10n.t("index-reload", "Reload"),
                L10n.t("help-reload-keys", "R / F Pickup"));
        addControlRow(content, "\uD83D\uDD00", L10n.t("index-change-weapons", "Change Weapons"),
                L10n.t("index-change-weapons-ctrl", "1 through 4 or Scroll Wheel"));
        addControlRow(content, "\uD83D\uDD04", L10n.t("index-swap-weapon-slots", "Switch Gun Slots"),
                L10n.t("index-swap-weapon-slots-ctrl", "T"));
        addControlRow(content, "\uD83D\uDEE1", L10n.t("index-stow-weapons", "Stow Weapons"),
                L10n.t("index-stow-weapons-ctrl", "E"));
        addControlRow(content, "\uD83D\uDDFA", L10n.t("index-toggle-minimap", "Toggle Minimap"),
                L10n.t("help-map-keys", "V / M or G = large map"));

        // Separator
        View sep = new View(activity);
        sep.setLayoutParams(new ViewGroup.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, dp(1)));
        sep.setBackgroundColor(0x33FFFFFF);
        content.addView(sep);

        // === Tips (actual game behaviour) ===
        addSectionHeader(content, "\uD83D\uDCA1 " + L10n.t("help-tips", "Tips"), GOLD);
        addTip(content, "\uD83C\uDFD7\uFE0F " + L10n.t("help-tip-cover", "Cover"),
                L10n.t("help-tip-cover-text",
                        "Trees, rocks, walls and buildings block bullets. Fight from behind cover and peek only to shoot."));
        addTip(content, "\uD83D\uDD2B " + L10n.t("help-tip-weapons", "Weapons"),
                L10n.t("help-tip-weapons-text",
                        "Shotguns dominate close range, snipers long range, SMGs up close. Reload behind cover."));
        addTip(content, "\uD83D\uDC65 " + L10n.t("help-tip-team", "Team"),
                L10n.t("help-tip-team-text",
                        "Stick together and revive downed teammates. Share ammo and healing items."));
        addTip(content, "\u2604\uFE0F " + L10n.t("help-tip-zone", "Red zone"),
                L10n.t("help-tip-zone-text",
                        "The gas shrinks over time. Stay inside the safe zone; move early instead of sprinting at the end."));
        addTip(content, "\uD83C\uDF92 " + L10n.t("help-tip-loot", "Loot"),
                L10n.t("help-tip-loot-text",
                        "Walk near an item and tap the PICK UP prompt. Better armour and a backpack win fights."));

        panel.addView(content);

        // Close button
        mCloseBtn = new Button(activity);
        mCloseBtn.setText(L10n.t("help-got-it", "Got it!") + " \u2713");
        mCloseBtn.setBackgroundColor(GREEN);
        mCloseBtn.setShadowLayer(dp(2), 0, dp(2), GREEN_SHADOW);
        mCloseBtn.setLayoutParams(new ViewGroup.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT, dp(44)));
        mCloseBtn.setGravity(Gravity.CENTER);
        mCloseBtn.setTextColor(Color.WHITE);
        mCloseBtn.setTextSize(TypedValue.COMPLEX_UNIT_SP, 16);
        mCloseBtn.setOnClickListener(new View.OnClickListener() {
            @Override public void onClick(View v) { hide(); }
        });
        panel.addView(mCloseBtn);

        outerScroll.addView(panel);

        mRoot.addView(outerScroll);
    }

    // ===== Layout Helpers =====

    private void addSectionHeader(LinearLayout parent, String text, int color) {
        TextView h = new TextView(mActivity);
        h.setText(text);
        h.setTextColor(color);
        h.setTextSize(TypedValue.COMPLEX_UNIT_SP, 15);
        h.setTypeface(Typeface.DEFAULT_BOLD);
        h.setPadding(0, dp(10), 0, dp(6));
        parent.addView(h);
    }

    private void addControlRow(LinearLayout parent, String glyph, String action, String desc) {
        LinearLayout row = new LinearLayout(mActivity);
        row.setOrientation(LinearLayout.HORIZONTAL);
        row.setGravity(Gravity.CENTER_VERTICAL);
        row.setPadding(0, dp(3), 0, dp(3));

        // Glyph/icon column
        TextView g = new TextView(mActivity);
        g.setText(glyph);
        g.setTextSize(TypedValue.COMPLEX_UNIT_SP, 16);
        g.setTextColor(GOLD);
        g.setGravity(Gravity.CENTER);
        g.setLayoutParams(new LinearLayout.LayoutParams(dp(32), ViewGroup.LayoutParams.WRAP_CONTENT));
        row.addView(g);

        // Action column
        TextView a = new TextView(mActivity);
        a.setText(action);
        a.setTextColor(Color.WHITE);
        a.setTextSize(TypedValue.COMPLEX_UNIT_SP, 13);
        a.setTypeface(Typeface.MONOSPACE);
        a.setLayoutParams(new LinearLayout.LayoutParams(dp(120), ViewGroup.LayoutParams.WRAP_CONTENT));
        row.addView(a);

        // Description column
        TextView d = new TextView(mActivity);
        d.setText(desc);
        d.setTextColor(0xFFAAAAAA);
        d.setTextSize(TypedValue.COMPLEX_UNIT_SP, 12);
        d.setLayoutParams(new LinearLayout.LayoutParams(0, ViewGroup.LayoutParams.WRAP_CONTENT, 1f));
        row.addView(d);

        parent.addView(row);
    }
    
    private void addTip(LinearLayout parent, String title, String text) {
        LinearLayout tip = new LinearLayout(mActivity);
        tip.setOrientation(LinearLayout.VERTICAL);
        tip.setPadding(dp(8), dp(6), dp(8), dp(6));
        tip.setBackgroundColor(0x22FFFFFF);
        
        TextView t = new TextView(mActivity);
        t.setText(title);
        t.setTextColor(GOLD);
        t.setTextSize(TypedValue.COMPLEX_UNIT_SP, 13);
        t.setTypeface(Typeface.DEFAULT_BOLD);
        tip.addView(t);
        
        TextView c = new TextView(mActivity);
        c.setText(text);
        c.setTextColor(0xFFCCCCCC);
        c.setTextSize(TypedValue.COMPLEX_UNIT_SP, 12);
        tip.addView(c);
        
        parent.addView(tip);
        addGap(parent, dp(4));
    }

    private void addGap(LinearLayout parent, int height) {
        View gap = new View(mActivity);
        gap.setLayoutParams(new ViewGroup.LayoutParams(1, height));
        parent.addView(gap);
    }

    private Button makeLinkButton(String label) {
        Button btn = new Button(mActivity);
        btn.setText(label);
        btn.setBackgroundColor(GREY);
        btn.setLayoutParams(new ViewGroup.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT, dp(38)));
        btn.setGravity(Gravity.CENTER);
        btn.setTextColor(BLUE);
        btn.setTextSize(TypedValue.COMPLEX_UNIT_SP, 13);
        return btn;
    }

    // ===== Public API =====

    public View root() { return mRoot; }

    public void show() {
        if (mOpen) return;
        mOpen = true;
        // The modal is built once at startup; rebuild when the language changed.
        String locale = SurvBridge.getLanguage();
        if (locale != null && !locale.equals(mBuiltLocale)) {
            buildUi();
        }
        mRoot.setVisibility(View.VISIBLE);
    }

    public void hide() {
        if (!mOpen) return;
        mOpen = false;
        mRoot.setVisibility(View.GONE);
    }

    public boolean isOpen() { return mOpen; }

    private int dp(int value) {
        return (int) (value * mActivity.getResources().getDisplayMetrics().density + 0.5f);
    }
}
