#
# Makefile to build unit tests
#

all: test_openperf

OP_ROOT := $(realpath ../../)
OP_TARGET := tests
include $(OP_ROOT)/mk/bootstrap.mk

TEST_BIN_DIR := $(OP_BUILD_ROOT)/bin
TEST_OBJ_DIR := $(OP_BUILD_ROOT)/obj/unit
TEST_SRC_DIR := $(OP_ROOT)/tests/unit

TEST_DEPENDS := catch
TEST_SOURCES := test_main.cpp
TEST_FLAGS :=

###
# Pull in test files
###
include $(TEST_SRC_DIR)/framework/directory.mk
include $(TEST_SRC_DIR)/lib/packet/directory.mk
include $(TEST_SRC_DIR)/lib/spirent_pga/directory.mk
include $(TEST_SRC_DIR)/modules/api/directory.mk
include $(TEST_SRC_DIR)/modules/block/directory.mk
include $(TEST_SRC_DIR)/modules/dynamic/directory.mk
include $(TEST_SRC_DIR)/modules/memory/directory.mk
include $(TEST_SRC_DIR)/modules/packet/analyzer/directory.mk
include $(TEST_SRC_DIR)/modules/packet/bpf/directory.mk
include $(TEST_SRC_DIR)/modules/packet/capture/directory.mk
include $(TEST_SRC_DIR)/modules/packet/generator/directory.mk
include $(TEST_SRC_DIR)/modules/packet/statistics/directory.mk
include $(TEST_SRC_DIR)/modules/packetio/directory.mk
include $(TEST_SRC_DIR)/modules/socket/directory.mk
include $(TEST_SRC_DIR)/modules/timesync/directory.mk

TEST_OBJECTS := $(call op_generate_objects,$(TEST_SOURCES),$(TEST_OBJ_DIR))

# Pull in dependencies
-include $(TEST_OBJECTS:.o=.d)
$(call op_include_dependencies,$(TEST_DEPENDS))

OP_CONFIG_OPTS += --enable-static=yes --enable-shared=no
OP_CPPFLAGS += $(addprefix -I,$(sort $(OP_INC_DIRS)))
OP_LDFLAGS += $(addprefix -L,$(sort $(OP_LIB_DIRS)))
OP_LDLIBS += -ldl

# Build rules
$(eval $(call op_generate_build_rules,$(TEST_SOURCES),,TEST_OBJ_DIR,TEST_DEPENDS,TEST_FLAGS))

# Need to fix immer at some point
$(TEST_OBJ_DIR)/modules/packetio/%.o: OP_CPPFLAGS += -Wno-unused-parameter -Wno-shadow

$(TEST_BIN_DIR)/test_openperf: $(TEST_OBJECTS) $(TEST_DEPENDS)
	$(call op_link_binary,$@,$(TEST_OBJECTS))

.PHONY: test_openperf
test_openperf: $(TEST_BIN_DIR)/test_openperf
	$(TEST_BIN_DIR)/test_openperf

.PHONY: clean
clean:
	@rm -rf $(OP_BUILD_ROOT)
