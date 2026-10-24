#pragma once
// Port of client/src/pingTest.ts: the pre-game region latency test.
//
// The web client measures the round trip of a 1-byte echo on each region's
// `/ptc` WebSocket (see server/src/gameServer.ts), then auto-selects the closest
// region and sends the region's zones with `find_game`. The native port runs the
// same protocol, but the transport is injected (`ConnectionFactory`), so the
// whole state machine is host-tested against `FakeConnection` and the app passes
// `createWebSocketConnectionTo`.
//
// Difference from the web: `pingTest.ts` keeps `ping` in seconds because it is
// only ever logged; the native menu shows the latency in milliseconds, so this
// port keeps milliseconds (the ordering/minimum logic is identical).
#include "Connection.h"

#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace surv {

// "No measurement yet" (the web's 9999 seconds, in ms).
inline constexpr double kNoPingMs = 9999.0 * 1000.0;

// One PING_TEST_URLS entry (built by the web at compile time from
// `Config.regions`; the native menu derives it from `site_info.pops[...]`).
struct PingTestEntry {
    std::string region;
    std::string zone;
    std::string url; // host:port, no scheme (e.g. "1.2.3.4:9000")
    bool https = false;

    double pingMs = kNoPingMs;
    bool active = false;
    bool complete = false;

    double sendDelay = 0.0;
    double sendTimeMs = 0.0;
    int sendCount = 0;
    int recvCount = 0;
    int recvCountMax = 6;
    int retryCount = 0;
    int retryCountMax = 1;

    // Set from the close/error callback so update() can destroy the socket
    // outside the callback (see PingTest::handleClose).
    bool needReopen = false;

    // Owned by the test entry, recreated on each retry.
    std::unique_ptr<Connection> conn;
};

class PingTest {
public:
    using NowFn = std::function<double()>; // monotonic milliseconds

    // `factory` creates the `/ptc` socket (the app passes
    // createWebSocketConnectionTo; tests pass FakeConnection). Without one,
    // start() is a no-op.
    explicit PingTest(ConnectionFactory factory = {}, NowFn now = {});

    // Replaces the region list and resets every measurement.
    void setEntries(std::vector<PingTestEntry> entries);
    const std::vector<PingTestEntry>& tests() const { return _tests; }

    // Marks the matching regions active (pingTest.ts start()).
    void start(const std::vector<std::string>& regions);
    // Closes the sockets of every active test (menu hidden / match started).
    void stop();
    // Advances the open sockets and the 8 Hz echo cadence (pingTest.ts update()).
    void update(float dt);

    // All started tests finished (isComplete()).
    bool isComplete() const;
    // Number of started tests, for the "measuring..." state.
    int testsStarted() const { return _testsStarted; }

    // Region list in entry order, de-duplicated (getRegionList()).
    std::vector<std::string> regionList() const;
    // Lowest-latency region, or "" when there is nothing to measure.
    std::string bestRegion() const;
    // Zones of `region`, fastest first (getZones()).
    std::vector<std::string> zones(const std::string& region) const;

    // {"complete":bool,"bestRegion":str,"tests":[{"region","zone","pingMs",
    //   "active","complete"}]} -- consumed by the Java menu.
    std::string resultsJson() const;

    // site_info `pops` -> entries. Accepts [{"region","zone","url"|"address",
    // "https"}]; malformed/missing addresses are dropped.
    static std::vector<PingTestEntry> entriesFromJson(const std::string& json);

private:
    void complete(PingTestEntry& test);
    void handleClose(PingTestEntry& test);
    void open(PingTestEntry& test);

    ConnectionFactory _factory;
    NowFn _now;
    std::vector<PingTestEntry> _tests;
    int _testsStarted = 0;
    int _testsCompleted = 0;
};

} // namespace surv
