// Host tests for net/ErrorLog.cpp, the port of client/src/errorLogs.ts: the
// request budget/throttle, 1% sampling, the two-uncaught-error cap and the
// exact /api/report_error document. The poster/clock/random are injected.
#include "TestFramework.h"

#include "net/ErrorLog.h"
#include "ui/Json.h"

#include <string>
#include <utility>
#include <vector>

using namespace surv;
using namespace surv_test;

namespace {

struct Recorder {
    std::vector<std::pair<std::string, std::string>> posts;

    ErrorLog::Poster poster() {
        return [this](const std::string& url, const std::string& body) {
            posts.push_back({url, body});
        };
    }
};

struct Fixture {
    Recorder recorder;
    double now = 1000.0;
    double random = 1.0; // never sampled unless a test overrides it

    ErrorLog make() {
        return ErrorLog(recorder.poster(), "http://api.test/", [this] { return now; },
                        [this] { return random; });
    }
};

ui::JsonValue parse(const std::string& json) {
    ui::JsonValue value;
    CHECK(ui::JsonParser::parse(json, value));
    return value;
}

} // namespace

TEST(error_log_url_normalisation) {
    CHECK_EQ(reportErrorUrl("http://api.test"), std::string("http://api.test/api/report_error"));
    CHECK_EQ(reportErrorUrl("http://api.test/"), std::string("http://api.test/api/report_error"));
    CHECK_EQ(reportErrorUrl(""), std::string("/api/report_error"));
}

TEST(error_log_body_shape) {
    CHECK_EQ(ErrorLog::makeBody("errorLog", ""), std::string("{\"loc\":\"errorLog\"}"));
    CHECK_EQ(ErrorLog::makeBody("errorLog", "{\"a\":1}"),
             std::string("{\"loc\":\"errorLog\",\"data\":{\"a\":1}}"));
    // Quotes/newlines in the location must not break the document.
    CHECK_EQ(ErrorLog::makeBody("a\"b\n", ""), std::string("{\"loc\":\"a\\\"b\\n\"}"));
}

TEST(error_log_sends_documents_and_decays_budget) {
    Fixture f;
    ErrorLog log = f.make();
    CHECK_EQ(log.requests(), 0);

    log.logError("boom", "{\"detail\":\"x\"}");
    CHECK_EQ(f.recorder.posts.size(), static_cast<size_t>(1));
    CHECK_EQ(f.recorder.posts[0].first, std::string("http://api.test/api/report_error"));
    CHECK_EQ(log.reportsSent(), 1);

    ui::JsonValue body = parse(f.recorder.posts[0].second);
    CHECK_EQ(body.getString("loc"), std::string("errorLog"));
    CHECK_EQ(body.get("data")->getString("error"), std::string("boom"));
    CHECK_EQ(body.get("data")->get("data")->getString("detail"), std::string("x"));

    // update() decays one request per second while not throttled.
    CHECK_EQ(log.requests(), 1);
    log.tick();
    CHECK_EQ(log.requests(), 0);
    log.tick();
    CHECK_EQ(log.requests(), 0);
}

TEST(error_log_throttles_after_five_reports) {
    Fixture f;
    ErrorLog log = f.make();
    for (int i = 0; i < 5; ++i) {
        log.logError("e" + std::to_string(i));
    }
    CHECK_EQ(f.recorder.posts.size(), static_cast<size_t>(5));
    CHECK(!log.throttled());

    // The sixth report trips the throttle instead of sending.
    log.logError("sixth");
    CHECK(log.throttled());
    CHECK_EQ(f.recorder.posts.size(), static_cast<size_t>(5));
    CHECK_EQ(log.requests(), 6);

    // Inside the 180 s window nothing is sent and tick() keeps the throttle.
    f.now += 179000.0;
    log.tick();
    CHECK(log.throttled());
    // A suppressed report re-arms the window, exactly like errorLogs.ts store().
    log.logError("blocked");
    CHECK(log.throttled());
    CHECK_EQ(f.recorder.posts.size(), static_cast<size_t>(5));

    // Once the re-armed window passes, the budget resets and reports flow again.
    f.now += 180001.0;
    log.tick();
    CHECK(!log.throttled());
    CHECK_EQ(log.requests(), 0);
    log.logError("after");
    CHECK_EQ(f.recorder.posts.size(), static_cast<size_t>(6));
    CHECK_EQ(log.requests(), 1);
}

TEST(error_log_store_generic_sampling) {
    Fixture f;
    ErrorLog log = f.make();

    // "account" errors are always dropped (errorLogs.ts).
    f.random = 0.0;
    log.storeGeneric("account", "load_profile_error");
    CHECK_EQ(f.recorder.posts.size(), static_cast<size_t>(0));

    // Outside the 1% sample: dropped.
    f.random = 0.5;
    log.storeGeneric("menu", "parse_failed");
    CHECK_EQ(f.recorder.posts.size(), static_cast<size_t>(0));
    CHECK_EQ(log.requests(), 0);

    // Inside it: sent with parent/child.
    f.random = 0.005;
    log.storeGeneric("menu", "parse_failed");
    CHECK_EQ(f.recorder.posts.size(), static_cast<size_t>(1));
    ui::JsonValue body = parse(f.recorder.posts[0].second);
    CHECK_EQ(body.getString("loc"), std::string("storeGeneric"));
    CHECK_EQ(body.get("data")->getString("parent"), std::string("menu"));
    CHECK_EQ(body.get("data")->getString("child"), std::string("parse_failed"));
}

TEST(error_log_window_errors_are_capped) {
    Fixture f;
    ErrorLog log = f.make();
    for (int i = 0; i < 4; ++i) {
        log.logWindowOnError("{\"message\":\"crash" + std::to_string(i) + "\"}");
    }
    CHECK_EQ(log.windowErrors(), 2);
    CHECK_EQ(f.recorder.posts.size(), static_cast<size_t>(2));
    ui::JsonValue body = parse(f.recorder.posts[1].second);
    CHECK_EQ(body.getString("loc"), std::string("windowOnError"));
    CHECK_EQ(body.get("data")->getString("message"), std::string("crash1"));
}
