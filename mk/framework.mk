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

FW_SOURCES :=
FW_DEPENDS :=
FW_LDLIBS  :=

FW_MODULES := core memory

# Load each module's directory.mk file
include $(patsubst %, $(FW_SRC_DIR)/%/directory.mk, $(FW_MODULES))

FW_OBJECTS := $(call op_generate_objects,$(FW_SOURCES),$(FW_OBJ_DIR))

FW_LIBRARY := openperf_framework
FW_TARGET := $(FW_LIB_DIR)/lib$(FW_LIBRARY).a

# Update library build options before loading dependencies. That way
# archive ordering should be correct.
OP_LDLIBS += -Wl,--whole-archive -l$(FW_LIBRARY) -Wl,--no-whole-archive $(FW_LDLIBS)

-include $(FW_OBJECTS:.o=.d)
$(call op_include_dependencies,$(FW_DEPENDS))

###
# Build rules
###
$(eval $(call op_generate_build_rules,$(FW_SOURCES),FW_SRC_DIR,FW_OBJ_DIR,FW_DEPENDS))
$(eval $(call op_generate_clean_rules,framework,FW_TARGET,FW_OBJECTS))

$(FW_TARGET): $(FW_OBJECTS)
	$(call op_link_library,$@,$(FW_OBJECTS))

.PHONY: framework
framework: $(FW_TARGET)
