#
# Makefile component for our the inception stack
#

STACK_REQ_VARS := \
	ICP_ROOT \
	ICP_BUILD_ROOT
$(call icp_check_vars,$(STACK_REQ_VARS))

STACK_SRC_DIR := $(ICP_ROOT)/src/stack
STACK_OBJ_DIR := $(ICP_BUILD_ROOT)/obj/stack
STACK_INC_DIR := $(STACK_SRC_DIR)/include
STACK_LIB_DIR := $(ICP_BUILD_ROOT)/stack

STACK_SOURCES :=
STACK_INCLUDES :=

include $(STACK_SRC_DIR)/files.mk

STACK_OBJECTS := $(patsubst %, $(STACK_OBJ_DIR)/%, \
	$(patsubst %.c, %.o, $(filter %.c, $(STACK_SOURCES))))

# Pull in dependencies
-include $(STACK_OBJECTS:.o=.d)

STACK_CPPFLAGS := $(addprefix -I,\
	$(STACK_SRC_DIR) $(addprefix $(STACK_SRC_DIR)/,$(STACK_INCLUDES)))
STACK_TARGET := $(STACK_LIB_DIR)/libstack.a

ICP_INC_DIRS += $(STACK_INC_DIR)
ICP_LIB_DIRS += $(STACK_LIB_DIR)
ICP_LDLIBS += -Wl,--whole-archive -lstack -Wl,--no-whole-archive

###
# Build rules
###

$(STACK_OBJ_DIR)/%.o: $(STACK_SRC_DIR)/%.c
	@mkdir -p $(dir $@)
	$(strip $(ICP_CC) -o $@ -c $(STACK_CPPFLAGS) $(STACK_CFLAGS) $(ICP_COPTS) $<)

$(STACK_LIB_DIR)/libstack.a: $(STACK_OBJECTS)
	@mkdir -p $(dir $@)
	$(strip $(ICP_AR) $(ICP_ARFLAGS) $@ $(STACK_OBJECTS))

.PHONY: stack
stack: $(STACK_TARGET)

.PHONY: clean_stack
clean_stack:
	@rm -rf $(STACK_OBJ_DIR) $(STACK_LIB_DIR)
clean: clean_stack
