#
# Makefile component for timesync code
#

TS_TEST_REQ_VARS := \
	OP_ROOT \
	OP_BUILD_ROOT
$(call op_check_vars,$(TS_TEST_REQ_VARS))

TS_TEST_SRC_DIR := $(OP_ROOT)/src/modules/timesync
TS_TEST_OBJ_DIR := $(OP_BUILD_ROOT)/obj/modules/timesync
TS_TEST_LIB_DIR := $(OP_BUILD_ROOT)/lib

OP_INC_DIRS += $(OP_ROOT)/src/modules
OP_LIB_DIRS += $(TS_TEST_LIB_DIR)

TS_TEST_SOURCES :=
TS_TEST_DEPENDS :=
TS_TEST_LDLIBS :=

include $(TS_TEST_SRC_DIR)/directory.mk

TS_TEST_OBJECTS := $(call op_generate_objects,$(TS_TEST_SOURCES),$(TS_TEST_OBJ_DIR))

TS_TEST_LIBRARY := openperf_timesync_test
TS_TEST_TARGET := $(TS_TEST_LIB_DIR)/lib$(TS_TEST_LIBRARY).a

OP_LDLIBS += -Wl,--whole-archive -l$(TS_TEST_LIBRARY) -Wl,--no-whole-archive $(TS_TEST_LDLIBS)

# Load external dependencies
-include $(TS_TEST_OBJECTS:.o=.d)
$(call op_include_dependencies,$(TS_TEST_DEPENDS))

###
# Build rules
###
$(eval $(call op_generate_build_rules,$(TS_TEST_SOURCES),TS_TEST_SRC_DIR,TS_TEST_OBJ_DIR,TS_TEST_DEPENDS))
$(eval $(call op_generate_clean_rules,timesync,TS_TEST_TARGET,TS_TEST_OBJECTS))

$(TS_TEST_TARGET): $(TS_TEST_OBJECTS)
	$(call op_link_library,$@,$(TS_TEST_OBJECTS))

.PHONY: timesync_test
timesync_test: $(TS_TEST_TARGET)
