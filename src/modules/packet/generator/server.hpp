#ifndef _OP_PACKET_GENERATOR_SERVER_HPP_
#define _OP_PACKET_GENERATOR_SERVER_HPP_

#include <map>
#include <vector>

#include "core/op_core.h"
#include "packet/generator/api.hpp"
#include "packet/generator/source.hpp"
#include "packetio/internal_client.hpp"

namespace openperf::packet::generator::api {

class server
{
public:
    server(void* context, core::event_loop& loop);

    reply_msg handle_request(const request_list_generators&);
    reply_msg handle_request(const request_create_generator&);
    reply_msg handle_request(const request_delete_generators&);

    reply_msg handle_request(const request_get_generator&);
    reply_msg handle_request(const request_delete_generator&);

    reply_msg handle_request(const request_start_generator&);
    reply_msg handle_request(const request_stop_generator&);

    reply_msg handle_request(const request_toggle_generator&);

    reply_msg handle_request(const request_list_generator_results&);
    reply_msg handle_request(const request_delete_generator_results&);
    reply_msg handle_request(const request_get_generator_result&);
    reply_msg handle_request(const request_delete_generator_result&);

    reply_msg handle_request(const request_list_tx_flows&);
    reply_msg handle_request(const request_get_tx_flow&);

    /*
     * Source's know their own id's, so we don't need to store
     * them in a map.  Just use a (sorted) vector so we don't
     * store id's twice.
     */
    enum source_state { none = 0, idle, active };
    using source_value_type = packetio::packet::generic_source;
    using source_item = std::pair<source_value_type, source_state>;

    /*
     * Results don't know their id's, so we store them in an associative
     * container with the id as the key. Futhermore, we (the server)
     * own the results.  We just let the source borrow them for a while.
     */
    using result_value_type = source_result;
    using result_value_ptr = std::unique_ptr<result_value_type>;
    using result_map = std::map<core::uuid, result_value_ptr>;

private:
    core::event_loop& m_loop;
    packetio::internal::api::client m_client;
    std::unique_ptr<void, op_socket_deleter> m_socket;

    std::vector<source_item> m_sources;

    /* result id --> result */
    result_map m_results;
};

} // namespace openperf::packet::generator::api

#endif /* _OP_PACKET_GENERATOR_SERVER_HPP_ */
