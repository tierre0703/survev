#pragma once
// Bundled news/patch-notes for the main menu's news box.
//
// The web client hard-codes these entries in `client/index.html` (`#news-block`
// entries with `<div data-date="YYYY-MM-DD">`, `<small class="news-date">` and
// `.news-paragraph` lines) and renders them without any network fetch. The
// native client mirrors that: the newest entry first, each with a date and a
// short list of paragraphs.
//
// `NewsEntry::href` is empty for in-app entries; a non-empty value means the
// entry links to a web page the client cannot open (shown, not tappable).
#include <string>
#include <vector>

namespace ui {

struct NewsParagraph {
    // Literal English text, or an l10n key when the loaded locale defines one.
    std::string text;
    // Web `.highlight` (gold, bold) / `.redacted` (red, bold) styling.
    enum class Style { Normal, Highlight, Redacted };
    Style style = Style::Normal;
};

struct NewsEntry {
    std::string date;   // YYYY-MM-DD, descending order
    std::string title;  // literal English, or an l10n key
    std::vector<NewsParagraph> paragraphs;
    std::string href;   // empty = no external link
};

// Newest first, mirroring the web client's `#news-block` order.
const std::vector<NewsEntry>& newsEntries();

} // namespace ui
