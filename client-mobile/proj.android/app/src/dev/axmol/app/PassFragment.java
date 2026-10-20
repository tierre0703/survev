/****************************************************************************
 * PassFragment.java — Survev Pass / Battle Pass screen.
 *
 * New screen matching web client's pass system:
 *   - Season info with timer countdown
 *   - Level progress bar with XP counter
 *   - Tier rewards list (free + premium tracks)
 *   - Quest tracker (daily/weekly/seasonal/challenge)
 *   - Premium upgrade purchase button
 *   - Animated completion effects
 *
 * Stub data structure - will connect to real API later.
 *
 * Threading: all SurvBridge calls on UI thread, network via SurvApi.
 ****************************************************************************/
package dev.axmol.app;

import android.app.Activity;
import android.app.AlertDialog;
import android.graphics.Color;
import android.graphics.drawable.GradientDrawable;
import android.os.Handler;
import android.os.Looper;
import android.util.Log;
import android.util.TypedValue;
import android.view.Gravity;
import android.view.View;
import android.view.ViewGroup;
import android.view.animation.AlphaAnimation;
import android.widget.Button;
import android.widget.FrameLayout;
import android.widget.LinearLayout;
import android.widget.ScrollView;
import android.widget.TextView;
import android.widget.Toast;

import java.util.ArrayList;
import java.util.List;

public class PassFragment {
    private static final String TAG = "PassFragment";

    // Colors matching web client pass styling
    private static final int PASS_PURPLE     = 0xFF9B59B6;
    private static final int PASS_PURPLE_DARK = 0xFF6E3D7F;
    private static final int GOLD            = 0xFFFFD700;
    private static final int GREEN           = 0xFF83AF50;
    private static final int BLUE            = 0xFF3498DB;
    private static final int XP_BLUE         = 0xFF2980B9;
    private static final int FREE_TRACK_BG   = 0xFF4A4A4A;
    private static final int PREM_TRACK_BG   = 0xFF6A3A6A;
    private static final int PANEL_BG        = 0xF01A1A2E;

    // Tier rarity colors for rewards
    private static final int TIER_COMMON     = 0xFFFFFFFF;
    private static final int TIER_RARE       = 0xFF0070DD;
    private static final int TIER_EPIC       = 0xFFA335EE;
    private static final int TIER_LEGENDARY  = 0xFFFF8000;

    private final Activity mActivity;
    private final FrameLayout mRoot;
    private boolean mOpen = false;

    // --- widgets ---
    private final TextView mSeasonLabel;
    private final TextView mSeasonTimer;
    private final TextView mLevelLabel;
    private final TextView mXpLabel;
    private final View mXpBarFill;
    private final ScrollView mTierScroll;
    private final LinearLayout mTierList;
    private final LinearLayout mQuestSection;
    private Button mBuyPremiumBtn = null;
    private final Button mCloseBtn;
    private final TextView mStatusText;

    // --- state ---
    private int mCurrentLevel = 1;
    private int mCurrentXp = 1250;
    private int mXpForNextLevel = 3000;
    private int mMaxLevel = 50;
    private boolean mHasPremium = false;
    private List<PassTier> mTiers = new ArrayList<>();
    private List<QuestEntry> mQuests = new ArrayList<>();
    private Handler mHandler = new Handler(Looper.getMainLooper());
    private Runnable mTimerRunnable;

    // Callback for close event
    public interface OnCloseListener {
        void onClose();
    }
    private OnCloseListener mOnCloseListener;

    public PassFragment(Activity activity) {
        mActivity = activity;

        mRoot = new FrameLayout(activity);
        mRoot.setBackgroundColor(0xCC000000);

        // Full-height ScrollView for landscape responsiveness
        ScrollView outerScroll = new ScrollView(activity);
        outerScroll.setLayoutParams(new FrameLayout.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.MATCH_PARENT));

        // Main panel - wider than other screens
        LinearLayout panel = new LinearLayout(activity);
        panel.setOrientation(LinearLayout.VERTICAL);
        panel.setGravity(Gravity.CENTER_HORIZONTAL);
        panel.setBackgroundColor(PANEL_BG);
        panel.setPadding(dp(16), dp(14), dp(16), dp(14));
        FrameLayout.LayoutParams panelParams = new FrameLayout.LayoutParams(
                dp(380), ViewGroup.LayoutParams.WRAP_CONTENT);
        panelParams.gravity = Gravity.CENTER;
        panel.setLayoutParams(panelParams);

        // ===== Header =====
        LinearLayout headerRow = new LinearLayout(activity);
        headerRow.setOrientation(LinearLayout.HORIZONTAL);
        headerRow.setGravity(Gravity.CENTER_VERTICAL);

        TextView titleIcon = new TextView(activity);
        titleIcon.setText("\uD83C\uDFC6");
        titleIcon.setTextSize(TypedValue.COMPLEX_UNIT_SP, 24);
        headerRow.addView(titleIcon);

        TextView title = new TextView(activity);
        title.setText(" SURVEV PASS");
        title.setTextColor(GOLD);
        title.setTextSize(TypedValue.COMPLEX_UNIT_SP, 20);
        title.setTypeface(android.graphics.Typeface.DEFAULT_BOLD);
        title.setLayoutParams(new LinearLayout.LayoutParams(0, ViewGroup.LayoutParams.WRAP_CONTENT, 1f));
        headerRow.addView(title);

        mCloseBtn = new Button(activity);
        mCloseBtn.setText("\u2715");
        mCloseBtn.setBackgroundColor(0xFF444444);
        mCloseBtn.setTextColor(Color.WHITE);
        mCloseBtn.setOnClickListener(new View.OnClickListener() {
            @Override public void onClick(View v) { 
                hide(); 
                if (mOnCloseListener != null) mOnCloseListener.onClose();
            }
        });
        headerRow.addView(mCloseBtn);

        panel.addView(headerRow);

        // ===== Season Info =====
        mSeasonLabel = new TextView(activity);
        mSeasonLabel.setText("Season 1: Drop Zone");
        mSeasonLabel.setTextColor(PASS_PURPLE);
        mSeasonLabel.setTextSize(TypedValue.COMPLEX_UNIT_SP, 14);
        mSeasonLabel.setGravity(Gravity.CENTER);
        panel.addView(mSeasonLabel);

        mSeasonTimer = new TextView(activity);
        mSeasonTimer.setText("Ends in: --d --h --m");
        mSeasonTimer.setTextColor(0xFFCCCCCC);
        mSeasonTimer.setTextSize(TypedValue.COMPLEX_UNIT_SP, 11);
        mSeasonTimer.setGravity(Gravity.CENTER);
        panel.addView(mSeasonTimer);

        addGap(panel, dp(8));

        // ===== Level & XP Progress =====
        LinearLayout levelRow = new LinearLayout(activity);
        levelRow.setOrientation(LinearLayout.HORIZONTAL);
        levelRow.setGravity(Gravity.CENTER_VERTICAL);

        mLevelLabel = new TextView(activity);
        mLevelLabel.setText("LEVEL " + mCurrentLevel);
        mLevelLabel.setTextColor(GOLD);
        mLevelLabel.setTextSize(TypedValue.COMPLEX_UNIT_SP, 18);
        mLevelLabel.setTypeface(android.graphics.Typeface.DEFAULT_BOLD);
        levelRow.addView(mLevelLabel);

        mXpLabel = new TextView(activity);
        mXpLabel.setText(mCurrentXp + "/" + mXpForNextLevel + " XP");
        mXpLabel.setTextColor(XP_BLUE);
        mXpLabel.setTextSize(TypedValue.COMPLEX_UNIT_SP, 12);
        mXpLabel.setLayoutParams(new LinearLayout.LayoutParams(0, ViewGroup.LayoutParams.WRAP_CONTENT, 1f));
        mXpLabel.setGravity(Gravity.END);
        levelRow.addView(mXpLabel);

        panel.addView(levelRow);

        // XP Bar
        LinearLayout xpBarContainer = new LinearLayout(activity);
        xpBarContainer.setOrientation(LinearLayout.HORIZONTAL);

        View xpBarBg = new View(activity);
        xpBarBg.setBackgroundColor(0xFF333333);
        xpBarBg.setLayoutParams(new LinearLayout.LayoutParams(0, dp(20), 1f));

        float xpRatio = (float) mCurrentXp / mXpForNextLevel;
        mXpBarFill = new View(activity);
        mXpBarFill.setBackgroundColor(XP_BLUE);
        mXpBarFill.setLayoutParams(new LinearLayout.LayoutParams(
                0, dp(20), xpRatio));

        xpBarContainer.addView(xpBarBg);
        xpBarContainer.addView(mXpBarFill);
        panel.addView(xpBarContainer);

        addGap(panel, dp(10));

        // ===== Tier Rewards (scrollable) =====
        TextView tierHeader = new TextView(activity);
        tierHeader.setText("\uD83C\uDF81 REWARDS");
        tierHeader.setTextColor(GOLD);
        tierHeader.setTextSize(TypedValue.COMPLEX_UNIT_SP, 14);
        tierHeader.setTypeface(android.graphics.Typeface.DEFAULT_BOLD);
        panel.addView(tierHeader);

        mTierScroll = new ScrollView(activity);
        mTierScroll.setLayoutParams(new FrameLayout.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT, dp(180)));

        mTierList = new LinearLayout(activity);
        mTierList.setOrientation(LinearLayout.VERTICAL);
        mTierScroll.addView(mTierList);
        panel.addView(mTierScroll);

        addGap(panel, dp(8));

        // ===== Quest Tracker =====
        mQuestSection = new LinearLayout(activity);
        mQuestSection.setOrientation(LinearLayout.VERTICAL);
        mQuestSection.setBackgroundColor(0x22FFFFFF);
        mQuestSection.setPadding(dp(8), dp(6), dp(8), dp(6));

        TextView questHeader = new TextView(activity);
        questHeader.setText("\uD83D\uDCDC QUESTS");
        questHeader.setTextColor(GOLD);
        questHeader.setTextSize(TypedValue.COMPLEX_UNIT_SP, 14);
        questHeader.setTypeface(android.graphics.Typeface.DEFAULT_BOLD);
        mQuestSection.addView(questHeader);

        // Quest categories
        addQuestCategory(mQuestSection, "Daily", 0xFF4CAF50);
        addQuestCategory(mQuestSection, "Weekly", 0xFF3498DB);
        addQuestCategory(mQuestSection, "Seasonal", 0xFF9B59B6);
        addQuestCategory(mQuestSection, "Challenges", 0xFFFF9800);

        panel.addView(mQuestSection);

        addGap(panel, dp(8));

        // ===== Premium Purchase =====
        if (!mHasPremium) {
            mBuyPremiumBtn = new Button(activity);
            mBuyPremiumBtn.setText("\uD83D\uDC51 UNLOCK PREMIUM PASS - $9.99");
            mBuyPremiumBtn.setBackgroundColor(PASS_PURPLE);
            mBuyPremiumBtn.setTextColor(Color.WHITE);
            mBuyPremiumBtn.setTextSize(TypedValue.COMPLEX_UNIT_SP, 14);
            mBuyPremiumBtn.setLayoutParams(new ViewGroup.LayoutParams(
                    ViewGroup.LayoutParams.MATCH_PARENT, dp(44)));
            mBuyPremiumBtn.setOnClickListener(new View.OnClickListener() {
                @Override public void onClick(View v) { purchasePremium(); }
            });
            panel.addView(mBuyPremiumBtn);
        }

        // Status
        mStatusText = new TextView(activity);
        mStatusText.setTextColor(0xFF808080);
        mStatusText.setTextSize(TypedValue.COMPLEX_UNIT_SP, 11);
        mStatusText.setGravity(Gravity.CENTER);
        panel.addView(mStatusText);

        outerScroll.addView(panel);

        mRoot.addView(outerScroll);

        // Initialize data
        initStubData();
        renderTiers();
        startSeasonTimer();
    }

    public void setOnClose(OnCloseListener listener) {
        mOnCloseListener = listener;
    }

    // ===== Data Initialization (stub) =====

    private void initStubData() {
        // Generate 50 tiers of rewards
        String[] freeRewards = {
            "100 Coins", "200 Coins", "500 Coins", "1000 Coins", "1500 Credits",
            "Bandages x5", "Soda x3", "Emote: GG", "Med Kit x2", "Player Icon: Basic",
            "2500 Coins", "Weapon Skin: Rusty", "3000 Coins", "Boost: Soda x5", "3500 Coins",
            "Emote: Dance", "4000 Coins", "Outfit: Camo", "4500 Coins", "Melee: Knife",
            "5000 Coins", "Emote: Heart", "5500 Credits", "6000 Coins", "Pickaxe: Stone",
            "6500 Coins", "Gl Trail: Smoke", "7000 Coins", "7500 Coins", "Outfit: Arctic",
            "8000 Coins", "Emote: Thumbs Up", "8500 Coins", "9000 Coins", "9500 Coins",
            "10000 Coins", "Legendary Crate", "11000 Coins", "12000 Coins", "13000 Coins",
            "Title: Survivor", "14000 Coins", "15000 Coins", "Mythic Token", "16000 Coins",
            "17000 Coins", "18000 Coins", "19000 Coins", "20000 Coins + Crown"
        };

        String[] premRewards = {
            "500 Coins", "1000 Credits", "1500 Coins", "Outfit: Gold", "2500 Credits",
            "Weapon Skin: Golden", "3500 Coins", "Emote: Legendary Dance", "4500 Credits",
            "Player Icon: Crown", "5500 Coins", "Outfit: Dragon", "6500 Credits",
            "Pickaxe: Laser", "7500 Coins", "Gl Trail: Rainbow", "8500 Coins",
            "Outfit: Shadow", "9500 Credits", "10500 Coins", "Melee: Katana",
            "11500 Coins", "Emote: Mythic Spin", "12500 Coins", "13500 Credits",
            "14500 Coins", "15500 Coins", "Outfit: Celestial", "16500 Coins",
            "17500 Credits", "18500 Coins", "19500 Coins", "20500 Credits",
            "21500 Coins", "22500 Credits", "23500 Credits", "24500 Coins",
            "25500 Coins", "26500 Credits", "27500 Credits", "28500 Credits",
            "29500 Credits", "30500 Credits", "31500 Credits", "32500 Credits",
            "33500 Credits", "34500 Credits", "35500 Credits", "36500 Coins + Title: Pass Master"
        };

        for (int i = 0; i < mMaxLevel; i++) {
            PassTier tier = new PassTier();
            tier.level = i + 1;
            tier.unlocked = i < mCurrentLevel;
            tier.freeReward = (i < freeRewards.length) ? freeRewards[i] : ((i + 1) * 500) + " Coins";
            tier.premReward = (i < premRewards.length) ? premRewards[i] : ((i + 1) * 750) + " Credits";
            tier.freeClaimed = i < mCurrentLevel - 1;
            tier.premClaimed = i < mCurrentLevel - 1 && mHasPremium;

            // Assign rarity based on tier milestones
            if ((i + 1) % 10 == 0) tier.rarity = 3; // Epic every 10 levels
            else if ((i + 1) % 5 == 0) tier.rarity = 2; // Rare every 5 levels
            else if (i == mMaxLevel - 1) tier.rarity = 4; // Legendary last tier
            else tier.rarity = (i % 3 == 0) ? 1 : 0; // Some common

            mTiers.add(tier);
        }

        // Stub quests
        mQuests.add(makeQuest("Deal 10,000 damage to enemies", 7500, 10000, "daily", 250));
        mQuests.add(makeQuest("Collect 50 items from loot crates", 30, 50, "daily", 150));
        mQuests.add(makeQuest("Play 5 matches", 2, 5, "weekly", 500));
        mQuests.add(makeQuest("Get 25 kills total", 15, 25, "weekly", 350));
        mQuests.add(makeQuest("Reach top 10 in 3 matches", 1, 3, "seasonal", 750));
        mQuests.add(makeQuest("Build 100 structures", 45, 100, "seasonal", 500));
        mQuests.add(makeQuest("Win a match without healing", 0, 1, "challenge", 1000));
        mQuests.add(makeQuest("Get a kill with every weapon type", 3, 8, "challenge", 1500));
    }

    private QuestEntry makeQuest(String name, int progress, int target, String category, int xpReward) {
        QuestEntry q = new QuestEntry();
        q.name = name;
        q.progress = progress;
        q.target = target;
        q.category = category;
        q.xpReward = xpReward;
        q.complete = progress >= target;
        return q;
    }

    // ===== Rendering =====

    private void renderTiers() {
        mTierList.removeAllViews();

        // Show tiers around current level (±7 levels)
        int startLevel = Math.max(0, mCurrentLevel - 3);
        int endLevel = Math.min(mTiers.size(), mCurrentLevel + 7);

        for (int i = startLevel; i < endLevel; i++) {
            PassTier tier = mTiers.get(i);
            renderTierRow(tier);
        }
    }

    private void renderTierRow(PassTier tier) {
        LinearLayout row = new LinearLayout(mActivity);
        row.setOrientation(LinearLayout.HORIZONTAL);
        row.setGravity(Gravity.CENTER_VERTICAL);
        row.setPadding(dp(4), dp(4), dp(4), dp(4));

        // Background based on unlock status
        GradientDrawable bg = new GradientDrawable();
        bg.setColor(tier.unlocked ? 0x33FFFFFF : 0x15FFFFFF);
        bg.setCornerRadius(dp(4));
        row.setBackground(bg);

        // Level number
        TextView lvlNum = new TextView(mActivity);
        lvlNum.setText(String.valueOf(tier.level));
        lvlNum.setTextSize(TypedValue.COMPLEX_UNIT_SP, 12);
        lvlNum.setTextColor(tier.unlocked ? GOLD : 0xFF666666);
        lvlNum.setGravity(Gravity.CENTER);
        lvlNum.setLayoutParams(new LinearLayout.LayoutParams(dp(28), ViewGroup.LayoutParams.WRAP_CONTENT));
        row.addView(lvlNum);

        // Free track reward
        LinearLayout freeBox = makeRewardBox(tier.freeReward, tier.rarity,
                tier.unlocked, tier.freeClaimed, FREE_TRACK_BG);
        row.addView(freeBox, new LinearLayout.LayoutParams(0, dp(40), 1f));

        // Premium track reward
        LinearLayout premBox = makeRewardBox(tier.premReward, Math.min(tier.rarity + 1, 4),
                tier.unlocked && mHasPremium, tier.premClaimed, PREM_TRACK_BG);
        row.addView(premBox, new LinearLayout.LayoutParams(0, dp(40), 1f));

        // Claim buttons
        if (tier.unlocked && !tier.freeClaimed) {
            Button claimFree = new Button(mActivity);
            claimFree.setText("FREE");
            claimFree.setBackgroundColor(GREEN);
            claimFree.setTextColor(Color.WHITE);
            claimFree.setTextSize(TypedValue.COMPLEX_UNIT_SP, 9);
            claimFree.setOnClickListener(new View.OnClickListener() {
                @Override public void onClick(View v) { claimReward(tier.level, false); }
            });
            row.addView(claimFree, new LinearLayout.LayoutParams(dp(48), dp(32)));
        } else if (tier.unlocked && mHasPremium && !tier.premClaimed) {
            Button claimPrem = new Button(mActivity);
            claimPrem.setText("CLAIM");
            claimPrem.setBackgroundColor(PASS_PURPLE);
            claimPrem.setTextColor(Color.WHITE);
            claimPrem.setTextSize(TypedValue.COMPLEX_UNIT_SP, 9);
            claimPrem.setOnClickListener(new View.OnClickListener() {
                @Override public void onClick(View v) { claimReward(tier.level, true); }
            });
            row.addView(claimPrem, new LinearLayout.LayoutParams(dp(56), dp(32)));
        } else {
            // Spacer
            View spacer = new View(mActivity);
            spacer.setLayoutParams(new ViewGroup.LayoutParams(dp(48), dp(32)));
            row.addView(spacer);
        }

        mTierList.addView(row);
    }

    private LinearLayout makeRewardBox(String reward, int rarity, boolean unlocked,
                                        boolean claimed, int bgColor) {
        LinearLayout box = new LinearLayout(mActivity);
        box.setOrientation(LinearLayout.VERTICAL);
        box.setGravity(Gravity.CENTER);
        box.setPadding(dp(4), dp(2), dp(4), dp(2));

        GradientDrawable boxBg = new GradientDrawable();
        boxBg.setColor(bgColor);
        boxBg.setCornerRadius(dp(4));
        if (claimed) {
            boxBg.setStroke(dp(2), GREEN);
        } else if (unlocked) {
            boxBg.setStroke(dp(1), getRarityColor(rarity));
        }
        box.setBackground(boxBg);

        if (claimed) {
            TextView check = new TextView(mActivity);
            check.setText("\u2705");
            check.setTextSize(TypedValue.COMPLEX_UNIT_SP, 14);
            box.addView(check);
        } else if (!unlocked) {
            TextView lock = new TextView(mActivity);
            lock.setText("\uD83D\uDD12");
            lock.setTextColor(0xFF666666);
            lock.setTextSize(TypedValue.COMPLEX_UNIT_SP, 12);
            box.addView(lock);
        } else {
            TextView rew = new TextView(mActivity);
            rew.setText(reward.length() > 20 ? reward.substring(0, 18) + ".." : reward);
            rew.setTextColor(getRarityColor(rarity));
            rew.setTextSize(TypedValue.COMPLEX_UNIT_SP, 10);
            rew.setGravity(Gravity.CENTER);
            box.addView(rew);
        }

        return box;
    }

    private void addQuestCategory(LinearLayout parent, String name, int color) {
        LinearLayout catContainer = new LinearLayout(mActivity);
        catContainer.setOrientation(LinearLayout.VERTICAL);
        catContainer.setPadding(0, dp(4), 0, dp(4));

        TextView catHeader = new TextView(mActivity);
        catHeader.setText(name.toUpperCase());
        catHeader.setTextColor(color);
        catHeader.setTextSize(TypedValue.COMPLEX_UNIT_SP, 12);
        catHeader.setTypeface(android.graphics.Typeface.DEFAULT_BOLD);
        catContainer.addView(catHeader);

        // Add quests for this category
        int count = 0;
        for (QuestEntry q : mQuests) {
            if (q.category.equalsIgnoreCase(name) && count < 2) {
                LinearLayout questRow = makeQuestRow(q);
                catContainer.addView(questRow);
                count++;
            }
        }

        if (count == 0) {
            TextView none = new TextView(mActivity);
            none.setText("No " + name.toLowerCase() + " quests available");
            none.setTextColor(0xFF666666);
            none.setTextSize(TypedValue.COMPLEX_UNIT_SP, 10);
            catContainer.addView(none);
        }

        parent.addView(catContainer);
    }

    private LinearLayout makeQuestRow(QuestEntry q) {
        LinearLayout row = new LinearLayout(mActivity);
        row.setOrientation(LinearLayout.HORIZONTAL);
        row.setGravity(Gravity.CENTER_VERTICAL);
        row.setPadding(dp(4), dp(2), dp(4), dp(2));

        // Quest name
        TextView name = new TextView(mActivity);
        name.setText(q.name);
        name.setTextColor(q.complete ? GREEN : Color.WHITE);
        name.setTextSize(TypedValue.COMPLEX_UNIT_SP, 11);
        name.setLayoutParams(new LinearLayout.LayoutParams(0, ViewGroup.LayoutParams.WRAP_CONTENT, 1f));
        name.setSingleLine(true);
        row.addView(name);

        // Progress bar background
        LinearLayout progBg = new LinearLayout(mActivity);
        progBg.setBackgroundColor(0xFF333333);

        // Progress fill
        float ratio = Math.min(1f, (float) q.progress / q.target);
        View progFill = new View(mActivity);
        progFill.setBackgroundColor(q.complete ? GREEN : BLUE);

        progBg.addView(progFill, new LinearLayout.LayoutParams(
                (int)(dp(60) * ratio), dp(12)));
        progBg.setLayoutParams(new LinearLayout.LayoutParams(dp(60), dp(12)));
        row.addView(progBg);

        // Progress text
        TextView progText = new TextView(mActivity);
        progText.setText(q.progress + "/" + q.target);
        progText.setTextColor(0xFFAAAAAA);
        progText.setTextSize(TypedValue.COMPLEX_UNIT_SP, 10);
        row.addView(progText);

        // XP reward
        TextView xp = new TextView(mActivity);
        xp.setText("+" + q.xpReward);
        xp.setTextColor(GOLD);
        xp.setTextSize(TypedValue.COMPLEX_UNIT_SP, 10);
        row.addView(xp);

        return row;
    }

    private int getRarityColor(int rarity) {
        switch (rarity) {
            case 0: return TIER_COMMON;
            case 1: return TIER_RARE;
            case 2: return TIER_EPIC;
            case 3: return TIER_LEGENDARY;
            default: return TIER_COMMON;
        }
    }

    // ===== Actions =====

    private void claimReward(int level, boolean premium) {
        // Animation flash
        AlphaAnimation flash = new AlphaAnimation(1.0f, 0.3f);
        flash.setDuration(150);
        flash.setRepeatMode(AlphaAnimation.REVERSE);
        flash.setRepeatCount(2);
        mTierList.startAnimation(flash);

        // Update state
        for (PassTier t : mTiers) {
            if (t.level == level) {
                if (premium) t.premClaimed = true;
                else t.freeClaimed = true;
                break;
            }
        }

        toast(premium ? "Premium reward claimed!" : "Free reward claimed!");
        renderTiers();
        SurvBridge.saveConfig();
    }

    private void purchasePremium() {
        // Stub: Would open Google Play billing or web payment flow
        new AlertDialog.Builder(mActivity)
                .setTitle("Unlock Premium Pass")
                .setMessage("Get access to the exclusive premium reward track including legendary skins, emotes, and more!\n\nPrice: $9.99")
                .setPositiveButton("Purchase", (dialog, which) -> {
                    mHasPremium = true;
                    if (mBuyPremiumBtn != null) mBuyPremiumBtn.setVisibility(View.GONE);
                    toast("Premium Pass unlocked! (demo)");
                    renderTiers();
                    SurvBridge.saveConfig();
                })
                .setNegativeButton("Cancel", null)
                .show();
    }

    // ===== Timer =====

    private void startSeasonTimer() {
        // Stub timer counting down (would use real API data)
        final long[] timeLeft = {7 * 24 * 60 * 60 * 1000L}; // 7 days in ms

        mTimerRunnable = new Runnable() {
            @Override
            public void run() {
                if (!mOpen) return;

                timeLeft[0] -= 1000;
                if (timeLeft[0] <= 0) {
                    mSeasonTimer.setText("Season ending soon!");
                    return;
                }

                long days = timeLeft[0] / (24 * 60 * 60 * 1000);
                long hours = (timeLeft[0] % (24 * 60 * 60 * 1000)) / (60 * 60 * 1000);
                long mins = (timeLeft[0] % (60 * 60 * 1000)) / (60 * 1000);

                mSeasonTimer.setText("Ends in: " + days + "d " + hours + "h " + mins + "m");

                mHandler.postDelayed(this, 1000);
            }
        };

        mHandler.post(mTimerRunnable);
    }

    private void stopSeasonTimer() {
        if (mTimerRunnable != null) {
            mHandler.removeCallbacks(mTimerRunnable);
            mTimerRunnable = null;
        }
    }

    // ===== Helpers =====

    private void addGap(LinearLayout parent, int height) {
        View gap = new View(mActivity);
        gap.setLayoutParams(new ViewGroup.LayoutParams(1, height));
        parent.addView(gap);
    }

    private void toast(String text) {
        Toast.makeText(mActivity, text, Toast.LENGTH_SHORT).show();
    }

    // ===== Public API =====

    public View root() { return mRoot; }

    public void show() {
        if (mOpen) return;
        mOpen = true;
        mRoot.setVisibility(View.VISIBLE);
        refresh();
        startSeasonTimer();
        Log.i(TAG, "pass show");
    }

    public void hide() {
        if (!mOpen) return;
        mOpen = false;
        mRoot.setVisibility(View.GONE);
        stopSeasonTimer();
        Log.i(TAG, "pass hide");
    }

    public boolean isOpen() { return mOpen; }

    public void refresh() {
        // Would fetch real data from API
        String config = SurvBridge.getConfigJson();
        String passLevel = Json.peekString(config, "passLevel");
        if (passLevel != null) {
            try {
                mCurrentLevel = Integer.parseInt(passLevel);
                mLevelLabel.setText("LEVEL " + mCurrentLevel);
            } catch (Exception e) {}
        }

        // Update XP bar
        float xpRatio = (float) mCurrentXp / mXpForNextLevel;
        mXpLabel.setText(mCurrentXp + "/" + mXpForNextLevel + " XP");

        // Animate XP bar change
        LinearLayout.LayoutParams fillParams = new LinearLayout.LayoutParams(
                0, dp(20), xpRatio);
        mXpBarFill.setLayoutParams(fillParams);

        renderTiers();
    }

    // ===== Types =====

    static class PassTier {
        int level;
        boolean unlocked;
        String freeReward;
        String premReward;
        boolean freeClaimed;
        boolean premClaimed;
        int rarity; // 0=common, 1=rare, 2=epic, 3=legendary
    }

    static class QuestEntry {
        String name;
        int progress;
        int target;
        String category;
        int xpReward;
        boolean complete;
    }

    private int dp(int value) {
        return (int) (value * mActivity.getResources().getDisplayMetrics().density + 0.5f);
    }
}
