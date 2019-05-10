#
# Makefile component for building the lightweight IP (lwip) stack
#

###
# Load stack info; always required for now
###
LWIP_REQ_VARS := \
	ICP_ROOT \
	ICP_BUILD_ROOT
$(call icp_check_vars,$(LWIP_REQ_VARS))

LWIP_SRC_DIR := $(ICP_ROOT)/deps/lwip/src
LWIP_INC_DIR := $(LWIP_SRC_DIR)/include
LWIP_OBJ_DIR := $(ICP_BUILD_ROOT)/obj/lwip

# Update global variables
ICP_INC_DIRS += $(LWIP_INC_DIR)

###
# Sources and objects
###
LWIP_CORE_SOURCES := \
	core/init.c \
	core/def.c \
	core/dns.c \
	core/inet_chksum.c \
	core/ip.c \
	core/mem.c \
	core/netif.c \
	core/raw.c \
	core/stats.c \
	core/sys.c \
	core/tcp.c \
	core/udp.c \
	netif/ethernet.c

LWIP_CORE4_SOURCES := \
	core/ipv4/autoip.c \
	core/ipv4/dhcp.c \
	core/ipv4/etharp.c \
	core/ipv4/icmp.c \
	core/ipv4/igmp.c \
	core/ipv4/ip4_frag.c \
	core/ipv4/ip4.c \
	core/ipv4/ip4_addr.c

LWIP_CORE6_SOURCES := \
	core/ipv6/dhcp6.c \
	core/ipv6/ethip6.c \
	core/ipv6/icmp6.c \
	core/ipv6/inet6.c \
	core/ipv6/ip6.c \
	core/ipv6/ip6_addr.c \
	core/ipv6/ip6_frag.c \
	core/ipv6/mld6.c \
	core/ipv6/nd6.c

LWIP_API_SOURCES := \
	api/err.c \
	api/netifapi.c

LWIP_SOURCES := \
	$(LWIP_CORE_SOURCES) \
	$(LWIP_CORE4_SOURCES) \
	$(LWIP_CORE6_SOURCES) \
	$(LWIP_API_SOURCES)

LWIP_OBJECTS := $(patsubst %, $(LWIP_OBJ_DIR)/%, \
	$(patsubst %.c, %.o, $(LWIP_SOURCES)))

# Pull in object dependencies, maybe
-include $(LWIP_OBJECTS:.o=.d)

###
# Build rules
###
$(LWIP_OBJECTS): | $(PIO_DEPENDS)

# Using large windows with window scaling can introduce a tautological
# comparison in tcp.c.  It's safe to ignore.
$(LWIP_OBJ_DIR)/core/tcp.o: LWIP_CFLAGS += -Wno-tautological-constant-out-of-range-compare

$(LWIP_OBJ_DIR)/%.o: $(LWIP_SRC_DIR)/%.c
	@mkdir -p $(dir $@)
	$(strip $(ICP_CC) -o $@ -c $(LWIP_CPPFLAGS) $(ICP_CPPFLAGS) $(LWIP_CFLAGS) $(ICP_COPTS) $<)

.PHONY: clean_lwip
clean_lwip:
	@rm -rf $(LWIP_OBJ_DIR)
clean: clean_lwip
