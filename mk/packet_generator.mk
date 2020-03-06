#
# Makefile component for packet generator code
#

PG_REQ_VARS := \
	OP_ROOT \
	OP_BUILD_ROOT
$(call op_check_vars,$(PG_REQ_VARS))

PG_SRC_DIR := $(OP_ROOT)/src/modules/packet/generator
PG_OBJ_DIR := $(OP_BUILD_ROOT)/obj/modules/packet/generator
PG_LIB_DIR := $(OP_BUILD_ROOT)/lib

OP_INC_DIRS += $(OP_ROOT)/src/modules
OP_LIB_DIRS += $(PG_LIB_DIR)

PG_SOURCES :=
PG_DEPENDS :=
PG_LDLIBS :=

include $(PG_SRC_DIR)/directory.mk

PG_OBJECTS := $(call op_generate_objects,$(PG_SOURCES),$(PG_OBJ_DIR))

PG_LIBRARY := openperf_packet_generator
PG_TARGET := $(PG_LIB_DIR)/lib$(PG_LIBRARY).a

OP_LDLIBS += -Wl,--whole-archive -l$(PG_LIBRARY) -Wl,--no-whole-archive $(PG_LDLIBS)

# Load external dependencies
-include $(PG_OBJECTS:.o=.d)
$(call op_include_dependencies,$(PG_DEPENDS))

###
# Build rules
###
$(eval $(call op_generate_build_rules,$(PG_SOURCES),PG_SRC_DIR,PG_OBJ_DIR,PG_DEPENDS))
$(eval $(call op_generate_clean_rules,packet_generator,PG_TARGET,PG_OBJECTS))

$(PG_TARGET): $(PG_OBJECTS)
	$(call op_link_library,$@,$(PG_OBJECTS))

.PHONY: packet_generator
packet_generator: $(PG_TARGET)
