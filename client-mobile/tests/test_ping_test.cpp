// Host tests for net/PingTest.cpp, the port of client/src/pingTest.ts.
//
// The transport is the shared FakeConnection, so the echo cadence, retry and
// completion rules are deterministic (the clock is injected too).
#include "TestFramework.h"

#include "FakeConnection.h"
#include "net/PingTest.h"
#include "net/Regions.h"
#include "ui/Json.h"

#include <memory>
#include <string>
#include <vector>

using namespace surv;
using namespace surv_test;

namespace {

using PopEntryList = std::vector<PingTestEntry>;

// Values that outlive the socket: PingTest destroys the connection when a test
// completes or is stopped, so the counters live in a harness-owned record.
struct ConnRecord {
    std::string url;
    std::vector<std::vector<uint8_t>> sent;
    int closeCount = 0;
};

class TrackedConnection : public FakeConnection {
public:
    TrackedConnection(std::string url, std::shared_ptr<ConnRecord> record)
        : FakeConnection(std::move(url)), record(std::move(record)) {
        this->record->url = this->url;
    }

    void send(const uint8_t* data, size_t len) override {
        FakeConnection::send(data, len);
        record->sent = sent;
    }
    void close(const std::string& reason) override {
        FakeConnection::close(reason);
        record->closeCount = closeCount;
    }

    std::shared_ptr<ConnRecord> record;
};

class PingHarness {
public:
    PingHarness() : ping([this](const std::string& url) {
                        auto record = std::make_shared<ConnRecord>();
                        auto conn = std::make_unique<TrackedConnection>(url, record);
                        conns.push_back(conn.get());
                        records.push_back(std::move(record));
                        return std::unique_ptr<Connection>(std::move(conn));
                    },
                    [this] { return nowMs; }) {}

    std::vector<PingTestEntry> entries() {
        PopEntryList list;
        PingTestEntry na;
        na.region = "na";
        na.zone = "na";
        na.url = "10.0.0.1:9000";
        list.push_back(std::move(na));
        PingTestEntry eu;
        eu.region = "eu";
        eu.zone = "eu-west";
        eu.url = "10.0.0.2:9000";
        eu.https = true;
        list.push_back(std::move(eu));
        PingTestEntry asia;
        asia.region = "as";
        asia.zone = "as";
        asia.url = "10.0.0.3:9000";
        list.push_back(std::move(asia));
        return list;
    }

    // The socket PingTest currently owns for the newest test (never use it
    // after completion/stop: PingTest destroys it).
    FakeConnection* latest() { return conns.empty() ? nullptr : conns.back(); }

    // One echo round trip on `conn`: advance the clock, deliver the frame.
    void echo(PingTest& test, FakeConnection* conn, double latencyMs) {
        nowMs += latencyMs;
        conn->pushFrame({0});
        test.update(0.0f);
    }

    double nowMs = 1000.0;
    std::vector<FakeConnection*> conns;
    std::vector<std::shared_ptr<ConnRecord>> records;
    PingTest ping;
};

} // namespace

TEST(ping_entries_from_json) {
    const auto entries = PingTest::entriesFromJson(
        "[{\"region\":\"na\",\"zone\":\"na\",\"url\":\"1.2.3.4:9000\",\"https\":false},"
        "{\"region\":\"eu\",\"zone\":\"eu-west\",\"address\":\"5.6.7.8:9002\",\"https\":true},"
        "{\"region\":\"\",\"url\":\"9.9.9.9:9000\"},"
        "{\"region\":\"bad\"}]");
    CHECK_EQ(entries.size(), static_cast<size_t>(2));
    CHECK_EQ(entries[0].region, std::string("na"));
    CHECK_EQ(entries[0].zone, std::string("na"));
    CHECK_EQ(entries[0].url, std::string("1.2.3.4:9000"));
    CHECK_EQ(entries[1].url, std::string("5.6.7.8:9002"));
    CHECK(entries[1].https);
    CHECK_EQ(entries[1].zone, std::string("eu-west"));

    // zone defaults to the region (one zone per region on this server).
    const auto missingZone = PingTest::entriesFromJson(
        "[{\"region\":\"sa\",\"url\":\"1.1.1.1:9000\"}]");
    CHECK_EQ(missingZone.size(), static_cast<size_t>(1));
    CHECK_EQ(missingZone[0].zone, std::string("sa"));

    CHECK(PingTest::entriesFromJson("not json").empty());
    CHECK(PingTest::entriesFromJson("[]").empty());
}

TEST(ping_start_only_selected_regions) {
    PingHarness h;
    h.ping.setEntries(h.entries());
    CHECK_EQ(h.ping.regionList().size(), static_cast<size_t>(3));
    CHECK_EQ(h.ping.regionList()[2], std::string("as"));

    h.ping.start({"eu"});
    CHECK_EQ(h.ping.testsStarted(), 1);
    CHECK(!h.ping.tests()[0].active);
    CHECK(h.ping.tests()[1].active);
    CHECK(!h.ping.tests()[2].active);

    // Unknown regions are ignored (the web filters with `regions.includes`).
    h.ping.start({"nope"});
    CHECK_EQ(h.ping.testsStarted(), 1);

    // Re-starting an active region does not double count.
    h.ping.start({"eu"});
    CHECK_EQ(h.ping.testsStarted(), 1);
}

TEST(ping_measures_echo_round_trip) {
    PingHarness h;
    h.ping.setEntries(h.entries());
    h.ping.start({"na"});
    CHECK(!h.ping.isComplete());

    h.ping.update(0.0f);
    CHECK_EQ(h.records.size(), static_cast<size_t>(1));
    CHECK_EQ(h.records[0]->url, std::string("ws://10.0.0.1:9000/ptc"));
    FakeConnection* conn = h.latest();

    // Not open yet: nothing is sent.
    h.ping.update(0.5f);
    CHECK_EQ(conn->sent.size(), static_cast<size_t>(0));

    conn->open();
    h.ping.update(0.01f);
    CHECK_EQ(conn->sent.size(), static_cast<size_t>(1));
    CHECK_EQ(conn->sent[0].size(), static_cast<size_t>(1));

    // Fast echo wins; a later slower one does not raise the minimum.
    h.echo(h.ping, conn, 42.0);
    CHECK_NEAR(h.ping.tests()[0].pingMs, 42.0, 0.001);
    CHECK_EQ(h.ping.tests()[0].recvCount, 1);

    // The 8 Hz cadence: no second send until sendDelay (0.125 s) elapses.
    h.ping.update(0.05f);
    CHECK_EQ(conn->sent.size(), static_cast<size_t>(1));
    h.ping.update(0.1f);
    CHECK_EQ(conn->sent.size(), static_cast<size_t>(2));

    h.echo(h.ping, conn, 21.0);
    CHECK_NEAR(h.ping.tests()[0].pingMs, 21.0, 0.001);

    // recvCountMax (6) completes the test and closes the socket.
    while (h.ping.tests()[0].recvCount < 6) {
        h.ping.update(0.2f);
        h.echo(h.ping, conn, 30.0);
    }
    h.ping.update(0.2f);
    CHECK(h.ping.isComplete());
    CHECK(h.ping.tests()[0].complete);
    CHECK(!h.ping.tests()[0].active);
    CHECK_NEAR(h.ping.tests()[0].pingMs, 21.0, 0.001);
    CHECK_EQ(h.records[0]->closeCount, 1);
}

TEST(ping_retries_once_then_gives_up) {
    PingHarness h;
    h.ping.setEntries(h.entries());
    h.ping.start({"na"});
    h.ping.update(0.0f);

    // First close: retry (retryCount 0 -> 1), a second socket is opened.
    h.latest()->pushClose(1006, "lost");
    h.ping.update(0.0f);
    CHECK_EQ(h.ping.tests()[0].retryCount, 1);
    CHECK(!h.ping.tests()[0].complete);
    h.ping.update(0.0f);
    CHECK_EQ(h.records.size(), static_cast<size_t>(2));

    // Second close: retries exhausted -> the test completes with no ping.
    h.latest()->pushClose(1006, "lost");
    h.ping.update(0.0f);
    CHECK_EQ(h.ping.tests()[0].retryCount, 2);
    CHECK(h.ping.tests()[0].complete);
    CHECK(h.ping.isComplete());
    CHECK_NEAR(h.ping.tests()[0].pingMs, kNoPingMs, 0.001);
    // With no measurement the region still resolves (pingTest.ts getRegion()).
    CHECK_EQ(h.ping.bestRegion(), std::string("na"));
}

TEST(ping_best_region_and_zones) {
    PingHarness h;
    h.ping.setEntries(h.entries());
    h.ping.start({"na", "eu"});

    h.ping.update(0.0f);
    CHECK_EQ(h.records.size(), static_cast<size_t>(2));
    FakeConnection* na = h.conns[0];
    FakeConnection* eu = h.conns[1];
    na->open();
    eu->open();
    h.ping.update(0.01f);
    CHECK_EQ(na->sent.size(), static_cast<size_t>(1));
    CHECK_EQ(eu->sent.size(), static_cast<size_t>(1));

    // eu answers faster even though it is not the first entry.
    h.echo(h.ping, eu, 35.0);
    h.echo(h.ping, na, 165.0);

    CHECK_EQ(h.ping.bestRegion(), std::string("eu"));
    CHECK_NEAR(h.ping.tests()[0].pingMs, 200.0, 0.001);
    CHECK_NEAR(h.ping.tests()[1].pingMs, 35.0, 0.001);
    // The unstarted region keeps the sentinel and never wins.
    CHECK_NEAR(h.ping.tests()[2].pingMs, kNoPingMs, 0.001);

    const auto zones = h.ping.zones("eu");
    CHECK_EQ(zones.size(), static_cast<size_t>(1));
    CHECK_EQ(zones[0], std::string("eu-west"));
    CHECK(h.ping.zones("missing").empty());
}

TEST(ping_results_json) {
    PingHarness h;
    h.ping.setEntries(h.entries());
    h.ping.start({"na"});
    h.ping.update(0.0f);
    FakeConnection* conn = h.latest();
    conn->open();
    h.ping.update(0.01f);
    h.echo(h.ping, conn, 55.0);

    const std::string json = h.ping.resultsJson();
    ui::JsonValue root;
    CHECK(ui::JsonParser::parse(json, root));
    CHECK(!root.getBool("complete"));
    CHECK_EQ(root.getString("bestRegion"), std::string("na"));
    const ui::JsonValue* tests = root.get("tests");
    CHECK(tests != nullptr);
    CHECK_EQ(tests->size(), static_cast<size_t>(3));
    CHECK_EQ(tests->at(0)->getInt("pingMs"), 55);
    CHECK(!tests->at(0)->getBool("complete"));
    CHECK_EQ(tests->at(0)->getString("zone"), std::string("na"));
    // Unmeasured regions serialize as -1.
    CHECK_EQ(tests->at(2)->getInt("pingMs"), -1);
}

TEST(ping_stop_closes_sockets) {
    PingHarness h;
    h.ping.setEntries(h.entries());
    h.ping.start({"na"});
    h.ping.update(0.0f);
    FakeConnection* conn = h.latest();
    conn->open();
    h.ping.update(0.01f);
    CHECK_EQ(h.records[0]->closeCount, 0);

    h.ping.stop();
    CHECK_EQ(h.records[0]->closeCount, 1);
    CHECK(!h.ping.tests()[0].active);
    // stop() does not fake a completion.
    CHECK(!h.ping.isComplete());
    CHECK_EQ(h.ping.testsStarted(), 1);

    // setEntries() resets everything for the next menu visit.
    h.ping.setEntries(h.entries());
    CHECK_EQ(h.ping.testsStarted(), 0);
    CHECK_NEAR(h.ping.tests()[1].pingMs, kNoPingMs, 0.001);
}

// The probe targets are generated from Config.regions (tools/codegen_regions.mjs)
// exactly like the web's PING_TEST_URLS: zone == region and a non-empty url.
TEST(ping_regions_generated_table) {
    const auto regions = pingTestRegions();
    CHECK(!regions.empty());
    for (const auto& region : regions) {
        CHECK(!region.region.empty());
        CHECK_EQ(region.zone, region.region);
        CHECK(!region.url.empty());
        CHECK(!region.active);
        CHECK(!region.complete);
        CHECK_NEAR(region.pingMs, kNoPingMs, 0.001);
    }
}
