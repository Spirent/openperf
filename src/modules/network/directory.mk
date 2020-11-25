#
# Makefile component to specify network code
#

NETWORK_DEPENDS += api expected framework json pistache swagger_model \
	timesync versions

NETWORK_SOURCES += \
	api_converters.cpp \
	api_transmogrify.cpp \
	handler.cpp \
	init.cpp \
	server.cpp \
	generator.cpp \
	generator_stack.cpp \
	task.cpp \
	internal_server.cpp \
	internal_server_stack.cpp \
	internal_utils.cpp \
	network_options.c

ifeq ($(PLATFORM), linux)
	NETWORK_SOURCES += \
	internal_utils_linux.cpp
endif

NETWORK_SOURCES += \
	drivers/kernel.cpp \
	drivers/dpdk.cpp

NETWORK_SOURCES += \
	firehose/server_tcp.cpp \
	firehose/server_udp.cpp

NETWORK_VERSIONED_FILES := init.cpp
NETWORK_UNVERSIONED_OBJECTS := \
	$(call op_generate_objects,$(filter-out $(NETWORK_VERSIONED_FILES),$(NETWORK_SOURCES)),$(NETWORK_OBJ_DIR))

$(NETWORK_OBJ_DIR)/init.o: $(NETWORK_UNVERSIONED_OBJECTS)
$(NETWORK_OBJ_DIR)/init.o: OP_CPPFLAGS += \
	-DBUILD_COMMIT="\"$(GIT_COMMIT)\"" \
	-DBUILD_NUMBER="\"$(BUILD_NUMBER)\"" \
	-DBUILD_TIMESTAMP="\"$(TIMESTAMP)\""
