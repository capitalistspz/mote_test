#include "wiimote.hpp"
#include "reports.hpp"
#include "internal_logging.hpp"
#include "writes.hpp"
#include "byteswap.hpp"
#include <cuchar>
#include <future>
std::string to_string(std::wstring_view wstr){
    char mb[MB_LEN_MAX];
    std::string out;
    std::mbstate_t state{};
    for (auto const c : wstr){
        auto size = std::c16rtomb(mb, c, &state);
        out.append(mb, size);
    }
    return out;
}

#if defined(WIIMOTELIBPP_DEBUG)
#define assert(x, msg) if (!(x)) { throw std::runtime_error(msg); };
#else
#define assert(x, msg)
#endif

constexpr static unsigned int MAX_MESSAGE_LENGTH = 22;

wiimote::wiimote(const std::filesystem::path &device_path)
        : m_device(hid_open_path(device_path.string().c_str())), m_extension(std::monostate{}) {
    if (!m_device)
        throw std::runtime_error(to_string(hid_error(nullptr)));

    init();

}

wiimote::wiimote(uint16_t vendor_id, uint16_t product_id, std::wstring_view serial)
: m_device(hid_open(vendor_id, product_id, serial.data())), m_extension(std::monostate{}) {
    if (!m_device)
        throw std::runtime_error(to_string(hid_error(nullptr)));

    init();

}

wiimote::wiimote(wiimote && other) noexcept {
    // Close other thread
    other.m_running = false;
    other.m_read_thread.join();
    other.m_write_thread.join();
    other.mem_req_queue.thread.join();

    // Copy state and device
    m_state = other.m_state;
    m_device = other.m_device;

    m_write_queue = std::move(other.m_write_queue);

    // Prevent old instance from closing the device that this instance is now using
    other.m_device = nullptr;
    init();
}

wiimote::~wiimote() {
    m_write_thread.join();
    m_read_thread.join();
    log_info("Read thread joined");
    hid_close(m_device);
}

void wiimote::init() {
    m_running = true;
    m_write_thread = std::thread(&wiimote::write_loop, this);
    m_read_thread = std::thread(&wiimote::read_loop, this);
    mem_req_queue.thread = std::thread(&wiimote::mem_req_loop, this);

    request_status();
    request_extension();
    detect_motionplus();

    set_reporting_mode(true, REP_IN_BUTTONS_ACC_IR_12B);
    request_wiimote_calibration(true);
}

void wiimote::write(std::vector<uint8_t>&& data) {
    data[1] |= get_rumble();
    std::scoped_lock lock(m_writer_mutex);
    m_write_queue.push(std::move(data));
}

void wiimote::write(std::span<uint8_t> data) {
    data[1] |= get_rumble();
    std::scoped_lock writer_lock(m_writer_mutex);
    m_write_queue.emplace(data.begin(), data.end());
}

ssize_t wiimote::read(std::span<uint8_t> data) {
    return hid_read(m_device, data.data(), data.size());
}

void wiimote::request_status(){
    RequestStatusReport rep{};
    auto rep_span = span_of(rep);
    write(rep_span);
}

void wiimote::mem_request_write(std::array<uint8_t, 4> address, std::vector<uint8_t>&& data) {
    MemWriteReport report;
    report.address = address;
    report.size = data.size();

    std::move(data.begin(), data.end(), report.data.begin());
    write_mem_write_report(report);
}

void wiimote::mem_request_read(std::array<uint8_t, 4> address, size_t size) {
    assert(size < std::numeric_limits<uint16_t>::max(), "Read too large");

    MemReadReport report;
    report.address = address;
    report.size_big_endian = bswap_on_le<std::uint16_t>(size);
    {
        std::scoped_lock lock(m_mem_read_update_mutex);
        m_state.read_requests.emplace(m_state.read_req_counter, report.address, size);
        log_info("Pushed request: (ID: {}, Address: {:x}, Size: {})", m_state.read_req_counter++, bswap_on_le(*(uint32_t*)&address), size);
    }
    write_mem_read_report(report);
}

void wiimote::request_extension() {
    mem_request_write({MEM_REGISTER, 0xA4, 0x00, 0xf0}, {0x55});
    mem_request_write({MEM_REGISTER, 0xA4, 0x00, 0xfb}, {0x00});
    mem_request_read(addresses::extension_identifier, 6);
}
void wiimote::request_wiimote_calibration(bool allow_second_attempt) {
    if (allow_second_attempt)
        mem_request_read(addresses::wiimote_calibration, 10);
    else
        mem_request_read(addresses::wiimote_calibration_mirror, 10);
}

void wiimote::detect_motionplus() {
    mem_request_read(addresses::motionplus_identifier, 6);
}

void wiimote::activate_motionplus(MotionPlusMode mode) {
    switch (mode) {
        case MotionPlusMode::MOTIONPLUS_ONLY:
            mem_request_write({MemType::MEM_REGISTER, 0xA6, 0x00, 0xFE}, {0x04});
            break;
        case MotionPlusMode::EXTENSION_ONLY:
            mem_request_write({MemType::MEM_REGISTER, 0xA4, 0x00, 0xF0}, {0x55});
            break;
        case MotionPlusMode::NUNCHUK_PASSTHROUGH:
            mem_request_write({MemType::MEM_REGISTER, 0xA6, 0x00, 0xFE}, {0x05});
            break;
        case MotionPlusMode::CLASSIC_PASSTHROUGH:
            mem_request_write({MemType::MEM_REGISTER, 0xA6, 0x00, 0xFE}, {0x07});
            break;
        default:
            return;
    }
}

void wiimote::init_motionplus() {
    mem_request_write({MemType::MEM_REGISTER, 0xA6, 0x00, 0xf0}, {0x55});
}

void wiimote::update_motionplus() {
    if (m_motionplus){
        return;
    }
    request_motion_plus_calibration();
    m_motionplus = MotionPlusRaw{};
}

void wiimote::request_motion_plus_calibration() {
    mem_request_read(addresses::motionplus_calibration_fast, 16);
    mem_request_read(addresses::motionplus_calibration_slow, 16);
}


void wiimote::set_reporting_mode(bool continuous, input_reports report) {
    write({REP_OUT_DATA_REPORT_MODE, uint8_t(0x04 * continuous), report});
}

void wiimote::read_loop() {
    std::array<uint8_t, MAX_MESSAGE_LENGTH> buffer{0};
    while (m_running.load(std::memory_order_relaxed)) {
        const auto bytes = read(buffer);
        if (bytes == 0)
            continue;
        else if (bytes < 0) {
            log_error("Failed");
            continue;
        }
        const auto id = input_reports(buffer[0]);
        auto data = buffer.data();
        auto offset = 1;

        switch (id) {
            case REP_IN_STATUS_INFORMATION:
            {
                assert(bytes == 7, "Expected 7 bytes");
                offset += handle_buttons_only(data + offset);
                offset += handle_status(data + offset);
                break;
            }
            case REP_IN_MEMORY_READ:
                offset += handle_buttons_only(data + offset);
                offset += handle_mem_read(data + offset);
                break;
            case REP_IN_ACK_OUTPUT_REPORT:
                offset += handle_buttons_only(data + offset);
                offset += handle_acknowledgement(data + offset);
                break;
            case REP_IN_BUTTONS:
            assert(bytes == 3, "Expected 3 bytes.");
                offset += handle_buttons_only(data + offset);
                break;
            case REP_IN_BUTTONS_ACC:
            assert(bytes == 6, "Expected 6 bytes");
                offset += handle_buttons_acc(data + offset);
                break;
            case REP_IN_BUTTONS_EXT_8B:
            assert(bytes == 11, "Expected 11 bytes");
                offset += handle_buttons_only(data + offset);
                offset += handle_extension_data({buffer.begin() + offset, 8});
                break;
            case REP_IN_BUTTONS_ACC_IR_12B:
            assert(bytes == 18, "Expected 18 bytes");
                offset += handle_buttons_acc(data + offset);
                offset += handle_ir_data_extended(data + offset);
                break;
            case REP_IN_BUTTONS_EXT_19B:
            assert(bytes == 22, "Expected 22 bytes");
                offset += handle_buttons_only(data + offset);
                offset += handle_extension_data({buffer.begin() + offset, 19});
                break;
            case REP_IN_BUTTONS_ACC_EXT_16B:
            assert(bytes == 22, "Expected 22 bytes");
                offset += handle_buttons_acc(data + offset);
                offset += handle_extension_data({buffer.begin() + offset, 16});
                break;
            case REP_IN_BUTTONS_IR_10B_EXTENSION_9B:
            assert(bytes == 22, "Expected 22 bytes")
                offset += handle_buttons_only(data + offset);
                offset += handle_ir_data_extended(data + offset);
                offset += handle_extension_data({buffer.begin() + offset, 9});
                break;
            case REP_IN_BUTTONS_ACC_IR_10B_EXTENSION_6B:
            assert(bytes == 22, "Expected 22 bytes")

                offset += handle_buttons_acc(data + offset);
                offset += handle_ir_data_basic(data + offset);
                offset += handle_extension_data({buffer.begin() + offset, 6});
                break;
            case REP_IN_BUTTONS_ACC_IR_36B:
            assert(bytes == 18, "Expected 18 bytes")
                offset += handle_interleaved_a(data + offset);
                break;
            case REP_IN_BUTTONS_ACC_IR_36B_ALT:
            assert(bytes == 18, "Expected 18 bytes")
                offset += handle_interleaved_b(data + offset);
                break;
            default:
                log_error("Received unhandled report {:#x}", uint8_t(id));
                break;
        }
    }
}

void wiimote::write_loop(){
    using namespace std::chrono;
    using namespace std::chrono_literals;
    auto last_time = high_resolution_clock::now();
    while (m_running.load(std::memory_order_relaxed)){
        m_writer_mutex.lock();
        if (!m_write_queue.empty()){
            std::vector<uint8_t> res = std::move(m_write_queue.front());
            m_write_queue.pop();
            hid_write(m_device, res.data(), res.size());
        }
        m_writer_mutex.unlock();

        while ((high_resolution_clock::now() - last_time) < 5ms){
            std::this_thread::yield();
        }
        last_time = high_resolution_clock::now();
    }
}


void wiimote::write_mem_read_report(MemReadReport report){
    std::scoped_lock ul(mem_req_queue.element_mutex);
    mem_req_queue.queue.emplace(report);
}

void wiimote::write_mem_write_report(MemWriteReport report){
    std::scoped_lock ul(mem_req_queue.element_mutex);
    mem_req_queue.queue.emplace(report);
}

void wiimote::mem_req_loop(){
    auto& queue = mem_req_queue.queue;
    while (m_running.load(std::memory_order_relaxed)){
        std::unique_lock ul(mem_req_queue.control_mutex);
        mem_req_queue.control_cv.wait(ul, [m = &mem_req_queue] {return (!m->wait_for_read) && (!m->wait_for_write); });
        if (!queue.empty()){
            std::unique_lock em(mem_req_queue.element_mutex);
            auto& res = queue.front();
            if (auto* p = std::get_if<MemWriteReport>(&res)){
                write(span_of(*p));
                mem_req_queue.wait_for_write = true;
            }
            else {
                write(span_of(std::get<MemReadReport>(res)));
                mem_req_queue.wait_for_read = true;
            }
            queue.pop();
            mem_req_queue.element_cv.notify_one();
            // To be unlocked in read thread by acknowledge (0x22) or read report (0x21)
            ul.release();
        }

    }
}