#pragma once
#include <condition_variable>
#include <queue>
#include <thread>
#include <shared_mutex>
#include <mutex>
#include <span>
#include <variant>
#include <filesystem>
#include <hidapi.h>
#include <optional>
#include <fmt/format.h>
#include <fmt/ranges.h>

#include "vec.hpp"
#include "extensions.hpp"
#include "enums.hpp"
#include "writes.hpp"

#define WIIMOTELIBPP_DEFINE_ENUM_FLAG_OPERATORS(T) \
    constexpr inline T operator~ (T a) { return static_cast<T>( ~static_cast<std::underlying_type<T>::type>(a) ); } \
    constexpr inline T operator| (T a, T b) { return static_cast<T>( static_cast<std::underlying_type<T>::type>(a) | static_cast<std::underlying_type<T>::type>(b) ); } \
    constexpr inline T operator& (T a, T b) { return static_cast<T>( static_cast<std::underlying_type<T>::type>(a) & static_cast<std::underlying_type<T>::type>(b) ); } \
    constexpr inline T operator^ (T a, T b) { return static_cast<T>( static_cast<std::underlying_type<T>::type>(a) ^ static_cast<std::underlying_type<T>::type>(b) ); } \
    constexpr inline T& operator|= (T& a, T b) { return a = a | b; } \
    constexpr inline T& operator&= (T& a, T b) { return a = a & b; } \
    constexpr inline T& operator^= (T& a, T b) { return a = a & b; } \
    constexpr inline bool operator !(T flags){ return static_cast<std::underlying_type<T>::type>(flags) == 0; }     \

enum class button_flags : uint16_t {
    NONE = 0x00,
    DPAD_LEFT = 0x01,
    DPAD_RIGHT = 0x02,
    DPAD_DOWN = 0x04,
    DPAD_UP = 0x08 ,
    PLUS = 0x10,
    TWO = 0x01 << 8,
    ONE = 0x02 << 8,
    B = 0x04 << 8,
    A = 0x08 << 8,
    MINUS = 0x10 << 8,
    HOME = 0x80 << 8,
    ALL_BUTTONS_PRESSED = TWO | ONE | B | A | HOME | DPAD_LEFT | DPAD_RIGHT | DPAD_DOWN | DPAD_UP | PLUS | MINUS
};


WIIMOTELIBPP_DEFINE_ENUM_FLAG_OPERATORS(button_flags)

enum class led_flags : uint8_t {
    // This bit enables rumble
    _DO_NOT_SET = 0x01,
    ONE = 0x10,
    TWO = 0x20,
    THREE = 0x40,
    FOUR = 0x80,
    ALL = ONE | TWO | THREE | FOUR
};

WIIMOTELIBPP_DEFINE_ENUM_FLAG_OPERATORS(led_flags)

template <typename T> requires std::is_standard_layout_v<T>
static std::span<uint8_t> span_of(T& value){
    return {reinterpret_cast<uint8_t*>(&value), sizeof(T)};
}

template <typename T> requires std::is_standard_layout_v<T>
static std::span<const uint8_t> span_of(T const& value){
    return {reinterpret_cast<uint8_t const*>(&value), sizeof(T)};
}

struct ir_dot {
    vec2<uint16_t> position;
    uint8_t size{};
    bool disabled{};
    uint8_t intensity{};
    vec2<uint16_t> min;
    vec2<uint16_t> max;
};

struct wiimote_status {
    bool battery_very_low;
    bool extension_connected;
    bool speaker_enabled;
    bool rumble;
    bool ir_enabled;
    uint8_t battery_level;
    led_flags leds;
};

class wiimote {
    struct MemReadRequest {
        size_t id;
        address_t address{{0}};
        size_t expected_size;
        std::vector<uint8_t> data;

    };

    struct full_state {
        wiimote_status status{};
        button_flags buttons{};
        std::array<ir_dot, 4> ir_dots;
        vec3<uint16_t> acc;
        Calibration acc_calib;
        std::queue<MemReadRequest> read_requests;
        size_t read_req_counter = 0u;
        bool rumble = false;
    };

public:
    explicit wiimote(std::filesystem::path const &device_path);

    wiimote(uint16_t vendor_id, uint16_t product_id, std::wstring_view serial);

    wiimote(wiimote const &) = delete;

    wiimote(wiimote &&) noexcept ;

    ~wiimote();

private:
    void init();

    void set_reporting_mode(bool continuous, input_reports report);

    void write(std::vector<uint8_t>&& data);
    void write(std::span<uint8_t> data);

    ssize_t read(std::span<uint8_t> data);

    size_t handle_extension_data(std::span<uint8_t> const data);
    size_t handle_ir_data_basic(uint8_t *);
    size_t handle_ir_data_extended(uint8_t *);
    size_t handle_ir_full(uint8_t const*, bool initial);
    size_t handle_buttons_only(uint8_t const *const r);
    size_t handle_buttons_acc(uint8_t const *const);
    size_t handle_interleaved_a(uint8_t const*r);
    size_t handle_interleaved_b(uint8_t const*r);
    size_t handle_mem_read(uint8_t *r);
    size_t handle_status(uint8_t const *status);
    size_t handle_acknowledgement(const uint8_t *data);
    void handle_wiimote_calibration_data(std::span<uint8_t const> data, bool retry_on_checksum_fail = false);

    void mem_request_write(std::array<uint8_t, 4> address, std::vector<uint8_t>&& data);
    void mem_request_read(std::array<uint8_t, 4> address, size_t);

    void write_mem_read_report(MemReadReport report);
    void write_mem_write_report(MemWriteReport report);

    void on_mem_read(const MemReadRequest &req);


    void request_extension();
    void request_wiimote_calibration(bool allow_second_attempt);
    void request_motion_plus_calibration();
    void detect_motionplus();
    void activate_motionplus(MotionPlusMode mode = MotionPlusMode::MOTIONPLUS_ONLY);
    void init_motionplus();
    void update_motionplus();

    void read_loop();
    void write_loop();
    void mem_req_loop();

    bool get_rumble() const;

public:
    button_flags get_buttons() const;

    std::optional<std::array<ir_dot, 4>> ir_dots() const;

    wiimote_status status() const;

    vec3<float> accelerometer() const;

    std::optional<vec3<float>> motionplus() const;

public:
    void set_rumble(bool);

    void set_leds(led_flags leds);

    void request_status();

private: // Threading
    // For buttons
    mutable std::shared_mutex m_button_mutex;
    // For accelerometer and accelerometer calibration
    mutable std::shared_mutex m_acc_mutex;
    // For IR data and IR calibration
    mutable std::shared_mutex m_ir_mutex;
    // For extensions, including motionplus, and their calibration
    mutable std::shared_mutex m_extension_mutex;
    // For status
    mutable std::shared_mutex m_status_mutex;
    // For keeping track of reads
    mutable std::mutex m_mem_read_update_mutex;
    // For rumble
    mutable std::shared_mutex m_rumble_mutex;

    std::thread m_read_thread;

    std::thread m_write_thread;
    std::queue<std::vector<uint8_t>> m_write_queue;
    std::mutex m_writer_mutex;

    struct {
        std::atomic_bool wait_for_read;
        std::atomic_bool wait_for_write;
        std::mutex control_mutex;
        std::condition_variable control_cv;

        std::mutex element_mutex;
        std::condition_variable_any element_cv;
        std::queue<std::variant<MemWriteReport, MemReadReport>> queue;

        std::thread thread;
    } mem_req_queue;


    std::atomic_bool m_running;
private:
    full_state m_state{};
    std::variant<std::monostate, NunchukRaw, ClassicController> m_extension{};
    std::optional<MotionPlusRaw> m_motionplus;


private:
    hid_device *m_device;
};

