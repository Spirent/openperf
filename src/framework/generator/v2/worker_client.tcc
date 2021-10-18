#include <algorithm>
#include <cassert>
#include <random>
#include <stdexcept>

#include "zmq.h"

#include "core/op_core.h"
#include "generator/v2/worker_client.hpp"

namespace openperf::generator::worker {

/* XXX: Length is arbitrary */
static constexpr auto max_endpoint_length = 256;

/**
 * Generate a random endpoint for worker synchronization.
 * A random endpoint is useful to prevent races on sequential
 * synchronized messages.
 */
static std::string random_endpoint()
{
    static const std::string_view chars =
        "abcdefghijklmnaoqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890";
    std::mt19937_64 generator{std::random_device()()};
    std::uniform_int_distribution<> dist(0, chars.size() - 1);
    std::string to_return("inproc://worker_sync_");
    std::generate_n(std::back_inserter(to_return), 8, [&] {
        return chars[dist(generator)];
    });

    return (to_return);
}

template <typename Derived, typename ResultType, typename LoadType>
client<Derived, ResultType, LoadType>::client(void* context,
                                              std::string_view endpoint)
    : m_socket(
        op_socket_get_server(context, ZMQ_PUB, std::string(endpoint).c_str()))
{
    assert(endpoint.size() < max_endpoint_length);
}

template <typename Derived, typename ResultType, typename LoadType>
std::string client<Derived, ResultType, LoadType>::endpoint() const
{
    auto endpoint = std::string(max_endpoint_length, '\0');
    auto endpoint_length = endpoint.length();

    if (auto error = zmq_getsockopt(m_socket.get(),
                                    ZMQ_LAST_ENDPOINT,
                                    endpoint.data(),
                                    &endpoint_length)) {
        throw std::runtime_error("Could not retreive ZMQ endpoint: "
                                 + std::string(zmq_strerror(error)));
    }

    assert(endpoint_length < max_endpoint_length);

    endpoint.resize(endpoint_length);
    return (endpoint);
}

template <typename Derived, typename ResultType, typename LoadType>
void client<Derived, ResultType, LoadType>::start(void* context,
                                                  ResultType* result,
                                                  unsigned nb_workers)
{
    auto syncpoint = random_endpoint();
    auto sync = op_task_sync_socket(context, syncpoint.c_str());
    assert(sync);

    const auto& child = static_cast<Derived&>(*this);
    if (auto error = child.send_start(m_socket.get(), syncpoint, result);
        error && error != ETERM) {
        throw std::runtime_error("Could not send start message to workers: "
                                 + std::string(zmq_strerror(error)));
    }

    op_task_sync_block_and_warn(
        &sync,
        nb_workers,
        1000,
        "Still waiting on start acknowledgment from workers "
        "(syncpoint = %s)\n",
        syncpoint.c_str());
}

template <typename Derived, typename ResultType, typename LoadType>
void client<Derived, ResultType, LoadType>::stop(void* context,
                                                 unsigned nb_workers)
{
    auto syncpoint = random_endpoint();
    auto sync = op_task_sync_socket(context, syncpoint.c_str());
    assert(sync);

    const auto& child = static_cast<Derived&>(*this);
    if (auto error = child.send_stop(m_socket.get(), syncpoint);
        error && error != ETERM) {
        throw std::runtime_error("Could not send stop message to workers: "
                                 + std::string(zmq_strerror(error)));
    }

    op_task_sync_block_and_warn(
        &sync,
        nb_workers,
        1000,
        "Still waiting on stop acknowledgment from workers "
        "(syncpoint = %s)\n",
        syncpoint.c_str());
}

template <typename Derived, typename ResultType, typename LoadType>
void client<Derived, ResultType, LoadType>::terminate()
{
    const auto& child = static_cast<Derived&>(*this);
    if (auto error = child.send_terminate(m_socket.get());
        error && error != ETERM) {
        throw std::runtime_error("Could not send terminate message to workers: "
                                 + std::string(zmq_strerror(error)));
    }
}

template <typename Derived, typename ResultType, typename LoadType>
void client<Derived, ResultType, LoadType>::update(LoadType&& loads)
{
    const auto& child = static_cast<Derived&>(*this);
    if (auto error = child.send_update(m_socket.get(), std::move(loads));
        error && error != ETERM) {
        throw std::runtime_error("Could not send update message to workers: "
                                 + std::string(zmq_strerror(error)));
    }
}

} // namespace openperf::generator::worker
