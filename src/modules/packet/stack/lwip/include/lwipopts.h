#ifndef _OP_PACKETIO_STACK_LWIPOPTS_H_
#define _OP_PACKETIO_STACK_LWIPOPTS_H_

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
#define LWIP_ERRNO_STDINCLUDE 1

#define LWIP_RAW 1
#define LWIP_SOCKET 0
#define LWIP_NETCONN 0
#define LWIP_NETCONN_SEM_PER_THREAD 1

#define LWIP_IPV6 1
#define IPV6_FRAG_COPYHEADER 1

/* Network Interface options */
#define LWIP_CHECKSUM_CTRL_PER_NETIF 1
#define LWIP_NETIF_HOSTNAME 1

/* Protocol support */
#define LWIP_DHCP 1
#define LWIP_AUTOIP 1
#define LWIP_DHCP_AUTOIP_COOP 1
#define LWIP_DHCP_DOES_ACD_CHECK 1 /* let's be a good net-citizen */
#define LWIP_IPV6_AUTOCONFIG 1
#define LWIP_IPV6_DHCP6 1
#define LWIP_IPV6_SEND_ROUTER_SOLICIT 1
/* Duplicate address detection is disabled for now because AAT has no way to
 * check when IP becomes enabled */
#define LWIP_IPV6_DUP_DETECT_ATTEMPTS 0

/* Miscellaneous options */
#define LWIP_STATS 1
#define LWIP_STATS_DISPLAY 1
#define LWIP_STATS_LARGE 1
#define MEMP_STATS 1
#define MIB2_STATS 1
#include "atomic_mib2_macros.h"

/* Memory allocation options */
#define MEM_ALIGNMENT 8
#define MEM_LIBC_MALLOC 1
#define MEM_USE_POOLS 0
#define MEMP_MEM_MALLOC 1
#define MEMP_USE_CUSTOM_POOLS 0

#define PBUF_POOL_BUFSIZE (2048 + 128) /* DPDK default MBUF buffer size */
#define PBUF_PRIVATE_SIZE 64

/* IP options */
/* Disabled fragmentation for TCP GSO which creates segments > MTU */
#define IP_FRAG 0
#define LWIP_IPV6_FRAG 0

/* TCP options */
#define LWIP_TCP_SACK_OUT 1
#define LWIP_WND_SCALE 1

/*
 * The "extra args" value allows one to put extra data into the TCP PCB.
 * We use it here to keep track of the actual re-transmission count, as
 * iperf conveniently queries and displays it while tests are running.
 */
#define LWIP_TCP_PCB_NUM_EXT_ARGS 1

/*
 * Pick a send {queue, buffer} length that minimizes internal processing.
 * The stack will use whatever queue and buffer size is necessary to fill
 * the specified window.
 */
#define TCP_SND_QUEUELEN TCP_SNDQUEUELEN_OVERFLOW
#define TCP_SND_BUF TCP_WND

#define TCP_SNDLOWAT 1
#define TCP_MSS 1460
#define TCP_WND (2 * 1024 * 1024) /* 2 MB */
#define TCP_RCV_SCALE 6
#define TCP_OVERSIZE TCP_MSS
#define TCP_LISTEN_BACKLOG 1

/* Socket options */
#define SO_REUSE 1

/* Memcpy options */
#define MEMCPY(dst, src, len) PACKETIO_MEMCPY(dst, src, len)
#define SMEMCPY(dst, src, len) PACKETIO_MEMCPY(dst, src, len)
#define MEMMOVE(dst, src, len) PACKETIO_MEMCPY(dst, src, len)

/* Random number generator (used by dhcp6) */
#define LWIP_RAND() rand()

/* We've already got these */
#define LWIP_DONT_PROVIDE_BYTEORDER_FUNCTIONS 1

/*
 * Debugging options
 * Enable the next three options plus whatever content you want.
 */
//#define LWIP_DEBUG 1
//#define LWIP_DBG_MIN_LEVEL LWIP_DBG_LEVEL_ALL
//#define LWIP_DBG_TYPES_ON LWIP_DBG_ON

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

/* Local implementation options */
#define LWIP_PACKET 1
//#define PACKET_DEBUG     LWIP_DBG_ON

#endif /* _OP_PACKETIO_STACK_LWIPOPTS_H_ */
