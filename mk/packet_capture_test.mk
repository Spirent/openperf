#
# Makefile component for packet capture test code
#

CAP_TEST_REQ_VARS := \
	OP_ROOT \
	OP_BUILD_ROOT
$(call op_check_vars,$(CAP_TEST_REQ_VARS))

CAP_TEST_SRC_DIR := $(OP_ROOT)/src/modules/packet/capture
CAP_TEST_OBJ_DIR := $(OP_BUILD_ROOT)/obj/modules/packet/capture
CAP_TEST_LIB_DIR := $(OP_BUILD_ROOT)/lib

OP_INC_DIRS += $(OP_ROOT)/src/modules
OP_LIB_DIRS += $(CAP_TEST_LIB_DIR)

CAP_TEST_SOURCES :=
CAP_TEST_DEPENDS :=
CAP_TEST_LDLIBS :=

include $(CAP_TEST_SRC_DIR)/directory.mk

CAP_TEST_OBJECTS := $(call op_generate_objects,$(CAP_TEST_SOURCES),$(CAP_TEST_OBJ_DIR))

CAP_TEST_LIBRARY := openperf_packet_capture_test
CAP_TEST_TARGET := $(CAP_TEST_LIB_DIR)/lib$(CAP_TEST_LIBRARY).a

OP_LDLIBS += -Wl,--whole-archive -l$(CAP_TEST_LIBRARY) -Wl,--no-whole-archive $(CAP_TEST_LDLIBS)

# Load external dependencies
-include $(CAP_TEST_OBJECTS:.o=.d)
$(call op_include_dependencies,$(CAP_TEST_DEPENDS))

###
# Build rules
###
$(eval $(call op_generate_build_rules,$(CAP_TEST_SOURCES),CAP_TEST_SRC_DIR,CAP_TEST_OBJ_DIR,CAP_TEST_DEPENDS))
$(eval $(call op_generate_clean_rules,packet_capture,CAP_TEST_TARGET,CAP_TEST_OBJECTS))

$(CAP_TEST_TARGET): $(CAP_TEST_OBJECTS)
	$(call op_link_library,$@,$(CAP_TEST_OBJECTS))

.PHONY: packet_capture_test
packet_capture_test: $(CAP_TEST_TARGET)
