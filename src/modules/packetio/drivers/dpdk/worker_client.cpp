#include <cstring>
#include <random>

#include <zmq.h>

#include "core/icp_core.h"
#include "packetio/drivers/dpdk/worker_api.h"

namespace icp {
namespace packetio {
namespace dpdk {
namespace worker {

static zmq_msg_t make_message(size_t extra = 0)
{
    zmq_msg_t msg;
    if (zmq_msg_init_size(&msg, sizeof(message) + extra) == -1) {
        throw std::runtime_error("Could not initialize 0MQ message: "
                                 + std::string(zmq_strerror(errno)));
    }
    return (msg);
}

static zmq_msg_t create_message(const start_msg& start)
{
    zmq_msg_t msg = make_message(start.endpoint.size());
    message* m = reinterpret_cast<message*>(zmq_msg_data(&msg));
    m->type = message_type::START;
    m->endpoint_size = start.endpoint.size() + 1;
    std::strcpy(const_cast<char*>(m->endpoint), start.endpoint.c_str());

    return (msg);
}

static zmq_msg_t create_message(const stop_msg& stop)
{
    zmq_msg_t msg = make_message(stop.endpoint.size());
    message* m = reinterpret_cast<message*>(zmq_msg_data(&msg));
    m->type = message_type::STOP;
    m->endpoint_size = stop.endpoint.size() + 1;
    std::strcpy(const_cast<char*>(m->endpoint), stop.endpoint.c_str());

    return (msg);
}

static zmq_msg_t create_message(const configure_msg& config)
{
    auto descriptors_bytes = sizeof(queue::descriptor) * config.descriptors.size();
    zmq_msg_t msg = make_message(descriptors_bytes);
    message *m = reinterpret_cast<message*>(zmq_msg_data(&msg));
    m->type = message_type::CONFIG;
    m->descriptors_size = config.descriptors.size();
    std::memcpy(const_cast<queue::descriptor*>(m->descriptors),
                config.descriptors.data(), descriptors_bytes);

    return (msg);
}

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

static void send_message(void *socket, zmq_msg_t& msg)
{
    /*
     * Note: sendmsg takes ownership of the messages's buffer, so we only
     * need to clean up on errors.
     */
    int length_or_err = zmq_msg_send(&msg, socket, 0);
    if (length_or_err == -1) {
        zmq_msg_close(&msg);
        if (errno != ETERM) {
            throw std::runtime_error("Could not send 0MQ message: "
                                     + std::string(zmq_strerror(errno)));
        }
    }
}

client::client(void *context, size_t nb_workers)
    : m_context(context)
    , m_workers(nb_workers)
    , m_socket(icp_socket_get_server(context, ZMQ_PUB, endpoint.c_str()))
{}

void client::start()
{
    auto syncpoint = random_endpoint();
    auto msg = create_message(start_msg{ .endpoint = syncpoint });
    auto sync = icp_task_sync_socket(const_cast<void*>(m_context), syncpoint.c_str());

    send_message(m_socket.get(), msg);

    icp_task_sync_block_and_warn(&sync, m_workers, 1000,
                                 "Still waiting on start acknowledgment from queue workers "
                                 "(syncpoint = %s)\n", syncpoint.c_str());
}

void client::stop()
{
    auto syncpoint = random_endpoint();
    auto msg = create_message(stop_msg{ .endpoint = syncpoint });
    auto sync = icp_task_sync_socket(const_cast<void*>(m_context), syncpoint.c_str());

    send_message(m_socket.get(), msg);

    icp_task_sync_block_and_warn(&sync, m_workers, 1000,
                                 "Still waiting on stop acknowledgment from queue workers "
                                 "(syncpoint = %s)\n", syncpoint.c_str());
}

void client::configure(const std::vector<queue::descriptor>& descriptors)
{
    auto msg = create_message(configure_msg{ .descriptors = descriptors });
    send_message(m_socket.get(), msg);
}

}
}
}
}
