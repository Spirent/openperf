#
# Makefile component for memory generator
#

MEMORY_REQ_VARS := \
	OP_ROOT \
	OP_BUILD_ROOT
$(call op_check_vars,$(MEMORY_REQ_VARS))

MEMORY_SRC_DIR := $(OP_ROOT)/src/modules/memory
MEMORY_OBJ_DIR := $(OP_BUILD_ROOT)/obj/modules/memory
MEMORY_LIB_DIR := $(OP_BUILD_ROOT)/lib

OP_INC_DIRS += $(OP_ROOT)/src/modules
OP_LIB_DIRS += $(MEMORY_LIB_DIR)

MEMORY_SOURCES :=
MEMORY_DEPENDS :=
MEMORY_LDLIBS :=

include $(MEMORY_SRC_DIR)/directory.mk

MEMORY_OBJECTS := $(call op_generate_objects,$(MEMORY_SOURCES),$(MEMORY_OBJ_DIR))

MEMORY_LIBRARY := openperf_memory
MEMORY_TARGET := $(MEMORY_LIB_DIR)/lib$(MEMORY_LIBRARY).a

OP_LDLIBS += -Wl,--whole-archive -l$(MEMORY_LIBRARY) -Wl,--no-whole-archive $(MEMORY_LDLIBS)

# Load external dependencies
-include $(MEMORY_OBJECTS:.o=.d)
$(call op_include_dependencies,$(MEMORY_DEPENDS))

###
# Build rules
###
$(eval $(call op_generate_build_rules,$(MEMORY_SOURCES),MEMORY_SRC_DIR,MEMORY_OBJ_DIR,MEMORY_DEPENDS))
$(eval $(call op_generate_clean_rules,memory,MEMORY_TARGET,MEMORY_OBJECTS))

$(MEMORY_TARGET): $(MEMORY_OBJECTS)
	$(call op_link_library,$@,$(MEMORY_OBJECTS))

.PHONY: memory
memory: $(MEMORY_TARGET)
