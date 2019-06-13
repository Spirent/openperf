#
# Makefile component for API unit tests
#

API_TEST_REQ_VARS := \
	ICP_ROOT \
	ICP_BUILD_ROOT
$(call icp_check_vars,$(ICP_REQ_VARS))

API_TEST_SRC_DIR := $(ICP_ROOT)/src/modules/api
API_TEST_OBJ_DIR := $(ICP_BUILD_ROOT)/obj/modules/api
API_TEST_LIB_DIR := $(ICP_BUILD_ROOT)/lib

# We explicitly state our includes, e.g. include module/<header.h>
ICP_INC_DIRS += $(API_TEST_INC_DIR)
ICP_LIB_DIRS += $(API_TEST_LIB_DIR)

API_TEST_SOURCES :=
API_TEST_DEPENDS :=
API_TEST_LDLIBS  :=

include $(API_TEST_SRC_DIR)/module.mk

API_TEST_OBJECTS := $(call icp_generate_objects,$(API_TEST_SOURCES),$(API_TEST_OBJ_DIR))

API_TEST_LIBRARY := icp_api_test
API_TEST_TARGET := $(API_TEST_LIB_DIR)/lib$(API_TEST_LIBRARY).a

# Update library build options before loading dependencies. That way
# archive ordering should be correct.
ICP_LDLIBS += -Wl,--whole-archive -l$(API_TEST_LIBRARY) -Wl,--no-whole-archive $(API_TEST_LDLIBS)

# Load external dependencies
-include $(API_TEST_OBJECTS:.o=.d)
$(call icp_include_dependencies,$(API_TEST_DEPENDS))

###
# Build rules
###
$(eval $(call icp_generate_build_rules,$(API_TEST_SOURCES),API_TEST_SRC_DIR,API_TEST_OBJ_DIR,API_TEST_DEPENDS,API_TEST_FLAGS))
$(eval $(call icp_generate_clean_rules,api_test,API_TEST_TARGET,API_TEST_OBJECTS))

$(API_TEST_TARGET): $(API_TEST_OBJECTS)
	$(call icp_link_library,$@,$(API_TEST_OBJECTS))

.PHONY: api_test
api_test: $(API_TEST_TARGET)
