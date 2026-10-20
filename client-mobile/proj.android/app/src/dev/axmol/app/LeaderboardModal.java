/****************************************************************************
 * LeaderboardModal.java — In-game leaderboard matching web's #ui-leaderboard.
 *
 * Shows the remaining players ranked by kills/damage, with the current player
 * highlighted. Opens from the game HUD (web: tap leaderboard icon).
 * Layout matches web client CSS: dark translucent panel, player rows with
 * rank, name, kills, damage, team indicators.
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
import android.widget.ScrollView;
import android.widget.TextView;

import java.util.ArrayList;
import java.util.List;

public class LeaderboardModal {
    private static final String TAG = "LeaderboardModal";
    private static final int PANEL_BG = 0x90000000;
    private static final int GREEN = 0xFF83AF50;
    private static final int GOLD = 0xFFFFD700;

    private final Activity mActivity;
    private final FrameLayout mRoot;
    private boolean mOpen = false;

    // Leaderboard entry struct
    public static class Entry {
        public int rank;
        public String name;
        public int kills;
        public float damage;
        public boolean isActive;
        public boolean isAlive;
        public int team;
        public int playerId;
    }

    private final LinearLayout mList;
    private Button mCloseBtn;
    private int mActivePlayerId = -1;

    public LeaderboardModal(Activity activity) {
        mActivity = activity;
        mRoot = new FrameLayout(activity);
        mRoot.setBackgroundColor(0xCC000000);

        // Full-height ScrollView for landscape responsiveness
        ScrollView outerScroll = new ScrollView(activity);
        outerScroll.setLayoutParams(new FrameLayout.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.MATCH_PARENT));

        LinearLayout panel = new LinearLayout(activity);
        panel.setOrientation(LinearLayout.VERTICAL);
        panel.setGravity(Gravity.CENTER);
        panel.setBackgroundColor(PANEL_BG);
        panel.setPadding(dp(20), dp(12), dp(20), dp(12));

        // Title row: "Leaderboard" + close
        LinearLayout headerRow = new LinearLayout(activity);
        headerRow.setOrientation(LinearLayout.HORIZONTAL);
        headerRow.setGravity(Gravity.CENTER);

        TextView title = new TextView(activity);
        title.setText("Leaderboard");
        title.setTextColor(GOLD);
        title.setTextSize(TypedValue.COMPLEX_UNIT_SP, 22);
        title.setGravity(Gravity.CENTER);
        title.setPadding(0, 0, 0, dp(8));
        headerRow.addView(title);
        panel.addView(headerRow);

        // Column headers
        LinearLayout colHeader = new LinearLayout(activity);
        colHeader.setOrientation(LinearLayout.HORIZONTAL);
        colHeader.setGravity(Gravity.CENTER);
        colHeader.setPadding(dp(4), dp(4), dp(4), dp(4));

        TextView rankH = makeColHeader("#", 30);
        TextView nameH = makeColHeader("Player", 160);
        TextView killsH = makeColHeader("Kills", 50);
        TextView dmgH = makeColHeader("Damage", 60);
        TextView statusH = makeColHeader("H", 30);

        colHeader.addView(rankH);
        colHeader.addView(nameH);
        colHeader.addView(killsH);
        colHeader.addView(dmgH);
        colHeader.addView(statusH);
        panel.addView(colHeader);

        // Player list (scrollable)
        ScrollView scroll = new ScrollView(activity);
        scroll.setLayoutParams(new FrameLayout.LayoutParams(dp(360), dp(400)));

        mList = new LinearLayout(activity);
        mList.setOrientation(LinearLayout.VERTICAL);
        scroll.addView(mList);
        panel.addView(scroll);

        // Close button
        mCloseBtn = makeCloseButton("Close");
        mCloseBtn.setOnClickListener(new View.OnClickListener() {
            @Override public void onClick(View v) { hide(); }
        });
        panel.addView(mCloseBtn);

        outerScroll.addView(panel);

        mRoot.addView(outerScroll);
    }

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

    public void setActivePlayerId(int id) {
        mActivePlayerId = id;
    }

    /** Update the leaderboard with current player data. */
    public void update(List<Entry> entries) {
        mList.removeAllViews();
        if (entries == null || entries.isEmpty()) {
            TextView empty = new TextView(mActivity);
            empty.setText("No data");
            empty.setTextColor(0xFF808080);
            empty.setTextSize(TypedValue.COMPLEX_UNIT_SP, 14);
            mList.addView(empty);
            return;
        }

        for (int i = 0; i < entries.size(); i++) {
            final Entry e = entries.get(i);
            LinearLayout row = new LinearLayout(mActivity);
            row.setOrientation(LinearLayout.HORIZONTAL);
            row.setGravity(Gravity.CENTER);
            row.setPadding(dp(4), dp(2), dp(4), dp(2));

            boolean isActive = e.playerId == mActivePlayerId;
            int textColor = isActive ? GREEN : (e.isAlive ? Color.WHITE : 0xFF808080);
            int bgColor = isActive ? 0x3383AF50 : (i % 2 == 0 ? 0x22000000 : 0x44000000);

            row.setBackgroundColor(bgColor);

            TextView rankTv = makeCell(String.valueOf(i + 1), 30, textColor, 14);
            TextView nameTv = makeCell(e.name, 160, textColor, 14);
            TextView killsTv = makeCell(String.valueOf(e.kills), 50, textColor, 14);
            TextView dmgTv = makeCell(String.format("%.0f", e.damage), 60, textColor, 14);
            TextView statusTv = makeCell(e.isAlive ? "A" : "D", 30, e.isAlive ? 0xFF83AF50 : 0xFFFF7878, 12);

            row.addView(rankTv);
            row.addView(nameTv);
            row.addView(killsTv);
            row.addView(dmgTv);
            row.addView(statusTv);
            mList.addView(row);
        }
    }

    // ===== Helpers =====

    private TextView makeColHeader(String text, int width) {
        TextView tv = new TextView(mActivity);
        tv.setText(text);
        tv.setTextColor(GOLD);
        tv.setTextSize(TypedValue.COMPLEX_UNIT_SP, 12);
        tv.setLayoutParams(new FrameLayout.LayoutParams(dp(width), dp(24)));
        tv.setGravity(Gravity.CENTER);
        return tv;
    }

    private TextView makeCell(String text, int width, int color, float size) {
        TextView tv = new TextView(mActivity);
        tv.setText(text);
        tv.setTextColor(color);
        tv.setTextSize(TypedValue.COMPLEX_UNIT_SP, size);
        tv.setLayoutParams(new FrameLayout.LayoutParams(dp(width), dp(24)));
        tv.setGravity(Gravity.CENTER);
        return tv;
    }

    private Button makeCloseButton(String label) {
        Button btn = new Button(mActivity);
        btn.setText(label);
        btn.setBackgroundColor(GREEN);
        btn.setLayoutParams(new ViewGroup.LayoutParams(dp(140), dp(40)));
        btn.setGravity(Gravity.CENTER);
        btn.setTextColor(Color.WHITE);
        btn.setTextSize(TypedValue.COMPLEX_UNIT_SP, 15);
        return btn;
    }

    private int dp(int value) {
        return (int) (value * mActivity.getResources().getDisplayMetrics().density + 0.5f);
    }
}