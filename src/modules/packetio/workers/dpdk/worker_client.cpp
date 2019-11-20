#include <cstring>
#include <random>

#include <zmq.h>

#include "core/op_core.h"
#include "packetio/workers/dpdk/worker_api.hpp"

namespace openperf::packetio::dpdk::worker {

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
    std::generate_n(std::back_inserter(to_return), 8,
                    [&]{ return chars[dist(generator)]; });

    return (to_return);
}

client::client(void *context)
    : m_socket(op_socket_get_server(context, ZMQ_PUB, endpoint.data()))
{}

void client::start(void* context, unsigned nb_workers)
{
    auto syncpoint = random_endpoint();
    auto sync = op_task_sync_socket(context, syncpoint.c_str());
    if (auto error = send_message(m_socket.get(), start_msg{ syncpoint });
        error && error != ETERM) {
        throw std::runtime_error("Could not send start message to workers: "
                                 + std::string(zmq_strerror(error)));
    }
    op_task_sync_block_and_warn(&sync, nb_workers, 1000,
                                 "Still waiting on start acknowledgment from queue workers "
                                 "(syncpoint = %s)\n", syncpoint.c_str());
}

void client::stop(void* context, unsigned nb_workers)
{
    auto syncpoint = random_endpoint();
    auto sync = op_task_sync_socket(context, syncpoint.c_str());
    if (auto error = send_message(m_socket.get(), stop_msg{ syncpoint });
        error && error != ETERM) {
        throw std::runtime_error("Could not send stop message to workers: "
                                 + std::string(zmq_strerror(error)));
    }
    op_task_sync_block_and_warn(&sync, nb_workers, 1000,
                                 "Still waiting on stop acknowledgment from queue workers "
                                 "(syncpoint = %s)\n", syncpoint.c_str());
}

void client::add_descriptors(const std::vector<descriptor>& descriptors)
{
    if (auto error = send_message(m_socket.get(), add_descriptors_msg { descriptors });
        error && error != ETERM) {
        throw std::runtime_error("Could not send add descriptors message to workers: "
                                 + std::string(zmq_strerror(error)));
    }
}

void client::del_descriptors(const std::vector<descriptor>& descriptors)
{
    if (auto error = send_message(m_socket.get(), del_descriptors_msg { descriptors });
        error && error != ETERM) {
        throw std::runtime_error("Could not send delete descriptors message to workers: "
                                 + std::string(zmq_strerror(error)));
    }
}

}
