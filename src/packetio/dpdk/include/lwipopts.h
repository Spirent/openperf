#ifndef _LWIPOPTS_H_
#define _LWIPOPTS_H_

/**
 * This file contains definitions to control the build and runtime behavior
 * of the LwIP stack. Documentation for each option may be found in
 * <lwip>/src/include/lwip/opt.h
 */

/* Build minimal, lockless stack */
#define NO_SYS 1
#define LWIP_SOCKET 0
#define LWIP_NETCONN 0
#define SYS_LIGHTWEIGHT_PROT 0
#define LWIP_TCPIP_CORE_LOCKING 0

/* Protocol support */
#define LWIP_DHCP 1

/* Use appropriate thread names */
#define TCPIP_THREAD_NAME "icp_stack"
#define DEFAULT_THREAD_NAME "icp_stack_misc"

/* Miscellaneous options */
#define LWIP_STATS_DISPLAY 1
#define LWIP_NETIF_TX_SINGLE_PBUF 1

/* Memory allocation options */
#define MEM_ALIGNMENT 8
#define MEM_LIBC_MALLOC 1
#define MEM_USE_POOLS 0
#define MEMP_MEM_MALLOC 1
#define MEMP_USE_CUSTOM_POOLS 0

/* Performance options */
#define TCP_MSS 1500

/* We've already got these */
#define LWIP_DONT_PROVIDE_BYTEORDER_FUNCTIONS 1

/* Debugging options */
//#define LWIP_DEBUG 0

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

#endif /* _LWIPOPTS_H_ */
