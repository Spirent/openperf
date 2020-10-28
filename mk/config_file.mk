#
# Makefile component for OpenPerf configuration file support.
#

CFGFILE_REQ_VARS := \
	OP_ROOT \
	OP_BUILD_ROOT
$(call op_check_vars,$(CFGFILE_REQ_VARS))

CFGFILE_SRC_DIR := $(OP_ROOT)/src/framework/config
CFGFILE_OBJ_DIR := $(OP_BUILD_ROOT)/obj/framework
CFGFILE_INC_DIR := $(OP_ROOT)/src/framework/config
CFGFILE_LIB_DIR := $(OP_BUILD_ROOT)/lib

CFGFILE_SOURCES :=
CFGFILE_DEPENDS :=
CFGFILE_LDLIBS  :=

include $(CFGFILE_SRC_DIR)/directory.mk

CFGFILE_OBJECTS := $(call op_generate_objects,$(CFGFILE_SOURCES),$(CFGFILE_OBJ_DIR))

CFGFILE_LIBRARY := openperf_config_file
CFGFILE_TARGET := $(CFGFILE_LIB_DIR)/lib$(CFGFILE_LIBRARY).a

OP_INC_DIRS += $(CFGFILE_INC_DIR)
OP_LIB_DIRS += $(CFGFILE_LIB_DIR)
OP_LDLIBS += -Wl,--whole-archive -l$(CFGFILE_LIBRARY) -Wl,--no-whole-archive $(CFGFILE_LDLIBS)

# Load external dependencies, too
-include $(CFGFILE_OBJECTS:.o=.d)
$(call op_include_dependencies,$(CFGFILE_DEPENDS))

###
# Build rules
###
$(eval $(call op_generate_build_rules,$(CFGFILE_SOURCES),CFGFILE_SRC_DIR,CFGFILE_OBJ_DIR,CFGFILE_DEPENDS,CFGFILE_FLAGS))
$(eval $(call op_generate_clean_rules,config_file,CFGFILE_TARGET,CFGFILE_OBJECTS))

# XXX: I don't know why this explicit dependency is necessary to pull in yaml_cpp.
$(CFGFILE_TARGET): $(CFGFILE_OBJECTS) | $(CFGFILE_DEPENDS)
	$(call op_link_library,$@,$(CFGFILE_OBJECTS))

.PHONY: config_file
config_file: $(CFGFILE_TARGET)
