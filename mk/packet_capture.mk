#
# Makefile component for capture code
#

CAP_REQ_VARS := \
	OP_ROOT \
	OP_BUILD_ROOT
$(call op_check_vars,$(CAP_REQ_VARS))

CAP_SRC_DIR := $(OP_ROOT)/src/modules/packet/capture
CAP_OBJ_DIR := $(OP_BUILD_ROOT)/obj/modules/packet/capture
CAP_LIB_DIR := $(OP_BUILD_ROOT)/lib

OP_INC_DIRS += $(OP_ROOT)/src/modules
OP_LIB_DIRS += $(CAP_LIB_DIR)

CAP_SOURCES :=
CAP_DEPENDS :=
CAP_LDLIBS :=

include $(CAP_SRC_DIR)/directory.mk

CAP_OBJECTS := $(call op_generate_objects,$(CAP_SOURCES),$(CAP_OBJ_DIR))

CAP_LIBRARY := openperf_packet_capture
CAP_TARGET := $(CAP_LIB_DIR)/lib$(CAP_LIBRARY).a

OP_LDLIBS += -Wl,--whole-archive -l$(CAP_LIBRARY) -Wl,--no-whole-archive $(CAP_LDLIBS)

# Load external dependencies
-include $(CAP_OBJECTS:.o=.d)
$(call op_include_dependencies,$(CAP_DEPENDS))

###
# Build rules
###
$(eval $(call op_generate_build_rules,$(CAP_SOURCES),CAP_SRC_DIR,CAP_OBJ_DIR,CAP_DEPENDS))
$(eval $(call op_generate_clean_rules,packet_capture,CAP_TARGET,CAP_OBJECTS))

$(CAP_TARGET): $(CAP_OBJECTS)
	$(call op_link_library,$@,$(CAP_OBJECTS))

.PHONY: packet_capture
packet_capture: $(CAP_TARGET)
