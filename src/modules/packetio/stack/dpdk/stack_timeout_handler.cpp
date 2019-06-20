#include <chrono>
#include <stdexcept>
#include <string>
#include <unistd.h>

#include <sys/timerfd.h>

#include "lwip/sys.h"

#include "packetio/stack/dpdk/stack_timeout_handler.h"
#include "packetio/stack/tcpip_handlers.h"
#include "core/icp_log.h"

namespace icp::packetio::dpdk {

stack_timeout_handler::stack_timeout_handler()
    : m_fd(timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK))
{
    if (m_fd == -1) {
        throw std::runtime_error("Could not create kernel timer: "
                                 + std::string(strerror(errno)));
    }

    /* Set our initial timeout */
    handle_event();
}

stack_timeout_handler::~stack_timeout_handler()
{
    close(m_fd);
}

uint32_t stack_timeout_handler::poll_id() const
{
    return (std::numeric_limits<uint32_t>::max() - 1);
}

static constexpr timespec duration_to_timespec(std::chrono::milliseconds ms)
{
    return (timespec {
            .tv_sec  = std::chrono::duration_cast<std::chrono::seconds>(ms).count(),
            .tv_nsec = std::chrono::duration_cast<std::chrono::nanoseconds>(ms).count() });
}

void stack_timeout_handler::handle_event()
{
    std::chrono::milliseconds sleeptime = tcpip::handle_timeouts();

    auto wake_up = itimerspec {
        .it_interval = { 0, 0 },
        .it_value = duration_to_timespec(sleeptime)
    };

    if (timerfd_settime(m_fd, 0, &wake_up, nullptr) == -1) {
        throw std::runtime_error("Could not rearm kernel timer: "
                                 + std::string(strerror(errno)));
    }
}

int stack_timeout_handler::event_fd() const
{
    return (m_fd);
}

static void read_timerfd(int fd, void* arg __attribute__((unused)))
{
    uint64_t count;
    if (read(fd, &count, sizeof(count)) == -1) {
        ICP_LOG(ICP_LOG_WARNING, "Spurious stack timeout\n");
    }
}

pollable_event<stack_timeout_handler>::event_callback
stack_timeout_handler::event_callback_function() const
{
    return (read_timerfd);
}

}
