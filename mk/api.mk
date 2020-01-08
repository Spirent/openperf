#
# Makefile component for openperf framework
#

API_REQ_VARS := \
	OP_ROOT \
	OP_BUILD_ROOT
$(call op_check_vars,$(OP_REQ_VARS))

API_SRC_DIR := $(OP_ROOT)/src/modules/api
API_OBJ_DIR := $(OP_BUILD_ROOT)/obj/modules/api
API_LIB_DIR := $(OP_BUILD_ROOT)/lib

# We explicitly state our includes, e.g. include module/<header.h>
OP_INC_DIRS += $(OP_ROOT)/src/modules
OP_LIB_DIRS += $(API_LIB_DIR)

API_SOURCES :=
API_DEPENDS :=
API_LDLIBS  :=

include $(API_SRC_DIR)/directory.mk

API_OBJECTS := $(call op_generate_objects,$(API_SOURCES),$(API_OBJ_DIR))

API_LIBRARY := openperf_api
API_TARGET := $(API_LIB_DIR)/lib$(API_LIBRARY).a

# Update library build options before loading dependencies. That way
# archive ordering should be correct.
OP_LDLIBS += -Wl,--whole-archive -l$(API_LIBRARY) -Wl,--no-whole-archive $(API_LDLIBS)

# Load external dependencies, too
-include $(API_OBJECTS:.o=.d)
$(call op_include_dependencies,$(API_DEPENDS))

###
# Build rules
###
$(eval $(call op_generate_build_rules,$(API_SOURCES),API_SRC_DIR,API_OBJ_DIR,API_DEPENDS,API_FLAGS))
$(eval $(call op_generate_clean_rules,api,API_TARGET,API_OBJECTS))

$(API_TARGET): $(API_OBJECTS)
	$(call op_link_library,$@,$(API_OBJECTS))

.PHONY: api
api: $(API_TARGET)
