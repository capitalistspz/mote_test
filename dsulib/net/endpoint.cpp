#include "endpoint.hpp"

sns::endpoint::endpoint(std::string_view address, uint16_t port_number)
: m_address_in() {
    m_address_in.sin_family = AF_INET;
    const auto result = inet_aton(address.data(), &m_address_in.sin_addr);
    if (result == 0)
        throw std::invalid_argument("address");
            m_address_in.sin_port = htons(port_number);
}

sns::endpoint::endpoint(in_addr_t address, uint16_t port_number)
: m_address_in() {
    m_address_in.sin_family = AF_INET;
    m_address_in.sin_addr.s_addr = htonl(address);
    m_address_in.sin_port = htons(port_number);
}

sns::endpoint::endpoint(const sockaddr_in& addr)
        :m_address_in(addr){}

sns::endpoint::endpoint(const sockaddr& addr, size_t size)
        : m_address_in(){
    std::memcpy(&m_address_in, &addr, size);
}

sns::endpoint::endpoint(const sockaddr_storage& storage)
        : m_address_in(*reinterpret_cast<sockaddr_in const*>(&storage)) {
}

const sockaddr* sns::endpoint::data() const {
    return (sockaddr*)&m_address_in;
}

uint16_t sns::endpoint::port() const {
    return ntohs(m_address_in.sin_port);
}

size_t sns::endpoint::size() const {
    return sizeof(m_address_in);
}

const char* sns::endpoint::address() const {
    return inet_ntoa(m_address_in.sin_addr);
}

auto sns::endpoint::operator <=> (const endpoint& ep) const{
    const auto thisValue = comparison_value();
    const auto thatValue = ep.comparison_value();
    using R = decltype(thisValue <=> thatValue);
    if (thisValue < thatValue)
        return R::less;
    else if (thisValue == thatValue)
        return R::equal;
    else
        return R::greater;
}


auto sns::endpoint::operator == (const endpoint& ep) const {
    return m_address_in.sin_port == ep.m_address_in.sin_port && m_address_in.sin_addr.s_addr == ep.m_address_in.sin_addr.s_addr;
}

uint64_t inline sns::endpoint::comparison_value() const {
    return (static_cast<uint64_t>(m_address_in.sin_addr.s_addr) << sizeof(in_port_t)) + m_address_in.sin_port;
}
