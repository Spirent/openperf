#include <algorithm>
#include <chrono>

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>

#include "timesync/chrono.hpp"
#include "timesync/socket.hpp"

namespace openperf::timesync::ntp {

socket::socket(const addrinfo* ai)
    : m_fd(::socket(ai->ai_family, SOCK_DGRAM, IPPROTO_UDP))
{
    if (m_fd == -1) {
        throw std::runtime_error("Could not create socket: "
                                 + std::string(strerror(errno)));
    }

    /* Try to enable socket time stamping */
    int enable = 1;
    if (setsockopt(*m_fd, SOL_SOCKET, SO_TIMESTAMP, &enable, sizeof(enable))
        == -1) {
        close(*m_fd);
        throw std::runtime_error("Could not enable socket time stamping: "
                                 + std::string(strerror(errno)));
    }

    /* Finally, connect the socket */
    if (connect(*m_fd, ai->ai_addr, ai->ai_addrlen) == -1) {
        close(*m_fd);
        throw std::runtime_error("Could not connect socket: "
                                 + std::string(strerror(errno)));
    }
}

socket::~socket()
{
    if (m_fd) { close(*m_fd); }
}

socket::socket(socket&& other) noexcept
    : m_fd(other.m_fd)
{
    other.m_fd.reset();
}

socket& socket::operator=(socket&& other) noexcept
{
    if (this != &other) { m_fd.swap(other.m_fd); }
    return (*this);
}

int socket::fd() const { return (m_fd.value_or(-1)); }

tl::expected<counter::ticks, int> socket::send(const std::byte buffer[],
                                               size_t length)
{
    assert(m_fd.has_value());

    int flags = 0;
#if defined(MSG_NOSIGNAL)
    flags |= MSG_NOSIGNAL;
#endif

    auto now = counter::now();
    if (::send(*m_fd, buffer, length, flags) == -1) {
        return (tl::make_unexpected(errno));
    }
    return (now);
}

/*
 * Calculate the clock offset between the host's gettimeofday clock
 * and our timecounter based reference clock.
 * To do this we use the following algorithm, based on the technique
 * described in MoonGen: A Scriptable High-Speed Packet Generator:
 *
 * 1. Read the time from each clock.
 * 2. Repeat in the opposite order.
 * 3. Calculate the average delta between the clocks
 * 4. Repeat this calculation a few times
 * 5. Report the minimum (absolute) offset
 *
 * The repeated trials are to lessen the chance of random delays
 * in reading the clock values.
 */
static constexpr int offset_calc_trials = 3;
static bintime get_clock_offset()
{
    std::array<std::chrono::nanoseconds, offset_calc_trials> offsets;

    std::generate_n(offsets.data(), offset_calc_trials, []() {
        using sys_clock = std::chrono::system_clock;
        using ref_clock = timesync::chrono::monotime;
        auto x1 = sys_clock::now();
        auto x2 = ref_clock::now();
        auto y1 = ref_clock::now();
        auto y2 = sys_clock::now();

        auto delta = ((y2 - x1) - (y1 - x2)) / 2;
        auto offset = x1.time_since_epoch() - x2.time_since_epoch();

        return (offset + delta);
    });

    std::sort(std::begin(offsets),
              std::end(offsets),
              [](const auto& left, const auto& right) {
                  return (std::chrono::abs(left) < std::chrono::abs(right));
              });

    return (to_bintime(offsets[0]));
}

/*
 * Convert a timestamp from the host's time-of-day clock to a tick value from
 * our reference counter. We do that by applying the following algorithm:
 * 1. Get the current offset between the host and our reference clock.
 * 2. Use the offset to convert the input timestamp from the host time scale
 *    to our timecounter time scale.
 * 3. Get the current timecounter time and the tick count for that time.
 * 4. Calculate the delta between the corrected timestamp and the reference
 *    time.
 * 5. Apply the delta to the tick reference.
 *
 * While rather cumbersome, it's the best way to timestamp incoming packets,
 * since the only way the OS can timestamp the receive time is with its own
 * clock.  And using the OS clock eliminates the uncertain delay between when
 * the receive interrupt fires and when our socket gets woken up.
 */
static counter::ticks to_ticks(const timeval& tv)
{
    auto local_ts = to_bintime(tv) - get_clock_offset();
    auto [ref_time, ref_ticks] = chrono::monotime_ticks();
    auto delta = ref_time - local_ts;
    auto freq = counter::frequency().count();
    auto dt = freq * delta.bt_sec + ((freq * (delta.bt_frac >> 32)) >> 32);
    auto ticks = ref_ticks - dt;

    /* If this isn't true, we really screwed up somewhere */
    assert(ticks < ref_ticks);

    return (ticks);
}

tl::expected<counter::ticks, int> socket::recv(std::byte buffer[],
                                               size_t& length)
{
    assert(m_fd.has_value());

    auto storage = sockaddr_storage{};
    auto iov = iovec{.iov_base = buffer, .iov_len = length};
    std::array<std::byte, 1024> ctrl;

    auto msg = msghdr{.msg_name = std::addressof(storage),
                      .msg_namelen = sizeof(storage),
                      .msg_iov = std::addressof(iov),
                      .msg_iovlen = 1,
                      .msg_control = ctrl.data(),
                      .msg_controllen = ctrl.size(),
                      .msg_flags = 0};

    /*
     * Make sure we have a timestamp ready in case we fail to
     * receive a timestamp from the socket.
     */
    auto timestamp = counter::now();
    auto recv_or_err = recvmsg(*m_fd, &msg, 0);
    if (recv_or_err == -1) { return (tl::make_unexpected(errno)); }
    length = recv_or_err;

    for (auto cmsg = CMSG_FIRSTHDR(&msg); cmsg != nullptr;
         cmsg = CMSG_NXTHDR(&msg, cmsg)) {
        if (cmsg->cmsg_level == SOL_SOCKET && cmsg->cmsg_type == SCM_TIMESTAMP
            && cmsg->cmsg_len >= sizeof(timeval)) {
            /*
             * We have a timestamp from the kernel, but unfortunately, it was
             * made with the wrong clock. We need to convert it to our native
             * tick format.
             */
            auto tv = *(reinterpret_cast<timeval*>(CMSG_DATA(cmsg)));
            timestamp = to_ticks(tv);
        }
    }

    return (timestamp);
}

} // namespace openperf::timesync::ntp
