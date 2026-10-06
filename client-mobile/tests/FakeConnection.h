#pragma once
// In-memory Connection used to drive the Game net flow on the host without
// axmol. Mirrors the observable behaviour of WebsocketConnection: events are
// queued and delivered by pump() on the main thread.
#include "../src/net/Connection.h"
#include <deque>
#include <string>
#include <utility>
#include <vector>

namespace surv_test {

class FakeConnection : public surv::Connection {
public:
    explicit FakeConnection(std::string url) : url(std::move(url)) {}

    surv::ConnectionState state() const override { return _state; }
    size_t bufferedAmount() const override { return _buffered; }

    void send(const uint8_t* data, size_t len) override {
        sent.emplace_back(data, data + len);
    }

    void close(const std::string& reason) override {
        closeCount++;
        closeReason = reason;
        if (_state < surv::ConnectionState::Closing) {
            _state = surv::ConnectionState::Closing;
        }
    }

    void pump() override {
        if (!_openDelivered && _state == surv::ConnectionState::Open) {
            _openDelivered = true;
            if (onOpen) {
                onOpen();
            }
        }
        while (!_incoming.empty()) {
            std::vector<uint8_t> frame = std::move(_incoming.front());
            _incoming.pop_front();
            if (onMessage) {
                onMessage(std::move(frame));
            }
        }
        if (_pendingClose && !_closeDelivered) {
            _closeDelivered = true;
            if (onClose) {
                onClose(_closeCode, _closeReason);
            }
        }
    }

    // --- test drivers ---
    void open() { _state = surv::ConnectionState::Open; }

    void pushFrame(std::vector<uint8_t> frame) { _incoming.push_back(std::move(frame)); }

    void pushClose(uint16_t code, std::string reason) {
        _state = surv::ConnectionState::Closed;
        _pendingClose = true;
        _closeCode = code;
        _closeReason = std::move(reason);
    }

    std::string url;
    std::vector<std::vector<uint8_t>> sent;
    int closeCount = 0;
    std::string closeReason;

private:
    surv::ConnectionState _state = surv::ConnectionState::Connecting;
    size_t _buffered = 0;
    bool _openDelivered = false;
    bool _pendingClose = false;
    bool _closeDelivered = false;
    uint16_t _closeCode = 0;
    std::string _closeReason;
    std::deque<std::vector<uint8_t>> _incoming;
};

} // namespace surv_test
