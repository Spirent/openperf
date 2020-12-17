#
# Makefile component for openperf framework
#

FW_REQ_VARS := \
	OP_ROOT \
	OP_BUILD_ROOT \
	FW_MODULES
$(call op_check_vars,$(OP_REQ_VARS))

FW_SRC_DIR := $(OP_ROOT)/src/framework
FW_OBJ_DIR := $(OP_BUILD_ROOT)/obj/framework
FW_INC_DIR := $(OP_ROOT)/src/framework
FW_LIB_DIR := $(OP_BUILD_ROOT)/lib

# We explicitly state our includes, e.g. include module/<header.h>
OP_INC_DIRS += $(FW_INC_DIR)
OP_LIB_DIRS += $(FW_LIB_DIR)

FW_LISTS_SOURCES :=
FW_LISTS_DEPENDS :=
FW_LISTS_LDLIBS  :=

include $(FW_SRC_DIR)/core/directory.mk

FW_LISTS_OBJECTS := $(call op_generate_objects,$(FW_LISTS_SOURCES),$(FW_OBJ_DIR))

FW_LISTS_LIBRARY := openperf_framework_lists
FW_LISTS_TARGET := $(FW_LIB_DIR)/lib$(FW_LISTS_LIBRARY).a

OP_LDLIBS += -Wl,--whole-archive -l$(FW_LISTS_LIBRARY) -Wl,--no-whole-archive $(FW_LISTS_LDLIBS)

###
# Build rules
###

$(eval $(call op_generate_build_rules,$(FW_LISTS_SOURCES),FW_SRC_DIR,FW_OBJ_DIR,FW_LISTS_DEPENDS))
$(eval $(call op_generate_clean_rules,framework,FW_LISTS_TARGET,FW_LISTS_OBJECTS))

$(FW_LISTS_TARGET): $(FW_LISTS_OBJECTS)
	$(call op_link_library,$@,$(FW_LISTS_OBJECTS))

.PHONY: framework_lists
framework_lists: $(FW_LISTS_TARGET)
