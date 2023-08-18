#include <array>
#include <cstdint>

class address_t {
public:
    constexpr address_t(std::array<uint8_t, 4> addr)
    : m_value(addr)  {
    }

    template <size_t TotalBytes, size_t MatchedBytes = TotalBytes> requires ((MatchedBytes <= TotalBytes) && (MatchedBytes < 3))
    constexpr bool matches_low (std::array<uint8_t, TotalBytes> addr) const{
        for (auto i = 0; i < TotalBytes; ++i){
            if (m_value[(4 - i) - 1] != addr[TotalBytes - 1 - i]){
                return false;
            }
        }
        return true;
    }
    template <std::integral Number, size_t Bytes = sizeof(Number)> requires(Bytes <= 4)
    constexpr bool matches_low(Number n){
        return (as_uint32() >> (8 * (4 - Bytes))) == n;
    }

    constexpr bool operator==(std::array<uint8_t, 4> addr) const {
        return m_value == addr;
    }
    constexpr bool operator==(address_t addr) const {
        return m_value == addr.m_value;
    }
    auto value() const -> decltype(auto) { return m_value; }
    uint32_t as_uint32() const { return *reinterpret_cast<const uint32_t*>(m_value.data()); }

    operator std::array<uint8_t,4> () const {
        return m_value;
    }
private:
    std::array<uint8_t, 4> m_value;
};

namespace addresses {
    constexpr address_t motionplus_calibration_fast = {{0x04,0xA6, 0x00, 0x20}};
    constexpr address_t motionplus_calibration_slow = {{0x04,0xA6, 0x00, 0x36}};
    constexpr address_t extension_identifier = {{0x04,0xA4, 0x00, 0xFA}};
    constexpr address_t motionplus_identifier = {{0x04,0xA6, 0x00, 0xFA}};
    constexpr address_t wiimote_calibration = {{0x00,0x00, 0x00, 0x16}};
    constexpr address_t wiimote_calibration_mirror = {{0x00,0x00, 0x00, 0x20}};

}
