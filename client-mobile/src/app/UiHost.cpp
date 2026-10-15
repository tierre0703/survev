#include "UiHost.h"

// `installUiBridge` is defined in the Android-only UiBridge.cpp on Android and
// here for every other platform; the guard keeps exactly one definition.
#if (AX_TARGET_PLATFORM != AX_PLATFORM_ANDROID)
#include "axmol.h"
#endif

namespace surv {

namespace {

// Process-wide slots. Installed once at startup (AppDelegate) and never
// replaced afterwards, so plain pointers are enough and no locking is needed:
// every access happens on the main/GL thread (ARCHITECTURE.md §4.2).
UiBridge* g_bridge = nullptr;
UiHost* g_host = nullptr;

NullUiBridge& nullBridge() {
    static NullUiBridge bridge;
    return bridge;
}

NullUiHost& nullHost() {
    static NullUiHost host;
    return host;
}

} // namespace

UiBridge* uiBridge() { return g_bridge ? g_bridge : &nullBridge(); }

void setUiBridge(UiBridge* bridge) { g_bridge = bridge; }

UiHost* uiHost() { return g_host ? g_host : &nullHost(); }

void setUiHost(UiHost* host) { g_host = host; }

// The desktop/test build has no Java half: `installUiBridge` stays a no-op and
// the Android build provides the real one (`UiBridge.cpp`, the only translation
// unit that may see JNI headers).
#if (AX_TARGET_PLATFORM != AX_PLATFORM_ANDROID)
void installUiBridge(MenuScene*, GameScene*) {}
#endif

} // namespace surv