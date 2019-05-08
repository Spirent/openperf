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
CFGFILE_OBJECTS :=
CFGFILE_DEPENDS :=
CFGFILE_LDLIBS  :=

include $(CFGFILE_SRC_DIR)/module.mk

CFGFILE_OBJECTS += $(patsubst %, $(CFGFILE_OBJ_DIR)/%, \
	$(patsubst %.c, %.o, $(filter %.c, $(CFGFILE_SOURCES))))

CFGFILE_OBJECTS += $(patsubst %, $(CFGFILE_OBJ_DIR)/%, \
	$(patsubst %.cpp, %.o, $(filter %.cpp, $(CFGFILE_SOURCES))))

CFGFILE_LIBRARY := icp_config_file
CFGFILE_TARGET := $(CFGFILE_LIB_DIR)/lib$(CFGFILE_LIBRARY).a

-include $(CFGFILE_OBJECTS:.o=.d)

ICP_INC_DIRS += $(CFGFILE_INC_DIR)
ICP_LIB_DIRS += $(CFGFILE_LIB_DIR)
ICP_LDLIBS += -Wl,--whole-archive -l$(CFGFILE_LIBRARY) -Wl,--no-whole-archive $(CFGFILE_LDLIBS)

# Load external dependencies, too
$(foreach DEP, $(CFGFILE_DEPENDS), $(eval include $(ICP_ROOT)/mk/$(DEP).mk))

###
# Build rules
###
$(CFGFILE_OBJECTS): | $(CFGFILE_DEPENDS)

$(CFGFILE_OBJ_DIR)/%.o: $(CFGFILE_SRC_DIR)/%.c
	@mkdir -p $(dir $@)
	$(strip $(ICP_CC) -o $@ -c $(ICP_CPPFLAGS) $(ICP_CFLAGS) $(ICP_COPTS) $(ICP_DEFINES) $<)

$(CFGFILE_OBJ_DIR)/%.o: $(CFGFILE_SRC_DIR)/%.cpp
	@mkdir -p $(dir $@)
	$(strip $(ICP_CXX) -o $@ -c $(ICP_CPPFLAGS) $(ICP_CXXFLAGS) $(ICP_COPTS) $(ICP_DEFINES) $<)

$(CFGFILE_TARGET): $(CFGFILE_OBJECTS)
	@mkdir -p $(dir $@)
	$(strip $(ICP_AR) $(ICP_ARFLAGS) $@ $(CFGFILE_OBJECTS))

.PHONY: config_file
config_file: $(CFGFILE_TARGET)

.PHONY: clean_config_file
clean_config_file:
	@rm -rf $(CFGFILE_OBJ_DIR) $(CFGFILE_TARGET)
clean: clean_config_file
