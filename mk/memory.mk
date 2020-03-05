#
# Makefile component for memory generator
#

MEMGEN_REQ_VARS := \
	OP_ROOT \
	OP_BUILD_ROOT
$(call op_check_vars,$(MEMGEN_REQ_VARS))

MEMGEN_SOURCES :=
MEMGEN_INCLUDES :=
MEMGEN_DEPENDS :=
MEMGEN_LIBS :=

MEMGEN_SRC_DIR := $(OP_ROOT)/src/modules/memory
MEMGEN_OBJ_DIR := $(OP_BUILD_ROOT)/obj/modules/memory
MEMGEN_LIB_DIR := $(OP_BUILD_ROOT)/lib

include $(MEMGEN_SRC_DIR)/directory.mk

MEMGEN_OBJECTS := $(call op_generate_objects,$(MEMGEN_SOURCES),$(MEMGEN_OBJ_DIR))

MEMGEN_INC_DIRS := $(dir $(MEMGEN_SRC_DIR)) $(addprefix $(MEMGEN_SRC_DIR)/,$(MEMGEN_INCLUDES))
MEMGEN_FLAGS := $(addprefix -I,$(MEMGEN_INC_DIRS))

MEMGEN_LIBRARY := openperf_memory
MEMGEN_TARGET := $(MEMGEN_LIB_DIR)/lib$(MEMGEN_LIBRARY).a

OP_INC_DIRS += $(MEMGEN_INC_DIRS)
OP_LIB_DIRS += $(MEMGEN_LIB_DIR)
OP_LDLIBS += -Wl,--whole-archive -l$(MEMGEN_LIBRARY) -Wl,--no-whole-archive $(MEMGEN_LIBS)

# Load dependencies
-include $(MEMGEN_OBJECTS:.o=.d)
$(call op_include_dependencies,$(MEMGEN_DEPENDS))

###
# Build rules
###
$(eval $(call op_generate_build_rules,$(MEMGEN_SOURCES),MEMGEN_SRC_DIR,MEMGEN_OBJ_DIR,MEMGEN_DEPENDS))
$(eval $(call op_generate_clean_rules,memory,MEMGEN_TARGET,MEMGEN_OBJECTS))

$(MEMGEN_TARGET): $(MEMGEN_OBJECTS)
	$(call op_link_library,$@,$(MEMGEN_OBJECTS))

.PHONY: memory
memory: $(MEMGEN_TARGET)
