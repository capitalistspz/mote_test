#pragma once
#include "core.hpp"

namespace types {
    enum class event_type : u32 {
        PROTOCOL_VERSION = 0x100000,
        CONTROLLER_STATUS = 0x100001,
        CONTROLLER_DATA = 0x100002,
        MOTOR_STATUS = 0x110001,
        RUMBLE = 0x110002
    };

    constexpr auto out_msg_size(event_type e){
        switch (e) {
            case event_type::PROTOCOL_VERSION:
                return 2;
            case event_type::CONTROLLER_STATUS:
                return 12;
            case event_type::CONTROLLER_DATA:
                return 80;
                break;
            case event_type::MOTOR_STATUS:
                return 12;
                break;
            case event_type::RUMBLE:
                return 0;
                break;
        }
        return 0;
    }

    enum class SlotState : u8 {
        DISCONNECTED = 0,
        RESERVED = 1,
        CONNECTED = 2
    };

    enum class GyroModel : u8 {
        N_A = 0,
        LIMITED = 1,
        FULL = 2
    };

    enum class ConnectionType : u8 {
        N_A = 0,
        USB = 1,
        BT = 2
    };

    enum class BatteryLevel : u8 {
        N_A = 0x00,
        DYING = 0x01,
        LOW = 0x02,
        MEDIUM = 0x03,
        HIGH = 0x04,
        FULL = 0x05,
        CHARGING = 0xEE,
        CHARGED = 0xEF
    };

    enum class RegistrationMode : u8 {
        ALL,
        SLOT,
        MAC_ADDRESS
    };

#pragma pack(push, 1)
    struct controller_info {
        u8 controller_slot;
    };

    struct device_info {
        u8 slot;
        SlotState slot_state;
        GyroModel model;
        ConnectionType conn_type;
        buffer<6> mac_address;
        BatteryLevel battery;
    };
    static_assert(sizeof(device_info) == 11);



    struct touch_data {
        u8 active;
        u8 id;
        u16 x;
        u16 y;
    };
    static_assert(sizeof(touch_data) == 6);
}


namespace msg {
    struct header { // NOLINT(*-pro-type-member-init)
        buffer<4> magic_string = {'D','S','U','S'};
        u16 protocol_version = 1001;
        u16 packet_length = sizeof(types::event_type);
        u32 crc32;
        u32 id;
        // Excluding header, but includes event type
        types::event_type type;
    };
    static_assert(sizeof(header) == 20);

    struct status_request {
        u32 slot_count;
        buffer <4> slots;
    };
    static_assert(sizeof(status_request) == 8);

    struct controller_data_request {
        types::RegistrationMode reg_mode;
        u8 slot;
        buffer<6> mac_address;
    };
    static_assert(sizeof(controller_data_request) == 8);

    struct motor_request {
        controller_data_request req;
        u8 motor_id;
        u8 motor_intensity;
    };
    static_assert(sizeof(motor_request) == 10);

    struct protocol_info_report {
        u16 protocol_version;
    };
    static_assert(sizeof(protocol_info_report) == 2);

    struct status_report {
        types::device_info dev;
        u8 motor_count = 0;
    };
    static_assert(sizeof(status_report) == 12);



    struct controller_data_report {
        types::device_info dev;
        u8 connected;
        const u32 packet_no{0};
        struct {
            bool share: 1;
            bool l3: 1;
            bool r3: 1;
            bool options: 1;
            bool dpad_up: 1;
            bool dpad_right: 1;
            bool dpad_down: 1;
            bool dpad_left: 1;

            bool l2: 1;
            bool r2: 1;
            bool l1: 1;
            bool r1: 1;
            bool x: 1;
            bool a: 1;
            bool b: 1;
            bool y: 1;
            uint8_t home;
            uint8_t touch;
        } buttons;
        struct {
            u8 x;
            u8 y;
        } stick_l;
        struct {
            u8 x;
            u8 y;
        } stick_r;
        struct  {
            u8 dpad_left;
            u8 dpad_down;
            u8 dpad_right;
            u8 dpad_up;
            u8 y;
            u8 b;
            u8 a;
            u8 x;
            u8 l1;
            u8 r1;
            u8 r2;
            u8 l2;
        } analog_buttons;
        types::touch_data touch_1;
        types::touch_data touch_2;
        u64 acc_timestamp_us;
        // In gs
        struct {
            float x;
            float y;
            float z;
        } acc;
        // In deg/sec
        struct {
            float pitch;
            float yaw;
            float roll;
        } gyro;
    };
    static_assert(sizeof(controller_data_report) == 80);
}

#pragma pack(pop)



