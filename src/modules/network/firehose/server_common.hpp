#ifndef _OP_NETWORK_FIREHOSE_SERVER_COMMON_HPP_
#define _OP_NETWORK_FIREHOSE_SERVER_COMMON_HPP_

#include <atomic>
#include <tl/expected.hpp>
#include <arpa/inet.h>
#include "protocol.hpp"
#include "../drivers/driver.hpp"

namespace openperf::network::internal::firehose {

const static size_t recv_buffer_size = 4096;
const static size_t send_buffer_size = 4096;

enum connection_state_t {
    STATE_NONE = 0, /**< Invalid state */
    STATE_WAITING,  /**< Connection is waiting for client requests */
    STATE_READING,  /**< Connection is reading data from the client */
    STATE_WRITING,  /**< Connection is writting data to the client */
    STATE_ERROR,    /**< Connection has encountered an error */
    STATE_DONE /**< Client has disconnected and connection should be closed */
};

struct connection_t
{
    int fd;
    connection_state_t state;
    size_t bytes_left;
    std::vector<uint8_t> request;
    union
    {
        struct sockaddr client;
        struct sockaddr_in client4;
        struct sockaddr_in6 client6;
    };
};

class server
{
public:
    server() = default;
    server(const server&) = delete;
    virtual ~server() = default;

    virtual void run_accept_thread() = 0;
    virtual void run_worker_thread() = 0;

protected:
    std::atomic_int m_fd;
    int m_port;
    drivers::network_driver_ptr m_driver;
};

inline socklen_t get_sa_len(const struct sockaddr* sa)
{
    switch (sa->sa_family) {
    case AF_INET:
        return sizeof(struct sockaddr_in);
    case AF_INET6:
        return sizeof(struct sockaddr_in6);
    default:
        return sizeof(struct sockaddr);
    }
}

inline in_port_t get_sa_port(const struct sockaddr* sa)
{
    switch (sa->sa_family) {
    case AF_INET:
        return (((struct sockaddr_in*)sa)->sin_port);
    case AF_INET6:
        return (((struct sockaddr_in6*)sa)->sin6_port);
    default:
        return (0);
    }
}

inline void* get_sa_addr(const struct sockaddr* sa)
{
    switch (sa->sa_family) {
    case AF_INET:
        return (&((struct sockaddr_in*)sa)->sin_addr);
    case AF_INET6:
        return (&((struct sockaddr_in6*)sa)->sin6_addr);
    default:
        return (NULL);
    }
}

inline const char* get_state_string(connection_state_t state)
{
#define CASE_MAP(s)                                                            \
    case s:                                                                    \
        return #s
    switch (state) {
        CASE_MAP(STATE_NONE);
        CASE_MAP(STATE_WAITING);
        CASE_MAP(STATE_READING);
        CASE_MAP(STATE_WRITING);
        CASE_MAP(STATE_ERROR);
        CASE_MAP(STATE_DONE);
    default:
        return "UNKNOWN";
    }
#undef CASE_MAP
}

inline int
parse_request(uint8_t*& data, size_t& data_length, connection_t& conn)
{
    firehose::request_t request;

    auto req = firehose::parse_request(data, data_length);
    if (req) {
        switch (req.value().action) {
        case firehose::action_t::GET:
            conn.bytes_left = request.length;
            conn.state = STATE_WRITING;
            break;
        case firehose::action_t::PUT:
            conn.bytes_left = request.length;
            conn.state = STATE_READING;
            break;
        default:
            conn.bytes_left = 0;
            conn.state = STATE_ERROR;
            return -1;
        }
    }

    return 0;
}

} // namespace openperf::network::internal::firehose

#endif