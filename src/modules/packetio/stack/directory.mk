#
# Makefile component for stack code
#

$(call op_check_vars, OP_PACKETIO_DRIVER)

PIO_INCLUDES += stack/include

PIO_STACK_DIR := $(PIO_SRC_DIR)/stack/$(OP_PACKETIO_DRIVER)

include $(PIO_STACK_DIR)/directory.mk

PIO_SOURCES += \
	stack/netif_wrapper.cpp \
	stack/tcp_in.c \
	stack/tcp_out.c \
	stack/tcp_stubs.c \
	stack/tcpip.cpp \
	stack/ipv6_netifapi.c

PIO_SOURCES += $(addprefix stack/$(OP_PACKETIO_DRIVER)/,$(PIO_STACK_SOURCES))
