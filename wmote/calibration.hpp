#pragma once
#include <cstdint>

struct MotionPlusCalibrationData {
    uint16_t yaw_zero;
    uint16_t roll_zero;
    uint16_t pitch_zero;

    uint16_t yaw_scale;
    uint16_t roll_scale;
    uint16_t pitch_scale;

    uint8_t degrees_div_6;
    uint8_t unique_id;
    uint16_t crc_32;

};
static_assert(sizeof(MotionPlusCalibrationData) == 16);

struct WiimoteCalibrationData {
    uint8_t x_zero_high;
    uint8_t y_zero_high;
    uint8_t z_zero_high;
    uint8_t z_zero_low: 2;
    uint8_t y_zero_low: 2;
    uint8_t x_zero_low: 2;
    uint8_t _unused1: 2;
    uint8_t x_grav_high;
    uint8_t y_grav_high;
    uint8_t z_grav_high;
    uint8_t z_grav_low: 2;
    uint8_t y_grav_low: 2;
    uint8_t x_grav_low: 2;
    uint8_t _unused2: 2;
    uint8_t speaker_volume: 7;
    bool rumble: 1;
    uint8_t checksum;
};
static_assert(sizeof(WiimoteCalibrationData) == 10);