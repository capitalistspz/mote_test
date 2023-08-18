#include "wiimote.hpp"
#include "writes.hpp"

button_flags wiimote::get_buttons() const {
    std::shared_lock lock(m_button_mutex);
    return m_state.buttons;
}

std::optional<std::array<ir_dot, 4>> wiimote::ir_dots() const {
    {
        std::shared_lock status_lock (m_status_mutex);
        if (m_state.status.ir_enabled){
            return {};
        }
    }
    std::shared_lock ir_lock(m_ir_mutex);
    return m_state.ir_dots;
}

wiimote_status wiimote::status() const {
    m_status_mutex.lock_shared();
    auto status = m_state.status;
    m_status_mutex.unlock_shared();

    m_rumble_mutex.lock_shared();
    status.rumble = m_state.rumble;
    m_rumble_mutex.unlock_shared();

    return status;
}

vec3<float> get_acc_calibrated(vec3<uint16_t> acc, vec3<uint16_t> zero, vec3<uint16_t> gravity){
    const vec3<float> zero_f(zero);
    return {(acc.x - zero_f.x) / (gravity.x - zero_f.x),
            (acc.y - zero_f.y) / (gravity.y - zero_f.y),
            (acc.z - zero_f.z) / (gravity.z - zero_f.z)};
}

vec3<float> wiimote::accelerometer() const {
    std::shared_lock acc_lock(m_acc_mutex);
    auto& calib = m_state.acc_calib;
    const vec3<float> zero_f = calib.zero;

    return {(m_state.acc.x - zero_f.x) / ((calib.gravity.x - zero_f.x) * 8),
            -(m_state.acc.z - zero_f.z) / ((calib.gravity.z - zero_f.z) * 8),
            -(m_state.acc.y - zero_f.y) / ((calib.gravity.y - zero_f.y) * 8)};
//    return m_state.acc;
}


bool wiimote::get_rumble() const {
    std::shared_lock rumble_lock(m_rumble_mutex);
    return m_state.rumble;
}


void wiimote::set_rumble(bool val) {
    {
        std::scoped_lock rumble_lock(m_rumble_mutex);
        // This is what actually sets the rumble
        m_state.rumble = val;
    }
    RumbleReport rumble;
    write(span_of(rumble));
}

void wiimote::set_leds(led_flags leds) {
    uint8_t led_val = (static_cast<uint8_t>(leds) & 0xF0) >> 4;
    LEDReport report {.led = led_val};
    write(span_of(report));
}


std::optional<vec3<float>> wiimote::motionplus() const {
    std::shared_lock mpls_lock(m_extension_mutex);
    if (!m_motionplus || m_motionplus->mode == MotionPlusMode::EXTENSION_ONLY){
        return {};
    }

    vec3<float> zero;
    vec3<float> grav;
    vec3<float> multiplier;

    auto& calib_x = m_motionplus->is_slow_mode.x ?  m_motionplus->slow_mode_calib : m_motionplus->fast_mode_calib;
    zero.x = calib_x.zero.x;
    grav.x = calib_x.gravity.x;
    multiplier.x = calib_x.degrees_div_6 * 6;

    auto& calib_y = m_motionplus->is_slow_mode.y ?  m_motionplus->slow_mode_calib : m_motionplus->fast_mode_calib;
    zero.y = calib_y.zero.y;
    grav.y = calib_y.gravity.y;
    multiplier.y = calib_y.degrees_div_6 * 6;

    auto& calib_z = m_motionplus->is_slow_mode.z ?  m_motionplus->slow_mode_calib : m_motionplus->fast_mode_calib;
    zero.z = calib_z.zero.z;
    grav.z = calib_z.gravity.z;
    multiplier.z = calib_z.degrees_div_6 * 6;

    return vec3<float>{(m_motionplus->gyro_raw.x - zero.x) * multiplier.x / (grav.x - zero.x),
                        (m_motionplus->gyro_raw.y - zero.y) * multiplier.y / (grav.y - zero.y),
                        (m_motionplus->gyro_raw.z - zero.z) * multiplier.z / (grav.z - zero.z)};
}
