#include "Defs.h"

#include <cstdlib>

namespace surv {

float RangeDef::random() const {
    if (isConstant || min == max) {
        return min;
    }
    return min + (max - min) * (static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX));
}

static const DefProvider* g_defProvider = nullptr;

const DefProvider* getDefProvider() {
    return g_defProvider;
}

void setDefProvider(const DefProvider* provider) {
    g_defProvider = provider;
}

} // namespace surv
