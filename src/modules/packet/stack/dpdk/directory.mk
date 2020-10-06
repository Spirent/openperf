#
# DPDK specific components for our LwIP based stack
#

STACK_SOURCES += \
	dpdk/gso_utils.cpp \
	dpdk/lwip.cpp \
	dpdk/memory.cpp \
	dpdk/net_interface.cpp \
	dpdk/netif_utils.cpp \
	dpdk/offload_utils.cpp \
	dpdk/pbuf.c \
	dpdk/pbuf_utils.c \
	dpdk/stack_factory.cpp \
	dpdk/sys_arch.cpp \
	dpdk/sys_mailbox.cpp \
	dpdk/tcpip_input.cpp \
	dpdk/tcpip_mbox.cpp

$(STACK_OBJ_DIR)/dpdk/%.o: OP_CPPFLAGS += -Wno-register
