#
# Makefile component for packet protocol code
#

PP_REQ_VARS := \
	OP_ROOT \
	OP_BUILD_ROOT
$(call op_check_vars,$(PP_REQ_VARS))

PP_SRC_DIR := $(OP_ROOT)/src/modules/packet/protocol
PP_OBJ_DIR := $(OP_BUILD_ROOT)/obj/modules/packet/protocol
PP_LIB_DIR := $(OP_BUILD_ROOT)/lib

OP_INC_DIRS += $(OP_ROOT)/src/modules
OP_LIB_DIRS += $(PP_LIB_DIR)

PP_SOURCES :=
PP_DEPENDS :=
PP_LDLIBS :=

include $(PP_SRC_DIR)/directory.mk

PP_OBJECTS := $(call op_generate_objects,$(PP_SOURCES),$(PP_OBJ_DIR))

PP_LIBRARY := openperf_packet_protocol
PP_TARGET := $(PP_LIB_DIR)/lib$(PP_LIBRARY).a

OP_LDLIBS += -l$(PP_LIBRARY)

# Load external dependencies
-include $(PP_OBJECTS:.o=.d)
$(call op_include_dependencies,$(PP_DEPENDS))

###
# Build rules
###
$(eval $(call op_generate_build_rules,$(PP_SOURCES),PP_SRC_DIR,PP_OBJ_DIR,PP_DEPENDS))
$(eval $(call op_generate_clean_rules,packet_protocol,PP_TARGET,PP_OBJECTS))

$(PP_TARGET): $(PP_OBJECTS)
	$(call op_link_library,$@,$(PP_OBJECTS))

.PHONY: packet_protocol
packet_protocol: $(PP_TARGET)
