#
# Makefile component for block code
#

TVLP_REQ_VARS := \
	OP_ROOT \
	OP_BUILD_ROOT
$(call op_check_vars,$(TVLP_REQ_VARS))

TVLP_SRC_DIR := $(OP_ROOT)/src/modules/block
TVLP_OBJ_DIR := $(OP_BUILD_ROOT)/obj/modules/block
TVLP_LIB_DIR := $(OP_BUILD_ROOT)/lib

OP_INC_DIRS += $(OP_ROOT)/src/modules
OP_LIB_DIRS += $(TVLP_LIB_DIR)

TVLP_SOURCES :=
TVLP_DEPENDS :=
TVLP_LDLIBS :=

include $(TVLP_SRC_DIR)/directory.mk

TVLP_OBJECTS := $(call op_generate_objects,$(TVLP_SOURCES),$(TVLP_OBJ_DIR))

TVLP_LIBRARY := openperf_block
TVLP_TARGET := $(TVLP_LIB_DIR)/lib$(TVLP_LIBRARY).a

OP_LDLIBS += -Wl,--whole-archive -l$(TVLP_LIBRARY) -Wl,--no-whole-archive $(TVLP_LDLIBS)

# Load external dependencies
-include $(TVLP_OBJECTS:.o=.d)
$(call op_include_dependencies,$(TVLP_DEPENDS))

###
# Build rules
###
$(eval $(call op_generate_build_rules,$(TVLP_SOURCES),TVLP_SRC_DIR,TVLP_OBJ_DIR,TVLP_DEPENDS))
$(eval $(call op_generate_clean_rules,block,TVLP_TARGET,TVLP_OBJECTS))

$(TVLP_TARGET): $(TVLP_OBJECTS)
	$(call op_link_library,$@,$(TVLP_OBJECTS))

.PHONY: block
block: $(TVLP_TARGET)
