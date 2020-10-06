#ifndef _OP_PACKET_STACK_NETIF_UTILS_HPP_
#define _OP_PACKET_STACK_NETIF_UTILS_HPP_

#include <cstdint>
#include <string>

#include "lwip/netif.h"

namespace openperf::packet::stack {

uint8_t netif_id_match(std::string_view id_str);

err_t netif_inject(struct netif*, void* packet);

}

#endif /* _OP_PACKET_STACK_NETIF_UTILS_HPP_ */
