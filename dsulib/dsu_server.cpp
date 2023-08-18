#include <ranges>
#include "dsu_server.hpp"
#include "logger.hpp"
#include "crc.hpp"
#include <random>
#include <cassert>
#include <chrono>

u32 rand_u32() {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<u32> dist{};

    return dist(gen);
}

template<typename T>
std::span<const u8> span_of(T const &data) {
    return {reinterpret_cast<u8 const *>(&data), sizeof(T)};
}

dsu_server::dsu_server()
        : m_id(rand_u32()) {

}

bool dsu_server::start(sns::endpoint const &ep) {
    const auto error = m_socket.bind(ep);
    if (error) {
        log_info("Failed to start dsu server: {}", error.message());
        return false;
    }
    log_info("Successfully started dsu server");
    m_read_thread = std::jthread([this] { read_loop(); });
    m_write_thread = std::jthread([this] {send_loop(); });
    return true;
}

void dsu_server::read_loop() {
    m_running = true;
    sns::endpoint endpoint{};
    buffer<512> buffer;
    while (m_running.load(std::memory_order_relaxed)) {
        const auto res = m_socket.receive_from(buffer, endpoint, 0);
        if (!res)
            throw std::runtime_error(res.error().message());
        if (*res < sizeof(msg::header)) {
            log_info("Received only {} bytes from ep {}:{}", *res, endpoint.address(), endpoint.port());
            continue;
        }
        auto data = buffer | std::views::take(*res);
        auto header = (msg::header *) data.data();


        auto client_it = m_clients.find(header->id);
        if (client_it == m_clients.cend()) {
            std::tie(client_it, std::ignore) = m_clients.emplace(header->id, endpoint);
        }

        using namespace types;

        switch (header->type) {
            case event_type::PROTOCOL_VERSION: {
                log_info("Client protocol version: {}", header->protocol_version);
                msg::protocol_info_report rep{protocol_version};

                send(header->type, span_of(rep), client_it->second);
                break;
            }
            case event_type::CONTROLLER_STATUS: {
                auto val = (msg::status_request *) (data.data() + sizeof(msg::header));
                if (m_status_handler) {
                    auto responses = m_status_handler(*val);
                    for (auto const& response : responses){
                        send(header->type, span_of(response), client_it->second);
                    }
                } else {
                    log_info("Received status while no handler set");
                }
                break;
            }
            case event_type::CONTROLLER_DATA: {
                auto val = (msg::controller_data_request *) (data.data() + sizeof(msg::header));

                if (m_controller_data_handler) {
                    auto responses = m_controller_data_handler(*val);
                    for (auto& response : responses){
                        auto& pack_no  = *const_cast<u32*>(&response.packet_no);
                        pack_no = ++client_it->second.data_packet_no;
                        send(header->type, span_of(response), client_it->second);
                    }
                } else {
                    log_info("Received controller data while no handler set");
                }
                break;
            }
            case event_type::MOTOR_STATUS:
            case event_type::RUMBLE:
            default:
                log_info("Unhandled data");
        }
    }
    log_info("Reading stopped");
}

void dsu_server::set_status_handler(const status_handler &handler) {
    m_status_handler = handler;
}

void dsu_server::set_controller_data_handler(const controller_data_handler &handler) {
    m_controller_data_handler = handler;
}

void dsu_server::send(types::event_type type, std::span<const u8> data, dsu_server::client_t &client) {
    // Message size, no header
    const auto size = out_msg_size(type);
    constexpr auto header_size = sizeof(msg::header);

    std::vector<u8> out_data(size + header_size);
    auto header = (msg::header *) out_data.data();
    header->packet_length = size + 4;
    header->id = m_id;
    header->magic_string = dsu_server::magic_string;
    header->protocol_version = dsu_server::protocol_version;
    header->type = type;
    header->crc32 = 0;
    std::copy(data.begin(), data.end(), out_data.begin() + header_size);
    header->crc32 = crc(out_data.begin(), out_data.end());

    const auto res = m_socket.send_to(out_data, client.ep, 0);
    assert(res.has_value());
}

void dsu_server::stop() {
    m_running = false;
}

void dsu_server::send_loop() {
    using namespace std::chrono;
    using namespace std::chrono_literals;
    auto prev = system_clock::now();
    auto time = prev;

    while (m_running.load(std::memory_order_relaxed)){
        for (time = system_clock::now(); (time - prev) < 5ms; time = system_clock::now()){
            // Waste time
        }
        if (m_controller_data_handler){
            for (auto&[id, client] : m_clients){
                msg::controller_data_request req{};
                req.reg_mode = types::RegistrationMode::SLOT;
                req.slot = 0;
                req.mac_address = {};
                auto responses = m_controller_data_handler(req);
                for (auto& response : responses){
                    auto& pack_no  = *const_cast<u32*>(&response.packet_no);
                    pack_no = ++client.data_packet_no;
                    send(types::event_type::CONTROLLER_DATA, span_of(response), client);
                }
            }
            prev = system_clock::now();

        }

    }
}


