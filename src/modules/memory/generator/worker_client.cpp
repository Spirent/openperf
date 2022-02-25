#include "generator/v2/worker_client.tcc"
#include "message/serialized_message.hpp"
#include "memory/generator/worker_api.hpp"

namespace openperf::generator::worker {
template class client<memory::generator::worker::client_impl,
                      memory::generator::result,
                      memory::generator::worker::io_load>;
} // namespace openperf::generator::worker

namespace openperf::memory::generator::worker {

int client_impl::send_start(void* socket,
                            std::string_view syncpoint,
                            result* result) const
{
    return (message::send(
        socket, serialize(start_msg{std::string(syncpoint), result})));
}

int client_impl::send_stop(void* socket, std::string_view syncpoint) const
{
    return (message::send(socket, serialize(stop_msg{std::string(syncpoint)})));
}

int client_impl::send_terminate(void* socket) const
{
    return (message::send(socket, serialize(term_msg{})));
}

int client_impl::send_update(void* socket, io_load&& load) const
{
    return (message::send(socket, serialize(update_msg{std::move(load)})));
}

} // namespace openperf::memory::generator::worker
