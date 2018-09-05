#ifndef _PACKETIO_NETIF_H_
#define _PACKETIO_NETIF_H_

#include "lwip/err.h"
#include "lwip/netif.h"

err_t packetio_netif_init(struct netif *netif);

#endif /* _PACKETIO_NETIF_H_ */
