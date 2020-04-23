#
# Makefile component for block code
#

BLOCK_REQ_VARS := \
	OP_ROOT \
	OP_BUILD_ROOT
$(call op_check_vars,$(BLOCK_REQ_VARS))

BLOCK_SRC_DIR := $(OP_ROOT)/src/modules/block
BLOCK_OBJ_DIR := $(OP_BUILD_ROOT)/obj/modules/block
BLOCK_LIB_DIR := $(OP_BUILD_ROOT)/lib

OP_INC_DIRS += $(OP_ROOT)/src/modules
OP_LIB_DIRS += $(BLOCK_LIB_DIR)

BLOCK_SOURCES :=
BLOCK_DEPENDS :=
BLOCK_LDLIBS :=

include $(BLOCK_SRC_DIR)/directory.mk

BLOCK_OBJECTS := $(call op_generate_objects,$(BLOCK_SOURCES),$(BLOCK_OBJ_DIR))

BLOCK_LIBRARY := openperf_block
BLOCK_TARGET := $(BLOCK_LIB_DIR)/lib$(BLOCK_LIBRARY).a

OP_LDLIBS += -Wl,--whole-archive -l$(BLOCK_LIBRARY) -Wl,--no-whole-archive $(BLOCK_LDLIBS)

# Load external dependencies
-include $(BLOCK_OBJECTS:.o=.d)
$(call op_include_dependencies,$(BLOCK_DEPENDS))

###
# Build rules
###
$(eval $(call op_generate_build_rules,$(BLOCK_SOURCES),BLOCK_SRC_DIR,BLOCK_OBJ_DIR,BLOCK_DEPENDS))
$(eval $(call op_generate_clean_rules,block,BLOCK_TARGET,BLOCK_OBJECTS))

$(BLOCK_TARGET): $(BLOCK_OBJECTS)
	$(call op_link_library,$@,$(BLOCK_OBJECTS))

.PHONY: block
block: $(BLOCK_TARGET)
