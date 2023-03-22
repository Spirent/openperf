/**
 * netifapi functions for IPv6
 *
 * lwip provides netifapi functions for IPv4 which support stack
 * configuration while the stack is running by sending messages
 * between threads instead of using a global lock mutex.
 *
 * Since lwip does not provide netifapi functions for IPv6 and
 * openperf is configured with global locking disabled, the
 * necessary netifapi functions are implemented here.
 */

#include "lwip/opt.h"

#if LWIP_NETIF_API /* don't build if not configured for use in lwipopts.h */
#if LWIP_IPV6

#include "lwip/etharp.h"
#include "lwip/netifapi.h"
#include "lwip/memp.h"
#include "lwip/dhcp6.h"
#include "lwip/priv/tcpip_priv.h"
#include "packet/stack/lwip/netifapi_ipv6.h"
#include "packet/stack/lwip/nd6_op.h"

#include <string.h> /* strncpy */

struct netifapi_ipv6_msg
{
    struct tcpip_api_call_data call;
    struct netif* netif;
    union
    {
        struct
        {
            u8_t from_mac_48bit;
        } add_linklocal;
        struct
        {
            NETIFAPI_IPADDR_DEF(ip6_addr_t, ipaddr);
            NETIFAPI_IPADDR_DEF(ip6_addr_t, gwaddr);
            s8_t addr_idx;
            u8_t prefix_len;
        } add;
        struct
        {
            NETIFAPI_IPADDR_DEF(ip6_addr_t, ipaddr);
        } gateway;
    } msg;
};

#define NETIFAPI_VAR_REF(name) API_VAR_REF(name)
#define NETIFAPI_VAR_DECLARE(name)                                             \
    API_VAR_DECLARE(struct netifapi_ipv6_msg, name)
#define NETIFAPI_VAR_ALLOC(name)                                               \
    API_VAR_ALLOC(struct netifapi_ipv6_msg, MEMP_NETIFAPI_MSG, name, ERR_MEM)
#define NETIFAPI_VAR_FREE(name) API_VAR_FREE(MEMP_NETIFAPI_MSG, name)

err_t netifapi_do_netif_create_ip6_linklocal_address(
    struct tcpip_api_call_data* m)
{
    /* cast through void* to silence alignment warnings.
     * We know it works because the structs have been instantiated as struct
     * netifapi_msg */
    struct netifapi_ipv6_msg* msg = (struct netifapi_ipv6_msg*)(void*)m;

    netif_create_ip6_linklocal_address(
        msg->netif, API_EXPR_REF(msg->msg.add_linklocal.from_mac_48bit));
    return ERR_OK;
}

/**
 * Call netif_add_ip6_address() inside the tcpip_thread context.
 */
static err_t netifapi_do_netif_add_ip6_address(struct tcpip_api_call_data* m)
{
    /* cast through void* to silence alignment warnings.
     * We know it works because the structs have been instantiated as struct
     * netifapi_msg */
    err_t err;
    struct netifapi_ipv6_msg* msg = (struct netifapi_ipv6_msg*)(void*)m;

    if ((err = netif_add_ip6_address(msg->netif,
                                 API_EXPR_REF(msg->msg.add.ipaddr),
                                 &API_EXPR_REF(msg->msg.add.addr_idx))) != 0)
        return err;
#if LWIP_IPV6_DUP_DETECT_ATTEMPTS == 0
    /* Address is always valid when not using duplicate address detection.
     * netif_create_ip6_linklocal_address() does this, but netif_add_ip6_address() does not.
     */
    netif_ip6_addr_set_state(msg->netif, API_EXPR_REF(msg->msg.add.addr_idx), IP6_ADDR_PREFERRED);
#endif
    netif_set_ip6_prefix_len(msg->netif, API_EXPR_REF(msg->msg.add.addr_idx), API_EXPR_REF(msg->msg.add.prefix_len));
    return err;
}

/**
 * Call netif_ip6_addr_set() inside the tcpip_thread context.
 */
static err_t netifapi_do_netif_set_ip6_address(struct tcpip_api_call_data* m)
{
    /* cast through void* to silence alignment warnings.
     * We know it works because the structs have been instantiated as struct
     * netifapi_msg */
    struct netifapi_ipv6_msg* msg = (struct netifapi_ipv6_msg*)(void*)m;

    netif_ip6_addr_set(msg->netif,
                       API_EXPR_REF(msg->msg.add.addr_idx),
                       API_EXPR_REF(msg->msg.add.ipaddr));
    netif_set_ip6_prefix_len(msg->netif, API_EXPR_REF(msg->msg.add.addr_idx), API_EXPR_REF(msg->msg.add.prefix_len));
    return ERR_OK;
}

/**
 * Call netif_set_ip6_gateway() inside the tcpip_thread context.
 */
static err_t netifapi_do_netif_set_ip6_gateway(struct tcpip_api_call_data* m)
{
    /* cast through void* to silence alignment warnings.
     * We know it works because the structs have been instantiated as struct
     * netifapi_msg */
    struct netifapi_ipv6_msg* msg = (struct netifapi_ipv6_msg*)(void*)m;
    netif_set_ip6_gateway_addr(msg->netif, API_EXPR_REF(msg->msg.gateway.ipaddr));
    return ERR_OK;
}

/**
 * @ingroup netifapi_netif
 * Call netif_create_ip6_linklocal_address() in a thread-safe way by running
 * that function inside the tcpip_thread context.
 *
 * @note for params @see netif_create_ip6_linklocal_address()
 */
err_t netifapi_netif_create_ip6_linklocal_address(struct netif* netif,
                                                  u8_t from_mac_48bit)
{
    err_t err;
    NETIFAPI_VAR_DECLARE(msg);
    NETIFAPI_VAR_ALLOC(msg);

    NETIFAPI_VAR_REF(msg).netif = netif;
    NETIFAPI_VAR_REF(msg).msg.add_linklocal.from_mac_48bit =
        NETIFAPI_VAR_REF(from_mac_48bit);
    err = tcpip_api_call(netifapi_do_netif_create_ip6_linklocal_address,
                         &API_VAR_REF(msg).call);
    NETIFAPI_VAR_FREE(msg);
    return err;
}

/**
 * @ingroup netifapi_netif
 * Call netif_add_ip6_address() in a thread-safe way by running that function
 * inside the tcpip_thread context.
 *
 * @note for params @see netif_add_ip6_address()
 */
err_t netifapi_netif_add_ip6_address(struct netif* netif,
                                     const ip6_addr_t* ipaddr,
                                     u8_t prefix_len,
                                     s8_t* chosen_idx)
{
    err_t err;
    NETIFAPI_VAR_DECLARE(msg);
    NETIFAPI_VAR_ALLOC(msg);

    if (ipaddr == NULL) { ipaddr = IP6_ADDR_ANY6; }

    NETIFAPI_VAR_REF(msg).netif = netif;
    NETIFAPI_VAR_REF(msg).msg.add.ipaddr = NETIFAPI_VAR_REF(ipaddr);
    NETIFAPI_VAR_REF(msg).msg.add.prefix_len = prefix_len;
    NETIFAPI_VAR_REF(msg).msg.add.addr_idx = 0;
    err = tcpip_api_call(netifapi_do_netif_add_ip6_address,
                         &API_VAR_REF(msg).call);
    if (chosen_idx) *chosen_idx = NETIFAPI_VAR_REF(msg).msg.add.addr_idx;
    NETIFAPI_VAR_FREE(msg);
    return err;
}

/**
 * @ingroup netifapi_netif
 * Call netif_ip6_addr_set() in a thread-safe way by running that function
 * inside the tcpip_thread context.
 *
 * @note for params @see netif_ip6_addr_set()
 */
err_t netifapi_netif_set_ip6_address(struct netif* netif,
                                     s8_t addr_idx,
                                     const ip6_addr_t* ipaddr,
                                     u8_t prefix_len)
{
    err_t err;
    NETIFAPI_VAR_DECLARE(msg);
    NETIFAPI_VAR_ALLOC(msg);

    if (ipaddr == NULL) { ipaddr = IP6_ADDR_ANY6; }

    NETIFAPI_VAR_REF(msg).netif = netif;
    NETIFAPI_VAR_REF(msg).msg.add.addr_idx = addr_idx;
    NETIFAPI_VAR_REF(msg).msg.add.ipaddr = NETIFAPI_VAR_REF(ipaddr);
    NETIFAPI_VAR_REF(msg).msg.add.prefix_len = prefix_len;
    err =
        tcpip_api_call(netifapi_do_netif_set_ip6_address, &API_VAR_REF(msg).call);
    NETIFAPI_VAR_FREE(msg);
    return err;
}

/**
 * @ingroup netifapi_netif
 * Set interface gateway address in a thread-safe way by running that function
 * inside the tcpip_thread context.
 */
err_t netifapi_netif_set_ip6_gateway(struct netif* netif,
                                     const ip6_addr_t* gateway)
{
    err_t err;
    NETIFAPI_VAR_DECLARE(msg);
    NETIFAPI_VAR_ALLOC(msg);

    if (gateway == NULL) { gateway = IP6_ADDR_ANY6; }

    NETIFAPI_VAR_REF(msg).netif = netif;
    NETIFAPI_VAR_REF(msg).msg.gateway.ipaddr = NETIFAPI_VAR_REF(gateway);
    err =
        tcpip_api_call(netifapi_do_netif_set_ip6_gateway, &API_VAR_REF(msg).call);
    NETIFAPI_VAR_FREE(msg);
    return err;
}

err_t netifapi_dhcp6_enable_stateless(struct netif* netif)
{
    return netifapi_netif_common(netif, NULL, dhcp6_enable_stateless);
}

err_t netifapi_dhcp6_enable_stateful(struct netif* netif)
{
    return netifapi_netif_common(netif, NULL, dhcp6_enable_stateful);
}

err_t netifapi_dhcp6_disable(struct netif* netif)
{
    return netifapi_netif_common(netif, dhcp6_disable, NULL);
}

#endif /* LWIP_IPV6 */
#endif /* LWIP_NETIF_API */
