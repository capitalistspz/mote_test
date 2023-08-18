#include <numeric>

#include "wiimote.hpp"
#include "reads.hpp"
#include "internal_logging.hpp"
#include "calibration.hpp"
#include "byteswap.hpp"

size_t wiimote::handle_status(uint8_t const *const data) {
    auto status = reinterpret_cast<WiimoteStatus const *>(data);
    {
        std::scoped_lock lock(m_status_mutex);
        m_state.status.battery_very_low = status->battery_very_low;
        m_state.status.extension_connected = status->extension_connected;
        m_state.status.speaker_enabled = status->speaker_enabled;
        m_state.status.ir_enabled = status->ir_enabled;
        m_state.status.leds = static_cast<led_flags>(status->led_state);
        m_state.status.battery_level = status->battery_level;
    }
    return sizeof(WiimoteStatus);
}

size_t wiimote::handle_buttons_only(uint8_t const *const data) {
    {
        std::scoped_lock lock(m_button_mutex);
        m_state.buttons = *reinterpret_cast<const button_flags *>(data);
    }
    return sizeof(ButtonData);
}

size_t wiimote::handle_buttons_acc(uint8_t const *const data) {
    auto pack = reinterpret_cast<const ButtonsAccData *>(data);
    auto button_data = pack->button_data;
    handle_buttons_only(data);

    uint16_t x = (pack->x << 2) | (button_data.b0b6 << 1) | button_data.b0b5;
    uint16_t y = (pack->y << 2) | button_data.b1b5 << 1;
    uint16_t z = (pack->z << 2) | button_data.b1b6 << 1;

    {
        std::scoped_lock lock(m_acc_mutex);
        m_state.acc.x = x;
        m_state.acc.y = y;
        m_state.acc.z = z;
    }
    return sizeof(ButtonsAccData);
}

size_t wiimote::handle_acknowledgement(uint8_t const *data) {
    auto ack = reinterpret_cast<Acknowledgement const *>(data);
    if ((ack->output_report == output_reports::REP_OUT_WRITE_TO_MEMORY) && mem_req_queue.wait_for_write){
        mem_req_queue.wait_for_write = false;
        mem_req_queue.control_mutex.unlock();
        mem_req_queue.control_cv.notify_one();
    }
    if (ack->error_code) {
        log_error("Received error code {:#x} on report {:#x}", ack->error_code, ack->output_report);
    }
    return sizeof(Acknowledgement);
}

size_t wiimote::handle_ir_data_basic(uint8_t *data) {
    auto pack = reinterpret_cast<IRBasic *>(data);
    std::scoped_lock lock(m_ir_mutex);
    auto &ir = m_state.ir_dots;

    for (auto i = 0; i < 2; ++i) {
        auto &ir_a = ir[i * 2];
        auto &ir_b = ir[i * 2 + 1];
        ir_a.position.x = pack->x1_low | (pack->x1_high << 8);
        ir_a.position.y = pack->y1_low | (pack->y1_high << 8);

        ir_b.position.x = pack->x2_low | (pack->x2_high << 8);
        ir_b.position.y = pack->y2_low | (pack->y2_high << 8);

        ir_a.size = 0;
        ir_b.size = 0;

        ir_a.disabled = (ir_a.position.x == 0x3ffu && ir_a.position.y == 0x3ffu);
        ir_b.disabled = (ir_b.position.x == 0x3ffu && ir_b.position.y == 0x3ffu);
    }
    return sizeof(IRBasic) * 2;
}

size_t wiimote::handle_ir_data_extended(uint8_t *data) {
    auto pack = reinterpret_cast<IRExtendedSingle *>(data);
    std::scoped_lock lock(m_ir_mutex);
    for (auto index = 0; index < 4; ++index) {
        auto const &ir_data = pack[index];
        auto &dot = m_state.ir_dots[index];
        dot.position.x = ir_data.x_low | (ir_data.x_high << 8);
        dot.position.y = ir_data.y_low | (ir_data.y_high << 8);
        dot.size = ir_data.size;
    }
    return sizeof(IRExtendedSingle) * 4;
}

size_t wiimote::handle_ir_full(uint8_t const *data, bool initial) {
    auto pack = reinterpret_cast<IRFullSingle const *>(data);
    std::scoped_lock lock(m_ir_mutex);
    const auto offset = (!initial) * 2;
    for (auto i = 0u; i < 2; ++i) {
        auto &dot = m_state.ir_dots[i + offset];
        auto const &ir_data = pack[i];
        dot.position.x = ir_data.x_low | (ir_data.x_high << 8);
        dot.position.y = ir_data.y_low | (ir_data.y_high << 8);
        dot.size = ir_data.size;

        dot.min.x = ir_data.x_min;
        dot.min.y = ir_data.y_min;
        dot.intensity = ir_data.intensity;
    }
    return sizeof(IRFullSingle) * 2;
}

size_t wiimote::handle_interleaved_a(uint8_t const *data) {
    handle_buttons_only(data);
    auto const button_data_ptr = reinterpret_cast<ButtonData const *>(data);
    {
        std::scoped_lock lock(m_acc_mutex);
        m_state.acc.x = *(data + 1);
        m_state.acc.z |= (button_data_ptr->b0b6 << 5) | (button_data_ptr->b0b5 << 4);
        m_state.acc.z |= (button_data_ptr->b1b6 << 7) | (button_data_ptr->b1b5 << 6);
    }

    handle_ir_full(data + 4, true);
    return 22;
}

size_t wiimote::handle_interleaved_b(uint8_t const *data) {
    auto const button_data_ptr = reinterpret_cast<ButtonData const *>(data + 0);
    data += handle_buttons_only(data);
    {
        std::scoped_lock lock(m_acc_mutex);
        m_state.acc.y = *data;
        m_state.acc.z |= (button_data_ptr->b0b6 << 5) | (button_data_ptr->b0b5 << 4);
        m_state.acc.z |= (button_data_ptr->b1b6 << 7) | (button_data_ptr->b1b5 << 6);
    }
    handle_ir_full(data + 4, true);
    return 22;
}

size_t wiimote::handle_mem_read(uint8_t *data) {
    constexpr size_t data_size = sizeof(MemoryReadData);
    auto pack = reinterpret_cast<MemoryReadData const*>(data);
    const auto error = pack->error;
    auto const address = bswap_on_le(pack->address_low_bytes_big_endian);
    auto const size = pack->size + 1;

    std::scoped_lock read_requests_lock (m_mem_read_update_mutex);

    if (m_state.read_requests.empty()){
        log_error("Received unexpected mem read report from: {:#x}", address);
        return data_size;
    }

    auto& front = m_state.read_requests.front();
    const auto expected_address = (bswap_on_le(front.address.as_uint32()) + front.data.size());
    if (address != (expected_address & 0xFFFF)){
        log_error("Received read from {:#x}, when expecting from {:#x}, ignoring.", address, expected_address);
        return data_size;
    }

    // "Completes" the read request
    auto do_pop = [&]{
        if (mem_req_queue.wait_for_read){
            mem_req_queue.wait_for_read = false;
            mem_req_queue.control_mutex.unlock();
            mem_req_queue.control_cv.notify_one();
            log_info("Popped request: (ID: {}, Address: {:x}, Size: {})",front.id, bswap_on_le(front.address.as_uint32()), front.expected_size);
        }
        m_state.read_requests.pop();
    };

    if (error){
        do_pop();
        switch (error) {
            case 7: {
                log_error("Attempted to read from write only address {:02x}, or from disconnected expansion",
                          expected_address);
                break;
            }
            case 8: {
                log_error("Attempted to read from nonexistent memory address with low bytes {:02x}", expected_address);
                break;
            }
            default:
                log_error("Received unknown memory read error {:02x} when reading from memory address with low bytes",
                          expected_address);
        }
        return data_size;
    }

    std::copy(pack->data.cbegin(), pack->data.cbegin() + size, std::back_inserter(front.data));
    if (front.expected_size != front.data.size()){
        return data_size;
    }

    const MemReadRequest front_copy = front;
    do_pop();
    m_mem_read_update_mutex.unlock();
    on_mem_read(front_copy);
    return data_size;
}

void wiimote::on_mem_read(MemReadRequest const &req) {

    if ((req.address == addresses::extension_identifier) || (req.address == addresses::motionplus_identifier)) {
        const auto id = vec_to_u48(req.data);
        const auto ext_id = extension_id(id);
        std::scoped_lock ext_lock(m_extension_mutex);
        switch (ext_id) {
            case extension_id::NUNCHUK:
                log_info("Detected nunchuk");
                m_extension = NunchukRaw{};
                break;
            case extension_id::MPLS:
                log_info("Detected motionplus");
                m_extension = {};
                m_motionplus->mode = MotionPlusMode::MOTIONPLUS_ONLY;
                update_motionplus();
                set_reporting_mode(true, input_reports::REP_IN_BUTTONS_ACC_EXT_16B);
                break;
            case extension_id::CLASSIC:
                log_info("Detected classic controller");
                m_extension = ClassicController{};
                break;
            case extension_id::MPLS_CLASSIC:
                log_info("Detected classic controller passthrough");
                m_extension = ClassicController{};
                break;
            case extension_id::MPLS_NUNCHUK:
                log_info("Detected passthrough nunchuk");
                update_motionplus();
                m_motionplus->mode = MotionPlusMode::NUNCHUK_PASSTHROUGH;
                m_extension = NunchukRaw{};
                set_reporting_mode(true, input_reports::REP_IN_BUTTONS_ACC_EXT_16B);
                break;
            case extension_id::INACTIVE_MPLS:
            {
                init_motionplus();
                if (std::holds_alternative<NunchukRaw>(m_extension)){
                    activate_motionplus(MotionPlusMode::NUNCHUK_PASSTHROUGH);
                }
                else if (std::holds_alternative<ClassicController>(m_extension)){
                    activate_motionplus(MotionPlusMode::CLASSIC_PASSTHROUGH);
                }
                else {
                    activate_motionplus(MotionPlusMode::MOTIONPLUS_ONLY);
                }
                update_motionplus();
                break;
            }

            case extension_id::DEACTIVATED_MPLS_ACTIVE_EXT:
            {
                log_info("Detected deactivated motionplus");
                update_motionplus();
                m_motionplus->mode = MotionPlusMode::EXTENSION_ONLY;
                break;
            }
            default:
                log_info("Unhandled extension {:#x}", id);
                break;
        }

    } else if (req.address == addresses::wiimote_calibration) {
        handle_wiimote_calibration_data(req.data, true);

    } else if (req.address == addresses::wiimote_calibration_mirror) {
        handle_wiimote_calibration_data(req.data, false);
    }
    else if (req.address == addresses::motionplus_calibration_fast){
    }
    //log_info("Complete read request from {:#x}", bswap_on_le(req.address.as_uint32()));
}

void handle_motionplus_ext(NunchukRaw &nunchuk, uint8_t *data) {
    const auto pack = reinterpret_cast<NunchukPassthrough const *>(data);
    nunchuk.button_c = !pack->c;
    nunchuk.button_z = !pack->z;
    nunchuk.stick_raw.x = pack->stick_x;
    nunchuk.stick_raw.y = pack->stick_y;
    nunchuk.acc_raw.x = pack->acc_x_high << 2 | pack->acc_x_low << 1 | nunchuk.acc_raw.x & 1;
    nunchuk.acc_raw.y = pack->acc_y_high << 2 | pack->acc_y_low << 1 | nunchuk.acc_raw.y & 1;
    nunchuk.acc_raw.z = pack->acc_z_high << 2 | pack->acc_z_low << 1 | nunchuk.acc_raw.z & 1;
}

void handle_motionplus_ext(ClassicController &classic_controller, uint8_t *data) {
    const auto pack = reinterpret_cast<ClassicControllerPassthroughData const *>(data);
    log_error("Classic Controller passthrough is yet to be implemented");
}

void handle_motionplus_ext(std::monostate &, uint8_t *) {}

size_t handle_ext(NunchukRaw &nunchuk, uint8_t const *data) {
    auto pack = reinterpret_cast<NunchukData const *>(data);
    nunchuk.button_c = !pack->c;
    nunchuk.button_z = !pack->z;
    nunchuk.stick_raw.x = pack->stick_x;
    nunchuk.stick_raw.y = pack->stick_y;
    nunchuk.acc_raw.x = pack->acc_x_high << 2 | pack->acc_x_low;
    nunchuk.acc_raw.y = pack->acc_y_high << 2 | pack->acc_y_low;
    nunchuk.acc_raw.z = pack->acc_z_high << 2 | pack->acc_z_low;
    return sizeof(NunchukData);
}


size_t handle_ext(auto &, uint8_t const *) {
    log_info("Received extension data, when there is no extension");
    return 0;
}

size_t wiimote::handle_extension_data(std::span<uint8_t> data) {
    std::scoped_lock lock(m_extension_mutex);

    if (m_motionplus && m_motionplus->mode != MotionPlusMode::EXTENSION_ONLY) {
        const auto pack = reinterpret_cast<MotionPlusData *>(data.data());
        if (!pack->contains_mpls_data) {
            std::visit([&data](auto &t) { handle_motionplus_ext(t, data.data()); }, m_extension);
            return sizeof(MotionPlusData);
        }

        uint16_t yaw = pack->yaw_low | pack->yaw_high << 8;
        uint16_t roll = pack->roll_low | pack->roll_high << 8;
        uint16_t pitch = pack->pitch_low | pack->pitch_high << 8;
        m_motionplus->gyro_raw = {yaw, pitch, roll};
        m_motionplus->is_slow_mode = {pack->pitch_slow_mode, pack->yaw_slow_mode, pack->roll_slow_mode};
    } else {
        size_t byte_count = 0;
        std::visit([&data, &byte_count](auto &t) { byte_count = handle_ext(t, data.data()); }, m_extension);
        return byte_count;
    }
    return data.size();
}

void wiimote::handle_wiimote_calibration_data(std::span<const uint8_t> data, bool retry_on_checksum_fail) {
    //log_info("Calibrating wiimote accelerometers");
    const uint8_t checksum = std::accumulate(data.begin(), data.end() - 1, 0x55, std::plus<uint8_t>());

    const auto calib_data = (WiimoteCalibrationData *) data.data();
    if (checksum != calib_data->checksum) {
        if (retry_on_checksum_fail) {
            log_error(
                    "Wiimote calibration checksum mismatch: calculated checksum is {:#x} while retrieved checksum is {:#x}, requesting from mirror",
                    checksum, calib_data->checksum);
            request_wiimote_calibration(false);
        } else
            log_error(
                    "Wiimote calibration checksum mismatch: calculated checksum was {:#x} while retrieved checksum {:#x}",
                    checksum, calib_data->checksum);
        return;
    }

    std::scoped_lock acc_lock(m_acc_mutex);
    auto &calib = m_state.acc_calib;
    calib.zero.x = calib_data->x_zero_high << 2 | calib_data->x_zero_low;
    calib.zero.y = calib_data->y_zero_high << 2 | calib_data->y_zero_low;
    calib.zero.z = calib_data->z_zero_high << 2 | calib_data->z_zero_low;
    calib.gravity.x = calib_data->x_grav_high << 2 | calib_data->x_grav_low;
    calib.gravity.y = calib_data->y_grav_high << 2 | calib_data->y_grav_low;
    calib.gravity.z = calib_data->z_grav_high << 2 | calib_data->z_grav_low;
}



