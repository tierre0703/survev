// axmol-backed ConfigStorage (see Config.h). Kept in its own translation unit
// so the rest of the UI config stays engine-free for the host tests.
#include "Config.h"

#include "axmol.h"

namespace ui {

UserDefaultConfigStorage::UserDefaultConfigStorage(const std::string& key) : _key(key) {}

std::string UserDefaultConfigStorage::read() {
    // The engine's UserDefault returns a const char* / std::string depending on
    // the version; normalise to std::string.
    const std::string value(
        ax::UserDefault::getInstance()->getStringForKey(_key.c_str(), ""));
    return value;
}

void UserDefaultConfigStorage::write(const std::string& json) {
    ax::UserDefault::getInstance()->setStringForKey(_key.c_str(), json);
    ax::UserDefault::getInstance()->flush();
}

} // namespace ui
