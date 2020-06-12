#
# Makefile component for packet analyzer code
#

PA_TEST_REQ_VARS := \
	OP_ROOT \
	OP_BUILD_ROOT
$(call op_check_vars,$(PA_TEST_REQ_VARS))

PA_TEST_SRC_DIR := $(OP_ROOT)/src/modules/packet/analyzer
PA_TEST_OBJ_DIR := $(OP_BUILD_ROOT)/obj/modules/packet/analyzer
PA_TEST_LIB_DIR := $(OP_BUILD_ROOT)/lib

OP_INC_DIRS += $(OP_ROOT)/src/modules
OP_LIB_DIRS += $(PA_TEST_LIB_DIR)

PA_TEST_SOURCES :=
PA_TEST_DEPENDS :=
PA_TEST_LDLIBS :=

include $(PA_TEST_SRC_DIR)/directory.mk

PA_TEST_OBJECTS := $(call op_generate_objects,$(PA_TEST_SOURCES),$(PA_TEST_OBJ_DIR))

PA_TEST_LIBRARY := openperf_packet_analyzer
PA_TEST_TARGET := $(PA_TEST_LIB_DIR)/lib$(PA_TEST_LIBRARY).a

OP_LDLIBS += -l$(PA_TEST_LIBRARY)

# Load external dependencies
-include $(PA_TEST_OBJECTS:.o=.d)
$(call op_include_dependencies,$(PA_TEST_DEPENDS))

###
# Build rules
###
$(eval $(call op_generate_build_rules,$(PA_TEST_SOURCES),PA_TEST_SRC_DIR,PA_TEST_OBJ_DIR,PA_TEST_DEPENDS))
$(eval $(call op_generate_clean_rules,packet_analyzer_test,PA_TEST_TARGET,PA_TEST_OBJECTS))

$(PA_TEST_TARGET): $(PA_TEST_OBJECTS)
	$(call op_link_library,$@,$(PA_TEST_OBJECTS))

.PHONY: packet_analyzer
packet_analyzer_test: $(PA_TEST_TARGET)
