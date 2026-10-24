#pragma once
// Port of client/src/errorLogs.ts: the client-side error reporter.
//
// The web client samples and throttles `/api/report_error` reports (at most five
// before a 180 second cool-off, 1% sampling for `storeGeneric`, at most two
// uncaught-window errors per page). The same budget is applied natively; the
// HTTP transport and the clock are injected so the policy is host-tested and the
// app can post through axmol (`net/Api.h postJson`).
//
// The Java crash handler cannot use this class (the process is dying and the
// game thread may never run again); it posts one report directly through
// `SurvApi.reportError` and deliberately bypasses the budget.
#include <functional>
#include <string>

namespace surv {

// `apiBaseUrl` -> the report endpoint, normalised like net/Api.h findGame()
// (trailing slash dropped, empty base -> the relative path).
std::string reportErrorUrl(const std::string& apiBaseUrl);

class ErrorLog {
public:
    // Sends one request. The app passes `postJson` (axmol HttpClient); tests
    // record the documents instead.
    using Poster = std::function<void(const std::string& url, const std::string& body)>;
    using Clock = std::function<double()>;  // monotonic milliseconds (Date.now())
    using Random = std::function<double()>; // [0, 1)

    ErrorLog(Poster poster, std::string apiBaseUrl, Clock clock = {}, Random random = {});

    // errorLogs.ts update(): decays the request budget, or lifts the throttle.
    // The web runs it from a 1 s interval; GameScene calls it once per second.
    void tick();

    // errorLogs.ts storeGeneric(): 1% sampled; "account" errors are always
    // dropped (they only mean "a network request failed").
    void storeGeneric(const std::string& parent, const std::string& child);

    // errorLogs.ts logWindowOnError(): at most two per process.
    void logWindowOnError(const std::string& errorJson);

    // errorLogs.ts logError(): `dataJson` is an optional JSON value.
    void logError(const std::string& error, const std::string& dataJson = "");

    // Diagnostics (tests + the `[error]` dev trace).
    int requests() const { return _requests; }
    bool throttled() const { return _throttle; }
    int reportsSent() const { return _reportsSent; }
    int windowErrors() const { return _errorLogCount; }
    const std::string& lastBody() const { return _lastBody; }

    // {"loc":...,"data":...} with `dataJson` embedded verbatim when non-empty.
    static std::string makeBody(const std::string& loc, const std::string& dataJson);
    // Minimal JSON string escaping for the documents above.
    static std::string escape(const std::string& text);

private:
    void store(const std::string& loc, const std::string& dataJson);
    bool sample();

    Poster _poster;
    std::string _url;
    Clock _clock;
    Random _random;

    // errorLogs.ts state.
    int _requests = 0;
    bool _throttle = false;
    double _throttleTimeout = 0.0;
    int _errorLogCount = 0;

    int _reportsSent = 0;
    std::string _lastBody;
};

} // namespace surv
