#pragma clang diagnostic push
#pragma ide diagnostic ignored "ArgumentSelectionDefects"

#include <iostream>
#include <chrono>
#include <limits>
#include "dsulib/dsu_server.hpp"
#include "wmote/logging.hpp"
#include "wmote/wiimote.hpp"
#include "wmote/reads.hpp"

constexpr float float_max = std::numeric_limits<float>::infinity();
constexpr float float_min = -std::numeric_limits<float>::infinity();
using namespace std::chrono_literals;

int main() {
    set_info_logger([](std::string const &s) { std::cout << s << '\n'; });

    wiimote mote(0x057e, 0x0306, {});
    dsu_server server;
    // Server handlers
    auto wm_status_get = [&mote](uint8_t slot_no) {
        auto connected = mote.test_connected();
        auto wm_status = mote.status();

        types::device_info dev;
        dev.slot = slot_no;
        dev.slot_state = connected ? types::SlotState::CONNECTED : types::SlotState::DISCONNECTED;
        dev.model = types::GyroModel::LIMITED;
        dev.conn_type = types::ConnectionType::BT;

        if (wm_status.battery_very_low) {
            dev.battery = types::BatteryLevel::DYING;
        } else {
            dev.battery = types::BatteryLevel::CHARGED;
        }
        return dev;
    };
    {

        server.set_status_handler([&wm_status_get](msg::status_request const &req) {
            std::vector<msg::status_report> reports;
            for (auto i{0u}; i < req.slot_count; ++i) {
                reports.emplace_back(wm_status_get(req.slots[i]), 0);
            }
            return reports;
        });
        using namespace std::chrono;
        using namespace std::chrono_literals;
        server.set_controller_data_handler([&mote, &wm_status_get](msg::controller_data_request const &req) {
            const static auto start_time = system_clock::now();
            std::vector<msg::controller_data_report> reports;

            if (req.reg_mode != types::RegistrationMode::SLOT)
                return reports;

            msg::controller_data_report rep;
            rep.dev = wm_status_get(req.slot);

            if (rep.dev.slot_state != types::SlotState::CONNECTED) {
                rep.connected = false;
                reports.push_back(rep);
                return reports;
            }
            rep.connected = true;
            // Buttons
            {
                auto buttons = mote.get_buttons();
                rep.buttons.dpad_down = !!(buttons & button_flags::DPAD_DOWN);
                rep.buttons.dpad_up = !!(buttons & button_flags::DPAD_UP);
                rep.buttons.dpad_left = !!(buttons & button_flags::DPAD_LEFT);
                rep.buttons.dpad_right = !!(buttons & button_flags::DPAD_RIGHT);
                rep.buttons.home = !!(buttons & button_flags::DPAD_RIGHT);
                rep.buttons.a = !!(buttons & button_flags::A);
                rep.buttons.b = !!(buttons & button_flags::B);
                rep.buttons.x = !!(buttons & button_flags::ONE);
                rep.buttons.y = !!(buttons & button_flags::TWO);
                rep.buttons.options = !!(buttons & button_flags::PLUS);
                rep.buttons.share = !!(buttons & button_flags::MINUS);
                rep.buttons.touch = false;
            }

            auto acc = mote.accelerometer();
            rep.acc_timestamp_us = duration_cast<microseconds>(system_clock::now() - start_time).count();
            rep.acc.x = acc.x;
            rep.acc.y = acc.y;
            rep.acc.z = acc.z;

            auto mpls = mote.motionplus();
            if (mpls) {
                rep.dev.model = types::GyroModel::FULL;
                rep.gyro.pitch = mpls->x;
                rep.gyro.yaw = mpls->y;
                rep.gyro.roll = mpls->z;
            }
            // Analog buttons
            {
                rep.analog_buttons.dpad_down = rep.buttons.dpad_down * 255;
                rep.analog_buttons.dpad_up = rep.buttons.dpad_up * 255;
                rep.analog_buttons.dpad_right = rep.buttons.dpad_right * 255;
                rep.analog_buttons.a = rep.buttons.a * 255;
                rep.analog_buttons.x = rep.buttons.x * 255;
                rep.analog_buttons.b = rep.buttons.b * 255;
                rep.analog_buttons.y = rep.buttons.y * 255;
            }
            reports.push_back(rep);

            return reports;
        });
    }
    //server.start({"127.0.0.1", 26760});
    size_t led_index = 0;
    vec3<float> acc_max{float_min, float_min, float_min};
    vec3<float> acc_min{float_max, float_max, float_max};
    std::this_thread::sleep_for(1s);

    for (;;) {
        // Wait for button release
        if (!!(mote.get_buttons() & button_flags::A)) {
            while (!!(mote.get_buttons() & button_flags::A));
            mote.set_rumble(true);
        }
        if (!!(mote.get_buttons() & button_flags::B)) {
            while (!!(mote.get_buttons() & button_flags::B));
            mote.set_rumble(false);
        }

        if (!!(mote.get_buttons() & button_flags::PLUS)) {
            while (!!(mote.get_buttons() & button_flags::PLUS));

            uint8_t mask = 0;
            mask |= 1 << (4 + (led_index % 4));
            if (led_index >= 4)
                mask |= 1 << (4 + ((led_index - 3) % 4));

            mote.set_leds((led_flags) mask);
            ++led_index;
        }

        if (!!(mote.get_buttons() & button_flags::MINUS)) {
            while (!!(mote.get_buttons() & button_flags::MINUS));

            uint8_t mask = 0;
            mask |= 1 << (4 + (led_index % 4));
            if (led_index >= 4)
                mask |= 1 << (4 + ((led_index - 3) % 4));

            mote.set_leds((led_flags) mask);
            --led_index;
        }
//        auto acc = mote.motionplus();
//        acc_min = min(acc, acc_min);
//        acc_max = max(acc, acc_max);
//        std::cout << fmt::format("Current: ({:8.4f}, {:8.4f}, {:8.4f})\t"
//                                 "Min: ({:8.4f}, {:8.4f}, {:8.4f})\t"
//                                 "Max: ({:8.4f}, {:8.4f}, {:8.4f})\t\n",
//                                 acc.x, acc.y, acc.z,
//                                 acc_min.x, acc_min.y, acc_min.z,
//                                 acc_max.x, acc_max.y, acc_max.z);
    }

    return 0;
}

#pragma clang diagnostic pop