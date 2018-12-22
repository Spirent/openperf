#
# Makefile component for stack code
#

$(call icp_check_vars, ICP_PACKETIO_DRIVER)

PIO_INCLUDES += stack/include
PIO_DEPENDS += socket_server

PIO_STACK_DIR := $(PIO_SRC_DIR)/stack/$(ICP_PACKETIO_DRIVER)

include $(PIO_STACK_DIR)/module.mk

PIO_SOURCES += \
	stack/tcpip.cpp \
	stack/timeouts.c

PIO_SOURCES += $(addprefix stack/$(ICP_PACKETIO_DRIVER)/,$(PIO_STACK_SOURCES))
