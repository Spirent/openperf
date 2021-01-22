#include <cerrno>
#include <system_error>

#include "socket/server/generic_socket.hpp"
#include "socket/server/packet_socket.hpp"
#include "socket/server/socket_utils.hpp"

namespace openperf::socket::server {

tl::expected<generic_socket, int>
make_socket(openperf::socket::server::allocator& allocator,
            int domain,
            int type,
            int protocol)
{
    try {
        if (domain == AF_PACKET) {
            switch (type & 0xff) {
            case SOCK_DGRAM:
            case SOCK_RAW:
                return (
                    generic_socket(packet_socket(allocator, type, protocol)));
            default:
                return (tl::make_unexpected(EACCES));
            }
        }

        return (make_socket_common(allocator, domain, type, protocol));
    } catch (const std::system_error& e) {
        return (tl::make_unexpected(e.code().value()));
    }
}

} // namespace openperf::socket::server
