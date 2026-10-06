#pragma once
// Port of shared/net/connection.ts: the engine-independent Connection
// abstraction. The browser's `WebSocket` is replaced by the axmol adapter in
// WebSocketConnection.h/.cpp; tests and replays use an in-memory implementation
// so the join/pump/dispatch flow can be verified on the host.
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace surv {

// matches WebSocket.readyState (shared/net/connection.ts ConnectionState)
enum class ConnectionState : uint8_t {
    Connecting = 0,
    Open = 1,
    Closing = 2,
    Closed = 3,
};

class Connection {
public:
    // A binary frame as received from the server. Ownership is transferred to
    // the callback so the adapter does not need to keep the buffer alive.
    using MessageCallback = std::function<void(std::vector<uint8_t>&& frame)>;
    using CloseCallback = std::function<void(uint16_t code, const std::string& reason)>;

    virtual ~Connection() = default;

    // Mirrors the connection.ts callbacks. onOpen/onMessage/onClose/onError are
    // always invoked on the main thread (from pump()).
    std::function<void()> onOpen;
    MessageCallback onMessage;
    std::function<void()> onError;
    CloseCallback onClose;

    virtual ConnectionState state() const = 0;
    virtual size_t bufferedAmount() const = 0;
    virtual void send(const uint8_t* data, size_t len) = 0;
    virtual void close(const std::string& reason = "") = 0;

    // Drains incoming frames queued by the network thread and invokes the
    // callbacks in order. MUST be called on the main thread (game update);
    // the web client gets this for free because WebSocket events fire on the
    // main thread, while axmol delivers them through a scheduler.
    virtual void pump() {}

    // Mirrors Connection.resetAndClose(): detach callbacks, then close.
    void resetAndClose() {
        onOpen = nullptr;
        onMessage = nullptr;
        onError = nullptr;
        onClose = nullptr;
        close();
    }
};

// Factory used by Game to create a connection for a join URL. The app passes
// createWebSocketConnection (WebSocketConnection.h); tests pass a fake.
using ConnectionFactory = std::function<std::unique_ptr<Connection>(const std::string& url)>;

} // namespace surv
