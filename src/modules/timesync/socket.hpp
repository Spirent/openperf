#ifndef _OP_TIMESYNC_SOCKET_HPP_
#define _OP_TIMESYNC_SOCKET_HPP_

#include <cstddef>
#include <optional>

#include "tl/expected.hpp"

#include "timesync/chrono.hpp"
#include "timesync/counter.hpp"

struct addrinfo;

namespace openperf::timesync::ntp {

class socket
{
    std::optional<int> m_fd = std::nullopt;

public:
    socket(const addrinfo* ai);
    ~socket();

    socket(const socket&) = delete;
    socket& operator=(const socket&) = delete;

    socket(socket&&) noexcept;
    socket& operator=(socket&&) noexcept;

    int fd() const;

    tl::expected<counter::ticks, int> send(const std::byte buffer[],
                                           size_t length);
    tl::expected<counter::ticks, int> recv(std::byte buffer[], size_t& length);
};

} // namespace openperf::timesync::ntp

#endif /* _OP_TIMESYNC_SOCKET_HPP_ */
