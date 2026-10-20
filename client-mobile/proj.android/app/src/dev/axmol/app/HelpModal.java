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
    private final Button mCloseBtn;
    private boolean mOpen = false;

    public HelpModal(Activity activity) {
        mActivity = activity;

        mRoot = new FrameLayout(activity);
        mRoot.setBackgroundColor(0xCC000000);

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
        title.setText("\uD83D\uDCD6 How to Play");
        title.setTextColor(Color.WHITE);
        title.setTextSize(TypedValue.COMPLEX_UNIT_SP, 20);
        title.setGravity(Gravity.CENTER);
        title.setTypeface(Typeface.DEFAULT_BOLD);
        panel.addView(title);

        // Scrollable content
        ScrollView scroll = new ScrollView(activity);
        scroll.setLayoutParams(new FrameLayout.LayoutParams(dp(380), dp(480)));

        LinearLayout content = new LinearLayout(activity);
        content.setOrientation(LinearLayout.VERTICAL);
        content.setPadding(dp(10), dp(8), dp(10), dp(8));

        // === Movement Controls ===
        addSectionHeader(content, "\uD83D\uDEAA Movement", 0xFF4CAF50);
        addControlRow(content, "\uD83D\uDCCD", "Drag left side", "Move your character");
        addControlRow(content, "\uD83D\uDD04", "Double-tap left", "Sprint / Dash");
        addControlRow(content, "\uD83E\uDD18", "Pinch/Spread", "Zoom in/out");
        
        addGap(content, dp(8));

        // === Combat Controls ===
        addSectionHeader(content, "\u2694\uFE0F Combat", 0xFFF44336);
        addControlRow(content, "\uD83D\uDD1E", "Tap right side", "Shoot / Attack");
        addControlRow(content, "\uD83C\uDFAF", "Drag right side", "Aim direction");
        addControlRow(content, "\uD83D\uDD27", "Pick up icon", "Collect items");
        addControlRow(content, "\uD83D\uDEE1\uFE0F", "Hold weapon", "Open item menu");
        addControlRow(content, "\uD83D\uDDD2", "Swipe on item", "Switch weapons");
        
        addGap(content, dp(8));

        // === Interaction ===
        addSectionHeader(content, "\u270B Interactions", 0xFF2196F3);
        addControlRow(content, "\uD83D\uDCC5", "Tap door/crate", "Open / Loot");
        addControlRow(content, "\uD83D\uDCED", "Tap teammate", "View stats");
        addControlRow(content, "\uD83D\uDCAC", "Chat button", "Team chat");
        addControlRow(content, "\uD83D\uDE4C", "Emote button", "Quick emote wheel");

        // Separator
        View sep = new View(activity);
        sep.setLayoutParams(new ViewGroup.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, dp(1)));
        sep.setBackgroundColor(0x33FFFFFF);
        content.addView(sep);

        // === Advanced Tips ===
        addSectionHeader(content, "\uD83D\uDCA1 Pro Tips", GOLD);
        
        addTip(content, "\uD83C\uDFD7\uFE0F Building", "Use walls for cover. Build stairs to reach high ground. A 2x2 base with doors is a strong starter.");
        addTip(content, "\uD83D\uDD2B Weapons", "Shotguns dominate close range. Snipers for long distance. SMGs have fast fire rate but low damage.");
        addTip(content, "\uD83D\uDC65 Team", "Stick together! Revive downed teammates before they get eliminated. Share ammo and heals.");
        addTip(content, "\u2604\uFE0F Gas Zone", "The gas shrinks over time. Stay ahead of it but don't rush to the center too early - enemies will be waiting.");

        // Tutorial link stub
        addGap(content, dp(10));
        Button tutorialBtn = makeLinkButton("\u25B6 Watch Tutorial Video");
        tutorialBtn.setOnClickListener(new View.OnClickListener() {
            @Override public void onClick(View v) {
                // Stub: would open YouTube or in-app video
                android.util.Log.i(TAG, "Tutorial video requested (stub)");
            }
        });
        content.addView(tutorialBtn);

        scroll.addView(content);
        panel.addView(scroll);

        // Close button
        mCloseBtn = new Button(activity);
        mCloseBtn.setText("Got it! \u2713");
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
