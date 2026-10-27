#include "NativeCrash.h"

#if AX_TARGET_PLATFORM == AX_PLATFORM_ANDROID

#include <csignal>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <fcntl.h>
#include <sys/system_properties.h>
#include <unistd.h>
#include <unwind.h>

namespace surv {
namespace {

char g_crashPath[512] = {0};
volatile sig_atomic_t g_inHandler = 0;
constexpr int kSignals[] = {SIGSEGV, SIGABRT, SIGBUS, SIGILL, SIGFPE};

void writeAll(int fd, const char* text, size_t len) {
    while (len > 0) {
        const ssize_t written = write(fd, text, len);
        if (written <= 0) {
            return;
        }
        text += written;
        len -= static_cast<size_t>(written);
    }
}

void writeHex(int fd, uintptr_t value) {
    static constexpr char kHex[] = "0123456789abcdef";
    char buf[2 + sizeof(uintptr_t) * 2];
    size_t pos = 0;
    buf[pos++] = '0';
    buf[pos++] = 'x';
    bool started = false;
    for (int shift = static_cast<int>(sizeof(uintptr_t) * 8) - 4; shift >= 0; shift -= 4) {
        const unsigned digit = static_cast<unsigned>((value >> shift) & 0xf);
        if (digit != 0 || started || shift == 0) {
            started = true;
            buf[pos++] = kHex[digit];
        }
    }
    writeAll(fd, buf, pos);
}

struct UnwindState {
    int fd;
    int count;
};

_Unwind_Reason_Code unwindCallback(_Unwind_Context* ctx, void* arg) {
    auto* state = static_cast<UnwindState*>(arg);
    if (state->count++ >= 64) {
        return _URC_END_OF_STACK;
    }
    const uintptr_t pc = static_cast<uintptr_t>(_Unwind_GetIP(ctx));
    if (pc != 0) {
        writeAll(state->fd, "pc=", 3);
        writeHex(state->fd, pc);
        writeAll(state->fd, "\n", 1);
    }
    return _URC_NO_REASON;
}

void crashHandler(int sig, siginfo_t* info, void*) {
    // A fault inside the handler itself must not recurse; die straight away.
    if (g_inHandler) {
        _exit(128 + sig);
    }
    g_inHandler = 1;

    if (g_crashPath[0] != '\0') {
        const int fd = open(g_crashPath, O_WRONLY | O_CREAT | O_TRUNC, 0600);
        if (fd >= 0) {
            char head[128];
            const int n = std::snprintf(head, sizeof head, "signal=%d pid=%d\n", sig,
                                        static_cast<int>(getpid()));
            if (n > 0) {
                writeAll(fd, head, static_cast<size_t>(n));
            }
            if (info != nullptr && info->si_addr != nullptr) {
                writeAll(fd, "fault=", 6);
                writeHex(fd, reinterpret_cast<uintptr_t>(info->si_addr));
                writeAll(fd, "\n", 1);
            }
            // Bionic only exposes backtrace() from API 33; _Unwind_Backtrace
            // works on every supported API level and writes raw PCs.
            UnwindState state{fd, 0};
            _Unwind_Backtrace(unwindCallback, &state);
            close(fd);
        }
    }

    // Restore the default action and re-raise so the platform still emits its
    // tombstone/crash dialog after our report has been written.
    struct sigaction sa;
    std::memset(&sa, 0, sizeof sa);
    sa.sa_handler = SIG_DFL;
    sigemptyset(&sa.sa_mask);
    sigaction(sig, &sa, nullptr);
    raise(sig);
    _exit(128 + sig);
}

void installHandlers() {
    // A dedicated stack lets SIGSEGV on the normal stack still run the handler.
    static char altStack[64 * 1024];
    stack_t ss;
    std::memset(&ss, 0, sizeof ss);
    ss.ss_sp = altStack;
    ss.ss_size = sizeof altStack;
    sigaltstack(&ss, nullptr);

    struct sigaction sa;
    std::memset(&sa, 0, sizeof sa);
    sa.sa_sigaction = crashHandler;
    sa.sa_flags = SA_SIGINFO | SA_ONSTACK;
    sigemptyset(&sa.sa_mask);
    for (const int sig : kSignals) {
        sigaction(sig, &sa, nullptr);
    }
}

} // namespace

void installNativeCrashHandler(const std::string& path) {
    if (path.empty() || path.size() >= sizeof g_crashPath) {
        return;
    }
    std::snprintf(g_crashPath, sizeof g_crashPath, "%s", path.c_str());
    installHandlers();

    // Diagnostic-only fault injection for verifying the handler on a device:
    //   adb shell setprop debug.surv_crash 1   (relaunch; clear afterwards)
    char trigger[PROP_VALUE_MAX] = {0};
    if (__system_property_get("debug.surv_crash", trigger) > 0
        && std::string(trigger) == "1") {
        raise(SIGSEGV);
    }
}

} // namespace surv

#else

namespace surv {

void installNativeCrashHandler(const std::string&) {}

} // namespace surv

#endif
