#pragma once
// Android native crash capture (C++/NDK). The Java uncaught-exception handler
// in AppActivity covers ART exceptions; SIGSEGV/SIGABRT/SIGBUS/SIGILL/SIGFPE
// from the engine or ported code never reach Java, so the handler writes a
// small backtrace file that the next launch reports through /api/report_error.
#include <string>

namespace surv {

// Installs signal handlers that write `path` and then re-raise for the normal
// tombstone/crash dialog. `path` comes from AppActivity (getFilesDir()).
void installNativeCrashHandler(const std::string& path);

} // namespace surv
