#include "PingTest.h"

#include "../ui/Json.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>

namespace surv {

namespace {

double steadyNowMs() {
    using namespace std::chrono;
    return duration<double, std::milli>(steady_clock::now().time_since_epoch()).count();
}

// pingTest.ts hard-codes `sendDelay = 0.125` after every echo.
constexpr double kEchoDelay = 0.125;

// Minimal string escaping for the JSON snapshot (regions/zones are plain
// ASCII identifiers, but never build documents by concatenating raw input).
std::string escapeJson(const std::string& text) {
    std::string out;
    out.reserve(text.size() + 2);
    for (char c : text) {
        switch (c) {
            case '"': out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\t': out += "\\t"; break;
            default:
                if (static_cast<unsigned char>(c) < 0x20) {
                    char buf[7];
                    std::snprintf(buf, sizeof(buf), "\\u%04x", static_cast<unsigned>(c));
                    out += buf;
                } else {
                    out += c;
                }
                break;
        }
    }
    return out;
}

} // namespace

PingTest::PingTest(ConnectionFactory factory, NowFn now)
    : _factory(std::move(factory)), _now(now ? std::move(now) : NowFn(steadyNowMs)) {}

void PingTest::setEntries(std::vector<PingTestEntry> entries) {
    stop();
    _tests = std::move(entries);
    _testsStarted = 0;
    _testsCompleted = 0;
}

void PingTest::start(const std::vector<std::string>& regions) {
    if (!_factory) {
        return;
    }
    for (auto& test : _tests) {
        if (test.active || test.complete) {
            continue;
        }
        if (std::find(regions.begin(), regions.end(), test.region) == regions.end()) {
            continue;
        }
        test.active = true;
        ++_testsStarted;
    }
}

void PingTest::stop() {
    for (auto& test : _tests) {
        if (test.conn) {
            test.conn->close();
            test.conn.reset();
        }
        test.active = false;
        test.sendDelay = 0.0;
        test.sendCount = 0;
        test.recvCount = 0;
    }
}

void PingTest::open(PingTestEntry& test) {
    // pingTest.ts: `new WebSocket(\`ws${https ? "s" : ""}://${url}/ptc\`)`.
    const std::string url =
        std::string(test.https ? "wss://" : "ws://") + test.url + "/ptc";
    test.conn = _factory ? _factory(url) : nullptr;
    test.sendDelay = 0.0;
    test.sendCount = 0;
    test.recvCount = 0;
    if (!test.conn) {
        return;
    }
    // The callbacks never outlive the entry: `stop()`/`setEntries()` destroy the
    // connection, and events are only delivered from update() -> pump().
    test.conn->onMessage = [this, &test](std::vector<uint8_t>&&) {
        const double elapsed = _now() - test.sendTimeMs;
        test.pingMs = std::min(test.pingMs, elapsed);
        ++test.recvCount;
        test.sendDelay = kEchoDelay;
    };
    test.conn->onError = [this, &test] { handleClose(test); };
    test.conn->onClose = [this, &test](uint16_t, const std::string&) {
        handleClose(test);
    };
}

void PingTest::complete(PingTestEntry& test) {
    test.active = false;
    test.complete = true;
    ++_testsCompleted;
}

void PingTest::handleClose(PingTestEntry& test) {
    if (test.complete) {
        return;
    }
    if (test.retryCount++ >= test.retryCountMax) {
        complete(test);
    }
    // The socket is closed/destroyed by update() once pump() has returned:
    // doing it from inside the Connection's own onClose callback would free the
    // object whose member function is still running.
    test.needReopen = true;
}

void PingTest::update(float dt) {
    for (auto& test : _tests) {
        if (!test.active) {
            continue;
        }
        if (!test.conn) {
            open(test);
            if (!test.conn) {
                // No transport available yet: try again next frame.
                continue;
            }
        }
        test.conn->pump();
        if (test.needReopen) {
            test.needReopen = false;
            if (test.conn) {
                test.conn->close();
                test.conn.reset();
            }
            continue;
        }
        if (!test.conn) {
            // The pump delivered a close/error and cleared the socket.
            continue;
        }
        if (test.conn->state() != ConnectionState::Open) {
            continue;
        }
        // pingTest.ts: one echo in flight at a time, 8 Hz.
        test.sendDelay -= dt;
        if (test.sendCount == test.recvCount && test.sendDelay < 0.0) {
            test.sendTimeMs = _now();
            ++test.sendCount;
            const uint8_t echo = 0;
            test.conn->send(&echo, 1);
        }
        if (test.recvCount >= test.recvCountMax) {
            complete(test);
            if (test.conn) {
                test.conn->close();
                test.conn.reset();
            }
        }
    }
}

bool PingTest::isComplete() const {
    return _testsCompleted == _testsStarted && _testsStarted > 0;
}

std::vector<std::string> PingTest::regionList() const {
    std::vector<std::string> regions;
    for (const auto& test : _tests) {
        if (std::find(regions.begin(), regions.end(), test.region) == regions.end()) {
            regions.push_back(test.region);
        }
    }
    return regions;
}

std::string PingTest::bestRegion() const {
    const PingTestEntry* best = nullptr;
    for (const auto& test : _tests) {
        // Mirror the web's `sort((a, b) => a.ping - b.ping)` + `tests[0]`:
        // stable ties keep the earlier entry, and failed tests (kNoPingMs)
        // only win when nothing responded.
        if (!best || test.pingMs < best->pingMs) {
            best = &test;
        }
    }
    return best ? best->region : std::string();
}

std::vector<std::string> PingTest::zones(const std::string& region) const {
    std::vector<const PingTestEntry*> matches;
    for (const auto& test : _tests) {
        if (test.region == region) {
            matches.push_back(&test);
        }
    }
    std::stable_sort(matches.begin(), matches.end(), [](const auto* a, const auto* b) {
        return a->pingMs < b->pingMs;
    });
    std::vector<std::string> out;
    out.reserve(matches.size());
    for (const auto* match : matches) {
        out.push_back(match->zone);
    }
    return out;
}

std::string PingTest::resultsJson() const {
    std::string json = "{\"complete\":";
    json += isComplete() ? "true" : "false";
    json += ",\"bestRegion\":\"";
    json += escapeJson(bestRegion());
    json += "\",\"tests\":[";
    for (std::size_t i = 0; i < _tests.size(); ++i) {
        const auto& test = _tests[i];
        if (i != 0) {
            json += ',';
        }
        json += "{\"region\":\"";
        json += escapeJson(test.region);
        json += "\",\"zone\":\"";
        json += escapeJson(test.zone);
        // -1 = no measurement (the web's 9999 s sentinel).
        json += "\",\"pingMs\":";
        json += test.pingMs >= kNoPingMs ? std::string("-1")
                                         : std::to_string(std::lround(test.pingMs));
        json += ",\"active\":";
        json += test.active ? "true" : "false";
        json += ",\"complete\":";
        json += test.complete ? "true" : "false";
        json += '}';
    }
    json += "]}";
    return json;
}

std::vector<PingTestEntry> PingTest::entriesFromJson(const std::string& json) {
    std::vector<PingTestEntry> entries;
    ui::JsonValue root;
    if (!ui::JsonParser::parse(json, root)) {
        return entries;
    }
    const ui::JsonValue* list = &root;
    if (root.isObject()) {
        // Tolerate the snapshot shape too, so a caller can round-trip it.
        const ui::JsonValue* tests = root.get("tests");
        if (!tests) {
            return entries;
        }
        list = tests;
    }
    if (!list->isArray()) {
        return entries;
    }
    for (const auto& item : list->array) {
        if (!item.isObject()) {
            continue;
        }
        PingTestEntry entry;
        entry.region = item.getString("region");
        entry.zone = item.getString("zone", entry.region);
        entry.url = item.getString("url");
        if (entry.url.empty()) {
            entry.url = item.getString("address");
        }
        entry.https = item.getBool("https", false);
        if (entry.region.empty() || entry.url.empty()) {
            continue;
        }
        entries.push_back(std::move(entry));
    }
    return entries;
}

} // namespace surv
