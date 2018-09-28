#
# Makefile component to specify PacketIO code
#
PIO_DEPENDS += swagger_model

PIO_SOURCES += \
	init.cpp \
	interface_handler.cpp \
	interface_server.cpp \
	port_handler.cpp \
	port_server.cpp

include $(PIO_SRC_DIR)/drivers/module.mk
include $(PIO_SRC_DIR)/memory/module.mk
include $(PIO_SRC_DIR)/stack/module.mk
