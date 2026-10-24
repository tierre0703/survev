#include "Regions.h"

namespace surv {

namespace {

// tools/codegen_regions.mjs: one aggregate-initialised entry per region
// ({"region","zone","url","https"}; PingTestEntry's measurement fields keep
// their defaults above).
struct RegionRow {
    const char* region;
    const char* zone;
    const char* url;
    bool https;
};

const RegionRow kRegions[] = {
#include "generatedRegions.inc"
};

} // namespace

std::vector<PingTestEntry> pingTestRegions() {
    std::vector<PingTestEntry> entries;
    entries.reserve(sizeof(kRegions) / sizeof(kRegions[0]));
    for (const auto& row : kRegions) {
        PingTestEntry entry;
        entry.region = row.region;
        entry.zone = row.zone;
        entry.url = row.url;
        entry.https = row.https;
        entries.push_back(std::move(entry));
    }
    return entries;
}

} // namespace surv
