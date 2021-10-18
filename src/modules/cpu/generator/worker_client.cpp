#include "generator/v2/worker_client.tcc"
#include "message/serialized_message.hpp"
#include "cpu/generator/worker_api.hpp"

namespace openperf::generator::worker {
template class client<cpu::generator::worker::client_impl,
                      cpu::generator::result,
                      std::vector<double>>;
} // namespace openperf::generator::worker

namespace openperf::cpu::generator::worker {

int client_impl::send_start(void* socket,
                            std::string_view syncpoint,
                            result* result) const
{
    return (message::send(
        socket, serialize_command(start_msg{std::string(syncpoint), result})));
}

int client_impl::send_stop(void* socket, std::string_view syncpoint) const
{
    return (message::send(socket,
                          serialize_command(stop_msg{std::string(syncpoint)})));
}

int client_impl::send_terminate(void* socket) const
{
    return (message::send(socket, serialize_command(term_msg{})));
}

int client_impl::send_update(void* socket, std::vector<double>&& load) const
{
    return (
        message::send(socket, serialize_command(update_msg{std::move(load)})));
}

} // namespace openperf::cpu::generator::worker
