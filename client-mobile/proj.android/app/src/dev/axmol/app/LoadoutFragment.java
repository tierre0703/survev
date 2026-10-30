/****************************************************************************
 * LoadoutFragment.java — S4 loadout editor (ARCHITECTURE.md §7, step S4).
 *
 * Enhanced Java loadout screen with:
 * - Category tabs (outfit, melee, emote, heal, boost, player_icon, crosshair)
 * - Rarity color coding (Stock → Mythic)
 * - Sort options (Newest, Alpha, Rarity)
 * - Item details panel with name, rarity, acquired source and lore (the web
 *   #modal-customize-item-header/footer), incl. emote subcategories
 * - Category tabs get the web's unacknowledged-item alert dot
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
import android.text.InputType;
import android.text.SpannableString;
import android.text.Spanned;
import android.text.style.ForegroundColorSpan;
import android.util.Log;
import android.util.TypedValue;
import android.view.Gravity;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.EditText;
import android.widget.FrameLayout;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.ScrollView;
import android.widget.SeekBar;
import android.widget.TextView;

import com.google.gson.JsonArray;
import com.google.gson.JsonElement;
import com.google.gson.JsonObject;
import com.google.gson.JsonParser;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.Comparator;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

public class LoadoutFragment {
    private static final String TAG = "LoadoutFragment";

    // Web loadoutMenu.ts Rarities table (Stock/Common grey, Uncommon green,
    // Rare cyan, Epic magenta, Mythic orange).
    private static final int RARITY_STOCK     = 0xFFC5C5C5;
    private static final int RARITY_COMMON    = 0xFFC5C5C5;
    private static final int RARITY_UNCOMMON  = 0xFF12FF00;
    private static final int RARITY_RARE      = 0xFF00DEFF;
    private static final int RARITY_EPIC      = 0xFFF600FF;
    private static final int RARITY_MYTHIC    = 0xFFD96100;

    // Web rarity label keys (en.json loadout-stock..mythic).
    private static final String[] RARITY_KEYS =
            {"stock", "common", "uncommon", "rare", "epic", "mythic"};

    // Web #modal-customize-item-source / -lore colours.
    private static final int DETAIL_SOURCE_COLOR = 0xFFFFDB7B;
    private static final int DETAIL_LORE_COLOR   = 0xFFD2FF7B;

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
    private final TextView mTitle;
    private final TextView mSelectionLabel;
    // Detail panel: the web modal-customize header (name + rarity) and footer
    // (source + lore) as separate lines.
    private final LinearLayout mDetailPanel;
    private final TextView mDetailName;
    private final TextView mDetailRarity;
    private final TextView mDetailSource;
    private final TextView mDetailLore;
    private final Button mNewItemBtn;
    private final LinearLayout mEmoteSlotRow;
    private final Button[] mEmoteSlotButtons = new Button[6];
    private final Button mSortBtn;
    private final Button mPrevBtn;
    private final Button mNextBtn;
    private final Button mCloseBtn;
    private final Button[] mCatButtons = new Button[7];

    // Account item source display names ({"type","name"}), resolved once from
    // SurvBridge.getLoadoutSourcesJson(). Static: def data never changes.
    private static Map<String, String> sSourceNames;
    
    // Crosshair editor widgets
    private final LinearLayout mCrosshairEditor;
    private final ImageView mCrosshairPreview;
    private final LinearLayout mColorPickerRow;
    private final Button[] mColorButtons;
    private String mCrosshairType = "crosshair_default";
    private SeekBar mSizeSlider;
    private SeekBar mStrokeSlider;
    private TextView mSizeValue;
    private TextView mStrokeValue;
    private TextView mSizeLabel;
    private TextView mStrokeLabel;
    private TextView mHexLabel;
    private EditText mColorHex;

    // --- state ---
    private String[] mCategories = {"outfit", "melee", "emote", "heal", "boost", "player_icon", "crosshair"};
    private static final int EMOTE_SLOT_COUNT = 6;
    private static final String[] EMOTE_SLOT_LABELS = {"Top", "Right", "Bottom", "Left", "Win", "Death"};
    // l10n suffix per EmoteSlot (Top/Right/Bottom/Left/Win/Death).
    private static final String[] EMOTE_SLOT_KEYS = {"top", "right", "bottom", "left", "win", "death"};
    private int mSelectedCat = 0;
    private int mPage = 0;
    private int mEmoteSlot = 0;
    private boolean mOpen = false;
    private int mSortMode = 0; // 0=newest, 1=alpha, 2=rarity
    // Real shared/defs items (owned by this account), refreshed on each show.
    private final List<ItemEntry> mItems = new ArrayList<ItemEntry>();
    
    // Crosshair state (web config.ts: color int, size 0.25..1.0, stroke 0..1.5;
    // the editor exposes the same ranges as the web sliders, and the in-game
    // sprite bakes 0.25 stroke steps in Content/gui/crosshair).
    private int mCrosshairColor = 0xFFFFFF; // web default (white)
    private float mCrosshairSize = 1.0f;
    private float mCrosshairStroke = 0.0f;

    private static final float CROSSHAIR_SIZE_MIN = 0.25f;
    private static final float CROSSHAIR_SIZE_MAX = 1.0f;
    private static final float CROSSHAIR_STROKE_MAX = 1.5f;
    private static final int CROSSHAIR_STROKE_STEPS = 6; // 0.25 per step

    private static final String[] CROSSHAIR_COLORS = {"#FF0000", "#00FF00", "#0000FF", "#00FFFF", "#FF00FF", "#FFFF00", "#FFFFFF"};

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
        
        mTitle = new TextView(activity);
        mTitle.setTextColor(Color.WHITE);
        mTitle.setTextSize(TypedValue.COMPLEX_UNIT_SP, 20);
        mTitle.setGravity(Gravity.CENTER);
        mTitle.setLayoutParams(new LinearLayout.LayoutParams(0, ViewGroup.LayoutParams.WRAP_CONTENT, 1f));
        titleRow.addView(mTitle);

        mSortBtn = makeSmallButton("");
        mSortBtn.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                cycleSort();
            }
        });
        titleRow.addView(mSortBtn);
        
        panel.addView(titleRow);

        // Category row
        mCategoryRow = new LinearLayout(activity);
        mCategoryRow.setOrientation(LinearLayout.HORIZONTAL);
        mCategoryRow.setGravity(Gravity.CENTER);
        mCategoryRow.setPadding(0, dp(6), 0, dp(6));
        
        // Web #modal-customize-header: the category tabs are icon-only (the
        // localized `loadout-title-*` name is shown above the grid). Each tab
        // gets an equal weight so the icons stay tappable.
        for (int i = 0; i < mCategories.length; i++) {
            final int catIdx = i;
            Button catBtn = makeCatButton("", i == 0);
            mCatButtons[i] = catBtn;
            Drawable catIcon = scaledDrawable(
                    loadAssetBitmap("gui/" + CATEGORY_ICONS[i] + ".png", 4), 20);
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
            LinearLayout.LayoutParams clp = new LinearLayout.LayoutParams(
                    0, ViewGroup.LayoutParams.WRAP_CONTENT, 1f);
            mCategoryRow.addView(catBtn, clp);
        }
        panel.addView(mCategoryRow);

        // Localized category title (web #modal-customize-cat-title).
        mSelectionLabel = new TextView(activity);
        mSelectionLabel.setTextColor(GOLD);
        mSelectionLabel.setTextSize(TypedValue.COMPLEX_UNIT_SP, 14);
        mSelectionLabel.setPadding(0, 0, 0, dp(4));
        mSelectionLabel.setGravity(Gravity.CENTER);
        panel.addView(mSelectionLabel);

        // New-item confirmation banner (web modal-item-confirm reduced to the
        // touch screen): appears while account items have status < Confirmed.
        mNewItemBtn = makeSmallButton(L10n.t("loadout-new-items", "New items unlocked - tap to confirm"));
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
            Button slotBtn = makeSmallButton(emoteSlotLabel(i));
            // Six equal-width slots: the localized labels (e.g. "Izquierda",
            // "Esquerda") need a smaller face than the generic small button.
            slotBtn.setTextSize(TypedValue.COMPLEX_UNIT_SP, 9);
            slotBtn.setPadding(dp(1), dp(2), dp(1), dp(2));
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
        
        // Colour swatches (web #color-picker). The web has no text label here:
        // the hex input below carries the "#" prefix.
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
                    mCrosshairColor = Color.parseColor(CROSSHAIR_COLORS[idx]);
                    if (mColorHex != null) mColorHex.setText(crosshairColorHex(mCrosshairColor));
                    updateCrosshairPreview();
                    updateColorSelection();
                    saveCrosshairSettings();
                }
            });
            mColorPickerRow.addView(mColorButtons[i]);
        }
        mCrosshairEditor.addView(mColorPickerRow);
        
        // Hex colour input (web #color-picker-hex): any #RRGGBB, applied on
        // Done/focus loss. The swatches above stay as shortcuts.
        LinearLayout hexRow = new LinearLayout(activity);
        hexRow.setOrientation(LinearLayout.HORIZONTAL);
        hexRow.setGravity(Gravity.CENTER_VERTICAL);
        hexRow.setPadding(0, dp(4), 0, dp(2));
        TextView hexLbl = new TextView(activity);
        hexLbl.setText("#");
        hexLbl.setTextColor(Color.WHITE);
        hexLbl.setTextSize(TypedValue.COMPLEX_UNIT_SP, 13);
        hexRow.addView(hexLbl);
        mHexLabel = hexLbl;
        mColorHex = new EditText(activity);
        mColorHex.setSingleLine(true);
        mColorHex.setInputType(InputType.TYPE_CLASS_TEXT);
        mColorHex.setTextSize(TypedValue.COMPLEX_UNIT_SP, 12);
        mColorHex.setTextColor(Color.WHITE);
        mColorHex.setHint("#RRGGBB");
        mColorHex.setHintTextColor(0xFF888888);
        LinearLayout.LayoutParams hexLp = new LinearLayout.LayoutParams(dp(100), dp(40));
        hexLp.setMargins(dp(6), 0, 0, 0);
        hexRow.addView(mColorHex, hexLp);
        mColorHex.setOnEditorActionListener(new TextView.OnEditorActionListener() {
            @Override
            public boolean onEditorAction(TextView v, int actionId, android.view.KeyEvent event) {
                applyHexColor();
                return true;
            }
        });
        mColorHex.setOnFocusChangeListener(new View.OnFocusChangeListener() {
            @Override public void onFocusChange(View v, boolean hasFocus) {
                if (!hasFocus) applyHexColor();
            }
        });
        mCrosshairEditor.addView(hexRow);

        // Size slider: web #crosshair-size (0.25..1.0, step 0.01 here).
        LinearLayout sizeRow = makeSliderRow(L10n.t("loadout-size", "Size"), 75,
                Math.round((mCrosshairSize - CROSSHAIR_SIZE_MIN) * 100));
        mSizeLabel = (TextView) sizeRow.getChildAt(0);
        mSizeValue = (TextView) sizeRow.getChildAt(1);
        mSizeValue.setText(String.format("%.2f", mCrosshairSize));
        mSizeSlider = (SeekBar) sizeRow.getChildAt(2);
        mSizeSlider.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {
            @Override public void onProgressChanged(SeekBar sb, int progress, boolean fromUser) {
                mCrosshairSize = CROSSHAIR_SIZE_MIN + progress / 100f;
                mSizeValue.setText(String.format("%.2f", mCrosshairSize));
                if (fromUser) updateCrosshairPreview();
            }
            @Override public void onStartTrackingTouch(SeekBar sb) {}
            @Override public void onStopTrackingTouch(SeekBar sb) { saveCrosshairSettings(); }
        });
        mCrosshairEditor.addView(sizeRow);

        // Stroke slider: web #crosshair-stroke (0..1.5); the sprite has 0.25 steps.
        LinearLayout strokeRow = makeSliderRow(L10n.t("loadout-stroked", "Stroke"),
                CROSSHAIR_STROKE_STEPS, Math.round(mCrosshairStroke * 4.0f));
        mStrokeLabel = (TextView) strokeRow.getChildAt(0);
        mStrokeValue = (TextView) strokeRow.getChildAt(1);
        mStrokeValue.setText(String.format("%.2f", mCrosshairStroke));
        mStrokeSlider = (SeekBar) strokeRow.getChildAt(2);
        mStrokeSlider.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {
            @Override public void onProgressChanged(SeekBar sb, int progress, boolean fromUser) {
                mCrosshairStroke = progress / 4.0f;
                mStrokeValue.setText(String.format("%.2f", mCrosshairStroke));
                if (fromUser) updateCrosshairPreview();
            }
            @Override public void onStartTrackingTouch(SeekBar sb) {}
            @Override public void onStopTrackingTouch(SeekBar sb) { saveCrosshairSettings(); }
        });
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

        // Detail panel (web #modal-customize-item-header + footer): name and
        // rarity on one row, then the localized source and lore lines.
        mDetailPanel = new LinearLayout(activity);
        mDetailPanel.setOrientation(LinearLayout.VERTICAL);
        mDetailPanel.setPadding(dp(8), dp(4), dp(8), dp(4));
        mDetailPanel.setBackgroundColor(0xFF252525);
        mDetailPanel.setVisibility(View.GONE);

        LinearLayout detailHeader = new LinearLayout(activity);
        detailHeader.setOrientation(LinearLayout.HORIZONTAL);
        detailHeader.setGravity(Gravity.CENTER_VERTICAL);

        mDetailName = new TextView(activity);
        mDetailName.setTextColor(Color.WHITE);
        mDetailName.setTextSize(TypedValue.COMPLEX_UNIT_SP, 14);
        mDetailName.setTypeface(null, android.graphics.Typeface.BOLD);
        detailHeader.addView(mDetailName, new LinearLayout.LayoutParams(
                0, ViewGroup.LayoutParams.WRAP_CONTENT, 1f));

        mDetailRarity = new TextView(activity);
        mDetailRarity.setTextSize(TypedValue.COMPLEX_UNIT_SP, 12);
        mDetailRarity.setTypeface(null, android.graphics.Typeface.BOLD);
        mDetailRarity.setPadding(dp(6), 0, 0, 0);
        detailHeader.addView(mDetailRarity);
        mDetailPanel.addView(detailHeader);

        mDetailSource = new TextView(activity);
        mDetailSource.setTextColor(DETAIL_SOURCE_COLOR);
        mDetailSource.setTextSize(TypedValue.COMPLEX_UNIT_SP, 11);
        mDetailPanel.addView(mDetailSource);

        mDetailLore = new TextView(activity);
        mDetailLore.setTextColor(DETAIL_LORE_COLOR);
        mDetailLore.setTextSize(TypedValue.COMPLEX_UNIT_SP, 11);
        mDetailLore.setTypeface(null, android.graphics.Typeface.ITALIC);
        mDetailLore.setMaxLines(4);
        mDetailLore.setEllipsize(android.text.TextUtils.TruncateAt.END);
        mDetailPanel.addView(mDetailLore);

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
        mCloseBtn.setText(L10n.t("index-close", "Close") + " \u2715");
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
        // Localized chrome (web l10n keys); re-applied on every refresh so a
        // language change in Settings is picked up when the screen reopens.
        updateChromeStrings();

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
        updateChromeStrings();

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
                try {
                    mCrosshairColor = Color.parseColor(chColor);
                } catch (Exception ignored) {
                    mCrosshairColor = 0xFFFFFF;
                }
            } else {
                mCrosshairColor = 0xFFFFFF;
            }
            // Clamp old editor values (0.7/1.4 size, 0/1/2 stroke) into the web
            // slider ranges; saving then stores the web-compatible floats.
            mCrosshairSize = (float) Math.max(CROSSHAIR_SIZE_MIN,
                    Math.min(CROSSHAIR_SIZE_MAX, chSize));
            mCrosshairStroke = (float) Math.max(0.0, Math.min(CROSSHAIR_STROKE_MAX, chStroke));

            if (!selectedType.isEmpty()) mCrosshairType = selectedType;
            if (mColorHex != null) mColorHex.setText(crosshairColorHex(mCrosshairColor));
            if (mSizeSlider != null) {
                mSizeSlider.setProgress(Math.round((mCrosshairSize - CROSSHAIR_SIZE_MIN) * 100));
                mSizeValue.setText(String.format("%.2f", mCrosshairSize));
            }
            if (mStrokeSlider != null) {
                mStrokeSlider.setProgress(crosshairStrokeBucket(mCrosshairStroke));
                mStrokeValue.setText(String.format("%.2f", mCrosshairStroke));
            }
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

        // Web #modal-customize-cat-title: the localized category name above the
        // grid (the selected item's name lives in the detail header below).
        mSelectionLabel.setText(categoryTitle(mSelectedCat));
        mSelectionLabel.setTextColor(GOLD);

        refreshGrid(cat, selectedType);
    }

    /** Re-applies the localized chrome strings (web l10n keys). */
    private void updateChromeStrings() {
        mTitle.setText(L10n.t("index-loadout", "Loadout"));
        mSortBtn.setText(sortLabel());
        if (mSizeLabel != null) mSizeLabel.setText(L10n.t("loadout-size", "Size"));
        if (mStrokeLabel != null) mStrokeLabel.setText(L10n.t("loadout-stroked", "Stroke"));
        if (mHexLabel != null) mHexLabel.setText("#");
        mCloseBtn.setText(L10n.t("index-close", "Close") + " \u2715");
        for (int i = 0; i < mEmoteSlotButtons.length; i++) {
            if (mEmoteSlotButtons[i] != null) {
                mEmoteSlotButtons[i].setText(emoteSlotLabel(i));
            }
        }
        for (int i = 0; i < mCatButtons.length && i < mCategories.length; i++) {
            if (mCatButtons[i] != null) mCatButtons[i].setContentDescription(categoryTitle(i));
        }
    }

    /** Web loadout-title-&lt;loadoutType&gt;, upper-cased like selectCat(). */
    private String categoryTitle(int catIdx) {
        String key = "loadout-title-" + mCategories[catIdx];
        return L10n.t(key, catLabel(mCategories[catIdx])).toUpperCase(java.util.Locale.ROOT);
    }

    /** Emote slot label (Top/Right/Bottom/Left/Win/Death), localized. */
    private String emoteSlotLabel(int slot) {
        if (slot < 0 || slot >= EMOTE_SLOT_KEYS.length) return "";
        return L10n.t("loadout-emote-" + EMOTE_SLOT_KEYS[slot], EMOTE_SLOT_LABELS[slot]);
    }

    /** Web #modal-customize-sort option labels (no invented "Sort:" prefix). */
    private String sortLabel() {
        if (mSortMode == 1) return L10n.t("loadout-alpha", "Alpha");
        if (mSortMode == 2) return L10n.t("loadout-rarity", "Rarity");
        return L10n.t("loadout-newest", "Newest");
    }

    // --- New-item confirmation (web loadoutMenu onItems/tryBeginConfirming) ---

    private String itemCategory(String tab) {
        return "player_icon".equals(tab) ? "emote" : tab;
    }

    private void refreshItemStatus() {
        AccountManager account = AccountManager.peek();
        if (account == null || !account.isLoggedIn()) {
            mNewItemBtn.setVisibility(View.GONE);
            updateCategoryAlerts(null);
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
            mNewItemBtn.setText(String.format(
                    java.util.Locale.ROOT,
                    L10n.t("loadout-new-item", "New: %s - tap to confirm"),
                    names.toString()));
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
        updateCategoryAlerts(account);
    }

    /**
     * Web loadoutMenu.ts setCategoryAlerts: a dot on every category tab that
     * still has unacknowledged items. The opened category was just acked above,
     * so its dot clears in the same refresh (the web behaves the same way).
     */
    private void updateCategoryAlerts(AccountManager account) {
        boolean loggedIn = account != null && account.isLoggedIn();
        for (int i = 0; i < mCatButtons.length && i < mCategories.length; i++) {
            Button tab = mCatButtons[i];
            if (tab == null) continue;
            String wanted = itemCategory(mCategories[i]);
            boolean hasNew = false;
            if (loggedIn) {
                for (AccountManager.ItemInfo info : account.getItems()) {
                    if (info.status >= 1) continue;
                    ItemEntry entry = findItem(info.type);
                    if (entry != null && wanted.equals(entry.category)) {
                        hasNew = true;
                        break;
                    }
                }
            }
            // The dot keeps the web's alert look but the client's gold accent
            // (it is the same colour as the tile NEW badge). The tabs are
            // icon-only, so the dot is the button text.
            String dot = hasNew ? "\u25CF" : "";
            SpannableString text = new SpannableString(dot);
            if (hasNew) {
                text.setSpan(new ForegroundColorSpan(GOLD), 0, dot.length(),
                        Spanned.SPAN_EXCLUSIVE_EXCLUSIVE);
            }
            tab.setText(text);
            tab.setTextSize(TypedValue.COMPLEX_UNIT_SP, 9);
            tab.setContentDescription(categoryTitle(i) + (hasNew ? " \u25CF" : ""));
        }
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
        // Web #modal-customize-item-header: white name, rarity label in the
        // rarity colour.
        mDetailName.setText(entry.displayName);
        mDetailRarity.setText(getRarityName(entry.rarity));
        mDetailRarity.setTextColor(entry.rarityColor);
        // Web footer #modal-customize-item-source ("Acquired: <source>").
        mDetailSource.setText(L10n.t("loadout-acquired", "Acquired") + ": "
                + sourceLabel(entry.source));
        // Web footer #modal-customize-item-lore: emotes show their subcategory
        // (`Category: Faces`) instead of the lore text.
        String lore;
        if ("emote".equals(entry.category)) {
            String subcat = entry.subcat == null || entry.subcat.isEmpty()
                    ? "other" : entry.subcat;
            lore = L10n.t("loadout-category", "Category") + ": "
                    + L10n.t("emote-subcat-" + subcat, subcat);
        } else {
            lore = entry.description == null ? "" : entry.description;
        }
        mDetailLore.setText(lore);
        mDetailLore.setVisibility(lore.isEmpty() ? View.GONE : View.VISIBLE);
        mDetailPanel.setVisibility(View.VISIBLE);
    }

    /**
     * Web loadoutMenu.ts getItemSourceName + the translate chain: the account
     * source is a def type ("unlock_new_account", "pass_survivr1"), resolved to
     * the def name ("new-account") and then localized through
     * `loadout-<name>` / `<name>` / the raw name.
     */
    private String sourceLabel(String source) {
        if (source == null || source.isEmpty()) return "Unknown";
        String name = sourceNameMap().get(source);
        if (name == null || name.isEmpty()) name = source;
        String localized = L10n.t("loadout-" + name, null);
        if (localized == null) localized = L10n.t(name, null);
        return localized != null ? localized : name;
    }

    private Map<String, String> sourceNameMap() {
        if (sSourceNames != null) return sSourceNames;
        sSourceNames = new HashMap<String, String>();
        String json = SurvBridge.getLoadoutSourcesJson();
        if (json == null || json.isEmpty()) return sSourceNames;
        try {
            JsonElement parsed = JsonParser.parseString(json);
            if (!parsed.isJsonArray()) return sSourceNames;
            for (JsonElement element : parsed.getAsJsonArray()) {
                if (element == null || !element.isJsonObject()) continue;
                JsonObject obj = element.getAsJsonObject();
                String type = jsonString(obj, "type");
                String name = jsonString(obj, "name");
                if (!type.isEmpty() && !name.isEmpty()) sSourceNames.put(type, name);
            }
        } catch (Exception e) {
            Log.w(TAG, "loadout sources parse failed: " + e.getMessage());
        }
        return sSourceNames;
    }

    // --- Sorting ---
    
    private void cycleSort() {
        mSortMode = (mSortMode + 1) % 3;
        mSortBtn.setText(sortLabel());
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

    private static int crosshairStrokeBucket(float stroke) {
        int bucket = Math.round(stroke * 4.0f);
        return Math.max(0, Math.min(CROSSHAIR_STROKE_STEPS, bucket));
    }

    private static String crosshairColorHex(int color) {
        return String.format("#%06X", color & 0xFFFFFF);
    }

    private void updateCrosshairPreview() {
        // Stroke buckets are generated for the in-game sprite (0.25 steps); reuse
        // the same art here so the preview matches the selected stroke.
        Bitmap bitmap = loadAssetBitmap(
                "gui/crosshair/" + mCrosshairType + "_s" + crosshairStrokeBucket(mCrosshairStroke) + ".png", 2);
        if (bitmap == null) {
            bitmap = loadItemBitmap(mCrosshairType);
        }
        mCrosshairPreview.setImageBitmap(bitmap);
        if (bitmap != null) {
            ColorMatrix matrix = new ColorMatrix(new float[] {
                    Color.red(mCrosshairColor) / 255f, 0, 0, 0, 0,
                    0, Color.green(mCrosshairColor) / 255f, 0, 0, 0,
                    0, 0, Color.blue(mCrosshairColor) / 255f, 0, 0,
                    0, 0, 0, 1, 0,
            });
            mCrosshairPreview.setColorFilter(new ColorMatrixColorFilter(matrix));
        } else {
            mCrosshairPreview.clearColorFilter();
        }
        // Web getCrosshairDims: the art is 64px * size (0.25..1.0).
        float scale = Math.max(0.25f, mCrosshairSize);
        mCrosshairPreview.setScaleX(scale);
        mCrosshairPreview.setScaleY(scale);
    }

    private void updateColorSelection() {
        for (int i = 0; i < mColorButtons.length; i++) {
            // Add border effect for selected
            GradientDrawable bg = new GradientDrawable();
            bg.setColor(Color.parseColor(CROSSHAIR_COLORS[i]));
            bg.setCornerRadius(dp(10));
            if (Color.parseColor(CROSSHAIR_COLORS[i]) == mCrosshairColor) {
                bg.setStroke(dp(3), Color.WHITE);
            } else {
                bg.setStroke(dp(1), 0xFF666666);
            }
            mColorButtons[i].setBackground(bg);
        }
    }

    /** Web #color-picker-hex: applies a typed #RRGGBB colour (invalid input is ignored). */
    private void applyHexColor() {
        if (mColorHex == null) return;
        String text = mColorHex.getText() != null ? mColorHex.getText().toString().trim() : "";
        if (text.startsWith("#")) text = text.substring(1);
        if (!text.matches("[0-9a-fA-F]{6}")) {
            mColorHex.setText(crosshairColorHex(mCrosshairColor));
            return;
        }
        mCrosshairColor = 0xFF000000 | Integer.parseInt(text, 16);
        mColorHex.setText(crosshairColorHex(mCrosshairColor));
        mColorHex.clearFocus();
        updateColorSelection();
        updateCrosshairPreview();
        saveCrosshairSettings();
    }

    private void saveCrosshairSettings() {
        // Send only the crosshair object; the C++ bridge merges it into the
        // current loadout (outfit/emotes/etc. must not be replaced). Values are
        // the web config.ts floats (size 0.25..1.0, stroke 0..1.5).
        StringBuilder json = new StringBuilder();
        json.append("{\"crosshair\":{\"color\":\"").append(crosshairColorHex(mCrosshairColor)).append("\"");
        json.append(",\"size\":").append(String.format(java.util.Locale.US, "%.2f", mCrosshairSize));
        json.append(",\"stroke\":").append(String.format(java.util.Locale.US, "%.2f", mCrosshairStroke));
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
        String subcat = ""; // emote subcategory key ("faces", "logos", ...)
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
                e.subcat = jsonString(obj, "subcat");
                // loadout.ts getUserAvailableItems: unlock_default items carry
                // source "unlock_default"; account items override it below.
                e.source = jsonString(obj, "source");
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
                    // The profile source wins; an empty one keeps the
                    // unlock_default source from the item snapshot.
                    if (info.source != null && !info.source.isEmpty()) {
                        e.source = info.source;
                    }
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
        if (rarity < 0 || rarity >= RARITY_KEYS.length) return "Unknown";
        String key = RARITY_KEYS[rarity];
        // en.json loadout-stock/common/uncommon/rare/epic/mythic.
        return L10n.t("loadout-" + key, Character.toUpperCase(key.charAt(0)) + key.substring(1));
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

    /** Label | value | SeekBar row (web `.crosshair-slider-container`). */
    private LinearLayout makeSliderRow(String label, int max, int progress) {
        LinearLayout row = new LinearLayout(mActivity);
        row.setOrientation(LinearLayout.HORIZONTAL);
        row.setGravity(Gravity.CENTER_VERTICAL);
        row.setPadding(0, dp(2), 0, dp(2));

        TextView lbl = new TextView(mActivity);
        lbl.setText(label);
        lbl.setTextColor(Color.WHITE);
        lbl.setTextSize(TypedValue.COMPLEX_UNIT_SP, 13);
        lbl.setLayoutParams(new LinearLayout.LayoutParams(dp(56), ViewGroup.LayoutParams.WRAP_CONTENT));
        row.addView(lbl);

        TextView value = new TextView(mActivity);
        value.setTextColor(0xFF83AF50);
        value.setTextSize(TypedValue.COMPLEX_UNIT_SP, 12);
        value.setLayoutParams(new LinearLayout.LayoutParams(dp(40), ViewGroup.LayoutParams.WRAP_CONTENT));
        row.addView(value);

        SeekBar slider = new SeekBar(mActivity);
        slider.setMax(Math.max(1, max));
        slider.setProgress(Math.max(0, Math.min(max, progress)));
        slider.setLayoutParams(new LinearLayout.LayoutParams(0, ViewGroup.LayoutParams.WRAP_CONTENT, 1f));
        row.addView(slider);
        return row;
    }

    private int dp(int value) {
        return (int) (value * mActivity.getResources().getDisplayMetrics().density + 0.5f);
    }
}
