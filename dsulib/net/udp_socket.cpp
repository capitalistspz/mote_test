#pragma clang diagnostic push
#pragma ide diagnostic ignored "readability-make-member-function-const"
#include "udp_socket.hpp"

#include <stdexcept>
#include <cerrno>
#include <cstring>
#include <unistd.h>
#include <cassert>
#include <span>

void throw_with_errno(){
    throw std::runtime_error(std::strerror(errno));
}

namespace sns {
    udp_socket::udp_socket(){
        socket_fd = socket(PF_INET, SOCK_DGRAM, 0);
        assert(socket_fd >= 0);
    }

    std::error_code udp_socket::bind(const sns::endpoint& local_ep){
        const auto result = ::bind(socket_fd, local_ep.data(), local_ep.size());
        if (result == 0)
            return {};
        return {errno, std::system_category()};
    }

    /**
     * @param buffer the buffer to store the received m_data in
     * @param length the capacity of the buffer
     * @param flags flags to alter how the receive operation behaves
     * @param out_remote_ep the remote endpoint that the data has been received from
     * @returns number of bytes received, or else returns a negative error (see errno) if no m_data is received and the operation is non-blocking
     * */
    result<size_t> udp_socket::receive_from(std::span<uint8_t> buffer, sns::endpoint &out_remote_ep, int flags) {
        ssize_t bytes;
        sockaddr_storage clientAddress{};
        socklen_t clientAddressLength = sizeof(clientAddress);
        bytes = ::recvfrom(socket_fd, buffer.data(), buffer.size(), flags, reinterpret_cast<sockaddr *>(&clientAddress), &clientAddressLength);
        if (bytes < 0){
            return result<size_t>::error({errno, std::system_category()});
        }
        auto temp = bytes; // Breakpoint

        out_remote_ep = sns::endpoint{clientAddress};

        return temp;
    }

    result<size_t> udp_socket::send_to(std::span<uint8_t> data, const sns::endpoint& remote_ep, int flags) {
        const auto bytes = ::sendto(socket_fd, data.data(), data.size(), flags, remote_ep.data(), remote_ep.size());
        if (bytes < 0)
            return result<size_t>::error({errno, std::system_category()});
        else
            return bytes;
    }

    void udp_socket::close(){
        ::close(socket_fd);
    }

    std::error_code udp_socket::shutdown(shutdown_type shutdownType){
        const auto result = ::shutdown(socket_fd, (int)shutdownType);
        if (result < 0)
            return {errno, std::system_category()};
        return {};
    }

    std::error_code udp_socket::set_option(int opt) {
        const auto result = ::setsockopt(socket_fd, SOL_SOCKET, (int)opt, nullptr, 0);
        if (result < 0)
            return {errno, std::system_category()};
        return {};
    }

    udp_socket::~udp_socket() {
        close();
    }
}
#pragma clang diagnostic pop