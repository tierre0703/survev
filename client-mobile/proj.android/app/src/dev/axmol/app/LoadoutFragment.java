/****************************************************************************
 * LoadoutFragment.java — S4 loadout editor (ARCHITECTURE.md §7, step S4).
 *
 * The Java loadout screen: category tabs (outfit, melee, emote, heal, boost,
 * player_icon), a paged item grid, and selection that round-trips through the
 * C++ loadout validator via SurvBridge.setLoadout / getLoadoutJson.
 *
 * Threading: all SurvBridge calls are synchronous on the UI thread.
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

public class LoadoutFragment {
    private static final String TAG = "LoadoutFragment";

    private final Activity mActivity;
    private final FrameLayout mRoot;
    private final LinearLayout mCategoryRow;
    private final LinearLayout mGrid;
    private final TextView mSelectionLabel;
    private final Button mEmoteSlotBtn;
    private final Button mPrevBtn;
    private final Button mNextBtn;
    private final Button mCloseBtn;

    // --- state ---
    private String[] mCategories = {"outfit", "melee", "emote", "heal", "boost", "player_icon"};
    private int mSelectedCat = 0;
    private int mPage = 0;
    private int mEmoteSlot = 0;
    private int mEmoteCount = 8;
    private boolean mOpen = false;

    // Callback for when the loadout changes (C++ validator round-trip).
    private OnChangedListener mOnChanged;

    public interface OnChangedListener {
        void onChanged(String category, String type, int emoteSlot);
    }

    public LoadoutFragment(Activity activity) {
        mActivity = activity;

        mRoot = new FrameLayout(activity);
        mRoot.setBackgroundColor(0xE0000000); // fully dim, semi-transparent overlay

        // Main panel (centered, like web #modal-customize).
        LinearLayout panel = new LinearLayout(activity);
        panel.setOrientation(LinearLayout.VERTICAL);
        panel.setGravity(Gravity.CENTER);
        panel.setLayoutParams(new FrameLayout.LayoutParams(
                ViewGroup.LayoutParams.WRAP_CONTENT, ViewGroup.LayoutParams.WRAP_CONTENT));
        panel.setBackgroundColor(0xFF2A2A2A);
        panel.setPadding(dp(16), dp(16), dp(16), dp(16));

        // Title
        TextView title = new TextView(activity);
        title.setText("Customize");
        title.setTextColor(Color.WHITE);
        title.setTextSize(TypedValue.COMPLEX_UNIT_SP, 24);
        title.setGravity(Gravity.CENTER);
        title.setPadding(0, 0, 0, dp(8));
        panel.addView(title);

        // Category row
        mCategoryRow = new LinearLayout(activity);
        mCategoryRow.setOrientation(LinearLayout.HORIZONTAL);
        mCategoryRow.setGravity(Gravity.CENTER);
        mCategoryRow.setPadding(0, 0, 0, dp(8));
        for (int i = 0; i < mCategories.length; i++) {
            final int catIdx = i;
            final String cat = mCategories[i];
            Button catBtn = makeCatButton(cat, i == 0);
            catBtn.setOnClickListener(new View.OnClickListener() {
                @Override
                public void onClick(View v) {
                    selectCategory(catIdx);
                }
            });
            mCategoryRow.addView(catBtn);
        }
        panel.addView(mCategoryRow);

        // Selection label
        mSelectionLabel = new TextView(activity);
        mSelectionLabel.setTextColor(0xFFD4A843);
        mSelectionLabel.setTextSize(TypedValue.COMPLEX_UNIT_SP, 15);
        mSelectionLabel.setPadding(0, 0, 0, dp(4));
        panel.addView(mSelectionLabel);

        // Emote slot selector (visible only for emote category)
        mEmoteSlotBtn = new Button(activity);
        mEmoteSlotBtn.setText("Emote slot 1");
        mEmoteSlotBtn.setBackgroundColor(0xFF4A4A4A);
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

        // Item grid (scrollable)
        ScrollView scroll = new ScrollView(activity);
        scroll.setLayoutParams(new FrameLayout.LayoutParams(
                dp(600), dp(400)));
        mGrid = new LinearLayout(activity);
        mGrid.setOrientation(LinearLayout.VERTICAL);
        scroll.addView(mGrid);
        panel.addView(scroll);

        // Paging controls + Close
        LinearLayout bottomRow = new LinearLayout(activity);
        bottomRow.setOrientation(LinearLayout.HORIZONTAL);
        bottomRow.setGravity(Gravity.CENTER);

        mPrevBtn = new Button(activity);
        mPrevBtn.setText("<");
        mPrevBtn.setBackgroundColor(0xFF3A3A3A);
        mPrevBtn.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                if (mPage > 0) { mPage--; refresh(); }
            }
        });
        bottomRow.addView(mPrevBtn);

        mNextBtn = new Button(activity);
        mNextBtn.setText(">");
        mNextBtn.setBackgroundColor(0xFF3A3A3A);
        mNextBtn.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                mPage++;
                refresh();
            }
        });
        bottomRow.addView(mNextBtn);

        mCloseBtn = new Button(activity);
        mCloseBtn.setText("Back");
        mCloseBtn.setBackgroundColor(0xFF5A5A5A);
        mCloseBtn.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                hide();
            }
        });
        bottomRow.addView(mCloseBtn);

        panel.addView(bottomRow);

        mRoot.addView(panel);
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
        Log.i(TAG, "loadout hide");
    }

    public boolean isOpen() { return mOpen; }

    public void refresh() {
        // Read the validated loadout from C++.
        String loadoutJson = SurvBridge.getLoadoutJson();
        String configJson = SurvBridge.getConfigJson();

        // Determine current selection and available items for this category.
        String cat = mCategories[mSelectedCat];
        String selectedType = "";

        if ("emote".equals(cat)) {
            // Get the emote list from loadoutJson
            List<String> emotes = parseArray(loadoutJson, "emotes");
            mEmoteCount = emotes.size();
            if (mEmoteSlot >= mEmoteCount && mEmoteCount > 0) mEmoteSlot = 0;
            if (mEmoteSlot < emotes.size()) {
                selectedType = emotes.get(mEmoteSlot);
            }
            mEmoteSlotBtn.setVisibility(View.VISIBLE);
            mEmoteSlotBtn.setText("Emote " + (mEmoteSlot + 1) + "/" + mEmoteCount
                    + " : " + localizeName(selectedType));
        } else {
            selectedType = Json.peekString(loadoutJson, cat);
            mEmoteSlotBtn.setVisibility(View.GONE);
        }

        String display = selectedType == null || selectedType.isEmpty()
                ? "Stock" : localizeName(selectedType);
        mSelectionLabel.setText("Category: " + cat + "  |  " + display);

        // Build the item grid from the C++ def provider.
        // We get the available items via the validated loadout's keys,
        // but since Java can't call C++ DefProvider directly, we use
        // the loadout JSON structure — it contains the currently-selected
        // categories/emotes, and we let the C++ selector drive availability.
        //
        // For now use a static list derived from the C++ defaults.
        refreshGrid(cat, selectedType);
    }

    private void refreshGrid(String category, String selectedType) {
        mGrid.removeAllViews();

        // Get the available items list from C++ via SurvBridge.
        // The C++ Loadout::items() returns the available types per category.
        // We mirror the C++ defaults here — the loadout JSON from C++
        // publishes the validated selection; the Java side shows a grid of
        // known types and lets the C++ validator reject invalid ones.
        List<String> items = getCategoryItems(category);

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
                currentRow.setPadding(0, dp(4), 0, 0);
                mGrid.addView(currentRow);
            }

            final String type = items.get(i);
            String label = type.isEmpty() ? "Stock" : localizeName(type);
            boolean isSelected = type.equals(selectedType);

            Button itemBtn = new Button(mActivity);
            itemBtn.setText(label);
            itemBtn.setBackgroundColor(isSelected ? 0xFF5A7A38 : 0xFF3A3A3A);
            itemBtn.setTextColor(isSelected ? Color.WHITE : 0xFFB0B0B0);
            final String cat = mCategories[mSelectedCat];
            final int slot = mEmoteSlot;
            itemBtn.setOnClickListener(new View.OnClickListener() {
                @Override
                public void onClick(View v) {
                    selectItem(cat, type, slot);
                }
            });

            if (currentRow != null) {
                currentRow.addView(itemBtn);
            }
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
                btn.setBackgroundColor(i == idx ? 0xFF5A7A38 : 0xFF3A3A3A);
            }
        }
    }

    private void selectItem(String category, String type, int emoteSlot) {
        // Build loadout JSON for the C++ validator, then pass it through the bridge.
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

    // --- Item data (mirrors C++ ui::Loadout::items) ---

    private List<String> getCategoryItems(String category) {
        List<String> items = new ArrayList<String>();
        if ("outfit".equals(category)) {
            items.add("outfitBase"); items.add("outfitWoodland"); items.add("outfitSnow");
            items.add("outfitDesert"); items.add("outfitJungle");
        } else if ("melee".equals(category)) {
            items.add("fists");
            items.add("katana_rusted"); items.add("katana");
        } else if ("emote".equals(category)) {
            items.add("emote_gg"); items.add("emote_wp"); items.add("emote_cry");
            items.add("emote_laugh"); items.add("emote_heart"); items.add("emote_thumbs_up");
            items.add("emote_wave"); items.add("emote_dab");
        } else if ("heal".equals(category)) {
            items.add("heal_basic"); items.add("heal_advanced");
        } else if ("boost".equals(category)) {
            items.add("boost_basic"); items.add("boost_advanced");
        } else if ("player_icon".equals(category)) {
            items.add(""); items.add("emote_gg"); items.add("emote_wp");
        }
        return items;
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
        // Try the C++ localization via the bridge.
        String l10n = SurvBridge.getL10nJson();
        String localized = Json.peekString(l10n, "game-" + type);
        if (localized != null && !localized.isEmpty()) return localized;
        // Fallback: humanize the key name.
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
        btn.setBackgroundColor(selected ? 0xFF5A7A38 : 0xFF3A3A3A);
        btn.setPadding(dp(8), dp(6), dp(8), dp(6));
        return btn;
    }

    private int dp(int value) {
        return (int) (value * mActivity.getResources().getDisplayMetrics().density + 0.5f);
    }
}