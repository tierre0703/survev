// axmol WebSocket adapter for the engine-independent Connection interface.
// Mirrors shared/net/connection.ts (WebsocketConnection): binary frames, and
// onOpen/onMessage/onClose/onError delivered on the main thread.
#include "WebSocketConnection.h"

#include "axmol.h"
#include "network/WebSocket.h"

#include <deque>
#include <mutex>
#include <utility>

namespace surv {
namespace {

using ax::network::WebSocket;

// Thread-safe queue of WebSocket events. The axmol Delegate callbacks may fire
// from the scheduler thread; Game::update() calls pump() on the main thread to
// deliver them in order (the web client gets this for free because browser
// WebSocket events already run on the main thread).
class WebSocketConnectionImpl : public Connection, public WebSocket::Delegate {
public:
    explicit WebSocketConnectionImpl(const std::string& url) {
        _ws = std::make_unique<WebSocket>();
        if (!_ws->open(this, url)) {
            _ws.reset();
            // Surface the failure through the normal event path so Game sees it.
            std::lock_guard<std::mutex> lock(_mutex);
            _events.push_back(Event::makeClose(1006, "open_failed"));
        }
    }

    ~WebSocketConnectionImpl() override {
        // Dropping the WebSocket purges pending events and stops its io thread,
        // so no callback can re-enter a half-destroyed object.
        _ws.reset();
    }

    ConnectionState state() const override {
        if (!_ws) {
            return ConnectionState::Closed;
        }
        switch (_ws->getReadyState()) {
            case WebSocket::State::CONNECTING:
                return ConnectionState::Connecting;
            case WebSocket::State::OPEN:
                return ConnectionState::Open;
            case WebSocket::State::CLOSING:
                return ConnectionState::Closing;
            case WebSocket::State::CLOSED:
            default:
                return ConnectionState::Closed;
        }
    }

    size_t bufferedAmount() const override { return 0; }

    void send(const uint8_t* data, size_t len) override {
        if (_ws && _ws->getReadyState() == WebSocket::State::OPEN) {
            _ws->send(data, static_cast<unsigned int>(len));
        }
    }

    void close(const std::string& reason) override {
        if (!_ws) {
            return;
        }
        // connection.ts: ws.close(3000, reason) when a reason is supplied.
        if (reason.empty()) {
            _ws->closeAsync();
        } else {
            _ws->closeAsync(3000, reason);
        }
    }

    void pump() override {
        std::deque<Event> events;
        {
            std::lock_guard<std::mutex> lock(_mutex);
            events.swap(_events);
        }
        for (auto& ev : events) {
            switch (ev.kind) {
                case Event::Kind::Open:
                    if (this->Connection::onOpen) this->Connection::onOpen();
                    break;
                case Event::Kind::Message:
                    if (this->Connection::onMessage) this->Connection::onMessage(std::move(ev.data));
                    break;
                case Event::Kind::Error:
                    if (this->Connection::onError) this->Connection::onError();
                    break;
                case Event::Kind::Close:
                    if (this->Connection::onClose) this->Connection::onClose(ev.code, ev.reason);
                    break;
            }
        }
    }

    // --- WebSocket::Delegate (network/scheduler thread) ---
    void onOpen(WebSocket* /*ws*/) override {
        std::lock_guard<std::mutex> lock(_mutex);
        _events.push_back(Event::makeOpen());
    }

    void onMessage(WebSocket* /*ws*/, const WebSocket::Data& data) override {
        std::vector<uint8_t> frame;
        if (data.bytes && data.len > 0) {
            frame.assign(reinterpret_cast<const uint8_t*>(data.bytes),
                         reinterpret_cast<const uint8_t*>(data.bytes) + data.len);
        }
        std::lock_guard<std::mutex> lock(_mutex);
        _events.push_back(Event::makeMessage(std::move(frame)));
    }

    void onClose(WebSocket* /*ws*/, uint16_t code, std::string_view reason) override {
        std::lock_guard<std::mutex> lock(_mutex);
        _events.push_back(Event::makeClose(code, std::string(reason)));
    }

    void onError(WebSocket* /*ws*/, const WebSocket::ErrorCode& /*error*/) override {
        std::lock_guard<std::mutex> lock(_mutex);
        _events.push_back(Event::makeError());
    }

private:
    struct Event {
        enum class Kind { Open, Message, Error, Close };

        Kind kind = Kind::Error;
        std::vector<uint8_t> data;
        uint16_t code = 0;
        std::string reason;

        static Event makeOpen() { return Event{Kind::Open, {}, 0, {}}; }
        static Event makeError() { return Event{Kind::Error, {}, 0, {}}; }
        static Event makeMessage(std::vector<uint8_t>&& d) {
            return Event{Kind::Message, std::move(d), 0, {}};
        }
        static Event makeClose(uint16_t c, std::string r) {
            return Event{Kind::Close, {}, c, std::move(r)};
        }
    };

    std::unique_ptr<WebSocket> _ws;
    mutable std::mutex _mutex;
    std::deque<Event> _events;
};

} // namespace

std::unique_ptr<Connection> createWebSocketConnection(const std::string& url) {
    if (url.empty()) {
        return nullptr;
    }
    return std::make_unique<WebSocketConnectionImpl>(url);
}

} // namespace surv
