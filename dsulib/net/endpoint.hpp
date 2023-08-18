#pragma once

#include <netinet/in.h>
#include <string_view>
#include <arpa/inet.h>
#include <cstring>
#include <cerrno>
#include <stdexcept>
#include <string>
#include <iostream>
#include <iomanip>
#include <utility>

namespace sns {

    class endpoint {
    public:
        /**
         * @param address numbers-and-dots notation address e.g. 127.0.0.1
         * @param port_number
         */
        endpoint(std::string_view address, uint16_t port_number);

        /**
         *
         * @param address ip address in bytes representation
         * @param port_number
         */
        endpoint(in_addr_t address, uint16_t port_number);

        explicit endpoint(const sockaddr_in &addr);

        endpoint(const sockaddr &addr, size_t size);

        explicit endpoint(const sockaddr_storage &storage);

        /**
         * Creates an invalid endpoint
         */
        endpoint() = default;

        endpoint(endpoint const &ep) = default;

        endpoint(endpoint &&ep) noexcept = default;


        endpoint &operator=(const endpoint &rhs) = default;

        endpoint &operator=(endpoint &&rhs) noexcept = default;

        /**
         * @return The underlying socket endpoint
         */
        [[nodiscard]] const sockaddr *data() const;

        /**
         * @return The capacity of the underlying socket endpoint
         */
        [[nodiscard]] size_t size() const;

        /**
         *
         * @return The port of this endpoint in host order
         */
        [[nodiscard]] uint16_t port() const;

        /**
         *
         * @return The number and dots representation of the address endpoint
         */
        [[nodiscard]] const char *address() const;

        auto operator<=>(const endpoint &ep) const;

        auto operator==(const endpoint &ep) const;

        [[nodiscard]] uint64_t inline comparison_value() const;

    protected:
        sockaddr_in m_address_in;
    };
}