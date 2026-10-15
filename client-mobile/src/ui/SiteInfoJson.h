#pragma once
// Engine-independent re-encoding of the `/api/site_info` response (net/SiteInfo.h)
// into the JSON snapshot the Java UI reads through `SurvBridge.getSiteInfoJson`
// (ARCHITECTURE.md §5.3).
//
// The encoder lives in this header rather than `SiteInfoJson.cpp`'s own because
// it has no engine dependency at all; keeping the declaration here means the
// host tests (which do not compile the axmol HTTP client) can assert the exact
// document the Java region spinner parses.
#include "../net/SiteInfo.h"

#include <string>

namespace ui {

std::string siteInfoToJson(const surv::SiteInfo& info);

} // namespace ui
