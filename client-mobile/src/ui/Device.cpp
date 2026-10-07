#include "Device.h"

namespace ui {

Device& Device::get() {
    static Device device;
    return device;
}

} // namespace ui
