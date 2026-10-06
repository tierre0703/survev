#include "Defs.h"

namespace surv {

static const DefProvider* g_defProvider = nullptr;

const DefProvider* getDefProvider() {
    return g_defProvider;
}

void setDefProvider(const DefProvider* provider) {
    g_defProvider = provider;
}

} // namespace surv
