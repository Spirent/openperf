#include <limits>

#include "core/icp_core.h"
#include "drivers/dpdk/model/port_info.h"

namespace icp {
namespace packetio {
namespace dpdk {
namespace model {

port_info_flag operator | (port_info_flag a, port_info_flag b)
{
    return static_cast<port_info_flag>(static_cast<int>(a) | static_cast<int>(b));
}

port_info_flag operator & (port_info_flag a, port_info_flag b)
{
    return static_cast<port_info_flag>(static_cast<int>(a) & static_cast<int>(b));
}

port_info::port_info(const char *name)
{
    /**
     * Bwa ha ha!  This is very not efficient.
     */
    bool found = false;
    std::vector<std::unique_ptr<port_info_data>> all_info;
    port_info_data::make_all(all_info);
    for (auto& info : all_info) {
        if (info->name == name) {
            found = true;
            _data = std::move(info);
            break;
        }
    }

    if (!_data) {
        icp_log(ICP_LOG_WARNING, "No explicit configuration found for driver = %s."
                "  Using defaults.\n", name);
    }
}

uint16_t port_info::rx_queue_count() const
{
    return (_data ? _data->nb_rx_queues : std::numeric_limits<uint16_t>::max());
}

uint16_t port_info::tx_queue_count() const
{
    return (_data ? _data->nb_tx_queues : std::numeric_limits<uint16_t>::max());
}

uint16_t port_info::rx_desc_count() const
{
    return (_data ? _data->nb_rx_desc : 1024);
}

uint16_t port_info::tx_desc_count() const
{
    return (_data ? _data->nb_tx_desc : 256);
}

bool port_info::lsc_interrupt() const
{
    return (_data ?
            (_data->flags & port_info_flag::LSC_INTR) == port_info_flag::LSC_INTR
            : false);
}

bool port_info::rxq_interrupt() const
{
    return (_data ?
            (_data->flags & port_info_flag::RXQ_INTR) == port_info_flag::RXQ_INTR
            : false);
}

struct af_packet_config : public port_info_data::registrar<af_packet_config>
{
    af_packet_config()
    {
        name = "net_af_packet";
        nb_tx_queues = std::numeric_limits<uint16_t>::max();
        nb_rx_queues = std::numeric_limits<uint16_t>::max();
        nb_rx_desc = 0;
        nb_tx_desc = 0;
        flags = port_info_flag::NONE;
    }
};

struct ring_config : public port_info_data::registrar<ring_config>
{
    ring_config()
    {
        name = "net_ring";
        nb_tx_queues = 1;
        nb_rx_queues = 1;
        nb_rx_desc = 0;
        nb_tx_desc = 0;
        flags = port_info_flag::NONE;
    }
};

struct virtio_config : public port_info_data::registrar<virtio_config>
{
    virtio_config()
    {
        name = "net_virtio";
        nb_tx_queues = 1;
        nb_rx_queues = 1;
        nb_rx_desc = 256;
        nb_tx_desc = 256;
        flags = port_info_flag::LSC_INTR | port_info_flag::RXQ_INTR;
    }
};

struct vmxnet3_config : public port_info_data::registrar<vmxnet3_config>
{
    vmxnet3_config()
    {
        name = "net_vmxnet3";
        nb_tx_queues = 2;
        nb_rx_queues = 2;
        nb_rx_desc = 2048;
        nb_tx_desc = 512;
        flags = port_info_flag::NONE;
    }
};

struct em_config : public port_info_data::registrar<em_config>
{
    em_config()
    {
        name = "net_e1000_em";
        nb_tx_queues = 1;
        nb_rx_queues = 1;
        nb_rx_desc = 1024;
        nb_tx_desc = 256;
        flags = port_info_flag::NONE;
    }
};

struct igb_config : public port_info_data::registrar<igb_config>
{
    igb_config()
    {
        name = "net_e1000_igb";
        nb_tx_queues = 1;
        nb_rx_queues = 1;
        nb_rx_desc = 1024;
        nb_tx_desc = 256;
        flags = port_info_flag::LSC_INTR | port_info_flag::RXQ_INTR;
    }
};

struct igb_vf_config : public port_info_data::registrar<igb_vf_config>
{
    igb_vf_config()
    {
        name = "net_e1000_igb_vf";
        nb_tx_queues = 1;
        nb_rx_queues = 1;
        nb_rx_desc = 1024;
        nb_tx_desc = 256;
        flags = port_info_flag::RXQ_INTR;
    }
};

struct ixgbe_config : public port_info_data::registrar<ixgbe_config>
{
    ixgbe_config()
    {
        name= "net_ixgbe";
        nb_tx_queues = 2;
        nb_rx_queues = 2;
        nb_rx_desc = 2048;
        nb_tx_desc = 256;
        flags = port_info_flag::LSC_INTR | port_info_flag::RXQ_INTR;
     }
};

struct ixgbe_vf_config : public port_info_data::registrar<ixgbe_vf_config>
{
    ixgbe_vf_config()
    {
        name = "net_ixgbe_vf";
        nb_tx_queues = 2;
        nb_rx_queues = 2;
        nb_rx_desc = 2048;
        nb_tx_desc = 256;
        flags = port_info_flag::RXQ_INTR;
    }
};

}
}
}
}
