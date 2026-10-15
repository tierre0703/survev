#include "NewsJson.h"
#include "Config.h" // jsonToString (the shared encoder)

namespace ui {

namespace {

const char* styleName(NewsParagraph::Style style) {
    switch (style) {
        case NewsParagraph::Style::Highlight:
            return "highlight";
        case NewsParagraph::Style::Redacted:
            return "redacted";
        case NewsParagraph::Style::Normal:
        default:
            return "normal";
    }
}

JsonValue makeString(const std::string& text) {
    JsonValue value;
    value.kind = JsonValue::Kind::String;
    value.string = text;
    return value;
}

} // namespace

std::string newsToJson(const std::vector<NewsEntry>& entries) {
    JsonValue root;
    root.kind = JsonValue::Kind::Array;
    root.array.reserve(entries.size());
    for (const NewsEntry& entry : entries) {
        JsonValue item;
        item.kind = JsonValue::Kind::Object;
        item.object["date"] = makeString(entry.date);
        item.object["title"] = makeString(entry.title);
        item.object["href"] = makeString(entry.href);
        JsonValue paragraphs;
        paragraphs.kind = JsonValue::Kind::Array;
        paragraphs.array.reserve(entry.paragraphs.size());
        for (const NewsParagraph& paragraph : entry.paragraphs) {
            JsonValue line;
            line.kind = JsonValue::Kind::Object;
            line.object["text"] = makeString(paragraph.text);
            line.object["style"] = makeString(styleName(paragraph.style));
            paragraphs.array.push_back(std::move(line));
        }
        item.object["paragraphs"] = std::move(paragraphs);
        root.array.push_back(std::move(item));
    }
    return jsonToString(root);
}

} // namespace ui