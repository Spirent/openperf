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
CFGFILETEST_OBJECTS :=
CFGFILETEST_DEPENDS :=
CFGFILETEST_LDLIBS  :=

include $(CFGFILETEST_SRC_DIR)/module.mk

CFGFILETEST_OBJECTS += $(patsubst %, $(CFGFILETEST_OBJ_DIR)/%, \
	$(patsubst %.c, %.o, $(filter %.c, $(CFGFILETEST_SOURCES))))

CFGFILETEST_OBJECTS += $(patsubst %, $(CFGFILETEST_OBJ_DIR)/%, \
	$(patsubst %.cpp, %.o, $(filter %.cpp, $(CFGFILETEST_SOURCES))))

CFGFILETEST_LIBRARY := icp_config_file_test
CFGFILETEST_TARGET := $(CFGFILETEST_LIB_DIR)/lib$(CFGFILETEST_LIBRARY).a

-include $(CFGFILETEST_OBJECTS:.o=.d)

ICP_INC_DIRS += $(CFGFILETEST_INC_DIR)
ICP_LIB_DIRS += $(CFGFILETEST_LIB_DIR)
ICP_LDLIBS += -l$(CFGFILETEST_LIBRARY) $(CFGFILETEST_LDLIBS)

# Load external dependencies, too
$(foreach DEP, $(CFGFILETEST_DEPENDS), $(eval include $(ICP_ROOT)/mk/$(DEP).mk))

###
# Build rules
###
$(CFGFILETEST_OBJECTS): | $(CFGFILETEST_DEPENDS)

$(CFGFILETEST_OBJ_DIR)/%.o: $(CFGFILETEST_SRC_DIR)/%.c
	@mkdir -p $(dir $@)
	$(strip $(ICP_CC) -o $@ -c $(ICP_CPPFLAGS) $(ICP_CFLAGS) $(ICP_COPTS) $(ICP_DEFINES) $<)

$(CFGFILETEST_OBJ_DIR)/%.o: $(CFGFILETEST_SRC_DIR)/%.cpp
	@mkdir -p $(dir $@)
	$(strip $(ICP_CXX) -o $@ -c $(ICP_CPPFLAGS) $(ICP_CXXFLAGS) $(ICP_COPTS) $(ICP_DEFINES) $<)

$(CFGFILETEST_TARGET): $(CFGFILETEST_OBJECTS)
	@mkdir -p $(dir $@)
	$(strip $(ICP_AR) $(ICP_ARFLAGS) $@ $(CFGFILETEST_OBJECTS))

.PHONY: config_file_test
config_file_test: $(CFGFILETEST_TARGET)

.PHONY: clean_config_file_test
clean_config_file_test:
	@rm -rf $(CFGFILETEST_OBJ_DIR) $(CFGFILETEST_TARGET)
clean: clean_config_file_test
