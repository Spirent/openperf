#
# Makefile component to specify PacketIO code
#
PIO_DEPENDS += swagger_model

PIO_SOURCES += \
	packetio_init.cpp \
	packetio_port_handler.cpp \
	packetio_port_server.cpp

include $(PIO_SRC_DIR)/drivers/module.mk
