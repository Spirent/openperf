#ifndef _ICP_UUID_H_
#define _ICP_UUID_H_

#include <cstdint>
#include <string>

namespace icp::core {

class uuid
{
    alignas(8) uint8_t  m_octets[16];

public:
    static uuid random();  /* constructor for random uuid */

    uuid();
    uuid(const std::string_view);
    uuid(const uint8_t[15]);
    uuid(std::initializer_list<uint8_t>);

    uint8_t operator[](size_t idx) const;
    const uint8_t* data() const;
};

std::string to_string(const uuid&);

} /* namespace icp::core */

namespace std {

template <>
struct hash<icp::core::uuid>
{
    size_t operator()(const icp::core::uuid& uuid) const noexcept
    {
        auto data = reinterpret_cast<const uint64_t*>(uuid.data());
        auto h1 = std::hash<uint64_t>{}(data[0]);
        auto h2 = std::hash<uint64_t>{}(data[1]);
        return (h1 ^ (h2 << 1));
    }
};

} /* namespace std */
#endif /* _ICP_UUID_H_ */
