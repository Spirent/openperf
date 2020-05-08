#
# Makefile component for packet generator code
#

PG_TEST_REQ_VARS := \
	OP_ROOT \
	OP_BUILD_ROOT
$(call op_check_vars,$(PG_TEST_REQ_VARS))

PG_TEST_SRC_DIR := $(OP_ROOT)/src/modules/packet/generator
PG_TEST_OBJ_DIR := $(OP_BUILD_ROOT)/obj/modules/packet/generator
PG_TEST_LIB_DIR := $(OP_BUILD_ROOT)/lib

OP_INC_DIRS += $(OP_ROOT)/src/modules
OP_LIB_DIRS += $(PG_TEST_LIB_DIR)

PG_TEST_SOURCES :=
PG_TEST_DEPENDS :=
PG_TEST_LDLIBS :=

include $(PG_TEST_SRC_DIR)/directory.mk

PG_TEST_OBJECTS := $(call op_generate_objects,$(PG_TEST_SOURCES),$(PG_TEST_OBJ_DIR))

PG_TEST_LIBRARY := openperf_packet_generator
PG_TEST_TARGET := $(PG_TEST_LIB_DIR)/lib$(PG_TEST_LIBRARY).a

OP_LDLIBS += -Wl,--whole-archive -l$(PG_TEST_LIBRARY) -Wl,--no-whole-archive $(PG_TEST_LDLIBS)

# Load external dependencies
-include $(PG_TEST_OBJECTS:.o=.d)
$(call op_include_dependencies,$(PG_TEST_DEPENDS))

###
# Build rules
###
$(eval $(call op_generate_build_rules,$(PG_TEST_SOURCES),PG_TEST_SRC_DIR,PG_TEST_OBJ_DIR,PG_TEST_DEPENDS))
$(eval $(call op_generate_clean_rules,packet_generator_test,PG_TEST_TARGET,PG_TEST_OBJECTS))

$(PG_TEST_TARGET): $(PG_TEST_OBJECTS)
	$(call op_link_library,$@,$(PG_TEST_OBJECTS))

.PHONY: packet_generator
packet_generator_test: $(PG_TEST_TARGET)
