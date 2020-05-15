#include <poll.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <climits>

#include "packet/capture/pistache_utils.hpp"
#include "core/op_core.h"

namespace openperf::pistache_utils {

static int gettid() { return (static_cast<int>(syscall(__NR_gettid))); }

bool write_status_line(std::ostream& os,
                       Pistache::Http::Version version,
                       Pistache::Http::Code code)
{
    os << version << ' ' << static_cast<int>(code) << ' ' << code
       << Pistache::Http::crlf;

    return !os.bad();
}

bool write_headers(std::ostream& os,
                   const Pistache::Http::Header::Collection& headers)
{
    for (const auto& header : headers.list()) {
        os << header->name() << ": ";
        header->write(os);
        os << Pistache::Http::crlf;
        if (!os) return false;
    }

    return true;
}

bool write_cookies(std::ostream& os, const Pistache::Http::CookieJar& cookies)
{
    for (const auto& cookie : cookies) {
        os << "Set-Cookie: " << cookie << Pistache::Http::crlf;
        if (!os) return false;
    }

    return true;
}

std::string get_chunk_header_str(size_t chunk_size)
{
    std::ostringstream os;
    os << std::hex << chunk_size << Pistache::Http::crlf;
    return os.str();
}

std::string get_chunk_trailer_str()
{
    std::ostringstream os;
    os << Pistache::Http::crlf;
    return os.str();
}

std::string get_last_chunk_str()
{
    std::ostringstream os;
    os << std::hex << 0 << Pistache::Http::crlf << Pistache::Http::crlf;
    return os.str();
}

ssize_t
send_to_peer(Pistache::Tcp::Peer& peer, const void* data, size_t len, int flags)
{
    int result;
#ifdef PISTACHE_USE_SSL
    if (peer->ssl() != NULL) {
        result = SSL_write((SSL*)peer.ssl(), data, len);
        if (result == 0) {
            auto error_code = SSL_get_error((SSL*)peer.ssl(), result);
            if (error_code != SSL_ERROR_WANT_WRITE) { result = -1; }
        }
    } else {
#endif /* PISTACHE_USE_SSL */
        result = ::send(peer.fd(), data, len, flags);
        if (result < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) { result = 0; }
        }
#ifdef PISTACHE_USE_SSL
    }
#endif /* PISTACHE_USE_SSL */
    return result;
}

ssize_t
send_to_peer_timeout(Pistache::Tcp::Peer& peer,
                     const void* data,
                     size_t len,
                     int flags,
                     const std::chrono::duration<int64_t, std::milli>& timeout)
{
    size_t total_sent = 0;

    while (total_sent < len) {
        auto remain = (len - total_sent);
        auto nsent =
            send_to_peer(peer,
                         reinterpret_cast<void*>(
                             reinterpret_cast<uintptr_t>(data) + total_sent),
                         remain,
                         flags);
        if (nsent < 0) {
            // Unable to send
            break;
        }
        if (nsent == 0) {
            // Socket is full
            int msec = std::min(timeout.count(), int64_t(INT_MAX));
            if (msec == 0) { return total_sent; }

            pollfd pfd{peer.fd(), POLLOUT, 0};
            if (::poll(&pfd, 1, msec) != 1) { return total_sent; }
        }
        total_sent += nsent;
    }
    return total_sent;
}

Pistache::Tcp::Transport* get_transport(Pistache::Http::ResponseWriter& writer)
{
    // HACK!!!!!
    //
    // The ResponseWriter class has a private transport_ member variable
    // but doesn't provide any way to get at it.
    //
    // As a work around, a class is defined with identical memory layout
    // and all of the members exposed as public members.
    //
    class MockResponseWriter : public Pistache::Http::Response
    {
    public:
        std::weak_ptr<Pistache::Tcp::Peer> peer_;
        Pistache::DynamicStreamBuf buf_;
        Pistache::Tcp::Transport* transport_;
        Pistache::Http::Timeout timeout_;
    };
    static_assert(sizeof(MockResponseWriter)
                  == sizeof(Pistache::Http::ResponseWriter));

    // Need to use reinterpret_cast because using a fake type
    auto transport = reinterpret_cast<MockResponseWriter*>(&writer)->transport_;

    OP_LOG(OP_LOG_DEBUG,
           "Retrived Transport %p from ResponseWriter.",
           (void*)transport);

    assert(transport);

    return transport;
}

void transport_peer_disable(Pistache::Tcp::Transport& transport,
                            Pistache::Tcp::Peer& peer)
{
    OP_LOG(OP_LOG_DEBUG,
           "Disabling Pistache peer fd %d. (current tid=%d)",
           peer.fd(),
           gettid());

    // Ideally we would remove the peer from the reactor, but there is no
    // easy way to do that without major modifications to Pistache, so
    // instead just disable the fd so the reactor won't touch it.
    transport.reactor()->modifyFd(transport.key(),
                                  peer.fd(),
                                  Pistache::Polling::NotifyOn::None,
                                  Pistache::Polling::Mode::Edge);
}

void transport_peer_enable(Pistache::Tcp::Transport& transport,
                           Pistache::Tcp::Peer& peer)
{
    OP_LOG(OP_LOG_DEBUG,
           "Enabling Pistache peer fd %d. (current tid=%d)",
           peer.fd(),
           gettid());

    transport.reactor()->modifyFd(transport.key(),
                                  peer.fd(),
                                  Pistache::Polling::NotifyOn::Read,
                                  Pistache::Polling::Mode::Edge);
}

void transport_exec(Pistache::Tcp::Transport& transport,
                    std::function<void()>&& func)
{
    auto timer_fd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK);

    assert(timer_fd > 0);
    if (timer_fd < 0) {
        OP_LOG(OP_LOG_ERROR, "Could not create timerfd");
        return;
    }

    OP_LOG(OP_LOG_DEBUG,
           "Arming timer to invoke function from transport thread. (current "
           "tid=%d)",
           gettid());

    auto result = Pistache::Async::Promise<uint64_t>(
        [&](Pistache::Async::Deferred<uint64_t> deferred) {
            transport.armTimer(timer_fd,
                               std::chrono::duration<int64_t, std::milli>{1},
                               std::move(deferred));
        });
    result.then([=]([[maybe_unused]] uint64_t numWakeup) { func(); },
                [=](std::exception_ptr exc) {
                    std::rethrow_exception(std::move(exc));
                });

    OP_LOG(OP_LOG_DEBUG,
           "Waiting for timer function to complete. (current tid=%d)",
           gettid());

    Pistache::Async::Barrier<uint64_t> barrier(result);
    barrier.wait();

    OP_LOG(OP_LOG_DEBUG, "Timer completed. (current tid=%d)", gettid());

    close(timer_fd);
}

} // namespace openperf::pistache_utils
