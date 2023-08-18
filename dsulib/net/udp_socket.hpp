#pragma once
#include <span>
#include <tuple>

#include "enums.hpp"
#include "endpoint.hpp"
#include "result.hpp"

namespace sns {
    class udp_socket;
}

class sns::udp_socket {
public:
    udp_socket();
    ~udp_socket();

    /**
    *
    * @param local_ep endpoint to bind the socket to
    */
    std::error_code bind(const endpoint&);

    /**
     * @param buffer the buffer to store the received m_data in
     * @param length the capacity of the buffer
     * @param flags flags to alter how the receive operation behaves
     * @param out_remote_ep the remote endpoint that the data has been received from
     * @returns number of bytes received, or else returns a negative error (see errno) if no m_data is received and the operation is non-blocking
     * */
    result<size_t> receive_from(std::span<uint8_t> buffer, sns::endpoint &out_remote_ep, int flags);

    /**
     * @param buffer the buffer to send the bytes from
     * @param length the number of bytes to read from the buffer
     * @param flags flags to alter how the send operation behaves
     * @param remote_ep the remote endpoint that the data will be sent to
     * @returns number of bytes received, or else returns a negative error (see errno) if no m_data is received and the operation is non-blocking
     * */
    result<size_t> send_to(std::span<uint8_t> data, const sns::endpoint &remote_ep, int flags);

    /**
     * Closes the socket
     */
    void close();

    /**
     * @param shutdownType the type of socket io operation to shutdown
     */
    std::error_code shutdown(sns::shutdown_type shutdownType);
    /**
     *
     * @param opt socket option to set
     */
    std::error_code set_option(int opt);

// Compiler doesn't allow split declaration for these

    template <typename Value>
    std::error_code set_option(sns::option_name name, const Value &value) {
        const auto result = ::setsockopt(socket_fd, SOL_SOCKET, (int)name, &value, sizeof(value));
        if (result < 0)
            return {errno, std::system_category()};
        return {};
    }

    template <typename Value>
    std::error_code get_option(sns::option_name name, Value &outValue) const {
        const auto result = ::getsockopt(socket_fd, SOL_SOCKET, (int)name, outValue, sizeof(outValue));
        if (result < 0)
            return {errno, std::system_category()};
        return {};
    }
private:
    int socket_fd;
};