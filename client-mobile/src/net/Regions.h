#pragma once
// The compiled-in region list for the pre-game latency test (client/src/
// pingTest.ts).
//
// The web build bakes `Config.regions` into the bundle as `PING_TEST_URLS`
// (client/vite.config.mts); the native build produces the identical table with
// `tools/codegen_regions.mjs` -> `generatedRegions.inc`. Nothing comes from
// `site_info` (which only carries per-region populations), so no server change
// is involved.
#include "PingTest.h"

#include <vector>

namespace surv {

// One entry per `Config.regions` key: {"region","zone","url","https"} with
// zone == region, exactly like PING_TEST_URLS. Returns a fresh list (the
// entries own their sockets).
std::vector<PingTestEntry> pingTestRegions();

} // namespace surv
