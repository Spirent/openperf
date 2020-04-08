#
# Makefile component for block code
#

CPU_REQ_VARS := \
	OP_ROOT \
	OP_BUILD_ROOT
$(call op_check_vars,$(CPU_REQ_VARS))

CPU_SRC_DIR := $(OP_ROOT)/src/modules/cpu
CPU_OBJ_DIR := $(OP_BUILD_ROOT)/obj/modules/cpu
CPU_LIB_DIR := $(OP_BUILD_ROOT)/lib

OP_INC_DIRS += $(OP_ROOT)/src/modules
OP_LIB_DIRS += $(CPU_LIB_DIR)

CPU_SOURCES :=
CPU_DEPENDS :=
CPU_LDLIBS :=

include $(CPU_SRC_DIR)/directory.mk

CPU_OBJECTS := $(call op_generate_objects,$(CPU_SOURCES),$(CPU_OBJ_DIR))

CPU_LIBRARY := openperf_cpu
CPU_TARGET := $(CPU_LIB_DIR)/lib$(CPU_LIBRARY).a

OP_LDLIBS += -Wl,--whole-archive -l$(CPU_LIBRARY) -Wl,--no-whole-archive $(CPU_LDLIBS)

# Load external dependencies
-include $(CPU_OBJECTS:.o=.d)
$(call op_include_dependencies,$(CPU_DEPENDS))

###
# Build rules
###
$(eval $(call op_generate_build_rules,$(CPU_SOURCES),CPU_SRC_DIR,CPU_OBJ_DIR,CPU_DEPENDS))
$(eval $(call op_generate_clean_rules,cpu,CPU_TARGET,CPU_OBJECTS))

$(CPU_TARGET): $(CPU_OBJECTS)
	$(call op_link_library,$@,$(CPU_OBJECTS))

.PHONY: cpu
cpu: $(CPU_TARGET)