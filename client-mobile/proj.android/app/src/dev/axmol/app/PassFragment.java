/****************************************************************************
 * PassFragment.java - Survev Pass / quests screen (client/src/ui/pass.ts).
 *
 * The Java screen is the M8 account pass view: real XP/level from
 * `/api/user/get_pass`, the next unlock item from the generated PassDefs table,
 * and the two server quest slots with progress, XP, completion state and the
 * reroll flow (`/api/user/refresh_quest`). There is no premium purchase: the
 * web pass has none, and the server's `set_pass_unlock` is a no-op.
 *
 * Threading: AccountManager callbacks arrive on the UI thread; SurvBridge calls
 * (l10n) happen there too.
 ****************************************************************************/
package dev.axmol.app;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.DialogInterface;
import android.graphics.Bitmap;
import android.graphics.Color;
import android.graphics.drawable.GradientDrawable;
import android.os.Handler;
import android.os.Looper;
import android.util.Log;
import android.util.TypedValue;
import android.view.Gravity;
import android.view.View;
import android.view.ViewGroup;
import android.view.animation.OvershootInterpolator;
import android.widget.FrameLayout;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.ScrollView;
import android.widget.TextView;

import java.util.ArrayList;
import java.util.List;

public class PassFragment {
    private static final String TAG = "PassFragment";

    private static final int PASS_PURPLE = 0xFF9B59B6;
    private static final int GOLD = 0xFFFFD700;
    private static final int GREEN = 0xFF83AF50;
    private static final int BLUE = 0xFF3498DB;
    private static final int XP_BLUE = 0xFF2980B9;
    private static final int PANEL_BG = 0xF01A1A2E;
    private static final int BAR_BG = 0xFF333333;

    private final Activity mActivity;
    private final FrameLayout mRoot;
    private final AccountManager mAccount;
    private final Handler mHandler = new Handler(Looper.getMainLooper());

    private final TextView mPassName;
    private final TextView mLevelLabel;
    private final TextView mXpLabel;
    private final LinearLayout mXpBarContainer;
    private final View mXpBarBg;
    private final View mXpBarFill;
    private final TextView mUnlockLabel;
    private final ImageView mUnlockImage;
    private final ImageView mUnlockTypeIcon;
    private final TextView mUnlockTitle;
    private final LinearLayout mRewardRow;
    private final FrameLayout mArtBox;
    private final LinearLayout mQuestList;
    private final TextView mStatusText;

    private final List<QuestRow> mQuestRows = new ArrayList<QuestRow>();
    private AccountManager.PassInfo mPass;
    private boolean mOpen = false;
    private boolean mLoading = false;
    // Level-up pulse bookkeeping (pass.ts animatePassLevelUp): the level and
    // pass type the reward art currently shows.
    private int mRenderedLevel = 0;
    private String mRenderedPassType = "";

    private final Runnable mTimerRunnable = new Runnable() {
        @Override
        public void run() {
            if (!mOpen) {
                return;
            }
            updateQuestTimers();
            mHandler.postDelayed(this, 1000);
        }
    };

    public PassFragment(Activity activity) {
        mActivity = activity;
        mAccount = AccountManager.get(activity);

        mRoot = new FrameLayout(activity);
        mRoot.setBackgroundColor(0xCC000000);
        mRoot.setVisibility(View.GONE);

        ScrollView outerScroll = new ScrollView(activity);
        outerScroll.setLayoutParams(new FrameLayout.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.MATCH_PARENT));

        LinearLayout panel = new LinearLayout(activity);
        panel.setOrientation(LinearLayout.VERTICAL);
        panel.setBackgroundColor(PANEL_BG);
        panel.setPadding(dp(18), dp(14), dp(18), dp(14));
        FrameLayout.LayoutParams panelParams = new FrameLayout.LayoutParams(
                dp(420), ViewGroup.LayoutParams.WRAP_CONTENT);
        panelParams.gravity = Gravity.CENTER;
        panel.setLayoutParams(panelParams);

        // ===== Header =====
        LinearLayout headerRow = new LinearLayout(activity);
        headerRow.setOrientation(LinearLayout.HORIZONTAL);
        headerRow.setGravity(Gravity.CENTER_VERTICAL);

        TextView titleIcon = new TextView(activity);
        titleIcon.setText("\uD83C\uDFC6");
        titleIcon.setTextSize(TypedValue.COMPLEX_UNIT_SP, 22);
        headerRow.addView(titleIcon);

        mPassName = new TextView(activity);
        mPassName.setText("SURVEV PASS");
        mPassName.setTextColor(GOLD);
        mPassName.setTextSize(TypedValue.COMPLEX_UNIT_SP, 20);
        mPassName.setTypeface(null, android.graphics.Typeface.BOLD);
        mPassName.setLayoutParams(new LinearLayout.LayoutParams(
                0, ViewGroup.LayoutParams.WRAP_CONTENT, 1f));
        headerRow.addView(mPassName);

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
                if (mOnCloseListener != null) mOnCloseListener.onClose();
            }
        });
        headerRow.addView(close, new LinearLayout.LayoutParams(
                ViewGroup.LayoutParams.WRAP_CONTENT, dp(36)));
        panel.addView(headerRow);

        // ===== Level + XP =====
        LinearLayout levelRow = new LinearLayout(activity);
        levelRow.setOrientation(LinearLayout.HORIZONTAL);
        levelRow.setGravity(Gravity.CENTER_VERTICAL);
        levelRow.setPadding(0, dp(10), 0, 0);

        mLevelLabel = new TextView(activity);
        mLevelLabel.setText("LEVEL 1");
        mLevelLabel.setTextColor(GOLD);
        mLevelLabel.setTextSize(TypedValue.COMPLEX_UNIT_SP, 18);
        mLevelLabel.setTypeface(null, android.graphics.Typeface.BOLD);
        levelRow.addView(mLevelLabel);

        mXpLabel = new TextView(activity);
        mXpLabel.setText("0/50 XP");
        mXpLabel.setTextColor(XP_BLUE);
        mXpLabel.setTextSize(TypedValue.COMPLEX_UNIT_SP, 12);
        mXpLabel.setGravity(Gravity.END);
        mXpLabel.setLayoutParams(new LinearLayout.LayoutParams(
                0, ViewGroup.LayoutParams.WRAP_CONTENT, 1f));
        levelRow.addView(mXpLabel);
        panel.addView(levelRow);

        mXpBarContainer = new LinearLayout(activity);
        mXpBarContainer.setOrientation(LinearLayout.HORIZONTAL);
        mXpBarBg = new View(activity);
        mXpBarBg.setBackgroundColor(BAR_BG);
        mXpBarBg.setLayoutParams(new LinearLayout.LayoutParams(0, dp(14), 1f));
        mXpBarFill = new View(activity);
        mXpBarFill.setBackgroundColor(XP_BLUE);
        mXpBarFill.setLayoutParams(new LinearLayout.LayoutParams(0, dp(14), 0f));
        mXpBarContainer.addView(mXpBarBg);
        mXpBarContainer.addView(mXpBarFill);
        LinearLayout.LayoutParams barParams = new LinearLayout.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT, dp(14));
        barParams.topMargin = dp(4);
        panel.addView(mXpBarContainer, barParams);

        mUnlockLabel = new TextView(activity);
        mUnlockLabel.setTextColor(0xFFCCCCCC);
        mUnlockLabel.setTextSize(TypedValue.COMPLEX_UNIT_SP, 12);
        mUnlockLabel.setPadding(0, dp(6), 0, dp(6));

        // pass.ts setPassUnlockImage: the next unlock's art + category icon and
        // title, next to the existing "Next reward" label.
        LinearLayout rewardRow = new LinearLayout(activity);
        rewardRow.setOrientation(LinearLayout.HORIZONTAL);
        rewardRow.setGravity(Gravity.CENTER_VERTICAL);
        rewardRow.setPadding(0, dp(6), 0, dp(2));
        mRewardRow = rewardRow;

        FrameLayout artBox = new FrameLayout(activity);
        artBox.setLayoutParams(new LinearLayout.LayoutParams(dp(84), dp(84)));
        mArtBox = artBox;
        mUnlockImage = new ImageView(activity);
        mUnlockImage.setScaleType(ImageView.ScaleType.FIT_CENTER);
        artBox.addView(mUnlockImage, new FrameLayout.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.MATCH_PARENT));
        mUnlockTypeIcon = new ImageView(activity);
        mUnlockTypeIcon.setScaleType(ImageView.ScaleType.FIT_CENTER);
        mUnlockTypeIcon.setBackgroundColor(0x66000000);
        FrameLayout.LayoutParams iconParams = new FrameLayout.LayoutParams(dp(26), dp(26));
        iconParams.gravity = Gravity.BOTTOM | Gravity.END;
        artBox.addView(mUnlockTypeIcon, iconParams);
        rewardRow.addView(artBox);

        LinearLayout rewardText = new LinearLayout(activity);
        rewardText.setOrientation(LinearLayout.VERTICAL);
        rewardText.setPadding(dp(10), 0, 0, 0);
        rewardText.setLayoutParams(new LinearLayout.LayoutParams(
                0, ViewGroup.LayoutParams.WRAP_CONTENT, 1f));

        mUnlockTitle = new TextView(activity);
        mUnlockTitle.setTextColor(GOLD);
        mUnlockTitle.setTextSize(TypedValue.COMPLEX_UNIT_SP, 11);
        mUnlockTitle.setTypeface(null, android.graphics.Typeface.BOLD);
        mUnlockTitle.setSingleLine(true);
        rewardText.addView(mUnlockTitle);
        rewardText.addView(mUnlockLabel);
        rewardRow.addView(rewardText);
        panel.addView(rewardRow);

        // ===== Quests =====
        TextView questHeader = new TextView(activity);
        questHeader.setText("\uD83D\uDCDC QUESTS");
        questHeader.setTextColor(GOLD);
        questHeader.setTextSize(TypedValue.COMPLEX_UNIT_SP, 14);
        questHeader.setTypeface(null, android.graphics.Typeface.BOLD);
        panel.addView(questHeader);

        mQuestList = new LinearLayout(activity);
        mQuestList.setOrientation(LinearLayout.VERTICAL);
        mQuestList.setPadding(0, dp(2), 0, dp(2));
        panel.addView(mQuestList);

        mStatusText = new TextView(activity);
        mStatusText.setTextColor(0xFF808080);
        mStatusText.setTextSize(TypedValue.COMPLEX_UNIT_SP, 11);
        mStatusText.setGravity(Gravity.CENTER);
        mStatusText.setPadding(0, dp(6), 0, 0);
        panel.addView(mStatusText);

        outerScroll.addView(panel);
        mRoot.addView(outerScroll);
    }

    public interface OnCloseListener {
        void onClose();
    }

    private OnCloseListener mOnCloseListener;

    public void setOnClose(OnCloseListener listener) {
        mOnCloseListener = listener;
    }

    public View root() {
        return mRoot;
    }

    public void show() {
        if (mOpen) {
            return;
        }
        mOpen = true;
        mRoot.setVisibility(View.VISIBLE);
        mHandler.post(mTimerRunnable);
        loadPass();
        Log.i(TAG, "pass show");
    }

    public void hide() {
        if (!mOpen) {
            return;
        }
        mOpen = false;
        mRoot.setVisibility(View.GONE);
        mHandler.removeCallbacks(mTimerRunnable);
        Log.i(TAG, "pass hide");
    }

    public boolean isOpen() {
        return mOpen;
    }

    /** Re-reads the cached account state; the network refresh happens on show. */
    public void refresh() {
        if (!mAccount.isLoggedIn()) {
            renderLoggedOut();
            return;
        }
        if (mPass == null) {
            loadPass();
        } else {
            render();
        }
    }

    // --- data ---------------------------------------------------------------

    private void loadPass() {
        if (mLoading) {
            return;
        }
        if (!mAccount.isLoggedIn()) {
            renderLoggedOut();
            return;
        }
        mLoading = true;
        mStatusText.setText("Loading pass...");
        mAccount.getPass(true, new AccountManager.PassCallback() {
            @Override
            public void onResult(boolean success, AccountManager.PassInfo pass) {
                mLoading = false;
                if (pass != null) {
                    mPass = pass;
                }
                if (!success) {
                    mStatusText.setText("Failed to load pass.");
                    render();
                    return;
                }
                mStatusText.setText("");
                render();
            }
        });
    }

    private void renderLoggedOut() {
        mPass = null;
        mPassName.setText("SURVEV PASS");
        mQuestList.removeAllViews();
        mQuestRows.clear();
        mStatusText.setText(l10n("quest-login-prompt", "Log in to get quests!"));
        mLevelLabel.setText("LEVEL -");
        mXpLabel.setText("-");
        mUnlockLabel.setText("");
        mUnlockImage.setVisibility(View.GONE);
        mUnlockTypeIcon.setVisibility(View.GONE);
        mUnlockTitle.setVisibility(View.GONE);
        mRewardRow.setVisibility(View.GONE);
        mRenderedLevel = 0;
        mRenderedPassType = "";
        setXpRatio(0f);
    }

    private void render() {
        if (mPass == null) {
            return;
        }
        GeneratedPassData.PassDef def = GeneratedPassData.pass(mPass.type);
        int maxLevel = GeneratedPassData.PASS_MAX_LEVEL;
        int level = Math.max(1, Math.min(mPass.level, maxLevel));
        int levelXp = GeneratedPassData.xpForLevel(mPass.type, level);
        int xp = Math.max(0, mPass.xp);
        if (def == null) {
            Log.w(TAG, "unknown pass type " + mPass.type);
        }

        mPassName.setText(l10n(mPass.type, mPass.type).toUpperCase());
        mLevelLabel.setText("LEVEL " + level);
        mXpLabel.setText(xp + "/" + levelXp + " XP");
        setXpRatio(levelXp > 0 ? Math.min(1f, (float) xp / levelXp) : 0f);

        if (!mPass.type.equals(mRenderedPassType)) {
            mRenderedPassType = mPass.type;
            mRenderedLevel = 0;
        }

        String nextItem = GeneratedPassData.nextUnlockItem(mPass.type, level);
        if (level >= maxLevel) {
            mUnlockLabel.setText("Pass complete - max level reached.");
        } else if (!nextItem.isEmpty()) {
            mUnlockLabel.setText("Next reward (Lv " + (level + 1) + "): "
                    + l10n("game-" + nextItem, nextItem));
        } else {
            mUnlockLabel.setText("Next reward level: " + (level + 1));
        }
        updateUnlockArt(nextItem, level);

        renderQuests();
    }

    /**
     * pass.ts setPassUnlockImage: show the next unlock's generated art tile
     * (`img/loadout/<type>.png`, outfit tint via the shared multiply matrix),
     * its loadout category icon and title. A level increase between renders
     * plays the web's level-up pulse (the art pops in and the level label
     * scales).
     */
    private void updateUnlockArt(String type, int level) {
        GeneratedPassData.ItemDef item = type.isEmpty() ? null : GeneratedPassData.item(type);
        Bitmap art = item != null ? ItemArt.load(mActivity, "img/loadout/" + item.type + ".png", 2) : null;
        if (art != null) {
            mArtBox.setVisibility(View.VISIBLE);
            mUnlockImage.setImageBitmap(art);
            if ("outfit".equals(item.category)) {
                ItemArt.tint(mUnlockImage, item.tint);
            } else {
                mUnlockImage.clearColorFilter();
            }
            mUnlockImage.setVisibility(View.VISIBLE);

            Bitmap icon = ItemArt.load(mActivity, "gui/loadout-" + item.category + ".png", 4);
            mUnlockTypeIcon.setImageBitmap(icon);
            mUnlockTypeIcon.setVisibility(icon != null ? View.VISIBLE : View.GONE);

            mUnlockTitle.setText(l10n("loadout-title-" + item.category, item.category).toUpperCase());
            mUnlockTitle.setVisibility(View.VISIBLE);

            if (mRenderedLevel > 0 && level > mRenderedLevel) {
                animateUnlockPulse();
            }
        } else {
            // Keep the text label (it still names the next level); only the art
            // column disappears when the def/PNG is missing.
            mArtBox.setVisibility(View.GONE);
            mUnlockImage.setVisibility(View.GONE);
            mUnlockTypeIcon.setVisibility(View.GONE);
            mUnlockTitle.setVisibility(View.GONE);
        }
        mRenderedLevel = level;
    }

    private void animateUnlockPulse() {
        mUnlockImage.setAlpha(0f);
        mUnlockImage.setScaleX(0.7f);
        mUnlockImage.setScaleY(0.7f);
        mUnlockImage.animate().alpha(1f).scaleX(1f).scaleY(1f)
                .setDuration(420).setInterpolator(new OvershootInterpolator(1.4f)).start();

        mUnlockTypeIcon.setAlpha(0f);
        mUnlockTypeIcon.animate().alpha(1f).setDuration(420).start();

        mUnlockTitle.setAlpha(0f);
        mUnlockTitle.animate().alpha(1f).setDuration(420).start();

        mLevelLabel.setScaleX(1.15f);
        mLevelLabel.setScaleY(1.15f);
        mLevelLabel.animate().scaleX(1f).scaleY(1f).setDuration(300).start();
    }

    private void setXpRatio(float ratio) {
        ratio = Math.max(0f, Math.min(1f, ratio));
        // Two weighted segments (unfilled + filled) so the bar always spans the
        // full width: a single weighted fill inside a weighted track would
        // render ratio/(1+ratio) instead of ratio.
        mXpBarBg.setLayoutParams(new LinearLayout.LayoutParams(0, dp(14), 1f - ratio));
        mXpBarFill.setLayoutParams(new LinearLayout.LayoutParams(0, dp(14), ratio));
        mXpBarContainer.requestLayout();
    }

    private void renderQuests() {
        mQuestList.removeAllViews();
        mQuestRows.clear();

        if (mPass == null || mPass.quests.isEmpty()) {
            TextView none = new TextView(mActivity);
            none.setText("No quests available.");
            none.setTextColor(0xFF777777);
            none.setTextSize(TypedValue.COMPLEX_UNIT_SP, 11);
            mQuestList.addView(none);
            return;
        }

        for (int i = 0; i < mPass.quests.size(); i++) {
            AccountManager.QuestInfo quest = mPass.quests.get(i);
            GeneratedPassData.QuestDef def = GeneratedPassData.quest(quest.type);
            QuestRow row = new QuestRow();
            row.data = quest;
            row.expiresAt = System.currentTimeMillis() + quest.timeToRefresh;

            LinearLayout box = new LinearLayout(mActivity);
            box.setOrientation(LinearLayout.VERTICAL);
            GradientDrawable bg = new GradientDrawable();
            bg.setColor(0x22FFFFFF);
            bg.setCornerRadius(dp(4));
            box.setBackground(bg);
            box.setPadding(dp(8), dp(6), dp(8), dp(6));

            LinearLayout top = new LinearLayout(mActivity);
            top.setOrientation(LinearLayout.HORIZONTAL);
            top.setGravity(Gravity.CENTER_VERTICAL);

            TextView name = new TextView(mActivity);
            name.setText(l10n(quest.type, quest.type));
            name.setTextColor(quest.complete ? GREEN : Color.WHITE);
            name.setTextSize(TypedValue.COMPLEX_UNIT_SP, 13);
            name.setSingleLine(true);
            name.setLayoutParams(new LinearLayout.LayoutParams(
                    0, ViewGroup.LayoutParams.WRAP_CONTENT, 1f));
            top.addView(name);

            TextView xp = new TextView(mActivity);
            xp.setText("+" + (def != null ? def.xp : 0) + " XP");
            xp.setTextColor(GOLD);
            xp.setTextSize(TypedValue.COMPLEX_UNIT_SP, 11);
            top.addView(xp);
            box.addView(top);

            LinearLayout middle = new LinearLayout(mActivity);
            middle.setOrientation(LinearLayout.HORIZONTAL);
            middle.setGravity(Gravity.CENTER_VERTICAL);
            middle.setPadding(0, dp(4), 0, 0);

            int progress = Math.min(quest.progress, quest.target);
            LinearLayout barBg = new LinearLayout(mActivity);
            barBg.setBackgroundColor(BAR_BG);
            View barFill = new View(mActivity);
            barFill.setBackgroundColor(quest.complete ? GREEN : BLUE);
            float ratio = quest.target > 0 ? (float) progress / quest.target : 0f;
            barBg.addView(barFill, new LinearLayout.LayoutParams(
                    (int) (dp(130) * ratio), dp(10)));
            barBg.setLayoutParams(new LinearLayout.LayoutParams(dp(130), dp(10)));
            middle.addView(barBg);

            TextView counter = new TextView(mActivity);
            counter.setText(progressText(def, progress) + "/"
                    + progressText(def, quest.target));
            counter.setTextColor(0xFFAAAAAA);
            counter.setTextSize(TypedValue.COMPLEX_UNIT_SP, 11);
            counter.setPadding(dp(6), 0, 0, 0);
            middle.addView(counter);

            row.refresh = new TextView(mActivity);
            row.refresh.setText("Refresh");
            row.refresh.setTextColor(BLUE);
            row.refresh.setTextSize(TypedValue.COMPLEX_UNIT_SP, 11);
            row.refresh.setPadding(dp(10), dp(2), 0, dp(2));
            row.refresh.setClickable(true);
            row.refresh.setOnClickListener(new View.OnClickListener() {
                @Override public void onClick(View v) {
                    confirmRefresh(row.data);
                }
            });
            LinearLayout.LayoutParams refreshLp = new LinearLayout.LayoutParams(
                    0, ViewGroup.LayoutParams.WRAP_CONTENT, 1f);
            row.refresh.setLayoutParams(refreshLp);
            row.refresh.setGravity(Gravity.END);
            middle.addView(row.refresh);

            box.addView(middle);

            row.timer = new TextView(mActivity);
            row.timer.setTextSize(TypedValue.COMPLEX_UNIT_SP, 10);
            row.timer.setTextColor(0xFF888888);
            box.addView(row.timer);

            mQuestRows.add(row);
            mQuestList.addView(box, questBoxParams());
        }

        updateQuestTimers();
    }

    private LinearLayout.LayoutParams questBoxParams() {
        LinearLayout.LayoutParams lp = new LinearLayout.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT);
        lp.topMargin = dp(5);
        return lp;
    }

    private void updateQuestTimers() {
        long now = System.currentTimeMillis();
        for (int i = 0; i < mQuestRows.size(); i++) {
            QuestRow row = mQuestRows.get(i);
            boolean refreshable = (!row.data.rerolled && !row.data.complete)
                    || row.expiresAt - now <= 0;
            row.refresh.setVisibility(refreshable ? View.VISIBLE : View.GONE);

            String state;
            if (row.data.complete && row.data.rerolled) {
                state = "Rerolled - new quest next refresh.";
            } else if (row.data.complete) {
                long remaining = Math.max(0, row.expiresAt - now);
                state = remaining > 0
                        ? "Complete - refreshes in " + humanizeTime(remaining / 1000.0, false)
                        : "Complete - refresh available.";
            } else if (row.data.rerolled) {
                state = "Rerolled once this season.";
            } else {
                state = "";
            }
            row.timer.setText(state);
            row.timer.setVisibility(state.isEmpty() ? View.GONE : View.VISIBLE);
        }
    }

    private void confirmRefresh(final AccountManager.QuestInfo quest) {
        new AlertDialog.Builder(mActivity)
                .setTitle("Refresh quest")
                .setMessage("Replace this quest with a new one?")
                .setPositiveButton("Refresh", new DialogInterface.OnClickListener() {
                    @Override public void onClick(DialogInterface dialog, int which) {
                        mLoading = true;
                        mStatusText.setText("Refreshing quest...");
                        mAccount.refreshQuest(quest.idx, new AccountManager.SimpleCallback() {
                            @Override public void onResult(boolean success) {
                                mLoading = false;
                                mStatusText.setText(success
                                        ? ""
                                        : "That quest cannot be refreshed right now.");
                                loadPass();
                            }
                        });
                    }
                })
                .setNegativeButton("Cancel", null)
                .show();
    }

    private static String progressText(GeneratedPassData.QuestDef def, int value) {
        if (def != null && def.timed) {
            return humanizeTime(value, false);
        }
        return String.valueOf(value);
    }

    /** pass.ts humanizeTime (seconds). */
    private static String humanizeTime(double time, boolean minutesFloor) {
        long hours = (long) Math.floor(Math.ceil(time / 60.0) / 60.0);
        long minutes = minutesFloor
                ? (long) Math.floor(time / 60.0) % 60
                : (long) Math.ceil(time / 60.0) % 60;
        return (hours > 0 ? hours + "h " : "") + minutes + "m";
    }

    private String l10n(String key, String fallback) {
        String text = Json.peekString(SurvBridge.getL10nJson(), key);
        return text != null && !text.isEmpty() ? text : fallback;
    }

    private int dp(int value) {
        return (int) (value * mActivity.getResources().getDisplayMetrics().density + 0.5f);
    }

    private static final class QuestRow {
        AccountManager.QuestInfo data;
        long expiresAt;
        TextView refresh;
        TextView timer;
    }
}
