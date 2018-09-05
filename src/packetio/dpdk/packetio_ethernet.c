#include "core/icp_core.h"
#include "lwip/err.h"
#include "netif/ethernet.h"

const struct eth_addr ethbroadcast = {{0xff, 0xff, 0xff, 0xff, 0xff, 0xff}};
const struct eth_addr ethzero      = {{0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};

err_t ethernet_input(struct pbuf *p, struct netif *netif)
{
    icp_log(ICP_LOG_INFO, "%s: p = %p, netif= %p\n",
            __func__, (void *)p, (void *)netif);
    (void)p;
    (void)netif;
    return (ERR_OK);
}

err_t ethernet_output(struct netif* netif, struct pbuf* p,
                      const struct eth_addr* src, const struct eth_addr* dst,
                      u16_t eth_type)
{
    icp_log(ICP_LOG_INFO, "%s: netif= %p, p = %p\n",
            __func__, (void *)netif, (void *)p);
    (void)netif;
    (void)p;
    (void)src;
    (void)dst;
    (void)eth_type;
    return (ERR_OK);
}
