ICP_DPDK_TARGET ?= default-linuxicp-clang

PIO_DEPENDS += dpdk

PIO_INCLUDES += include

PIO_SOURCES += \
	$(LWIP_SOURCES) \
	packetio_init.c \
	packetio_memp.c \
	packetio_netif.c \
	packetio_pbuf.c \
	packetio_port.c \
	packetio_port_config.c \
	packetio_stack.c \
	packetio_sys.c

PIO_LIBS += -lcap
