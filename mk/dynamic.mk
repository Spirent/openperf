#
# Makefile component for Dynamic Results module
#

DYNAMIC_REQ_VARS := \
	OP_ROOT \
	OP_BUILD_ROOT
$(call op_check_vars,$(DYNAMIC_REQ_VARS))

DYNAMIC_SRC_DIR := $(OP_ROOT)/src/modules/dynamic
DYNAMIC_OBJ_DIR := $(OP_BUILD_ROOT)/obj/modules/dynamic
DYNAMIC_LIB_DIR := $(OP_BUILD_ROOT)/lib

OP_INC_DIRS += $(OP_ROOT)/src/modules
OP_LIB_DIRS += $(DYNAMIC_LIB_DIR)

DYNAMIC_SOURCES :=
DYNAMIC_DEPENDS :=
DYNAMIC_LDLIBS :=

include $(DYNAMIC_SRC_DIR)/directory.mk

DYNAMIC_OBJECTS := $(call op_generate_objects,$(DYNAMIC_SOURCES),$(DYNAMIC_OBJ_DIR))

DYNAMIC_LIBRARY := openperf_dynamic
DYNAMIC_TARGET := $(DYNAMIC_LIB_DIR)/lib$(DYNAMIC_LIBRARY).a

OP_LDLIBS += -Wl,--whole-archive -l$(DYNAMIC_LIBRARY) -Wl,--no-whole-archive $(DYNAMIC_LDLIBS)

# Load external dependencies
-include $(DYNAMIC_OBJECTS:.o=.d)
$(call op_include_dependencies,$(DYNAMIC_DEPENDS))

###
# Build rules
###
$(eval $(call op_generate_build_rules,$(DYNAMIC_SOURCES),DYNAMIC_SRC_DIR,DYNAMIC_OBJ_DIR,DYNAMIC_DEPENDS))
$(eval $(call op_generate_clean_rules,dynamic,DYNAMIC_TARGET,DYNAMIC_OBJECTS))

$(DYNAMIC_TARGET): $(DYNAMIC_OBJECTS)
	$(call op_link_library,$@,$(DYNAMIC_OBJECTS))

.PHONY: dynamic
dynamic: $(DYNAMIC_TARGET)
