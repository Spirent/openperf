#ifndef _OP_PACKET_ANALYZER_SERVER_HPP_
#define _OP_PACKET_ANALYZER_SERVER_HPP_

#include <map>
#include <vector>

#include "core/op_core.h"
#include "packet/analyzer/api.hpp"
#include "packet/analyzer/sink.hpp"
#include "packetio/internal_client.hpp"

namespace openperf::packet::analyzer::api {

class server
{
    core::event_loop& m_loop;
    packetio::internal::api::client m_client;
    std::unique_ptr<void, op_socket_deleter> m_socket;

    /*
     * Since sinks know their own id's, we store them in a
     * vector.  That way we don't needlessly store each id
     * twice.
     */
    using sink_value_type = packetio::packets::generic_sink;
    std::vector<sink_value_type> m_sinks;

    /*
     * Analyzer results don't know their own id's, so we store
     * those in an associative container with the id as the key.
     * Furthermore, we (the server) own the results.  The sink
     * just borrows a result instance when running.
     */
    using result_value_type = sink_result;
    using result_value_ptr = std::unique_ptr<result_value_type>;
    using result_map = std::map<core::uuid, result_value_ptr>;

    /* result id --> result */
    result_map m_results;

public:
    server(void* context, core::event_loop& loop);

    reply_msg handle_request(const request_list_analyzers&);
    reply_msg handle_request(const request_create_analyzer&);
    reply_msg handle_request(const request_delete_analyzers&);

    reply_msg handle_request(const request_get_analyzer&);
    reply_msg handle_request(const request_delete_analyzer&);

    reply_msg handle_request(const request_start_analyzer&);
    reply_msg handle_request(const request_stop_analyzer&);

    reply_msg handle_request(const request_list_analyzer_results&);
    reply_msg handle_request(const request_delete_analyzer_results&);
    reply_msg handle_request(const request_get_analyzer_result&);
    reply_msg handle_request(const request_delete_analyzer_result&);

    reply_msg handle_request(const request_list_rx_streams&);
    reply_msg handle_request(const request_get_rx_stream&);
};

} // namespace openperf::packet::analyzer::api

#endif /* _OP_PACKET_ANALYZER_SERVER_HPP_ */
