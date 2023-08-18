#pragma once
#include <array>
#include <queue>
#include <cstdint>

#include "reports.hpp"
enum extension_type {
    EXT_NONE,
    EXT_NUNCHUK,
    EXT_CLASSIC_CONTROLLER,
    EXT_MOTION_PLUS,
    EXT_PASSTHROUGH_NUNCHUK,
    EXT_PASSTHROUGH_CLASSIC_CONTROLLER
};
enum MemType : uint8_t {
    MEM_EEPROM = 0x0,
    MEM_REGISTER = 0x04
};
