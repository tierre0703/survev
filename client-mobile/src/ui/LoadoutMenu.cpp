#include "LoadoutMenu.h"
#include "Files.h"
#include "../render/Defs.h"
#include <algorithm>
#include <sstream>

namespace ui {

namespace {
std::string tr(Localization* loc, const std::string& key, const std::string& fallback) {
    const std::string text = loc ? loc->translate(key) : "";
    return text.empty() ? fallback : text;
}
} // namespace

void LoadoutMenu::build(ax::Node* root, Config* config, Localization* loc) {
    _loadout = std::make_unique<Loadout>(config);
    _loc = loc;
    // Web `#modal-customize-wrapper`: a 940x600 two/three-pane modal. The native
    // port uses the same 1000x600 footprint as the other modals and stacks the
    // category strip, the item grid and the footer.
    auto* panel = _modal.build(1000, 600, tr(loc, "loadout-title-outfit", "Customize"));
    _modal.setCloseLabel(tr(loc, "index-back-to-main", "Back"));
    root->addChild(_modal.overlay(), 100);

    // Category strip (web `#modal-customize-header` tabs, in loadoutMenu.ts
    // order). "player_icon" is the standalone tab on the far right.
    const char* categories[] = {"outfit", "melee", "emote", "heal", "boost", "player_icon"};
    for (int i = 0; i < 6; ++i) {
        const std::string category = categories[i];
        _catButtons[i] = Button::create(tr(loc, "loadout-title-" + category, category), 150, 44);
        _catButtons[i]->setFontSize(15);
        panel->addChild(_catButtons[i]);
        _catButtons[i]->setCssPosition(20 + i * 160, 52);
        _catButtons[i]->onClick = [this, category] { _category = category; _page = 0; refresh(); };
    }
    // Selected item name (web `#modal-customize-item-name`).
    _selection = makeLabel("", 18, true);
    _selection->setAnchorPoint(ax::Vec2(0.5f, 0.5f));
    _selection->setPosition(ax::Vec2(500, 470));
    panel->addChild(_selection);

    _itemsRoot = ax::Node::create();
    _itemsRoot->setContentSize(ax::Size(1000, 600));
    panel->addChild(_itemsRoot);

    // Emote slot selector (web emote wheel -> a compact text control here).
    _slotButton = Button::create("", 460, 40);
    _slotButton->setColors(ax::Color3B(122, 122, 122), ax::Color3B(62, 62, 62));
    _slotButton->setFontSize(15);
    panel->addChild(_slotButton);
    _slotButton->setCssPosition(270, 500);
    _slotButton->onClick = [this] {
        const size_t slots = _loadout->validated().get("emotes")->array.size();
        if (slots) {
            _emoteSlot = (_emoteSlot + 1) % slots;
        }
        refresh();
    };

    // Paging (web uses a scrollable `#modal-customize-list`).
    _previous = Button::create("<", 100, 40);
    _previous->setColors(ax::Color3B(122, 122, 122), ax::Color3B(62, 62, 62));
    panel->addChild(_previous);
    _previous->setCssPosition(20, 550);
    _previous->onClick = [this] {
        if (_page) {
            --_page;
            refresh();
        }
    };
    _next = Button::create(">", 100, 40);
    _next->setColors(ax::Color3B(122, 122, 122), ax::Color3B(62, 62, 62));
    panel->addChild(_next);
    _next->setCssPosition(880, 550);
    _next->onClick = [this] {
        ++_page;
        refresh();
    };
}

void LoadoutMenu::show() {
    if (_modal.isOpen()) {
        return;
    }
    // Load the same atlas frame names used by player/loot rendering, before a map exists.
    for (const auto* atlas : {"shared", "loadout"}) {
        std::istringstream index(Files::readText(std::string("atlas/") + atlas + "/index.txt"));
        std::string file;
        while (std::getline(index, file)) {
            if (!file.empty() && file.back() == '\r') file.pop_back();
            if (!file.empty()) ax::SpriteFrameCache::getInstance()->addSpriteFramesWithFile(file);
        }
    }
    refresh();
    _modal.show();
}

void LoadoutMenu::refresh() {
    _itemsRoot->removeAllChildren();
    auto items = _loadout->items(_category);
    if (_category == "player_icon") items.insert(items.begin(), "");
    constexpr size_t pageSize = 12;
    const size_t pages = std::max(size_t(1), (items.size() + pageSize - 1) / pageSize);
    _page = std::min(_page, pages - 1);
    _previous->setEnabled(_page > 0);
    _next->setEnabled(_page + 1 < pages);

    // Highlight the active category tab (web `.modal-customize-cat-selected`).
    const char* categories[] = {"outfit", "melee", "emote", "heal", "boost", "player_icon"};
    for (int i = 0; i < 6; ++i) {
        if (!_catButtons[i]) continue;
        const bool selected = _category == categories[i];
        _catButtons[i]->setToggle(selected);
    }

    const auto selected = _loadout->validated();
    const auto value = _category == "emote" ? selected.get("emotes")->at(_emoteSlot)->asString()
                                            : selected.getString(_category);
    const std::string display = value.empty() ? tr(_loc, "loadout-stock", "Stock")
                                              : tr(_loc, "game-" + value, value);
    _selection->setString(tr(_loc, "loadout-category", "Category") + ": " + _category + "  |  " + display);

    _slotButton->setVisible(_category == "emote");
    _slotButton->setLabel(tr(_loc, "loadout-title-emote", "Emotes") + " "
        + std::to_string(_emoteSlot + 1) + "/"
        + std::to_string(_loadout->validated().get("emotes")->array.size()));

    const auto* provider = surv::getDefProvider();
    for (size_t i = _page * pageSize; i < items.size() && i < (_page + 1) * pageSize; ++i) {
        const auto type = items[i];
        const auto cell = i % pageSize;
        auto* button = Button::create(type.empty() ? tr(_loc, "loadout-stock", "Stock")
                                                   : tr(_loc, "game-" + type, type),
                                      225, 105);
        button->setFontSize(14);
        // Cells are tightly packed; keep the system font so long emote/outfit
        // names do not overflow the box (the modal stays on the web metrics).
        button->setBulkFont(true);
        button->setColors(ax::Color3B(60, 60, 60), ax::Color3B(40, 40, 40));
        button->showBadge(type == value);
        _itemsRoot->addChild(button);
        button->setCssPosition(25 + (cell % 4) * 240, 165 + (cell / 4) * 112);
        button->onClick = [this, type] {
            _loadout->select(_category, type, _emoteSlot);
            refresh();
        };
        const auto* def = provider ? provider->gameObject(type) : nullptr;
        if (!def) continue;
        const auto frame = _category == "outfit" ? def->skin.baseSprite : def->img.sprite;
        if (frame.empty() || !ax::SpriteFrameCache::getInstance()->getSpriteFrameByName(frame)) continue;
        const uint32_t colors[] = {def->skin.baseTint, def->skin.baseTintRed, def->skin.baseTintBlue};
        const int count = _category == "outfit" ? 3 : 1;
        for (int variant = 0; variant < count; ++variant) {
            auto* sprite = ax::Sprite::createWithSpriteFrameName(frame);
            const auto size = sprite->getContentSize();
            sprite->setScale(38 / std::max(size.width, size.height));
            sprite->setPosition(ax::Vec2(80 + variant * 34, 80));
            const uint32_t tint = count == 3 ? colors[variant] : def->img.tint;
            sprite->setColor(ax::Color3B((tint >> 16) & 255, (tint >> 8) & 255, tint & 255));
            button->addChild(sprite);
        }
    }
}

} // namespace ui
