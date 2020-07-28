#include "packetio/drivers/dpdk/mbuf_tx.hpp"
#include "packetio/drivers/dpdk/port/tx_sink.hpp"

namespace openperf::packetio::dpdk::port {

static uint16_t tx_sink_dummy_callback([[maybe_unused]] uint16_t port_id,
                                       [[maybe_unused]] uint16_t queue_id,
                                       [[maybe_unused]] rte_mbuf* packets[],
                                       [[maybe_unused]] uint16_t nb_packets,
                                       [[maybe_unused]] void* user_param)
{
    return nb_packets;
}

static auto g_tx_sink_callback = tx_sink_dummy_callback;

std::string callback_tx_sink::description() { return ("Tx sink"); }

tx_callback<tx_sink>::tx_callback_fn callback_tx_sink::callback()
{
    return (g_tx_sink_callback);
}

void* callback_tx_sink::callback_arg() const { return nullptr; }

static tx_sink::variant_type make_tx_sink_feature(uint16_t port_id)
{
    return (callback_tx_sink(port_id));
}


tx_sink::tx_sink(uint16_t port_id)
    : feature(make_tx_sink_feature(port_id))
{}

void tx_sink::set_callback(tx_sink::tx_callback_fn tx_sink_callback)
{
    g_tx_sink_callback = tx_sink_callback;
}

} // namespace openperf::packetio::dpdk::port
