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
	port_utils.cpp \
	stack_handler.cpp \
	stack_server.cpp \
	stack_utils.cpp

include $(PIO_SRC_DIR)/drivers/module.mk
include $(PIO_SRC_DIR)/memory/module.mk
include $(PIO_SRC_DIR)/stack/module.mk

.PHONY: $(PIO_SRC_DIR)/init.cpp
$(PIO_OBJ_DIR)/init.o: ICP_CPPFLAGS += \
	-DBUILD_COMMIT="\"$(GIT_COMMIT)\"" \
	-DBUILD_NUMBER="\"$(BUILD_NUMBER)\"" \
	-DBUILD_TIMESTAMP="\"$(TIMESTAMP)\""
