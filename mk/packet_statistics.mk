#
# Makefile component for packet statistics code
#

PS_REQ_VARS := \
	OP_ROOT \
	OP_BUILD_ROOT
$(call op_check_vars,$(PS_REQ_VARS))

PS_SRC_DIR := $(OP_ROOT)/src/modules/packet/statistics
PS_OBJ_DIR := $(OP_BUILD_ROOT)/obj/modules/packet/statistics
PS_LIB_DIR := $(OP_BUILD_ROOT)/lib

OP_INC_DIRS += $(OP_ROOT)/src/modules
OP_LIB_DIRS += $(PS_LIB_DIR)

PS_SOURCES :=
PS_DEPENDS :=
PS_LDLIBS :=

include $(PS_SRC_DIR)/directory.mk

PS_OBJECTS := $(call op_generate_objects,$(PS_SOURCES),$(PS_OBJ_DIR))

PS_LIBRARY := openperf_packet_statistics
PS_TARGET := $(PS_LIB_DIR)/lib$(PS_LIBRARY).a

OP_LDLIBS += -l$(PS_LIBRARY)

# Load external dependencies
-include $(PS_OBJECTS:.o=.d)
$(call op_include_dependencies,$(PS_DEPENDS))

###
# Build rules
###
$(eval $(call op_generate_build_rules,$(PS_SOURCES),PS_SRC_DIR,PS_OBJ_DIR,PS_DEPENDS))
$(eval $(call op_generate_clean_rules,packet_statistics,PS_TARGET,PS_OBJECTS))

$(PS_TARGET): $(PS_OBJECTS)
	$(call op_link_library,$@,$(PS_OBJECTS))

.PHONY: packet_statistics
packet_statistics: $(PS_TARGET)
