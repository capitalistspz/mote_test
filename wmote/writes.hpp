#pragma once

#include "reports.hpp"

#pragma pack(push, 1)
struct RumbleReport {
    output_reports report = output_reports::REP_OUT_RUMBLE;
    uint8_t _unused: 8;
};

static_assert(sizeof(RumbleReport) == 2);

struct LEDReport {
    output_reports report = output_reports::REP_OUT_PLAYER_LED;
    uint8_t _unused: 4;
    uint8_t led: 4;
};
static_assert(sizeof(LEDReport) == 2);

struct MemReadReport {
    output_reports report = output_reports::REP_OUT_READ_FROM_MEMORY;
    std::array<uint8_t, 4> address;
    uint16_t size_big_endian;
};
static_assert(sizeof(MemReadReport) == 7);

struct MemWriteReport {
    output_reports report = output_reports::REP_OUT_WRITE_TO_MEMORY;
    std::array<uint8_t, 4> address;
    uint8_t size;
    std::array<uint8_t, 16> data;

};
static_assert(sizeof(MemWriteReport) == 22);
#pragma pack(pop)