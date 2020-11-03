#ifndef _OP_NETWORK_INTERNAL_SERVER_HPP_
#define _OP_NETWORK_INTERNAL_SERVER_HPP_

#include <atomic>
#include <thread>

#include "modules/dynamic/spool.hpp"
#include "firehose/server_common.hpp"
#include "models/server.hpp"

namespace openperf::network::internal {

class server : public model::server
{
public:
    server(const model::server&);
    ~server() override;
    void reset();

private:
    std::unique_ptr<firehose::server> server_ptr;
};

} // namespace openperf::network::internal

#endif // _OP_CPU_GENERATOR_HPP_
