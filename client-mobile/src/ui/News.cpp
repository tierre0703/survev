#include "News.h"

namespace ui {

namespace {
NewsParagraph plain(const char* text) {
    NewsParagraph p;
    p.text = text;
    p.style = NewsParagraph::Style::Normal;
    return p;
}
NewsParagraph highlight(const char* text) {
    NewsParagraph p;
    p.text = text;
    p.style = NewsParagraph::Style::Highlight;
    return p;
}
NewsParagraph redacted(const char* text) {
    NewsParagraph p;
    p.text = text;
    p.style = NewsParagraph::Style::Redacted;
    return p;
}

// Port of the web client's `#news-block` entries (client/index.html). The web
// client hard-codes the English text and ships it as markup, so the native
// entries carry **literal English** strings (the web client has no news l10n
// keys). `NewsEntry::title`/`NewsParagraph::text` can still be an l10n key when
// the loaded locale defines one; the menu falls back to the literal text.
const std::vector<NewsEntry>& build() {
    static const std::vector<NewsEntry> entries = {
        { "2026-09-25", "Season Pass & Quests",
          { highlight("The Survivr Pass is here!"),
            plain("Complete quests to earn XP and unlock new skins, emotes and particles.") }, "" },
        { "2026-09-08", "New Perks",
          { plain("Four new perks have been added to the perk pool."),
            highlight("Find them in-game and adapt your strategy.") }, "" },
        { "2026-08-24", "Weapon Balance",
          { plain("Adjusted damage falloff and recoil across several weapons.") }, "" },
        { "2026-08-09", "Limited-Time Event",
          { plain("A new limited-time event is live on all regions."),
            redacted("Event ends soon - don't miss out!") }, "" },
        { "2026-07-15", "Client Update",
          { plain("Improved performance, hit registration and mobile controls.") }, "" },
    };
    return entries;
}

} // namespace

const std::vector<NewsEntry>& newsEntries() { return build(); }

} // namespace ui
