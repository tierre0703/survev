#pragma once
// Native main-menu chrome (StartMenu). This is the helper class for the start
// screen (splash + logo, centred name/region/play block, news box, bottom-right
// settings/mute icons and the settings/help/join modals). It used to be
// declared next to the removed C++ UiOverlay; the implementation lives in
// StartMenu.cpp.
#include "Config.h"
#include "Localization.h"
#include "Ui.h"

#include <functional>
#include <string>
#include <vector>

namespace ax {
class Node;
} // namespace ax

namespace ui {

class StartMenu {
public:
    // Builds the whole menu (background, logo, name/region/play buttons, news
    // box, bottom-right settings/mute icons, modals) under `root`.
    void build(ax::Node* root, Config* config, Localization* loc);
    // Hides all menu chrome (match started) / shows it again (match ended).
    void setVisible(bool visible);
    void setTeamVisible(bool team);
    void refresh();
    // Enables/disables one of the Play Solo/Duo/Squad buttons (mode 0/1/2).
    void setPlayEnabled(int index, bool enabled);
    // Targeted refresh hooks used by the scene (each widget owns its own
    // renderer, so the scene only updates what changed).
    void setRegionLabel(const std::string& label);
    void setMuteState(bool muted);
    void setLanguageLabel(const std::string& label);
    // The invite code collected by the join-team modal (consumed by the scene).
    const std::string& pendingRoomCode() const;
    // Localized status line (menu errors / "Finding game...").
    void setError(const std::string& l10nKey, const std::string& fallback);
    void clearError();

    std::function<void(int teamMode)> onQuickStart;
    std::function<void(bool create)> onTeamRequested;

private:
    void buildPanel();
    void buildNewsBox();
    void buildBottomIcons();
    void buildSettingsModal();
    void buildHelpModal();
    void buildJoinModal();
    void requestTeam(bool create);

    Config* _config = nullptr;
    Localization* _loc = nullptr;
    ax::Node* _root = nullptr;
    ax::Sprite* _background = nullptr;
    ax::LayerColor* _overlayDim = nullptr;
    ax::Sprite* _logo = nullptr;
    Panel* _panel = nullptr;
    TextField* _nameField = nullptr;
    Button* _regionBtn = nullptr;
    Button* _playBtns[3] = {nullptr, nullptr, nullptr};
    Button* _joinTeamBtn = nullptr;
    Button* _createTeamBtn = nullptr;
    Button* _customizeBtn = nullptr;
    Button* _helpBtn = nullptr;
    Button* _settingsBtn = nullptr;
    Button* _muteBtn = nullptr;
    Panel* _newsPanel = nullptr;
    ax::Label* _errorLabel = nullptr;
    Modal _settingsModal;
    Modal _helpModal;
    Modal _joinModal;
    TextField* _roomField = nullptr;
    Button* _languageBtn = nullptr;
    Button* _modalMuteBtn = nullptr;
    std::vector<Slider*> _sliders;
    std::string _pendingRoomCode;
    bool _visible = true;
};

} // namespace ui
