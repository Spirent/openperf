#ifndef _OP_UUID_HPP_
#define _OP_UUID_HPP_

#include <cstdint>
#include <string>
#include <array>
#include <random>
#include <stdexcept>

namespace openperf::core {

class uuid
{
    alignas(8) std::array<uint8_t, 16> m_octets;

    friend bool operator<(const uuid&, const uuid&);
    friend bool operator==(const uuid&, const uuid&);

public:
    static uuid random() /* constructor for random uuid */
    {
        std::mt19937_64 generator{std::random_device()()};
        std::uniform_int_distribution<> dist(0, 255);
        std::array<uint8_t, 16> data;

        data[0] = dist(generator);
        data[1] = dist(generator);
        data[2] = dist(generator);
        data[3] = dist(generator);
        data[4] = dist(generator);
        data[5] = dist(generator);
        data[6] = (dist(generator) & 0x0f) | 0x40; /* version 4 */
        data[7] = dist(generator);
        data[8] = (dist(generator) & 0x3f) | 0x40; /* variant */
        data[9] = dist(generator);
        data[10] = dist(generator);
        data[11] = dist(generator);
        data[12] = dist(generator);
        data[13] = dist(generator);
        data[14] = dist(generator);
        data[15] = dist(generator);

        return (uuid(data.data()));
    }

    uuid()
        : m_octets{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}
    {}

    uuid(const std::string_view input)
    {
        /* Sanity check input */
        static constexpr std::string_view delimiters("-");
        static constexpr std::string_view hex_digits = "0123456789abcdefABCDEF";

        if (input.length() != 36) {
            throw std::runtime_error("UUID must be 36 characters long");
        }

        for (size_t i = 0; i < input.length(); i++) {
            if (i == 8 || i == 13 || i == 18 || i == 23) {
                if (delimiters.find(input.at(i)) == std::string::npos) {
                    throw std::runtime_error("Invalid UUID format");
                }
            } else {
                if (hex_digits.find(input.at(i)) == std::string::npos) {
                    throw std::runtime_error(
                        std::string(&input.at(i))
                        + " is not a valid hexadecimal character");
                }
            }
        }

        /* Parse the string into bytes */
        size_t idx = 0, octet = 0;
        while (idx < 36) {
            if (delimiters.find(input.at(idx)) != std::string::npos) { idx++; }

            /* read two characters; convert to bytes */
            char buffer[3] = {};
            input.copy(buffer, 2, idx);
            m_octets[octet++] = std::strtol(buffer, nullptr, 16);
            idx += 2;
        }
    }

    uuid(const uint8_t data[16])
        : m_octets{data[0],
                   data[1],
                   data[2],
                   data[3],
                   data[4],
                   data[5],
                   data[6],
                   data[7],
                   data[8],
                   data[9],
                   data[10],
                   data[11],
                   data[12],
                   data[13],
                   data[14],
                   data[15]}
    {}

    uuid(std::initializer_list<uint8_t> data)
    {
        if (data.size() != 16) {
            throw std::runtime_error("16 items required");
        }

        size_t idx = 0;
        for (auto value : data) { m_octets[idx++] = value; }
    }

    uint8_t operator[](size_t idx) const
    {
        if (idx > 15) {
            throw std::out_of_range(std::to_string(idx)
                                    + " is not between 0 and 15");
        }
        return m_octets[idx];
    }

    const uint8_t* data() const { return (&m_octets[0]); }

    uint8_t* data() { return (&m_octets[0]); }
};

inline bool operator==(const uuid& lhs, const uuid& rhs)
{
    return (lhs.m_octets == rhs.m_octets);
}

inline bool operator<(const uuid& lhs, const uuid& rhs)
{
    return (lhs.m_octets < rhs.m_octets);
}

inline std::string to_string(const uuid& uuid)
{
    char buffer[37]; /* 36 chars + NUL */
    snprintf(
        buffer,
        37,
        "%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x",
        uuid[0],
        uuid[1],
        uuid[2],
        uuid[3],
        uuid[4],
        uuid[5],
        uuid[6],
        uuid[7],
        uuid[8],
        uuid[9],
        uuid[10],
        uuid[11],
        uuid[12],
        uuid[13],
        uuid[14],
        uuid[15]);
    return (std::string(buffer));
}

} /* namespace openperf::core */

namespace std {

template <> struct hash<openperf::core::uuid>
{
    size_t operator()(const openperf::core::uuid& uuid) const noexcept
    {
        auto data = reinterpret_cast<const uint64_t*>(uuid.data());
        auto h1 = std::hash<uint64_t>{}(data[0]);
        auto h2 = std::hash<uint64_t>{}(data[1]);
        return (h1 ^ (h2 << 1));
    }
};

} /* namespace std */
#endif /* _OP_UUID_HPP_ */
