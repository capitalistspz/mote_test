#pragma once
#include <thread>
#include <mutex>
#include <functional>
#include <queue>
#include <span>
#include <unordered_map>

#include "net/udp_socket.hpp"
#include "net/endpoint.hpp"
#include "messages.hpp"

using status_handler = std::function<std::vector<msg::status_report>(msg::status_request const&)>;
using controller_data_handler = std::function<std::vector<msg::controller_data_report>(msg::controller_data_request const&)>;

class dsu_server {
    constexpr static u16 protocol_version = 1001;
    constexpr static buffer<4> magic_string = {'D','S','U','S'};

    // packet number, endpoint
    struct client_t {
        explicit client_t(sns::endpoint const& endpoint) : data_packet_no(0), ep(endpoint)  {

        }
        u32 data_packet_no;
        sns::endpoint ep;
        std::mutex data_packet_no_mutex{};
    };
public:
    dsu_server();
public:
    bool start(sns::endpoint const& ep);
    void stop();

    void set_status_handler(const status_handler& handler);
    void set_controller_data_handler(const controller_data_handler& handler);
private:
    status_handler m_status_handler;
    controller_data_handler m_controller_data_handler;
private:
    void send(types::event_type type, std::span<const u8> data, client_t& client);
    void read_loop();
    void send_loop();
private:
    u32 m_id;
    std::unordered_map<u32, client_t> m_clients;
    sns::udp_socket m_socket;

    std::jthread m_read_thread;
    std::jthread m_write_thread;
    std::atomic_bool m_running;
};


