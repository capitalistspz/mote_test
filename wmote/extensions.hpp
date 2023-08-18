#pragma once
#include <cstdint>
#include <array>
#include <vector>
#include "vec.hpp"
#include "memory.hpp"

#define wm_wm_part(n) static_cast<uint64_t>(data[n]) << (8 * (5 - n))
constexpr uint64_t vec_to_u48(std::vector<uint8_t> data){
    return wm_wm_part(0) | wm_wm_part(1) | wm_wm_part(2) | wm_wm_part(3) | wm_wm_part(4) | wm_wm_part(5);
}

constexpr uint64_t arr6_to_u48(std::array<uint8_t, 6> data){
    return wm_wm_part(0) | wm_wm_part(1) | wm_wm_part(2) | wm_wm_part(3) | wm_wm_part(4) | wm_wm_part(5);
}
#undef wm_wm_part

enum class extension_id : uint64_t {
    NUNCHUK = arr6_to_u48({0x00, 0x00, 0xA4, 0x20, 0x00, 0x00}),
    CLASSIC = arr6_to_u48({0x00, 0x00, 0xA4, 0x20, 0x01, 0x01}),
    CLASSIC_PRO = arr6_to_u48({0x10, 0x00, 0xA4, 0x20, 0x01, 0x01}),
    MPLS = arr6_to_u48({0x00, 0x00, 0xA4, 0x20, 0x04, 0x05}),
    MPLS_NUNCHUK = arr6_to_u48({0x00, 0x00, 0xA4, 0x20, 0x05, 0x05}),
    MPLS_CLASSIC = arr6_to_u48({0x00, 0x00, 0xA4, 0x20, 0x07, 0x05}),

    INACTIVE_MPLS = arr6_to_u48({0x00, 0x00, 0xA6, 0x20, 0x00, 0x05}),
    DEACTIVATED_MPLS_ACTIVE_EXT = arr6_to_u48({0x00, 0x00, 0xA6, 0x20, 0x04, 0x05}),
    DEACTIVATED_MPLS_NUNCHUK = arr6_to_u48({0x00, 0x00, 0xA6, 0x20, 0x05, 0x05}),
    DEACTIVATED_MPLS_CLASSIC = arr6_to_u48({0x00, 0x00, 0xA6, 0x20, 0x07, 0x05}),
};

enum class MotionPlusMode {
    // Just motionplus
    MOTIONPLUS_ONLY,
    EXTENSION_ONLY,
    NUNCHUK_PASSTHROUGH,
    CLASSIC_PASSTHROUGH

};

constexpr std::array<uint8_t, 3> extension_address = {0xA4, 0x00, 0xFA};

constexpr std::array<uint8_t, 3> motion_plus_address = {0xA6, 0x00, 0xFA};

struct Calibration {
    vec3<uint16_t> gravity = { 0x240, 0x240, 0x240 };
    vec3<uint16_t> zero = {0x200, 0x200, 0x200};
};

struct NunchukCalibration : Calibration{
    vec2<uint8_t> stick_zero;
    constexpr static uint16_t acc_range = 1024;
    constexpr static uint8_t stick_range = 255;
};

struct NunchukRaw {
    NunchukCalibration calibration;

    vec2<uint8_t> stick_raw;
    vec3<uint16_t> acc_raw;
    bool button_c;
    bool button_z;
};

struct MotionPlusCalibration : Calibration {
    uint8_t degrees_div_6;
};

struct MotionPlusRaw {
    MotionPlusCalibration fast_mode_calib;
    MotionPlusCalibration slow_mode_calib;
    vec3<bool> is_slow_mode;
    vec3<uint16_t> gyro_raw;
    MotionPlusMode mode;
};

struct ClassicController {
    //TODO
};