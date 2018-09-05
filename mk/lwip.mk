#
# Makefile component for building the lightweight IP (lwip) stack
#

LWIP_CORE_SOURCES := \
	src/core/init.c \
  src/core/def.c \
  src/core/dns.c \
  src/core/inet_chksum.c \
  src/core/ip.c \
  src/core/mem.c \
  src/core/netif.c \
  src/core/raw.c \
  src/core/stats.c \
  src/core/sys.c \
  src/core/tcp.c \
  src/core/tcp_in.c \
  src/core/tcp_out.c \
  src/core/timeouts.c \
  src/core/udp.c \
	src/netif/ethernet.c

LWIP_CORE4_SOURCES := \
  src/core/ipv4/autoip.c \
  src/core/ipv4/dhcp.c \
  src/core/ipv4/etharp.c \
  src/core/ipv4/icmp.c \
  src/core/ipv4/igmp.c \
  src/core/ipv4/ip4_frag.c \
  src/core/ipv4/ip4.c \
  src/core/ipv4/ip4_addr.c

LWIP_CORE6_SOURCES := \
  src/core/ipv6/dhcp6.c \
  src/core/ipv6/ethip6.c \
  src/core/ipv6/icmp6.c \
  src/core/ipv6/inet6.c \
  src/core/ipv6/ip6.c \
  src/core/ipv6/ip6_addr.c \
  src/core/ipv6/ip6_frag.c \
  src/core/ipv6/mld6.c \
  src/core/ipv6/nd6.c

LWIP_API_SOURCES := \
	src/api/err.c

LWIP_SOURCES := \
	$(LWIP_CORE_SOURCES) \
	$(LWIP_CORE4_SOURCES) \
	$(LWIP_CORE6_SOURCES) \
	$(LWIP_API_SOURCES)

LWIP_INCLUDES := src/include
