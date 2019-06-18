#
# Makefile component to specify PacketIO code
#
PIO_DEPENDS += expected libzmq swagger_model versions

PIO_SOURCES += \
	init.cpp \
	interface_handler.cpp \
	interface_server.cpp \
	interface_utils.cpp \
	json_transmogrify.cpp \
	port_handler.cpp \
	port_server.cpp \
	port_utils.cpp \
	stack_handler.cpp \
	stack_server.cpp \
	stack_utils.cpp

include $(PIO_SRC_DIR)/drivers/directory.mk
include $(PIO_SRC_DIR)/memory/directory.mk
include $(PIO_SRC_DIR)/pga/directory.mk
include $(PIO_SRC_DIR)/stack/directory.mk

.PHONY: $(PIO_SRC_DIR)/init.cpp
%/init.o: ICP_CPPFLAGS += \
	-DBUILD_COMMIT="\"$(GIT_COMMIT)\"" \
	-DBUILD_NUMBER="\"$(BUILD_NUMBER)\"" \
	-DBUILD_TIMESTAMP="\"$(TIMESTAMP)\""
