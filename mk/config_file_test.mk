#
# Makefile component for Inception configuration file support.
#

CFGFILETEST_REQ_VARS := \
	ICP_ROOT \
	ICP_BUILD_ROOT
$(call icp_check_vars,$(CFGFILETEST_REQ_VARS))

CFGFILETEST_SRC_DIR := $(ICP_ROOT)/src/framework/config
CFGFILETEST_OBJ_DIR := $(ICP_BUILD_ROOT)/obj/framework
CFGFILETEST_INC_DIR := $(ICP_ROOT)/src/framework/config
CFGFILETEST_LIB_DIR := $(ICP_BUILD_ROOT)/lib

CFGFILETEST_SOURCES :=
CFGFILETEST_DEPENDS :=
CFGFILETEST_LDLIBS  :=

include $(CFGFILETEST_SRC_DIR)/module.mk

CFGFILETEST_OBJECTS := $(call icp_generate_objects,$(CFGFILETEST_SOURCES),$(CFGFILETEST_OBJ_DIR))

CFGFILETEST_LIBRARY := icp_config_file_test
CFGFILETEST_TARGET := $(CFGFILETEST_LIB_DIR)/lib$(CFGFILETEST_LIBRARY).a

ICP_INC_DIRS += $(CFGFILETEST_INC_DIR)
ICP_LIB_DIRS += $(CFGFILETEST_LIB_DIR)
ICP_LDLIBS += -Wl,--whole-archive -l$(CFGFILETEST_LIBRARY) -Wl,--no-whole-archive $(CFGFILETEST_LDLIBS)

# Load external dependencies
-include $(CFGFILETEST_OBJECTS:.o=.d)
$(call icp_include_dependencies,$(CFGFILETEST_DEPENDS))

###
# Build rules
###
$(eval $(call icp_generate_build_rules,$(CFGFILETEST_SOURCES),CFGFILETEST_SRC_DIR,CFGFILETEST_OBJ_DIR,CFGFILETEST_DEPENDS))
$(eval $(call icp_generate_clean_rules,config_file_test,CFGFILETEST_TARGET,CFGFILETEST_OBJECTS))

$(CFGFILETEST_TARGET): $(CFGFILETEST_OBJECTS) $(CFGFILETEST_DEPENDS)
	$(call icp_link_library,$@,$(CFGFILETEST_OBJECTS))

.PHONY: config_file_test
config_file_test: $(CFGFILETEST_TARGET)
