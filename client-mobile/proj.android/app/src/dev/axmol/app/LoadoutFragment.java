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
import android.graphics.Color;
import android.graphics.drawable.GradientDrawable;
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

    private final Activity mActivity;
    private final FrameLayout mRoot;
    private final LinearLayout mCategoryRow;
    private final LinearLayout mGrid;
    private final TextView mSelectionLabel;
    private final TextView mDetailPanel;      // item details area
    private final Button mEmoteSlotBtn;
    private final Button mSortBtn;
    private final Button mPrevBtn;
    private final Button mNextBtn;
    private final Button mCloseBtn;
    
    // Crosshair editor widgets
    private final LinearLayout mCrosshairEditor;
    private final TextView mCrosshairPreview;
    private final LinearLayout mColorPickerRow;
    private final Button[] mColorButtons;
    private final Button mSizeBtn;
    private final Button mStrokeBtn;

    // --- state ---
    private String[] mCategories = {"outfit", "melee", "emote", "heal", "boost", "player_icon", "crosshair"};
    private int mSelectedCat = 0;
    private int mPage = 0;
    private int mEmoteSlot = 0;
    private int mEmoteCount = 8;
    private boolean mOpen = false;
    private int mSortMode = 0; // 0=newest, 1=alpha, 2=rarity
    
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

        // Emote slot selector (visible only for emote category)
        mEmoteSlotBtn = new Button(activity);
        mEmoteSlotBtn.setText("Emote Slot 1");
        mEmoteSlotBtn.setBackgroundColor(0xFF4A4A4A);
        mEmoteSlotBtn.setTextColor(Color.WHITE);
        mEmoteSlotBtn.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                if (mEmoteCount > 0) {
                    mEmoteSlot = (mEmoteSlot + 1) % mEmoteCount;
                    refresh();
                }
            }
        });
        mEmoteSlotBtn.setVisibility(View.GONE);
        panel.addView(mEmoteSlotBtn);

        // === Crosshair Editor (visible only for crosshair category) ===
        mCrosshairEditor = new LinearLayout(activity);
        mCrosshairEditor.setOrientation(LinearLayout.VERTICAL);
        mCrosshairEditor.setPadding(dp(8), dp(8), dp(8), dp(8));
        mCrosshairEditor.setBackgroundColor(0xFF333333);
        mCrosshairEditor.setVisibility(View.GONE);
        
        // Preview area
        mCrosshairPreview = new TextView(activity);
        mCrosshairPreview.setText("+");
        mCrosshairPreview.setTextColor(Color.RED);
        mCrosshairPreview.setTextSize(TypedValue.COMPLEX_UNIT_SP, 36);
        mCrosshairPreview.setGravity(Gravity.CENTER);
        mCrosshairPreview.setPadding(0, dp(12), 0, dp(12));
        mCrosshairEditor.addView(mCrosshairPreview);
        
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
    }

    public void setOnChanged(OnChangedListener listener) {
        mOnChanged = listener;
    }

    public View root() { return mRoot; }

    public void show() {
        if (mOpen) return;
        mOpen = true;
        mRoot.setVisibility(View.VISIBLE);
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

        // Determine current selection and available items for this category.
        String cat = mCategories[mSelectedCat];
        String selectedType = "";

        // Show/hide appropriate panels based on category
        boolean isCrosshair = "crosshair".equals(cat);
        mCrosshairEditor.setVisibility(isCrosshair ? View.VISIBLE : View.GONE);
        mGrid.setVisibility(isCrosshair ? View.GONE : View.GONE);
        mEmoteSlotBtn.setVisibility(View.GONE);

        if (isCrosshair) {
            // Update crosshair editor state from the validated loadout
            // ({"crosshair":{"color":0xRRGGBB,"size":"1.00","stroke":"0.00"}}).
            String chColor = Json.peekNestedString(loadoutJson, "crosshair", "color");
            if (chColor == null) {
                int colorInt = (int) Json.peekNestedDouble(loadoutJson, "crosshair", "color", 0xFFFFFF);
                chColor = String.format("#%06X", colorInt & 0xFFFFFF);
            }
            double chSize = Json.peekNestedDouble(loadoutJson, "crosshair", "size", 1.0);
            double chStroke = Json.peekNestedDouble(loadoutJson, "crosshair", "stroke", 0.0);

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
            updateCrosshairPreview();
            updateColorSelection();
            
            mSelectionLabel.setText("Customize your crosshair appearance");
            return;
        }

        if ("emote".equals(cat)) {
            List<String> emotes = parseArray(loadoutJson, "emotes");
            mEmoteCount = Math.max(emotes.size(), 1);
            if (mEmoteSlot >= mEmoteCount && mEmoteCount > 0) mEmoteSlot = 0;
            if (mEmoteSlot < emotes.size()) {
                selectedType = emotes.get(mEmoteSlot);
            }
            mEmoteSlotBtn.setVisibility(View.VISIBLE);
            mEmoteSlotBtn.setText("Emote " + (mEmoteSlot + 1) + "/" + mEmoteCount
                    + " : " + localizeName(selectedType));
        } else {
            selectedType = Json.peekString(loadoutJson, cat);
        }

        String display = selectedType == null || selectedType.isEmpty()
                ? "Stock" : localizeName(selectedType);
        int rarityColor = getRarityColor(selectedType);
        mSelectionLabel.setText(catLabel(cat) + ": " + display);
        mSelectionLabel.setTextColor(rarityColor != RARITY_STOCK ? rarityColor : GOLD);

        refreshGrid(cat, selectedType);
    }

    private void refreshGrid(String category, String selectedType) {
        mGrid.removeAllViews();

        List<ItemEntry> items = getCategoryItems(category);
        
        // Apply sorting
        sortItems(items, mSortMode);

        // Paginate
        final int pageSize = 12;
        int pages = (items.size() + pageSize - 1) / pageSize;
        if (pages < 1) pages = 1;
        if (mPage >= pages) mPage = pages - 1;
        mPrevBtn.setEnabled(mPage > 0);
        mNextBtn.setEnabled(mPage + 1 < pages);

        // Build grid rows (4 per row)
        int start = mPage * pageSize;
        int end = Math.min(start + pageSize, items.size());

        LinearLayout currentRow = null;
        for (int i = start; i < end; i++) {
            int col = (i - start) % 4;
            if (col == 0) {
                currentRow = new LinearLayout(mActivity);
                currentRow.setOrientation(LinearLayout.HORIZONTAL);
                currentRow.setPadding(0, dp(3), 0, 0);
                mGrid.addView(currentRow);
            }

            final ItemEntry entry = items.get(i);
            String label = entry.displayName;
            boolean isSelected = entry.type.equals(selectedType);

            Button itemBtn = new Button(mActivity);
            itemBtn.setText(label);
            
            // Background: selected gets green, otherwise use rarity color (darkened)
            int bgColor = isSelected ? SELECT_GREEN : 0xFF3A3A3A;
            itemBtn.setBackgroundColor(bgColor);
            
            // Text color: use rarity color for unselected, white for selected
            itemBtn.setTextColor(isSelected ? Color.WHITE : entry.rarityColor);
            itemBtn.setSingleLine(true);
            
            final String cat = mCategories[mSelectedCat];
            final int slot = mEmoteSlot;
            itemBtn.setOnClickListener(new View.OnClickListener() {
                @Override
                public void onClick(View v) {
                    selectItem(cat, entry.type, slot);
                    showItemDetail(entry);
                }
            });

            if (currentRow != null) {
                LinearLayout.LayoutParams ilp = new LinearLayout.LayoutParams(
                        0, dp(40), 1f);
                ilp.setMargins(dp(2), 0, dp(2), 0);
                currentRow.addView(itemBtn, ilp);
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
            sb.append(entry.description);
        }
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
        }
        // mode 0 = newest (default order)
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
        StringBuilder loadoutJson = new StringBuilder();
        loadoutJson.append("{\"").append(category).append("\":\"").append(escapeJson(type)).append("\"");
        if ("emote".equals(category)) {
            loadoutJson.append(",\"emotes\":[\"").append(escapeJson(type)).append("\"]");
        }
        loadoutJson.append("}");

        boolean accepted = SurvBridge.setLoadout(loadoutJson.toString());
        Log.i(TAG, "select " + category + "=" + type + " accepted=" + accepted);

        if (mOnChanged != null) {
            mOnChanged.onChanged(category, type, emoteSlot);
        }

        refresh();
    }
    
    // --- Crosshair helpers ---
    
    private void updateCrosshairPreview() {
        mCrosshairPreview.setTextColor(Color.parseColor(CROSSHAIR_COLORS[mCrosshairColorIdx]));
        float sizeMult[] = {0.7f, 1.0f, 1.4f};
        float baseSize = 36 * sizeMult[mCrosshairSize];
        mCrosshairPreview.setTextSize(TypedValue.COMPLEX_UNIT_SP, baseSize);
        
        // Adjust stroke via shadow layer simulation
        int strokeWidth[] = {0, 1, 2};
        mCrosshairPreview.setShadowLayer(
            strokeWidth[mCrosshairStroke], 0, 0, 
            Color.parseColor(CROSSHAIR_COLORS[mCrosshairColorIdx]));
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

    // --- Item data (mirrors C++ ui::Loadout::items) with rarity ---

    static class ItemEntry {
        String type;
        String displayName;
        int rarity; // 0-5 stock->mythic
        int rarityColor;
        String description;
    }

    private List<ItemEntry> getCategoryItems(String category) {
        List<ItemEntry> items = new ArrayList<ItemEntry>();
        
        if ("outfit".equals(category)) {
            addItem(items, "", "Stock", 0, "Default outfit");
            addItem(items, "outfitBase", "Base Skin", 1, "Standard military uniform");
            addItem(items, "outfitWoodland", "Woodland", 2, "Forest camouflage pattern");
            addItem(items, "outfitSnow", "Arctic", 3, "Snow terrain camouflage");
            addItem(items, "outfitDesert", "Desert", 3, "Desert tan uniform");
            addItem(items, "outfitJungle", "Jungle", 4, "Tropical jungle camo");
        } else if ("melee".equals(category)) {
            addItem(items, "", "Fists", 0, "Your bare hands");
            addItem(items, "katana_rusted", "Rusty Katana", 2, "A worn katana blade");
            addItem(items, "katana", "Katana", 4, "Sharp samurai sword");
        } else if ("emote".equals(category)) {
            addItem(items, "emote_gg", "GG", 1, "Good Game!");
            addItem(items, "emote_wp", "Well Played", 1, "Respectful nod");
            addItem(items, "emote_cry", "Cry", 2, "Tears of defeat");
            addItem(items, "emote_laugh", "Laugh", 2, "Hysterical laughter");
            addItem(items, "emote_heart", "Heart", 3, "Send some love");
            addItem(items, "emote_thumbs_up", "Thumbs Up", 1, "Approval!");
            addItem(items, "emote_wave", "Wave", 1, "Friendly greeting");
            addItem(items, "emote_dab", "Dab", 4, "Dab dance move");
        } else if ("heal".equals(category)) {
           addItem(items, "heal_basic", "Bandages", 1, "Basic healing item");
            addItem(items, "heal_advanced", "Med Kit", 3, "Full health restore");
        } else if ("boost".equals(category)) {
            addItem(items, "boost_basic", "Soda", 1, "Minor speed boost");
            addItem(items, "boost_advanced", "Pills", 3, "Major adrenaline boost");
        } else if ("player_icon".equals(category)) {
            addItem(items, "", "Default", 0, "Standard icon");
            addItem(items, "emote_gg", "GG Icon", 1, "GG as icon");
            addItem(items, "emote_wp", "WP Icon", 1, "Well Played icon");
        } else if ("crosshair".equals(category)) {
            // Crosshair is handled by the editor, not grid
        }
        return items;
    }
    
    private void addItem(List<ItemEntry> items, String type, String name, int rarity, String desc) {
        ItemEntry e = new ItemEntry();
        e.type = type;
        e.displayName = name;
        e.rarity = rarity;
        e.rarityColor = getRarityFromInt(rarity);
        e.description = desc;
        items.add(e);
    }
    
    private int getRarityColor(String type) {
        if (type == null || type.isEmpty()) return RARITY_STOCK;
        // Map known items to their rarity colors
        List<ItemEntry> allItems = new ArrayList<>();
        String[] cats = {"outfit", "melee", "emote", "heal", "boost", "player_icon"};
        for (String cat : cats) {
            allItems.addAll(getCategoryItems(cat));
        }
        for (ItemEntry e : allItems) {
            if (e.type.equals(type)) return e.rarityColor;
        }
        return RARITY_COMMON;
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
