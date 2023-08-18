#pragma once

#include <cstdint>
#include <array>

struct ButtonData {
    bool dpad_left: 1;
    bool dpad_right: 1;
    bool dpad_down: 1;
    bool dpad_up: 1;
    bool plus: 1;
    // Byte 0 Bit 5
    bool b0b5: 1;
    // Byte 0 Bit 6
    bool b0b6: 1;
    // Byte 0 Bit 7
    bool b1b7: 1;
    bool two: 1;
    bool one: 1;
    bool b: 1;
    bool a: 1;
    bool minus: 1;
    // Byte 1 Bit 5
    bool b1b5: 1;
    // Byte 1 Bit 6
    bool b1b6: 1;
    bool home: 1;
};
static_assert(sizeof(ButtonData) == 2);

struct ButtonsAccData {
    ButtonData button_data;
    uint8_t x;
    uint8_t y;
    uint8_t z;
};
static_assert(sizeof(ButtonsAccData) == 5);


struct WiimoteStatus {
    bool battery_very_low: 1;
    bool extension_connected: 1;
    bool speaker_enabled: 1;
    bool ir_enabled: 1;
    uint8_t led_state: 4;
    uint8_t _zeroes[2];
    uint8_t battery_level;
};
static_assert(sizeof(WiimoteStatus) == 4);

#pragma pack(push, 1)
struct MemoryReadData {
    uint8_t error: 4;
    uint8_t size: 4;
    uint16_t address_low_bytes_big_endian;
    std::array<uint8_t, 16> data;
};
#pragma pack(pop)
static_assert(sizeof(MemoryReadData) == 19);

struct Acknowledgement {
    uint8_t output_report;
    uint8_t error_code;
};
static_assert(sizeof(Acknowledgement) == 2);

struct MotionPlusData {
    uint8_t yaw_low;
    uint8_t roll_low;
    uint8_t pitch_low;
    bool pitch_slow_mode: 1;
    bool yaw_slow_mode: 1;
    uint8_t yaw_high: 6;
    bool extension_connected: 1;
    bool roll_slow_mode: 1;
    uint8_t roll_high: 6;
    uint8_t _unused: 1;
    bool contains_mpls_data: 1;
    uint8_t pitch_high: 6;
};
static_assert(sizeof(MotionPlusData) == 6);

struct IRBasic {
    uint8_t x1_low;
    uint8_t y1_low;
    uint8_t x2_high: 2;
    uint8_t y2_high: 2;
    uint8_t x1_high: 2;
    uint8_t y1_high: 2;
    uint8_t x2_low;
    uint8_t y2_low;
};
static_assert(sizeof(IRBasic) == 5);

struct IRExtendedSingle {
    uint8_t x_low;
    uint8_t y_low;
    uint8_t size: 4;
    uint8_t x_high: 2;
    uint8_t y_high: 2;
};
static_assert(sizeof(IRExtendedSingle) == 3);

struct IRFullSingle {
    uint8_t x_low;
    uint8_t y_low;
    uint8_t size: 4;
    uint8_t x_high: 2;
    uint8_t y_high: 2;
    uint8_t x_min;
    uint8_t y_min;
    uint8_t x_max;
    uint8_t y_max;
    uint8_t _unused;
    uint8_t intensity;
};
static_assert(sizeof(IRFullSingle) == 9);

struct NunchukData {
    uint8_t stick_x;
    uint8_t stick_y;
    uint8_t acc_x_high;
    uint8_t acc_y_high;
    uint8_t acc_z_high;
    bool z: 1;
    bool c: 1;
    uint8_t acc_x_low: 2;
    uint8_t acc_y_low: 2;
    uint8_t acc_z_low: 2;
};
static_assert(sizeof(NunchukData) == 6);

struct NunchukPassthrough {
    uint8_t stick_x;
    uint8_t stick_y;
    uint8_t acc_x_high;
    uint8_t acc_y_high;
    bool extension_connected: 1;
    uint8_t acc_z_high: 7;
    uint8_t _unused: 1;
    bool contains_mpls_data: 1;
    bool z: 1;
    bool c: 1;
    uint8_t acc_x_low: 1;
    uint8_t acc_y_low: 1;
    uint8_t acc_z_low: 2;
};

static_assert(sizeof(NunchukPassthrough) == 6);

struct ClassicControllerFormat1 {
    uint8_t lx: 6;
    uint8_t rx_high: 2;

    uint8_t ly: 6;
    uint8_t rx_mid: 2;

    uint8_t ry: 5;
    uint8_t lt: 2;
    uint8_t rx_low: 1;

    uint8_t rt: 5;
    uint8_t lt_low: 3;

    uint8_t _unused: 1;
    bool b_rt: 1;
    bool b_plus: 1;
    bool b_home: 1;
    bool b_minus: 1;
    bool b_lt: 1;
    bool b_dpad_down: 1;
    bool b_dpad_right: 1;

    bool b_dpad_up: 1;
    bool b_dpad_left: 1;
    bool b_dpad_zr: 1;
    bool b_x: 1;
    bool b_a: 1;
    bool b_y: 1;
    bool b_b: 1;
    bool b_zl: 1;
};
static_assert(sizeof(ClassicControllerFormat1) == 6);

struct ClassicControllerFormat2 {
    uint8_t lx_high;
    uint8_t rx_high;
    uint8_t ly_high;
    uint8_t ry_high;
    uint8_t lx_low: 2;
    uint8_t rx_low: 2;
    uint8_t ly_low: 2;
    uint8_t ry_low: 2;
    uint8_t lt;
    uint8_t rt;

    uint8_t _unused: 1;
    bool b_rt: 1;
    bool b_plus: 1;
    bool b_home: 1;
    bool b_minus: 1;
    bool b_lt: 1;
    bool b_dpad_down: 1;
    bool b_dpad_right: 1;

    bool b_dpad_up: 1;
    bool b_dpad_left: 1;
    bool b_dpad_zr: 1;
    bool b_x: 1;
    bool b_a: 1;
    bool b_y: 1;
    bool b_b: 1;
    bool b_zl: 1;
};
static_assert(sizeof(ClassicControllerFormat2) == 9);

struct ClassicControllerFormat3 {
    uint8_t lx;
    uint8_t rx;
    uint8_t ly;
    uint8_t ry;
    uint8_t lt;
    uint8_t rt;

    uint8_t _unused: 1;
    bool b_rt: 1;
    bool b_plus: 1;
    bool b_home: 1;
    bool b_minus: 1;
    bool b_lt: 1;
    bool b_dpad_down: 1;
    bool b_dpad_right: 1;

    bool b_dpad_up: 1;
    bool b_dpad_left: 1;
    bool b_dpad_zr: 1;
    bool b_x: 1;
    bool b_a: 1;
    bool b_y: 1;
    bool b_b: 1;
    bool b_zl: 1;
};

struct ClassicControllerPassthroughData {
    bool b_dpad_up: 1;
    uint8_t lx: 5;
    uint8_t rx_high: 2;

    bool b_dpad_left: 1;
    uint8_t ly: 5;
    uint8_t rx_middle: 2;
    uint8_t ry: 5;
    uint8_t lt_high: 2;
    uint8_t rx_low: 1;

    uint8_t rt: 5;
    uint8_t lt: 3;

    bool extension_connected: 1;
    bool b_rt: 1;
    bool b_plus: 1;
    bool b_home: 1;
    bool b_minus: 1;
    bool b_lt: 1;
    bool b_dpad_down: 1;
    bool b_dpad_right: 1;

    bool _unused: 1;
    bool contains_mpls_data: 1;
    bool b_dpad_zr: 1;
    bool b_x: 1;
    bool b_a: 1;
    bool b_y: 1;
    bool b_b: 1;
    bool b_zl: 1;
};

static_assert(sizeof(ClassicControllerPassthroughData) == 6);