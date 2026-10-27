/****************************************************************************
 * LoadoutFragment.java — S4 loadout editor (ARCHITECTURE.md §7, step S4).
 *
 * Enhanced Java loadout screen with:
 * - Category tabs (outfit, melee, emote, heal, boost, player_icon, crosshair)
 * - Rarity color coding (Stock → Mythic)
 * - Sort options (Newest, Alpha, Rarity)
 * - Item details panel with name, rarity, description
 * - Crosshair editor with color/size/stroke options
 * - Emote slot wheel visualization
 * - Paged item grid with selection highlighting
 *
 * Threading: all SurvBridge calls are synchronous on the UI thread.
 ****************************************************************************/
package dev.axmol.app;

import android.app.Activity;
import android.graphics.Bitmap;
import android.graphics.Color;
import android.graphics.ColorMatrix;
import android.graphics.ColorMatrixColorFilter;
import android.graphics.drawable.BitmapDrawable;
import android.graphics.drawable.Drawable;
import android.graphics.drawable.GradientDrawable;
import android.util.Log;
import android.util.TypedValue;
import android.view.Gravity;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.FrameLayout;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.ScrollView;
import android.widget.TextView;

import com.google.gson.JsonArray;
import com.google.gson.JsonElement;
import com.google.gson.JsonObject;
import com.google.gson.JsonParser;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.Comparator;
import java.util.List;

public class LoadoutFragment {
    private static final String TAG = "LoadoutFragment";

    // Rarity colors matching web client
    private static final int RARITY_STOCK     = 0xFFB0B0B0; // grey
    private static final int RARITY_COMMON    = 0xFFFFFFFF; // white
    private static final int RARITY_UNCOMMON  = 0xFF1EFF00; // green
    private static final int RARITY_RARE      = 0xFF0070DD; // blue
    private static final int RARITY_EPIC      = 0xFFA335EE; // purple
    private static final int RARITY_MYTHIC    = 0xFFFF8000; // orange

    private static final int SELECT_GREEN = 0xFF5A7A38;
    private static final int PANEL_BG    = 0xFF2A2A2A;
    private static final int GOLD        = 0xFFD4A843;

    // Customize list metrics from the web CSS (.customize-list-item 96px with a
    // 64px item image; selected has a 5px #00ff00 border). The Android panel is
    // narrower, so the tiles are 92dp and 3 per row.
    private static final int TILE_DP = 92;
    private static final int TILE_IMAGE_DP = 64;
    private static final int TILE_MARGIN_DP = 5;
    private static final int ITEM_PAGE_SIZE = 9; // 3 rows x 3 columns
    private static final String[] CATEGORY_ICONS = {
            "loadout-outfit", "loadout-melee", "loadout-emote", "loadout-heal",
            "loadout-boost", "loadout-player-icon", "loadout-crosshair",
    };

    // Decoded item art (Content/img/loadout/<type>.png) is shared across
    // refreshes by ItemArt's bitmap cache.

    private final Activity mActivity;
    private final FrameLayout mRoot;
    private final LinearLayout mCategoryRow;
    private final LinearLayout mGrid;
    private final TextView mSelectionLabel;
    private final TextView mDetailPanel;      // item details area
    private final Button mNewItemBtn;
    private final LinearLayout mEmoteSlotRow;
    private final Button[] mEmoteSlotButtons = new Button[6];
    private final Button mSortBtn;
    private final Button mPrevBtn;
    private final Button mNextBtn;
    private final Button mCloseBtn;
    
    // Crosshair editor widgets
    private final LinearLayout mCrosshairEditor;
    private final ImageView mCrosshairPreview;
    private final LinearLayout mColorPickerRow;
    private final Button[] mColorButtons;
    private String mCrosshairType = "crosshair_default";
    private final Button mSizeBtn;
    private final Button mStrokeBtn;

    // --- state ---
    private String[] mCategories = {"outfit", "melee", "emote", "heal", "boost", "player_icon", "crosshair"};
    private static final int EMOTE_SLOT_COUNT = 6;
    private static final String[] EMOTE_SLOT_LABELS = {"Top", "Right", "Bottom", "Left", "Win", "Death"};
    private int mSelectedCat = 0;
    private int mPage = 0;
    private int mEmoteSlot = 0;
    private boolean mOpen = false;
    private int mSortMode = 0; // 0=newest, 1=alpha, 2=rarity
    // Real shared/defs items (owned by this account), refreshed on each show.
    private final List<ItemEntry> mItems = new ArrayList<ItemEntry>();
    
    // Crosshair state
    private int mCrosshairColorIdx = 0; // default red
    private int mCrosshairSize = 1;     // 0=small, 1=medium, 2=large
    private int mCrosshairStroke = 1;   // 0=thin, 1=medium, 2=thick
    
    private static final String[] CROSSHAIR_COLORS = {"#FF0000", "#00FF00", "#0000FF", "#00FFFF", "#FF00FF", "#FFFF00", "#FFFFFF"};
    private static final String[] SIZE_LABELS = {"Small", "Medium", "Large"};
    private static final String[] STROKE_LABELS = {"Thin", "Medium", "Thick"};

    // Callback for when the loadout changes (C++ validator round-trip).
    private OnChangedListener mOnChanged;

    public interface OnChangedListener {
        void onChanged(String category, String type, int emoteSlot);
    }

    public LoadoutFragment(Activity activity) {
        mActivity = activity;

        mRoot = new FrameLayout(activity);
        mRoot.setBackgroundColor(0xE0000000);

        // Full-height ScrollView for landscape responsiveness
        ScrollView outerScroll = new ScrollView(activity);
        outerScroll.setLayoutParams(new FrameLayout.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.MATCH_PARENT));

        // Main panel (centered, like web #modal-customize).
        LinearLayout panel = new LinearLayout(activity);
        panel.setOrientation(LinearLayout.VERTICAL);
        panel.setGravity(Gravity.CENTER);
        panel.setLayoutParams(new FrameLayout.LayoutParams(
                dp(340), ViewGroup.LayoutParams.WRAP_CONTENT));
        panel.setBackgroundColor(PANEL_BG);
        panel.setPadding(dp(14), dp(14), dp(14), dp(14));

        // Title row with sort and close buttons
        LinearLayout titleRow = new LinearLayout(activity);
        titleRow.setOrientation(LinearLayout.HORIZONTAL);
        titleRow.setGravity(Gravity.CENTER_VERTICAL);
        
        TextView title = new TextView(activity);
        title.setText("\u2622 Customize");
        title.setTextColor(Color.WHITE);
        title.setTextSize(TypedValue.COMPLEX_UNIT_SP, 20);
        title.setGravity(Gravity.CENTER);
        title.setLayoutParams(new LinearLayout.LayoutParams(0, ViewGroup.LayoutParams.WRAP_CONTENT, 1f));
        titleRow.addView(title);

        mSortBtn = makeSmallButton("Sort: Newest");
        mSortBtn.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                cycleSort();
            }
        });
        titleRow.addView(mSortBtn);
        
        // Close button in title row (always visible at top)
        Button titleCloseBtn = new Button(activity);
        titleCloseBtn.setText("<");
        titleCloseBtn.setBackgroundColor(0xFF5A5A5A);
        titleCloseBtn.setTextColor(Color.WHITE);
        titleCloseBtn.setTextSize(TypedValue.COMPLEX_UNIT_SP, 16);
        LinearLayout.LayoutParams tclp = new LinearLayout.LayoutParams(dp(36), dp(36));
        tclp.leftMargin = dp(4);
        titleCloseBtn.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                hide();
            }
        });
        titleRow.addView(titleCloseBtn);
        
        panel.addView(titleRow);

        // Category row
        mCategoryRow = new LinearLayout(activity);
        mCategoryRow.setOrientation(LinearLayout.HORIZONTAL);
        mCategoryRow.setGravity(Gravity.CENTER);
        mCategoryRow.setPadding(0, dp(6), 0, dp(6));
        
        // Short labels for categories to fit
        String[] catLabels = {"Skin", "Melee", "Emoji", "Heal", "Boost", "Icon", "Scope"};
        for (int i = 0; i < mCategories.length; i++) {
            final int catIdx = i;
            Button catBtn = makeCatButton(catLabels[i], i == 0);
            Drawable catIcon = scaledDrawable(
                    loadAssetBitmap("gui/" + CATEGORY_ICONS[i] + ".png", 4), 16);
            if (catIcon != null) {
                catBtn.setCompoundDrawablesWithIntrinsicBounds(null, catIcon, null, null);
                catBtn.setCompoundDrawablePadding(dp(1));
            }
            catBtn.setOnClickListener(new View.OnClickListener() {
                @Override
                public void onClick(View v) {
                    selectCategory(catIdx);
                }
            });
            mCategoryRow.addView(catBtn);
        }
        panel.addView(mCategoryRow);

        // Selection label with rarity color
        mSelectionLabel = new TextView(activity);
        mSelectionLabel.setTextColor(GOLD);
        mSelectionLabel.setTextSize(TypedValue.COMPLEX_UNIT_SP, 14);
        mSelectionLabel.setPadding(0, 0, 0, dp(4));
        mSelectionLabel.setGravity(Gravity.CENTER);
        panel.addView(mSelectionLabel);

        // New-item confirmation banner (web modal-item-confirm reduced to the
        // touch screen): appears while account items have status < Confirmed.
        mNewItemBtn = makeSmallButton("New items unlocked - tap to confirm");
        mNewItemBtn.setBackgroundColor(GOLD);
        mNewItemBtn.setTextColor(Color.BLACK);
        mNewItemBtn.setVisibility(View.GONE);
        mNewItemBtn.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                confirmNewItems();
            }
        });
        panel.addView(mNewItemBtn);

        // Emote slot selector (visible only for emote category): the four wheel
        // wedges plus the win/death auto-emotes (EmoteSlot 0..5).
        mEmoteSlotRow = new LinearLayout(activity);
        mEmoteSlotRow.setOrientation(LinearLayout.HORIZONTAL);
        mEmoteSlotRow.setGravity(Gravity.CENTER);
        for (int i = 0; i < EMOTE_SLOT_COUNT; i++) {
            final int slot = i;
            Button slotBtn = makeSmallButton(EMOTE_SLOT_LABELS[i]);
            LinearLayout.LayoutParams slp = new LinearLayout.LayoutParams(
                    0, ViewGroup.LayoutParams.WRAP_CONTENT, 1f);
            slp.setMargins(dp(1), 0, dp(1), 0);
            slotBtn.setOnClickListener(new View.OnClickListener() {
                @Override
                public void onClick(View v) {
                    mEmoteSlot = slot;
                    refresh();
                }
            });
            mEmoteSlotButtons[i] = slotBtn;
            mEmoteSlotRow.addView(slotBtn, slp);
        }
        mEmoteSlotRow.setVisibility(View.GONE);
        panel.addView(mEmoteSlotRow);

        // === Crosshair Editor (visible only for crosshair category) ===
        mCrosshairEditor = new LinearLayout(activity);
        mCrosshairEditor.setOrientation(LinearLayout.VERTICAL);
        mCrosshairEditor.setPadding(dp(8), dp(8), dp(8), dp(8));
        mCrosshairEditor.setBackgroundColor(0xFF333333);
        mCrosshairEditor.setVisibility(View.GONE);
        
        // Preview area: the real crosshair sprite, tinted by the picked colour
        // (web #modal-content-right-crosshair / customize-crosshair-image).
        mCrosshairPreview = new ImageView(activity);
        mCrosshairPreview.setScaleType(ImageView.ScaleType.FIT_CENTER);
        mCrosshairPreview.setPadding(0, dp(12), 0, dp(12));
        LinearLayout.LayoutParams previewLp = new LinearLayout.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT, dp(128));
        mCrosshairEditor.addView(mCrosshairPreview, previewLp);
        
        // Color picker row
        TextView colorLbl = new TextView(activity);
        colorLbl.setText("Color:");
        colorLbl.setTextColor(Color.WHITE);
        colorLbl.setTextSize(TypedValue.COMPLEX_UNIT_SP, 13);
        mCrosshairEditor.addView(colorLbl);
        
        mColorPickerRow = new LinearLayout(activity);
        mColorPickerRow.setOrientation(LinearLayout.HORIZONTAL);
        mColorButtons = new Button[CROSSHAIR_COLORS.length];
        for (int i = 0; i < CROSSHAIR_COLORS.length; i++) {
            final int idx = i;
            mColorButtons[i] = new Button(activity);
            mColorButtons[i].setText("");
            GradientDrawable cd = new GradientDrawable();
            cd.setColor(Color.parseColor(CROSSHAIR_COLORS[i]));
            cd.setCornerRadius(dp(10));
            mColorButtons[i].setBackground(cd);
            LinearLayout.LayoutParams clp = new LinearLayout.LayoutParams(dp(32), dp(32));
            clp.rightMargin = dp(4);
            mColorButtons[i].setLayoutParams(clp);
            mColorButtons[i].setOnClickListener(new View.OnClickListener() {
                @Override
                public void onClick(View v) {
                    mCrosshairColorIdx = idx;
                    updateCrosshairPreview();
                    updateColorSelection();
                    saveCrosshairSettings();
                }
            });
            mColorPickerRow.addView(mColorButtons[i]);
        }
        mCrosshairEditor.addView(mColorPickerRow);
        
        // Size button
        LinearLayout sizeRow = new LinearLayout(activity);
        sizeRow.setOrientation(LinearLayout.HORIZONTAL);
        TextView sizeLbl = new TextView(activity);
        sizeLbl.setText("Size:");
        sizeLbl.setTextColor(Color.WHITE);
        sizeLbl.setTextSize(TypedValue.COMPLEX_UNIT_SP, 13);
        sizeRow.addView(sizeLbl);
        mSizeBtn = makeSmallButton("Medium");
        mSizeBtn.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                mCrosshairSize = (mCrosshairSize + 1) % 3;
                mSizeBtn.setText(SIZE_LABELS[mCrosshairSize]);
                updateCrosshairPreview();
                saveCrosshairSettings();
            }
        });
        sizeRow.addView(mSizeBtn);
        mCrosshairEditor.addView(sizeRow);
        
        // Stroke button
        LinearLayout strokeRow = new LinearLayout(activity);
        strokeRow.setOrientation(LinearLayout.HORIZONTAL);
        TextView strokeLbl = new TextView(activity);
        strokeLbl.setText("Stroke:");
        strokeLbl.setTextColor(Color.WHITE);
        strokeLbl.setTextSize(TypedValue.COMPLEX_UNIT_SP, 13);
        strokeRow.addView(strokeLbl);
        mStrokeBtn = makeSmallButton("Medium");
        mStrokeBtn.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                mCrosshairStroke = (mCrosshairStroke + 1) % 3;
                mStrokeBtn.setText(STROKE_LABELS[mCrosshairStroke]);
                updateCrosshairPreview();
                saveCrosshairSettings();
            }
        });
        strokeRow.addView(mStrokeBtn);
        mCrosshairEditor.addView(strokeRow);
        
        panel.addView(mCrosshairEditor);

        // Item grid (scrollable)
        ScrollView scroll = new ScrollView(activity);
        scroll.setLayoutParams(new FrameLayout.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT, dp(280)));
        mGrid = new LinearLayout(activity);
        mGrid.setOrientation(LinearLayout.VERTICAL);
        scroll.addView(mGrid);
        panel.addView(scroll);

        // Detail panel (shows selected item info)
        mDetailPanel = new TextView(activity);
        mDetailPanel.setTextColor(0xFFDDDDDD);
        mDetailPanel.setTextSize(TypedValue.COMPLEX_UNIT_SP, 12);
        mDetailPanel.setPadding(dp(8), dp(4), dp(8), dp(4));
        mDetailPanel.setBackgroundColor(0xFF252525);
        mDetailPanel.setVisibility(View.GONE);
        panel.addView(mDetailPanel);

        // Paging controls + Close
        LinearLayout bottomRow = new LinearLayout(activity);
        bottomRow.setOrientation(LinearLayout.HORIZONTAL);
        bottomRow.setGravity(Gravity.CENTER);

        mPrevBtn = new Button(activity);
        mPrevBtn.setText("<");
        mPrevBtn.setBackgroundColor(0xFF3A3A3A);
        mPrevBtn.setTextColor(Color.WHITE);
        mPrevBtn.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                if (mPage > 0) { mPage--; refresh(); }
            }
        });
        bottomRow.addView(mPrevBtn);

        // Page indicator
        TextView pageIndicator = new TextView(activity);
        pageIndicator.setText("1/1");
        pageIndicator.setId(View.generateViewId());
        pageIndicator.setTextColor(0xFFAAAAAA);
        pageIndicator.setTextSize(TypedValue.COMPLEX_UNIT_SP, 12);
        pageIndicator.setGravity(Gravity.CENTER);
        pageIndicator.setPadding(dp(16), 0, dp(16), 0);
        bottomRow.addView(pageIndicator);

        mNextBtn = new Button(activity);
        mNextBtn.setText(">");
        mNextBtn.setBackgroundColor(0xFF3A3A3A);
        mNextBtn.setTextColor(Color.WHITE);
        mNextBtn.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                mPage++;
                refresh();
            }
        });
        bottomRow.addView(mNextBtn);

        mCloseBtn = new Button(activity);
        mCloseBtn.setText("Back \u2715");
        mCloseBtn.setBackgroundColor(0xFF5A5A5A);
        mCloseBtn.setTextColor(Color.WHITE);
        mCloseBtn.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                hide();
            }
        });
        bottomRow.addView(mCloseBtn);

        panel.addView(bottomRow);

        outerScroll.addView(panel);

        mRoot.addView(outerScroll);
        
        // Initialize color selection
        updateColorSelection();

        // The profile can land after the screen is first built (device login is
        // asynchronous); refresh the owned list when it arrives.
        AccountManager account = AccountManager.peek();
        if (account != null) {
            account.addListener(new AccountManager.Listener() {
                @Override
                public void onAccountChanged(AccountManager.Profile profile) {
                    if (mOpen) refresh();
                }

                @Override
                public void onAccountError(String error) {
                }
            });
        }
    }

    public void setOnChanged(OnChangedListener listener) {
        mOnChanged = listener;
    }

    public View root() { return mRoot; }

    public void show() {
        if (mOpen) return;
        mOpen = true;
        mRoot.setVisibility(View.VISIBLE);
        // Re-sync the owned account items before the first read: the profile
        // fetch can finish before installUiBridge installs the C++ bridge, so
        // AccountManager's early setAccountItems call may have been dropped.
        AccountManager account = AccountManager.peek();
        if (account != null && account.isLoggedIn()) {
            SurvBridge.setAccountItems(account.getItemTypesJson());
        }
        refresh();
        Log.i(TAG, "loadout show");
    }

    public void hide() {
        if (!mOpen) return;
        mOpen = false;
        mRoot.setVisibility(View.GONE);
        // Persist the (possibly changed) selection to the account once, on
        // close: the crosshair buttons fire per tap and the account endpoint is
        // rate limited (web account.setLoadout is debounced by its UI too).
        AccountManager account = AccountManager.peek();
        if (account != null) {
            account.pushLoadout(SurvBridge.getLoadoutJson());
        }
        Log.i(TAG, "loadout hide");
    }

    public boolean isOpen() { return mOpen; }

    public void refresh() {
        String loadoutJson = SurvBridge.getLoadoutJson();
        JsonObject loadout = parseObject(loadoutJson);

        // Determine current selection and available items for this category.
        String cat = mCategories[mSelectedCat];
        String selectedType = "";

        // Show/hide appropriate panels based on category. The item grid stays
        // visible for crosshair too: it is the crosshair type picker.
        boolean isCrosshair = "crosshair".equals(cat);
        boolean isEmote = "emote".equals(cat);
        mCrosshairEditor.setVisibility(isCrosshair ? View.VISIBLE : View.GONE);
        mGrid.setVisibility(View.VISIBLE);
        mEmoteSlotRow.setVisibility(isEmote ? View.VISIBLE : View.GONE);

        // Owned item list + metadata from shared/defs (show() re-synced the
        // account items first).
        loadItems();
        refreshItemStatus();

        if (isCrosshair) {
            // Update crosshair editor state from the validated loadout
            // ({"crosshair":{"type":...,"color":0xRRGGBB,"size":"1.00","stroke":"0.00"}}).
            JsonObject crosshair = loadout != null && loadout.has("crosshair")
                    && loadout.get("crosshair").isJsonObject()
                    ? loadout.getAsJsonObject("crosshair") : null;
            String chColor = null;
            double chSize = 1.0;
            double chStroke = 0.0;
            if (crosshair != null) {
                selectedType = jsonString(crosshair, "type");
                JsonElement color = crosshair.get("color");
                if (color != null && color.isJsonPrimitive()) {
                    try {
                        chColor = String.format("#%06X",
                                color.getAsInt() & 0xFFFFFF);
                    } catch (Exception ignored) {
                        chColor = "#FFFFFF";
                    }
                }
                if (crosshair.has("size")) {
                    try { chSize = crosshair.get("size").getAsDouble(); } catch (Exception ignored) {}
                }
                if (crosshair.has("stroke")) {
                    try { chStroke = crosshair.get("stroke").getAsDouble(); } catch (Exception ignored) {}
                }
            }
            if (chColor != null) {
                for (int i = 0; i < CROSSHAIR_COLORS.length; i++) {
                    if (CROSSHAIR_COLORS[i].equalsIgnoreCase(chColor)) {
                        mCrosshairColorIdx = i;
                        break;
                    }
                }
            }
            double[] sizes = {0.7, 1.0, 1.4};
            double[] strokes = {0.0, 1.0, 2.0};
            mCrosshairSize = 1;
            for (int i = 0; i < sizes.length; i++) {
                if (Math.abs(chSize - sizes[i]) < 0.2) mCrosshairSize = i;
            }
            mCrosshairStroke = 1;
            for (int i = 0; i < strokes.length; i++) {
                if (Math.abs(chStroke - strokes[i]) < 0.5) mCrosshairStroke = i;
            }

            mSizeBtn.setText(SIZE_LABELS[mCrosshairSize]);
            mStrokeBtn.setText(STROKE_LABELS[mCrosshairStroke]);
            if (!selectedType.isEmpty()) mCrosshairType = selectedType;
            updateCrosshairPreview();
            updateColorSelection();
        } else if ("emote".equals(cat)) {
            List<String> emotes = parseArray(loadoutJson, "emotes");
            while (emotes.size() < EMOTE_SLOT_COUNT) emotes.add("");
            if (mEmoteSlot < 0 || mEmoteSlot >= EMOTE_SLOT_COUNT) mEmoteSlot = 0;
            selectedType = emotes.get(mEmoteSlot);
            for (int i = 0; i < EMOTE_SLOT_COUNT; i++) {
                mEmoteSlotButtons[i].setBackgroundColor(
                        i == mEmoteSlot ? SELECT_GREEN : 0xFF3A3A3A);
                Drawable icon = scaledDrawable(loadItemBitmap(emotes.get(i)), 22);
                mEmoteSlotButtons[i].setCompoundDrawablesWithIntrinsicBounds(null, icon, null, null);
                mEmoteSlotButtons[i].setCompoundDrawablePadding(dp(2));
            }
        } else {
            if (loadout != null) {
                selectedType = jsonString(loadout, cat);
            }
        }

        ItemEntry selected = findItem(selectedType);
        String display = selected != null && !selected.displayName.isEmpty()
                ? selected.displayName
                : (selectedType == null || selectedType.isEmpty()
                        ? ("player_icon".equals(cat) ? "None" : "Stock")
                        : localizeName(selectedType));
        int rarityColor = selected != null ? selected.rarityColor : RARITY_STOCK;
        String prefix = isEmote
                ? "Emote (" + EMOTE_SLOT_LABELS[mEmoteSlot] + ") "
                : catLabel(cat) + ": ";
        mSelectionLabel.setText(prefix + display);
        mSelectionLabel.setTextColor(rarityColor != RARITY_STOCK ? rarityColor : GOLD);

        refreshGrid(cat, selectedType);
    }

    // --- New-item confirmation (web loadoutMenu onItems/tryBeginConfirming) ---

    private String itemCategory(String tab) {
        return "player_icon".equals(tab) ? "emote" : tab;
    }

    private void refreshItemStatus() {
        AccountManager account = AccountManager.peek();
        if (account == null || !account.isLoggedIn()) {
            mNewItemBtn.setVisibility(View.GONE);
            return;
        }
        List<String> pending = new ArrayList<String>();
        StringBuilder names = new StringBuilder();
        for (AccountManager.ItemInfo info : account.getItems()) {
            if (info.status >= 1 || pending.contains(info.type)) continue;
            pending.add(info.type);
            ItemEntry entry = findItem(info.type);
            if (names.length() > 0) names.append(", ");
            names.append(entry != null ? entry.displayName : info.type);
        }
        if (!pending.isEmpty()) {
            mNewItemBtn.setText("New: " + names + " - tap to confirm");
            mNewItemBtn.setVisibility(View.VISIBLE);
        } else {
            mNewItemBtn.setVisibility(View.GONE);
        }
        // Opening a category acks its items (web setItemsAckd); the local copy is
        // updated by setItemStatus so this fires once per item.
        String wanted = itemCategory(mCategories[mSelectedCat]);
        List<String> ack = new ArrayList<String>();
        for (AccountManager.ItemInfo info : account.getItems()) {
            if (info.status >= 2) continue;
            ItemEntry entry = findItem(info.type);
            if (entry != null && wanted.equals(entry.category)) {
                ack.add(info.type);
            }
        }
        if (!ack.isEmpty()) account.setItemStatus(2, ack);
    }

    private void confirmNewItems() {
        AccountManager account = AccountManager.peek();
        if (account == null || !account.isLoggedIn()) {
            mNewItemBtn.setVisibility(View.GONE);
            return;
        }
        List<String> types = new ArrayList<String>();
        for (AccountManager.ItemInfo info : account.getItems()) {
            if (info.status < 1 && !types.contains(info.type)) types.add(info.type);
        }
        if (!types.isEmpty()) account.setItemStatus(1, types);
        mNewItemBtn.setVisibility(View.GONE);
        Log.i(TAG, "confirmed " + types.size() + " new item(s)");
    }

    // --- Item art (Content/img/loadout/<type>.png, generated from the web SVGs) ---

    private Bitmap loadAssetBitmap(String path) {
        return ItemArt.load(mActivity, path);
    }

    private Bitmap loadAssetBitmap(String path, int sampleSize) {
        return ItemArt.load(mActivity, path, sampleSize);
    }

    private Bitmap loadItemBitmap(String type) {
        return ItemArt.loadItem(mActivity, type);
    }

    // Web outfit art is a white silhouette with an SVG multiply filter
    // (helpers.getSvgFilterForTint); the same matrix works on the PNG.
    private void tintImage(ImageView view, int tint) {
        ItemArt.tint(view, tint);
    }

    private Drawable scaledDrawable(Bitmap bitmap, int sizeDp) {
        if (bitmap == null) return null;
        int px = dp(sizeDp);
        Bitmap scaled = Bitmap.createScaledBitmap(bitmap, px, px, true);
        BitmapDrawable drawable = new BitmapDrawable(mActivity.getResources(), scaled);
        drawable.setBounds(0, 0, px, px);
        return drawable;
    }

    // One square art tile from the web customize list (web: .customize-list-item
    // with a 64px item image and a 5px green selected border).
    private View makeItemTile(final ItemEntry entry, boolean selected) {
        FrameLayout tile = new FrameLayout(mActivity);
        LinearLayout.LayoutParams lp = new LinearLayout.LayoutParams(dp(TILE_DP), dp(TILE_DP));
        lp.setMargins(dp(TILE_MARGIN_DP), dp(TILE_MARGIN_DP), dp(TILE_MARGIN_DP), 0);
        tile.setLayoutParams(lp);

        GradientDrawable background = new GradientDrawable();
        background.setColor(0x66000000);
        background.setStroke(dp(selected ? 4 : 1), selected ? 0xFF00FF00 : 0xFF3A3A3A);
        tile.setBackground(background);
        tile.setAlpha(selected ? 1.0f : 0.8f);
        tile.setContentDescription(entry.displayName);

        Bitmap bitmap = loadItemBitmap(entry.type);
        if (bitmap != null) {
            ImageView image = new ImageView(mActivity);
            FrameLayout.LayoutParams ilp = new FrameLayout.LayoutParams(
                    dp(TILE_IMAGE_DP), dp(TILE_IMAGE_DP), Gravity.CENTER);
            image.setLayoutParams(ilp);
            image.setScaleType(ImageView.ScaleType.FIT_CENTER);
            image.setImageBitmap(bitmap);
            if ("outfit".equals(entry.category)) {
                tintImage(image, entry.tint);
            }
            tile.addView(image);
        } else {
            TextView fallback = new TextView(mActivity);
            fallback.setText(entry.displayName);
            fallback.setTextColor(entry.rarityColor);
            fallback.setTextSize(TypedValue.COMPLEX_UNIT_SP, 10);
            fallback.setGravity(Gravity.CENTER);
            fallback.setPadding(dp(4), dp(4), dp(4), dp(4));
            tile.addView(fallback, new FrameLayout.LayoutParams(
                    ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.MATCH_PARENT));
        }

        final String cat = mCategories[mSelectedCat];
        final int slot = mEmoteSlot;
        // Newly acquired item (account status == New): small gold NEW tag.
        if (entry.isNew) {
            TextView badge = new TextView(mActivity);
            badge.setText("NEW");
            badge.setTextColor(Color.BLACK);
            badge.setTextSize(TypedValue.COMPLEX_UNIT_SP, 8);
            badge.setTypeface(null, android.graphics.Typeface.BOLD);
            badge.setGravity(Gravity.CENTER);
            GradientDrawable badgeBg = new GradientDrawable();
            badgeBg.setColor(GOLD);
            badgeBg.setCornerRadius(dp(3));
            badge.setBackground(badgeBg);
            FrameLayout.LayoutParams blp = new FrameLayout.LayoutParams(
                    ViewGroup.LayoutParams.WRAP_CONTENT, dp(14), Gravity.TOP | Gravity.RIGHT);
            blp.topMargin = dp(2);
            blp.rightMargin = dp(2);
            badge.setPadding(dp(3), 0, dp(3), 0);
            tile.addView(badge, blp);
        }
        tile.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                selectItem(cat, entry.type, slot);
                showItemDetail(entry);
            }
        });
        return tile;
    }

    private void refreshGrid(String category, String selectedType) {
        mGrid.removeAllViews();

        List<ItemEntry> items = getCategoryItems(category);
        
        // Apply sorting
        sortItems(items, mSortMode);

        // Paginate
        final int pageSize = ITEM_PAGE_SIZE;
        int pages = (items.size() + pageSize - 1) / pageSize;
        if (pages < 1) pages = 1;
        if (mPage >= pages) mPage = pages - 1;
        mPrevBtn.setEnabled(mPage > 0);
        mNextBtn.setEnabled(mPage + 1 < pages);

        // Build grid rows (3 per row, matching the web's 96px tiles in the
        // mobile modal width).
        int start = mPage * pageSize;
        int end = Math.min(start + pageSize, items.size());

        LinearLayout currentRow = null;
        for (int i = start; i < end; i++) {
            int col = (i - start) % 3;
            if (col == 0) {
                currentRow = new LinearLayout(mActivity);
                currentRow.setOrientation(LinearLayout.HORIZONTAL);
                currentRow.setGravity(Gravity.CENTER);
                mGrid.addView(currentRow);
            }

            final ItemEntry entry = items.get(i);
            boolean isSelected = entry.type.equals(selectedType);
            if (currentRow != null) {
                currentRow.addView(makeItemTile(entry, isSelected));
            }
        }
        
        // Update detail panel for selected item
        if (selectedType != null && !selectedType.isEmpty()) {
            ItemEntry selEntry = findItem(items, selectedType);
            if (selEntry != null) {
                showItemDetail(selEntry);
            }
        } else {
            mDetailPanel.setVisibility(View.GONE);
        }
    }
    
    private ItemEntry findItem(List<ItemEntry> items, String type) {
        for (ItemEntry e : items) {
            if (e.type.equals(type)) return e;
        }
        return null;
    }
    
    private void showItemDetail(ItemEntry entry) {
        if (entry == null) {
            mDetailPanel.setVisibility(View.GONE);
            return;
        }
        StringBuilder sb = new StringBuilder();
        sb.append(getRarityName(entry.rarity)).append(" | ");
        sb.append(entry.displayName).append("\n");
        if (entry.description != null && !entry.description.isEmpty()) {
            sb.append(entry.description).append("\n");
        }
        String source = entry.source == null || entry.source.isEmpty() ? "Unknown" : entry.source;
        String sourceLabel = L10n.t("loadout-" + source, source);
        if (sourceLabel == null || sourceLabel.isEmpty()) {
            sourceLabel = source;
        }
        sb.append(L10n.t("loadout-acquired", "Acquired")).append(": ").append(sourceLabel);
        mDetailPanel.setText(sb.toString());
        mDetailPanel.setTextColor(entry.rarityColor);
        mDetailPanel.setVisibility(View.VISIBLE);
    }

    // --- Sorting ---
    
    private void cycleSort() {
        mSortMode = (mSortMode + 1) % 3;
        String[] labels = {"Sort: Newest", "Sort: A-Z", "Sort: Rarity"};
        mSortBtn.setText(labels[mSortMode]);
        refresh();
    }
    
    private void sortItems(List<ItemEntry> items, int mode) {
        if (mode == 1) {
            // Alphabetical
            items.sort(new Comparator<ItemEntry>() {
                @Override
                public int compare(ItemEntry a, ItemEntry b) {
                    return a.displayName.compareToIgnoreCase(b.displayName);
                }
            });
        } else if (mode == 2) {
            // By rarity (highest first)
            items.sort(new Comparator<ItemEntry>() {
                @Override
                public int compare(ItemEntry a, ItemEntry b) {
                    return Integer.compare(b.rarity, a.rarity);
                }
            });
        } else {
            // Newest first (web `loadout-newest`; timeAcquired comes from the
            // account item list, 0 for catalog-only items).
            items.sort(new Comparator<ItemEntry>() {
                @Override
                public int compare(ItemEntry a, ItemEntry b) {
                    return Long.compare(b.timeAcquired, a.timeAcquired);
                }
            });
        }
    }

    // --- Category/Item selection ---

    private void selectCategory(int idx) {
        mSelectedCat = idx;
        mPage = 0;
        mEmoteSlot = 0;
        refresh();
        // Update category button highlights
        for (int i = 0; i < mCategoryRow.getChildCount(); i++) {
            View child = mCategoryRow.getChildAt(i);
            if (child instanceof Button) {
                Button btn = (Button) child;
                btn.setBackgroundColor(i == idx ? SELECT_GREEN : 0xFF3A3A3A);
            }
        }
    }

    private void selectItem(String category, String type, int emoteSlot) {
        String json;
        if ("emote".equals(category)) {
            // Merge into the 6-slot array (4 wheel + win/death); the old code
            // replaced the array with one entry, wiping the other slots.
            List<String> emotes = parseArray(SurvBridge.getLoadoutJson(), "emotes");
            while (emotes.size() < EMOTE_SLOT_COUNT) emotes.add("");
            int slot = Math.max(0, Math.min(emoteSlot, EMOTE_SLOT_COUNT - 1));
            emotes.set(slot, type);
            StringBuilder arr = new StringBuilder("[");
            for (int i = 0; i < EMOTE_SLOT_COUNT; i++) {
                if (i > 0) arr.append(',');
                arr.append('"').append(escapeJson(emotes.get(i))).append('"');
            }
            arr.append(']');
            json = "{\"emotes\":" + arr + "}";
        } else if ("crosshair".equals(category)) {
            // Nested loadout.ts shape; a flat {"crosshair":"type"} is ignored
            // by the validator.
            json = "{\"crosshair\":{\"type\":\"" + escapeJson(type) + "\"}}";
        } else {
            json = "{\"" + category + "\":\"" + escapeJson(type) + "\"}";
        }

        boolean accepted = SurvBridge.setLoadout(json);
        Log.i(TAG, "select " + category + "=" + type + " slot=" + emoteSlot
                + " accepted=" + accepted);

        if (mOnChanged != null) {
            mOnChanged.onChanged(category, type, emoteSlot);
        }

        refresh();
    }
    
    // --- Crosshair helpers ---
    
    private void updateCrosshairPreview() {
        // Bake stroke buckets are generated for the in-game sprite; reuse the
        // same art here so the preview matches the selected stroke.
        Bitmap bitmap = loadAssetBitmap(
                "gui/crosshair/" + mCrosshairType + "_s" + mCrosshairStroke + ".png", 2);
        if (bitmap == null) {
            bitmap = loadItemBitmap(mCrosshairType);
        }
        mCrosshairPreview.setImageBitmap(bitmap);
        int color = Color.parseColor(CROSSHAIR_COLORS[mCrosshairColorIdx]);
        if (bitmap != null) {
            ColorMatrix matrix = new ColorMatrix(new float[] {
                    Color.red(color) / 255f, 0, 0, 0, 0,
                    0, Color.green(color) / 255f, 0, 0, 0,
                    0, 0, Color.blue(color) / 255f, 0, 0,
                    0, 0, 0, 1, 0,
            });
            mCrosshairPreview.setColorFilter(new ColorMatrixColorFilter(matrix));
        } else {
            mCrosshairPreview.clearColorFilter();
        }
        float[] sizeMult = {0.7f, 1.0f, 1.4f};
        float scale = sizeMult[mCrosshairSize];
        mCrosshairPreview.setScaleX(scale);
        mCrosshairPreview.setScaleY(scale);
    }
    
    private void updateColorSelection() {
        for (int i = 0; i < mColorButtons.length; i++) {
            // Add border effect for selected
            GradientDrawable bg = new GradientDrawable();
            bg.setColor(Color.parseColor(CROSSHAIR_COLORS[i]));
            bg.setCornerRadius(dp(10));
            if (i == mCrosshairColorIdx) {
                bg.setStroke(dp(3), Color.WHITE);
            } else {
                bg.setStroke(dp(1), 0xFF666666);
            }
            mColorButtons[i].setBackground(bg);
        }
    }
    
    private void saveCrosshairSettings() {
        // Send only the crosshair object; the C++ bridge merges it into the
        // current loadout (outfit/emotes/etc. must not be replaced) and maps
        // the editor's 0..2 indices to the web's float size/stroke.
        float sizeValues[] = {0.7f, 1.0f, 1.4f};
        float strokeValues[] = {0.0f, 1.0f, 2.0f};
        StringBuilder json = new StringBuilder();
        json.append("{\"crosshair\":{\"color\":\"").append(CROSSHAIR_COLORS[mCrosshairColorIdx]).append("\"");
        json.append(",\"size\":").append(sizeValues[mCrosshairSize]);
        json.append(",\"stroke\":").append(strokeValues[mCrosshairStroke]);
        json.append("}}");
        SurvBridge.setLoadout(json.toString());
    }

    // --- Item data (owned shared/defs items, via SurvBridge.loadoutItemsJson) ---

    static class ItemEntry {
        String type;
        String category;
        String displayName;
        int rarity; // 0-5 stock->mythic
        int rarityColor;
        String description;
        String sprite;
        int tint = 0xFFFFFF;
        String source = "";
        long timeAcquired = 0L;
        boolean isNew = false;
    }

    // Reads the C++ item snapshot: [{"type","category","name","lore","sprite",
    // "rarity"}]. The C++ side has already filtered to owned items
    // (unlock_default + account items) and mapped effect categories to the
    // loadout's heal/boost tabs.
    private void loadItems() {
        mItems.clear();
        String json = SurvBridge.getLoadoutItemsJson();
        if (json == null || json.isEmpty()) return;
        try {
            JsonElement parsed = JsonParser.parseString(json);
            if (!parsed.isJsonArray()) return;
            String l10n = SurvBridge.getL10nJson();
            JsonArray array = parsed.getAsJsonArray();
            for (JsonElement element : array) {
                if (element == null || !element.isJsonObject()) continue;
                JsonObject obj = element.getAsJsonObject();
                ItemEntry e = new ItemEntry();
                e.type = jsonString(obj, "type");
                e.category = jsonString(obj, "category");
                e.sprite = jsonString(obj, "sprite");
                String name = jsonString(obj, "name");
                String l10nName = Json.peekString(l10n, "game-" + e.type);
                e.displayName = l10nName != null && !l10nName.isEmpty() ? l10nName : name;
                if (e.displayName == null || e.displayName.isEmpty()) {
                    e.displayName = e.type.replace('_', ' ');
                }
                String lore = jsonString(obj, "lore");
                String l10nLore = Json.peekString(l10n, "game-" + e.type + "-lore");
                e.description = l10nLore != null && !l10nLore.isEmpty() ? l10nLore : lore;
                try {
                    e.rarity = obj.has("rarity") ? obj.get("rarity").getAsInt() : 0;
                } catch (Exception ignored) {
                    e.rarity = 0;
                }
                try {
                    e.tint = obj.has("tint") ? obj.get("tint").getAsInt() : 0xFFFFFF;
                } catch (Exception ignored) {
                    e.tint = 0xFFFFFF;
                }
                e.rarityColor = getRarityFromInt(e.rarity);
                mItems.add(e);
            }
        } catch (Exception e) {
            Log.w(TAG, "loadout items parse failed: " + e.getMessage());
        }
        applyAccountMetadata();
    }

    /** Attach account-only fields (source/time/status) for the detail panel. */
    private void applyAccountMetadata() {
        AccountManager account = AccountManager.peek();
        if (account == null || !account.isLoggedIn()) {
            return;
        }
        List<AccountManager.ItemInfo> owned = account.getItems();
        for (ItemEntry e : mItems) {
            for (AccountManager.ItemInfo info : owned) {
                if (e.type.equals(info.type)) {
                    e.source = info.source;
                    e.timeAcquired = info.timeAcquired;
                    e.isNew = info.status == 0;
                    break;
                }
            }
        }
    }

    private List<ItemEntry> getCategoryItems(String category) {
        List<ItemEntry> items = new ArrayList<ItemEntry>();
        // player_icon uses the emote pool (shared/defs emote defs); add an
        // explicit "None" entry because its validated default is "".
        String wanted = "player_icon".equals(category) ? "emote" : category;
        for (ItemEntry e : mItems) {
            if (wanted.equals(e.category)) items.add(e);
        }
        if ("player_icon".equals(category)) {
            addItem(items, "", "None", RARITY_STOCK, "No player icon");
        }
        return items;
    }

    private void addItem(List<ItemEntry> items, String type, String name, int rarity, String desc) {
        ItemEntry e = new ItemEntry();
        e.type = type;
        e.category = "player_icon";
        e.displayName = name;
        e.rarity = 0;
        e.rarityColor = rarity;
        e.description = desc;
        items.add(e);
    }

    private ItemEntry findItem(String type) {
        if (type == null || type.isEmpty()) return null;
        for (ItemEntry e : mItems) {
            if (e.type.equals(type)) return e;
        }
        return null;
    }
    
    private int getRarityFromInt(int rarity) {
        switch (rarity) {
            case 0: return RARITY_STOCK;
            case 1: return RARITY_COMMON;
            case 2: return RARITY_UNCOMMON;
            case 3: return RARITY_RARE;
            case 4: return RARITY_EPIC;
            case 5: return RARITY_MYTHIC;
            default: return RARITY_STOCK;
        }
    }
    
    private String getRarityName(int rarity) {
        switch (rarity) {
            case 0: return "Stock";
            case 1: return "Common";
            case 2: return "Uncommon";
            case 3: return "Rare";
            case 4: return "Epic";
            case 5: return "Mythic";
            default: return "Unknown";
        }
    }
    
    private String catLabel(String cat) {
        if ("outfit".equals(cat)) return "Outfit";
        if ("melee".equals(cat)) return "Melee";
        if ("emote".equals(cat)) return "Emote";
        if ("heal".equals(cat)) return "Heal Effect";
        if ("boost".equals(cat)) return "Boost Effect";
        if ("player_icon".equals(cat)) return "Player Icon";
        if ("crosshair".equals(cat)) return "Crosshair";
        return cat;
    }

    // --- JSON helpers ---

    private JsonObject parseObject(String json) {
        if (json == null || json.isEmpty()) return null;
        try {
            JsonElement element = JsonParser.parseString(json);
            return element != null && element.isJsonObject() ? element.getAsJsonObject() : null;
        } catch (Exception e) {
            return null;
        }
    }

    private String jsonString(JsonObject obj, String key) {
        if (obj == null || !obj.has(key)) return "";
        try {
            JsonElement element = obj.get(key);
            return element != null && element.isJsonPrimitive() ? element.getAsString() : "";
        } catch (Exception e) {
            return "";
        }
    }

    private List<String> parseArray(String json, String key) {
        List<String> items = new ArrayList<String>();
        if (json == null) return items;
        int at = json.indexOf("\"" + key + "\"");
        if (at < 0) return items;
        int colon = json.indexOf(':', at);
        if (colon < 0) return items;
        int arrayStart = json.indexOf('[', colon);
        if (arrayStart < 0) return items;
        StringBuilder seg = new StringBuilder();
        boolean inString = false;
        for (int i = arrayStart; i < json.length(); i++) {
            char c = json.charAt(i);
            if (inString) {
                if (c == '\\') { seg.append(c); i++; if (i < json.length()) seg.append(json.charAt(i)); }
                else if (c == '"') {
                    items.add(seg.toString());
                    seg = new StringBuilder();
                    inString = false;
                } else { seg.append(c); }
            } else {
                if (c == '"') inString = true;
                else if (c == ']') break;
            }
        }
        return items;
    }

    private String localizeName(String type) {
        if (type == null || type.isEmpty()) return "Stock";
        String l10n = SurvBridge.getL10nJson();
        String localized = Json.peekString(l10n, "game-" + type);
        if (localized != null && !localized.isEmpty()) return localized;
        return type.replace('_', ' ');
    }

    private String escapeJson(String s) {
        if (s == null) return "";
        StringBuilder out = new StringBuilder();
        for (int i = 0; i < s.length(); i++) {
            char c = s.charAt(i);
            switch (c) {
                case '"': out.append("\\\""); break;
                case '\\': out.append("\\\\"); break;
                case '\n': out.append("\\n"); break;
                case '\r': out.append("\\r"); break;
                default: out.append(c); break;
            }
        }
        return out.toString();
    }

    private Button makeCatButton(String label, boolean selected) {
        Button btn = new Button(mActivity);
        btn.setText(label);
        btn.setBackgroundColor(selected ? SELECT_GREEN : 0xFF3A3A3A);
        btn.setTextColor(Color.WHITE);
        btn.setPadding(dp(6), dp(4), dp(6), dp(4));
        btn.setTextSize(TypedValue.COMPLEX_UNIT_SP, 11);
        btn.setSingleLine(true);
        return btn;
    }
    
    private Button makeSmallButton(String label) {
        Button btn = new Button(mActivity);
        btn.setText(label);
        btn.setBackgroundColor(0xFF4A4A4A);
        btn.setTextColor(Color.WHITE);
        btn.setTextSize(TypedValue.COMPLEX_UNIT_SP, 11);
        btn.setPadding(dp(6), dp(2), dp(6), dp(2));
        btn.setSingleLine(true);
        return btn;
    }

    private int dp(int value) {
        return (int) (value * mActivity.getResources().getDisplayMetrics().density + 0.5f);
    }
}
