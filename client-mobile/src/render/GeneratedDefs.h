#pragma once
// Installs the codegen'd render DefProvider (see tools/codegen_render_defs.mjs
// and GeneratedDefs.cpp). Call once at startup before loading a map.
namespace surv {

void installGeneratedDefs();

} // namespace surv
