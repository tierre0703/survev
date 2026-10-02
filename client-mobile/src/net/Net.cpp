// Forces construction of the definition registries and provides a small
// initializer that the app can call at startup (before any networking).
#include "Net.h"

namespace surv {

void initDefinitionRegistries() {
    // Touch the registries so their static tables are built eagerly and any
    // data error is surfaced at startup rather than mid-game.
    (void)GameObjectDefs().size();
    (void)MapObjectDefs().size();
}

} // namespace surv