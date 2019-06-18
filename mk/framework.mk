#
# Makefile component for inception framework
#

FW_REQ_VARS := \
	ICP_ROOT \
	ICP_BUILD_ROOT \
	FW_MODULES
$(call icp_check_vars,$(ICP_REQ_VARS))

FW_SRC_DIR := $(ICP_ROOT)/src/framework
FW_OBJ_DIR := $(ICP_BUILD_ROOT)/obj/framework
FW_INC_DIR := $(ICP_ROOT)/src/framework
FW_LIB_DIR := $(ICP_BUILD_ROOT)/lib

# We explicitly state our includes, e.g. include module/<header.h>
ICP_INC_DIRS += $(FW_INC_DIR)
ICP_LIB_DIRS += $(FW_LIB_DIR)

FW_SOURCES :=
FW_DEPENDS :=
FW_LDLIBS  :=

FW_MODULES := core memory net

# Load each module's directory.mk file
include $(patsubst %, $(FW_SRC_DIR)/%/directory.mk, $(FW_MODULES))

FW_OBJECTS := $(call icp_generate_objects,$(FW_SOURCES),$(FW_OBJ_DIR))

FW_LIBRARY := icp_framework
FW_TARGET := $(FW_LIB_DIR)/lib$(FW_LIBRARY).a

# Update library build options before loading dependencies. That way
# archive ordering should be correct.
ICP_LDLIBS += -Wl,--whole-archive -l$(FW_LIBRARY) -Wl,--no-whole-archive $(FW_LDLIBS)

-include $(FW_OBJECTS:.o=.d)
$(call icp_include_dependencies,$(FW_DEPENDS))

###
# Build rules
###
$(eval $(call icp_generate_build_rules,$(FW_SOURCES),FW_SRC_DIR,FW_OBJ_DIR,FW_DEPENDS))
$(eval $(call icp_generate_clean_rules,framework,FW_TARGET,FW_OBJECTS))

$(FW_TARGET): $(FW_OBJECTS)
	$(call icp_link_library,$@,$(FW_OBJECTS))

.PHONY: framework
framework: $(FW_TARGET)
