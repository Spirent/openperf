#
# Makefile component for stack files and flags
#

STACK_INCLUDES += \
	include \
	lwip/src/include

LWIP_CORE_SOURCES := \
	lwip/src/core/init.c \
  lwip/src/core/def.c \
  lwip/src/core/dns.c \
  lwip/src/core/inet_chksum.c \
  lwip/src/core/ip.c \
  lwip/src/core/mem.c \
  lwip/src/core/netif.c \
  lwip/src/core/raw.c \
  lwip/src/core/stats.c \
  lwip/src/core/sys.c \
  lwip/src/core/tcp.c \
  lwip/src/core/tcp_in.c \
  lwip/src/core/tcp_out.c \
  lwip/src/core/timeouts.c \
  lwip/src/core/udp.c

LWIP_CORE4_SOURCES := \
  lwip/src/core/ipv4/autoip.c \
  lwip/src/core/ipv4/dhcp.c \
  lwip/src/core/ipv4/etharp.c \
  lwip/src/core/ipv4/icmp.c \
  lwip/src/core/ipv4/igmp.c \
  lwip/src/core/ipv4/ip4_frag.c \
  lwip/src/core/ipv4/ip4.c \
  lwip/src/core/ipv4/ip4_addr.c

LWIP_CORE6_SOURCES := \
  lwip/src/core/ipv6/dhcp6.c \
  lwip/src/core/ipv6/ethip6.c \
  lwip/src/core/ipv6/icmp6.c \
  lwip/src/core/ipv6/inet6.c \
  lwip/src/core/ipv6/ip6.c \
  lwip/src/core/ipv6/ip6_addr.c \
  lwip/src/core/ipv6/ip6_frag.c \
  lwip/src/core/ipv6/mld6.c \
  lwip/src/core/ipv6/nd6.c

LWIP_SOURCES := $(LWIP_CORE_SOURCES) $(LWIP_CORE4_SOURCES) $(LWIP_CORE6_SOURCES)

STACK_SOURCES += \
	$(LWIP_SOURCES) \
	ethernet.c \
	sys.c
