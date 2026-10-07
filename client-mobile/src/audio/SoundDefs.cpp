#include "SoundDefs.h"

namespace surv {
namespace audio {

static const SoundDefProvider* g_soundDefProvider = nullptr;

const SoundDefProvider* getSoundDefProvider() {
    return g_soundDefProvider;
}

void setSoundDefProvider(const SoundDefProvider* provider) {
    g_soundDefProvider = provider;
}

} // namespace audio
} // namespace surv
