#
# Makefile component for API unit tests
#

API_TEST_REQ_VARS := \
	OP_ROOT \
	OP_BUILD_ROOT
$(call op_check_vars,$(OP_REQ_VARS))

API_TEST_SRC_DIR := $(OP_ROOT)/src/modules/api
API_TEST_OBJ_DIR := $(OP_BUILD_ROOT)/obj/modules/api
API_TEST_LIB_DIR := $(OP_BUILD_ROOT)/lib

# We explicitly state our includes, e.g. include module/<header.h>
OP_INC_DIRS += $(API_TEST_INC_DIR)
OP_LIB_DIRS += $(API_TEST_LIB_DIR)

API_TEST_SOURCES :=
API_TEST_DEPENDS :=
API_TEST_LDLIBS  :=

include $(API_TEST_SRC_DIR)/directory.mk

API_TEST_OBJECTS := $(call op_generate_objects,$(API_TEST_SOURCES),$(API_TEST_OBJ_DIR))

API_TEST_LIBRARY := openperf_api_test
API_TEST_TARGET := $(API_TEST_LIB_DIR)/lib$(API_TEST_LIBRARY).a

# Update library build options before loading dependencies. That way
# archive ordering should be correct.
OP_LDLIBS += -Wl,--whole-archive -l$(API_TEST_LIBRARY) -Wl,--no-whole-archive $(API_TEST_LDLIBS)

# Load external dependencies
-include $(API_TEST_OBJECTS:.o=.d)
$(call op_include_dependencies,$(API_TEST_DEPENDS))

###
# Build rules
###
$(eval $(call op_generate_build_rules,$(API_TEST_SOURCES),API_TEST_SRC_DIR,API_TEST_OBJ_DIR,API_TEST_DEPENDS,API_TEST_FLAGS))
$(eval $(call op_generate_clean_rules,api_test,API_TEST_TARGET,API_TEST_OBJECTS))

$(API_TEST_TARGET): $(API_TEST_OBJECTS)
	$(call op_link_library,$@,$(API_TEST_OBJECTS))

.PHONY: api_test
api_test: $(API_TEST_TARGET)
