#
# Makefile component for timesync code
#

TS_REQ_VARS := \
	OP_ROOT \
	OP_BUILD_ROOT
$(call op_check_vars,$(TS_REQ_VARS))

TS_SRC_DIR := $(OP_ROOT)/src/modules/timesync
TS_OBJ_DIR := $(OP_BUILD_ROOT)/obj/modules/timesync
TS_LIB_DIR := $(OP_BUILD_ROOT)/lib

OP_INC_DIRS += $(OP_ROOT)/src/modules
OP_LIB_DIRS += $(TS_LIB_DIR)

TS_SOURCES :=
TS_DEPENDS :=
TS_LDLIBS :=

include $(TS_SRC_DIR)/directory.mk

TS_OBJECTS := $(call op_generate_objects,$(TS_SOURCES),$(TS_OBJ_DIR))

TS_LIBRARY := openperf_timesync
TS_TARGET := $(TS_LIB_DIR)/lib$(TS_LIBRARY).a

OP_LDLIBS += -Wl,--whole-archive -l$(TS_LIBRARY) -Wl,--no-whole-archive $(TS_LDLIBS)

# Load external dependencies
-include $(TS_OBJECTS:.o=.d)
$(call op_include_dependencies,$(TS_DEPENDS))

###
# Build rules
###
$(eval $(call op_generate_build_rules,$(TS_SOURCES),TS_SRC_DIR,TS_OBJ_DIR,TS_DEPENDS))
$(eval $(call op_generate_clean_rules,timesync,TS_TARGET,TS_OBJECTS))

$(TS_TARGET): $(TS_OBJECTS)
	$(call op_link_library,$@,$(TS_OBJECTS))

.PHONY: timesync
timesync: $(TS_TARGET)
