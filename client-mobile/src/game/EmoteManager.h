#pragma once
// Emote/ping system for the C++ mobile port.
// Port of client/src/emote.ts: world-space floating emotes, team ping indicators,
// map pings, send/receive from server.
#include "../net/Messages.h"
#include "../render/Defs.h"
#include "../render/Camera.h"
#include "../render/PixiLike.h"
#include <cstdint>
#include <memory>
#include <string>
#include <vector>
#include <unordered_map>

namespace surv {

namespace audio {
class AudioManager;
}

class GameWorld;

// A floating emote sprite above a player (port of emote.ts addEmote).
struct EmoteSprite {
    bool alive = false;
    bool isNew = false;
    uint16_t playerId = 0;
    std::string type;
    Vec2 pos;
    float lifeIn = 0.75f;
    float life = 1.0f;
    float lifeOut = 0.1f;
    pix::Container* container = nullptr;
    pix::Sprite* circleOuter = nullptr;
    pix::Sprite* sprite = nullptr;
    float baseScale = 0.55f;
    int zIdx = 0;
};

// A team ping indicator in the world (port of emote.ts Indicator).
struct PingIndicator {
    bool displayed = false;
    float fadeIn = 0.0f;
    float life = 0.0f;
    float fadeOut = 0.0f;
    Vec2 pos;
    uint16_t playerId = 0;
    bool mapEvent = false;
    bool worldDisplay = true;
    pix::Container* pingContainer = nullptr;
    pix::Container* indContainer = nullptr;
    pix::Sprite* borderSprite = nullptr;
    pix::Sprite* pingSprite = nullptr;
    pix::Sprite* indSpriteOuter = nullptr;
    pix::Sprite* indSpriteInner = nullptr;
};

// A minimap ping sprite (client/src/ui/ui.ts createPing + objects/mapSprite.ts):
// the ping def's map texture plus the expanding pulse ring. Hud draws these
// over the baked minimap; EmoteManager owns the lifetimes. `scale` carries the
// web's raw createPing scale (map events 0.3, player pings 0.15).
struct MapPing {
    bool active = true;
    bool pulse = false;
    std::string texture;
    Vec2 pos;
    uint16_t playerId = 0;
    uint32_t tint = 0xffffff;
    float scale = 0.2f;
    float life = 0.0f;
    float maxLife = 0.0f;
    float ticker = 0.0f;
};

class EmoteManager {
public:
    EmoteManager();
    ~EmoteManager();

    // Called when an emote or ping arrives via UpdateMsg
    void addPing(const Emote& msg, bool factionMode, GameWorld& ctx);
    void addEmote(const Emote& msg, GameWorld& ctx);

    // Per-frame: update emotes, indicators, send queued pings/emotes
    void update(float dt, GameWorld& ctx);

    // Queue emote/ping for sending to server (called from input)
    void queueEmote(const std::string& type, const Vec2& pos);
    void queuePing(const std::string& type, const Vec2& pos);

    // Outgoing queue consumed by Game::update() (mirrors game.ts newPings/newEmotes)
    struct OutgoingEmote {
        std::string type;
        Vec2 pos;
        bool isPing;
    };
    std::vector<OutgoingEmote> consumeOutgoing();

    // Reset (match end / free)
    void reset();

    // Test/diagnostic access to the pooled world indicators.
    const PingIndicator* indicator(int idx) const {
        return (idx >= 0 && idx < kPingIndicatorCount) ? &_indicators[static_cast<size_t>(idx)] : nullptr;
    }
    // Active minimap ping sprites, drawn by game/Hud.
    const std::vector<MapPing>& mapPings() const { return _mapPings; }

private:
    static constexpr int kPingIndicatorCount = 6; // 4 team + airdrop + airstrike
    // World ping indicator scales (emote.ts baseScale 0.4/0.5, shrunk so the
    // airdrop direction icon + arrow stays small; see the mockup HUD note).
    static constexpr float kPingIconScale = 0.22f;
    static constexpr float kPingEdgeScale = 0.28f;
    static constexpr float kEmoteLifeIn = 0.75f;
    static constexpr float kEmoteLife = 1.0f;
    static constexpr float kEmoteLifeOut = 0.1f;
    static constexpr float kPingFadeIn = 0.5f;
    static constexpr float kPingLife = 4.25f;
    static constexpr float kPingFadeOut = 0.1f;

    EmoteSprite* allocEmote();
    void freeEmote(EmoteSprite* e);

    std::vector<EmoteSprite> _emotePool;
    std::array<PingIndicator, kPingIndicatorCount> _indicators{};
    std::vector<MapPing> _mapPings;
    int _zIdxNext = 0;

    // Outgoing queue
    std::vector<OutgoingEmote> _outgoing;
};

} // namespace surv