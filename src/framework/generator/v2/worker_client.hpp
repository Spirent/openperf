#ifndef _OP_GENERATOR_WORKER_CLIENT_HPP_
#define _OP_GENERATOR_WORKER_CLIENT_HPP_

#include <string>
#include <memory>

#include "core/op_socket.h"

namespace openperf::generator::worker {

template <typename Derived, typename ResultType, typename LoadType> class client
{
    std::unique_ptr<void, op_socket_deleter> m_socket;

public:
    client(void* context, std::string_view endpoint);

    std::string endpoint() const;

    void start(void* context, ResultType* result, unsigned nb_workers);
    void stop(void* context, unsigned nb_workers);
    void terminate();
    void update(LoadType&& load);
};

} // namespace openperf::generator::worker

#endif /* _OP_GENERATOR_WORKER_CLIENT_HPP_ */
