#
# Makefile component for Inception configuration file support.
#

CFGFILE_REQ_VARS := \
	ICP_ROOT \
	ICP_BUILD_ROOT
$(call icp_check_vars,$(CFGFILE_REQ_VARS))

CFGFILE_SRC_DIR := $(ICP_ROOT)/src/framework/config
CFGFILE_OBJ_DIR := $(ICP_BUILD_ROOT)/obj/framework
CFGFILE_INC_DIR := $(ICP_ROOT)/src/framework/config
CFGFILE_LIB_DIR := $(ICP_BUILD_ROOT)/lib

CFGFILE_SOURCES :=
CFGFILE_DEPENDS :=
CFGFILE_LDLIBS  :=

include $(CFGFILE_SRC_DIR)/directory.mk

CFGFILE_OBJECTS := $(call icp_generate_objects,$(CFGFILE_SOURCES),$(CFGFILE_OBJ_DIR))

CFGFILE_LIBRARY := icp_config_file
CFGFILE_TARGET := $(CFGFILE_LIB_DIR)/lib$(CFGFILE_LIBRARY).a

ICP_INC_DIRS += $(CFGFILE_INC_DIR)
ICP_LIB_DIRS += $(CFGFILE_LIB_DIR)
ICP_LDLIBS += -Wl,--whole-archive -l$(CFGFILE_LIBRARY) -Wl,--no-whole-archive $(CFGFILE_LDLIBS)

# Load external dependencies, too
-include $(CFGFILE_OBJECTS:.o=.d)
$(call icp_include_dependencies,$(CFGFILE_DEPENDS))

###
# Build rules
###
$(eval $(call icp_generate_build_rules,$(CFGFILE_SOURCES),CFGFILE_SRC_DIR,CFGFILE_OBJ_DIR,CFGFILE_DEPENDS))
$(eval $(call icp_generate_clean_rules,config_file,CFGFILE_TARGET,CFGFILE_OBJECTS))

$(CFGFILE_TARGET): $(CFGFILE_OBJECTS) $(CFGFILE_DEPENDS)
	$(call icp_link_library,$@,$(CFGFILE_OBJECTS))

.PHONY: config_file
config_file: $(CFGFILE_TARGET)
