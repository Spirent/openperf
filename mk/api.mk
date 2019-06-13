#
# Makefile component for inception framework
#

API_REQ_VARS := \
	ICP_ROOT \
	ICP_BUILD_ROOT
$(call icp_check_vars,$(ICP_REQ_VARS))

API_SRC_DIR := $(ICP_ROOT)/src/modules/api
API_OBJ_DIR := $(ICP_BUILD_ROOT)/obj/modules/api
API_LIB_DIR := $(ICP_BUILD_ROOT)/lib

# We explicitly state our includes, e.g. include module/<header.h>
ICP_INC_DIRS += $(API_INC_DIR)
ICP_LIB_DIRS += $(API_LIB_DIR)

API_SOURCES :=
API_DEPENDS :=
API_LDLIBS  :=

include $(API_SRC_DIR)/module.mk

API_OBJECTS := $(call icp_generate_objects,$(API_SOURCES),$(API_OBJ_DIR))

API_LIBRARY := icp_api
API_TARGET := $(API_LIB_DIR)/lib$(API_LIBRARY).a

# Update library build options before loading dependencies. That way
# archive ordering should be correct.
ICP_LDLIBS += -Wl,--whole-archive -l$(API_LIBRARY) -Wl,--no-whole-archive $(API_LDLIBS)

# Load external dependencies, too
-include $(API_OBJECTS:.o=.d)
$(call icp_include_dependencies,$(API_DEPENDS))

###
# Build rules
###
$(eval $(call icp_generate_build_rules,$(API_SOURCES),API_SRC_DIR,API_OBJ_DIR,API_DEPENDS,API_FLAGS))
$(eval $(call icp_generate_clean_rules,api,API_TARGET,API_OBJECTS))

$(API_TARGET): $(API_OBJECTS)
	$(call icp_link_library,$@,$(API_OBJECTS))

.PHONY: api
api: $(API_TARGET)
