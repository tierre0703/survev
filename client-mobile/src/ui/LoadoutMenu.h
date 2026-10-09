#pragma once
#include "Loadout.h"
#include "Ui.h"
#include <memory>

namespace ui {
// Native counterpart of client/src/ui/loadoutMenu.ts. GUI-less emotes use text.
class LoadoutMenu {
public:
    void build(ax::Node* root, Config* config, Localization* loc);
    void show();
    bool isOpen() const { return _modal.isOpen(); }
    void hide() { _modal.hide(); }

private:
    void refresh();
    Modal _modal;
    std::unique_ptr<Loadout> _loadout;
    Localization* _loc = nullptr;
    ax::Node* _itemsRoot = nullptr;
    ax::Label* _selection = nullptr;
    Button* _slotButton = nullptr;
    Button* _previous = nullptr;
    Button* _next = nullptr;
    Button* _catButtons[6] = {nullptr, nullptr, nullptr, nullptr, nullptr, nullptr};
    std::string _category = "outfit";
    size_t _page = 0;
    size_t _emoteSlot = 0;
};
} // namespace ui
