#
# Makefile component to specify PacketIO code
#
PIO_DEPENDS += expected libzmq swagger_model versions

PIO_SOURCES += \
	init.cpp \
	interface_handler.cpp \
	interface_server.cpp \
	interface_utils.cpp \
	internal_client.cpp \
	internal_transmogrify.cpp \
	internal_server.cpp \
	json_transmogrify.cpp \
	port_handler.cpp \
	port_server.cpp \
	port_utils.cpp \
	stack_handler.cpp \
	stack_server.cpp \
	stack_utils.cpp

include $(PIO_SRC_DIR)/drivers/directory.mk
include $(PIO_SRC_DIR)/memory/directory.mk
include $(PIO_SRC_DIR)/stack/directory.mk
include $(PIO_SRC_DIR)/workers/directory.mk

PIO_VERSIONED_FILES := init.cpp
PIO_UNVERSIONED_OBJECTS :=\
	$(call op_generate_objects,$(filter-out $(PIO_VERSIONED_FILES),$(PIO_SOURCES)),$(PIO_OBJ_DIR))

$(PIO_OBJ_DIR)/init.o : $(PIO_UNVERSIONED_OBJECTS)
$(PIO_OBJ_DIR)/init.o: OP_CPPFLAGS += \
	-DBUILD_COMMIT="\"$(GIT_COMMIT)\"" \
	-DBUILD_NUMBER="\"$(BUILD_NUMBER)\"" \
	-DBUILD_TIMESTAMP="\"$(TIMESTAMP)\""
