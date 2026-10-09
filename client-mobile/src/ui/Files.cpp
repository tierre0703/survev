#include "Files.h"

#include "axmol.h"

#include <algorithm>
#include <vector>

namespace ui {

std::string Files::readText(const std::string& path) {
    if (path.empty()) {
        return {};
    }
    auto* fu = ax::FileUtils::getInstance();
    const std::string full = fu->fullPathForFilename(path);
    if (full.empty() || !fu->isFileExist(full)) {
        // Fall back to the raw name (some platforms accept it directly).
        if (!fu->isFileExist(path)) {
            return {};
        }
        return fu->getStringFromFile(path);
    }
    return fu->getStringFromFile(full);
}

bool Files::readBytes(const std::string& path, std::string& out) {
    out.clear();
    if (path.empty()) {
        return false;
    }
    auto* fu = ax::FileUtils::getInstance();
    const std::string full = fu->fullPathForFilename(path);
    const std::string target = full.empty() ? path : full;
    if (!fu->isFileExist(target)) {
        return false;
    }
    ax::Data data = fu->getDataFromFile(target);
    if (data.isNull() || data.getSize() == 0) {
        return false;
    }
    out.assign(reinterpret_cast<const char*>(data.getBytes()), data.getSize());
    return true;
}

bool Files::readJson(const std::string& path, JsonValue& out) {
    std::string text;
    if (!readBytes(path, text)) {
        text = readText(path);
    }
    if (text.empty()) {
        return false;
    }
    return JsonParser::parse(text, out);
}

bool Files::exists(const std::string& path) {
    if (path.empty()) {
        return false;
    }
    auto* fu = ax::FileUtils::getInstance();
    if (fu->isFileExist(path)) {
        return true;
    }
    const std::string full = fu->fullPathForFilename(path);
    return !full.empty() && fu->isFileExist(full);
}

std::string Files::fullPath(const std::string& path) {
    return ax::FileUtils::getInstance()->fullPathForFilename(path);
}

void Files::registerFonts(const std::string& dir) {
    for (const auto& entry : kGuiFontFiles) {
        const std::string path = dir.empty() ? std::string(entry.file) : dir + "/" + entry.file;
        if (!exists(path)) {
            continue;
        }
        // Touch the file so the FreeType face is parsed once at startup instead
        // of on the first Label creation (axmol caches faces lazily).
        std::string bytes;
        readBytes(path, bytes);
    }
}

std::string Files::fontFace(const std::string& family, bool bold) {
    for (const auto& entry : kGuiFontFiles) {
        const std::string name = entry.alias;
        if (name.rfind(family, 0) != 0) {
            continue;
        }
        const bool isBold = name.find("Bold") != std::string::npos;
        if (isBold == bold) {
            const std::string path = std::string("fonts/") + entry.file;
            return exists(path) ? path : std::string();
        }
    }
    return {};
}

std::string Files::resolveRelative(const std::string& documentPath, const std::string& ref) {
    if (ref.empty()) {
        return {};
    }

    // Absolute / already-resolved references pass through.
    if (ref[0] == '/' || (ref.size() > 1 && ref[1] == ':')) {
        return ref;
    }

    std::string dir = documentPath;
    const std::size_t slash = dir.find_last_of("/\\");
    dir = slash == std::string::npos ? std::string() : dir.substr(0, slash + 1);

    std::string combined = dir + ref;
    // Normalize `./` and `../` segments.
    std::vector<std::string> parts;
    std::string segment;
    for (std::size_t i = 0; i <= combined.size(); i++) {
        const char c = i < combined.size() ? combined[i] : '/';
        if (c == '/' || c == '\\') {
            if (segment == "..") {
                if (!parts.empty()) {
                    parts.pop_back();
                }
            } else if (!segment.empty() && segment != ".") {
                parts.push_back(segment);
            }
            segment.clear();
        } else {
            segment.push_back(c);
        }
    }
    std::string result;
    for (std::size_t i = 0; i < parts.size(); i++) {
        if (i > 0) {
            result.push_back('/');
        }
        result += parts[i];
    }
    return result;
}

} // namespace ui

// Free-function entry point declared in Ui.h (avoids naming the ambiguous
// `ui::Files` from files that have `using namespace ax`).
namespace surv { namespace ui {
void registerGuiFonts(const char* dir) {
    ::ui::Files::registerFonts(dir ? dir : "");
}
} } // namespace surv::ui
