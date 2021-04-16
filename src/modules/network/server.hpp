#ifndef _OP_NETWORK_SERVER_HPP_
#define _OP_NETWORK_SERVER_HPP_

#include <memory>

#include "framework/core/op_socket.h"

#include "network/api.hpp"
#include "network/generator_stack.hpp"
#include "network/internal_server_stack.hpp"

namespace openperf::network::api {

class server
{
private:
    internal::generator_stack m_generator_stack;
    network::internal::server_stack m_server_stack;
    std::unique_ptr<void, op_socket_deleter> m_socket;

public:
    server(void* context, core::event_loop& loop);

    reply_msg handle_request(const request::generator::list&);
    reply_msg handle_request(const request::generator::get&);
    reply_msg handle_request(const request::generator::create&);
    reply_msg handle_request(const request::generator::erase&);
    reply_msg handle_request(const request::generator::bulk::create&);
    reply_msg handle_request(const request::generator::bulk::erase&);
    reply_msg handle_request(const request::generator::start&);
    reply_msg handle_request(const request::generator::stop&);
    reply_msg handle_request(const request::generator::bulk::start&);
    reply_msg handle_request(const request::generator::bulk::stop&);
    reply_msg handle_request(const request::generator::toggle&);
    reply_msg handle_request(const request::statistic::list&);
    reply_msg handle_request(const request::statistic::get&);
    reply_msg handle_request(const request::statistic::erase&);
    reply_msg handle_request(const request::server::list&);
    reply_msg handle_request(const request::server::get&);
    reply_msg handle_request(const request::server::create&);
    reply_msg handle_request(const request::server::erase&);
    reply_msg handle_request(const request::server::bulk::create&);
    reply_msg handle_request(const request::server::bulk::erase&);
};

} // namespace openperf::network::api

#endif // _OP_NETWORK_SERVER_HPP_
