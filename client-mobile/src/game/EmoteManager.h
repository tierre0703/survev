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

namespace audio { class AudioManager; }
namespace surv {

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
    bool mapEvent = false;
    bool worldDisplay = true;
    pix::Container* pingContainer = nullptr;
    pix::Container* indContainer = nullptr;
    pix::Sprite* borderSprite = nullptr;
    pix::Sprite* pingSprite = nullptr;
    pix::Sprite* indSpriteOuter = nullptr;
    pix::Sprite* indSpriteInner = nullptr;
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

private:
    static constexpr int kPingIndicatorCount = 6; // 4 team + airdrop + airstrike
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
    int _zIdxNext = 0;

    // Outgoing queue
    std::vector<OutgoingEmote> _outgoing;
};

} // namespace surv