#
# Makefile component to specify PacketIO code
#
PIO_DEPENDS += swagger_model expected

PIO_SOURCES += \
	init.cpp \
	interface_handler.cpp \
	interface_server.cpp \
	interface_utils.cpp \
	json_transmogrify.cpp \
	port_handler.cpp \
	port_server.cpp \
	port_utils.cpp

include $(PIO_SRC_DIR)/drivers/module.mk
include $(PIO_SRC_DIR)/memory/module.mk
include $(PIO_SRC_DIR)/stack/module.mk
