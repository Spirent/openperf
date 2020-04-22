#
# Makefile component for memory testing
#

MEMORY_TEST_REQ_VARS := \
	OP_ROOT \
	OP_BUILD_ROOT
$(call op_check_vars,$(MEMORY_TEST_REQ_VARS))

MEMORY_TEST_SRC_DIR := $(OP_ROOT)/src/modules/memory
MEMORY_TEST_OBJ_DIR := $(OP_BUILD_ROOT)/obj/modules/memory
MEMORY_TEST_LIB_DIR := $(OP_BUILD_ROOT)/lib

OP_INC_DIRS += $(OP_ROOT)/src/modules
OP_LIB_DIRS += $(MEMORY_TEST_LIB_DIR)

MEMORY_TEST_SOURCES :=
MEMORY_TEST_DEPENDS :=
MEMORY_TEST_LDLIBS :=

include $(MEMORY_TEST_SRC_DIR)/directory.mk

MEMORY_TEST_OBJECTS := $(call op_generate_objects,$(MEMORY_TEST_SOURCES),$(MEMORY_TEST_OBJ_DIR))

MEMORY_TEST_LIBRARY := openperf_memory_test
MEMORY_TEST_TARGET := $(MEMORY_TEST_LIB_DIR)/lib$(MEMORY_TEST_LIBRARY).a

OP_LDLIBS += -Wl,--whole-archive -l$(MEMORY_TEST_LIBRARY) -Wl,--no-whole-archive $(MEMORY_TEST_LDLIBS)

# Load external dependencies
-include $(MEMORY_TEST_OBJECTS:.o=.d)
$(call op_include_dependencies,$(MEMORY_TEST_DEPENDS))

###
# Build rules
###
$(eval $(call op_generate_build_rules,$(MEMORY_TEST_SOURCES),MEMORY_TEST_SRC_DIR,MEMORY_TEST_OBJ_DIR,MEMORY_TEST_DEPENDS))
$(eval $(call op_generate_clean_rules,memory,MEMORY_TEST_TARGET,MEMORY_TEST_OBJECTS))

$(MEMORY_TEST_TARGET): $(MEMORY_TEST_OBJECTS)
	$(call op_link_library,$@,$(MEMORY_TEST_OBJECTS))

.PHONY: memory_test
memory_test: $(MEMORY_TEST_TARGET)
