#
# Makefile component for stack code
#

STACK_REQ_VARS := \
	OP_ROOT \
	OP_BUILD_ROOT
$(call op_check_vars,$(STACK_REQ_VARS))

STACK_SRC_DIR := $(OP_ROOT)/src/modules/packet/stack
STACK_OBJ_DIR := $(OP_BUILD_ROOT)/obj/modules/packet/stack
STACK_LIB_DIR := $(OP_BUILD_ROOT)/lib

OP_INC_DIRS += $(OP_ROOT)/src/modules
OP_LIB_DIRS += $(STACK_LIB_DIR)

STACK_SOURCES :=
STACK_DEPENDS :=
STACK_LDLIBS :=

include $(STACK_SRC_DIR)/directory.mk

STACK_OBJECTS := $(call op_generate_objects,$(STACK_SOURCES),$(STACK_OBJ_DIR))

STACK_INC_DIRS := $(dir $(STACK_SRC_DIR)) $(addprefix $(STACK_SRC_DIR)/,$(STACK_INCLUDES))
STACK_FLAGS := $(addprefix -I,$(STACK_INC_DIRS))

STACK_LIBRARY := openperf_packet_stack
STACK_TARGET := $(STACK_LIB_DIR)/lib$(STACK_LIBRARY).a

OP_INC_DIRS += $(STACK_INC_DIRS)
OP_LDLIBS += -Wl,--whole-archive -l$(STACK_LIBRARY) -Wl,--no-whole-archive $(STACK_LDLIBS)

# Load external dependencies
-include $(STACK_OBJECTS:.o=.d)
$(call op_include_dependencies,$(STACK_DEPENDS))

###
# We link the lwip compilation objects directly into the stack target as that
# provides the bulk of the core functionality.
###
include $(OP_ROOT)/mk/lwip.mk

LWIP_REQ_VARS := \
	LWIP_OBJECTS
$(call op_check_vars,$(LWIP_REQ_VARS))

###
# Build rules
###
$(eval $(call op_generate_build_rules,$(STACK_SOURCES),STACK_SRC_DIR,STACK_OBJ_DIR,STACK_DEPENDS,STACK_FLAGS))
$(eval $(call op_generate_clean_rules,packet_stack,STACK_TARGET,STACK_OBJECTS))

$(STACK_TARGET): $(STACK_OBJECTS) $(LWIP_OBJECTS)
	$(call op_link_library,$@,$(STACK_OBJECTS) $(LWIP_OBJECTS))

.PHONY: packet_stack
packet_stack: $(STACK_TARGET)

clean_packet_stack: clean_lwip
