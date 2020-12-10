#
# Makefile component for sljit
#

SLJIT_REQ_VARS := \
	OP_ROOT \
	OP_BUILD_ROOT
$(call op_check_vars,$(SLJIT_REQ_VARS))

SLJIT_SRC_DIR := $(OP_ROOT)/deps/sljit
SLJIT_OBJ_DIR := $(OP_BUILD_ROOT)/obj/sljit
SLJIT_BLD_DIR := $(OP_BUILD_ROOT)/sljit
SLJIT_INC_DIR := $(SLJIT_SRC_DIR)/sljit_src
SLJIT_LIB_DIR := $(SLJIT_BLD_DIR)/lib

SLJIT_FLAGS := -DSLJIT_CONFIG_AUTO=1

# Update global variables
OP_INC_DIRS += $(SLJIT_INC_DIR)
OP_LIB_DIRS += $(SLJIT_LIB_DIR)
OP_LDLIBS += -lsljit
OP_DEFINES += $(SLJIT_FLAGS)

SLJIT_SOURCES += \
	sljit_src/sljitLir.c

SLJIT_OBJECTS := $(call op_generate_objects,$(SLJIT_SOURCES),$(SLJIT_OBJ_DIR))

# Pull in object dependencies, maybe
-include $(SLJIT_OBJECTS:.o=.d)

SLJIT_TARGET := $(SLJIT_LIB_DIR)/libsljit.a

###
# Build rules
###
$(eval $(call op_generate_build_rules,$(SLJIT_SOURCES),SLJIT_SRC_DIR,SLJIT_OBJ_DIR,,SLJIT_FLAGS))
$(eval $(call op_generate_clean_rules,sljit,SLJIT_TARGET,SLJIT_OBJECTS))

$(SLJIT_OBJECTS): OP_CFLAGS += -Wno-sign-compare

$(SLJIT_TARGET): $(SLJIT_OBJECTS)
	$(call op_link_library,$@,$(SLJIT_OBJECTS))

.PHONY: sljit
sljit: $(SLJIT_TARGET)
