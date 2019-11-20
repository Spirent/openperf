#
# Makefile to build unit tests
#

all: test_openperf

OP_ROOT := $(realpath ../../)
OP_TARGET := tests
include $(OP_ROOT)/mk/bootstrap.mk

BUILD_DIR := $(OP_BUILD_ROOT)/obj/unit
TARGET_DIR := $(OP_BUILD_ROOT)/bin

SRC_DEPENDS :=
BLD_DEPENDS := catch api_test config_file framework packetio_test socket_test

###
# Framework test files
###
SOURCES := \
	test_main.cpp \
	framework/test_config_file_utils.cpp \
	framework/test_config_utils.cpp \
	framework/test_enum_flags.cpp \
	framework/test_hashtab.cpp \
	framework/test_init_factory.cpp \
	framework/test_list.cpp \
	framework/test_logging.cpp \
	framework/test_mac_address.cpp \
	framework/test_ipv4_address.cpp \
	framework/test_ipv4_network.cpp \
	framework/test_offset_ptr.cpp \
	framework/test_std_allocator.cpp \
	framework/test_task.cpp \
	framework/test_units_rate.cpp \
	framework/test_uuid.cpp \
	modules/api/test_api_internal_client.cpp \
	modules/packetio/test_forwarding_table.cpp \
	modules/packetio/test_recycle.cpp \
	modules/packetio/test_transmit_table.cpp \
	modules/socket/test_circular_buffer.cpp \
	modules/socket/test_event_queue.cpp

OBJECTS := $(call op_generate_objects,$(SOURCES),$(BUILD_DIR))

# Pull in dependencies
-include $(OBJECTS:.o=.d)
$(call op_include_dependencies,$(BLD_DEPENDS))

OP_CONFIG_OPTS += --enable-static=yes --enable-shared=no
OP_CPPFLAGS += $(addprefix -I,$(sort $(OP_INC_DIRS)))
OP_LDFLAGS += $(addprefix -L,$(sort $(OP_LIB_DIRS)))

# Build rules
$(eval $(call op_generate_build_rules,$(SOURCES),,BUILD_DIR,BLD_DEPENDS))

# Need to fix immer at some point
$(BUILD_DIR)/modules/packetio/%.o: OP_CPPFLAGS += -Wno-unused-parameter -Wno-shadow

$(TARGET_DIR)/test_openperf: $(OBJECTS) $(BLD_DEPENDS)
	$(call op_link_binary,$@,$(OBJECTS))

.PHONY: test_openperf
test_openperf: $(TARGET_DIR)/test_openperf
	$(TARGET_DIR)/test_openperf

.PHONY: clean
clean:
	@rm -rf $(OP_BUILD_ROOT)
