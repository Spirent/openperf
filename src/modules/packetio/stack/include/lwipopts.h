#ifndef _ICP_PACKETIO_STACK_LWIPOPTS_H_
#define _ICP_PACKETIO_STACK_LWIPOPTS_H_

/**
 * This file contains definitions to control the build and runtime behavior
 * of the LwIP stack. Documentation for each option may be found in
 * <lwip>/src/include/lwip/opt.h
 */

/* Build minimal, lockless stack */
#define NO_SYS 0
#define SYS_LIGHTWEIGHT_PROT 0
#define LWIP_TCPIP_CORE_LOCKING 0
#define LWIP_NETIF_API 1
#define LWIP_CALLBACK_API 0
#define LWIP_EVENT_API 1
#define LWIP_ERRNO_INCLUDE <errno.h>

#define LWIP_SOCKET 0
#define LWIP_NETCONN 0
#define LWIP_NETCONN_SEM_PER_THREAD 1

#define LWIP_IPV6 1
#define IPV6_FRAG_COPYHEADER 1

/* Network Interface options */
#define LWIP_NETIF_HOSTNAME 1
#define LWIP_NETIF_LINK_CALLBACK 1
#define LWIP_NETIF_TX_SINGLE_PBUF 1

/* Protocol support */
#define LWIP_DHCP 1
#define LWIP_AUTOIP 1
#define LWIP_DHCP_AUTOIP_COOP 1
#define LWIP_DHCP_CHECK_LINK_UP 1
#define DHCP_DOES_ARP_CHECK 1  /* let's be a good net-citizen */

/* Miscellaneous options */
#define LWIP_STATS 1
#define LWIP_STATS_DISPLAY 1
#define MEMP_STATS 1
#define MIB2_STATS 1

/* Memory allocation options */
#define MEM_ALIGNMENT 8
#define MEM_LIBC_MALLOC 1
#define MEM_USE_POOLS 0
#define MEMP_MEM_MALLOC 1
#define MEMP_USE_CUSTOM_POOLS 0

/* TCP options */
#define TCP_LISTEN_BACKLOG 1
#define TCP_SND_BUF 65535
#define TCP_SND_QUEUELEN ((2 * (TCP_SND_BUF) + (TCP_MSS - 1))/(TCP_MSS))
#define TCP_MSS 1500

/* Socket options */
#define SO_REUSE 1

/* We've already got these */
#define LWIP_DONT_PROVIDE_BYTEORDER_FUNCTIONS 1

/* Debugging options */
#define LWIP_DEBUG 1

//#define ETHARP_DEBUG     LWIP_DBG_ON
//#define NETIF_DEBUG      LWIP_DBG_ON
//#define PBUF_DEBUG       LWIP_DBG_ON
//#define API_LIB_DEBUG    LWIP_DBG_ON
//#define API_MSG_DEBUG    LWIP_DBG_ON
//#define SOCKETS_DEBUG    LWIP_DBG_ON
//#define ICMP_DEBUG       LWIP_DBG_ON
//#define IGMP_DEBUG       LWIP_DBG_ON
//#define INET_DEBUG       LWIP_DBG_ON
//#define IP_DEBUG         LWIP_DBG_ON
//#define IP_REASS_DEBUG   LWIP_DBG_ON
//#define RAW_DEBUG        LWIP_DBG_ON
//#define MEM_DEBUG        LWIP_DBG_ON
//#define MEMP_DEBUG       LWIP_DBG_ON
//#define SYS_DEBUG        LWIP_DBG_ON
//#define TIMERS_DEBUG     LWIP_DBG_ON
//#define TCP_DEBUG        LWIP_DBG_ON
//#define TCP_INPUT_DEBUG  LWIP_DBG_ON
//#define TCP_FR_DEBUG     LWIP_DBG_ON
//#define TCP_RTO_DEBUG    LWIP_DBG_ON
//#define TCP_CWND_DEBUG   LWIP_DBG_ON
//#define TCP_WND_DEBUG    LWIP_DBG_ON
//#define TCP_OUTPUT_DEBUG LWIP_DBG_ON
//#define TCP_RST_DEBUG    LWIP_DBG_ON
//#define TCP_QLEN_DEBUG   LWIP_DBG_ON
//#define UDP_DEBUG        LWIP_DBG_ON
//#define TCPIP_DEBUG      LWIP_DBG_ON
//#define SLIP_DEBUG       LWIP_DBG_ON
//#define DHCP_DEBUG       LWIP_DBG_ON
//#define AUTOIP_DEBUG     LWIP_DBG_ON
//#define DNS_DEBUG        LWIP_DBG_ON
//#define IP6_DEBUG        LWIP_DBG_ON

/* Internal structure limits */

#endif /* _ICP_PACKETIO_STACK_LWIPOPTS_H_ */
