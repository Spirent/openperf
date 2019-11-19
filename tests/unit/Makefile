#
# Makefile to build unit tests
#

all: test_inception

ICP_ROOT := $(realpath ../../)
ICP_TARGET := tests
include $(ICP_ROOT)/mk/bootstrap.mk

BUILD_DIR := $(ICP_BUILD_ROOT)/obj/unit
TARGET_DIR := $(ICP_BUILD_ROOT)/bin

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

OBJECTS := $(call icp_generate_objects,$(SOURCES),$(BUILD_DIR))

# Pull in dependencies
-include $(OBJECTS:.o=.d)
$(call icp_include_dependencies,$(BLD_DEPENDS))

ICP_CONFIG_OPTS += --enable-static=yes --enable-shared=no
ICP_CPPFLAGS += $(addprefix -I,$(sort $(ICP_INC_DIRS)))
ICP_LDFLAGS += $(addprefix -L,$(sort $(ICP_LIB_DIRS)))

# Build rules
$(eval $(call icp_generate_build_rules,$(SOURCES),,BUILD_DIR,BLD_DEPENDS))

# Need to fix immer at some point
$(BUILD_DIR)/modules/packetio/%.o: ICP_CPPFLAGS += -Wno-unused-parameter -Wno-shadow

$(TARGET_DIR)/test_inception: $(OBJECTS) $(BLD_DEPENDS)
	$(call icp_link_binary,$@,$(OBJECTS))

.PHONY: test_inception
test_inception: $(TARGET_DIR)/test_inception
	$(TARGET_DIR)/test_inception

.PHONY: clean
clean:
	@rm -rf $(ICP_BUILD_ROOT)
