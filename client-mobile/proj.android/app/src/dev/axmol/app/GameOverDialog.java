/****************************************************************************
 * GameOverDialog.java — final result screen (ui.ts showStats).
 *
 * The web client builds #ui-stats: a gold victory/defeat title, the Rank /
 * Team Kills overview with an underline, one stats card per player
 * (name, Kills, Damage Dealt, Damage Taken, Survived) and the PLAY NEW GAME
 * (plus SPECTATE while the match is still running) actions.
 *
 * The C++ HUD used to draw a plain text panel for this. It now only latches the
 * modal state; `Game::gameOverJson` carries the same data here so the result
 * matches the web design (gold title, dark cards, and the mobile 125px card
 * proportions scaled for touch).
 ****************************************************************************/
package dev.axmol.app;

import android.app.Activity;
import android.graphics.Color;
import android.graphics.Typeface;
import android.graphics.drawable.Drawable;
import android.graphics.drawable.GradientDrawable;
import android.graphics.drawable.LayerDrawable;
import android.text.SpannableStringBuilder;
import android.text.Spanned;
import android.text.style.ForegroundColorSpan;
import android.text.style.RelativeSizeSpan;
import android.text.style.StyleSpan;
import android.util.TypedValue;
import android.view.Gravity;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewGroup;
import android.widget.FrameLayout;
import android.widget.HorizontalScrollView;
import android.widget.LinearLayout;
import android.widget.ScrollView;
import android.widget.TextView;

public class GameOverDialog {
    // ui.ts .ui-stats-header-title / .ui-stats-info-player-name: gold.
    private static final int GOLD = 0xFFFFD700;
    private static final int TEXT = 0xFFFFFFFF;
    private static final int TEXT_MUTED = 0xFFEDEDED;
    // #ui-stats-info-player: rgba(0,0,0,.75) with a black border.
    private static final int CARD_BG = 0xCC000000;
    // The native result keeps the reference's dark maroon scrim (the old C++ HUD
    // used a plain black dim; the game stays faintly readable behind it).
    private static final int SCRIM = 0xC014080B;
    private static final int PURPLE = 0xFF9B59B6;
    private static final int PURPLE_SHADOW = 0xFF6E3D7F;
    private static final int TEAL = 0xFF50AFAB;
    private static final int TEAL_SHADOW = 0xFF387C79;
    private static final int RED = 0xFFCC0000;
    private static final int BLUE = 0xFF007EFF;

    private final Activity mActivity;
    private final FrameLayout mRoot;
    private final JavaUiHost mHost;
    private boolean mOpen = false;
    private Json.GameOverEntry mData = new Json.GameOverEntry();

    public GameOverDialog(Activity activity, JavaUiHost host) {
        mActivity = activity;
        mHost = host;
        mRoot = new FrameLayout(activity);
        mRoot.setBackgroundColor(SCRIM);
        // The result screen is modal: swallow taps that miss a button.
        mRoot.setClickable(true);
        mRoot.setFocusable(true);
        mRoot.setVisibility(View.GONE);
    }

    public View root() { return mRoot; }

    public boolean isOpen() { return mOpen; }

    // ===== Public API =====

    /** Show the result for `Game::gameOverJson` (rebuilt for each message). */
    public void show(String json) {
        mData = Json.parseGameOver(json);
        build();
        mOpen = true;
        mRoot.setVisibility(View.VISIBLE);
        mRoot.setAlpha(0.0f);
        mRoot.animate().alpha(1.0f).setDuration(200).start();
    }

    public void hide() {
        mOpen = false;
        mRoot.animate().cancel();
        mRoot.setAlpha(1.0f);
        mRoot.setVisibility(View.GONE);
        mRoot.removeAllViews();
    }

    // ===== Build =====

    private void build() {
        mRoot.removeAllViews();

        ScrollView scroll = new ScrollView(mActivity);
        scroll.setFillViewport(true);
        scroll.setVerticalScrollBarEnabled(false);

        LinearLayout content = new LinearLayout(mActivity);
        content.setOrientation(LinearLayout.VERTICAL);
        content.setGravity(Gravity.CENTER);
        content.setPadding(dp(18), dp(10), dp(18), dp(10));
        // Center the column on tall screens while still allowing a scroll when
        // four player cards do not fit the landscape height.
        content.setMinimumHeight(Math.max(0, screenHeight() - dp(8)));

        content.addView(makeTitle(), new LinearLayout.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT));

        addSpace(content, dp(8));
        content.addView(makeOverview(), new LinearLayout.LayoutParams(
                ViewGroup.LayoutParams.WRAP_CONTENT, ViewGroup.LayoutParams.WRAP_CONTENT));

        if (!mData.players.isEmpty()) {
            addSpace(content, dp(12));
            content.addView(makeCards(), new LinearLayout.LayoutParams(
                    ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT));
        }

        addSpace(content, dp(16));
        content.addView(makeButtons(), new LinearLayout.LayoutParams(
                ViewGroup.LayoutParams.WRAP_CONTENT, ViewGroup.LayoutParams.WRAP_CONTENT));

        scroll.addView(content, new FrameLayout.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT));
        mRoot.addView(scroll, new FrameLayout.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.MATCH_PARENT));
    }

    private TextView makeTitle() {
        final String text = titleText();
        TextView title = new TextView(mActivity);
        title.setText(text);
        title.setTextColor(GOLD);
        title.setTextSize(TypedValue.COMPLEX_UNIT_SP, text.length() > 24 ? 24 : 28);
        title.setTypeface(Typeface.DEFAULT_BOLD);
        title.setGravity(Gravity.CENTER);
        title.setIncludeFontPadding(false);
        title.setMaxLines(2);
        title.setShadowLayer(dp(2), 0, dp(2), 0xCC000000);
        return title;
    }

    /**
     * ui.ts getTitleVictoryText / getTitleDefeatText. `victory` and
     * `spectatingAnotherTeam` are computed natively from the same conditions
     * (isLocalTeamWinner = localTeamId == winningTeamId ||
     * (spectating && winningTeamId == teamId)).
     */
    private String titleText() {
        if (mData.victory) {
            if (mData.spectatingAnotherTeam && !mData.spectatedName.isEmpty()) {
                return mData.spectatedName + " " + L10n.t("game-won-the-game", "won the game.");
            }
            return mData.turkeyMode
                    ? L10n.t("game-turkey", "Winner winner turkey dinner!")
                    : L10n.t("game-chicken", "Winner winner chicken dinner!");
        }
        if (mData.spectatingAnotherTeam && !mData.spectatedName.isEmpty()) {
            return mData.spectatedName + " " + L10n.t("game-player-died", "died") + ".";
        }
        if (mData.teamMode > 1) {
            return L10n.t("game-team-eliminated", "Your team was eliminated.");
        }
        return L10n.t("game-You", "You") + " " + L10n.t("game-you-died", "died") + ".";
    }

    /** ui.ts getOverviewElems: faction counts, or Rank (+ Team Kills) with an underline. */
    private LinearLayout makeOverview() {
        LinearLayout row = new LinearLayout(mActivity);
        row.setOrientation(LinearLayout.HORIZONTAL);
        row.setGravity(Gravity.CENTER);

        if (mData.factionMode) {
            row.addView(makeOverviewGroup(
                    L10n.t("game-red-team", "Red"), String.valueOf(mData.aliveRed), RED, 18, 24));
            addHorizontalGap(row, dp(32));
            row.addView(makeOverviewGroup(
                    L10n.t("game-blue-team", "Blue"), String.valueOf(mData.aliveBlue), BLUE, 18, 24));
            return row;
        }

        final String rankKey;
        switch (mData.teamMode) {
            case 1: rankKey = "game-solo-rank"; break;
            case 2: rankKey = "game-duo-rank"; break;
            case 4: rankKey = "game-squad-rank"; break;
            default: rankKey = "game-rank"; break;
        }
        row.addView(makeOverviewGroup(L10n.t(rankKey, "Rank"), "#" + mData.teamRank,
                TEXT, 20, 28));
        if (mData.teamMode > 1) {
            addHorizontalGap(row, dp(36));
            row.addView(makeOverviewGroup(L10n.t("game-team-kills", "Team Kills"),
                    String.valueOf(mData.teamKills), TEXT, 20, 28));
        }
        return row;
    }

    /** Label + large bold value, underlined like `.ui-stats-header-overview div`. */
    private LinearLayout makeOverviewGroup(String label, String value, int color,
                                           int labelSp, int valueSp) {
        LinearLayout group = new LinearLayout(mActivity);
        group.setOrientation(LinearLayout.VERTICAL);
        group.setGravity(Gravity.CENTER_HORIZONTAL);

        SpannableStringBuilder sb = new SpannableStringBuilder();
        sb.append(label);
        sb.setSpan(new ForegroundColorSpan(color), 0, sb.length(),
                Spanned.SPAN_EXCLUSIVE_EXCLUSIVE);
        sb.append("  ");
        final int start = sb.length();
        sb.append(value);
        sb.setSpan(new RelativeSizeSpan(valueSp / (float) labelSp), start, sb.length(),
                Spanned.SPAN_EXCLUSIVE_EXCLUSIVE);
        sb.setSpan(new StyleSpan(Typeface.BOLD), start, sb.length(),
                Spanned.SPAN_EXCLUSIVE_EXCLUSIVE);

        TextView text = new TextView(mActivity);
        text.setText(sb);
        text.setTextColor(color);
        text.setTextSize(TypedValue.COMPLEX_UNIT_SP, labelSp);
        text.setGravity(Gravity.CENTER);
        text.setIncludeFontPadding(false);
        text.setShadowLayer(dp(1), 0, dp(1), 0x99000000);
        group.addView(text, new LinearLayout.LayoutParams(
                ViewGroup.LayoutParams.WRAP_CONTENT, ViewGroup.LayoutParams.WRAP_CONTENT));

        View underline = new View(mActivity);
        underline.setBackgroundColor(0x40FFFFFF);
        LinearLayout.LayoutParams lineLp = new LinearLayout.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT, dp(1));
        lineLp.topMargin = dp(3);
        group.addView(underline, lineLp);
        return group;
    }

    /**
     * One card per GameOverMsg.playerStats entry, matching the web's flex row.
     * Teams put the cards side by side (125px on the small web layout, 150dp
     * here); a solo result gets the wider single card from the reference.
     */
    private HorizontalScrollView makeCards() {
        LinearLayout row = new LinearLayout(mActivity);
        row.setOrientation(LinearLayout.HORIZONTAL);
        row.setGravity(Gravity.CENTER);
        // HorizontalScrollView children are left-aligned; the minimum width
        // keeps a short (solo / duo) row centred.
        row.setMinimumWidth(Math.max(0, screenWidth() - dp(36)));

        final boolean multi = mData.players.size() > 1;
        final int count = Math.max(1, mData.players.size());
        // Fit the whole team on one row when four cards share the screen; the
        // web's 125px mobile card scales to ~145dp here. Narrow screens still
        // scroll horizontally instead of squeezing the labels.
        final int available = Math.max(dp(150), screenWidth() - dp(36));
        final int cardW = multi
                ? Math.min(dp(150), (available - dp(8) * (count - 1)) / count)
                : Math.min(dp(320), screenWidth() - dp(56));
        for (int i = 0; i < mData.players.size(); i++) {
            LinearLayout.LayoutParams lp = new LinearLayout.LayoutParams(
                    cardW, ViewGroup.LayoutParams.WRAP_CONTENT);
            if (i > 0) lp.leftMargin = dp(8);
            row.addView(makeCard(mData.players.get(i), multi), lp);
        }

        HorizontalScrollView scroll = new HorizontalScrollView(mActivity);
        scroll.setHorizontalScrollBarEnabled(false);
        scroll.setFillViewport(true);
        scroll.addView(row, new FrameLayout.LayoutParams(
                ViewGroup.LayoutParams.WRAP_CONTENT, ViewGroup.LayoutParams.WRAP_CONTENT));
        return scroll;
    }

    private FrameLayout makeCard(Json.GameOverPlayer player, boolean multi) {
        FrameLayout card = new FrameLayout(mActivity);
        GradientDrawable bg = new GradientDrawable();
        bg.setColor(CARD_BG);
        bg.setCornerRadius(dp(5));
        bg.setStroke(dp(2), Color.BLACK);
        card.setBackground(bg);

        // .ui-stats-info-status:after: a 10% skull behind a dead player's stats.
        if (player.dead) {
            TextView skull = new TextView(mActivity);
            skull.setText("\u2620");
            skull.setTextColor(Color.WHITE);
            skull.setTextSize(TypedValue.COMPLEX_UNIT_SP, multi ? 54 : 72);
            skull.setGravity(Gravity.CENTER);
            skull.setAlpha(0.10f);
            card.addView(skull, new FrameLayout.LayoutParams(
                    ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.MATCH_PARENT));
        }

        LinearLayout body = new LinearLayout(mActivity);
        body.setOrientation(LinearLayout.VERTICAL);
        int pad = multi ? dp(8) : dp(14);
        body.setPadding(pad, multi ? dp(6) : dp(8), pad, multi ? dp(7) : dp(10));

        TextView name = new TextView(mActivity);
        name.setText(player.name != null && !player.name.isEmpty() ? player.name : "Player");
        name.setTextColor(GOLD);
        name.setTextSize(TypedValue.COMPLEX_UNIT_SP, multi ? 14 : 18);
        name.setTypeface(Typeface.create(Typeface.DEFAULT, Typeface.BOLD_ITALIC));
        name.setGravity(Gravity.CENTER);
        name.setSingleLine(true);
        name.setEllipsize(android.text.TextUtils.TruncateAt.END);
        name.setIncludeFontPadding(false);
        LinearLayout.LayoutParams nameLp = new LinearLayout.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT);
        nameLp.bottomMargin = multi ? dp(4) : dp(6);
        body.addView(name, nameLp);

        body.addView(makeStatRow(L10n.t("game-kills", "Kills"),
                String.valueOf(player.kills), multi));
        body.addView(makeStatRow(L10n.t("game-damage-dealt", "Damage Dealt"),
                String.valueOf(player.damageDealt), multi));
        body.addView(makeStatRow(L10n.t("game-damage-taken", "Damage Taken"),
                String.valueOf(player.damageTaken), multi));
        body.addView(makeStatRow(L10n.t("game-survived", "Survived"),
                humanizeTime(player.timeAlive), multi));

        card.addView(body, new FrameLayout.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT));
        return card;
    }

    /** .ui-stats-info: label right-aligned in 55%, bold value in the rest. */
    private LinearLayout makeStatRow(String label, String value, boolean multi) {
        LinearLayout row = new LinearLayout(mActivity);
        row.setOrientation(LinearLayout.HORIZONTAL);
        row.setGravity(Gravity.CENTER_VERTICAL);

        TextView labelView = new TextView(mActivity);
        labelView.setText(label);
        labelView.setTextColor(TEXT_MUTED);
        labelView.setTextSize(TypedValue.COMPLEX_UNIT_SP, multi ? 10 : 13);
        labelView.setGravity(Gravity.END);
        labelView.setIncludeFontPadding(false);
        LinearLayout.LayoutParams labelLp = new LinearLayout.LayoutParams(
                0, ViewGroup.LayoutParams.WRAP_CONTENT, 0.55f);
        row.addView(labelView, labelLp);

        TextView valueView = new TextView(mActivity);
        valueView.setText(value);
        valueView.setTextColor(TEXT);
        valueView.setTextSize(TypedValue.COMPLEX_UNIT_SP, multi ? 10 : 13);
        valueView.setTypeface(Typeface.DEFAULT_BOLD);
        valueView.setGravity(Gravity.START);
        valueView.setIncludeFontPadding(false);
        valueView.setPadding(dp(4), 0, 0, 0);
        LinearLayout.LayoutParams valueLp = new LinearLayout.LayoutParams(
                0, ViewGroup.LayoutParams.WRAP_CONTENT, 0.45f);
        row.addView(valueView, valueLp);
        return row;
    }

    /** .ui-stats-options: PLAY NEW GAME, plus SPECTATE while the match runs. */
    private LinearLayout makeButtons() {
        LinearLayout bar = new LinearLayout(mActivity);
        bar.setOrientation(LinearLayout.HORIZONTAL);
        bar.setGravity(Gravity.CENTER);

        TextView play = makeButton(L10n.t("game-play-new-game", "Play New Game"),
                PURPLE, PURPLE_SHADOW);
        play.setOnClickListener(new View.OnClickListener() {
            @Override public void onClick(View v) {
                hide();
                // ui.ts quitGame(): m_gameOver = true + onQuit -> menu/team room.
                if (mHost != null) mHost.quitToMenu();
            }
        });
        bar.addView(play, new LinearLayout.LayoutParams(
                mData.canSpectate ? dp(150) : dp(220), dp(44)));

        if (mData.canSpectate) {
            addHorizontalGap(bar, dp(10));
            TextView spectate = makeButton(L10n.t("game-spectate", "Spectate"),
                    TEAL, TEAL_SHADOW);
            spectate.setOnClickListener(new View.OnClickListener() {
                @Override public void onClick(View v) {
                    hide();
                    // ui.ts beginSpectating(): game.ts sends SpectateMsg Begin
                    // on the next update; the C++ spectating overlay takes over.
                    SurvBridge.spectate(1);
                }
            });
            bar.addView(spectate, new LinearLayout.LayoutParams(dp(150), dp(44)));
        }
        return bar;
    }

    private TextView makeButton(String label, final int body, final int shadow) {
        TextView btn = new TextView(mActivity);
        btn.setText(label);
        btn.setTextColor(Color.WHITE);
        btn.setTextSize(TypedValue.COMPLEX_UNIT_SP, 15);
        btn.setTypeface(Typeface.DEFAULT_BOLD);
        btn.setGravity(Gravity.CENTER);
        btn.setIncludeFontPadding(false);
        btn.setShadowLayer(dp(1), 0, dp(1), 0x66000000);
        btn.setClickable(true);
        btn.setBackground(buttonBg(body, shadow));
        btn.setOnTouchListener(new View.OnTouchListener() {
            @Override public boolean onTouch(View v, MotionEvent e) {
                int action = e.getActionMasked();
                if (action == MotionEvent.ACTION_DOWN) {
                    v.setAlpha(0.72f);
                } else if (action == MotionEvent.ACTION_UP || action == MotionEvent.ACTION_CANCEL) {
                    v.setAlpha(1.0f);
                }
                return false;
            }
        });
        return btn;
    }

    /** Rounded body with a darker inset bottom edge (MenuFragment's buttons). */
    private LayerDrawable buttonBg(int body, int shadow) {
        GradientDrawable bottom = new GradientDrawable();
        bottom.setColor(shadow);
        bottom.setCornerRadius(dp(6));
        GradientDrawable top = new GradientDrawable();
        top.setColor(body);
        top.setCornerRadius(dp(6));
        LayerDrawable ld = new LayerDrawable(new Drawable[] { bottom, top });
        ld.setLayerInset(1, 0, 0, 0, dp(3));
        return ld;
    }

    // ===== Helpers =====

    /** ui.ts humanizeTime: "4m 14s" (hours only when needed). */
    private static String humanizeTime(int time) {
        final int hours = time / 3600;
        final int minutes = (time / 60) % 60;
        final int seconds = time % 60;
        String text = "";
        if (hours > 0) text += hours + "h ";
        if (hours > 0 || minutes > 0) text += minutes + "m ";
        return text + seconds + "s";
    }

    private void addSpace(LinearLayout parent, int height) {
        View gap = new View(mActivity);
        parent.addView(gap, new LinearLayout.LayoutParams(1, height));
    }

    private void addHorizontalGap(LinearLayout parent, int width) {
        View gap = new View(mActivity);
        parent.addView(gap, new LinearLayout.LayoutParams(width, 1));
    }

    private int screenWidth() {
        return mActivity.getResources().getDisplayMetrics().widthPixels;
    }

    private int screenHeight() {
        return mActivity.getResources().getDisplayMetrics().heightPixels;
    }

    private int dp(int value) {
        return (int) (value * mActivity.getResources().getDisplayMetrics().density + 0.5f);
    }
}
