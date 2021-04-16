#ifndef _OP_NETWORK_INTERNAL_SERVER_HPP_
#define _OP_NETWORK_INTERNAL_SERVER_HPP_

#include <memory>

#include "models/server.hpp"

namespace openperf::network::internal {

namespace firehose {
class server;
}

class server : public model::server
{
public:
    server(const model::server&);
    ~server() override;
    void reset();
    stat_t stat() const override;

private:
    std::unique_ptr<firehose::server> server_ptr;
};

} // namespace openperf::network::internal

#endif // _OP_CPU_GENERATOR_HPP_
