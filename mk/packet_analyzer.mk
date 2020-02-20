#
# Makefile component for analyzer code
#

PA_REQ_VARS := \
	OP_ROOT \
	OP_BUILD_ROOT
$(call op_check_vars,$(PA_REQ_VARS))

PA_SRC_DIR := $(OP_ROOT)/src/modules/packet/analyzer
PA_OBJ_DIR := $(OP_BUILD_ROOT)/obj/modules/packet/analyzer
PA_LIB_DIR := $(OP_BUILD_ROOT)/lib

OP_INC_DIRS += $(OP_ROOT)/src/modules
OP_LIB_DIRS += $(PA_LIB_DIR)

PA_SOURCES :=
PA_DEPENDS :=
PA_LDLIBS :=

include $(PA_SRC_DIR)/directory.mk

PA_OBJECTS := $(call op_generate_objects,$(PA_SOURCES),$(PA_OBJ_DIR))

PA_LIBRARY := openperf_packet_analyzer
PA_TARGET := $(PA_LIB_DIR)/lib$(PA_LIBRARY).a

OP_LDLIBS += -Wl,--whole-archive -l$(PA_LIBRARY) -Wl,--no-whole-archive $(PA_LDLIBS)

# Load external dependencies
-include $(PA_OBJECTS:.o=.d)
$(call op_include_dependencies,$(PA_DEPENDS))

###
# Build rules
###
$(eval $(call op_generate_build_rules,$(PA_SOURCES),PA_SRC_DIR,PA_OBJ_DIR,PA_DEPENDS))
$(eval $(call op_generate_clean_rules,packet_analyzer,PA_TARGET,PA_OBJECTS))

$(PA_TARGET): $(PA_OBJECTS)
	$(call op_link_library,$@,$(PA_OBJECTS))

.PHONY: packet_analyzer
packet_analyzer: $(PA_TARGET)
