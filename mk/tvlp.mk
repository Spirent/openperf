#
# Makefile component for TVLP code
#

TVLP_REQ_VARS := \
	OP_ROOT \
	OP_BUILD_ROOT
$(call op_check_vars,$(TVLP_REQ_VARS))

TVLP_SRC_DIR := $(OP_ROOT)/src/modules/tvlp
TVLP_OBJ_DIR := $(OP_BUILD_ROOT)/obj/modules/tvlp
TVLP_LIB_DIR := $(OP_BUILD_ROOT)/plugins

OP_INC_DIRS += $(OP_ROOT)/src/modules
OP_LIB_DIRS += $(TVLP_LIB_DIR)
#OP_CPPFLAGS += -fPIC

TVLP_SOURCES :=
TVLP_DEPENDS :=
TVLP_LDLIBS := -lswagger_v1_model
TVLP_FLAGS:= -fPIC

include $(TVLP_SRC_DIR)/directory.mk

TVLP_OBJECTS := $(call op_generate_objects,$(TVLP_SOURCES),$(TVLP_OBJ_DIR))

TVLP_LIBRARY := openperf_tvlp
TVLP_TARGET := $(TVLP_LIB_DIR)/lib$(TVLP_LIBRARY).so

# Load external dependencies
-include $(TVLP_OBJECTS:.o=.d)
$(call op_include_dependencies,$(TVLP_DEPENDS))

###
# Build rules
###
$(eval $(call op_generate_build_rules,$(TVLP_SOURCES),TVLP_SRC_DIR,TVLP_OBJ_DIR,TVLP_DEPENDS,TVLP_FLAGS))
$(eval $(call op_generate_clean_rules,tvlp,TVLP_TARGET,TVLP_OBJECTS))

$(TVLP_TARGET): $(TVLP_OBJECTS)
	$(call op_link_plugin,$@,$(TVLP_OBJECTS),$(TVLP_LDLIBS))

.PHONY: tvlp
tvlp: $(TVLP_TARGET)
