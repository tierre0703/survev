#pragma once
// FairyGUI emote wheel (M7/S2): radial menu for selecting and sending emotes/pings.
// Mirrors the web client's emote.ts EmoteBarn emote wheel: a center-close button
// with 4 directional wedges (top/right/bottom/left), selected by angle from
// the touch position relative to the wheel center.
#include <array>
#include <string>
#include <vector>

namespace ax {
class Scene;
}

namespace fairygui {
class GBasicTextField;
class GComponent;
class GGraph;
}

namespace surv {

// A single wedge of the emote wheel (or the center close button).
struct EmoteWedge {
    bool isClose = false;
    std::string emoteType;  // empty for ping-only wedges
    std::string pingType;   // "ping_danger", "ping_airdrop", etc.
    std::string label;      // short label text
    uint32_t color = 0xffffff; // display tint

    // Angular range (radians, clockwise from top = -pi/2)
    float angleA = 0.0f;
    float angleC = 0.0f;

    // FGUI widgets (built lazily)
    fairygui::GGraph* bgPolygon = nullptr;
    fairygui::GGraph* highlight = nullptr;
    fairygui::GBasicTextField* labelText = nullptr;
    bool highlighted = false;
};

// Input/output for the emote wheel: one per frame from the touch system.
struct EmoteWheelInput {
    // Set externally each frame:
    bool triggered = false;     // the emote button is being held
    float screenX = 0.0f;      // trigger position (screen coords)
    float screenY = 0.0f;
    bool touched = false;       // true if there is an active touch on the wheel area

    // The emote loadout (from JoinedMsg). 4 emote type names.
    std::array<std::string, 4> loadout = {};

    // Output: the selection to send.
    std::string selectedEmote;  // set to the emote type on release
    std::string selectedPing;   // set to the ping type on release
    float worldX = 0.0f;        // world-space position for the ping
    float worldY = 0.0f;
    bool send = false;          // true when the player released on a wedge
};

// FairyGUI radial emote/ping wheel.
// Created once and shown/hidden on trigger. Receives input from GameScene
// and produces output via EmoteWheelInput.
class FguiEmoteWheel {
public:
    FguiEmoteWheel();
    ~FguiEmoteWheel();

    // Build FGUI widgets under the scene's FG root.
    void build(ax::Scene* scene);

    // Per-frame: read the input state and produce output.
    // Returns true if the wheel consumed the touch (so the aim/move pads
    // should not process the same touch).
    bool update(EmoteWheelInput& io, float dt);

    // Show/hide the wheel.
    void setVisible(bool visible);
    bool isVisible() const;

private:
    void buildWedges();
    void buildCenterClose();
    void buildWedge(int index, const std::string& label, uint32_t color,
                    float angleA, float angleC);
    int hitTestWedge(float dx, float dy) const;
    void highlightWedge(int index);

    // Owning reference (build() retains, ~FguiEmoteWheel releases). GRoot and
    // GComponent do not retain their children, so this is what keeps the wheel
    // widgets alive between frames; wedges are children of _container after
    // the addChild()/release() in build().
    fairygui::GComponent* _container = nullptr;
    std::array<EmoteWedge, 5> _wedges{}; // indices 0-3 = T/R/B/L, 4 = center close

    // Internal state
    bool _visible = false;
    float _animTicker = 0.0f;
    int _highlightedWedge = -1;
    float _centerX = 0.0f;
    float _centerY = 0.0f;
};

} // namespace surv