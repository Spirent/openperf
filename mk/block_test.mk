#
# Makefile component for block code
#

BLOCK_TEST_REQ_VARS := \
	OP_ROOT \
	OP_BUILD_ROOT
$(call op_check_vars,$(BLOCK_TEST_REQ_VARS))

BLOCK_SRC_DIR := $(OP_ROOT)/src/modules/block
BLOCK_OBJ_DIR := $(OP_BUILD_ROOT)/obj/modules/block
BLOCK_LIB_DIR := $(OP_BUILD_ROOT)/lib

OP_INC_DIRS += $(OP_ROOT)/src/modules
OP_LIB_DIRS += $(BLOCK_LIB_DIR)

BLOCK_TEST_SOURCES :=
BLOCK_TEST_DEPENDS :=
BLOCK_TEST_LDLIBS :=

include $(BLOCK_SRC_DIR)/directory.mk

BLOCK_TEST_OBJECTS := $(call op_generate_objects,$(BLOCK_TEST_SOURCES),$(BLOCK_OBJ_DIR))

BLOCK_TEST_LIBRARY := openperf_block_test
BLOCK_TEST_TARGET := $(BLOCK_LIB_DIR)/lib$(BLOCK_TEST_LIBRARY).a

OP_LDLIBS += -Wl,--whole-archive -l$(BLOCK_TEST_LIBRARY) -Wl,--no-whole-archive $(BLOCK_TEST_LDLIBS)

# Load external dependencies
-include $(BLOCK_TEST_OBJECTS:.o=.d)
$(call op_include_dependencies,$(BLOCK_TEST_DEPENDS))

###
# Build rules
###
$(eval $(call op_generate_build_rules,$(BLOCK_TEST_SOURCES),BLOCK_SRC_DIR,BLOCK_OBJ_DIR,BLOCK_TEST_DEPENDS))
$(eval $(call op_generate_clean_rules,block,BLOCK_TEST_TARGET,BLOCK_TEST_OBJECTS))

$(BLOCK_TEST_TARGET): $(BLOCK_TEST_OBJECTS)
	$(call op_link_library,$@,$(BLOCK_TEST_OBJECTS))

.PHONY: block_test
block_test: $(BLOCK_TEST_TARGET)
