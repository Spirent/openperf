#ifndef _OP_PACKET_STACK_NETIF_UTILS_HPP_
#define _OP_PACKET_STACK_NETIF_UTILS_HPP_

#include <cstdint>
#include <optional>
#include <string>

#include "lwip/netif.h"

namespace openperf::packet::stack {

const struct netif* netif_get_by_name(std::string_view id_str);

uint8_t netif_index_by_name(std::string_view id_str);

std::string interface_id_by_netif(const struct netif*);

err_t netif_inject(struct netif*, void* packet);

}

#endif /* _OP_PACKET_STACK_NETIF_UTILS_HPP_ */
