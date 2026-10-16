/****************************************************************************
 * HelpModal.java — "How to Play" screen matching web client #start-help.
 *
 * Lists all control mappings with touch variants (web: lines 1143-1249 of
 * index.html). Sections:
 *   - Controls: movement, aim, punch/shoot, change weapons, reload, scope zoom,
 *     pickup/loot/revive, use medical, drop item, cancel action, view map,
 *     toggle minimap, use ping, use emote
 *   - Tips: "How to Play" battle royale explanation, 2D PUBG comparison,
 *     loot/kill loop, red zone danger
 *
 * Uses the same gold headers, white text, and layout as the web client.
 ****************************************************************************/
package dev.axmol.app;

import android.app.Activity;
import android.graphics.Color;
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
    private static final int PANEL_BG = 0x90000000;
    private static final int DARK = 0xFF7A7A7A;
    private static final int GOLD = 0xFFFFD700;
    private static final int GREEN = 0xFF83AF50;

    private final Activity mActivity;
    private final FrameLayout mRoot;
    private boolean mOpen = false;

    public HelpModal(Activity activity) {
        mActivity = activity;
        mRoot = new FrameLayout(activity);
        mRoot.setBackgroundColor(0xCC000000); // dim overlay

        LinearLayout panel = new LinearLayout(activity);
        panel.setOrientation(LinearLayout.VERTICAL);
        panel.setGravity(Gravity.CENTER);
        panel.setBackgroundColor(PANEL_BG);
        panel.setPadding(dp(24), dp(16), dp(24), dp(16));

        // Title
        TextView title = new TextView(activity);
        title.setText("How to Play");
        title.setTextColor(Color.WHITE);
        title.setTextSize(TypedValue.COMPLEX_UNIT_SP, 24);
        title.setGravity(Gravity.CENTER);
        title.setPadding(0, 0, 0, dp(8));
        panel.addView(title);

        // Scrollable content
        ScrollView scroll = new ScrollView(activity);
        scroll.setLayoutParams(new FrameLayout.LayoutParams(dp(600), dp(400)));

        LinearLayout content = new LinearLayout(activity);
        content.setOrientation(LinearLayout.VERTICAL);
        content.setPadding(dp(16), 0, dp(16), 0);

        // === Controls section (web client #start-help) ===
        addSectionHeader(content, "Controls");
        addControlRow(content, "Movement", "Drag on left side");
        addControlRow(content, "Aim", "Drag on right side");
        addControlRow(content, "Punch / Shoot", "Tap right side");
        addControlRow(content, "Change Weapons", "Tap weapon icon");
        addControlRow(content, "Reload", "Tap ammo counter");
        addControlRow(content, "Scope Zoom", "Pinch / zoom buttons");
        addControlRow(content, "Pickup / Loot / Revive", "Tap nearby item / player");
        addControlRow(content, "Use Medical", "Tap heal icon");
        addControlRow(content, "Drop Item", "Drag from inventory");
        addControlRow(content, "Cancel Action", "Tap cancel button");
        addControlRow(content, "View Map", "Tap minimap");
        addControlRow(content, "Toggle Minimap", "Tap minimap toggle");
        addControlRow(content, "Use Ping", "Tap ping button");
        addControlRow(content, "Use Emote", "Tap emote button");

        // === Tips section (web client #start-help) ===
        addGap(content, dp(12));
        addSectionHeader(content, "How to Play");
        addTipParagraph(content,
                "The goal of survev.io is to be the last player standing. " +
                "You only live once per game - there is no respawn!");

        addSectionHeader2(content, "2D PUBG");
        addTipParagraph(content,
                "If you've played other battle royale games like PUBG, Fortnite or H1Z1, " +
                "then you're already halfway there! Think of survev.io as 2D PUBG " +
                "(with slightly less desync and more chicken).");

        addSectionHeader2(content, "Loot and Kill");
        addTipParagraph(content,
                "You'll begin the game with no items other than a simple backpack. " +
                "Move around the map to find loot: weapons, ammo, scopes, and medical items. " +
                "Eliminate other players and you can take their loot!");

        addSectionHeader2(content, "Red = Bad!");
        addTipParagraph(content,
                "Players aren't the only thing that can hurt you. The deadly red zone " +
                "will move in from the sides of the map and deal increasingly greater " +
                "damage if you stand in it. Keep an eye on the map and stay safe.");

        scroll.addView(content);
        panel.addView(scroll);

        // Close button (web: green, same as Play buttons)
        Button closeBtn = makeCloseButton("Back");
        closeBtn.setOnClickListener(new View.OnClickListener() {
            @Override public void onClick(View v) { hide(); }
        });
        panel.addView(closeBtn);

        mRoot.addView(panel);
    }

    // ===== Helpers =====

    private void addSectionHeader(LinearLayout parent, String text) {
        TextView h = new TextView(mActivity);
        h.setText(text);
        h.setTextColor(GOLD);
        h.setTextSize(TypedValue.COMPLEX_UNIT_SP, 20);
        h.setPadding(0, dp(8), 0, dp(4));
        parent.addView(h);
    }

    private void addSectionHeader2(LinearLayout parent, String text) {
        TextView h = new TextView(mActivity);
        h.setText(text);
        h.setTextColor(GOLD);
        h.setTextSize(TypedValue.COMPLEX_UNIT_SP, 18);
        h.setPadding(0, dp(6), 0, dp(4));
        parent.addView(h);
    }

    private void addControlRow(LinearLayout parent, String action, String control) {
        LinearLayout row = new LinearLayout(mActivity);
        row.setOrientation(LinearLayout.HORIZONTAL);
        row.setGravity(Gravity.CENTER);
        row.setPadding(0, dp(4), 0, dp(4));

        TextView actionLabel = new TextView(mActivity);
        actionLabel.setText(action);
        actionLabel.setTextColor(Color.WHITE);
        actionLabel.setTextSize(TypedValue.COMPLEX_UNIT_SP, 15);
        actionLabel.setLayoutParams(new FrameLayout.LayoutParams(dp(200), dp(28)));
        row.addView(actionLabel);

        TextView colon = new TextView(mActivity);
        colon.setText(": ");
        colon.setTextColor(Color.WHITE);
        colon.setTextSize(TypedValue.COMPLEX_UNIT_SP, 15);
        colon.setLayoutParams(new FrameLayout.LayoutParams(dp(16), dp(28)));
        row.addView(colon);

        TextView controlLabel = new TextView(mActivity);
        controlLabel.setText(control);
        controlLabel.setTextColor(GREEN);
        controlLabel.setTextSize(TypedValue.COMPLEX_UNIT_SP, 15);
        controlLabel.setLayoutParams(new FrameLayout.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT, dp(28)));
        row.addView(controlLabel);

        parent.addView(row);
    }

    private void addTipParagraph(LinearLayout parent, String text) {
        TextView p = new TextView(mActivity);
        p.setText(text);
        p.setTextColor(Color.WHITE);
        p.setTextSize(TypedValue.COMPLEX_UNIT_SP, 14);
        p.setPadding(0, dp(4), 0, dp(4));
        parent.addView(p);
    }

    private void addGap(LinearLayout parent, int height) {
        TextView gap = new TextView(mActivity);
        gap.setText("");
        gap.setLayoutParams(new FrameLayout.LayoutParams(1, height));
        parent.addView(gap);
    }

    private Button makeCloseButton(String label) {
        Button btn = new Button(mActivity);
        btn.setText(label);
        btn.setBackgroundColor(GREEN);
        btn.setLayoutParams(new ViewGroup.LayoutParams(dp(160), dp(44)));
        btn.setGravity(Gravity.CENTER);
        btn.setTextColor(Color.WHITE);
        btn.setTextSize(TypedValue.COMPLEX_UNIT_SP, 16);
        btn.setPadding(dp(12), dp(8), dp(12), dp(8));
        return btn;
    }

    // ===== Public API =====

    public View root() { return mRoot; }

    public void show() {
        mOpen = true;
        mRoot.setVisibility(View.VISIBLE);
    }

    public void hide() {
        mOpen = false;
        mRoot.setVisibility(View.GONE);
    }

    public boolean isOpen() { return mOpen; }

    private int dp(int value) {
        return (int) (value * mActivity.getResources().getDisplayMetrics().density + 0.5f);
    }
}